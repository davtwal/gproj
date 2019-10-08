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
#include "InputHandler.h"

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

#define _USE_MATH_DEFINES
#include <math.h>

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
    // 0: Ground plane
    {
      std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {0, 0, 1}, {0.7f, 0.7f, 0.7f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, 0.0f}, {0, 0, 1}, {0.7f, 0.7f, 0.7f}},// {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, 0.0f}, {0, 0, 1}, {0.7f, 0.7f, 0.7f}},// {1.0f, 1.0f}},
        {{-0.5f,  0.5f, 0.0f}, {0, 0, 1}, {0.7f, 0.7f, 0.7f}},// {0.0f, 1.0f}},
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
        {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {0.5f, 0.5f, 0.5f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {0.5f, 0.5f, 0.5f}},// {1.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {0.5f, 0.5f, 0.5f}},// {1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {0.5f, 0.5f, 0.5f}},// {0.0f, 1.0f}},

        {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {0.5f, 0.5f, 0.5}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {0.5f, 0.5f, 0.5}},// {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {0.5f, 0.5f, 0.5}},// {1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {0.5f, 0.5f, 0.5}},// {0.0f, 1.0f}}

        {{-0.5f, -0.5f, -0.5f}, {0, -1, 0}, {0.5f, 0.5f, 0.5f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0, -1, 0}, {0.5f, 0.5f, 0.5f}},// {1.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f}, {0, -1, 0}, {0.5f, 0.5f, 0.5f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0, -1, 0}, {0.5f, 0.5f, 0.5f}},// {1.0f, 0.0f}},

        {{-0.5f,  0.5f, -0.5f}, {0, 1, 0}, {0.5f, 0.5f, 0.5f}},// {1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 1, 0}, {0.5f, 0.5f, 0.5f}},// {0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0, 1, 0}, {0.5f, 0.5f, 0.5f}},// {1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 1, 0}, {0.5f, 0.5f, 0.5f}},// {0.0f, 1.0f}}

        {{-0.5f, -0.5f, -0.5f}, {-1, 0, 0}, {0.5f, 0.5f, 0.5f}},// {0.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {-1, 0, 0}, {0.5f, 0.5f, 0.5f}},// {1.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {-1, 0, 0}, {0.5f, 0.5f, 0.5f}},// {0.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {-1, 0, 0}, {0.5f, 0.5f, 0.5f}},// {1.0f, 1.0f}},

        {{ 0.5f, -0.5f, -0.5f}, {1, 0, 0}, {0.5f, 0.5f, 0.5f}},// {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {1, 0, 0}, {0.5f, 0.5f, 0.5f}},// {0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {1, 0, 0}, {0.5f, 0.5f, 0.5f}},// {1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {1, 0, 0}, {0.5f, 0.5f, 0.5f}},// {0.0f, 1.0f}}
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
    GLFWControl::Init();
    // open window
    m_window = new GLFWWindow(800, 800, "hey lol");
    m_inputHandler = new InputHandler(*m_window);

    m_window->setInputHandler(m_inputHandler);

    m_renderer = util::make_ptr<Renderer>();

    m_renderer->initGeneral(m_window);
    m_renderer->initSpecific();

    // fill scene
    auto  currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - std::chrono::high_resolution_clock::now()).count();

    m_scene.reserve(5);
    m_scene.emplace_back(util::make_ptr<Object>(m_renderer->getMeshManager().getMesh(0)))
      ->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ 20, 20, 20 });
      o.setPosition({ 0, 0, -0.5f });
      //o.setRotation(glm::angleAxis(time * glm::radians(90.f), glm::vec3{ 0.f, 0.f, 1.f }));
    };

    m_scene.emplace_back(util::make_ptr<Object>(m_renderer->getMeshManager().getMesh(2)))
      ->m_behavior = [](Object& o, float time, float dt) {
      o.setPosition({ 0, -.5f * sin(time), 0 });
      //o.setRotation(glm::angleAxis(time * glm::radians(90.f), glm::vec3{ 0.f, 0.f, 1.f }));
    };

    m_scene.emplace_back(util::make_ptr<Object>(m_renderer->getMeshManager().getMesh(1)))
      ->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ .5f, .5f, .5f });
      o.setPosition({ 0, 0, .5f * cos(time) });
      o.setRotation(glm::angleAxis(time * 2 * glm::radians(90.f), glm::vec3{ 1.f, 0.f, 0.f }));
    };

    m_scene.emplace_back(util::make_ptr<Object>(m_renderer->getMeshManager().getMesh(1)))
      ->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ .1f, .1f, .1f });
      o.setPosition({ 2 * sqrt(2) * cos(3 * time), 2 * sqrt(2) * sin(3 * time), 2 });
    };

    m_scene.emplace_back(util::make_ptr<Object>(m_renderer->getMeshManager().getMesh(1)))
      ->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ .1f, .1f, .1f });
      o.setPosition({ -2 * sqrt(2) * cos(3 * time), -2 * sqrt(2) * sin(3 * time), 2 });
    };

    m_camera
      .setNearDepth(0.1f)
      .setFarDepth(30.f)
      .setEyePos({ M_SQRT2 * 4.5f, 0, 7.0f })
      .setLookAt({ 0.f, 0.f, 0.f })
      .setFOVDeg(45.f);

    m_renderer->setCamera(m_camera);

    m_lights.push_back({
      {2, 2, 2},
      {-1, -1, -1},
      {1, 1, 1},
      5
      });

    m_lights.push_back({
      {-2, -2, -2},
      {1, 1, 1},
      {1, 1, 1},
      5
      });

    m_renderer->setDynamicLights({ m_lights.begin(), m_lights.end() });


    Renderer::SceneContainer scene;
    scene.reserve(m_scene.size());
    for (auto& obj : m_scene) {
      scene.push_back(*obj);
    }
    m_renderer->setScene(scene);

    m_startTime = ClockType::now();

    m_inputHandler->registerKeyFunction(GLFW_KEY_ESCAPE, [this]() {
      m_window->setShouldClose(true);
      },
      nullptr
    );   

    return 0;
  }

  int Application::loop() {
    ClockType::time_point prevTime = ClockType::now();
    ClockType::time_point curTime = ClockType::now();
    while (!m_renderer->done()) {
      GLFWControl::Poll();
      // input check'
      static constexpr float STEP_VALUE = 0.005f;
      static constexpr float RADIUS = M_SQRT2 * 4.5f;
      static float cameraCurrentT = 0.f;

      // Camera Movement Part 1:
      // W = Move up
      // S = Move down
      // A = Rotate around origin (+t)
      // D = Rotate around origin (-t)

      if(m_inputHandler->getKeyState(GLFW_KEY_A)) {
        cameraCurrentT += STEP_VALUE;
        m_camera.setEyePos(glm::vec3{ RADIUS * cos(cameraCurrentT), RADIUS * sin(cameraCurrentT), m_camera.getEyePos().z });
        m_camera.setLookAt({ 0, 0, 0 });
      }

      if(m_inputHandler->getKeyState(GLFW_KEY_D)) {
        cameraCurrentT -= STEP_VALUE;
        m_camera.setEyePos(glm::vec3{ RADIUS * cos(cameraCurrentT), RADIUS * sin(cameraCurrentT), m_camera.getEyePos().z });
        m_camera.setLookAt({ 0, 0, 0 });
      }

      prevTime = curTime;
      curTime = ClockType::now();

      float time = DurationType(curTime - m_startTime).count();
      float dt = DurationType(curTime - prevTime).count();

      for(auto& obj : m_scene) {
        obj->callBehavior(time, dt);
      }

      m_lights[0].m_position = { 2 * sqrt(2) * cos(3*time), 2 * sqrt(2) * sin(3*time), 2};
      m_lights[1].m_position = { -2 * sqrt(2) * cos(3* time), -2 * sqrt(2) * sin(3*time), 2 };

      m_renderer->drawFrame();
    }

    return 0;
  }

  int Application::shutdown() {
    m_scene.clear();

    m_renderer->shutdown();

    delete m_inputHandler; m_inputHandler = nullptr;
    delete m_window;

    GLFWControl::Shutdown();
    return 0;
  }
}
