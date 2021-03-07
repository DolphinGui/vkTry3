#ifndef MOVER_H_INCLUDE
#define MOVER_H_INCLUDE
#include "VCEngine.hpp"
#include <vulkan/vulkan.hpp>
namespace vcc{
class Mover{

public:
Mover(VCEngine* e);
~Mover();

private:
vk::CommandPool pool;
};
}
#endif