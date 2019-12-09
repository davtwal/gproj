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
#include "RenderSteps.h"
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
#include <thread>
#include <atomic>

#include "ImGui.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace dw {
  void MeshManager::loadBasicMeshes() {
    // 0: Ground plane
    {
      std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {0, 0, 1}, {0, 0, 0}, {0, 0, 0}, {0, 0}, {1.f, 1.f, 1.f}},// {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, 0.0f}, {0, 0, 1}, {0, 0, 0}, {0, 0, 0}, {1, 0}, {1.f, 1.f, 1.f}},// {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, 0.0f}, {0, 0, 1}, {0, 0, 0}, {0, 0, 0}, {1, 1}, {1.f, 1.f, 1.f}},// {1.0f, 1.0f}},
        {{-0.5f,  0.5f, 0.0f}, {0, 0, 1}, {0, 0, 0}, {0, 0, 0}, {0, 1}, {1.f, 1.f, 1.f}},// {0.0f, 1.0f}},
      };

      std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0,

        //4, 5, 6, 6, 7, 4
      };

      addMesh(vertices, indices).second.calculateTangents();
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

      addMesh(vertices, indices).second.calculateTangents();;
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

      addMesh(vertices, indices).second.calculateTangents();;
    }

    // 3: Sphere
    {
      std::vector<Vertex> vertices;
      std::vector<uint32_t> indices;

      static constexpr float NUM_LATITUDE_LINES = 20.f;
      static constexpr float NUM_LONGITUDE_LINES = 20.f;
      static constexpr float F_PI = (float)M_PI;
      static constexpr bool  NORMALS_POINT_OUTWARDS = false;

      const auto addVertex = [&](uint32_t m, uint32_t n){
        float pos_coeff = sin(F_PI * m / (NUM_LATITUDE_LINES - 1));

        glm::vec3 pos = { pos_coeff * cos(2 * F_PI * n / NUM_LONGITUDE_LINES),
                    pos_coeff * sin(2 * F_PI * n / NUM_LONGITUDE_LINES),
                    cos(F_PI * m / (NUM_LATITUDE_LINES - 1)) };

        glm::vec3 normal = glm::vec3(0) - pos;

        glm::vec2 texCoord = {0, 0};// { .5f - atan2(-normal.y, -normal.x), acos(-normal.z) / F_PI };

        if constexpr (NORMALS_POINT_OUTWARDS)
          normal = -normal;

        vertices.push_back({
          pos,
          normal,
          {0, 0, 1}, //tangent,
          {0, 1, 0}, //bitangent, must be 0, 1, 0 for the obj to register as skybox
          texCoord,
          {1, 1, 1}
          });
      };

      // POLE
      addVertex(0, 0);
      // Add triangles for this line
      for(uint32_t n = 0; n < NUM_LONGITUDE_LINES - 1; ++n) {
        indices.push_back(0);
      
        if constexpr (NORMALS_POINT_OUTWARDS) {
          indices.push_back(n + 1);
          indices.push_back(n + 2);
        } else {
          indices.push_back(n + 2);
          indices.push_back(n + 1);
        }
      }
      
      indices.push_back(0);
      
      if constexpr (NORMALS_POINT_OUTWARDS) {
        indices.push_back(NUM_LONGITUDE_LINES);
        indices.push_back(1);
      }
      else {
        indices.push_back(1);
        indices.push_back(NUM_LONGITUDE_LINES);
      }

      for(uint32_t m = 1; m < NUM_LATITUDE_LINES - 1; ++m) {
        for(uint32_t n = 0; n < NUM_LONGITUDE_LINES; ++n) {
          if constexpr (NORMALS_POINT_OUTWARDS) {
            if(m < NUM_LATITUDE_LINES - 2) {
              indices.push_back(vertices.size());
              indices.push_back(vertices.size() + NUM_LONGITUDE_LINES - 1);
              indices.push_back(vertices.size() + NUM_LONGITUDE_LINES);

              indices.push_back(vertices.size());
              indices.push_back(vertices.size() + NUM_LONGITUDE_LINES);
              indices.push_back(vertices.size() + 1);
            }
          } else {
            if (m < NUM_LATITUDE_LINES - 2) {
              indices.push_back(vertices.size());
              indices.push_back(vertices.size() + NUM_LONGITUDE_LINES);
              indices.push_back(vertices.size() + NUM_LONGITUDE_LINES - 1);

              indices.push_back(vertices.size());
              indices.push_back(vertices.size() + 1);
              indices.push_back(vertices.size() + NUM_LONGITUDE_LINES);
            }
          }

          addVertex(m, n);

          // create triangle
          // adjacent vertices are:
          /* (m - 1, n    )
           * (m - 1, n + 1)
           * (    m, n - 1)
           * (    m, n + 1)
           * (m + 1, n - 1)
           * (m + 1, n    )
           */
        }
      }

      // POLE
      addVertex(NUM_LATITUDE_LINES - 1, 0);

      // Add triangles for bottom line
      for(uint32_t n = 0; n < NUM_LONGITUDE_LINES - 1; ++n) {
        indices.push_back(vertices.size() - 1);
      
        if constexpr (NORMALS_POINT_OUTWARDS) {
          indices.push_back(vertices.size() - 2 - n);
          indices.push_back(vertices.size() - 3 - n);
        } else {
          indices.push_back(vertices.size() - 3 - n);
          indices.push_back(vertices.size() - 2 - n);
        }
      }
      
      indices.push_back(vertices.size() - 1);
      
      if constexpr (NORMALS_POINT_OUTWARDS) {
        indices.push_back(vertices.size() - NUM_LONGITUDE_LINES - 1);
        indices.push_back(vertices.size() - 2);
      }
      else {
        indices.push_back(vertices.size() - 2);
        indices.push_back(vertices.size() - NUM_LONGITUDE_LINES - 1);
      }

      vertices.shrink_to_fit();
      indices.shrink_to_fit();

      addMesh(vertices, indices).second.setMaterial(m_materialLoader.get().getSkyboxMtl());
    }
  }

  Application::Application()
    : m_mtlManager(m_textureManager),
      m_meshManager(m_mtlManager) {}

  int Application::parseCommandArgs(int, char**) {
    return 0;
  }

  int Application::run() {
    if (initialize() == 1 || loop() == 1 || shutdown() == 1)
      return 1;

    return 0;
  }

  util::ptr<Scene> Application::createPushScene() {
    auto scene = util::make_ptr<Scene>();

    // Ground plane
    auto obj_groundPlane = util::make_ptr<Object>(m_meshManager.getMesh(0));
    obj_groundPlane->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ 30, 30, 30 });
      o.setPosition({ 0, 0, 0 });
    };

    scene->addObject(obj_groundPlane);

    // Random objects
    auto obj_random0 = util::make_ptr<Object>(m_meshManager.getMesh(4));
    obj_random0->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ 2, 2, 2 });
      o.setPosition({ 0, 1.f, 2.f });
      o.setRotation(glm::angleAxis(time * glm::radians(90.f), glm::vec3{ 0, 0, 1 }) * glm::angleAxis(glm::radians(90.f), glm::vec3{ 1.f, 0.f, 0.f }));
    };

    auto obj_random1 = util::make_ptr<Object>(m_meshManager.getMesh(5));
    obj_random1->m_behavior = [](Object& o, float time, float dt) {
      o.setPosition({ 0, -1.f, .5f });
      o.setRotation(glm::angleAxis(time * glm::radians(90.f), glm::vec3{ 0, 0, 1 }) * glm::angleAxis(glm::radians(90.f), glm::vec3{ 1.f, 0.f, 0.f }));
    };

    scene->addObject(obj_random0);
    scene->addObject(obj_random1);

    // Flying color cube
    auto obj_flyingCube = util::make_ptr<Object>(m_meshManager.getMesh(1));
    obj_flyingCube->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ .5f, .5f, .5f });
      o.setPosition({ 1, 1, 2 + 0.f * cos(time) });
      o.setRotation(glm::angleAxis(time * 2 * glm::radians(90.f), glm::vec3{ 1.f, 0.f, 0.f }));
    };

    scene->addObject(obj_flyingCube);

    // Lights
    auto obj_circlingLight0 = util::make_ptr<Object>(m_meshManager.getMesh(1));
    obj_circlingLight0->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ .1f, .1f, .1f });
      o.setPosition({ 2 * sqrt(2) * cos(3 * time), 2 * sqrt(2) * sin(3 * time), 2 });
    };

    auto obj_circlingLight1 = util::make_ptr<Object>(m_meshManager.getMesh(1));
    obj_circlingLight1->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ .1f, .1f, .1f });
      o.setPosition({ -2 * sqrt(2) * cos(3 * time), -2 * sqrt(2) * sin(3 * time), 2 });
    };

    scene->addObject(obj_circlingLight0);
    scene->addObject(obj_circlingLight1);

    Camera camera;
    camera
      .setNearDepth(0.1f)
      .setFarDepth(200.f)
      .setEyePos({ M_SQRT2 * 4.5f, 0, 7.0f })
      .setLookAt({ 0.f, 0.f, 0.f })
      .setFOVDeg(45.f);

    scene->setCamera(camera);

    std::mt19937 rand_dev(time(NULL));
    std::uniform_real_distribution<float> pos_dist(-9, 9);
    std::uniform_real_distribution<float> color_dist(0, 1);
    std::uniform_real_distribution<float> atten_dist(0.5f, 4.f);

    for (uint32_t i = 0; i < FinalStep::MAX_LOCAL_LIGHTS; ++i) {
      auto l = util::make_ptr<Light>();
      l->setPosition(glm::vec3(pos_dist(rand_dev), pos_dist(rand_dev), abs(pos_dist(rand_dev) / 2)));
      l->setDirection(normalize(-l->getPosition()));
      l->setColor(glm::vec3(color_dist(rand_dev), color_dist(rand_dev), color_dist(rand_dev)));// / glm::vec3(i + 3);
      l->setAttenuation(glm::vec3(1.f + atten_dist(rand_dev), atten_dist(rand_dev), 1.f));
      l->setLocalRadius(20);
      l->setType(Light::Type::Point);

      scene->addLight(l);
    }

    scene->getLights()[0]->setAttenuation(glm::vec3(1.f, 0.5f, 0.1f));
    scene->getLights()[1]->setAttenuation(glm::vec3(1.f, 0.5f, 0.1f));

    for (uint32_t i = 2; i < scene->getLights().size(); ++i) {
      auto& light = scene->getLights()[i];
      auto lightObj = util::make_ptr<Object>(m_meshManager.getMesh(1));
      lightObj->m_behavior = [](Object& o, float time, float dt) {
        o.setScale({ .1f, .1f, .1f });
        o.setRotation(glm::angleAxis(time * 2 * glm::radians(90.f), glm::vec3{ 1.f, 0.f, 0.f }));
      };

      scene->addObject(lightObj);
    }

    ShadowedLight globalLight;
    globalLight.setPosition({ 5, 5, 5 })
      .setDirection(glm::normalize(glm::vec3(-1, -1, -1)))
      .setColor({ 1.f, 1.0f, 1.0f });

    ShadowedLight globalLight2;
    globalLight2.setPosition({ -5, -5, 5 })
      .setDirection(glm::normalize(glm::vec3(1, 1, -1)))
      .setColor({ 1.5f, 1.0f, 0.0f });

    scene->addGlobalLight(globalLight);
    scene->addGlobalLight(globalLight2);

    return scene;
  }

  int Application::initialize() {
    GLFWControl::Init();
    // open window

    static constexpr unsigned WINDOW_WIDTH = 1600;
    static constexpr unsigned WINDOW_HEIGHT = 800;
    static constexpr float    WINDOW_ASPECT = static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT;

    m_window = new GLFWWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "GPROJ - Loading Your Experience...");
    m_inputHandler = new InputHandler(*m_window);
    m_window->setInputHandler(m_inputHandler);
    m_window->setOnResizeCB([this](GLFWWindow* window, int nx, int ny) {
      m_resizedWindow = true;
      m_mainScene->getCamera().setAspect((float)nx / ny);
      m_secondScene->getCamera().setAspect((float)nx / ny);
    });

#ifdef DW_USE_IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForVulkan(m_window->getHandle(), true);
#endif


    m_renderer = util::make_ptr<Renderer>();

    m_renderer->init(m_window);

    // load the objects that i want
    m_meshManager.loadBasicMeshes();

    auto digipenLogo = m_textureManager.load("data/textures/DigiPen_RGB_Red.jpg");

    m_textureManager.uploadTextures(*m_renderer);
    m_meshManager.uploadMeshes(*m_renderer);

    std::atomic_bool continueDisplayingLogo = true;
    auto displayLogoThreadFn = [this, &continueDisplayingLogo, &digipenLogo]() {
      auto startTime = std::chrono::high_resolution_clock::now();
      while (continueDisplayingLogo || (std::chrono::high_resolution_clock::now() - startTime) < std::chrono::seconds(5) ) {
        GLFWControl::Poll();
        m_renderer->displayLogo(digipenLogo->second->getView());
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
      }
    };

    std::thread displayLogoThread(displayLogoThreadFn);

    m_meshManager.load("data/objects/lamp.obj");
    m_meshManager.load("data/objects/teapot.obj");
    m_meshManager.load("data/objects/icosahedron.obj");

    auto bg_iter = m_textureManager.load("data/textures/14-Hamarikyu_Bridge_B_3k.hdr");
    auto irr_iter = m_textureManager.load("data/textures/14-Hamarikyu_Bridge_B_3k.irr.hdr");

    tinyobj::material_t asphaltMtl = { "Asphalt", {0}, {1, 1, 1}, {1, 1, 1}, {0}, {0}, 0, 0, 0, 2, 0,
      "",
      "Asphalt/TexturesCom_Asphalt11_2x2_512_albedo.png",
      "",
      "",
      "",
      "", "", "", {}, {}, {}, {}, {}, {}, {}, {},
      0, 0, 0, 0, 0, 0, 0, 0,
      "Asphalt/TexturesCom_Asphalt11_2x2_512_roughness.png",
      "Asphalt/TexturesCom_Asphalt11_2x2_512_metallic.png",
      "", "",
      "Asphalt/TexturesCom_Asphalt11_2x2_512_normal.png",
      {}, {}, {}, {}, {}, 0, {}
    };

    
    m_meshManager.getMesh(0).get().setMaterial(m_mtlManager.getMtl(m_mtlManager.load(asphaltMtl)));

    // fill scenes
    static constexpr unsigned MAX_DYNAMIC_LIGHTS = 128;
    auto  currentTime = std::chrono::high_resolution_clock::now();
    float curTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - std::chrono::high_resolution_clock::now()).count();
      // skydome
    auto obj_skydome = util::make_ptr<Object>(m_meshManager.getMesh(3));
    obj_skydome->setPosition({ 0, 0, 0 });
    obj_skydome->setScale({50, 50, 50});

    m_mainScene = createPushScene();
    m_mainScene->addObject(obj_skydome);

    // Secondary scene
    m_secondScene = util::make_ptr<Scene>();
    {
      m_secondScene->addObject(obj_skydome);

      auto obj_groundPlane = util::make_ptr<Object>(m_meshManager.getMesh(0));
      obj_groundPlane->m_behavior = [](Object& o, float time, float dt) {
        o.setScale({ 10, 10, 10 });
        o.setPosition({ 0, 0, 0 });
      };

      m_secondScene->addObject(obj_groundPlane);

      auto obj_icosahedron = util::make_ptr<Object>(m_meshManager.getMesh(6));
      obj_icosahedron->m_behavior = [](Object& o, float time, float dt) {
        o.setScale({ 1.5f, 1.5f, 1.5f });
        o.setPosition({ 0, 0, 1.5f });
      };

      m_secondScene->addObject(obj_icosahedron);

      Camera camera;
      camera
        .setNearDepth(0.1f)
        .setFarDepth(200.f)
        .setEyePos({ M_SQRT2 * 4.5f, 0, 7.0f })
        .setLookAt({ 0.f, 0.f, 0.f })
        .setFOVDeg(45.f);

      m_secondScene->setCamera(camera);

      ShadowedLight globalLight;
      globalLight.setPosition({ 20, 20, 20 })
        .setDirection(glm::normalize(glm::vec3(-1, -1, -1)))
        .setColor({ 1.f, 1.0f, 1.0f });

      m_secondScene->addGlobalLight(globalLight);
    }

    // Set backgrounds
    m_mainScene->setBackground(bg_iter->second, irr_iter->second);
    m_secondScene->setBackground(bg_iter->second, irr_iter->second);

    // TODO: Each scene needs its own shader control
    m_curScene = m_mainScene;

    m_mainScene->getCamera().setAspect(WINDOW_ASPECT);
    m_secondScene->getCamera().setAspect(WINDOW_ASPECT);

    continueDisplayingLogo.store(false);
    displayLogoThread.join();

    m_textureManager.uploadTextures(*m_renderer);
    m_mtlManager.uploadMaterials(*m_renderer);
    m_meshManager.uploadMeshes(*m_renderer);
    m_renderer->setScene(m_mainScene);
    m_renderer->setShaderControl(&m_shaderControl);

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
    static bool showImGuiControls = true;
    m_inputHandler->registerKeyFunction(GLFW_KEY_ENTER, [this]() {
      showImGuiControls = !showImGuiControls;
      },
      nullptr
    );

    while (!m_renderer->done()) {
      GLFWControl::Poll();
      if (m_resizedWindow) {
        m_resizedWindow = false;
        m_renderer->restartWindow();
      }

#ifdef DW_USE_IMGUI
      ImGui_ImplGlfw_NewFrame();
      ImGui_ImplVulkan_NewFrame();
      ImGui::NewFrame();

      if (showImGuiControls) {
        ImGui::Begin("Shader Control (Press Enter to toggle visibility)");
        ImGui::DragFloat("Moment Bias", &m_shaderControl.global_momentBias, 0.00000005f, 0, .0001, "%.8f");
        ImGui::DragFloat("Depth Bias", &m_shaderControl.global_depthBias, 0.0001f, 0.1f, 0.1f, "%.4f");
        ImGui::DragFloat("Default Roughness", &m_shaderControl.geometry_defaultRoughness, 0.01, 0, 1);
        ImGui::DragFloat("Default Metallic", &m_shaderControl.geometry_defaultMetallic, 0.01, 0, 1);
        ImGui::DragFloat("Tone Map Exposure", &m_shaderControl.final_toneMapExposure, 0.1);
        ImGui::DragFloat("Tone Map Exponent", &m_shaderControl.final_toneMapExponent, 0.01);
        ImGui::End();

        static bool enableGlobalLight = true;
        static bool enableShadowMapBlur = true;
        ImGui::Begin("Render Step Control");
        ImGui::Checkbox("Global Lighting", reinterpret_cast<bool*>(&m_shaderControl.global_doGlobalLighting));
        ImGui::Checkbox("Shadows", reinterpret_cast<bool*>(&m_shaderControl.global_enableShadows));
        ImGui::Checkbox("IBL Lighting", reinterpret_cast<bool*>(&m_shaderControl.global_enableIBL));
        ImGui::Checkbox("HDR Background", reinterpret_cast<bool*>(&m_shaderControl.global_enableBackgrounds));
        ImGui::Checkbox("Local Lighting", reinterpret_cast<bool*>(&m_shaderControl.final_doLocalLighting));
        if (ImGui::Checkbox("Blur Shadow Maps", &enableShadowMapBlur))
          m_renderer->setShadowMapBlurEnabled(enableShadowMapBlur);
        if (ImGui::Checkbox("Submit Global Lighting (warning: weird)", &enableGlobalLight))
          m_renderer->setGlobalLightingEnabled(enableGlobalLight);
        ImGui::End();
      }

      ImGui::Begin("Scene Switcher");
      if (ImGui::Button("Main Scene") && m_curScene != m_mainScene) {
        m_shaderControl.global_momentBias = 0.00000005f;
        m_curScene = m_mainScene;
        m_renderer->setScene(m_curScene);
      }

      ImGui::SameLine();
      if (ImGui::Button("Second Scene") && m_curScene != m_secondScene) {
        m_shaderControl.global_momentBias = 0.00000445f;
        m_curScene = m_secondScene;
        m_renderer->setScene(m_secondScene);
      }
      ImGui::End();

      ImGui::Begin("Credits");
      ImGui::Text("DigiPen Institute of Technology\n Presents: GPROJ\n");
      ImGui::Text("\nwww.digipen.edu");
      ImGui::Text("\nCopyright (C) 2019 by DigiPen Corp, USA.\nAll rights reserved.");
      ImGui::Text("\nDeveloped by David T. Walker");
      ImGui::Text("\nInstructors:");
      ImGui::BulletText("Jen Sward");
      ImGui::BulletText("Matthew Picioccio");
      ImGui::BulletText("Andrew Kaplan");
      ImGui::Text("\nPresident:");
      ImGui::BulletText("Claude Comair");
      ImGui::Text("\nExecutives:");
      ImGui::BulletText("Jason Chu");
      ImGui::BulletText("John Bauer");
      ImGui::BulletText("Samir Abu Samra");
      ImGui::BulletText("Raymond Yan");
      ImGui::BulletText("Prasanna Ghali");
      ImGui::BulletText("Michele Comair");
      ImGui::BulletText("Xin Li");
      ImGui::BulletText("Erik Mohrmann");
      ImGui::BulletText("Melvin Gonsalvez");
      ImGui::BulletText("Christopher Comair");
      ImGui::End();

      ImGui::EndFrame();
#endif
      // input check'
      static constexpr float STEP_VALUE = 2.75f;
      static constexpr float ROTATE_STEP = 0.75f;
      static float RADIUS = M_SQRT2 * 4.5f;
      static float cameraCurrentT = 0.f;

      prevTime = curTime;
      curTime = ClockType::now();

      float timeCount = DurationType(curTime - m_startTime).count();
      float dt = DurationType(curTime - prevTime).count();
      char titleBuff[256] = { '\0' };
      float fps = 1.f / dt;
      if (fps >= 119.f)
        sprintf(titleBuff, "GPROJ - Sprinting Merilly at %.0f FPS", fps);
      else if (fps >= 59.f)
        sprintf(titleBuff, "GPROJ - Delightfully Running at %.0f FPS", fps);
      else if (fps >= 10.f)
        sprintf(titleBuff, "GPROJ - Chugging Along at %.0f FPS", fps);
      else
        sprintf(titleBuff, "GPROJ - Scared of Burning Out at %.0f FPS", fps);

      glfwSetWindowTitle(m_window->getHandle(), titleBuff);

      if (m_inputHandler->getKeyState(GLFW_KEY_A)) {
        m_curScene->getCamera().setEyePos(m_curScene->getCamera().getEyePos() + dt * m_curScene->getCamera().getRightDir() * STEP_VALUE);
      }

      if (m_inputHandler->getKeyState(GLFW_KEY_D)) {
        m_curScene->getCamera().setEyePos(m_curScene->getCamera().getEyePos() - dt * m_curScene->getCamera().getRightDir() * STEP_VALUE);
      }

      //float cameraZ = m_curScene->getCamera().getEyePos().z;

      if(m_inputHandler->getKeyState(GLFW_KEY_W)) {
        m_curScene->getCamera().setEyePos(m_curScene->getCamera().getEyePos() + dt * m_curScene->getCamera().getViewDir() * STEP_VALUE);
        //cameraZ += STEP_VALUE * dt * 2;
      }

      if(m_inputHandler->getKeyState(GLFW_KEY_S)) {
        m_curScene->getCamera().setEyePos(m_curScene->getCamera().getEyePos() - dt * m_curScene->getCamera().getViewDir() * STEP_VALUE);
        //cameraZ -= STEP_VALUE * dt * 2;
      }

      if (m_inputHandler->getKeyState(GLFW_KEY_LEFT)) {
        // set view dir
        auto curView = m_curScene->getCamera().getViewDir();
        // rotate by dt * ROTATE_STEP around the up direction
        curView = glm::toMat4(glm::angleAxis(ROTATE_STEP * dt * 1.5f, m_curScene->getCamera().getUpDir())) * glm::vec4(curView, 0);

        m_curScene->getCamera().setViewDir(curView);
        //m_curScene->getCamera().setViewDir(m_curScene->getCamera())
        //m_curScene->getCamera().setEyePos(m_curScene->getCamera().getEyePos() + dt * m_curScene->getCamera().getViewDir());
        //cameraZ += STEP_VALUE * dt * 2;
      }

      if (m_inputHandler->getKeyState(GLFW_KEY_RIGHT)) {// set view dir
        auto curView = m_curScene->getCamera().getViewDir();
        // rotate by dt * ROTATE_STEP around the up direction
        curView = glm::toMat4(glm::angleAxis(-ROTATE_STEP * dt * 1.5f, m_curScene->getCamera().getUpDir())) * glm::vec4(curView, 0);

        m_curScene->getCamera().setViewDir(curView);
        //cameraZ -= STEP_VALUE * dt * 2;
      }

      if(m_inputHandler->getKeyState(GLFW_KEY_UP)) {
        auto curView = m_curScene->getCamera().getViewDir();
        // rotate by dt * ROTATE_STEP around the right direction
        curView = glm::toMat4(glm::angleAxis(-ROTATE_STEP * dt, m_curScene->getCamera().getRightDir())) * glm::vec4(curView, 0);

        m_curScene->getCamera().setViewDir(curView);
      }

      if (m_inputHandler->getKeyState(GLFW_KEY_DOWN)) {
        auto curView = m_curScene->getCamera().getViewDir();
        // rotate by dt * ROTATE_STEP around the right direction
        curView = glm::toMat4(glm::angleAxis(ROTATE_STEP * dt, m_curScene->getCamera().getRightDir())) * glm::vec4(curView, 0);

        m_curScene->getCamera().setViewDir(curView);
      }

      //if (m_inputHandler->getKeyState(GLFW_KEY_EQUAL)) {
      //  //RADIUS += STEP_VALUE * dt;
      //}
      //
      //if (m_inputHandler->getKeyState(GLFW_KEY_MINUS)) {
      //  //RADIUS -= STEP_VALUE * dt;
      //}

      //m_curScene->getCamera().setEyePos(glm::vec3{ RADIUS * cos(cameraCurrentT), RADIUS * sin(cameraCurrentT), cameraZ });
      //m_curScene->getCamera().setLookAt({ 0, 0, 0 });

      for (auto& obj : m_curScene->getObjects()) {
        obj->callBehavior(timeCount, dt);
      }

      if (m_curScene == m_mainScene) {
        m_mainScene->getLights()[0]->setPosition({ 2 * sqrt(2) * cos(3 * timeCount), 2 * sqrt(2) * sin(3 * timeCount), 2 });
        m_mainScene->getLights()[1]->setPosition({ -2 * sqrt(2) * cos(3 * timeCount), -2 * sqrt(2) * sin(3 * timeCount), 2 });

        std::mt19937 rand_dev(time(NULL));
        std::uniform_real_distribution<float> shift(-1.f, 1.f);
        std::uniform_real_distribution<float> color(-0.5f, 0.5f);
        for (uint32_t i = 2; i < m_mainScene->getLights().size(); ++i) {
          auto& light = *m_mainScene->getLights()[i];
          auto& obj = m_mainScene->getObjects()[i + 4];
          light.setPosition(light.getPosition() + glm::vec3(shift(rand_dev) * dt, shift(rand_dev) * dt, shift(rand_dev) * dt));
          light.setPosition(glm::clamp(light.getPosition(), glm::vec3(-9.f, -9.f, 0.1f), glm::vec3(9.f)));
          light.setColor(light.getColor() + glm::vec3(color(rand_dev) * dt, color(rand_dev) * dt, color(rand_dev) * dt));
          light.setColor(glm::clamp(light.getColor(), 0.f, 1.f));
          obj->setPosition(light.getPosition());
        }
      }
      else if(m_curScene == m_secondScene) {
        
      }

      m_renderer->drawFrame();
    }

    return 0;
  }

  int Application::shutdown() {
    m_mainScene.reset();
    m_secondScene.reset();
    m_curScene.reset();

    ImGui_ImplGlfw_Shutdown();

    m_textureManager.clear();
    m_mtlManager.clear();
    m_meshManager.clear();
    m_renderer->shutdown();

    delete m_inputHandler; m_inputHandler = nullptr;
    delete m_window; m_window = nullptr;

    GLFWControl::Shutdown();
    return 0;
  }
}
