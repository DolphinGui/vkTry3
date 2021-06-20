#ifndef RENDERGROUP_H_INCLUDE
#define RENDERGROUP_H_INCLUDE

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

#include "concurrentqueue/concurrentqueue.h"

#include "VCEngine.hpp"

namespace vcc {
// This will use queue, do not try to use it along with this
template<int F, int W>
class RenderGroup
{
private:
  template<typename T>
  using WorkQueue = moodycamel::ConcurrentQueue<T>;
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
    const vk::CommandBufferBeginInfo usage;
    vk::Framebuffer target;
    RenderJob(Command buffer,
              vk::CommandBufferBeginInfo usage = vk::CommandBufferBeginInfo(
                vk::CommandBufferUsageFlagBits::eOneTimeSubmit))
      : commands(buffer)
      , usage(usage)
    {}
    void record(vk::CommandBuffer buffer)
    {
      buffer.begin(usage);
      commands(buffer, target);
      buffer.end();
    }

  private:
    const Command commands;
  };

  class Worker
  {
  private:
    struct Frame
    {
      // The amount of buffers allocated. Should probably be tested.
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
  protected:
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
    {
      for (Frame& frame : frames) {
        frame = Frame(device,
                      vk::CommandPoolCreateInfo(
                        vk::CommandPoolCreateFlagBits::eTransient, queueIndex),
                      *PresentImages++,
                      workerIndex);
      }
    }
    

  private:
    std::array<Frame, F> frames;
    std::thread thread;
    const vk::Device device;  // non-owning
    const vk::Queue graphics; // non-owning
    std::atomic_bool alive;
    std::mutex living;
    int frameNumber{};
    const WorkQueue<RenderJob>& jobquery;
    const WorkQueue<RenderJob>& submitquery;
    std::condition_variable& alarmclock;
    std::atomic<uint>& workload;
    // The maximum amount of jobs this can grab at once.
    static constexpr int maxJobGrab = 1;

    void present(const Frame&);
    void allocBuffers(int amount, const Frame&);
    void doStuff();
  };

public:
  template<typename Iterator>
  RenderGroup(const vcc::VCEngine&,
              const vk::ImageView& color,
              const vk::ImageView& depth,
              const vk::RenderPass&,
              const vk::SwapchainKHR&,
              Iterator swapchainImages,
              vk::Extent2D,
              int layers);
  void render(std::initializer_list<RenderJob> job);
  void advance();

private:
  vk::Device device;  // non-owning
  vk::Queue graphics; // non-owning
  int currentImage = 0;
  const vk::SwapchainKHR& swapchain;
  WorkQueue<RenderJob> jobquery;
  WorkQueue<vk::SubmitInfo> submitquery;
  std::array<Worker, W> workers;
  std::vector<PresentImage> images; // should get replaced on framebuffer resize
  std::atomic<uint> workload{};
  std::condition_variable wakeup{};
};

}
#endif