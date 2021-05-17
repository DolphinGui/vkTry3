#ifndef RENDERER_H_INCLUDE
#define RENDERER_H_INCLUDE
#include <atomic>
#include <bits/stdint-uintn.h>
#include <boost/container/static_vector.hpp>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "VCEngine.hpp"
#include "jobs/RecordJob.hpp"

namespace vcc {
template<int T>
class Renderer
{
public:
  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;
  Renderer() = delete;
  Renderer(const vk::Queue& graphics,
           const vk::Device& dev,
           uint32_t graphicsIndex);

  struct RenderJob
  {
    const vk::CommandBufferBeginInfo usage;
    RenderJob(std::function<void(vk::CommandBuffer)>&& buffer,
              vk::CommandBufferBeginInfo usage)
      : commands(std::move(buffer))
      , usage(usage)
    {}
    void execute(vk::CommandBuffer buffer){
      buffer.begin(usage);
      commands(buffer);
      buffer.end();
    }
  private:
    std::function<void(vk::CommandBuffer)> commands;
  };
  void submit(std::vector<RenderJob>&& record);
  void flush();
  bool done() const { return recordJobs.empty(); }

private:
  ~Renderer();

  struct Frame
  {
    vk::CommandPool pool;
    vk::Fence complete;
    std::vector<vk::CommandBuffer> buffers;
    vk::Semaphore done;
    Frame(vk::CommandPool pool,
          std::vector<vk::CommandBuffer> buffers,
          vk::Semaphore semaphore)
      : pool(pool)
      , buffers(buffers)
      , done(semaphore)
    {}
  };
  std::array<Frame, T> frames;
  vk::Device* dev;
  vk::Queue* graphics;
  std::thread thread;
  std::queue<std::vector<RenderJob>> recordJobs;
  std::atomic_bool alive;
  std::mutex living;
  int frameNumber{};

  void present(const Frame& frame, const vk::Semaphore& semaphore);
  void allocBuffers(const std::vector<RenderJob>& jobs, const Frame& frame);
  void doStuff();
};
}
#endif