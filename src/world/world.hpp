#ifndef WORLD_HPP
#define WORLD_HPP
#include <mujoco/mujoco.h>

class World {
public:
// MuJoCo data structures
    mjModel* m = NULL;                  // MuJoCo model一个巨型结构体
    mjData* d = NULL;                   // MuJoCo data也是一个巨型结构体

    World(const char* model_path);
    
    ~World();

    void step(double seconds = 1.0/60.0);
};
#endif