#ifndef MOVER_H_INCLUDE
#define MOVER_H_INCLUDE
#include <atomic>
#include <bits/stdint-uintn.h>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "VCEngine.hpp"
#include "concurrentqueue/blockingconcurrentqueue.h"

namespace vcc {
template<int bufferCount = 3>
class Mover
{

public:
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
    const vk::CommandBufferUsageFlags usage;
    vk::Fence signal;
    std::function<void(vk::CommandBuffer)> exec;
    MoveJob(vk::CommandBufferUsageFlags usage,
            std::function<void(vk::CommandBuffer)> exec,
            vk::Fence signal = nullptr)
      : usage(usage)
      , exec(exec)
      , signal(signal)
    {}
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
#endif