// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Abstractions.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 24d
// * Last Altered: 2019y 09m 24d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * Description :
// *  Contains helper functions that dont belong to any one class
// *  and instead do things on their own.
// *
// *
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

#ifndef DW_ABSTRACTIONS_H
#define DW_ABSTRACTIONS_H

struct VkBufferCopy;

namespace dw::abst {
  class Buffer;
  class CommandPool;
  class Queue;

  void StageAndTransferBuffer(VkBufferCopy const& copyRegion);

}

#endif

