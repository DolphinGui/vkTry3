#ifndef TASK_H_INCLUDE
#define TASK_H_INCLUDE
#include <vulkan/vulkan.hpp>
#include <vector>
#include <utility>
#include <stb/stb_image.h>

#include "Buffer.hpp"
#include "ImageBundle.hpp"
#include "data/UniformBufferObject.hpp"
#include "data/Vertex.hpp"

namespace vcc{
class Setup;

class Task{
public:
Task(
    Setup* s, 
    stbi_uc* image, 
    vk::DeviceSize imageSize, 
    std::vector<std::pair<Vertex, uint32_t>>* verts
    );
~Task();
void run();
private:
Setup* setup;
ImageBundle texture;

std::vector<std::pair<Vertex, uint32_t>>* vertices;
Buffer vertexB;
Buffer indexB;

std::vector<Buffer> uniformB;

vk::DescriptorPool descriptorPool;
std::vector<vk::DescriptorSet> descriptorSets;

std::vector<vk::CommandBuffer> commandBuffers;

std::vector<vk::Semaphore> imageAvailableS;
std::vector<vk::Semaphore> renderFinishedS;
std::vector<vk::Fence> inFlightF;
std::vector<vk::Fence> imagesInFlightF;
size_t currFrame = 0;

bool resized = false;
};
}
#endif