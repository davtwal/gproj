// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : Application.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 13d
// * Last Altered: 2019y 09m 13d
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

#include "Application.h"

#include "VulkanControl.h"
#include "GLFWControl.h"
#include "GLFWWindow.h"

#include "Surface.h"
#include "Swapchain.h"
#include "Framebuffer.h"  // this is used
#include "Trace.h"
#include "Queue.h"
#include "CommandBuffer.h"
#include "RenderPass.h"
#include "Shader.h"
#include "Vertex.h"
#include "Buffer.h"
#include "Mesh.h"

#include <cassert>
#include <algorithm>

namespace dw {
  int Application::parseCommandArgs(int, char**) {
    return 0;
  }

  int Application::run() {
    if (initialize() == 1 || loop() == 1 || shutdown() == 1)
      return 1;

    return 0;
  }

  int Application::initialize() {
    // start renderer
    m_renderer = util::make_ptr<Renderer>();

    m_renderer->initGeneral();
    m_renderer->initSpecific();

    // fill scene
    std::vector<Vertex> vertices = {
      {{-.5, -.5f}, {1, 0, 0}},
      {{.5f, -.5f}, {1, 1, 1}},
      {{.5f, .5f}, {0, 1, 0}},
      {{-.5f, .5f}, {0, 0, 1}},
    };

    std::vector<uint32_t> indices = {
      0, 1, 2, 2, 3, 0
    };

    m_mesh = util::make_ptr<Mesh>(vertices, indices);
    m_triangleObject.m_mesh = m_mesh;

    m_renderer->uploadMeshes({m_mesh});
    m_renderer->setScene({m_triangleObject});

    return 0;
  }

  int Application::loop() const {
    while (!m_renderer->done()) {
      m_renderer->drawFrame();
    }

    return 0;
  }

  int Application::shutdown() {
    m_mesh.reset();

    m_renderer->shutdown();
    return 0;
  }
}
