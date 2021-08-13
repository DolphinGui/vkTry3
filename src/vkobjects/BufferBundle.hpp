#pragma once

#include "vk_mem_alloc.h"
#include <atomic>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>
namespace vcc {
class VCEngine;
/*because these are really pointers to allocations, this uses move semantics.*/

class BufferBundle
{
public:
  vk::Buffer buffer;
  VmaAllocation mem;
  const VmaAllocator alloc; // non-owning
  BufferBundle(const vk::BufferCreateInfo& bufferInfo,
               const VmaAllocationCreateInfo& memInfo,
               const VmaAllocator& alloc)
    : alloc(alloc)
  {
    auto result =
      vmaCreateBuffer(alloc,
                      reinterpret_cast<const VkBufferCreateInfo*>(&bufferInfo),
                      &memInfo,
                      reinterpret_cast<VkBuffer*>(&buffer),
                      &mem,
                      nullptr);
    if (vk::Result(result) != vk::Result::eSuccess)
      throw std::runtime_error("allocbuffer failed.");
#ifndef NDEBUG
    std::cout << "buffer " << buffer << " constructed directly\n";
    debugCount++;
#endif
  }

  BufferBundle(vk::Buffer buffer,
               VmaAllocation mem,
               const VmaAllocator& allocator) noexcept
    : buffer(buffer)
    , mem(mem)
    , alloc(allocator)
  {
#ifndef NDEBUG
    std::cout << "buffer " << buffer << " constructed via copy\n";
    debugCount++;
#endif
  }

  BufferBundle(BufferBundle&& other) noexcept
    : buffer(other.buffer)
    , mem(other.mem)
    , alloc(other.alloc)
  {
    other.buffer = nullptr;
    other.mem = nullptr;
  }

  BufferBundle(const BufferBundle& other) = delete;

  ~BufferBundle()
  {
    vmaDestroyBuffer(alloc, static_cast<VkBuffer>(buffer), mem);
#ifndef NDEBUG
    std::cout << "buffer " << buffer << " destroyed\n";
#endif
  };

  std::pair<vk::Buffer, VmaAllocation> release() noexcept
  {
    auto b = buffer;
    auto m = mem;
    buffer = nullptr;
    mem = nullptr;
    return { b, m };
  }

#ifndef NDEBUG
  inline static int debugCount{};
#endif
};
}
