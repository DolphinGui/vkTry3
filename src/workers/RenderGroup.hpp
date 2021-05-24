#ifndef RENDERGROUP_H_INCLUDE
#define RENDERGROUP_H_INCLUDE

#include <array>
#include <bits/stdint-uintn.h>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <queue>
#include <tuple>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "concurrentqueue/blockingconcurrentqueue.h"

#include "Setup.hpp"
#include "VCEngine.hpp"

namespace vcc {
template<int N, int W>
class RenderGroup
{
private:
  template<typename T>
  using Queue = moodycamel::BlockingConcurrentQueue<T>;
  struct PresentImage
  {
    vk::Semaphore available;
    vk::Semaphore finished;
    vk::Framebuffer itself;
  };
  struct RenderJob
  {
    const vk::CommandBufferBeginInfo usage;
    vk::Framebuffer target;
    RenderJob(std::function<void(vk::CommandBuffer, vk::Framebuffer)>&& buffer,
              vk::CommandBufferBeginInfo usage)
      : commands(std::move(buffer))
      , usage(usage)
    {}
    void execute(vk::CommandBuffer buffer)
    {
      buffer.begin(usage);
      commands(buffer, target);
      buffer.end();
    }

  private:
    std::function<void(vk::CommandBuffer, vk::Framebuffer)> commands;
  };

  class Worker
  {
  private:
    struct Frame
    {
      const vk::Device device;
      const vk::CommandPool pool;
      const vk::Semaphore start;
      const vk::Semaphore complete;
      const vk::Framebuffer image;
      const std::vector<vk::CommandBuffer> buffers;

      Frame(vk::Device d,
            vk::CommandPoolCreateInfo poolInfo,
            int bufferCount,
            PresentImage image)
        : device(d)
        , pool(device.createCommandPool(poolInfo))
        , buffers(device.allocateCommandBuffers(
            vk::CommandBufferAllocateInfo(pool,
                                          vk::CommandBufferLevel::ePrimary,
                                          bufferCount)))
        , start(image.available)
        , complete(image.finished)
        , image(image.itself)
      {}

      ~Frame()
      {
        device.waitIdle();
        device.destroyCommandPool(pool);
      }
    };

  public:
    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;
    Worker() = delete;

  protected:
    template<typename It>
    Worker(vk::Device device,
           vk::Queue graphics,
           uint32_t queueIndex,
           Queue<RenderJob>& jobquery,
           int initialBuffercount,
           It PresentImages)
      : device(device)
      , graphics(graphics)
      , jobquery(jobquery)
      , alive{ true }
      , living()
    {
      for (Frame& frame : frames) {
        frame = Frame(device,
                      vk::CommandPoolCreateInfo(
                        vk::CommandPoolCreateFlagBits::eTransient, queueIndex),
                      initialBuffercount,
                      *PresentImages++);
      }
    }

  private:
    std::array<Frame, N> frames;
    std::thread thread;
    const vk::Device device;
    const vk::Queue graphics;
    std::atomic_bool alive;
    std::mutex living;
    int frameNumber{};
    const Queue<RenderJob>& jobquery;
    // The maximum amount of jobs this can grab at once.
    // Not sure if this is a good number, needs more testing
    static constexpr int maxJobGrab = 5;

    void present(const Frame&);
    void allocBuffers(int amount, const Frame&);
    void doStuff();
  };

public:
  template<typename Iterator>
  RenderGroup(const vcc::VCEngine,
              const vcc::Setup&,
              Iterator swapchain,
              vk::Extent2D,
              int layers);
  void render(std::initializer_list<RenderJob> job);
  void advance();

private:
  Queue<RenderJob> jobquery;
  std::array<Worker, W> workers;
  std::vector<PresentImage> images;
  vk::Device device;
};

}
#endif