#include "USB.hpp"
#include <cstdio>
#include <cstring>

// ----------------------------------------------------------------
// 构造 / 析构
// ----------------------------------------------------------------

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

// ----------------------------------------------------------------
// open / close
// ----------------------------------------------------------------

bool USB::open() {
    if (isOpen()) {
        last_error_ = "Already open";
        return false;
    }

    // 打开串口
    handle_ = CreateFileA(
        port_.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,              // 不共享
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (handle_ == INVALID_HANDLE_VALUE) {
        last_error_ = "CreateFile failed, port: " + port_;
        return false;
    }

    // 配置缓冲区
    SetupComm(handle_, 4096, 4096);

    // 配置串口参数
    DCB dcb = {};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(handle_, &dcb)) {
        last_error_ = "GetCommState failed";
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
        return false;
    }
    dcb.BaudRate = baudrate_;
    dcb.ByteSize = 8;           // 8位数据
    dcb.Parity   = NOPARITY;    // 无校验
    dcb.StopBits = ONESTOPBIT;  // 1位停止位
    dcb.fBinary  = TRUE;
    dcb.fParity  = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl  = DTR_CONTROL_DISABLE;
    dcb.fRtsControl  = RTS_CONTROL_DISABLE;

    if (!SetCommState(handle_, &dcb)) {
        last_error_ = "SetCommState failed";
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
        return false;
    }

    // 配置超时：接收每字节最多等待 10ms，总超时 50ms
    // 这样后台线程 ReadFile 不会永久阻塞
    COMMTIMEOUTS timeouts = {};
    timeouts.ReadIntervalTimeout         = 10;
    timeouts.ReadTotalTimeoutMultiplier  = 1;
    timeouts.ReadTotalTimeoutConstant    = 50;
    timeouts.WriteTotalTimeoutMultiplier = 1;
    timeouts.WriteTotalTimeoutConstant   = 50;
    SetCommTimeouts(handle_, &timeouts);

    // 清空串口缓冲
    PurgeComm(handle_, PURGE_RXCLEAR | PURGE_TXCLEAR);

    // 启动后台接收线程
    running_ = true;
    recv_thread_ = std::thread(&USB::recvThread, this);

    printf("[USB] Opened %s @ %u baud\n", port_.c_str(), baudrate_);
    return true;
}

void USB::close() {
    if (!isOpen()) return;

    // 停止接收线程
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

// ----------------------------------------------------------------
// send
// ----------------------------------------------------------------

int USB::send(const uint8_t* data, size_t len) {
    if (!isOpen() || data == nullptr || len == 0) return -1;

    DWORD written = 0;
    if (!WriteFile(handle_, data, static_cast<DWORD>(len), &written, nullptr)) {
        last_error_ = "WriteFile failed";
        return -1;
    }
    return static_cast<int>(written);
}

// ----------------------------------------------------------------
// recv（从环形缓冲区读，非阻塞）
// ----------------------------------------------------------------

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

// ----------------------------------------------------------------
// flush / available
// ----------------------------------------------------------------

void USB::flush() {
    std::lock_guard<std::mutex> lock(buf_mutex_);
    head_ = tail_ = 0;
    if (isOpen()) {
        PurgeComm(handle_, PURGE_RXCLEAR);
    }
}

size_t USB::available() const {
    std::lock_guard<std::mutex> lock(buf_mutex_);
    return (tail_ + buf_size_ - head_) % buf_size_;
}

// ----------------------------------------------------------------
// lastError
// ----------------------------------------------------------------

std::string USB::lastError() const {
    return last_error_;
}

// ----------------------------------------------------------------
// 后台接收线程
// ----------------------------------------------------------------

void USB::recvThread() {
    const size_t READ_SIZE = 256;
    uint8_t tmp[READ_SIZE];

    while (running_) {
        if (!isOpen()) break;

        DWORD bytes_read = 0;
        BOOL ok = ReadFile(handle_, tmp, READ_SIZE, &bytes_read, nullptr);

        if (!ok) {
            // 串口异常断开
            last_error_ = "ReadFile failed in recv thread";
            running_ = false;
            break;
        }

        if (bytes_read == 0) {
            // 超时，没有数据，继续等
            continue;
        }

        // 写入环形缓冲区
        std::lock_guard<std::mutex> lock(buf_mutex_);
        for (DWORD i = 0; i < bytes_read; i++) {
            size_t next_tail = (tail_ + 1) % buf_size_;
            if (next_tail == head_) {
                // 缓冲区满，丢弃最旧的数据（移动读指针）
                head_ = (head_ + 1) % buf_size_;
            }
            ring_buf_[tail_] = tmp[i];
            tail_ = (tail_ + 1) % buf_size_;
        }
    }
}
