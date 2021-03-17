#include <bits/stdint-uintn.h>
#include <mutex>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "Doer.hpp"
#include "Setup.hpp"
#include "VCEngine.hpp"
#include "src/jobs/RecordJob.hpp"
#include "src/vkobjects/CmdBuffer.hpp"

namespace vcc {
Doer::Doer(vcc::Setup* s,
           vk::Device* d,
           uint32_t graphicsIndex,
           uint32_t poolCount,
           uint32_t b)
  : dev(d)
  , set(s)
{

  for (int i = 0; i < poolCount; i++) {
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
    poolInfo.queueFamilyIndex = graphicsIndex;
    vk::CommandPool p;
    if (d->createCommandPool(&poolInfo, nullptr, &p) != vk::Result::eSuccess)
      throw std::runtime_error("failed to create command pool");
    std::vector holder(dev->allocateCommandBuffers(
      vk::CommandBufferAllocateInfo(p, vk::CommandBufferLevel::ePrimary, b)));
    commands.push_back(
      Frame{ p, std::vector<vcc::CmdBuffer>(holder.begin(), holder.end()) }
      // Not sure if copying commandBuffer is legal here.
    );
  }
}

Doer::~Doer()
{
  alive = false;
  for (auto p : commands) {
    dev->freeCommandBuffers(
      p.pool,
      std::vector<vk::CommandBuffer>(p.buffers.begin(), p.buffers.end()));
    dev->destroyCommandPool(p.pool);
    for (auto s : p.semaphores) {
      dev->destroySemaphore(s);
    }
  }
}
void
Doer::record(uint32_t p, uint32_t b, void (*funct)(vk::CommandBuffer c))
{
  static_cast<vk::CommandBuffer>(commands[p].buffers[b])
    .begin(vk::CommandBufferBeginInfo(
      vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
  funct(commands[p].buffers[b]);
  static_cast<vk::CommandBuffer>(commands[p].buffers[b]).end();
};

void
Doer::record(const RecordJob& job, Frame& frame)
{
  std::unique_lock<std::mutex> lock;
  deathtoll.wait(lock);
  if (!job.dependency) {
    record(*job.dependency, frame);
    // put semaphore
    frame.semaphores.push_back(dev->createSemaphore(vk::SemaphoreCreateInfo()));
  }
  for (auto buffer : frame.buffers) {
    if (buffer.state == bufferStates::kInitial) {
      buffer.cmd.begin(vk::CommandBufferBeginInfo());
      buffer.state = bufferStates::kRecording;
      job.exec(buffer);
      buffer.cmd.end();
      buffer.state = bufferStates::kPending;
      return;
    }
  }
}

void
Doer::start()
{
  while (alive) {
    for (Frame f : commands) {
      while (jobs.size() != 0) {
        record(jobs.front(), f);
        jobs.pop();
      }
      for (auto commands : commands) {
        for (auto buffer : commands.buffers) {
          if (buffer.state == bufferStates::kPending) {
            set->graphicsQueue.submit(vk::SubmitInfo(

            ));
          }
        }
      }
    }
  }
  deathtoll.notify_all();
}
}
