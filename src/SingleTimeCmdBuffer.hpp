#ifndef SINGLETIMECMDBUFFER_H_INCLUDE
#define SINGLETIMECMDBUFFER_H_INCLUDE

#include <vulkan/vulkan.hpp>
namespace vcc{
class SingleTimeCmdBuffer{
public:
    const vk::CommandBuffer cmd;
    SingleTimeCmdBuffer(const vk::Device* const dev, const vk::Queue* const graphics, const vk::CommandPool* const pool);
    ~SingleTimeCmdBuffer();
    void submit();
private:
    const vk::Device* device;
    const vk::Queue* graphicsQueue;
    const vk::CommandPool* cmdPool;
};
}
#endif