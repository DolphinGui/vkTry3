#ifndef PRESENTJOB_H_INCLUDE
#define PRESENTJOB_H_INCLUDE

#include <memory>
#include <vulkan/vulkan.hpp>

#include "vkobjects/CmdBuffer.hpp"

namespace vcc {
struct SubmitJob
{
public:
  vk::CommandBuffer* commands;
  SubmitJob* dependent; // change this to support multiple dependencies later
  const vk::CommandBufferUsageFlags usage;
  SubmitJob(vk::CommandBuffer& buffer, vk::CommandBufferUsageFlags usage)
    : commands(&buffer)
    , dependent(nullptr)
    , usage(usage)
  {}
  SubmitJob(vk::CommandBuffer& buffer,
            SubmitJob& depend,
            vk::CommandBufferUsageFlags usage)
    : commands(&buffer)
    , dependent(&depend)
    , usage(usage)
  {}
};
}
#endif