#include <algorithm>
#include <array>
#include <bits/stdint-uintn.h>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#include "Renderer.hpp"
#include "VCEngine.hpp"
#include "jobs/RecordJob.hpp"
#include "jobs/SubmitJob.hpp"
#include "vkobjects/CmdBuffer.hpp"

namespace vcc {

template<int T>
Renderer<T>::Renderer(const vk::Queue& g, const vk::Device& d, uint32_t graphicsIndex)
{

  for (auto i = frames.begin(); i != frames.end(); i++) {
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
    poolInfo.queueFamilyIndex = graphicsIndex;
    vk::CommandPool p;
    if (d.createCommandPool(&poolInfo, nullptr, &p) != vk::Result::eSuccess)
      throw std::runtime_error("failed to create command pool");
    Frame f = Frame(p);
    f.submitted = dev->createFence(vk::FenceCreateInfo());
    *i = f;
  }
}
template<int T>
Renderer<T>::~Renderer()
{
  alive = false;
  std::unique_lock<std::mutex> lock;
  deathtoll.wait(lock);
  for (Frame f : frames) {
    dev->destroyFence(f.submitted);
    dev->freeCommandBuffers(
      f.pool,
      std::vector<vk::CommandBuffer>(f.buffers.begin(), f.buffers.end()));
    dev->destroyCommandPool(f.pool);
    for (auto s : f.semaphores) {
      dev->destroySemaphore(s);
    }
  }
}

template<int T>
void
Renderer<T>::submit(std::vector<RecordJob> record)
{
  recordJobs.push(record);
}

template<int T>
void
Renderer<T>::allocBuffers(const std::vector<SubmitJob>& jobs,
                          const Frame& frame)
{
  int bufferCount(0);
  for (auto job : jobs) {
    bufferCount += countDependencies(job);
  }
  int buffersToAlloc(bufferCount - frame.buffers.size);
  if (buffersToAlloc >
      0) { // TODO: Add support for secondary command buffers later
    dev->allocateCommandBuffers(vk::CommandBufferAllocateInfo(
      frame.pool, vk::CommandBufferLevel::ePrimary, buffersToAlloc));
  }
  dev->waitForFences(frame.submitted);
  for (vcc::CmdBuffer& c : frame.buffers) {
    if (c.state != vcc::bufferStates::kInitial) {
      c.cmd.reset(); // determine later if resources should be released
    }
  }
}
// I'm sure there's a way to do this compiletime,
// but I'm also pretty sure it doesn't matter too much.
template<int T>
int
Renderer<T>::countDependencies(const SubmitJob& job)
{
  if (job.dependent)
    return countDependencies(*job.dependent) + 1;
  return 1;
}

template<int T>
vcc::SubmitJob
Renderer<T>::record(const RecordJob& job, Frame& frame)
{
  for (auto buffer : frame.buffers) {
    if (buffer.state == bufferStates::kInitial) {
      buffer.state = bufferStates::kRecording;
      job.record(buffer);
      buffer.state = bufferStates::kPending;
      if (!job.dependency)
        return SubmitJob(
          buffer, record(*job.dependency, frame), job.info.flags);
      return SubmitJob(buffer, nullptr, job.info.flags);
    }
  }
  throw std::runtime_error("not enough buffers allocated");
}
template<int T>
void
Renderer<T>::present(const std::vector<SubmitJob>& jobs,
                     const Frame& f,
                     const vk::Semaphore& s)
{
  std::vector<SubmitJob*> dependents;
  std::vector<vk::CommandBuffer*> buffers;

  dependents.reserve(jobs.size());
  buffers.reserve(jobs.size());
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

  if (dependents.size() != 0) {
    graphics->submit(vk::SubmitInfo(
      waitCount, &s, {}, buffers.size(), buffers[0], semaphoreCount, &sem));
    present(dependents, f, sem);
  }
  graphics->submit(
    vk::SubmitInfo(
      waitCount, &s, {}, buffers.size(), buffers[0], semaphoreCount, &sem),
    f.submitted);
}
template<int T>
void
Renderer<T>::start()
{
  std::vector<vcc::SubmitJob> jobs;
  while (alive) {
    for (Frame frame : frames) {
      jobs.reserve(recordJobs.size());
      while (recordJobs.size() != 0) {
        allocBuffers(recordJobs.front(), frame);
        jobs.push_back(record(recordJobs.front(), frame));
        recordJobs.pop();
      }
      present(jobs, frame, nullptr);
    }
  }
  deathtoll.notify_all();
}
}