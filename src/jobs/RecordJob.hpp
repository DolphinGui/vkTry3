#ifndef PRESENTJOB_H_INCLUDE
#define PRESENTJOB_H_INCLUDE
#include <vulkan/vulkan.hpp>

namespace vcc{
class PresentJob{
public:
const PresentJob* dependency;
const void (*exec)(vk::CommandBuffer);
PresentJob(const void (*funct)(vk::CommandBuffer), const PresentJob* dep):
    dependency(dep), exec(funct){}
};
}
#endif