// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : PipelineLayout.h
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

#ifndef DW_PIPELINE_LAYOUT_H
#define DW_PIPELINE_LAYOUT_H

#include "LogicalDevice.h"
namespace dw {
  CREATE_DEVICE_DEPENDENT(PipelineLayout)
  public:
    PipelineLayout(LogicalDevice& device);
    ~PipelineLayout();

    operator VkPipelineLayout() const;

  private:
    VkPipelineLayout m_layout;
  };
}

#endif
