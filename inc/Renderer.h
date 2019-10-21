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
  class ShadowMapStep;
  class BlurStep;
  class GlobalLightStep;
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
    alignas(04) float farDist;
    alignas(04) float nearDist;
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
    using GlobalLightContainer = std::vector<ShadowedLight>;

    void setScene(SceneContainer const& objects);
    void setLocalLights(LightContainer const& lights);
    void setGlobalLights(GlobalLightContainer lights);

    void setCamera(util::Ref<Camera> camera);

    void drawFrame();

    void shutdown();

    struct ShadowMappedLight {
      ShadowMappedLight(ShadowedLight const& light);

      ShadowedLight m_light;
      util::ptr<Framebuffer> m_depthBuffer;
    };

  private:
    static constexpr VkExtent3D SHADOW_DEPTH_MAP_EXTENT = { 1024, 1024, 1 };

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
    void setupFrameBuffers() const;
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
    util::ptr<Buffer> m_localLightsUBO;
    util::ptr<Buffer> m_globalLightsUBO;

    // gbuffer/deferred pass
    util::ptr<GeometryStep> m_geometryStep;
    util::ptr<Framebuffer> m_gbuffer{ nullptr };
    VkSemaphore m_deferredSemaphore{ nullptr };

    // shadow map pass
    util::ptr<ShadowMapStep> m_shadowMapStep;
    VkSemaphore m_shadowSemaphore{ nullptr };

    // blur pass
    util::ptr<BlurStep> m_blurStep{ nullptr };
    util::ptr<DependentImage> m_blurIntermediate{ nullptr };
    util::ptr<ImageView> m_blurIntermediateView{ nullptr };
    VkSemaphore m_blurSemaphore{ nullptr };

    // global lighting pass
    util::ptr<GlobalLightStep> m_globalLightStep;
    util::ptr<Framebuffer> m_globalLitFrameBuffer;
    VkSemaphore m_globalLightSemaphore{ nullptr };

    // final fsq pass
    util::ptr<FinalStep> m_finalStep;
    //util::ptr<Framebuffer> m_localLitFramebuffer;
    VkSemaphore m_localLitSemaphore{ nullptr };

    // Scene variables
    util::Ref<Camera> m_camera { s_defaultCamera };
    std::vector<ShadowMappedLight> m_globalLights;
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
