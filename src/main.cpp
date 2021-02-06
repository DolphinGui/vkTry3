#include "VCEngine.hpp"

int main() {
  vcc::VCEngine app(800,600, "vkTest");

  try {
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
