cmake_minimum_required(VERSION 3.16)# 要求用户电脑上的 CMake 版本不得低于 3.16
project(MujocoUAV)#项目名称
set(CMAKE_CXX_STANDARD 17)#c++17标准
set(MUJOCO_DIR "T:/ROBOMASTER_2/tool/mujoco210")#mujoco路径
# 1. 引入 FetchContent 来自动获取 GLFW
include(FetchContent)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.8
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)
# 2. 包含 MuJoCo 头文件
include_directories("${MUJOCO_DIR}/include")
# 3. 查找 MuJoCo 库 
set(MUJOCO_LIB "${MUJOCO_DIR}/lib/mujoco.lib") 
# 4. 定义可执行文件
add_executable(uav_sim src/main.cpp)
# 5. 链接所有库
# glfw 来自 FetchContent，opengl32 是 Windows 自带
target_link_libraries(uav_sim PRIVATE ${MUJOCO_LIB} glfw opengl32)
# 6. 复制 MuJoCo 的 DLL 到运行目录
add_custom_command(TARGET uav_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${MUJOCO_DIR}/bin/mujoco.dll"
    $<TARGET_FILE_DIR:uav_sim>)