#include "Setup.hpp"
#include "VCEngine.hpp"

int
main()
{
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(800, 600, "vkTest", nullptr, nullptr);
  vcc::VCEngine app(800, 600, "vkTest", window);
  vcc::Setup setup(&app);

  try {
    app.run(&setup);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
