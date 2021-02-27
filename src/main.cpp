#include "VCEngine.hpp"
#include "Setup.hpp"

int main() {
  vcc::VCEngine app(800,600, "vkTest");
  vcc::Setup setup(&app);
  try {
    app.run(&setup);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
