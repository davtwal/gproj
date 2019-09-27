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
#include "MeshManager.h"

#include <cassert>
#include <algorithm>
#include <unordered_map>
#include <chrono>

namespace dw {

  void MeshManager::clear() {
    m_loadedMeshes.clear();
    m_curKey = 0;
  }

  MeshManager::MeshKey MeshManager::addMesh(std::vector<Vertex> verts, std::vector<uint32_t> indices) {
    m_loadedMeshes.try_emplace(m_curKey, std::move(verts), std::move(indices));
    return m_curKey++;
  }

  util::Ref<Mesh> MeshManager::getMesh(MeshKey key) {
    return m_loadedMeshes.at(key);
  }

  void MeshManager::uploadMeshes(Renderer& renderer) {
    renderer.uploadMeshes(m_loadedMeshes);
  }

  void MeshManager::loadBasicMeshes() {
    std::vector<Vertex> vertices = {
      {{-0.5f, -0.5f, 0.0f},  {1.0f, 0.0f, 0.0f}},// {0.0f, 0.0f}},
      {{0.5f, -0.5f, 0.0f},   {0.0f, 1.0f, 0.0f}},// {1.0f, 0.0f}},
      {{0.5f, 0.5f, 0.0f},    {0.0f, 0.0f, 1.0f}},// {1.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.0f},   {1.0f, 1.0f, 1.0f}},// {0.0f, 1.0f}},

      {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},// {0.0f, 0.0f}},
      {{0.5f, -0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}},// {1.0f, 0.0f}},
      {{0.5f, 0.5f, -0.5f},   {0.0f, 0.0f, 1.0f}},// {1.0f, 1.0f}},
      {{-0.5f, 0.5f, -0.5f},  {1.0f, 1.0f, 1.0f}},// {0.0f, 1.0f}}
    };

    std::vector<uint32_t> indices = {
      0, 1, 2, 2, 3, 0,
      4, 5, 6, 6, 7, 4
    };

    addMesh(vertices, indices);
  }


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
    auto  currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - std::chrono::high_resolution_clock::now()).count();

    m_triangleObject = util::make_ptr<Object>(m_renderer->getMeshManager().getMesh(0));
    m_triangleObject->setPosition({ 0, .5f * sin(time), 0 });
    m_triangleObject->setRotation({ time * glm::radians(90.f), {0.f, 0.f, 1.f} });

    //m_triangleObject->setPosition()

    m_camera
      .setNearDepth(0.1f)
      .setFarDepth(10.f)
      .setEyePos({ 2.f, 2.f, 2.f })
      .setLookAt({ 0.f, 0.f, 0.f })
      .setFOVDeg(45.f);
      //.setDimensions(800, 800);

    m_renderer->setScene(m_camera, {*m_triangleObject});

    return 0;
  }

  int Application::loop() const {
    static auto startTime = std::chrono::high_resolution_clock::now();

    while (!m_renderer->done()) {
      auto  currentTime = std::chrono::high_resolution_clock::now();
      float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

      m_triangleObject->setPosition({ 0, .5f * sin(time), 0 });
      m_triangleObject->setRotation(glm::angleAxis(time * glm::radians(90.f), glm::vec3{0.f, 0.f, 1.f}));


      //m_renderer->setScene(m_camera, { *m_triangleObject });

      m_renderer->drawFrame();
    }

    return 0;
  }

  int Application::shutdown() {
    m_triangleObject.reset();

    m_renderer->shutdown();
    return 0;
  }
}
