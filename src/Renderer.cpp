// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Renderer.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 09m 26d
// * Last Altered: 2019y 10m 26d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "Renderer.h"

#include "VulkanControl.h"
#include "GLFWControl.h"
#include "GLFWWindow.h"
#include "Surface.h"
#include "Swapchain.h"
#include "Trace.h"
#include "Queue.h"
#include "CommandBuffer.h"
#include "RenderPass.h"
#include "Shader.h"
#include "Vertex.h"
#include "Buffer.h"
#include "Mesh.h"
#include "Framebuffer.h"  // ReSharper likes to think this isn't used. IT IS!!!
#include "MemoryAllocator.h"
#include "Camera.h"
#include "Image.h"

#include <array>
#include <cassert>
#include <algorithm>
#include <stdlib.h>
#include "Light.h"

#include "RenderSteps.h"

// Wrapper functions for aligned memory allocation
// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
void* alignedAlloc(size_t size, size_t alignment) {
  void* data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
  data = _aligned_malloc(size, alignment);
#else
  int res = posix_memalign(&data, alignment, size);
  if (res != 0)
    data = nullptr;
#endif
  return data;
}

void alignedFree(void* data) {
#if	defined(_MSC_VER) || defined(__MINGW32__)
  _aligned_free(data);
#else
  free(data);
#endif
}

static void transitionImageLayout(dw::CommandBuffer&  cmdBuff,
                                  dw::Image const&    image,
                                  VkImageLayout       oldLayout,
                                  VkImageLayout       newLayout)
{
  VkImageMemoryBarrier barrier = {
    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    nullptr,
    VK_ACCESS_MEMORY_WRITE_BIT,
    VK_ACCESS_MEMORY_READ_BIT,
    oldLayout,
    newLayout,
    VK_QUEUE_FAMILY_IGNORED,
    VK_QUEUE_FAMILY_IGNORED,
    image,
    {
      VK_IMAGE_ASPECT_COLOR_BIT,
      0,
      1,
      0,
      1
    }
  };
  VkPipelineStageFlags sourceStage, destinationStage;

  if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    if (dw::util::IsFormatStencil(image.getFormat())) {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  }
  else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && (
    newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL || newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    ) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  }

  else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(cmdBuff,
    sourceStage,
    destinationStage,
    0,
    0,
    nullptr,
    0,
    nullptr,
    1,
    &barrier);
}

namespace dw {
  Camera Renderer::s_defaultCamera;

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  ///////////////////////////// SETUP & SHUTDOWN //////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  void Renderer::initGeneral(GLFWWindow* window) {
    assert(window);
    m_window = window;
    setupInstance();
    setupHelpers();
    setupDevice();
    setupSurface();
    setupSwapChain();
    setupCommandPools();
  }

  void Renderer::initSpecific() {
    VkSemaphoreCreateInfo deferredSemaphoreCreateInfo = {
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      nullptr,
      0
    };

    if (vkCreateSemaphore(*m_device, &deferredSemaphoreCreateInfo, nullptr, &m_deferredSemaphore) != VK_SUCCESS)
      throw std::runtime_error("Could not create deferred rendering semaphore");

    //setupCommandBuffers();
    //setupDepthTestResources();
    //setupGBufferImages();
    setupSamplers();
    setupUniformBuffers();
    setupFrameBufferImages();
    setupRenderSteps();
    setupFrameBuffers();
    transitionRenderImages();
    //setupDescriptors();
    //setupShaders();

    //setupPipeline();

  }

  void Renderer::shutdown() {
    vkDeviceWaitIdle(*m_device);

    vkDestroySemaphore(*m_device, m_deferredSemaphore, nullptr);

    m_objList.clear();

    if (m_modelUBOdata)
      alignedFree(m_modelUBOdata);

    m_modelUBOdata = nullptr;

    vkDestroySampler(*m_device, m_sampler, nullptr);

    m_gbuffer.reset();

    m_lightsUBO.reset();
    m_modelUBO.reset();
    m_cameraUBO.reset();

    m_geometryStep.reset();
    m_finalStep.reset();

    m_transferCmdPool.reset();
    m_commandPool.reset();

    delete m_presentQueue;
    m_presentQueue = nullptr;
    m_swapchain.reset();

    m_surface.reset();

    delete m_graphicsQueue;
    m_graphicsQueue = nullptr;

    delete m_transferQueue;
    m_transferQueue = nullptr;

    delete m_device;
    m_device = nullptr;

#ifdef _DEBUG
    auto destroyFn = (PFN_vkDestroyDebugUtilsMessengerEXT)
        glfwGetInstanceProcAddress(*m_control,
                                   "vkDestroyDebugUtilsMessengerEXT");
    if (destroyFn) {
      destroyFn(*m_control, m_debugMessenger, nullptr);
      m_debugMessenger = nullptr;
    }
    else
      throw std::runtime_error("Debug Extension Not Present");
#endif

    delete m_control;
    m_control = nullptr;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //////////////////////////////// RENDER LOOP ////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  bool Renderer::done() const {
    return m_window->shouldClose();
  }

  void Renderer::drawFrame() {
    assert(m_swapchain->isPresentReady());
    if (m_objList.empty())
      return;

    uint32_t     nextImageIndex = m_swapchain->getNextImageIndex();
    Image const& nextImage      = m_swapchain->getNextImage();

    auto&           graphicsQueue   = m_graphicsQueue->get();
    VkCommandBuffer deferredCmdBuff = m_geometryStep->getCommandBuffer();//m_deferredCmdBuff->get();
    VkCommandBuffer presentCmdBuff  = m_finalStep->getCommandBuffer(nextImageIndex);//m_commandBuffers[nextImageIndex].get();

    updateUniformBuffers(nextImageIndex);

    VkPipelineStageFlags semaphoreWaitFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo         submitInfo        = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO,
      nullptr,
      1,
      &m_swapchain->getNextImageSemaphore(),
      &semaphoreWaitFlag,
      1,
      &deferredCmdBuff,
      1,
      &m_deferredSemaphore
    };

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);

    submitInfo.pWaitSemaphores   = &m_deferredSemaphore;
    submitInfo.pSignalSemaphores = &m_swapchain->getImageRenderReadySemaphore();
    submitInfo.pCommandBuffers   = &presentCmdBuff;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);

    //// deferred pass
    /// TODO: Something is wrong with the submit functions and semaphores.
    //graphicsQueue.submitOne(*m_deferredCmdBuff, 
    //                        {m_swapchain->getNextImageSemaphore()},
    //                        {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
    //                        {m_deferredSemaphore});
    //graphicsQueue.waitIdle();
    //
    //// fsq pass
    //
    //graphicsQueue.submitOne(m_commandBuffers[nextImageIndex],
    //                        {m_deferredSemaphore},
    //                        {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
    //                        {m_swapchain->getImageRenderReadySemaphore()});
    //graphicsQueue.waitIdle();

    m_swapchain->present();
    graphicsQueue.waitIdle();
  }

  void Renderer::uploadMeshes(std::unordered_map<uint32_t, Mesh>& meshes) const {
    std::vector<Mesh::StagingBuffs> stagingBuffers;

    stagingBuffers.reserve(meshes.size());
    for (auto& mesh : meshes) {
      stagingBuffers.push_back(mesh.second.createAllBuffs(*m_device));
      mesh.second.uploadStaging(stagingBuffers.back());
    }

    CommandBuffer& moveBuff = m_transferCmdPool->allocateCommandBuffer();

    moveBuff.start(true);
    for (size_t i = 0; i < meshes.size(); ++i) {
      meshes[i].uploadCmds(moveBuff, stagingBuffers[i]);
    }
    moveBuff.end();

    m_transferQueue->get().submitOne(moveBuff);
    m_transferQueue->get().waitIdle();

    m_transferCmdPool->freeCommandBuffer(moveBuff);
  }

  void Renderer::uploadMeshes(std::vector<util::Ref<Mesh>> const& meshes) const {
    std::vector<Mesh::StagingBuffs> stagingBuffers;
    stagingBuffers.reserve(meshes.size());
    for (auto& mesh : meshes) {
      stagingBuffers.push_back(mesh.get().createAllBuffs(*m_device));
      mesh.get().uploadStaging(stagingBuffers.back());
    }

    CommandBuffer& moveBuff = m_transferCmdPool->allocateCommandBuffer();

    moveBuff.start(true);
    for (size_t i = 0; i < meshes.size(); ++i) {
      meshes[i].get().uploadCmds(moveBuff, stagingBuffers[i]);
    }
    moveBuff.end();

    m_transferQueue->get().submitOne(moveBuff);
    m_transferQueue->get().waitIdle();

    m_transferCmdPool->freeCommandBuffer(moveBuff);
  }

  void Renderer::updateUniformBuffers(uint32_t imageIndex) {
    CameraUniform cam = {
      m_camera.get().getView(),
      m_camera.get().getProj(),
      m_camera.get().getEyePos(),
      m_camera.get().getViewDir()
    };

    void* data = m_cameraUBO->map();
    memcpy(data, &cam, sizeof(cam));
    m_cameraUBO->unMap();

    assert(m_modelUBOdata);

    for (uint32_t i = 0; i < m_objList.size(); ++i) {
      m_modelUBOdata[i].model = m_objList[i].get().getTransform();
    }

    data = m_modelUBO->map();
    memcpy(data, m_modelUBOdata, m_modelUBO->getSize());
    m_modelUBO->unMap();

    data                   = m_lightsUBO->map();
    LightUBO* lightUBOdata = reinterpret_cast<LightUBO*>(data);
    for (size_t i     = 0; i < m_lights.size(); ++i)
      lightUBOdata[i] = m_lights[i].get().getAsUBO();
    m_lightsUBO->unMap();

    // Flush to make changes visible to the host
    // we dont do this cus coherent on my machine
    //VkMappedMemoryRange memoryRange = vks::initializers::mappedMemoryRange();
    //memoryRange.memory = uniformBuffers.dynamic.memory;
    //memoryRange.size = uniformBuffers.dynamic.size;
    //vkFlushMappedMemoryRanges(device, 1, &memoryRange);
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //////////////////////////////// SCENE SETUP ////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  void Renderer::setCamera(util::Ref<Camera> camera) {
    m_camera = camera;
  }

  void Renderer::setDynamicLights(LightContainer const& lights) {
    m_lights = lights;
    m_lightsUBO.reset();

    VkDeviceSize lightUniformSize = sizeof(LightUBO);
    m_lightsUBO                   = util::make_ptr<Buffer>(Buffer::CreateUniform(*m_device,
                                                                                 lightUniformSize * lights.size()));

    m_finalStep->updateDescriptorSets(m_gbuffer->getImageViews(), *m_cameraUBO, *m_lightsUBO, m_sampler);
    m_finalStep->writeCmdBuff(m_swapchain->getFrameBuffers());
  }

  void Renderer::setScene(std::vector<util::Ref<Object>> const& objects) {
    m_objList = objects;

    if (m_modelUBOdata)
      alignedFree(m_modelUBOdata);

    prepareDynamicUniformBuffers();

    m_geometryStep->updateDescriptorSets(*m_modelUBO, *m_cameraUBO);
    m_geometryStep->writeCmdBuff(*m_gbuffer, m_objList, m_modelUBOdynamicAlignment);
  }

  void Renderer::prepareDynamicUniformBuffers() {
    const size_t minUboAlignment = m_device->getOwningPhysical().getLimits().minUniformBufferOffsetAlignment;

    m_modelUBOdynamicAlignment = sizeof(ObjectUniform);
    if (minUboAlignment > 0) {
      m_modelUBOdynamicAlignment = (m_modelUBOdynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }

    size_t modelUBOsize = m_modelUBOdynamicAlignment * m_objList.size();

    m_modelUBOdata = static_cast<ObjectUniform*>(alignedAlloc(modelUBOsize, m_modelUBOdynamicAlignment));
    assert(m_modelUBOdata);

    size_t numImages = m_swapchain->getNumImages();

    m_modelUBO.reset();
    m_modelUBO = util::make_ptr<Buffer>(Buffer::CreateUniform(*m_device, modelUBOsize));
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////// SETUP HELPERS ///////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //// GENERAL SETUP //////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //// INSTANCE & HELPER SETUP
  /////////////////////////////////////////////////////////////////////////////

  void Renderer::setupInstance() {
    m_control = new VulkanControl("GPROJ", VK_MAKE_VERSION(1, 1, 100), 1);

    std::vector<const char*> instanceExtensions;
    std::vector<const char*> instanceLayers;

    instanceExtensions = GLFWControl::GetRequiredVKExtensions();

#ifdef _DEBUG
    instanceLayers.push_back(VulkanControl::LAYER_STANDARD_VALIDATION);
    //instanceLayers.push_back(VulkanControl::LAYER_RENDERDOC_CAPTURE);

    instanceExtensions.push_back(VulkanControl::EXT_DEBUG_REPORT);
    instanceExtensions.push_back(VulkanControl::EXT_DEBUG_UTILS);
#endif

    m_control->initInstance(instanceExtensions, instanceLayers);
    m_control->registerPhysicalDevices();
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

  void Renderer::setupHelpers() {
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
      createFn(*m_control, &createInfo, nullptr, &m_debugMessenger);
    }
    else
      throw std::runtime_error("Debug Extension Not Present");
#endif
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //// DEVICE SETUP
  /////////////////////////////////////////////////////////////////////////////

  void Renderer::setupDevice() {
    auto& physical = m_control->getBestPhysical();

    auto devExt   = physical.getAvailableExtensions();
    auto devLayer = physical.getAvailableLayers();

    std::vector<const char*> deviceExtensions;
    std::vector<const char*> deviceLayers;

    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
    //deviceExtensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);

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
    queueList.push_back(std::make_pair(graphicsFamily, std::vector<float>({1})));

    if (graphicsFamily != transferFamily)
      queueList.push_back(std::make_pair(transferFamily, std::vector<float>({1})));

    m_device = new LogicalDevice(physical, deviceLayers, deviceExtensions, queueList, features, features, false);

    m_graphicsQueue = new util::Ref<Queue>(m_device->getBestQueue(VK_QUEUE_GRAPHICS_BIT));
    if (!m_graphicsQueue->get().isValid())
      throw std::runtime_error("no graphics queue available");

    m_transferQueue = new util::Ref<Queue>(m_device->getBestQueue(VK_QUEUE_TRANSFER_BIT));
    assert(m_transferQueue && m_transferQueue->get().isValid()); // graphics queues implicitly allow transfer
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //// SURFACE & SWAPCHAIN SETUP
  /////////////////////////////////////////////////////////////////////////////

  void Renderer::setupSurface() {
    m_surface = std::make_unique<Surface>(*m_window, m_device->getOwningPhysical());

    m_presentQueue = new util::Ref<Queue>(m_device->getPresentableQueue(*m_surface));
    if (!m_presentQueue->get().isValid())
      throw std::runtime_error("no present queue available");
  }

  void Renderer::setupSwapChain() {
    m_swapchain = std::make_unique<Swapchain>(*m_device, *m_surface, *m_presentQueue);
  }

  void Renderer::setupCommandPools() {
    m_commandPool     = util::make_ptr<CommandPool>(*m_device, m_graphicsQueue->get().getFamily());
    m_transferCmdPool = util::make_ptr<CommandPool>(*m_device, m_transferQueue->get().getFamily());
  }


  void Renderer::setupSamplers() {
    VkSamplerCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      nullptr,
      0,
      VK_FILTER_NEAREST,
      VK_FILTER_NEAREST,
      VK_SAMPLER_MIPMAP_MODE_LINEAR,
      VK_SAMPLER_ADDRESS_MODE_REPEAT,
      VK_SAMPLER_ADDRESS_MODE_REPEAT,
      VK_SAMPLER_ADDRESS_MODE_REPEAT,
      0.f,
      false,
      1.f, // can be 16x for filtering if feature support
      false,
      VK_COMPARE_OP_LESS,
      0.f,
      1.f,
      VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
      false
    };

    if (vkCreateSampler(*m_device, &createInfo, nullptr, &m_sampler) != VK_SUCCESS || !m_sampler)
      throw std::runtime_error("Could not create sampler");
  }

  void Renderer::setupUniformBuffers() {
    VkDeviceSize cameraUniformSize = sizeof(CameraUniform);

    m_cameraUBO = util::make_ptr<Buffer>(Buffer::CreateUniform(*m_device, cameraUniformSize));
  }

  void Renderer::setupFrameBufferImages() {
    VkExtent3D gbuffExtent = { m_swapchain->getImageSize().width, m_swapchain->getImageSize().height, 1 };

    m_gbuffer = util::make_ptr<Framebuffer>(*m_device, gbuffExtent);

    for (uint32_t i = 0; i < RenderStep::NUM_EXPECTED_GBUFFER_IMAGES; ++i)
      m_gbuffer->addImage(VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        gbuffExtent,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        1,
        1,
        false,
        false,
        false,
        false);

    m_gbuffer->addImage(VK_IMAGE_ASPECT_DEPTH_BIT,
      VK_IMAGE_TYPE_2D,
      VK_IMAGE_VIEW_TYPE_2D,
      VK_FORMAT_D32_SFLOAT,
      gbuffExtent,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      1,
      1,
      false,
      false,
      false,
      false);
  }

  void Renderer::setupRenderSteps() {
    m_geometryStep = util::make_ptr<GeometryStep>(*m_device, *m_commandPool);

    m_geometryStep->setupShaders();
    m_geometryStep->setupDescriptors();
    m_geometryStep->setupRenderPass({ m_gbuffer->getImages().begin(), m_gbuffer->getImages().end() });
    m_geometryStep->setupPipelineLayout();
    m_geometryStep->setupPipeline(m_swapchain->getImageSize());

    m_finalStep = util::make_ptr<FinalStep>(*m_device, *m_commandPool, m_swapchain->getNumImages());

    m_finalStep->setupShaders();
    m_finalStep->setupDescriptors();
    m_finalStep->setupRenderPass({m_swapchain->getImages()[0]});
    m_finalStep->setupPipelineLayout();
    m_finalStep->setupPipeline(m_swapchain->getImageSize());
  }
  
  void Renderer::setupFrameBuffers() {
    m_gbuffer->finalize(m_geometryStep->getRenderPass());

    // this is preferred when we are only using a color attachment on the output
    // framebuffers, e.g., when you are just rendering a FSQ to do the final lighting pass
    // and the backbuffer is just the final location in the rendering chain.
    //m_swapchain->createFramebuffers(*m_renderPass);

    // this method is used when you want additional attachments on each framebuffer.
    std::vector<Framebuffer> framebuffers;
    framebuffers.reserve(m_swapchain->getNumImages());

    for (size_t i = 0; i < m_swapchain->getNumImages(); ++i) {
      framebuffers.emplace_back(*m_device,
        m_finalStep->getRenderPass(),
        std::vector<VkImageView>{m_swapchain->getViews()[i]},
        VkExtent3D{ m_surface->getWidth(), m_surface->getHeight(), 1 });
    }

    m_swapchain->setFramebuffers(std::move(framebuffers));
  }

  void Renderer::transitionRenderImages() const {
    CommandBuffer& transBuff = m_transferCmdPool->allocateCommandBuffer();
    transBuff.start(true);

    for (auto& image : m_swapchain->getImages()) {
      transitionImageLayout(transBuff, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }

    transBuff.end();
    m_transferQueue->get().submitOne(transBuff);
    m_transferQueue->get().waitIdle();

    m_transferCmdPool->freeCommandBuffer(transBuff);

    CommandBuffer& graphicsBuff = m_commandPool->allocateCommandBuffer();
    graphicsBuff.start(true);

    const Image& depthImage = dynamic_cast<const Image&>(m_gbuffer->getImages().back());
    transitionImageLayout(graphicsBuff, depthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    graphicsBuff.end();
    m_graphicsQueue->get().submitOne(graphicsBuff);
    m_graphicsQueue->get().waitIdle();

    m_commandPool->freeCommandBuffer(graphicsBuff);
  }
}
