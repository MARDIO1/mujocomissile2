// 这个是mujoco的模板代码修改的，
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <GLFW/glfw3.h>
#include <mujoco/mujoco.h>

#include "world.hpp"
#include "player.hpp"
#include "plane.hpp"
#include "USB.hpp"

// 主主主主主函数！
// 相对路径：exe 在 build/Debug/ 或 build/Release/，model 在项目根/model/
const char path[200] = "../../model/all.xml";

// ----------------------------------------------------------------
// 串口配置（根据实际 COM 口修改）
// ----------------------------------------------------------------
const char* SERIAL_PORT = "COM10";
const uint32_t SERIAL_BAUD = 500000;

int main(int argc, const char **argv)
{
    World World1(path);
    Player Player1(World1, "render");
    Plane Plane1(0); // 绑定世界实体

    // ----------------------------------------------------------------
    // 初始化串口（失败时仿真照常运行，不强制退出）
    // ----------------------------------------------------------------
    USB usb(SERIAL_PORT, SERIAL_BAUD);
    bool usb_ok = usb.open();
    if (!usb_ok) {
        printf("[HIL] 串口 %s 打开失败：%s\n", SERIAL_PORT, usb.lastError().c_str());
        printf("[HIL] 仿真将在无串口模式下运行\n");
    } else {
        printf("[HIL] 串口已连接，开始 HIL 仿真\n");
    }

    // 用于接收控制帧的变量
    SimulationToPC_t ctrl_frame = {};
    // 用于发送 IMU 帧的变量
    SimulationFromPC_t imu_frame = {};
    // 发送帧率限制计数器（每 3 帧发一次，约 20Hz，减少 WriteFile 调用）
    int send_counter = 0;

    while (!glfwWindowShouldClose(Player1.window))
    {
        mjtNum simstart = World1.d->time;

        // --------------------------------------------------------
        // [HIL 接收] 从串口读取 STM32 发来的控制帧，收到立即打印
        // --------------------------------------------------------
        if (usb_ok && usb.hil_recv(ctrl_frame)) {
            printf("[HIL RX] fan_pwm=%.1f  surfaces=[%.2f, %.2f, %.2f, %.2f] deg\n",
                ctrl_frame.fan_pwm,
                ctrl_frame.surface_angle_d[0],
                ctrl_frame.surface_angle_d[1],
                ctrl_frame.surface_angle_d[2],
                ctrl_frame.surface_angle_d[3]);
            // TODO: 用 ctrl_frame.surface_angle_d 驱动 MuJoCo 舵面 actuator
        }

        // 真正重要的仿真
        while (World1.d->time - simstart < 1.0 / 60.0)
        {
            /*这里是自定义空气动力学运算部分*/
            mj_step(World1.m, World1.d);
        }

        // --------------------------------------------------------
        // [HIL 发送] 每 3 帧发一次（约 20Hz），减少 WriteFile 调用频率
        // --------------------------------------------------------
        if (usb_ok && (++send_counter >= 3)) {
            send_counter = 0;
            // TODO: 替换为 World1.d->sensordata[...] 的真实加速度/角速度
            imu_frame.acc_mps2[0]        = 0.0f;
            imu_frame.acc_mps2[1]        = 0.0f;
            imu_frame.acc_mps2[2]        = -9.81f;
            imu_frame.gyro_body_radps[0] = 0.0f;
            imu_frame.gyro_body_radps[1] = 0.0f;
            imu_frame.gyro_body_radps[2] = 0.0f;
            usb.hil_send(imu_frame);
        }

        // 后面都是渲染
        mjrRect viewport = {0, 0, 0, 0};
        glfwGetFramebufferSize(Player1.window, &viewport.width, &viewport.height);

        mjv_updateScene(World1.m, World1.d, &Player1.opt, NULL, &Player1.cam, mjCAT_ALL, &Player1.scn);
        mjr_render(viewport, &Player1.scn, &Player1.con);

        glfwSwapBuffers(Player1.window);
        glfwPollEvents();
    }

    return EXIT_SUCCESS;
}
