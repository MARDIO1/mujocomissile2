#include "USB.hpp"
#include <cstdio>
#include <cstring>

// ================================================================
// 模块二实现：底层串口收发
// ================================================================

USB::USB(const std::string& port, uint32_t baudrate, size_t buf_size)
    : port_(port)
    , baudrate_(baudrate)
    , handle_(INVALID_HANDLE_VALUE)
    , running_(false)
    , buf_size_(buf_size)
    , head_(0)
    , tail_(0)
{
    // 超过 COM9 的端口需要加 "\\.\" 前缀
    if (port_.find("\\\\.\\") == std::string::npos && port_.size() > 4) {
        port_ = "\\\\.\\" + port_;
    }
    ring_buf_.resize(buf_size_, 0);
}

USB::~USB() {
    close();
}

bool USB::open() {
    if (isOpen()) {
        last_error_ = "Already open";
        return false;
    }

    handle_ = CreateFileA(
        port_.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (handle_ == INVALID_HANDLE_VALUE) {
        last_error_ = "CreateFile failed, port: " + port_;
        return false;
    }

    SetupComm(handle_, 4096, 4096);

    DCB dcb = {};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(handle_, &dcb)) {
        last_error_ = "GetCommState failed";
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
        return false;
    }
    dcb.BaudRate        = baudrate_;
    dcb.ByteSize        = 8;
    dcb.Parity          = NOPARITY;
    dcb.StopBits        = ONESTOPBIT;
    dcb.fBinary         = TRUE;
    dcb.fParity         = FALSE;
    dcb.fOutxCtsFlow    = FALSE;
    dcb.fOutxDsrFlow    = FALSE;
    dcb.fDtrControl     = DTR_CONTROL_DISABLE;
    dcb.fRtsControl     = RTS_CONTROL_DISABLE;

    if (!SetCommState(handle_, &dcb)) {
        last_error_ = "SetCommState failed";
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
        return false;
    }

    // 接收超时：每字节最多 10ms，总超时 50ms（让后台线程不永久阻塞）
    // 写超时设为 0：立即返回，不阻塞仿真主循环
    COMMTIMEOUTS timeouts = {};
    timeouts.ReadIntervalTimeout         = 10;
    timeouts.ReadTotalTimeoutMultiplier  = 1;
    timeouts.ReadTotalTimeoutConstant    = 50;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant   = 0;
    SetCommTimeouts(handle_, &timeouts);

    PurgeComm(handle_, PURGE_RXCLEAR | PURGE_TXCLEAR);

    running_ = true;
    recv_thread_ = std::thread(&USB::recvThread, this);

    printf("[USB] Opened %s @ %u baud\n", port_.c_str(), baudrate_);
    return true;
}

void USB::close() {
    if (!isOpen()) return;

    running_ = false;
    if (recv_thread_.joinable()) {
        recv_thread_.join();
    }

    CloseHandle(handle_);
    handle_ = INVALID_HANDLE_VALUE;

    printf("[USB] Closed\n");
}

bool USB::isOpen() const {
    return handle_ != INVALID_HANDLE_VALUE;
}

int USB::send(const uint8_t* data, size_t len) {
    if (!isOpen() || data == nullptr || len == 0) return -1;

    DWORD written = 0;
    if (!WriteFile(handle_, data, static_cast<DWORD>(len), &written, nullptr)) {
        last_error_ = "WriteFile failed";
        return -1;
    }
    return static_cast<int>(written);
}

int USB::recv(uint8_t* buf, size_t len) {
    if (buf == nullptr || len == 0) return 0;

    std::lock_guard<std::mutex> lock(buf_mutex_);
    size_t count = 0;
    while (count < len && head_ != tail_) {
        buf[count++] = ring_buf_[head_];
        head_ = (head_ + 1) % buf_size_;
    }
    return static_cast<int>(count);
}

void USB::flush() {
    std::lock_guard<std::mutex> lock(buf_mutex_);
    head_ = tail_ = 0;
    if (isOpen()) PurgeComm(handle_, PURGE_RXCLEAR);
}

size_t USB::available() const {
    std::lock_guard<std::mutex> lock(buf_mutex_);
    return (tail_ + buf_size_ - head_) % buf_size_;
}

std::string USB::lastError() const {
    return last_error_;
}

// 后台接收线程：持续读取串口原始字节，写入环形缓冲区
void USB::recvThread() {
    const size_t READ_SIZE = 256;
    uint8_t tmp[READ_SIZE];

    while (running_) {
        if (!isOpen()) break;

        DWORD bytes_read = 0;
        BOOL ok = ReadFile(handle_, tmp, READ_SIZE, &bytes_read, nullptr);

        if (!ok) {
            last_error_ = "ReadFile failed in recv thread";
            running_ = false;
            break;
        }
        if (bytes_read == 0) continue; // 超时，无数据

        std::lock_guard<std::mutex> lock(buf_mutex_);
        for (DWORD i = 0; i < bytes_read; i++) {
            size_t next_tail = (tail_ + 1) % buf_size_;
            if (next_tail == head_) {
                // 缓冲区满，丢弃最旧数据
                head_ = (head_ + 1) % buf_size_;
            }
            ring_buf_[tail_] = tmp[i];
            tail_ = (tail_ + 1) % buf_size_;
        }
    }
}


// ================================================================
// 模块三实现：HIL 协议层
//   基于底层 send/recv，实现结构体级别的帧收发
// ================================================================

/**
 * hil_send：打包 SimulationFromPC_t 并发送给 STM32
 * 结构体已含帧头帧尾，直接 memcpy 为字节流发送
 */
bool USB::hil_send(const SimulationFromPC_t& data) {
    // 确保帧头帧尾正确（防止调用方忘记设置）
    SimulationFromPC_t frame = data;
    frame.head = HIL_FROM_PC_HEAD;
    frame.tail = HIL_FROM_PC_TAIL;

    int ret = send(reinterpret_cast<const uint8_t*>(&frame), sizeof(SimulationFromPC_t));
    return ret == static_cast<int>(sizeof(SimulationFromPC_t));
}

/**
 * hil_recv：从接收缓冲区中搜索并解析一帧 SimulationToPC_t
 *
 * 解析策略：
 *   1. 在缓冲区中搜索帧头 0xAA
 *   2. 检查从帧头开始是否有足够字节（sizeof(SimulationToPC_t)）
 *   3. 检查帧尾是否为 0xBB
 *   4. 通过则 memcpy 到 out，丢弃该帧之前的所有字节
 *   5. 不通过则跳过该帧头，继续搜索下一个
 */
bool USB::hil_recv(SimulationToPC_t& out) {
    const size_t FRAME_SIZE = sizeof(SimulationToPC_t);

    std::lock_guard<std::mutex> lock(buf_mutex_);

    // 计算缓冲区中可用字节数
    size_t avail = (tail_ + buf_size_ - head_) % buf_size_;

    while (avail >= FRAME_SIZE) {
        // 检查当前 head 是否是帧头
        if (ring_buf_[head_] != HIL_TO_PC_HEAD) {
            // 不是帧头，丢弃该字节，继续搜索
            head_ = (head_ + 1) % buf_size_;
            avail--;
            continue;
        }

        // 找到帧头，检查帧尾位置
        size_t tail_pos = (head_ + FRAME_SIZE - 1) % buf_size_;
        if (ring_buf_[tail_pos] != HIL_TO_PC_TAIL) {
            // 帧尾不对，跳过这个帧头，继续搜索
            head_ = (head_ + 1) % buf_size_;
            avail--;
            continue;
        }

        // 帧头帧尾都对，拷贝整帧数据
        uint8_t frame_buf[sizeof(SimulationToPC_t)];
        for (size_t i = 0; i < FRAME_SIZE; i++) {
            frame_buf[i] = ring_buf_[(head_ + i) % buf_size_];
        }
        memcpy(&out, frame_buf, FRAME_SIZE);

        // 消耗掉这一帧
        head_ = (head_ + FRAME_SIZE) % buf_size_;
        return true;
    }

    return false; // 没有完整帧
}
