#ifndef RECORDJOB_H_INCLUDE
#define RECORDJOB_H_INCLUDE
#include <functional>
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>

namespace vcc {
class RecordJob
{
public:
  const vk::CommandBufferBeginInfo info;
  const std::vector<vk::Semaphore>* wait;
  const vk::Semaphore signal;
  const vk::Fence tell;
  const vk::CommandBufferUsageFlags usage;

  RecordJob(const std::function<void(vk::CommandBuffer)>& funct,
            vk::CommandBufferUsageFlags usage,
            const vk::CommandBufferBeginInfo info = {},
            std::vector<vk::Semaphore>* wait = nullptr,
            vk::Device createSemaphore = nullptr,
            bool createFence = false)
    : exec(funct)
    , usage(usage)
    , info(info)
    , wait(wait)
    , signal([createSemaphore]() {
      if (createSemaphore)
        return createSemaphore.createSemaphore(vk::SemaphoreCreateInfo());
      else
        return vk::Semaphore();
    }())
    , tell([createSemaphore, createFence]() {
      if (createFence)
        return createSemaphore.createFence(vk::FenceCreateInfo());
      else
        return vk::Fence();
    }())
    , device(createSemaphore)
  {}

  void record(vk::CommandBuffer& b)
  {
#ifndef NDEBUG
    assert(command != vk::CommandBuffer(nullptr));
#endif
    b.begin(info);
    exec(b);
    b.end();
    command = b;
  }

private:
  const std::function<void(vk::CommandBuffer)> exec;
  const vk::Device device;
  vk::CommandBuffer command;
};
}
#endif