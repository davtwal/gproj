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
#include <random>

#define _USE_MATH_DEFINES
#include <math.h>

namespace dw {
  void MeshManager::loadBasicMeshes() {
    // 0: Ground plane
    {
      std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {0, 0, 1}, {}, {}, {0, 0}, {0.7f, 0.7f, 0.7f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, 0.0f}, {0, 0, 1}, {}, {}, {1, 0}, {0.7f, 0.7f, 0.7f}},// {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, 0.0f}, {0, 0, 1}, {}, {}, {1, 1}, {0.7f, 0.7f, 0.7f}},// {1.0f, 1.0f}},
        {{-0.5f,  0.5f, 0.0f}, {0, 0, 1}, {}, {}, {0, 1}, {0.7f, 0.7f, 0.7f}},// {0.0f, 1.0f}},
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
        {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {}, {}, {0, 0}, {1.0f, 0.0f, 0.0f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {}, {}, {1, 0}, {0.0f, 1.0f, 0.0f}},// {1.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {}, {}, {0, 1}, {0.0f, 0.0f, 1.0f}},// {1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {}, {}, {1, 1}, {1.0f, 1.0f, 1.0f}},// {0.0f, 1.0f}},

        {{-0.5f, -0.5f,  0.5f}, {0, 0,  1}, {}, {}, {1, 1}, {1.0f, 0.0f, 0.0f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0, 0,  1}, {}, {}, {0, 1}, {0.0f, 1.0f, 0.0f}},// {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0, 0,  1}, {}, {}, {1, 0}, {0.0f, 0.0f, 1.0f}},// {1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 0,  1}, {}, {}, {0, 0}, {1.0f, 1.0f, 1.0f}},// {0.0f, 1.0f}}
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
        {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {}, {}, {0, 0}, {0.5f, 0.5f, 0.5f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {}, {}, {1, 0}, {0.5f, 0.5f, 0.5f}},// {1.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {}, {}, {0, 1}, {0.5f, 0.5f, 0.5f}},// {1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {}, {}, {1, 1}, {0.5f, 0.5f, 0.5f}},// {0.0f, 1.0f}},

        {{-0.5f, -0.5f,  0.5f}, {0, 0,  1}, {}, {}, {1, 1}, {0.5f, 0.5f, 0.5}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0, 0,  1}, {}, {}, {0, 1}, {0.5f, 0.5f, 0.5}},// {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0, 0,  1}, {}, {}, {1, 0}, {0.5f, 0.5f, 0.5}},// {1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 0,  1}, {}, {}, {0, 0}, {0.5f, 0.5f, 0.5}},// {0.0f, 1.0f}}

        {{-0.5f, -0.5f, -0.5f}, {0, -1, 0}, {}, {}, {0, 0}, {0.5f, 0.5f, 0.5f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0, -1, 0}, {}, {}, {1, 0}, {0.5f, 0.5f, 0.5f}},// {1.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f}, {0, -1, 0}, {}, {}, {0, 1}, {0.5f, 0.5f, 0.5f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0, -1, 0}, {}, {}, {1, 1}, {0.5f, 0.5f, 0.5f}},// {1.0f, 0.0f}},

        {{-0.5f,  0.5f, -0.5f}, {0,  1, 0}, {}, {}, {1, 1}, {0.5f, 0.5f, 0.5f}},// {1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0,  1, 0}, {}, {}, {0, 1}, {0.5f, 0.5f, 0.5f}},// {0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0,  1, 0}, {}, {}, {1, 0}, {0.5f, 0.5f, 0.5f}},// {1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0,  1, 0}, {}, {}, {0, 0}, {0.5f, 0.5f, 0.5f}},// {0.0f, 1.0f}}

        {{-0.5f, -0.5f, -0.5f}, {-1, 0, 0}, {}, {}, {0, 0}, {0.5f, 0.5f, 0.5f}},// {0.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {-1, 0, 0}, {}, {}, {1, 0}, {0.5f, 0.5f, 0.5f}},// {1.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {-1, 0, 0}, {}, {}, {0, 1}, {0.5f, 0.5f, 0.5f}},// {0.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {-1, 0, 0}, {}, {}, {1, 1}, {0.5f, 0.5f, 0.5f}},// {1.0f, 1.0f}},

        {{ 0.5f, -0.5f, -0.5f}, { 1, 0, 0}, {}, {}, {1, 1}, {0.5f, 0.5f, 0.5f}},// {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 1, 0, 0}, {}, {}, {0, 1}, {0.5f, 0.5f, 0.5f}},// {0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, { 1, 0, 0}, {}, {}, {1, 0}, {0.5f, 0.5f, 0.5f}},// {1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 1, 0, 0}, {}, {}, {0, 0}, {0.5f, 0.5f, 0.5f}},// {0.0f, 1.0f}}
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

    // load the objects that i want
    m_meshManager.loadBasicMeshes();

    m_meshManager.load("data/objects/lamp.obj");
    m_meshManager.load("data/objects/teapot.obj", true);

    m_meshManager.uploadMeshes(*m_renderer);

    // fill scene
    auto  currentTime = std::chrono::high_resolution_clock::now();
    float curTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - std::chrono::high_resolution_clock::now()).count();

    // Ground plane
    m_scene.emplace_back(util::make_ptr<Object>(m_meshManager.getMesh(0)))
      ->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ 20, 20, 20 });
      o.setPosition({ 0, 0, -0.5f });
    };

    // Random objects
    m_scene.emplace_back(util::make_ptr<Object>(m_meshManager.getMesh(3)))
      ->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ 2, 2, 2 });
      o.setPosition({ 0, 1.f, 1.f });
      o.setRotation(glm::angleAxis(time * glm::radians(90.f), glm::vec3{0, 0, 1}) * glm::angleAxis(glm::radians(90.f), glm::vec3{ 1.f, 0.f, 0.f }));
    };

    m_scene.emplace_back(util::make_ptr<Object>(m_meshManager.getMesh(4)))
      ->m_behavior = [](Object& o, float time, float dt) {
      o.setPosition({ 0, -1.f, 0 });
      o.setRotation(glm::angleAxis(time * glm::radians(90.f), glm::vec3{ 0, 0, 1 }) * glm::angleAxis(glm::radians(90.f), glm::vec3{ 1.f, 0.f, 0.f }));
    };

    // Flying color cube
    m_scene.emplace_back(util::make_ptr<Object>(m_meshManager.getMesh(1)))
      ->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ .5f, .5f, .5f });
      o.setPosition({ 0, 0, .5f * cos(time) });
      o.setRotation(glm::angleAxis(time * 2 * glm::radians(90.f), glm::vec3{ 1.f, 0.f, 0.f }));
    };

    // Lights
    m_scene.emplace_back(util::make_ptr<Object>(m_meshManager.getMesh(1)))
      ->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ .1f, .1f, .1f });
      o.setPosition({ 2 * sqrt(2) * cos(3 * time), 2 * sqrt(2) * sin(3 * time), 2 });
    };

    m_scene.emplace_back(util::make_ptr<Object>(m_meshManager.getMesh(1)))
      ->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ .1f, .1f, .1f });
      o.setPosition({ -2 * sqrt(2) * cos(3 * time), -2 * sqrt(2) * sin(3 * time), 2 });
    };

    m_camera
      .setNearDepth(0.1f)
      .setFarDepth(50.f)
      .setEyePos({ M_SQRT2 * 4.5f, 0, 7.0f })
      .setLookAt({ 0.f, 0.f, 0.f })
      .setFOVDeg(45.f);

    m_renderer->setCamera(m_camera);

    static constexpr unsigned MAX_DYNAMIC_LIGHTS = 128;

    std::mt19937 rand_dev(time(NULL));
    std::uniform_real_distribution<float> pos_dist(-9, 9);
    std::uniform_real_distribution<float> color_dist(0, 1);
    std::uniform_real_distribution<float> atten_dist(0.5f, 4.f);

    for(uint32_t i = 0; i < MAX_DYNAMIC_LIGHTS; ++i) {
      Light l;
      l.setPosition(glm::vec3(pos_dist(rand_dev), pos_dist(rand_dev), abs(pos_dist(rand_dev) / 2)));
      l.setDirection(normalize(-l.getPosition()));
      l.setColor(glm::vec3(color_dist(rand_dev), color_dist(rand_dev), color_dist(rand_dev)));// / glm::vec3(i + 3);
      l.setAttenuation(glm::vec3(1.f + atten_dist(rand_dev), atten_dist(rand_dev), 1.f));
      l.setLocalRadius(20);
      l.setType(Light::Type::Point);

      m_lights.push_back(l);
    }

    m_lights[0].setAttenuation(glm::vec3(1.f, 0.5f, 0.1f));
    m_lights[1].setAttenuation(glm::vec3(1.f, 0.5f, 0.1f));

    for (uint32_t i = 2; i < m_lights.size(); ++i) {
      auto& light = m_lights[i];
      m_scene.emplace_back(util::make_ptr<Object>(m_meshManager.getMesh(1)))
        ->m_behavior = [](Object& o, float time, float dt) {
        o.setScale({ .1f, .1f, .1f });
        //o.setPosition(light.m_position);
        o.setRotation(glm::angleAxis(time * 2 * glm::radians(90.f), glm::vec3{ 1.f, 0.f, 0.f }));
      };
    }

    m_renderer->setLocalLights({ m_lights.begin(), m_lights.end() });

    ShadowedLight globalLight;
    globalLight.setPosition({ 5, 5, 5 })
      .setDirection(glm::normalize(glm::vec3(-1, -1, -1)))
      .setColor({0.5f, 0.5f, 0.5f});



    m_renderer->setGlobalLights({
      globalLight
    });

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
      static constexpr float STEP_VALUE = 1.f;
      static float RADIUS = M_SQRT2 * 4.5f;
      static float cameraCurrentT = 0.f;

      prevTime = curTime;
      curTime = ClockType::now();

      float timeCount = DurationType(curTime - m_startTime).count();
      float dt = DurationType(curTime - prevTime).count();
      char titleBuff[256] = { '\0' };
      sprintf(titleBuff, "hey lol - %.0f fps", 1 / dt);
      glfwSetWindowTitle(m_window->getHandle(), titleBuff);

      if (m_inputHandler->getKeyState(GLFW_KEY_A)) {
        cameraCurrentT += STEP_VALUE * dt;
      }

      if (m_inputHandler->getKeyState(GLFW_KEY_D)) {
        cameraCurrentT -= STEP_VALUE * dt;
      }

      float cameraZ = m_camera.getEyePos().z;

      if(m_inputHandler->getKeyState(GLFW_KEY_W)) {
        cameraZ += STEP_VALUE * dt * 2;
      }

      if(m_inputHandler->getKeyState(GLFW_KEY_S)) {
        cameraZ -= STEP_VALUE * dt * 2;
      }

      if (m_inputHandler->getKeyState(GLFW_KEY_EQUAL)) {
        RADIUS += STEP_VALUE * dt;
      }

      if (m_inputHandler->getKeyState(GLFW_KEY_MINUS)) {
        RADIUS -= STEP_VALUE * dt;
      }

      m_camera.setEyePos(glm::vec3{ RADIUS * cos(cameraCurrentT), RADIUS * sin(cameraCurrentT), cameraZ });
      m_camera.setLookAt({ 0, 0, 0 });

      for(auto& obj : m_scene) {
        obj->callBehavior(timeCount, dt);
      }

      m_lights[0].setPosition({ 2 * sqrt(2) * cos(3* timeCount), 2 * sqrt(2) * sin(3* timeCount), 2});
      m_lights[1].setPosition({ -2 * sqrt(2) * cos(3* timeCount), -2 * sqrt(2) * sin(3* timeCount), 2 });

      std::mt19937 rand_dev(time(NULL));
      std::uniform_real_distribution<float> shift(-1.f, 1.f);
      std::uniform_real_distribution<float> color(-0.5f, 0.5f);
      for (uint32_t i = 2; i < m_lights.size(); ++i) {
        auto& light = m_lights[i];
        auto& obj = m_scene[i + 4];
        light.setPosition(light.getPosition() + glm::vec3(shift(rand_dev) * dt, shift(rand_dev) * dt, shift(rand_dev) * dt));
        light.setPosition(glm::clamp(light.getPosition(), glm::vec3(-9.f, -9.f, 0.1f), glm::vec3(9.f)));
        light.setColor(light.getColor() + glm::vec3(color(rand_dev) * dt, color(rand_dev) * dt, color(rand_dev) * dt));
        light.setColor(glm::clamp(light.getColor(), 0.f, 1.f));
        obj->setPosition(light.getPosition());
      }

      m_renderer->drawFrame();
    }

    return 0;
  }

  int Application::shutdown() {
    m_scene.clear();

    m_meshManager.clear();
    m_renderer->shutdown();

    delete m_inputHandler; m_inputHandler = nullptr;
    delete m_window;

    GLFWControl::Shutdown();
    return 0;
  }
}
