// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : MemoryAllocator.h
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

#ifndef DW_MEMORY_ALLOCATOR_H
#define DW_MEMORY_ALLOCATOR_H

#include "PhysicalDevice.h"

namespace dw {
  CREATE_PHYSICAL_DEPENDENT(MemoryAllocator)
  public:
    MemoryAllocator(PhysicalDevice& physDev);
  
    NO_DISCARD uint32_t GetAppropriateMemType(uint32_t filter, VkMemoryPropertyFlags memProps) const;
  };
}

#endif
