#include "Task.hpp"
#include "Setup.hpp"

using namespace vcc;

Task::Task(
    Setup* s, 
    stbi_uc* image, 
    vk::DeviceSize imageSize, 
    std::vector<std::pair<Vertex, uint32_t>>* verts
    ):
    vertices(verts),
    texture(){

}