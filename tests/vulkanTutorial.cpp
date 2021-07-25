#define VMA_IMPLEMENTATION
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "Setup.hpp"
#include "Task.hpp"
#include "VCEngine.hpp"

int
main()
{
  glfwInit();
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(800, 600, "vkTest", nullptr, nullptr);
  vcc::VCEngine app(800, 600, "vkTest", window);
  vcc::Setup setup(app);
  vcc::Task task(setup, app);
  
  return EXIT_SUCCESS;
}
