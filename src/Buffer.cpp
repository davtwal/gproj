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
#include <cassert>
#include <stdexcept>

namespace dw {
  CREATE_PHYSICAL_DEPENDENT(MemoryAllocator)
  public:
    MemoryAllocator(PhysicalDevice& physDev);

    uint32_t GetAppropriateMemType(uint32_t filter, VkMemoryPropertyFlags flags) const;
  };

  MemoryAllocator::MemoryAllocator(PhysicalDevice& physDev)
    : m_physical(physDev) {

  }

  uint32_t MemoryAllocator::GetAppropriateMemType(uint32_t filter, VkMemoryPropertyFlags memProps) const {
    auto& props = m_physical.getMemoryProperties();
    for (uint32_t i = 0; i < props.memoryTypeCount; i++) {
      if ((filter & (1 << i)) && (props.memoryTypes[i].propertyFlags & memProps) == memProps) {
        return i;
      }
    }

    throw std::runtime_error("failed to find suitable memory type!");
  }
}

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

  Buffer::operator VkBuffer() const {
    return m_buffer;
  }

  Buffer::operator VkBuffer() {
    return m_buffer;
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

    vkCreateBuffer(m_device, &createInfo, nullptr, &m_buffer);
    if (!m_buffer)
      throw std::bad_alloc();

    MemoryAllocator allocator(getOwningPhysical());
    back(allocator, properties);
  }

  Buffer::Buffer(Buffer&& o) noexcept
    : m_device(o.m_device),
      m_buffer(o.m_buffer),
      m_memory(o.m_memory),
      m_memSize(o.m_memSize),
      m_isMapped(o.m_isMapped) {
    o.m_buffer = nullptr;
    o.m_memory = nullptr;
    o.m_memSize = 0;
    o.m_isMapped = false;
  }

  void Buffer::back(MemoryAllocator& allocator, VkMemoryPropertyFlags memFlags) {
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(m_device, m_buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo = {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      nullptr,
      memReqs.size,
      allocator.GetAppropriateMemType(memReqs.memoryTypeBits, memFlags)
    };

    if(vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS || !m_memory)
      throw std::bad_alloc();

    m_memSize = memReqs.size;

    if (vkBindBufferMemory(m_device, m_buffer, m_memory, 0) != VK_SUCCESS)
      throw std::bad_alloc();
  }

  Buffer::~Buffer() {
    if (m_memory) {
      vkFreeMemory(m_device, m_memory, nullptr);
      m_memory = nullptr;
    }

    if (m_buffer) {
      vkDestroyBuffer(m_device, m_buffer, nullptr);
      m_memory = nullptr;
    }
  }

  void* Buffer::map() {
    void* ret = nullptr;
    if (m_memory) {
      if (vkMapMemory(m_device, m_memory, 0, m_memSize, 0, &ret) != VK_SUCCESS)
        return nullptr;

      m_isMapped = true;
    }
    return ret;
  }

  void Buffer::unMap() {
    if (m_isMapped) {
      vkUnmapMemory(m_device, m_memory);
    }
  }

}
