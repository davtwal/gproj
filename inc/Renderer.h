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

#include "MeshManager.h"

#include <unordered_map>

namespace dw {
  //class Camera;
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

  struct FSQViewUniform {
    alignas(16) glm::vec3 eye;
    alignas(16) glm::vec3 dir;
  };

  class Renderer {
  public:
    // initialize
    void initGeneral();
    void initSpecific();

    NO_DISCARD bool done() const;
    NO_DISCARD MeshManager& getMeshManager();

    void uploadMeshes(std::vector<util::Ref<Mesh>> const& meshes) const;
    void uploadMeshes(std::unordered_map<uint32_t, Mesh>& meshes) const;

    using SceneContainer = std::vector<util::Ref<Object>>;

    void setScene(SceneContainer const& objects);
    void setCamera(util::Ref<Camera> camera);

    void drawFrame();

    void shutdown();

  private:
    //////////////////////////////////////////////////////
    //////////////////////////////////////////////////////
    //// HELPER FUNCTIONS
    //////////////////////////////////////////////////////
    // general vulkan setup
    void openWindow();
    void setupInstance();
    void setupHelpers(); // e.g. debug messenger
    void setupDevice();
    void setupSurface();
    void setupSwapChain();
    void setupCommandPools();
    void setupCommandBuffers();

    // specific to the rendering engine & what i support setup
    void setupGBufferImages();
    void setupDepthTestResources();
    void transitionRenderImages();
    void setupRenderSteps();
    void setupDescriptors();
    void setupUniformBuffers();
    void setupSamplers();
    void setupShaders();
    void setupPipeline();

    void setupGBufferFrameBuffer();
    void setupSwapChainFrameBuffers() const;
    void initManagers();

    // specific to the current scene
    void writeCommandBuffers();
    void releasePreviousDynamicUniforms();
    void prepareDynamicUniformBuffers();
    void updateDescriptorSets();

    // called every frame
    void updateUniformBuffers(uint32_t imageIndex);// , Camera& cam, Object& obj);

    // shutdown helpers
    void shutdownScene();
    void shutdownManagers();

    void recreateSwapChain();
    //////////////////////////////////////////////////////
    //////////////////////////////////////////////////////
    //// GENERAL VARIABLES
    //////////////////////////////////////////////////////
    static Camera s_defaultCamera;
    ///
    VulkanControl* m_control{ nullptr };
    GLFWWindow* m_window{ nullptr };
    InputHandler* m_inputHandler{ nullptr };
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

    VkSampler m_sampler{ nullptr };
    // gbuffer/deferred pass
    std::vector<DependentImage> m_gbufferImages;
    std::vector<ImageView> m_gbufferViews;
    util::ptr<DependentImage> m_depthStencilImage{ nullptr };
    util::ptr<ImageView> m_depthStencilView{ nullptr };
    util::ptr<Framebuffer> m_gbuffer{ nullptr };
    util::ptr<RenderPass> m_deferredPass{ nullptr };
    util::ptr<util::Ref<CommandBuffer>> m_deferredCmdBuff;
    util::ptr<Buffer> m_modelUBO;
    util::ptr<Buffer> m_cameraUBO;
    VkPipelineLayout m_deferredPipeLayout{ nullptr };
    VkPipeline m_deferredPipeline { nullptr };

    util::ptr<IShader>  m_triangleVertShader{ nullptr };
    util::ptr<IShader>  m_triangleFragShader{ nullptr };

    VkDescriptorSetLayout m_deferredDescSetLayout{ nullptr };
    VkDescriptorPool m_deferredDescPool{ nullptr };
    VkDescriptorSet m_deferredDescSet{ nullptr };

    VkSemaphore m_deferredSemaphore{ nullptr };

    // final fsq pass
    std::vector<util::Ref<CommandBuffer>> m_commandBuffers;
    util::ptr<RenderPass> m_finalPass{ nullptr };
    util::ptr<IShader> m_fsqVertShader{ nullptr };
    util::ptr<IShader> m_fsqFragShader{ nullptr };
    VkPipeline m_finalPipeline{ nullptr };
    VkPipelineLayout m_finalPipeLayout{ nullptr };
    VkDescriptorSetLayout m_finalDescSetLayout{ nullptr };
    VkDescriptorPool m_finalDescPool{ nullptr };
    std::vector<VkDescriptorSet> m_finalDescSets;

    //////////////////////////////////////////////////////
    //// Managers
    MeshManager m_meshManager;

    // Scene variables
    util::Ref<Camera> m_camera { s_defaultCamera };
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
