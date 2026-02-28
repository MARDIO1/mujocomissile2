cmake_minimum_required(VERSION 3.16)# 要求用户电脑上的 CMake 版本不得低于 3.16
project(MujocoUAV)#项目名称
set(CMAKE_CXX_STANDARD 17)#c++17标准

# mujoco210 路径（使用项目内相对路径）
set(MUJOCO_DIR "${CMAKE_SOURCE_DIR}/mujoco/mujoco210-windows-x86_64/mujoco210")

# mujoco210 自带 GLFW，直接使用，不需要 FetchContent
include_directories("${MUJOCO_DIR}/include")
set(MUJOCO_LIB "${MUJOCO_DIR}/bin/mujoco210.lib")
set(GLFW_LIB   "${MUJOCO_DIR}/bin/glfw3.lib")

include_directories("${CMAKE_SOURCE_DIR}/src/world"
                    "${CMAKE_SOURCE_DIR}/src/player"
                    "${CMAKE_SOURCE_DIR}/src"
                    "${CMAKE_SOURCE_DIR}/src/plane"
                    "${CMAKE_SOURCE_DIR}/src/API")

add_executable(uav_sim
                      src/main.cpp
                      src/world/world.cpp
                      src/player/player.cpp
                      src/plane/plane.cpp
                      src/API/USB.cpp
)

# 链接 mujoco210、glfw3、opengl32（Windows 自带）
target_link_libraries(uav_sim PRIVATE ${MUJOCO_LIB} ${GLFW_LIB} opengl32)

# 复制 mujoco210.dll 和 glfw3.dll 到运行目录
add_custom_command(TARGET uav_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MUJOCO_DIR}/bin/mujoco210.dll"
        $<TARGET_FILE_DIR:uav_sim>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MUJOCO_DIR}/bin/glfw3.dll"
        $<TARGET_FILE_DIR:uav_sim>
)
