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

  class Renderer {
  public:
    // initialize
    void initGeneral();
    void initSpecific();

    NO_DISCARD bool done() const;
    NO_DISCARD MeshManager& getMeshManager();

    void uploadMeshes(std::vector<util::Ref<Mesh>> const& meshes) const;
    void uploadMeshes(std::unordered_map<uint32_t, Mesh>& meshes) const;

    void setScene(Camera const& camera, std::vector<util::Ref<Object>> const& objects);
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
    void transitionSwapChainImages() const;

    // specific to the rendering engine & what i support setup
    void setupDepthTestResources();

    void setupShaders();
    void setupDescriptors();
    void setupRenderSteps();
    void setupSwapChainFrameBuffers() const;
    void setupPipeline();
    void initManagers();

    // drawing helpers
    void writeCommandBuffers();
    void updateUniformBuffers(uint32_t imageIndex);// , Camera& cam, Object& obj);

    // shutdown helpers
    void shutdownManagers();

    void recreateSwapChain();
    //////////////////////////////////////////////////////
    //////////////////////////////////////////////////////
    //// GENERAL VARIABLES
    //////////////////////////////////////////////////////
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
    util::ptr<DependentImage> m_depthStencilImage{ nullptr };
    util::ptr<ImageView> m_depthStencilView{ nullptr };
    util::ptr<RenderPass> m_renderPass{ nullptr };
    util::ptr<IShader>  m_triangleVertShader{ nullptr };
    util::ptr<IShader>  m_triangleFragShader{ nullptr };
    VkPipelineLayout m_graphicsPipelineLayout{ nullptr };
    VkPipeline m_graphicsPipeline{ nullptr };
    VkDescriptorSetLayout m_descriptorSetLayout{ nullptr };
    VkDescriptorPool m_descriptorPool{nullptr};

    //////////////////////////////////////////////////////
    //// Managers
    MeshManager m_meshManager;

    // Scene variables
    Camera m_camera;
    std::vector<util::Ref<Object>> m_objList;

    // Specific, per-swapchain-image variables
    std::vector<util::Ref<CommandBuffer>> m_commandBuffers;
    std::vector<Buffer> m_uniformBuffers;
    std::vector<VkDescriptorSet> m_descriptorSets;

#ifdef _DEBUG
    VkDebugUtilsMessengerEXT m_debugMessenger{ nullptr };
#endif
  };
}

#endif
