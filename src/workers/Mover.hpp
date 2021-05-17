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

namespace vcc {
class Mover
{

public:
  Mover(vk::Queue& transfer,
        vk::Device& dev,
        uint32_t transferIndex,
        uint32_t bufferCount = 1);
  Mover(const Mover&) = delete;
  Mover& operator= (const Mover&) = delete;
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
    MoveJob(vk::CommandBuffer& buffer,
            vk::CommandBufferUsageFlags usage,
            std::function<void(vk::CommandBuffer)> exec,
            vk::Fence signal = nullptr)
      : commands(&buffer)
      , usage(usage)
      , exec(exec)
      , signal(signal)
    {}

  private:
    vk::CommandBuffer* commands;
  };

  void const wait();
  void submit(MoveJob job);

private:
  vk::Device* device;
  vk::Queue* transferQueue;
  std::thread thread;
  vk::CommandPool pool;
  std::vector<vk::CommandBuffer> buffers;
  std::queue<MoveJob> recordJobs;
  std::atomic_bool alive;
  std::mutex living;
  std::mutex awakeLock;
  std::condition_variable asleep;
  const vk::Fence completed;

  void record(const MoveJob& job, vk::CommandBuffer);
  void doStuff();
};
}
#endif