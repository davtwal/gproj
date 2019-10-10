// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Renderer.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 09m 26d
// * Last Altered: 2019y 09m 26d
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

#ifndef DW_RENDERER_H
#define DW_RENDERER_H

#include "RenderPass.h"
#include "Utils.h"
#include "Object.h"
#include "Camera.h"
#include "Light.h"

#include "MeshManager.h"

#include <unordered_map>

namespace dw {
  //class Camera;
  class GeometryStep;
  class FinalStep;

  class PhysicalDevice;
  class VulkanControl;
  class GLFWWindow;
  class InputHandler;
  class LogicalDevice;
  class Surface;
  class Swapchain;
  class CommandPool;
  class CommandBuffer;
  class Buffer;
  class IShader;
  class Image;
  class DependentImage;
  class ImageView;
  class Framebuffer;

  struct ObjectUniform {
    alignas(16) glm::mat4 model;
  };

  struct CameraUniform {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 eyePos;
    alignas(16) glm::vec3 viewVec;
  };

  class Renderer {
  public:
    // initialize
    void initGeneral(GLFWWindow* window); // will NOT be delete-d by shutdown
    void initSpecific();

    NO_DISCARD bool done() const;

    void uploadMeshes(std::vector<util::Ref<Mesh>> const& meshes) const;
    void uploadMeshes(std::unordered_map<uint32_t, Mesh>& meshes) const;

    using SceneContainer = std::vector<util::Ref<Object>>;
    using LightContainer = std::vector<util::Ref<Light>>;

    void setScene(SceneContainer const& objects);
    void setDynamicLights(LightContainer const& lights);
    void setCamera(util::Ref<Camera> camera);

    void drawFrame();

    void shutdown();

  private:
    //////////////////////////////////////////////////////
    //////////////////////////////////////////////////////
    //// HELPER FUNCTIONS
    //////////////////////////////////////////////////////
    // general vulkan setup
    void setupInstance();
    void setupHelpers(); // e.g. debug messenger
    void setupDevice();
    void setupSurface();
    void setupSwapChain();
    void setupCommandPools();

    // specific to the rendering engine & what i support setup
    void setupSamplers();
    void setupUniformBuffers();
    void setupFrameBufferImages();
    void setupRenderSteps();
    void setupFrameBuffers();
    void transitionRenderImages() const;

    // specific to the current scene
    void releasePreviousDynamicUniforms();
    void prepareDynamicUniformBuffers();

    // called every frame
    void updateUniformBuffers(uint32_t imageIndex);// , Camera& cam, Object& obj);

    // shutdown helpers
    void shutdownScene();

    void recreateSwapChain();
    //////////////////////////////////////////////////////
    //////////////////////////////////////////////////////
    //// GENERAL VARIABLES
    //////////////////////////////////////////////////////
    static Camera s_defaultCamera;
    ///
    VulkanControl* m_control{ nullptr };
    GLFWWindow* m_window{ nullptr };
    LogicalDevice* m_device{ nullptr };

    util::ptr<Surface> m_surface{ nullptr };
    util::ptr<Swapchain> m_swapchain{ nullptr };
    util::ptr<CommandPool> m_commandPool{ nullptr };
    util::ptr<CommandPool> m_transferCmdPool{ nullptr };
    util::Ref<Queue>* m_graphicsQueue{ nullptr };
    util::Ref<Queue>* m_presentQueue{ nullptr };
    util::Ref<Queue>* m_transferQueue{ nullptr };

    //////////////////////////////////////////////////////
    //////////////////////////////////////////////////////
    //// SPECIFIC VARIABLES
    //////////////////////////////////////////////////////
    
    // Almost all of these may/will be replaced with encapsulating
    // classes, e.g. manager or other.

    // global
    VkSampler m_sampler{ nullptr };
    util::ptr<Buffer> m_modelUBO;
    util::ptr<Buffer> m_cameraUBO;
    util::ptr<Buffer> m_lightsUBO;

    // gbuffer/deferred pass
    util::ptr<GeometryStep> m_geometryStep;
    util::ptr<Framebuffer> m_gbuffer{ nullptr };
    VkSemaphore m_deferredSemaphore{ nullptr };

    // final fsq pass
    util::ptr<FinalStep> m_finalStep;

    // Scene variables
    util::Ref<Camera> m_camera { s_defaultCamera };
    LightContainer m_lights;
    SceneContainer m_objList;
    size_t m_modelUBOdynamicAlignment {0};
    ObjectUniform* m_modelUBOdata = nullptr;

    // Specific, per-swapchain-image variables

#ifdef _DEBUG
    VkDebugUtilsMessengerEXT m_debugMessenger{ nullptr };
#endif
  };
}

#endif
