#include "player.hpp"
#include "world.hpp"
static void keyboard(GLFWwindow* window, int key, int scancode, int act, int mods);
static void mouse_button(GLFWwindow* window, int button, int act, int mods);
static void mouse_move(GLFWwindow* window, double xpos, double ypos);
static void scroll(GLFWwindow* window, double xoffset, double yoffset);
Player::Player(World& world, std::string title): world(world){
    //this->world=world;不能这样赋值
        //渲染Init
    if (!glfwInit()) {
        mju_error("Could not initialize GLFW");
    }
    // 创建窗口, make OpenGL context current, request v-sync
    this->window = glfwCreateWindow(1200, 900, title.c_str(), NULL, NULL);
    glfwMakeContextCurrent(this->window);
    glfwSwapInterval(1);

    //initialize visualization data structures
    mjv_defaultCamera(&this->cam);
    mjv_defaultOption(&this->opt);
    mjv_defaultScene(&this->scn);
    mjr_defaultContext(&this->con);

    // create scene and context
    mjv_makeScene(this->world.m, &this->scn, 2000);
    mjr_makeContext(this->world.m, &this->con, mjFONTSCALE_150);

    glfwSetWindowUserPointer(this->window, this);//添加的一个东西，贴标签，虽然C风格回调函数，但是还可以用this
    // install GLFW mouse and keyboard callbacks
    glfwSetKeyCallback(this->window, keyboard);
    glfwSetCursorPosCallback(this->window, mouse_move);
    glfwSetMouseButtonCallback(this->window, mouse_button);
    glfwSetScrollCallback(this->window, scroll);
    }
Player::~Player(){
    //free visualization storage
    mjv_freeScene(&this->scn);
    mjr_freeContext(&this->con);
    glfwTerminate();
    //glfwDestroyWindow();
}
//键盘事件回调
static void keyboard(GLFWwindow* window, int key, int scancode, int act, int mods) {
  // backspace: reset simulation
    Player* p = static_cast<Player*>(glfwGetWindowUserPointer(window));
        if (act == GLFW_PRESS && key == GLFW_KEY_BACKSPACE) {
            mj_resetData(p->world.m, p->world.d); 
            mj_forward(p->world.m, p->world.d);
        }
}
//鼠标按钮回调
static void mouse_button(GLFWwindow* window, int button, int act, int mods) {
  // update button state
  Player* p = static_cast<Player*>(glfwGetWindowUserPointer(window));

  p->button_left = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS);
  p->button_middle = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE)==GLFW_PRESS);
  p->button_right = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS);

  // update mouse position
  glfwGetCursorPos(window, &p->lastx, &p->lasty);
}
//鼠标移动回调
static void mouse_move(GLFWwindow* window, double xpos, double ypos) {
  // no buttons down: nothing to do
  Player* p = static_cast<Player*>(glfwGetWindowUserPointer(window));
  if (!p->button_left && !p->button_middle && !p->button_right) {
    return;
  }
  // compute mouse displacement, save
  double dx = xpos - p->lastx;
  double dy = ypos - p->lasty;
  p->lastx = xpos;
  p->lasty = ypos;
  // get current window size
  int width, height;
  glfwGetWindowSize(window, &width, &height);
  // get shift key state
  bool mod_shift = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS ||
                    glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT)==GLFW_PRESS);
  // determine action based on mouse button
  mjtMouse action;
  if (p->button_right) {
    action = mod_shift ? mjMOUSE_MOVE_H : mjMOUSE_MOVE_V;
  } else if (p->button_left) {
    action = mod_shift ? mjMOUSE_ROTATE_H : mjMOUSE_ROTATE_V;
  } else {
    action = mjMOUSE_ZOOM;
  }
  // move camera
  mjv_moveCamera(p->world.m, action, dx/height, dy/height, &p->scn, &p->cam);
}
//鼠标滚轮回调
static void scroll(GLFWwindow* window, double xoffset, double yoffset) {
  // emulate vertical mouse motion = 5% of window height
  Player* p = static_cast<Player*>(glfwGetWindowUserPointer(window));
  mjv_moveCamera(p->world.m, mjMOUSE_ZOOM, 0, -0.05*yoffset, &p->scn, &p->cam);
}