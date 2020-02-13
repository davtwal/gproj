// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : MemoryAllocator.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 09m 26d
// * Last Altered: 2019y 09m 26d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "render/MemoryAllocator.h"
#include <stdexcept>

namespace dw {
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
