#include <algorithm>
#include <array>
#include <bits/stdint-uintn.h>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#include "Doer.hpp"
#include "VCEngine.hpp"
#include "jobs/PresentJob.hpp"
#include "jobs/RecordJob.hpp"
#include "vkobjects/CmdBuffer.hpp"

namespace vcc {

template<int T>
std::array<vk::CommandPool, T>
allocCommandBuffersStatic(const vk::Device& device, vk::CommandBufferAllocateInfo info)
{
  static_assert(T==info.commandBufferCount, "allocCommandBuffersStatic has incorrect info");
  std::array<vk::CommandPool, T> results;
  vkAllocateCommandBuffers(device, info, results);
  return results;
}

template<int T, int U, int V>
Doer<T, U, V>::Doer(vk::Queue& g,
              vk::Device& d,
              uint32_t graphicsIndex,
              uint32_t poolCount)
  : dev(&d)
  , graphics(&g)
{

  for (auto i = commands.begin(); i != commands.end(); i++) {
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
    poolInfo.queueFamilyIndex = graphicsIndex;
    vk::CommandPool p;
    if (d.createCommandPool(&poolInfo, nullptr, &p) != vk::Result::eSuccess)
      throw std::runtime_error("failed to create command pool");
    *i = Frame(p,
               allocCommandBuffersStatic<U>(d, vk::CommandBufferAllocateInfo(
                 p, vk::CommandBufferLevel::ePrimary, U)));
  }
}
template<int T, int U, int V>
Doer<T, U, V>::~Doer()
{
  alive = false;
  std::unique_lock<std::mutex> lock;
  deathtoll.wait(lock);
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
template<int T, int U, int V>
vcc::PresentJob
Doer<T, U, V>::record(const RecordJob& job, Frame<U, V>& frame)
{
  PresentJob dependency;
  if (!job.dependency)
    dependency = record(*job.dependency, frame);
  for (auto buffer : frame.buffers) {
    if (buffer.state == bufferStates::kInitial) {
      buffer.cmd.begin(vk::CommandBufferBeginInfo());
      buffer.state = bufferStates::kRecording;
      job.exec(buffer);
      buffer.cmd.end();
      buffer.state = bufferStates::kPending;
      return PresentJob(buffer, dependency);
    }
  }
  throw std::runtime_error("not enough buffers allocated");
}
template<int T, int U, int V>
void
Doer<T, U, V>::present(const std::vector<PresentJob>& jobs,
                 const Frame<U, V>& f,
                 const vk::Semaphore& s)
{
  std::vector<PresentJob*> dependents;
  std::vector<vk::CommandBuffer*> buffers;
  for (auto job : jobs) {
    dependents.push_back(job.dependent);
    buffers.push_back(&job.commands->cmd);
  }
  vk::Semaphore sem;
  int semaphoreCount = 0;
  if (dependents.size() != 0) {
    sem = dev->createSemaphore(vk::SemaphoreCreateInfo());
    f.semaphores.push_back(sem);
    semaphoreCount = 1;
  }
  int waitCount = (s) ? 1 : 0;
  graphics->submit(vk::SubmitInfo(
    waitCount, &s, {}, buffers.size(), buffers[0], semaphoreCount, &sem));
  if (dependents.size() != 0)
    present(dependents, f, sem);
}
template<int T, int U, int V>
void
Doer<T, U, V>::start()
{
  std::vector<vcc::PresentJob> jobs;
  while (alive) {
    for (Frame frame : commands) {
      while (records.size() != 0) {
        jobs.push_back(record(records.front(), frame));
        records.pop();
      }
      present(jobs, frame, nullptr);
    }
  }
  deathtoll.notify_all();
}
}
