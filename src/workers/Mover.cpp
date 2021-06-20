#include <algorithm>
#include <array>
#include <bits/stdint-uintn.h>
#include <boost/container/static_vector.hpp>
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
template<int b>
Mover<b>::Mover(const vk::Queue& t, const vk::Device& d, uint32_t transferIndex)
  : device(d)
  , transferQueue(t)
  , pool(d.createCommandPool(
      vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eTransient,
                                transferIndex),
      nullptr))
  , buffers(d.allocateCommandBuffers(
      vk::CommandBufferAllocateInfo(pool, vk::CommandBufferLevel::ePrimary, b)))
  , alive(true)
{}
template<int b>
Mover<b>::~Mover()
{
  alive = false; // deal with device loss later
  living.lock();
  transferQueue.waitIdle();
  device.destroyCommandPool(pool);
}
template<int b>
void
Mover<b>::submit(MoveJob&& job)
{
  recordJobs.enqueue(job);
}
template<int b>
void
Mover<b>::record(MoveJob& job, vk::CommandBuffer buffer)
{
  buffer.begin(vk::CommandBufferBeginInfo(job.usage));
  job.exec(buffer);
  buffer.end();
}
template<int b>
void
Mover<b>::doStuff()
{
  std::lock_guard lock(living);
  boost::container::static_vector<MoveJob, b> jobs;
  while (alive) {
    recordJobs.wait_dequeue_bulk(jobs.begin(), b);
    auto buffer = buffers.begin();
    while (!jobs.empty()) {
      record(jobs.back(), buffer++);
      jobs.pop_back();
    }
    transferQueue.waitIdle();
    transferQueue.submit(
      vk::SubmitInfo(0, {}, {}, buffers.size(), buffers.data(), {}, {}));
  }
}

}
