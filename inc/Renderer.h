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

namespace dw {
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

  class Renderer {
  public:
    // initialize
    void initGeneral();
    void initSpecific();

    bool done() const;

    void uploadMeshes(std::vector<util::ptr<Mesh>> const& meshes) const;

    void setScene(std::vector<Object> const& objects);
    void drawFrame();

    void shutdown();

  private:
    //////////////////////////////////////////////////////
    //////////////////////////////////////////////////////
    //// HELPER FUNCTIONS
    //////////////////////////////////////////////////////
    // general vulkan
    void openWindow();
    void setupInstance();
    void setupHelpers(); // e.g. debug messenger
    void setupDevice();
    void setupSurface();
    void setupSwapChain();
    void setupCommandPools();
    void setupCommandBuffers();
    void transitionSwapChainImages();

    // specific to the rendering engine & what i support
    void setupShaders();
    void setupDescriptors();
    void setupRenderSteps();
    void setupSwapChainFrameBuffers() const;
    void setupPipeline();

    // drawing helpers
    void writeCommandBuffers();
    void updateUniformBuffers(uint32_t imageIndex);

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
    util::ptr<RenderPass> m_renderPass{ nullptr };
    util::ptr<IShader>  m_triangleVertShader{ nullptr };
    util::ptr<IShader>  m_triangleFragShader{ nullptr };
    VkPipelineLayout m_graphicsPipelineLayout{ nullptr };
    VkPipeline m_graphicsPipeline{ nullptr };
    VkDescriptorSetLayout m_descriptorSetLayout{ nullptr };
    VkDescriptorPool m_descriptorPool{nullptr};

    // Specific, per-swapchain-image variables
    std::vector<util::Ref<CommandBuffer>> m_commandBuffers;
    std::vector<Buffer> m_uniformBuffers;
    std::vector<Object> m_objList;
    std::vector<VkDescriptorSet> m_descriptorSets;

#ifdef _DEBUG
    VkDebugUtilsMessengerEXT m_debugMessenger{ nullptr };
#endif
  };
}

#endif
