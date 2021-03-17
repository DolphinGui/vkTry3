#ifndef DOER_H_INCLUDE
#define DOER_H_IN
#include <bits/stdint-uintn.h>
#include <queue>
#include <thread>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <mutex>
#include <condition_variable>

#include "VCEngine.hpp"
#include "jobs/PresentJob.hpp"
#include "vkobjects/CmdBuffer.hpp"

namespace vcc{
class Doer{

public:
Doer(Setup* set, vk::Device* dev, uint32_t graphicsIndex, uint32_t poolCount, uint32_t bufferCount);
~Doer();
void record(uint32_t poolIndex, uint32_t bufferIndex, void (*funct)(vk::CommandBuffer c));
void start();
private:
struct Frame{
    vk::CommandPool pool;
    std::vector<vcc::CmdBuffer> buffers;
    std::vector<vk::Semaphore> semaphores;
};
std::vector<
    Frame
> commands;
void record(const PresentJob &job, Frame &frame);
vk::Device* dev;
vcc::Setup* set;
std::thread thread;
std::queue<PresentJob> jobs;
bool alive;
std::condition_variable deathtoll;
};
}
#endif