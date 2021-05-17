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

#include "Renderer.hpp"
#include "VCEngine.hpp"
#include "jobs/RecordJob.hpp"

namespace vcc {

namespace {
template<int T>
typename vcc::Renderer<T>::Frame
createFrame(const vk::Queue& g,
            const vk::Device& d,
            uint32_t graphicsIndex,
            int initialAlloc)
{
  vk::CommandPoolCreateInfo poolInfo{};
  poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
  poolInfo.queueFamilyIndex = graphicsIndex;
  vk::CommandPool p;
  if (d.createCommandPool(&poolInfo, nullptr, &p) != vk::Result::eSuccess)
    throw std::runtime_error("failed to create command pool");
  vk::Fence f = d.createFence(vk::FenceCreateInfo());
  return typename vcc::Renderer<T>::Frame{
    p,
    f,
    d.allocateCommandBuffers(vk::CommandBufferAllocateInfo(
      p, vk::CommandBufferLevel::ePrimary, initialAlloc)),
    d.createSemaphore(vk::SemaphoreCreateInfo())
  };
}
}

template<int T>
Renderer<T>::Renderer(const vk::Queue& g,
                      const vk::Device& d,
                      uint32_t graphicsIndex)
  : frames({ createFrame<T>(g, d, graphicsIndex, 1),
             createFrame<T>(g, d, graphicsIndex, 1),
             createFrame<T>(g, d, graphicsIndex, 1) })
  , alive(true)

{
  thread = std::thread(&Renderer::doStuff(), this);
  thread.detach();
}
template<int T>
Renderer<T>::~Renderer()
{
  alive = false;
  living.lock();
  for (Frame f : frames) {
    dev->destroyFence(f.complete);
    dev->freeCommandBuffers(f.pool, f.buffers);
    dev->destroyCommandPool(f.pool);
    for (auto s : f.semaphores) {
      dev->destroySemaphore(s);
    }
  }
}

template<int T>
void
Renderer<T>::doStuff()
{
  std::lock_guard lock(living);
  while (alive) {
    while (recordJobs.size() != 0) {
      allocBuffers(recordJobs.front(), frames[frameNumber]);
        auto buffer = frames[frameNumber].buffers.begin();
        for(RenderJob& job : recordJobs.front()){
          job.execute(*buffer++);
        }
        recordJobs.pop();
       present(frames[frameNumber], frames[frameNumber].done);//do sync
    }
  }
}

template<int T>
void
Renderer<T>::submit(std::vector<RenderJob>&& record)
{
  recordJobs.push(std::move(record));
}

template<int T>
void
Renderer<T>::allocBuffers(const std::vector<RenderJob>& jobs,
                          const Frame& frame)
{
  int buffersToAlloc(jobs.size() - frame.buffers.size);
  // TODO: Add support for secondary command buffers later
  if (buffersToAlloc > 0) {
    dev->allocateCommandBuffers(vk::CommandBufferAllocateInfo(
      frame.pool, vk::CommandBufferLevel::ePrimary, buffersToAlloc));
  }
  dev->waitForFences(frame.complete);
  dev->resetCommandPool(frame.pool);
}

template<int T>
void
Renderer<T>::present(const Frame& f, const vk::Semaphore& s)
{
  graphics->submit(
    vk::SubmitInfo(1, &s, {}, f.buffers.size(), f.buffers.data(), 1, f.done),
    f.complete);
  frameNumber = (frameNumber + 1) % T;
}

}
