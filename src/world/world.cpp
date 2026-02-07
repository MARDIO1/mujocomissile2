#include "SimWorld.hpp"
#include <cstdio>

World::World(const char* model_path) {
    /*  // load and compile model 根据文件扩展名自动选择加载 MJCF 模型的方式
  char error[1000] = "Could not load binary model";
  if (std::strlen(argv[1])>4 && !std::strcmp(argv[1]+std::strlen(argv[1])-4, ".mjb")) {
    m = mj_loadModel(argv[1], 0);
  } else {
    m = mj_loadXML(argv[1], 0, error, 1000);
  }
  if (!m) {
    mju_error("Load model error: %s", error);
  }

  //数据实例化
  d = mj_makeData(m);
  */
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
    /*世界主步进*/
    if (!m || !d) return;   
    mjtNum start_sim_time = d->time;
    // 循环步进直到达到指定的时间间隔（例如1/60秒）
    while (d->time - start_sim_time < seconds) {
        mj_step(m, d);
    }
}