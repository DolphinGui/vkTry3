#ifndef PRESENTJOB_H_INCLUDE
#define PRESENTJOB_H_INCLUDE
#include <vulkan/vulkan.hpp>

namespace vcc{
class RecordJob{
public:
const RecordJob* dependency;
const void (*exec)(vk::CommandBuffer);
RecordJob(const void (*funct)(vk::CommandBuffer), const RecordJob* dep):
    dependency(dep), exec(funct){}
};
}
#endif