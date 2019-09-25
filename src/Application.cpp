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

#include <cassert>
#include <algorithm>

namespace dw {
  int Application::parseCommandArgs(int, char**) {
    return 0;
  }

  int Application::run() {
    if (initialize() == 1 || loop() == 1 || shutdown() == 1)
      return 1;

    return 0;
  }

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                      VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                      void*                                       pUserData) {

    Trace& outTrace = messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                        ? Trace::All
                        : messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                            ? Trace::Info
                            : messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                ? Trace::Warn
                                : Trace::Error;

    outTrace << "VK:[" << (messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                             ? "GENERAL"
                             : messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
                                 ? "PERFORMANCE"
                                 : "VALIDATION")
        << "] " << pCallbackData->pMessage << Trace::Stop;

    return VK_FALSE;
  }

  void Application::fillInstanceExtLayerVecs(std::vector<const char*>& exts, std::vector<const char*>& layers) {
    exts = GLFWControl::GetRequiredVKExtensions();

#ifdef _DEBUG
    layers.push_back(VulkanControl::LAYER_STANDARD_VALIDATION);
    //instanceLayers.push_back(VulkanControl::LAYER_RENDERDOC_CAPTURE);

    exts.push_back(VulkanControl::EXT_DEBUG_REPORT);
    exts.push_back(VulkanControl::EXT_DEBUG_UTILS);
#endif
  }

  namespace {
    VkDebugUtilsMessengerEXT debug_messenger = nullptr;
  }

  void Application::setupDebug() {
#ifdef _DEBUG
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      nullptr,
      0,
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      debugCallback,
      nullptr
    };

    auto createFn = (PFN_vkCreateDebugUtilsMessengerEXT)glfwGetInstanceProcAddress(*m_control,
                                                                                   "vkCreateDebugUtilsMessengerEXT");

    if (createFn) {
      createFn(*m_control, &createInfo, nullptr, &debug_messenger);
    }
    else
      throw std::runtime_error("Debug Extension Not Present");
#endif
  }

  void Application::openWindow() {
    m_window       = new GLFWWindow(800, 800, "hey lol");
    m_inputHandler = new InputHandler(*m_window);

    m_window->setInputHandler(m_inputHandler);
  }

  void Application::setupInstance() {
    m_control = new VulkanControl("GPROJ", VK_MAKE_VERSION(1, 1, 100), 1);

    std::vector<const char*> instanceExtensions;
    std::vector<const char*> instanceLayers;

    fillInstanceExtLayerVecs(instanceExtensions, instanceLayers);

    m_control->initInstance(instanceExtensions, instanceLayers);
    m_control->registerPhysicalDevices();
  }

  void Application::setupDevice(PhysicalDevice& physical) {
    auto devExt   = physical.getAvailableExtensions();
    auto devLayer = physical.getAvailableLayers();

    std::vector<const char*> deviceExtensions;
    std::vector<const char*> deviceLayers;

    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    VkPhysicalDeviceFeatures features               = {};
    features.robustBufferAccess                     = 1;  // vulkan does bounds checking on buffer access for us
    features.fillModeNonSolid                       = 1;  // wireframe
    features.geometryShader                         = 1;  // enable geometry shaders
    features.tessellationShader                     = 1;  // enable tesselation shaders
    features.imageCubeArray                         = 1;  // enable cubemaps
    features.dualSrcBlend                           = 1;  // enable blending with multiple sources
    features.alphaToOne                             = 1;  // enable setting of alpha values to 1
    features.multiViewport                          = 1;  // enable using multiple viewports at once
    features.shaderTessellationAndGeometryPointSize = 1;  // enable tess/geometry shaders to have big point sizes
    features.wideLines                              = 1;  // enable lines wider than 1.0
    features.largePoints                            = 1;  // enable points bigger than 1.0

    uint32_t graphicsFamily = physical.pickQueueFamily(VK_QUEUE_GRAPHICS_BIT);
    uint32_t transferFamily = physical.pickQueueFamily(VK_QUEUE_TRANSFER_BIT);

    LogicalDevice::QueueList queueList;
    queueList.push_back(std::make_pair(graphicsFamily, std::vector<float>({ 1 })));

    if (graphicsFamily != transferFamily)
      queueList.push_back(std::make_pair(transferFamily, std::vector<float>({ 1 })));

    m_device = new LogicalDevice(physical, deviceLayers, deviceExtensions, queueList, features, features, false);

    // TODO better selection of presentation queue
    // its possible that queues that support graphics & presenting don't overlap
    // perhaps select inside of LogicalDevice which queue family out of the selected
    // would be best? because we can check for presentation support with just
    // a physical device, perhaps a function that returns ideal indices for
    // presenting, graphics, compute, etc.
    //
    // Queue manager?
    m_graphicsQueue = new util::Ref<Queue>(m_device->getBestQueue(VK_QUEUE_GRAPHICS_BIT));
    if (!(*m_graphicsQueue)->isValid())
      throw std::runtime_error("no graphics queue available");

    m_transferQueue = new util::Ref<Queue>(m_device->getBestQueue(VK_QUEUE_TRANSFER_BIT));
    assert(m_transferQueue && (*m_transferQueue)->isValid()); // graphics queues implicitly allow transfer
  }

  void Application::setupSurface() {
    m_surface = std::make_unique<Surface>(*m_window, m_device->getOwningPhysical());

    m_presentQueue = new util::Ref<Queue>(m_device->getPresentableQueue(*m_surface));
    if (!(*m_presentQueue)->isValid())
      throw std::runtime_error("no present queue available");
  }

  void Application::setupSwapChain() {
    m_swapchain = std::make_unique<Swapchain>(*m_device, *m_surface, *m_graphicsQueue);
  }


  void Application::setupRenderpasses() {
    m_renderPass = std::make_unique<RenderPass>(*m_device);
    m_renderPass->reserveAttachments(1);
    m_renderPass->reserveSubpasses(1);
    m_renderPass->reserveAttachmentRefs(1);
    m_renderPass->reserveSubpassDependencies(1);

    m_renderPass->addAttachment(m_swapchain->getImageAttachmentDesc());
    m_renderPass->addAttachmentRef(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, RenderPass::arfColor);

    m_renderPass->finishSubpass();

    m_renderPass->addSubpassDependency({
                                         VK_SUBPASS_EXTERNAL,
                                         0,
                                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                         0,
                                         VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
                                         0
                                       });

    m_renderPass->finishRenderPass();
  }

  void Application::setupSwapChainFrameBuffers() {
    m_swapchain->createFramebuffers(*m_renderPass);
  }
  void Application::setupShaders() {
    m_triangleVertShader = std::make_unique<Shader<ShaderStage::Vertex>>(ShaderModule::Load(*m_device, "fromBuffer_vert.spv"));
    m_triangleFragShader = std::make_unique<Shader<ShaderStage::Fragment>>(ShaderModule::Load(*m_device, "triangle_frag.spv"));
  }

  void Application::setupPipeline() {

    auto vertexAttributes = Vertex::GetBindingAttributes();
    auto vertexBindings = Vertex::GetBindingDescriptions();
    // Eventually each vertex type will have its own GetBindingDescriptions()
    // and GetAttributeDescriptions() sort of functions which can be put here.
    // maybe
    VkPipelineVertexInputStateCreateInfo vertInputInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      nullptr,
      0,
      static_cast<uint32_t>(vertexBindings.size()),
      vertexBindings.data(),
      static_cast<uint32_t>(vertexAttributes.size()),
      vertexAttributes.data()
    };

    // I'm not sure how I'll do the input assembly.
    // Most likely, I will only ever do triangle lists for everything, and
    // things like lines will have their own graphics pipeline
    // for debug draw or something.
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      nullptr,
      0,
      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      VK_FALSE
    };

    // This will want to be dynamic if I want to have dynamically resize-able windows
    // where the resolution is also changed, though I likely won't need that.
    // If I do allow for resizing windows, then either I make this dynamic, or
    // recreate the entire pipeline along with everything I'd already have to refresh:
    //  - Swap-chain
    //  - Surface
    //  - Frame-buffers
    // Which probably won't be a lot of extra work considering people expect changing
    // resolutions on high-graphical-intensity rendering engines to take time (e.g. games)
    // and we can take almost however long we want so long as its within ~5 seconds worst case
    // (a lot of time). Probably not dynamic.
    // Apparently minDepth is allowed to be greater than maxDepth but both need to be [0..1]
    VkViewport viewport = {
      0,
      0,
      static_cast<float>(m_surface->getWidth()),
      static_cast<float>(m_surface->getHeight()),
      0,
      1.f
    };

    // Its probably a little weird im using surface for the viewport and swapchain for the
    // extent, and I should probably make similar functions available for both. Later.
    VkRect2D scissor = {
      {0, 0},
      m_swapchain->getImageSize()
    };

    VkPipelineViewportStateCreateInfo viewportStateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      nullptr,
      0,
      1,
      &viewport,
      1,
      &scissor
    };

    // The only available things that can be dynamic by default are the
    // viewport, scissor, line width, depth bias, blend constants,
    // depth bounds, and some stencil stuff.
    VkPipelineRasterizationStateCreateInfo rasterizerInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      nullptr,
      0,
      VK_FALSE, // if I want stuff outside of the back/front of the frustum to be discarded or clamped
      VK_FALSE,
      VK_POLYGON_MODE_FILL, // for things like wireframe
      VK_CULL_MODE_BACK_BIT, // back/front culling: can reduce acne by front culling instead of back
      VK_FRONT_FACE_CLOCKWISE,
      VK_FALSE, // the rasterizer can alter depth values, its cool but optional
      0,
      0,
      0,
      1.f // anything thicker than 1.f requires the wideLines feature on the GPU
    };

    // I'm most likely not going to support multisampling unless I do an anti-aliasing
    // Project 5. If I need it for anything else then ???
    VkPipelineMultisampleStateCreateInfo multisampleInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      nullptr,
      0,
      VK_SAMPLE_COUNT_1_BIT,
      VK_FALSE,
      1.f,
      nullptr,
      VK_FALSE,
      VK_FALSE
    };

    //VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {};

    // This color blend attachment might need to become a per-framebuffer sort of thing?
    // It really depends on what the format is and what the use of the framebuffer is,
    // so I'm not sure where I'll end up putting this info.
    VkPipelineColorBlendAttachmentState colorAttachmentInfo = {
      VK_TRUE,
      VK_BLEND_FACTOR_SRC_ALPHA,
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      VK_BLEND_OP_ADD,
      VK_BLEND_FACTOR_ONE,
      VK_BLEND_FACTOR_ZERO,
      VK_BLEND_OP_ADD,
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    // I doubt this will ever change
    VkPipelineColorBlendStateCreateInfo colorBlendInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      nullptr,
      0,
      VK_FALSE, // Setting this to true changes color blend to color bitwise combination. Turns blending off for all attachments
      VK_LOGIC_OP_COPY,
      1,
      &colorAttachmentInfo,
      {0, 0, 0, 0} // Not sure what these do
    };

    // Compiling the shader stages together
    auto vertInfo = m_triangleVertShader->getCreateInfo();
    auto fragInfo = m_triangleFragShader->getCreateInfo();

    std::vector<VkPipelineShaderStageCreateInfo> stageInfo;
    stageInfo.reserve(2);
    stageInfo.push_back(vertInfo);
    stageInfo.push_back(fragInfo);

    // Pipeline layout. This is where I define both push constants and descriptor sets,
    // both of which aren't useful to me yet. I'm not sure how I'll eventually manage
    // the creation of these layouts, because shaders may or may not be compatible with
    // a given pipeline layout given if the shader actually uses descriptor sets.
    // But then again, I don't know enough about those yet so *shrug*
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      nullptr,
      0,
      0,
      nullptr,
      0,
      nullptr
    };

    vkCreatePipelineLayout(*m_device, &pipelineLayoutInfo, nullptr, &m_graphicsPipelineLayout);
    assert(m_graphicsPipelineLayout);

    // Note: It's possible to use different renderpasses with a pipeline, but they have to be
    // compatible with m_renderPass which is something different entirely. Perhaps I will
    // look into this, but I doubt I'll need it for a while.
    VkGraphicsPipelineCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      nullptr,
      0,
      static_cast<uint32_t>(stageInfo.size()),
      stageInfo.data(),
      &vertInputInfo,
      &inputAssemblyInfo,
      nullptr,  // No tessellation for now
      &viewportStateInfo,
      &rasterizerInfo,
      &multisampleInfo,
      nullptr, // no depth test for now (soon to change)
      &colorBlendInfo,
      nullptr, // no dynamic stages yet
      m_graphicsPipelineLayout,
      *m_renderPass,
      0,
      VK_NULL_HANDLE, // You can create derivative pipelines, maybe this will be good later but imnotsure
      -1
    };

    vkCreateGraphicsPipelines(*m_device, nullptr, 1, &createInfo, nullptr, &m_graphicsPipeline);
    assert(m_graphicsPipeline);
  }

  void Application::setupCommandPool() {
    m_commandPool = std::make_unique<CommandPool>(*m_device, (*m_graphicsQueue)->getFamily());
    m_transferCmdPool = std::make_unique<CommandPool>(*m_device, m_transferQueue->ref.getFamily());
  }

  void Application::setupVertexBuffer() {
    std::vector<Vertex> vertices = {
      {{-.5, -.5f}, {1, 0, 0}},
    {{.5f, -.5f}, {1, 1, 1}},
      {{.5f, .5f}, {0, 1, 0}},
    {{-.5f, .5f}, {0, 0, 1}},
    };

    std::vector<uint16_t> indices = {
      0, 1, 2, 2, 3, 0
    };

    size_t vertexSize = sizeof(Vertex) * vertices.size();
    size_t indexSize = sizeof(uint16_t) * indices.size();
    Buffer vertexStaging = Buffer::CreateStaging(*m_device, vertexSize);
    Buffer indexStaging = Buffer::CreateStaging(*m_device, indexSize);

    // the buffer has been created with host coherent so we don't invalidate/flush
    void* vertexData = vertexStaging.map();
    void* indexData = indexStaging.map();
    memcpy(vertexData, vertices.data(), vertexSize);
    memcpy(indexData, indices.data(), indexSize);
    vertexStaging.unMap();
    indexStaging.unMap();

    m_vertexBuffer = std::make_unique <Buffer>(Buffer::CreateVertex(*m_device, vertexSize));
    m_indexBuffer = std::make_unique<Buffer>(Buffer::CreateIndex(*m_device, indexSize));

    CommandBuffer& moveBuff = m_transferCmdPool->allocateCommandBuffer();

    moveBuff.start(true);
    // srcOffset/dstOffset = optional
    VkBufferCopy vertCopyRegion = {0, 0, vertexSize};
    VkBufferCopy indexCopyRegion = { 0, 0, indexSize };
    vkCmdCopyBuffer(moveBuff, vertexStaging, *m_vertexBuffer, 1, &vertCopyRegion);
    vkCmdCopyBuffer(moveBuff, indexStaging, *m_indexBuffer, 1, &indexCopyRegion);
    moveBuff.end();

    m_transferQueue->ref.submitOne(moveBuff);
    m_transferQueue->ref.waitIdle();

    m_transferCmdPool->freeCommandBuffer(moveBuff);
  }

  void Application::setupCommandBuffers() {
    // buffers will automatically be returned to the command pool and freed when the pool is destroyed
    size_t imageCount = m_swapchain->getNumImages();
    m_commandBuffers.reserve(imageCount);
    for (size_t i = 0; i < imageCount; ++i) {
      m_commandBuffers.emplace_back(m_commandPool->allocateCommandBuffer());
    }

    auto& framebuffers = m_swapchain->getFrameBuffers();
    for (size_t i = 0; i < m_swapchain->getNumImages(); ++i) {
      auto& commandBuffer = m_commandBuffers[i];

      commandBuffer->start(false);

      VkClearValue clearValue = {
        {{0.f}}
      };

      VkRenderPassBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        nullptr,
        *m_renderPass,
        framebuffers[i],
        {{0, 0}, m_swapchain->getImageSize()},
        1,
        &clearValue
      };

      vkCmdBeginRenderPass(*commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

      vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

      const VkBuffer& buff = (VkBuffer)*m_vertexBuffer;
      const VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(*commandBuffer, 0, 1, &buff, &offset);
      vkCmdBindIndexBuffer(*commandBuffer, *m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);
      vkCmdDrawIndexed(*commandBuffer, 6, 1, 0, 0, 0);

      vkCmdEndRenderPass(*commandBuffer);

      commandBuffer->end();
    }
  }

  int Application::initialize() {
    GLFWControl::Init();

    openWindow();
    setupInstance();
    setupDebug();

    auto&           physDevs = m_control->getPhysicalDevices();
    PhysicalDevice& physical = physDevs.front();

    setupDevice(physical);
    setupSurface();
    setupSwapChain();
    setupRenderpasses();
    setupSwapChainFrameBuffers();
    setupShaders();
    setupPipeline();
    setupCommandPool();
    setupVertexBuffer();
    setupCommandBuffers();

    return 0;
  }

  int Application::loop() const {
    assert(m_swapchain->isPresentReady());

    while (!m_window->shouldClose()) {
      GLFWControl::Poll();

      uint32_t     nextImageIndex = m_swapchain->getNextImageIndex();
      Image const& nextImage      = m_swapchain->getNextImage();

      auto& graphicsQueue = *m_graphicsQueue;

      graphicsQueue->submitOne(*m_commandBuffers[nextImageIndex],
                               {m_swapchain->getNextImageSemaphore()},
                               {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
                               {m_swapchain->getImageRenderReadySemaphore()});

      graphicsQueue->waitSubmit();
      graphicsQueue->waitIdle();

      m_swapchain->present();
      graphicsQueue->waitIdle();

      //graphicsQueue.present();
    }

    return 0;
  }

  void Application::shutdownDebug() {
    auto destroyFn = (PFN_vkDestroyDebugUtilsMessengerEXT)glfwGetInstanceProcAddress(*m_control,
                                                                                     "vkDestroyDebugUtilsMessengerEXT");
    if (destroyFn) {
      destroyFn(*m_control, debug_messenger, nullptr);
      debug_messenger = nullptr;
    }
    else
      throw std::runtime_error("Debug Extension Not Present");
  }

  int Application::shutdown() {
    vkDeviceWaitIdle(*m_device);

    delete m_graphicsQueue;
    m_graphicsQueue = nullptr;

    delete m_transferQueue;
    m_transferQueue = nullptr;

    delete m_presentQueue;
    m_presentQueue = nullptr;

    m_vertexBuffer.reset();
    vkDestroyPipeline(*m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(*m_device, m_graphicsPipelineLayout, nullptr);
    m_triangleFragShader.reset();
    m_triangleVertShader.reset();
    m_renderPass.reset();
    m_transferCmdPool.reset();
    m_commandPool.reset();
    m_swapchain.reset();
    m_surface.reset();

    delete m_device;
    m_device = nullptr;

    shutdownDebug();

    delete m_control;
    m_control = nullptr;

    delete m_window;
    m_window = nullptr;

    delete m_inputHandler;
    m_inputHandler = nullptr;

    GLFWControl::Shutdown();
    return 0;
  }
}
