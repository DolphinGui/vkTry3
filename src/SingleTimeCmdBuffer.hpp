#ifndef SINGLETIMECMDBUFFER_H_INCLUDE
#define SINGLETIMECMDBUFFER_H_INCLUDE

#include <vulkan/vulkan.hpp>
namespace vcc{
class SingleTimeCmdBuffer{
public:
    vk::CommandBuffer cmd;
    SingleTimeCmdBuffer(vk::Device* dev, vk::Queue* graphics, vk::CommandPool* pool);
    ~SingleTimeCmdBuffer();
    void submit();
private:
    vk::Device* device;
    vk::Queue* graphicsQueue;
    vk::CommandPool* cmdPool;
};
}
#endif