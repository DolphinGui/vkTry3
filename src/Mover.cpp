#include <algorithm>
#include <array>
#include <bits/stdint-uintn.h>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#include "Mover.hpp"
#include "VCEngine.hpp"
#include "jobs/RecordJob.hpp"
#include "jobs/SubmitJob.hpp"
#include "vkobjects/CmdBuffer.hpp"

namespace vcc {

template<int T>
Mover<T>::Mover(vk::Queue& g, vk::Device& d, uint32_t transferIndex)
{
  vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eTransient,
                                     transferIndex);
  if (d.createCommandPool(&poolInfo, nullptr, &pool) != vk::Result::eSuccess)
    throw std::runtime_error("failed to create command pool");
}
template<int T>
Mover<T>::~Mover()
{
  alive = false;
  std::unique_lock<std::mutex> lock;
  deathtoll.wait(lock);
}

template<int T>
void
Mover<T>::submit(RecordJob record)
{
  recordJobs.push(record);
}

template<int T>
vcc::SubmitJob
Mover<T>::record(const RecordJob& job)
{
  for (vcc::CmdBuffer b : buffers) {
    if (b.state == bufferStates::kInitial) {
      job.record(b);
    }
  }
  vcc::CmdBuffer buffer(
    dev->allocateCommandBuffers(vk::CommandBufferAllocateInfo(
      pool, vk::CommandBufferLevel::ePrimary, 1))[0]);
  job.record(buffer);
}

template<int T>
void
Mover<T>::present()
{
  transferQueue->submit(
    vk::SubmitInfo(0, {}, {}, submitJobs.size(), submitJobs.data()));
}

template<int T>
void
Mover<T>::start()
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
