#ifndef RENDERER_H_INCLUDE
#define RENDERER_H_INCLUDE
#include <bits/stdint-uintn.h>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <boost/container/static_vector.hpp>

#include "VCEngine.hpp"
#include "jobs/RecordJob.hpp"
#include "jobs/SubmitJob.hpp"
#include "vkobjects/CmdBuffer.hpp"

namespace vcc {
/* T is the amount of frames in flight
 */
template<int T>
class Renderer
{

public:
  Renderer(const vk::Queue& graphics, const vk::Device& dev, uint32_t graphicsIndex);
  ~Renderer();
  void start();
  void submit(std::vector<RecordJob> record);

private:
  struct Frame
  {
    vk::CommandPool pool;
    vk::Fence submitted;
    std::vector<vcc::CmdBuffer> buffers;
    std::queue<vk::Semaphore> semaphores;
    Frame(vk::CommandPool pool,
          std::vector<vcc::CmdBuffer> buffers,
          std::queue<vk::Semaphore> semaphores)
      : pool(pool)
      , buffers(buffers)
      , semaphores(semaphores)
    {}
  };
  std::array<Frame, T> frames;
  vk::Device* dev;
  vk::Queue* graphics;
  std::thread thread;
  std::queue<std::vector<RecordJob>> recordJobs;
  bool alive;
  std::condition_variable deathtoll;

  vcc::SubmitJob record(const RecordJob& job, Frame& frame);
  void present(const std::vector<SubmitJob>& jobs,
               const Frame& frame,
               const vk::Semaphore& semaphore);
  void allocBuffers(const std::vector<SubmitJob>& jobs, const Frame& frame);
  static int countDependencies(const SubmitJob& job);
};
}
#endif