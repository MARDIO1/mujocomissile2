#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <GLFW/glfw3.h>
#include <mujoco/mujoco.h>
#include "World.hpp"
#include <string>

class Player {
public:
    GLFWwindow* window = nullptr;
    
    // MuJoCo 渲染四件套
    mjvCamera cam;                      // abstract camera相机结构体
    mjvOption opt;                      // visualization options
    mjvScene scn;                       // abstract scene
    mjrContext con;                     // custom GPU context

    // 鼠标交互状态
    bool button_left = false;
    bool button_middle = false;
    bool button_right =  false;
    double lastx = 0;
    double lasty = 0;

    World& world; // 引用对应的物理世界
    std::string name;
    Player(World& world, std::string title);
    ~Player();


};

#endif