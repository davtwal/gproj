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
    {
      std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {0, 0, 1}, {1.0f, 0.0f, 0.0f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, 0.0f}, {0, 0, 1}, {0.0f, 1.0f, 0.0f}},// {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, 0.0f}, {0, 0, 1}, {0.0f, 0.0f, 1.0f}},// {1.0f, 1.0f}},
        {{-0.5f,  0.5f, 0.0f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f}},// {0.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {0, 0, 1}, {1.0f, 0.0f, 0.0f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0, 0, 1}, {0.0f, 1.0f, 0.0f}},// {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 0, 1}, {0.0f, 0.0f, 1.0f}},// {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f}},// {0.0f, 1.0f}}
      };

      std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
      };

      addMesh(vertices, indices);
    }

    // 1: Non-face-normal cube
    {
      std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1.0f, 0.0f, 0.0f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {0.0f, 1.0f, 0.0f}},// {1.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {0.0f, 0.0f, 1.0f}},// {1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1.0f, 1.0f, 1.0f}},// {0.0f, 1.0f}},

        {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1.0f, 0.0f, 0.0f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {0.0f, 1.0f, 0.0f}},// {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {0.0f, 0.0f, 1.0f}},// {1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f}},// {0.0f, 1.0f}}
      };

      vertices[0].normal = glm::normalize(glm::vec3{ 0, 0, -1 } + glm::vec3{ 0, -1, 0 } + glm::vec3{ -1, 0, 0 });
      vertices[1].normal = glm::normalize(glm::vec3{ 0, 0, -1 } + glm::vec3{ 0, -1, 0 } + glm::vec3{  1, 0, 0 });
      vertices[2].normal = glm::normalize(glm::vec3{ 0, 0, -1 } + glm::vec3{ 0,  1, 0 } + glm::vec3{ -1, 0, 0 });
      vertices[3].normal = glm::normalize(glm::vec3{ 0, 0, -1 } + glm::vec3{ 0,  1, 0 } + glm::vec3{  1, 0, 0 });
      vertices[4].normal = glm::normalize(glm::vec3{ 0, 0,  1 } + glm::vec3{ 0, -1, 0 } + glm::vec3{ -1, 0, 0 });
      vertices[5].normal = glm::normalize(glm::vec3{ 0, 0,  1 } + glm::vec3{ 0, -1, 0 } + glm::vec3{  1, 0, 0 });
      vertices[6].normal = glm::normalize(glm::vec3{ 0, 0,  1 } + glm::vec3{ 0,  1, 0 } + glm::vec3{ -1, 0, 0 });
      vertices[7].normal = glm::normalize(glm::vec3{ 0, 0,  1 } + glm::vec3{ 0,  1, 0 } + glm::vec3{  1, 0, 0 });

      std::vector<uint32_t> indices = {
        0, 2, 1, 1, 2, 3, // -Z
        4, 5, 6, 5, 7, 6, // +Z
        0, 1, 4, 1, 5, 4, // -Y
        6, 3, 2, 3, 6, 7, // +Y
        2, 0, 4, 2, 4, 6, // -X
        1, 3, 7, 5, 1, 7  // +X
      };

      addMesh(vertices, indices);
    }

    // 2: Face-normal cube
    {
      std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1.0f, 0.0f, 0.0f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {0.0f, 1.0f, 0.0f}},// {1.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {0.0f, 0.0f, 1.0f}},// {1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1.0f, 1.0f, 1.0f}},// {0.0f, 1.0f}},

        {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1.0f, 0.0f, 0.0f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {0.0f, 1.0f, 0.0f}},// {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {0.0f, 0.0f, 1.0f}},// {1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f}},// {0.0f, 1.0f}}

        {{-0.5f, -0.5f, -0.5f}, {0, -1, 0}, {1.0f, 0.0f, 0.0f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0, -1, 0}, {0.0f, 1.0f, 0.0f}},// {1.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f}, {0, -1, 0}, {1.0f, 0.0f, 0.0f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0, -1, 0}, {0.0f, 1.0f, 0.0f}},// {1.0f, 0.0f}},

        {{-0.5f,  0.5f, -0.5f}, {0, 1, 0}, {0.0f, 0.0f, 1.0f}},// {1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 1, 0}, {1.0f, 1.0f, 1.0f}},// {0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0, 1, 0}, {0.0f, 0.0f, 1.0f}},// {1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 1, 0}, {1.0f, 1.0f, 1.0f}},// {0.0f, 1.0f}}

        {{-0.5f, -0.5f, -0.5f}, {-1, 0, 0}, {1.0f, 0.0f, 0.0f}},// {0.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {-1, 0, 0}, {0.0f, 0.0f, 1.0f}},// {1.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {-1, 0, 0}, {1.0f, 0.0f, 0.0f}},// {0.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {-1, 0, 0}, {0.0f, 0.0f, 1.0f}},// {1.0f, 1.0f}},

        {{ 0.5f, -0.5f, -0.5f}, {1, 0, 0}, {0.0f, 1.0f, 0.0f}},// {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {1, 0, 0}, {1.0f, 1.0f, 1.0f}},// {0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {1, 0, 0}, {0.0f, 1.0f, 0.0f}},// {1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {1, 0, 0}, {1.0f, 1.0f, 1.0f}},// {0.0f, 1.0f}}
      };

      std::vector<uint32_t> indices = {
        0, 2, 1, 1, 2, 3, // need to reverse winding here
        4, 5, 6, 5, 7, 6,
        8, 9, 10, 9, 11, 10,
        14, 13, 12, 13, 14, 15,
        17, 16, 18, 17, 18, 19,
        20, 21, 22, 21, 23, 22
      };

      addMesh(vertices, indices);
    }
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

    m_scene.reserve(3);
    m_scene.emplace_back(util::make_ptr<Object>(m_renderer->getMeshManager().getMesh(0)))
      ->m_behavior = [](Object& o, float time, float dt) {
      o.setPosition({ 0, .5f * sin(time), 0 });
      o.setRotation(glm::angleAxis(time * glm::radians(90.f), glm::vec3{ 0.f, 0.f, 1.f }));
    };

    m_scene.emplace_back(util::make_ptr<Object>(m_renderer->getMeshManager().getMesh(2)))
      ->m_behavior = [](Object& o, float time, float dt) {
      o.setPosition({ 0, -.5f * sin(time), 0 });
      o.setRotation(glm::angleAxis(time * glm::radians(90.f), glm::vec3{ 0.f, 0.f, 1.f }));
    };

    m_scene.emplace_back(util::make_ptr<Object>(m_renderer->getMeshManager().getMesh(1)))
      ->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ .5f, .5f, .5f });
      o.setPosition({ 0, 0, .5f * cos(time) });
      o.setRotation(glm::angleAxis(time * 2 * glm::radians(90.f), glm::vec3{ 1.f, 0.f, 0.f }));
    };

    m_camera
      .setNearDepth(0.1f)
      .setFarDepth(10.f)
      .setEyePos({ 2.f, 2.f, 2.f })
      .setLookAt({ 0.f, 0.f, 0.f })
      .setFOVDeg(45.f);

    m_renderer->setCamera(m_camera);

    Renderer::SceneContainer scene;
    scene.reserve(m_scene.size());
    for (auto& obj : m_scene) {
      scene.push_back(*obj);
    }
    m_renderer->setScene(scene);

    m_startTime = ClockType::now();

    return 0;
  }

  int Application::loop() {
    ClockType::time_point prevTime = ClockType::now();
    ClockType::time_point curTime = ClockType::now();
    while (!m_renderer->done()) {
      prevTime = curTime;
      curTime = ClockType::now();

      float time = DurationType(curTime - m_startTime).count();
      float dt = DurationType(curTime - prevTime).count();

      for(auto& obj : m_scene) {
        obj->callBehavior(time, dt);
      }

      m_renderer->drawFrame();
    }

    return 0;
  }

  int Application::shutdown() {
    m_scene.clear();

    m_renderer->shutdown();
    return 0;
  }
}
