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
    std::function<void(vk::CommandBuffer)> exec;vk::
    Fence signal;
  };

  void submit(MoveJob&& job);

private:
  const vk::Device& device;
  const vk::Queue& transferQueue;
  std::thread thread;
  vk::CommandPool pool;
  std::vector<vk::CommandBuffer> buffers;
  moodycamel::BlockingConcurrentQueue<MoveJob> recordJobs;
  std::atomic_bool alive;
  std::mutex living;

  void record(MoveJob& job, vk::CommandBuffer);
  void doStuff();
};
}
