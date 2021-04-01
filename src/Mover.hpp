#ifndef MOVER_H_INCLUDE
#define MOVER_H_INCLUDE
#include <vulkan/vulkan.hpp>

#include "Doer.hpp"
#include "VCEngine.hpp"

namespace vcc {
class Mover : vcc::Doer
{

public:
  Mover(vk::Queue& g, vk::Device& d, uint32_t graphicsIndex);
  ~Mover();

private:
  vk::CommandPool pool;
};
}
#endif