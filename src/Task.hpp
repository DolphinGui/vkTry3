#ifndef TASK_H_INCLUDE
#define TASK_H_INCLUDE
#include <vulkan/vulkan.hpp>
#include "ImageBundle.hpp"

namespace vcc{
class Setup;

class Task{
public:
Task(Setup*);
~Task();
void run();
private:
Setup* setup;
ImageBundle texture;

};
}
#endif