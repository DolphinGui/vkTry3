#ifndef RECORDJOB_H_INCLUDE
#define RECORDJOB_H_INCLUDE
#include <vulkan/vulkan.hpp>

namespace vcc {
class RecordJob
{
public:
  RecordJob* dependency;
  const void (*exec)(vk::CommandBuffer);
  RecordJob(const void (*funct)(vk::CommandBuffer), RecordJob* dep)
    : dependency(dep)
    , exec(funct)
  {}
};
}
#endif