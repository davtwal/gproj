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

#include "app/Application.h"
#include "app/InputHandler.h"

#include "render/GLFWControl.h"
#include "render/GLFWWindow.h"
#include "render/VulkanControl.h"

#include "render/Surface.h"
#include "render/Swapchain.h"
#include "render/Framebuffer.h"  // this is used
#include "render/Queue.h"
#include "render/CommandBuffer.h"
#include "render/RenderPass.h"
#include "render/RenderSteps.h"
#include "render/Shader.h"
#include "render/Vertex.h"
#include "render/Buffer.h"
#include "render/Mesh.h"
#include "render/MeshManager.h"
#include "util/Trace.h"

#include <cassert>
#include <algorithm>
#include <unordered_map>
#include <chrono>
#include <random>
#include <thread>
#include <atomic>

#include "app/ImGui.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include "obj/Graphics.h"
#include "obj/Behavior.h"

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

      addMesh(vertices, indices).second->calculateTangents();
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

      addMesh(vertices, indices).second->calculateTangents();;
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

      addMesh(vertices, indices).second->calculateTangents();;
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

      addMesh(vertices, indices).second->setMaterial(m_materialLoader.get().getSkyboxMtl());
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

  static util::ptr<obj::Object> ObjFactoryMovableVisable(util::ptr<Mesh> mesh, obj::Behavior::BehaviorFn fn) {
    return util::make_ptr<Object>(3, 
      util::make_ptr<obj::Transform>(), 
      util::make_ptr<obj::Graphics>(std::move(mesh)), 
      util::make_ptr<obj::Behavior>(fn)
      );
  }

  util::ptr<Scene> Application::createPushScene() {
    auto scene = util::make_ptr<Scene>();

    using obj::Object;

    // Ground plane
    auto obj_groundPlane = ObjFactoryMovableVisable(m_meshManager.getMesh(0), [](Object* o, float time, float dt) {
      o->getTransform()->setScale({ 30, 30, 30 });
      o->getTransform()->setPosition({ 0, 0, 0 });
    });

    scene->addObject(obj_groundPlane);

    // Random objects
    auto obj_random0 = ObjFactoryMovableVisable(m_meshManager.getMesh(4), [](Object* o, float time, float dt) {
      o->getTransform()->setScale({ 2, 2, 2 });
      o->getTransform()->setPosition({ 0, 1.f, 2.f });
      o->getTransform()->setRotation(glm::angleAxis(time * glm::radians(90.f), glm::vec3{ 0, 0, 1 }) * glm::angleAxis(glm::radians(90.f), glm::vec3{ 1.f, 0.f, 0.f }));
    });

    auto obj_random1 = ObjFactoryMovableVisable(m_meshManager.getMesh(5), [](Object* o, float time, float dt) {
      o->getTransform()->setPosition({ 0, -1.f, .5f });
      o->getTransform()->setRotation(glm::angleAxis(time * glm::radians(90.f), glm::vec3{ 0, 0, 1 }) * glm::angleAxis(glm::radians(90.f), glm::vec3{ 1.f, 0.f, 0.f }));
    });

    scene->addObject(obj_random0);
    scene->addObject(obj_random1);

    // Flying color cube
    auto obj_flyingCube = ObjFactoryMovableVisable(m_meshManager.getMesh(1), [](Object* o, float time, float dt) {
      o->getTransform()->setScale({ .5f, .5f, .5f });
      o->getTransform()->setPosition({ 1, 1, 2 + 0.f * cos(time) });
      o->getTransform()->setRotation(glm::angleAxis(time * 2 * glm::radians(90.f), glm::vec3{ 1.f, 0.f, 0.f }));
    });

    scene->addObject(obj_flyingCube);

    // Lights
    auto obj_circlingLight0 = ObjFactoryMovableVisable(m_meshManager.getMesh(1), [](Object* o, float time, float dt) {
      o->getTransform()->setScale({ .1f, .1f, .1f });
      o->getTransform()->setPosition({ 2 * sqrt(2) * cos(3 * time), 2 * sqrt(2) * sin(3 * time), 2 });
    });

    auto obj_circlingLight1 = ObjFactoryMovableVisable(m_meshManager.getMesh(1), [](Object* o, float time, float dt) {
      o->getTransform()->setScale({ .1f, .1f, .1f });
      o->getTransform()->setPosition({ -2 * sqrt(2) * cos(3 * time), -2 * sqrt(2) * sin(3 * time), 2 });
    });

    scene->addObject(obj_circlingLight0);
    scene->addObject(obj_circlingLight1);

    auto obj_camera = util::make_ptr<Object>(1, util::make_ptr<Camera>());
    //obj_camera->attach(new obj::Camera);

    auto camera = obj_camera->get<obj::Camera>();
    camera
      ->setNear(0.1f)
      .setFar(200.f)
      .setWorldPos({ M_SQRT2 * 4.5f, 0, 7.0f })
      .lookAt({ 0.f, 0.f, 0.f })
      .setFOV(glm::radians(45.f));

    scene->setCamera(camera);

    std::mt19937 rand_dev(time(NULL));
    std::uniform_real_distribution<float> pos_dist(-9, 9);
    std::uniform_real_distribution<float> color_dist(0, 1);
    std::uniform_real_distribution<float> atten_dist(0.5f, 4.f);

    for (uint32_t i = 0; i < LocalLightingStep::MAX_LOCAL_LIGHTS; ++i) {
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
      auto lightObj = ObjFactoryMovableVisable(m_meshManager.getMesh(5), [](Object* o, float time, float dt) {
        o->getTransform()->setScale({ .1f, .1f, .1f });
        o->getTransform()->setRotation(glm::angleAxis(time * 2 * glm::radians(90.f), glm::vec3{ 1.f, 0.f, 0.f }));
      });

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

    auto bg_iter = m_textureManager.load("data/textures/14-Hamarikyu_Bridge_B_3k.hdr");
    auto irr_iter = m_textureManager.load("data/textures/14-Hamarikyu_Bridge_B_3k.irr.hdr");

    scene->setBackground(bg_iter->second, irr_iter->second);

    scene->addObject(obj_camera);

    return scene;
  }
  
  /*util::ptr<Scene> Application::createSecondaryScene() {
    auto scene = util::make_ptr<Scene>();
    {
      auto obj_groundPlane = util::make_ptr<Object>(m_meshManager.getMesh(0));
      obj_groundPlane->m_behavior = [](Object& o, float time, float dt) {
        o.setScale({ 10, 10, 10 });
        o.setPosition({ 0, 0, 0 });
      };

      scene->addObject(obj_groundPlane);

      auto obj_icosahedron = util::make_ptr<Object>(m_meshManager.getMesh(6));
      obj_icosahedron->m_behavior = [](Object& o, float time, float dt) {
        o.setScale({ 1.5f, 1.5f, 1.5f });
        o.setPosition({ 0, 0, 1.5f });
      };

      scene->addObject(obj_icosahedron);

      Camera camera;
      camera
        .setNearDepth(0.1f)
        .setFarDepth(200.f)
        .setEyePos({ M_SQRT2 * 4.5f, 0, 7.0f })
        .setLookAt({ 0.f, 0.f, 0.f })
        .setFOVDeg(45.f);

      scene->setCamera(camera);

      ShadowedLight globalLight;
      globalLight.setPosition({ 20, 20, 20 })
        .setDirection(glm::normalize(glm::vec3(-1, -1, -1)))
        .setColor({ 1.f, 1.0f, 1.0f });

      scene->addGlobalLight(globalLight);

      auto bg_iter = m_textureManager.load("data/textures/Factory_Catwalk_2k.hdr");
      auto irr_iter = m_textureManager.load("data/textures/Factory_Catwalk_2k.irr.hdr");

      scene->setBackground(bg_iter->second, irr_iter->second);
    }

    return scene;
  }

  util::ptr<Scene> Application::createIntroScene() {
    auto scene = util::make_ptr<Scene>();

    auto obj_groundPlane = util::make_ptr<Object>(m_meshManager.getMesh(0));
    obj_groundPlane->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ 10, 10, 10 });
      o.setPosition({ 0, 0, 0 });
    };
    scene->addObject(obj_groundPlane);

    auto obj_groundPlane2 = util::make_ptr<Object>(m_meshManager.getMesh(2));
    obj_groundPlane2->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ 10, 10, 1 });
      o.setPosition({ 10, 0, 0 });
    };
    scene->addObject(obj_groundPlane2);

    auto obj_groundPlane3 = util::make_ptr<Object>(m_meshManager.getMesh(2));
    obj_groundPlane3->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ 10, 10, 1 });
      o.setPosition({ -10, 0, 0 });
    };
    scene->addObject(obj_groundPlane3);

    auto obj_wallBox = util::make_ptr<Object>(m_meshManager.getMesh(2));
    obj_wallBox->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ 10, 1, 10 });
      o.setPosition({ 5, -5, 5 });
    };
    scene->addObject(obj_wallBox);

    auto obj_wallBox2 = util::make_ptr<Object>(m_meshManager.getMesh(2));
    obj_wallBox2->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ 10, 1, 10 });
      o.setPosition({ 0, 5, 5 });
    };
    scene->addObject(obj_wallBox2);

    auto obj_wallBox3 = util::make_ptr<Object>(m_meshManager.getMesh(2));
    obj_wallBox3->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ 10, 1, 10 });
      o.setPosition({ 17.5f, 5, 5 });
    };
    scene->addObject(obj_wallBox3);

    auto obj_goalPost = util::make_ptr<Object>(m_meshManager.getMesh(5));
    obj_goalPost->m_behavior = [](Object& o, float time, float dt) {
      o.setScale({ 2, 2, 2 });
      o.setPosition({ -12.5, -0.5f, 2.f });
      o.setRotation(glm::angleAxis(time * glm::radians(90.f), glm::vec3{ 0, 0, 1 }) * glm::angleAxis(glm::radians(90.f), glm::vec3{ 1.f, 0.f, 0.f }));
    };
    scene->addObject(obj_goalPost);

    for (uint32_t i = 0; i < 11; ++i) {
      auto l = util::make_ptr<Light>();
      l->setPosition(glm::vec3((i - 6.f) * 3, 0, (i + 1.f) * .25f));
      l->setDirection(normalize(-l->getPosition()));
      l->setColor(glm::vec3(0.7f, 0.3f, 0.7f));// / glm::vec3(i + 3);
      l->setAttenuation(glm::vec3(1.f, 1.f, 1.f));
      l->setLocalRadius(20);
      l->setType(Light::Type::Point);

      scene->addLight(l);
    }

    Camera camera;
    camera
      .setNearDepth(0.1f)
      .setFarDepth(200.f)
      .setEyePos({ 12.5f, 0, 2.0f })
      .setLookAt({ 0.f, 0.f, 0.f })
      .setFOVDeg(45.f);

    scene->setCamera(camera);

    ShadowedLight globalLight;
    globalLight.setPosition({ 20, 20, 20 })
      .setDirection(glm::normalize(glm::vec3(-1, -1, -1)))
      .setColor({ 1.f, 1.0f, 1.0f });

    scene->addGlobalLight(globalLight);

    auto bg_iter = m_textureManager.load("data/textures/Road_to_MonumentValley_Ref.hdr");
    auto irr_iter = m_textureManager.load("data/textures/Road_to_MonumentValley_Ref.irr.hdr");

    scene->setBackground(bg_iter->second, irr_iter->second);

    return scene;
  }
  */

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
      m_mainScene->getCamera()->setAspect((float)nx / ny);
      //m_secondScene->getCamera()->setAspect((float)nx / ny);
      //m_thirdScene->getCamera()->setAspect((float)nx / ny);
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

    std::atomic_bool continueDisplayingLogo = false;
    auto displayLogoThreadFn = [this, &continueDisplayingLogo, &digipenLogo]() {
      auto startTime = std::chrono::high_resolution_clock::now();
      while (continueDisplayingLogo || (std::chrono::high_resolution_clock::now() - startTime) < std::chrono::seconds(1) ) {
        GLFWControl::Poll();
        m_renderer->displayLogo(digipenLogo->second->getView());
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
      }
    };

    //std::thread displayLogoThread(displayLogoThreadFn);

    m_meshManager.load("data/objects/lamp.obj");
    m_meshManager.load("data/objects/teapot.obj");
    m_meshManager.load("data/objects/icosahedron.obj");

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

    
    m_meshManager.getMesh(0)->setMaterial(m_mtlManager.getMtl(m_mtlManager.load(asphaltMtl)));

    // fill scenes
    static constexpr unsigned MAX_DYNAMIC_LIGHTS = 128;
    auto  currentTime = std::chrono::high_resolution_clock::now();
    float curTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - std::chrono::high_resolution_clock::now()).count();
      // skydome
    auto obj_skydome = util::make_ptr<obj::Object>(1, util::make_ptr<Graphics>(m_meshManager.getMesh(3)));
    obj_skydome->getTransform()->setPosition({ 0, 0, 0 });
    obj_skydome->getTransform()->setScale({50, 50, 50});

    m_mainScene = createPushScene();
    m_mainScene->addObject(obj_skydome);

    // Secondary scene
    //m_secondScene = createSecondaryScene();
    //m_secondScene->addObject(obj_skydome);
    //
    //m_thirdScene = createIntroScene();
    //m_thirdScene->addObject(obj_skydome);

    // TODO: Each scene needs its own shader control
    m_shaderControl.geometry_defaultRoughness = 0.8f;
    m_shaderControl.geometry_defaultMetallic = 0.07f;
    m_curScene = m_mainScene;

    m_mainScene->getCamera()->setAspect(WINDOW_ASPECT);
    //m_secondScene->getCamera()->setAspect(WINDOW_ASPECT);
    //m_thirdScene->getCamera()->setAspect(WINDOW_ASPECT);

    //continueDisplayingLogo.store(false);
    //displayLogoThread.join();

    m_textureManager.uploadTextures(*m_renderer);
    m_mtlManager.uploadMaterials(*m_renderer);
    m_meshManager.uploadMeshes(*m_renderer);
    m_renderer->setScene(m_curScene);
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
    static int currentStage = 0;
    m_inputHandler->registerKeyFunction(GLFW_KEY_ENTER, [this]() {
        showImGuiControls = !showImGuiControls;
        currentStage = 2;
      },
      nullptr
    );


    while (!m_renderer->done()) {
      GLFWControl::Poll();
      if (m_resizedWindow) {
        m_resizedWindow = false;
        m_renderer->restartWindow();
      }

      auto changeToSecondStage = [this]() {
        m_shaderControl.global_momentBias = 0.00000445f;
        m_shaderControl.geometry_defaultRoughness = 0.46f;
        m_shaderControl.geometry_defaultMetallic = 0.76f;
        //m_curScene = m_secondScene;
        //m_renderer->setScene(m_secondScene);
      };

      auto changeToThirdStage = [this]() {
        m_shaderControl.global_momentBias = 0.00000215f;
        m_shaderControl.geometry_defaultRoughness = 0.16f;
        m_shaderControl.geometry_defaultMetallic = 0.08f;
        m_curScene = m_mainScene;
        m_renderer->setScene(m_mainScene);
      };

#ifdef DW_USE_IMGUI
      ImGui_ImplGlfw_NewFrame();
      ImGui_ImplVulkan_NewFrame();
      ImGui::NewFrame();

      if (showImGuiControls && currentStage >= 2) {
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

      if (currentStage == 0) {
        ImGui::Begin("Tutorial");
        ImGui::Text("Use WASD to move");
        ImGui::Text("Use the arrow keys to look around");
        ImGui::Text("Press Escape to quit");
        ImGui::Text("Press Enter to CHEAT");
        ImGui::End();
      }

      if (currentStage >= 2) {
        //if(ImGui::Begin("Object Editor")) {
        //  for(auto& object : m_curScene->getObjects()) {
        //    if(ImGui::BeginCombo("Mesh", object->get<obj::Graphics>()->getMesh()->getName().c_str())) {
        //      //m_meshManager.
        //      ImGui::EndCombo();
        //    }
        //  }
        //}
        ImGui::End();

        /*ImGui::Begin("Scene Switcher");
        if (ImGui::Button("Intro Scene") && m_curScene != m_thirdScene) {
          m_shaderControl.global_momentBias = 0.00000005f;
          m_shaderControl.geometry_defaultRoughness = 0.8f;
          m_shaderControl.geometry_defaultMetallic = 0.07f;
          m_curScene = m_thirdScene;
          m_renderer->setScene(m_thirdScene);
        }

        ImGui::SameLine();
        if (ImGui::Button("Test Scene") && m_curScene != m_secondScene) {
          changeToSecondStage();
          ImGui::SameLine();
        }

        ImGui::SameLine();
        if (ImGui::Button("Push Scene") && m_curScene != m_mainScene) {
          changeToThirdStage();
        }
        ImGui::End();*/

        if (ImGui::Begin("Credits")) {
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
        }
        ImGui::End();
      }

      ImGui::EndFrame();
#endif
      // input check'
      static constexpr float STEP_VALUE = 2.75f;
      static constexpr float ROTATE_STEP = 0.75f;

      prevTime = curTime;
      curTime = ClockType::now();

      float timeCount = DurationType(curTime - m_startTime).count();
      float dt = DurationType(curTime - prevTime).count();
      char titleBuff[256] = { '\0' };
      float fps = 1.f / dt;
      if (fps >= 119.f)
        sprintf(titleBuff, "GPROJ - Sprinting Merilly at %.0f FPS", fps);
      else if (fps >= 59.f)
        sprintf(titleBuff, "GPROJ - Cheerfully Running at %.0f FPS", fps);
      else if (fps >= 15.f)
        sprintf(titleBuff, "GPROJ - Chugging Along at %.0f FPS", fps);
      else
        sprintf(titleBuff, "GPROJ - Scared of Burning Out at %.0f FPS", fps);

      glfwSetWindowTitle(m_window->getHandle(), titleBuff);

      // TODO: moving camera

      /*  Controls:
       *  - W/A/S/D -> Move right/left/forward/backward
       *  - Mouse   -> Look around
       *
       */

      auto curCam = m_curScene->getCamera();
      if (m_inputHandler->getKeyState(GLFW_KEY_A)) {
        curCam->addLocalPos(-dt * STEP_VALUE * curCam->getRight());
      }

      if (m_inputHandler->getKeyState(GLFW_KEY_D)) {
        curCam->addLocalPos(dt * STEP_VALUE * curCam->getRight());
      }

      if(m_inputHandler->getKeyState(GLFW_KEY_W)) {
        curCam->addLocalPos(dt * STEP_VALUE * curCam->getForward());
      }

      if(m_inputHandler->getKeyState(GLFW_KEY_S)) {
        curCam->addLocalPos(-dt * STEP_VALUE * curCam->getForward());
      }

      if(m_inputHandler->getKeyState(GLFW_KEY_LEFT_SHIFT)) {
        glfwSetInputMode(m_window->getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      }
      else {
        glfwSetInputMode(m_window->getHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      }

      m_inputHandler->registerMouseMoveFunction([&curCam, this](double dx, double dy) {
        static constexpr float SENSITIVITY = 0.003f;
        if (m_inputHandler->getKeyState(GLFW_KEY_LEFT_SHIFT)) {
          curCam->addPitch(-dy * SENSITIVITY);
          curCam->addYaw(dx * SENSITIVITY);
        }
      });

      for (auto& obj : m_curScene->getObjects()) {
        if(obj->get<obj::Behavior>())
          obj->get<obj::Behavior>()->call(timeCount, dt);
      }

      if (m_curScene == m_mainScene) {
        m_curScene->getLights()[0]->setPosition({ 2 * sqrt(2) * cos(3 * timeCount), 2 * sqrt(2) * sin(3 * timeCount), 2 });
        m_curScene->getLights()[1]->setPosition({ -2 * sqrt(2) * cos(3 * timeCount), -2 * sqrt(2) * sin(3 * timeCount), 2 });

        std::mt19937 rand_dev(time(NULL));
        std::uniform_real_distribution<float> shift(-1.f, 1.f);
        std::uniform_real_distribution<float> color(-0.5f, 0.5f);
        for (uint32_t i = 2; i < m_mainScene->getLights().size() - 1; ++i) {
          auto& light = *m_mainScene->getLights()[i];
          auto& obj = m_mainScene->getObjects()[i + 4];
          light.setPosition(light.getPosition() + glm::vec3(shift(rand_dev) * dt, shift(rand_dev) * dt, shift(rand_dev) * dt));
          light.setPosition(glm::clamp(light.getPosition(), glm::vec3(-9.f, -9.f, 0.1f), glm::vec3(9.f)));
          light.setColor(light.getColor() + glm::vec3(color(rand_dev) * dt, color(rand_dev) * dt, color(rand_dev) * dt));
          light.setColor(glm::clamp(light.getColor(), 0.f, 1.f));
          obj->getTransform()->setPosition(light.getPosition());
        }
      }
      //else if(m_curScene == m_secondScene) {
      //  m_curScene->getCamera()->lookAt({ 0, 0, 1.5f });

      //  if(currentStage == 1 && glm::distance(m_curScene->getCamera()->getWorldPos(), {0, 0, 1.5f}) < .95f) {
      //    ++currentStage;
      //    m_curScene->getCamera()->setWorldPos({ 5, 0, 2.f });
      //    changeToThirdStage();
      //  }
      //}
      //else { // cur scene = 3rd
      //  auto eyePos = m_curScene->getCamera()->getWorldPos();

      //  if (currentStage == 0 && glm::length(eyePos - glm::vec3{ -12.5f, -0.5f, 2.f }) < 1.f) {
      //    ++currentStage;
      //    m_curScene->getCamera()->setWorldPos({ 0, 0, 1.5f });
      //    changeToSecondStage();
      //  }

      //  eyePos.x = glm::clamp(eyePos.x, -14.f, 14.f);
      //  eyePos.y = glm::clamp(eyePos.y, -4.f, 4.f);

      //  eyePos.z = 2.f;

      //  if (eyePos.x < 5 && eyePos.x > -5)
      //    eyePos.z -= .5f;

      //  m_curScene->getCamera()->setWorldPos(eyePos);
      //}

      m_renderer->drawFrame();
    }

    return 0;
  }

  int Application::shutdown() {
    m_mainScene.reset();
    //m_secondScene.reset();
    //m_thirdScene.reset();
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
