#pragma once
#ifndef USB_HPP
#define USB_HPP

#include <windows.h>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <cstdint>

// ================================================================
// 模块一：HIL 通信协议结构体
//   与 STM32 端 SimulationTask.c 中的结构体完全对应
//   必须使用 #pragma pack(1) 保证内存布局一致（无填充字节）
// ================================================================
#pragma pack(1)

/**
 * STM32 -> PC（仿真接收）
 * STM32 以 50Hz 频率发送控制量
 */
struct SimulationToPC_t {
    uint8_t head;               // 帧头 0xAA
    float   fan_pwm;            // 风扇 PWM 控制量
    float   surface_angle_d[4]; // 四个舵面偏转角（度）[FR, FL, BL, BR]
    uint8_t tail;               // 帧尾 0xBB
};

/**
 * PC（仿真）-> STM32（仿真发送）
 * 仿真每步后将 IMU 数据发回 STM32
 */
struct SimulationFromPC_t {
    uint8_t head;               // 帧头 0xCC
    float   acc_mps2[3];        // 机体系加速度 (m/s²) [x, y, z]
    float   gyro_body_radps[3]; // 机体系角速度 (rad/s) [roll_p, pitch_q, yaw_r]
    uint8_t tail;               // 帧尾 0xDD
};

#pragma pack()

// 帧头帧尾常量
static constexpr uint8_t HIL_TO_PC_HEAD   = 0xAA;
static constexpr uint8_t HIL_TO_PC_TAIL   = 0xBB;
static constexpr uint8_t HIL_FROM_PC_HEAD = 0xCC;
static constexpr uint8_t HIL_FROM_PC_TAIL = 0xDD;


// ================================================================
// 模块二：底层串口收发类（Win32 API）
//   - 后台接收线程，不阻塞仿真主循环
//   - 环形缓冲区存储原始字节
//   - 线程安全的 send / recv 接口
// ================================================================

/**
 * USB 串口通信类
 *
 * 使用示例：
 *   USB usb("COM3", 115200);
 *   usb.open();
 *   usb.send(buf, len);
 *   int n = usb.recv(buf, 64);  // 非阻塞
 *   usb.close();
 */
class USB {
public:
    /**
     * @param port      串口名称，如 "COM3"（超过COM9需写 "\\\\.\\COM10"）
     * @param baudrate  波特率，如 115200
     * @param buf_size  接收环形缓冲区大小（字节），默认 4096
     */
    USB(const std::string& port, uint32_t baudrate, size_t buf_size = 4096);
    ~USB();

    USB(const USB&) = delete;
    USB& operator=(const USB&) = delete;

    bool open();
    void close();
    bool isOpen() const;

    /**
     * 发送原始字节（阻塞）
     * @return 实际发送字节数，-1 失败
     */
    int send(const uint8_t* data, size_t len);

    /**
     * 从接收缓冲区读取原始字节（非阻塞）
     * @return 实际读取字节数（0 表示暂无数据）
     */
    int recv(uint8_t* buf, size_t len);

    void   flush();
    size_t available() const;
    std::string lastError() const;

    // ============================================================
    // 模块三：HIL 协议层接口
    //   基于底层 send/recv，实现结构体级别的收发
    // ============================================================

    /**
     * 发送仿真数据给 STM32（打包 SimulationFromPC_t）
     * @return true 发送成功
     */
    bool hil_send(const SimulationFromPC_t& data);

    /**
     * 尝试从接收缓冲区解析一帧 SimulationToPC_t（非阻塞）
     * 会在缓冲区中搜索帧头 0xAA，找到完整帧后解析
     * @param out  解析结果输出
     * @return true 成功解析到一帧
     */
    bool hil_recv(SimulationToPC_t& out);

private:
    void recvThread();

    std::string      port_;
    uint32_t         baudrate_;
    HANDLE           handle_;
    std::atomic<bool> running_;
    std::thread      recv_thread_;

    std::vector<uint8_t> ring_buf_;
    size_t               buf_size_;
    size_t               head_;
    size_t               tail_;
    mutable std::mutex   buf_mutex_;

    std::string last_error_;
};

#endif // USB_HPP
