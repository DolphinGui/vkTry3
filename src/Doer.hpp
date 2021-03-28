#ifndef DOER_H_INCLUDE
#define DOER_H_IN
#include <bits/stdint-uintn.h>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "VCEngine.hpp"
#include "jobs/SubmitJob.hpp"
#include "jobs/RecordJob.hpp"
#include "vkobjects/CmdBuffer.hpp"

namespace vcc {
/* T is the amount of frames in flight,
  U is the amount of command buffers
  V is the amount of semaphores allowed
*/
template<int T>
class Doer
{

public:
  Doer(vk::Queue& graphics,
       vk::Device& dev,
       uint32_t graphicsIndex,
       uint32_t poolCount);
  ~Doer();
  void start();

private:
  struct Frame
  {
    vk::CommandPool pool;
    vk::Fence submitted;
    std::vector<vcc::CmdBuffer> buffers;
    std::queue<vk::Semaphore> semaphores;
    Frame(vk::CommandPool pool,
          std::vector<vcc::CmdBuffer> buffers,
          std::vector<vk::Semaphore> semaphores)
    {
      this->pool = pool, this->buffers = buffers, this->semaphores = semaphores;
    }
  };
  std::array<Frame, T> commands;
  vcc::SubmitJob record(const RecordJob& job, Frame& frame);
  void present(const std::vector<SubmitJob>& jobs,
               const Frame& frame,
               const vk::Semaphore& semaphore);
  void allocBuffers(const std::vector<SubmitJob>& jobs, const Frame& frame);
  static int countDependencies(const SubmitJob& job);
  vk::Device* dev;
  vk::Queue* graphics;
  std::thread thread;
  std::queue<RecordJob> records;
  bool alive;
  std::condition_variable deathtoll;
};
}
#endif