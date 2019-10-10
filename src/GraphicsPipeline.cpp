// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : GraphicsPipeline.cpp
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

#include "GraphicsPipeline.h"
#include "RenderPass.h"

#include <stdexcept>
#include <algorithm>

namespace dw {
  GraphicsPipeline::operator VkPipeline() const {
    return m_pipeline;
  }

  GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& o) noexcept
    : m_device(o.m_device),
      m_pipeline(o.m_pipeline) {
    o.m_pipeline = nullptr;
  }

  GraphicsPipeline::GraphicsPipeline(LogicalDevice& device)
    : m_device(device) {
  }

  GraphicsPipeline::~GraphicsPipeline() {
    if (m_pipeline)
      vkDestroyPipeline(m_device, m_pipeline, nullptr);
  }

}
