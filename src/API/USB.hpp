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

/**
 * USB 串口通信类（Windows Win32 API）
 * 
 * 特性：
 *   - 后台接收线程，不阻塞仿真主循环
 *   - 环形缓冲区存储接收数据
 *   - 线程安全的 send / recv 接口
 * 
 * 使用示例：
 *   USB usb("COM3", 115200);
 *   usb.open();
 *   usb.send(buf, len);
 *   int n = usb.recv(buf, 64);  // 非阻塞，返回实际读取字节数
 *   usb.close();
 */
class USB {
public:
    /**
     * 构造函数
     * @param port      串口名称，如 "COM3"（超过COM9需写 "\\\\.\\COM10"）
     * @param baudrate  波特率，如 115200、9600
     * @param buf_size  接收环形缓冲区大小（字节），默认 4096
     */
    USB(const std::string& port, uint32_t baudrate, size_t buf_size = 4096);
    ~USB();

    // 禁止拷贝
    USB(const USB&) = delete;
    USB& operator=(const USB&) = delete;

    /**
     * 打开串口，启动后台接收线程
     * @return 成功返回 true
     */
    bool open();

    /**
     * 关闭串口，停止后台接收线程
     */
    void close();

    /**
     * 串口是否已打开
     */
    bool isOpen() const;

    /**
     * 发送数据（阻塞直到发送完毕）
     * @param data  数据指针
     * @param len   数据长度
     * @return 实际发送字节数，-1 表示失败
     */
    int send(const uint8_t* data, size_t len);

    /**
     * 从接收缓冲区读取数据（非阻塞）
     * @param buf   目标缓冲区
     * @param len   最多读取字节数
     * @return 实际读取字节数（0 表示暂无数据）
     */
    int recv(uint8_t* buf, size_t len);

    /**
     * 清空接收缓冲区
     */
    void flush();

    /**
     * 返回接收缓冲区中当前可读字节数
     */
    size_t available() const;

    /**
     * 获取上次错误描述
     */
    std::string lastError() const;

private:
    // 后台接收线程函数
    void recvThread();

    std::string     port_;          // 串口名
    uint32_t        baudrate_;      // 波特率
    HANDLE          handle_;        // Win32 串口句柄
    std::atomic<bool> running_;     // 接收线程运行标志
    std::thread     recv_thread_;   // 后台接收线程

    // 环形缓冲区
    std::vector<uint8_t>    ring_buf_;
    size_t                  buf_size_;
    size_t                  head_;  // 读指针
    size_t                  tail_;  // 写指针
    mutable std::mutex      buf_mutex_;

    std::string last_error_;
};

#endif // USB_HPP
