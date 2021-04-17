#ifndef RECORDJOB_H_INCLUDE
#define RECORDJOB_H_INCLUDE
#include <functional>
#include <vulkan/vulkan.hpp>

#include "vkobjects/CmdBuffer.hpp"
namespace vcc {
class RecordJob
{
public:
  RecordJob* dependency;
  const vk::CommandBufferBeginInfo info;

  RecordJob(const std::function<void(vk::CommandBuffer)>& funct,
            RecordJob* dep,
            const vk::CommandBufferBeginInfo info = {},
            vk::Fence* isDone = nullptr)
    : dependency(dep)
    , exec(funct)
    , info(info)
    , isDone(isDone)
  {}
  void record(vcc::CmdBuffer& b) const
  {
    b.state = bufferStates::kRecording;//can probably be removed
    b.cmd.begin(info);
    exec(b.cmd);
    b.cmd.end();
    b.state = bufferStates::kExecutable;
  }

private:
  const std::function<void(vk::CommandBuffer)> exec;
  vk::Fence* isDone;
};
}
#endif