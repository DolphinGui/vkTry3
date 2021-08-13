#pragma once
#include <atomic>
#include <bits/stdint-uintn.h>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "VCEngine.hpp"
#include "blockingconcurrentqueue.h"

namespace vcc {

class Mover
{

public:
  constexpr static int bufferCount = 3;
  Mover(const vk::Queue& transfer,
        const vk::Device& dev,
        uint32_t transferIndex);
  Mover(const Mover&) = delete;
  Mover& operator=(const Mover&) = delete;
  ~Mover();

  /*
  This does not own the signal fence, and the user is expected
  to keep the fence alive until it's signalled.
  */
  struct MoveJob
  {
    vk::CommandBufferUsageFlags usage;
    std::function<void(vk::CommandBuffer)> exec;
  };

  inline bool submit(MoveJob&& job) noexcept
  {
    return recordJobs.enqueue(std::move(job));
  };
  inline void wait(
    uint64_t timeout = std::numeric_limits<uint64_t>::max()) noexcept
  {
    if (busy.load(std::memory_order_relaxed))
      std::lock_guard<std::mutex> lock(done);
  };

private:
  const vk::Device& device;
  const vk::Queue& transferQueue;
  std::thread thread;
  vk::CommandPool pool;
  moodycamel::BlockingConcurrentQueue<MoveJob> recordJobs;
  std::mutex done{};
  std::atomic_bool busy{};
  std::atomic_bool alive;

  void doStuff();
};
}
