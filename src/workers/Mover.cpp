#include <EASTL/fixed_vector.h>
#include <algorithm>
#include <array>
#include <bits/stdint-uintn.h>
#include <boost/container/static_vector.hpp>
#include <boost/move/detail/type_traits.hpp>
#include <exception>
#include <iterator>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "Mover.hpp"
#include "VCEngine.hpp"

namespace vcc {

Mover::Mover(const vk::Queue& t, const vk::Device& d, uint32_t transferIndex)
  : device(d)
  , transferQueue(t)
  , pool(d.createCommandPool(
      vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eTransient,
                                transferIndex),
      nullptr))
  , alive(true)
  , busy(true)
{
  thread = std::thread(&Mover::doStuff, this);
}

Mover::~Mover()
{
  alive = false; // deal with device loss later
  if (thread.joinable())
    thread.join();
  transferQueue.waitIdle();
  device.destroyCommandPool(pool);
}
/*could have 2 command pools to better spread work, but deal with that later*/
void
Mover::doStuff()
{

  boost::container::static_vector<MoveJob, bufferCount> jobs{};
  while (alive) {
    {
      std::lock_guard<std::mutex> lock(done);
      transferQueue.waitIdle();
    }
    device.resetCommandPool(pool);
    jobs.clear();
    auto buffers = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(
      pool, vk::CommandBufferLevel::ePrimary, bufferCount));
    busy = false;
    recordJobs.wait_dequeue_bulk(std::inserter(jobs, jobs.begin()),
                                 bufferCount);
    busy = true;
    int index{};
    for (auto& [usage, exec] : jobs) {
      auto buffer = buffers[index++];
      buffer.begin(vk::CommandBufferBeginInfo(usage));
      exec(buffer);
      buffer.end();
    }
    transferQueue.submit(
      vk::SubmitInfo(0, {}, {}, jobs.size(), buffers.data(), {}, {}), nullptr);
  }
}

}
