// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Buffer.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 24d
// * Last Altered: 2019y 09m 24d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * Description :
// *
// *
// *
// *
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

#include "Buffer.h"
#include "MemoryAllocator.h"

#include <cassert>
#include <stdexcept>

namespace dw {
  Buffer Buffer::CreateStaging(LogicalDevice& device, VkDeviceSize size) {
    return Buffer(device,
                  size,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  }

  Buffer Buffer::CreateIndex(LogicalDevice& device, VkDeviceSize size, bool fromStaging) {
    VkFlags flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | (fromStaging ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0);
    return Buffer(device, size, flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  }

  Buffer Buffer::CreateVertex(LogicalDevice& device, VkDeviceSize size, bool fromStaging) {
    VkFlags flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | (fromStaging ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0);
    return Buffer(device, size, flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  }

  Buffer Buffer::CreateUniform(LogicalDevice& device, VkDeviceSize size, bool fromStaging) {
    VkFlags flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | (fromStaging ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0);
    return Buffer(device, size, flags, fromStaging
      ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
      : VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  }

  Buffer::operator VkBuffer() const {
    return m_info.buffer;
  }

  Buffer::operator VkBuffer() {
    return m_info.buffer;
  }

  VkDeviceMemory Buffer::getMemory() const {
    return m_memory;
  }

  Buffer::Buffer(LogicalDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    : m_device(device) {
    VkBufferCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      nullptr,
      0,  // TODO if we want sparse binding
      size,
      usage,
      VK_SHARING_MODE_EXCLUSIVE,
      0,    // ignored if VK_SHARING_MODE_EXCLUSIVE
      nullptr // ignored if VK_SHARING_MODE_EXCLUSIVE
    };

    vkCreateBuffer(m_device, &createInfo, nullptr, &m_info.buffer);
    if (!m_info.buffer)
      throw std::runtime_error("could not create buffer");

    m_info.range = size;

    MemoryAllocator allocator(getOwningPhysical());
    back(allocator, properties);
  }

  Buffer::Buffer(Buffer&& o) noexcept
    : m_device(o.m_device),
      m_memory(o.m_memory),
      m_memSize(o.m_memSize),
      m_info(o.m_info),
      m_isMapped(o.m_isMapped) {
    o.m_memory = nullptr;
    o.m_memSize = 0;
    o.m_isMapped = false;
    o.m_info = {nullptr, 0, 0};
  }

  void Buffer::back(MemoryAllocator& allocator, VkMemoryPropertyFlags memFlags) {
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(m_device, m_info.buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo = {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      nullptr,
      memReqs.size,
      allocator.GetAppropriateMemType(memReqs.memoryTypeBits, memFlags)
    };

    if(vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS || !m_memory)
      throw std::runtime_error("could not allocate memory for buffer");

    m_memSize = memReqs.size;

    if (vkBindBufferMemory(m_device, m_info.buffer, m_memory, 0) != VK_SUCCESS)
      throw std::runtime_error("could not bind memory to buffer");
  }

  Buffer::~Buffer() {
    if (m_memory) {
      vkFreeMemory(m_device, m_memory, nullptr);
      m_memory = nullptr;
    }

    if (m_info.buffer) {
      vkDestroyBuffer(m_device, m_info.buffer, nullptr);
      m_memory = nullptr;
    }
  }

  void* Buffer::map() {
    void* ret = nullptr;
    if (m_memory && !m_isMapped) {
      if (vkMapMemory(m_device, m_memory, 0, m_memSize, 0, &ret) != VK_SUCCESS)
        return nullptr;

      m_isMapped = true;
    }
    return ret;
  }

  void Buffer::unMap() {
    if (m_isMapped) {
      vkUnmapMemory(m_device, m_memory);
      m_isMapped = false;
    }
  }

  VkDescriptorBufferInfo const& Buffer::getDescriptorInfo() const {
    return m_info;
  }

  VkDeviceSize Buffer::getSize() const {
    return m_memSize;
  }
}
