#ifndef CMDBUFFERINFO_H_INCLUDE
#define CMDBUFFERINFO_H_INCLUDE
#include <vulkan/vulkan.hpp>

namespace vcc {
enum struct bufferStates
{
  kInitial,
  kRecording,
  kExecutable,
  kPending,
  kInvalid,
  kNonexistant
};
struct CmdBuffer
{
  vk::CommandBuffer cmd;
  bufferStates state;
  operator vk::CommandBuffer() { return cmd; }
  CmdBuffer(vk::CommandBuffer commands)
    : cmd(commands)
    , state(bufferStates::kInitial)
  {}
  CmdBuffer()
    : cmd(nullptr)
    , state(bufferStates::kNonexistant)
  {}
};
}

#endif