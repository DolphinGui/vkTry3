#ifndef DOER_H_INCLUDE
#define DOER_H_IN
#include <bits/stdint-uintn.h>
#include <vector>
#include <thread>

#include "VCEngine.hpp"
#include "vulkan/vulkan.hpp"

namespace vcc{
class Doer{

public:
Doer(Setup* set, vk::Device* dev, uint32_t graphicsIndex, uint32_t poolCount, uint32_t bufferCount);
~Doer();
void record(uint32_t poolIndex, uint32_t bufferIndex, void (*funct)(vk::CommandBuffer c));
void start();
private:
std::vector<
    std::pair<
        vk::CommandPool, 
        std::vector<vk::CommandBuffer>
    >
> commands;
vk::Device* dev;
vcc::Setup* set;
std::thread thread;
std::vector<void (*)(vk::CommandBuffer)> jobs;

void submit(uint32_t poolIndex, vk::SubmitInfo info);
};
}
#endif