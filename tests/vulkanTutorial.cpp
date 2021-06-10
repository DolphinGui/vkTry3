#include "Setup.hpp"
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
  vcc::Setup setup(&app);
  
  return EXIT_SUCCESS;
}
