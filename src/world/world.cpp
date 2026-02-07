#include "world.hpp"
#include <cstdio>

World::World(const char* model_path) {
    char error[1000] = "Could not load binary model";
    m = mj_loadXML(model_path, nullptr, error, 1000);
    if (!m) {
        mju_error("从xml加载模型失败，检查你的语法和调用: %s", error);
    }
    d = mj_makeData(m);
}
World::~World() {
    if (d) mj_deleteData(d);
    if (m) mj_deleteModel(m);
    
}

void World::step(double seconds) {
    if (!m || !d) return;   
    mjtNum start_sim_time = d->time;
    while (d->time - start_sim_time < seconds) {
        mj_step(m, d);
    }
}