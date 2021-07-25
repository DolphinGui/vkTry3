#pragma once

#include <array>
#include <bits/stdint-uintn.h>
#include <condition_variable>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <mutex>
#include <queue>
#include <tuple>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "blockingconcurrentqueue.h"
#include "gsl/gsl"

#include "VCEngine.hpp"
#include "gsl/span"

namespace vcc {
// This will use queue, do not try to use it along with this
class RenderGroup
{
private:
  constexpr static int W = 2;
  constexpr static int F = 3;

  template<typename T>
  using WorkQueue = moodycamel::BlockingConcurrentQueue<T>;
  struct PresentImage
  {
    vk::Semaphore available;
    vk::Semaphore finished;
    vk::Framebuffer itself;
    std::array<vk::Fence, W> finishFence;
  };
  struct RenderJob
  {
    using Command = void (*)(vk::CommandBuffer, vk::Framebuffer);
    vk::CommandBufferBeginInfo usage;
    vk::Framebuffer target;
    RenderJob(Command buffer,
              vk::CommandBufferBeginInfo usage = vk::CommandBufferBeginInfo(
                vk::CommandBufferUsageFlagBits::eOneTimeSubmit))
      : commands(buffer)
      , usage(usage)
    {}
    // don't execute a default constructed job
    void record(vk::CommandBuffer buffer)
    {
      buffer.begin(usage);
      commands(buffer, target);
      buffer.end();
    }

  private:
    Command commands;
  };

  class Worker
  {
  private:
    struct Frame
    {
      // The amount of buffers allocated. Should probably be tested.
      // Yes I know holding device pointer is probably inefficient
      // no, it probably won't be a lot of memory
      static constexpr int bufferCount = 1;
      const vk::Device device; // non-owning
      const vk::CommandPool pool;
      const vk::Semaphore start;    // non-owning
      const vk::Semaphore complete; // non-owning
      const vk::Fence finished;     // non-owning
      const vk::Framebuffer image;  // non-owning
      const std::vector<vk::CommandBuffer> buffers;

      Frame(vk::Device d,
            vk::CommandPoolCreateInfo poolInfo,
            PresentImage image,
            int fenceIndex)
        : device(d)
        , pool(device.createCommandPool(poolInfo))
        , buffers(device.allocateCommandBuffers(
            vk::CommandBufferAllocateInfo(pool,
                                          vk::CommandBufferLevel::ePrimary,
                                          bufferCount)))
        , start(image.available)
        , complete(image.finished)
        , image(image.itself)
        , finished(image.finishFence[fenceIndex])
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
    ~Worker()
    {
      alive = false;
      std::lock_guard lock(living);
      graphics.waitIdle();
    }

    template<typename It>
    Worker(vk::Device device,
           vk::Queue graphics,
           int workerIndex,
           uint32_t queueIndex,
           WorkQueue<RenderJob>& jobquery,
           WorkQueue<vk::SubmitInfo>& submitquery,
           std::condition_variable& alarmclock,
           std::atomic<uint>& workload,
           It PresentImages)
      : device(device)
      , graphics(graphics)
      , jobquery(jobquery)
      , submitquery(submitquery)
      , alive{ true }
      , living()
      , alarmclock(alarmclock)
      , workload(workload)
      , frames{ Frame(device,
                      vk::CommandPoolCreateInfo(
                        vk::CommandPoolCreateFlagBits::eTransient,
                        queueIndex),
                      *PresentImages++,
                      workerIndex),
                Frame(device,
                      vk::CommandPoolCreateInfo(
                        vk::CommandPoolCreateFlagBits::eTransient,
                        queueIndex),
                      *PresentImages++,
                      workerIndex),
                Frame(device,
                      vk::CommandPoolCreateInfo(
                        vk::CommandPoolCreateFlagBits::eTransient,
                        queueIndex),
                      *PresentImages++,
                      workerIndex) }
    {
      // std::fill
    }

  private:
    std::array<Frame, F> frames;
    std::thread thread;
    const vk::Device device;  // non-owning
    const vk::Queue graphics; // non-owning
    std::atomic_bool alive;
    std::mutex living;
    int frameNumber{};
    WorkQueue<RenderJob>& jobquery;
    WorkQueue<vk::SubmitInfo>& submitquery;
    std::condition_variable& alarmclock;
    std::atomic<uint>& workload;
    // The maximum amount of jobs this can grab at once.
    static constexpr int maxJobGrab = 1;

    void present(const Frame&);
    void allocBuffers(int amount, const Frame&);
    void doStuff();
  };

public:
  /* It must be the iterator ImageView of the swapchain vector.
    There must be at least N ImageViews in it */
  RenderGroup(const vcc::VCEngine& engine,
              const vk::ImageView& color,
              const vk::ImageView& depth,
              const vk::RenderPass& renderpass,
              const vk::SwapchainKHR& swapchain,
              gsl::span<vk::ImageView> swapChainImageViews,
              vk::Extent2D size,
              int layers);
  ~RenderGroup();
  void render(std::initializer_list<RenderJob> job);
  void advance();

private:
  vk::Device device;  // non-owning
  vk::Queue graphics; // non-owning
  int currentImage = 0;
  const vk::SwapchainKHR& swapchain;
  WorkQueue<RenderJob> jobquery;
  WorkQueue<vk::SubmitInfo> submitquery;
  std::vector<PresentImage> images; // should get replaced on framebuffer resize
  std::array<Worker, W> workers;
  std::atomic<uint> workload{};
  std::condition_variable wakeup{};
};

}

