// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Buffer.h
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

#ifndef DW_VK_BUFFER_H
#define DW_VK_BUFFER_H

#include "LogicalDevice.h"

namespace dw {
  // This is an internal class and not used outside. yet.
  class MemoryAllocator;

  CREATE_DEVICE_DEPENDENT(Buffer)
  public:
    //Buffer(LogicalDevice& device) : m_device(device) {}

    Buffer(LogicalDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    ~Buffer();

    static Buffer CreateStaging(LogicalDevice& device, VkDeviceSize size);
    static Buffer CreateVertex(LogicalDevice& device, VkDeviceSize size, bool fromStaging = true);
    static Buffer CreateIndex(LogicalDevice& device, VkDeviceSize size, bool fromStaging = true);
    static Buffer CreateUniform(LogicalDevice& device, VkDeviceSize size, bool fromStaging = false);
    // TODO other buffer types e.g. uniform buffers
    
    operator VkBuffer() const;
    operator VkBuffer();

    NO_DISCARD void* map();
    void unMap();

    NO_DISCARD VkDescriptorBufferInfo const& getDescriptorInfo() const;
    NO_DISCARD VkDeviceSize getSize() const;

  private:
    void back(MemoryAllocator& allocator, VkMemoryPropertyFlags memFlags);
    VkDeviceMemory m_memory{ nullptr };
    VkDeviceSize m_memSize{ 0 };
    VkDescriptorBufferInfo m_info{};
    bool m_isMapped{ false };
  };
}

#endif
