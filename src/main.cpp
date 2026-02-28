// 这个是mujoco的模板代码修改的，
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <GLFW/glfw3.h>
#include <mujoco/mujoco.h>

#include "world.hpp"
#include "player.hpp"
#include "plane.hpp"
// 主主主主主函数！
// 相对路径：exe 在 build/Debug/ 或 build/Release/，model 在项目根/model/
const char path[200] = "../../model/all.xml";
int main(int argc, const char **argv)
{
    World World1(path);
    Player Player1(World1, "render");
    Plane Plane1(0); // 绑定世界实体
    
    while (!glfwWindowShouldClose(Player1.window))
    {

        mjtNum simstart = World1.d->time;
        // 真正重要的仿真
        while (World1.d->time - simstart < 1.0 / 60.0)
        {
            /*这里是自定义空气动力学运算部分*/

            mj_step(World1.m, World1.d);
        }
        // 后面都是渲染
        //  get framebuffer viewport
        mjrRect viewport = {0, 0, 0, 0};
        glfwGetFramebufferSize(Player1.window, &viewport.width, &viewport.height);

        // update scene and render
        mjv_updateScene(World1.m, World1.d, &Player1.opt, NULL, &Player1.cam, mjCAT_ALL, &Player1.scn);
        mjr_render(viewport, &Player1.scn, &Player1.con);

        // swap OpenGL buffers (blocking call due to v-sync)
        glfwSwapBuffers(Player1.window);

        // process pending GUI events, call GLFW callbacks
        glfwPollEvents();
    }

    return EXIT_SUCCESS;
}
