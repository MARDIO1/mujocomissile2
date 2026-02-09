//这个是mujoco的模板代码修改的，
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <GLFW/glfw3.h>
#include <mujoco/mujoco.h>

#include "world.hpp"
#include "player.hpp"
//主主主主主函数！
const char path[100] = "T:\\ROBOMASTER_2\\Project\\mujocomissile\\model\\all.xml";
int main(int argc, const char** argv) {
	World World1(path);
	Player Player1(World1, "render");
    while (!glfwWindowShouldClose(Player1.window)) {
        // advance interactive simulation for 1/60 sec
        //  Assuming MuJoCo can simulate faster than real-time, which it usually can,
        //  this loop will finish on time for the next frame to be rendered at 60 fps.
        //  Otherwise add a cpu timer and exit this loop when it is time to render.
        mjtNum simstart = World1.d->time;
        while (World1.d->time - simstart < 1.0 / 60.0) {
            mj_step(World1.m, World1.d);
        }

        // get framebuffer viewport
        mjrRect viewport = { 0, 0, 0, 0 };
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
