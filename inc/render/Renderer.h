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
#include "MeshManager.h"
#include "Texture.h"

#include "obj/Object.h"
#include "obj/Camera.h"
#include "obj/Light.h"
#include "obj/Material.h"
#include "util/Utils.h"

#include "app/Scene.h"
#include "app/ImGui.h"

#include <unordered_map>

namespace dw {
  //class Camera;
  class GeometryStep;
  class ShadowMapStep;
  class BlurStep;
  class GlobalLightStep;
  class LocalLightingStep;
  class AmbientStep;
  class FinalStep;
  class SplashScreenStep;

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
    alignas(04) int mtlIndex;
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
    void init(GLFWWindow* window, bool startImgui = true);

    void restartWindow();

    NO_DISCARD bool done() const;

    void uploadMeshes(MeshManager::MeshMap& meshes) const;
    void uploadMaterials(MaterialManager::MtlMap& materials);
    void uploadTextures(TextureManager::TexMap& textures) const;

    void setScene(util::ptr<Scene> scene);

    void drawFrame();
    void displayLogo(util::ptr<ImageView> logoView);

    void shutdown(bool shutdownImgui = true);

    struct ShadowMappedLight {
      ShadowMappedLight(ShadowedLight const& light);

      ShadowedLight m_light;
      util::ptr<Framebuffer> m_depthBuffer;
    };

    // contains control
    struct ShaderControl {
      alignas(04) float global_momentBias {0.00000005f};
      alignas(04) float global_depthBias  {0.0004f};
      alignas(04) float geometry_defaultRoughness{ 0.16f };
      alignas(04) float geometry_defaultMetallic { 0.08f };
      alignas(04) float final_toneMapExposure{ 1.f };
      alignas(04) float final_toneMapExponent{ 1.f };
      alignas(04) int   final_doLocalLighting{ 1 };
      alignas(04) int   global_doGlobalLighting{ 1 };
      alignas(04) int   global_enableShadows{ 1 };
      alignas(04) int   global_enableIBL{ 1 };
      alignas(04) int   global_enableBackgrounds{ 1 };
    };

    void setShaderControl(ShaderControl* control);

    // NOTE: You can currently only disable local lighting via the shader control, as the local
    // lighting is done as a part of the final render pass rendering to the backbuffer.
    // Turn the whole step off and you get nothing, you lose, good day sir.

    void setShadowMapBlurEnabled(bool enabled = true) { m_blurEnabled = enabled; }
    void setGlobalLightingEnabled(bool enabled = true) { m_globalLightEnabled = enabled; }

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

    void setupWindow();
    void shutdownWindow();

    // shutdown helpers
    void shutdownScene();
    void recreateSwapChain();

#ifdef DW_USE_IMGUI
    // imgui
    void setupImGui();
    void shutdownImGui() const;
    VkDescriptorPool m_imguiDescriptorPool{ nullptr };
#endif

    // shader control
    ShaderControl* m_shaderControl{ nullptr };
    util::ptr<Buffer> m_shaderControlBuffer{ nullptr };
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
    util::ptr<CommandPool> m_graphicsCmdPool{ nullptr };
    util::ptr<CommandPool> m_transferCmdPool{ nullptr };
    util::ptr<CommandPool> m_computeCmdPool{ nullptr };
    util::Ref<Queue>* m_graphicsQueue{ nullptr };
    util::Ref<Queue>* m_presentQueue{ nullptr };
    util::Ref<Queue>* m_transferQueue{ nullptr };
    util::Ref<Queue>* m_computeQueue{ nullptr };

    //////////////////////////////////////////////////////
    //////////////////////////////////////////////////////
    //// SPECIFIC VARIABLES
    //////////////////////////////////////////////////////
    
    // Almost all of these may/will be replaced with encapsulating
    // classes, e.g. manager or other.

    // global
    VkSampler m_sampler{ nullptr };       //!< Sampler used for sampling the gbuffer
    util::ptr<Buffer> m_modelUBO;         //!< Contains all model matrices for objects in scene
    util::ptr<Buffer> m_cameraUBO;        //!< Contains rendering camera info
    util::ptr<Buffer> m_localLightsUBO;   //!< Contains all local light info
    util::ptr<Buffer> m_globalLightsUBO;  //!< Contains all global (shadow mapped) light info
    util::ptr<Buffer> m_globalImportanceUBO;
    util::ptr<Buffer> m_materialsUBO;     //!< Contains the coefficients for the materials
    // TODO: not this this is hacky
    MaterialManager::MtlMap* m_materials {nullptr};

    bool m_blurEnabled{ true };
    bool m_globalLightEnabled{ true };
    // bool m_ambientLightEnabled{ true };

    // logo display pass
    util::ptr<SplashScreenStep> m_splashScreenStep;

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

    // local lighting pass
    util::ptr<LocalLightingStep> m_localLightStep;
    util::ptr<Framebuffer> m_localLitFramebuffer;
    VkSemaphore m_localLightSemaphore{ nullptr };

    // ambient pass
    util::ptr<AmbientStep> m_ambientStep;
    util::ptr<Framebuffer> m_ambientFramebuffer;
    VkSemaphore m_ambientSemaphore{ nullptr };

    // final fsq pass
    util::ptr<FinalStep> m_finalStep;
    VkSemaphore m_finalSemaphore{ nullptr };

    // Scene variables
    util::ptr<Scene> m_scene{ nullptr };
    std::vector<ShadowMappedLight> m_globalLights;
    size_t m_modelUBOdynamicAlignment {0};
    ObjectUniform* m_modelUBOdata = nullptr;

    // Specific, per-swapchain-image variables

#ifdef _DEBUG
    VkDebugUtilsMessengerEXT m_debugMessenger{ nullptr };
#endif
  };
}

#endif
