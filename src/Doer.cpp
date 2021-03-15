#include <bits/stdint-uintn.h>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "Doer.hpp"
#include "Setup.hpp"
#include "VCEngine.hpp"
#include "src/jobs/PresentJob.hpp"
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
      std::pair(p, std::vector<vcc::CmdBuffer>(holder.begin(), holder.end())));
  }
}

Doer::~Doer()
{
  for (auto p : commands) {
    dev->freeCommandBuffers(
      p.first,
      std::vector<vk::CommandBuffer>(p.second.begin(), p.second.end()));
  }
  for (auto p : commands) {
    dev->destroyCommandPool(p.first);
  }
}
void
Doer::record(uint32_t p, uint32_t b, void (*funct)(vk::CommandBuffer c))
{
  static_cast<vk::CommandBuffer>(commands[p].second[b])
    .begin(vk::CommandBufferBeginInfo(
      vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
  funct(commands[p].second[b]);
  static_cast<vk::CommandBuffer>(commands[p].second[b]).end();
};

void
Doer::record(const PresentJob& job)
{
  if (!job.dependency) {
    record(*job.dependency);
    // put semaphore
  }
  // job.exec()
}

void
Doer::start()
{
  while (jobs.size() != 0) {
    record(jobs.front());
    jobs.pop();
  }
  for(auto commands : commands){
    for(auto buffer : commands.second){
      if(buffer.state == CmdBuffer::states::kExecutable){
        //set->graphicsQueue.submit(vk::SubmitInfo())
      }
    }
  }
}
}
