#include <algorithm>
#include <array>
#include <bits/stdint-uintn.h>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#include "Mover.hpp"
#include "VCEngine.hpp"

namespace vcc {

Mover::Mover(vk::Queue& t,
             vk::Device& d,
             uint32_t transferIndex,
             uint32_t bufferCount)
  : device(&d)
  , transferQueue(&t)
  , pool(d.createCommandPool(
      vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eTransient,
                                transferIndex),
      nullptr))
  , buffers(d.allocateCommandBuffers(
      vk::CommandBufferAllocateInfo(pool,
                                    vk::CommandBufferLevel::ePrimary,
                                    bufferCount)))
  , completed(d.createFence(vk::FenceCreateInfo()))
  , alive(true)

{}

Mover::~Mover()
{
  alive = false; // deal with device loss later
  living.lock();
  device->waitForFences(completed, VK_TRUE, UINT64_MAX);
  device->destroyFence(completed);
  device->destroyCommandPool(pool);
}

void
Mover::submit(MoveJob job)
{
  recordJobs.push(job);
  std::unique_lock<std::mutex> lock(awakeLock);
  asleep.notify_one();
}

void
Mover::record(const MoveJob& job, vk::CommandBuffer buffer)
{
  buffer.begin(vk::CommandBufferBeginInfo(job.usage));
  job.exec(buffer);
  buffer.end();
}

void const
Mover::wait()
{
  if (!recordJobs.empty()) // TODO: deal with device loss later
    device->waitForFences(completed, VK_TRUE, 100000);
}

void
Mover::doStuff()
{
  std::lock_guard lock(living);
  while (alive) {
    if (!recordJobs.empty()) {
      auto command = buffers.begin();
      while (!recordJobs.empty() && command != buffers.end()) {
        record(recordJobs.front(), *command++);
        recordJobs.pop();
      }
      if (device->getFenceStatus(completed) == vk::Result::eSuccess) {
        device->waitForFences(
          completed, VK_FALSE, 100000000); // TODO: deal with device loss later
        device->resetCommandPool(pool);
        device->resetFences(completed);
      }
      transferQueue->submit(
        vk::SubmitInfo(0, {}, {}, buffers.size(), buffers.data(), {}, {}),
        completed);
    } else {
      std::unique_lock<std::mutex> lock(awakeLock);
      asleep.wait(lock, [&]() { return !recordJobs.empty(); });
    }
  }
}

}
