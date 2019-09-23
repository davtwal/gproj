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
#include <string>
#include <vector>

#include "VulkanControl.h"
#include "GLFWControl.h"
#include "GLFWWindow.h"

#include "Surface.h"
#include "Swapchain.h"
#include "Trace.h"
#include "Queue.h"
#include "CommandBuffer.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include <cassert>

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

    LogicalDevice::QueueList queueList;
    queueList.push_back(std::make_pair(0, std::vector<float>({1})));

    m_device = new LogicalDevice(physical, deviceLayers, deviceExtensions, queueList, features, features, false);
  }

  void Application::setupSurface() {
    m_surface = new Surface(*m_window, m_device->getOwningPhysical());
  }

  void Application::setupSwapChain() {
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
      return;

    VkBool32 supported = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(m_device->getOwningPhysical(),
                                         (*m_graphicsQueue)->getFamily(),
                                         *m_surface,
                                         &supported);

    if (!supported)
      throw std::runtime_error("graphics queue does not support presenting");

    m_swapchain = new Swapchain(*m_device, *m_surface, *m_graphicsQueue);
  }


  void Application::setupRenderpasses() {
    m_renderPass = new RenderPass(*m_device);
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


  void Application::setupCommandBuffers() {
    m_commandPool = new CommandPool(*m_device, (*m_graphicsQueue)->getFamily());

    // buffers will automatically be returned to the command pool and freed when the pool is destroyed
    size_t imageCount = m_swapchain->getNumImages();
    m_commandBuffers.reserve(imageCount);
    for(size_t i = 0; i < imageCount; ++i) {
      m_commandBuffers.emplace_back(m_commandPool->allocateCommandBuffer());
    }



  }

  void Application::setupPipeline() {

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
    setupPipeline();
    setupCommandBuffers();

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

      //vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, nullptr);
      //vkCmdDraw(*commandBuffer, 3, 1, 0, 0);

      vkCmdEndRenderPass(*commandBuffer);

      commandBuffer->end();
    }
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
    delete m_graphicsQueue;
    m_graphicsQueue = nullptr;

    delete m_renderPass;
    m_renderPass = nullptr;

    delete m_commandPool;
    m_commandPool = nullptr;

    delete m_swapchain;
    m_swapchain = nullptr;

    delete m_surface;
    m_swapchain = nullptr;

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
