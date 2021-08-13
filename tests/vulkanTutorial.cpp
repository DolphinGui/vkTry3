#include <future>
#include <string>

#define VMA_IMPLEMENTATION
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "Setup.hpp"
#include "Task.hpp"
#include "VCEngine.hpp"
#include "data/Vertex.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

struct Image
{
  stbi_uc* pixels;
  uint32_t width, height, channels;
  vk::DeviceSize size;
  ~Image() { stbi_image_free(pixels); }
};

Image
createTextureImage(const char* path)
{
  int texWidth, texHeight, texChannels;
  stbi_uc* pixels =
    stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  VkDeviceSize imageSize = texWidth * texHeight * 4;
  if (!pixels) {
    throw std::runtime_error("failed to load texture image!");
  }
  return { pixels,
           static_cast<uint32_t>(texWidth),
           static_cast<uint32_t>(texHeight),
           static_cast<uint32_t>(texChannels),
           imageSize };
}

struct Model
{
  std::vector<Vertex> verticies;
  std::vector<uint32_t> indicies;
};

Model
loadModel(const char* path)
{
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(
        &attrib, &shapes, &materials, &warn, &err, path)) {
    throw std::runtime_error(warn + err);
  }

  std::unordered_map<Vertex, uint32_t> uniqueVertices{};
  std::vector<Vertex> verticies;
  std::vector<uint32_t> indicies;
  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
      Vertex vertex{};

      vertex.pos = { attrib.vertices[3 * index.vertex_index + 0],
                     attrib.vertices[3 * index.vertex_index + 1],
                     attrib.vertices[3 * index.vertex_index + 2] };

      vertex.texCoord = { attrib.texcoords[2 * index.texcoord_index + 0],
                          1.0f -
                            attrib.texcoords[2 * index.texcoord_index + 1] };

      vertex.color = { 1.0f, 1.0f, 1.0f };

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(verticies.size());
        verticies.push_back(vertex);
      }

      indicies.push_back(uniqueVertices[vertex]);
    }
  }
  return { verticies, indicies };
}

int
main()
{
  glfwInit();
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(800, 600, "vkTest", nullptr, nullptr);

  Image pixels = createTextureImage("textures/viking_room.png");
  Model house = loadModel("models/viking_room.obj");
  vcc::VCEngine app(800, 600, "vkTest", window);
  vcc::Setup setup(app);
  vcc::Task task(setup, app);
  task.run(pixels.pixels,
           { pixels.width, pixels.height },
           pixels.size,
           house.verticies,
           house.indicies);
  return EXIT_SUCCESS;
}
