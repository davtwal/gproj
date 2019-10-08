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

#include <array>
#include <cassert>
#include <algorithm>
#include <stdlib.h>
#include "Light.h"

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
    setupCommandBuffers();
  }

  void Renderer::initSpecific() {
    VkSemaphoreCreateInfo deferredSemaphoreCreateInfo = {
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      nullptr,
      0
    };

    if (vkCreateSemaphore(*m_device, &deferredSemaphoreCreateInfo, nullptr, &m_deferredSemaphore) != VK_SUCCESS)
      throw std::runtime_error("Could not create deferred rendering semaphore");

    setupDepthTestResources();
    setupGBufferImages();
    setupRenderSteps();
    setupDescriptors();
    setupUniformBuffers();
    setupSamplers();
    setupShaders();

    setupPipeline();
    setupGBufferFrameBuffer();
    setupSwapChainFrameBuffers();

    transitionRenderImages();
    initManagers();
  }

  void Renderer::shutdown() {
    vkDeviceWaitIdle(*m_device);

    vkDestroySemaphore(*m_device, m_deferredSemaphore, nullptr);

    shutdownManagers();
    m_objList.clear();

    /*if (m_modelUBOdata)
      alignedFree(m_modelUBOdata);

    m_modelUBOdata = nullptr;*/

    vkDestroySampler(*m_device, m_sampler, nullptr);

    vkDestroyPipeline(*m_device, m_deferredPipeline, nullptr);
    vkDestroyPipeline(*m_device, m_finalPipeline, nullptr);
    vkDestroyPipelineLayout(*m_device, m_deferredPipeLayout, nullptr);
    vkDestroyPipelineLayout(*m_device, m_finalPipeLayout, nullptr);

    m_deferredPass.reset();
    m_finalPass.reset();

    vkDestroyDescriptorPool(*m_device, m_finalDescPool, nullptr);
    vkDestroyDescriptorPool(*m_device, m_deferredDescPool, nullptr);
    vkDestroyDescriptorSetLayout(*m_device, m_finalDescSetLayout, nullptr);;
    vkDestroyDescriptorSetLayout(*m_device, m_deferredDescSetLayout, nullptr);

    m_gbufferViews.clear();
    m_gbufferImages.clear();
    m_depthStencilView.reset();
    m_depthStencilImage.reset();
    m_gbuffer.reset();

    //m_modelUBO.reset();
    m_lightsUBO.reset();
    m_cameraUBO.reset();
    m_commandBuffers.clear();

    m_fsqVertShader.reset();
    m_fsqFragShader.reset();
    m_triangleFragShader.reset();
    m_triangleVertShader.reset();

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

    GLFWControl::Poll();

    uint32_t     nextImageIndex = m_swapchain->getNextImageIndex();
    Image const& nextImage      = m_swapchain->getNextImage();

    auto&           graphicsQueue   = m_graphicsQueue->get();
    VkCommandBuffer deferredCmdBuff = m_deferredCmdBuff->get();
    VkCommandBuffer presentCmdBuff  = m_commandBuffers[nextImageIndex].get();

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

    /*assert(m_modelUBOdata);

    for (uint32_t i = 0; i < m_objList.size(); ++i) {
      m_modelUBOdata[i].model = m_objList[i].get().getTransform();
    }

    data = m_modelUBO->map();
    memcpy(data, m_modelUBOdata, m_modelUBO->getSize());
    m_modelUBO->unMap();*/

    data = m_lightsUBO->map();
    LightUBO* lightUBOdata = reinterpret_cast<LightUBO*>(data);
    for (size_t i = 0; i < m_lights.size(); ++i)
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
    m_lightsUBO = util::make_ptr<Buffer>(Buffer::CreateUniform(*m_device, lightUniformSize * lights.size()));
  }

  void Renderer::setScene(std::vector<util::Ref<Object>> const& objects) {
    m_objList = objects;

    //if (m_modelUBOdata)
    //  alignedFree(m_modelUBOdata);

    prepareDynamicUniformBuffers();
    updateDescriptorSets();
    writeCommandBuffers();
  }

  void Renderer::prepareDynamicUniformBuffers() {
    /*const size_t minUboAlignment = m_device->getOwningPhysical().getLimits().minUniformBufferOffsetAlignment;

    m_modelUBOdynamicAlignment = sizeof(ObjectUniform);
    if (minUboAlignment > 0) {
      m_modelUBOdynamicAlignment = (m_modelUBOdynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }

    size_t modelUBOsize = m_modelUBOdynamicAlignment * m_objList.size();

    m_modelUBOdata = static_cast<ObjectUniform*>(alignedAlloc(modelUBOsize, m_modelUBOdynamicAlignment));
    assert(m_modelUBOdata);

    size_t numImages = m_swapchain->getNumImages();

    m_modelUBO.reset();
    m_modelUBO = util::make_ptr<Buffer>(Buffer::CreateUniform(*m_device, modelUBOsize));*/
  }

  void Renderer::updateDescriptorSets() {
    // Descriptor sets are automatically freed once the pool is freed.
    // They can be individually freed if the pool was created with
    // VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT sets
    /*VkDescriptorBufferInfo modelUBOinfo = m_modelUBO->getDescriptorInfo();
    modelUBOinfo.range                  = sizeof(ObjectUniform);*/


    std::vector<VkWriteDescriptorSet> descriptorWrites;
    descriptorWrites.reserve(2 + m_finalDescSets.size() * (m_gbufferViews.size() + 2));
    descriptorWrites.push_back({
                                 VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                 nullptr,
                                 m_deferredDescSet,
                                 0,
                                 0,
                                 1,
                                 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                 nullptr,
                                 &m_cameraUBO->getDescriptorInfo(),
                                 nullptr
                               });
    /*descriptorWrites.push_back({
                                 VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                 nullptr,
                                 m_deferredDescSet,
                                 1,
                                 0,
                                 1,
                                 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                                 nullptr,
                                 &modelUBOinfo,
                                 nullptr
                               });*/

    std::vector<VkDescriptorImageInfo> imageInfos;
    imageInfos.reserve(m_gbufferViews.size());
    for (uint32_t i = 0; i < m_gbufferViews.size(); ++i) {
      imageInfos.push_back({m_sampler, m_gbufferViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
    }

    for (auto& set : m_finalDescSets) {
      descriptorWrites.push_back({
                                   VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                   nullptr,
                                   set,
                                   0,
                                   0,
                                   1,
                                   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                   nullptr,
                                   &m_cameraUBO->getDescriptorInfo(),
                                   nullptr
                                 });

      for (uint32_t j = 0; j < m_gbufferViews.size(); ++j) {
        descriptorWrites.push_back({
                                     VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                     nullptr,
                                     set,
                                     j + 1,
                                     0,
                                     1,
                                     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                     &imageInfos[j],
                                     nullptr,
                                     nullptr
                                   });
      }

      descriptorWrites.push_back({
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        nullptr,
        set,
        4,
        0,
        1,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        nullptr,
        &m_lightsUBO->getDescriptorInfo(),
        nullptr
        });
    }

    vkUpdateDescriptorSets(*m_device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(),
                           0,
                           nullptr);
  }

  void Renderer::writeCommandBuffers() {

    const auto count = m_swapchain->getNumImages();

    // 1: deferred pass
    if (!m_objList.empty()) {
      std::array<VkClearValue, 4> clearValues{};
      clearValues[0].color        = {{0, 0, 0, 0}};
      clearValues[1].color        = {{0}};
      clearValues[2].color        = {{0}};
      clearValues[3].depthStencil = {1.f, 0};

      auto& commandBuff = m_deferredCmdBuff->get();
      commandBuff.start(false);
      VkRenderPassBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        nullptr,
        *m_deferredPass,
        *m_gbuffer,
        {{0, 0}, m_swapchain->getImageSize()},
        static_cast<uint32_t>(clearValues.size()),
        clearValues.data()
      };


      vkCmdBindPipeline(commandBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, m_deferredPipeline);
      vkCmdBeginRenderPass(commandBuff, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

      Mesh* curMesh = nullptr;

      vkCmdBindDescriptorSets(commandBuff,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_deferredPipeLayout,
        0,
        1,
        &m_deferredDescSet,
        0,
        nullptr);

      for (uint32_t j = 0; j < m_objList.size(); ++j) {
        auto& obj = m_objList.at(j);

        vkCmdPushConstants(commandBuff, m_deferredPipeLayout,
          VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ObjectTransfPushConst), &obj.get().getTransform());

        if (!curMesh || !(obj.get().m_mesh.get() == *curMesh)) {
          curMesh = &obj.get().m_mesh.get();

          const VkBuffer&    buff   = curMesh->getVertexBuffer();
          const VkDeviceSize offset = 0;
          vkCmdBindVertexBuffers(commandBuff, 0, 1, &buff, &offset);
          vkCmdBindIndexBuffer(commandBuff, curMesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
        }

        ObjectMtlPushConst mtlPush = {
          {1, 1, 1},
          50
        };

        //vkCmdPushConstants(commandBuff, m_deferredPipeLayout,
        //  VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ObjectTransfPushConst), sizeof(ObjectMtlPushConst), &mtlPush);

        vkCmdDrawIndexed(commandBuff, curMesh->getNumIndices(), 1, 0, 0, 0);
      }

      vkCmdEndRenderPass(commandBuff);
      commandBuff.end();
    }

    // 2: final pass

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0, 0, 0, 0}};
    auto& framebuffers   = m_swapchain->getFrameBuffers();
    for (size_t i = 0; i < count; ++i) {
      auto& commandBuffer = m_commandBuffers[i].get();
  
      //auto pfnpush = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(*m_device, "vkCmdPushDescriptorSetKHR");

      VkRenderPassBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        nullptr,
        *m_finalPass,
        framebuffers[i],
        {{0, 0}, m_swapchain->getImageSize()},
        static_cast<uint32_t>(clearValues.size()),
        clearValues.data()
      };

      commandBuffer.start(false);
      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_finalPipeline);
      vkCmdBindDescriptorSets(commandBuffer,
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              m_finalPipeLayout,
                              0,
                              1,
                              &m_finalDescSets[i],
                              0,
                              nullptr);
      vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdDraw(commandBuffer, 4, 1, 0, 0);
      vkCmdEndRenderPass(commandBuffer);
      commandBuffer.end();
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////// MANAGERS //////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  void Renderer::initManagers() {
    m_meshManager.loadBasicMeshes();
    m_meshManager.uploadMeshes(*this);
  }

  void Renderer::shutdownManagers() {
    m_meshManager.clear();
  }

  MeshManager& Renderer::getMeshManager() {
    return m_meshManager;
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
    //deviceExtensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
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

  static void transitionImageLayout(CommandBuffer& cmdBuff,
                                    Image&         image,
                                    VkImageLayout  oldLayout,
                                    VkImageLayout  newLayout) {
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

      if (util::IsFormatStencil(image.getFormat())) {
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

      sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    ) {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
      destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

      sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
      //deelse if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
      //de  barrier.srcAccessMask = 0;
      //de  barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      //de
      //de}
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

  void Renderer::transitionRenderImages() const {
    CommandBuffer& transBuff = m_transferCmdPool->allocateCommandBuffer();
    transBuff.start(true);

    for (auto& image : m_swapchain->getImages()) {
      transitionImageLayout(transBuff, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }

    //for (auto& image : m_gbufferImages) {
    //  transitionImageLayout(transBuff, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    //}

    transBuff.end();

    m_transferQueue->get().submitOne(transBuff);
    m_transferQueue->get().waitIdle();

    m_transferCmdPool->freeCommandBuffer(transBuff);
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //// COMMAND POOLS & BUFFER SETUP
  /////////////////////////////////////////////////////////////////////////////

  void Renderer::setupCommandPools() {
    m_commandPool     = util::make_ptr<CommandPool>(*m_device, m_graphicsQueue->get().getFamily());
    m_transferCmdPool = util::make_ptr<CommandPool>(*m_device, m_transferQueue->get().getFamily());
  }

  void Renderer::setupCommandBuffers() {
    size_t imageCount = m_swapchain->getNumImages();
    m_commandBuffers.reserve(imageCount);
    for (size_t i = 0; i < imageCount; ++i) {
      m_commandBuffers.emplace_back(m_commandPool->allocateCommandBuffer());
    }

    m_deferredCmdBuff = util::make_ptr<util::Ref<CommandBuffer>>(m_commandPool->allocateCommandBuffer());
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //// SPECIFIC SETUP /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //// DEPTH TESTING RESOURCES
  /////////////////////////////////////////////////////////////////////////////

  void Renderer::setupDepthTestResources() {
    m_depthStencilImage = util::make_ptr<DependentImage>(*m_device);
    m_depthStencilImage->initImage(VK_IMAGE_TYPE_2D,
                                   VK_IMAGE_VIEW_TYPE_2D,
                                   VK_FORMAT_D32_SFLOAT,
                                   {m_swapchain->getImageSize().width, m_swapchain->getImageSize().height, 1},
                                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                   1,
                                   1,
                                   false,
                                   false,
                                   false,
                                   false);

    // TODO on MemoryAllocator : remove
    MemoryAllocator allocator(m_device->getOwningPhysical());
    m_depthStencilImage->back(allocator, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    CommandBuffer& transBuff = m_commandPool->allocateCommandBuffer();
    transBuff.start(true);
    transitionImageLayout(transBuff,
                          *m_depthStencilImage,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    transBuff.end();
    m_graphicsQueue->get().submitOne(transBuff);
    m_graphicsQueue->get().waitIdle();

    m_commandPool->freeCommandBuffer(transBuff);
    m_depthStencilView = std::make_shared<ImageView>(m_depthStencilImage->createView(VK_IMAGE_ASPECT_DEPTH_BIT));
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //// RENDER STEPS & BACK-BUFFERS SETUP
  /////////////////////////////////////////////////////////////////////////////

  void Renderer::setupRenderSteps() {
    m_deferredPass = std::make_unique<RenderPass>(*m_device);
    m_deferredPass->reserveAttachments(1);
    m_deferredPass->reserveSubpasses(1);
    m_deferredPass->reserveAttachmentRefs(1);
    m_deferredPass->reserveSubpassDependencies(1);

    for (uint32_t i = 0; i < m_gbufferImages.size(); ++i) {
      m_deferredPass->addAttachment(m_gbufferImages[i].getAttachmentDesc(VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                         VK_ATTACHMENT_STORE_OP_STORE,
                                                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
      m_deferredPass->addAttachmentRef(i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, RenderPass::arfColor);
    }

    m_deferredPass->addAttachment(m_depthStencilImage->getAttachmentDesc(VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                         VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                                         VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));
    m_deferredPass->addAttachmentRef(m_gbufferImages.size(),
                                     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                     RenderPass::arfDepthStencil);
    m_deferredPass->finishSubpass();

    m_deferredPass->addSubpassDependency({
                                           VK_SUBPASS_EXTERNAL,
                                           0,
                                           VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                           VK_ACCESS_MEMORY_READ_BIT,
                                           VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                           VK_DEPENDENCY_BY_REGION_BIT
                                         });

    m_deferredPass->addSubpassDependency({
                                           0,
                                           VK_SUBPASS_EXTERNAL,
                                           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                           VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                           VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                           VK_ACCESS_MEMORY_READ_BIT,
                                           VK_DEPENDENCY_BY_REGION_BIT
                                         });

    m_deferredPass->finishRenderPass();

    // final pass (fsq)
    m_finalPass = util::make_ptr<RenderPass>(*m_device);
    m_finalPass->addAttachment(m_swapchain->getImageAttachmentDesc());
    m_finalPass->addAttachmentRef(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, RenderPass::arfColor);

    /*for (uint32_t i = 0; i < m_gbufferImages.size(); ++i) {
      m_finalPass->addAttachment(m_gbufferImages[i].getAttachmentDesc(VK_ATTACHMENT_LOAD_OP_LOAD,
                                                                      VK_ATTACHMENT_STORE_OP_STORE,
                                                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
      m_finalPass->addInputRef(i + 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }*/

    m_finalPass->finishSubpass();

    m_finalPass->addSubpassDependency({
                                        VK_SUBPASS_EXTERNAL,
                                        0,
                                        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                        VK_ACCESS_MEMORY_WRITE_BIT,
                                        VK_ACCESS_SHADER_READ_BIT,
                                        VK_DEPENDENCY_BY_REGION_BIT
                                      });

    m_finalPass->addSubpassDependency({
                                        0,
                                        VK_SUBPASS_EXTERNAL,
                                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                        VK_ACCESS_MEMORY_READ_BIT,
                                        VK_DEPENDENCY_BY_REGION_BIT
                                      });

    m_finalPass->finishRenderPass();
  }

  void Renderer::setupSwapChainFrameBuffers() const {
    // this is preferred when we are only using a color attachment on the output
    // framebuffers, e.g., when you are just rendering a FSQ to do the final lighting pass
    // and the backbuffer is just the final location in the rendering chain.
    //m_swapchain->createFramebuffers(*m_renderPass);

    // this method is used when you want additional attachments on each framebuffer.
    std::vector<Framebuffer> framebuffers;
    framebuffers.reserve(m_swapchain->getNumImages());

    for (size_t i = 0; i < m_swapchain->getNumImages(); ++i) {
      std::vector<VkImageView> views(1);
      views.at(0) = m_swapchain->getViews()[i];

      framebuffers.emplace_back(*m_device,
                                *m_finalPass,
                                views,
                                VkExtent3D{m_surface->getWidth(), m_surface->getHeight(), 1});
    }

    m_swapchain->setFramebuffers(std::move(framebuffers));
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //// GBUFFER & SAMPLERS SETUP
  /////////////////////////////////////////////////////////////////////////////

  void Renderer::setupGBufferImages() {
    static constexpr unsigned GBUFFER_IMAGE_COUNT = 3; // pos, normal, albedo

    m_gbufferImages.reserve(GBUFFER_IMAGE_COUNT);
    m_gbufferViews.reserve(GBUFFER_IMAGE_COUNT);

    VkExtent3D gbuffExtent = {m_swapchain->getImageSize().width, m_swapchain->getImageSize().height, 1};
    for (unsigned i = 0; i < GBUFFER_IMAGE_COUNT; ++i) {
      m_gbufferImages.emplace_back(*m_device).initImage(VK_IMAGE_TYPE_2D,
                                                        VK_IMAGE_VIEW_TYPE_2D,
                                                        VK_FORMAT_R32G32B32A32_SFLOAT,
                                                        gbuffExtent,
                                                        VK_IMAGE_USAGE_SAMPLED_BIT |
                                                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                                        1,
                                                        1,
                                                        false,
                                                        false,
                                                        false,
                                                        false);
      MemoryAllocator allocator(m_device->getOwningPhysical());
      m_gbufferImages.back().back(allocator, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      m_gbufferViews.emplace_back(m_gbufferImages.back().createView());
    }
  }

  void Renderer::setupGBufferFrameBuffer() {
    std::vector<VkImageView> gbufferViews;
    gbufferViews.reserve(m_gbufferViews.size() + 1);

    for (auto& view : m_gbufferViews) {
      gbufferViews.emplace_back(view);
    }

    gbufferViews.emplace_back(*m_depthStencilView);

    VkExtent3D gbuffExtent = {m_swapchain->getImageSize().width, m_swapchain->getImageSize().height, 1};
    m_gbuffer              = util::make_ptr<Framebuffer>(*m_device, *m_deferredPass, gbufferViews, gbuffExtent);
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

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //// SHADER LOADING, UBO & DESCRIPTOR SET CREATION
  /////////////////////////////////////////////////////////////////////////////

  void Renderer::setupShaders() {
    m_triangleVertShader = util::make_ptr<Shader<ShaderStage::Vertex>>(ShaderModule::Load(*m_device,
                                                                                          "fromBuffer_transform_vert.spv"));
    m_triangleFragShader = util::make_ptr<Shader<ShaderStage::Fragment>>(ShaderModule::Load(*m_device,
                                                                                            "triangle_frag.spv"));

    m_fsqVertShader = util::make_ptr<Shader<ShaderStage::Vertex>>(ShaderModule::Load(*m_device, "fsq_vert.spv"));
    m_fsqFragShader = util::make_ptr<Shader<ShaderStage::Fragment>>(ShaderModule::Load(*m_device, "fsq_frag.spv"));
  }

  void Renderer::setupUniformBuffers() {
    VkDeviceSize cameraUniformSize = sizeof(CameraUniform);

    m_cameraUBO                    = util::make_ptr<Buffer>(Buffer::CreateUniform(*m_device, cameraUniformSize));
  }

  void Renderer::setupDescriptors() {
    // MVP matrices for object transformations
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {
      { // camera
        0,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        1,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        nullptr
      },
      //{
      //  1,
      //  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
      //  1,
      //  VK_SHADER_STAGE_VERTEX_BIT,
      //  nullptr
      //}
    };

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      nullptr,
      0, // VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR for push descriptor
      static_cast<uint32_t>(layoutBindings.size()),
      layoutBindings.data()
    };

    if (vkCreateDescriptorSetLayout(*m_device, &layoutCreateInfo, nullptr, &m_deferredDescSetLayout) != VK_SUCCESS || !
        m_deferredDescSetLayout)
      throw std::runtime_error("Could not create descriptor set layout");

    ///////////////////////////////////////////////////////
    // POOL AND SETS

    // create uniform buffers - one for each swapchain image
    std::vector<VkDescriptorPoolSize> poolSizes = {
      { // camera
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        1
      },
      //{
      //  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
      //  1
      //}
    };

    VkDescriptorPoolCreateInfo poolCreateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      nullptr,
      0,
      1,
      static_cast<uint32_t>(poolSizes.size()),
      poolSizes.data()
    };

    if (vkCreateDescriptorPool(*m_device, &poolCreateInfo, nullptr, &m_deferredDescPool) != VK_SUCCESS || !
        m_deferredDescPool)
      throw std::runtime_error("Could not create descriptor pool");

    //////////////////
    // SETS

    VkDescriptorSetAllocateInfo descSetAllocInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      nullptr,
      m_deferredDescPool,
      1,
      &m_deferredDescSetLayout
    };

    if (vkAllocateDescriptorSets(*m_device, &descSetAllocInfo, &m_deferredDescSet) != VK_SUCCESS)
      throw std::runtime_error("Could not allocate descriptor sets");

    // SAMPLERS
    std::vector<VkDescriptorSetLayoutBinding> finalBindings;
    finalBindings.resize(m_gbufferImages.size() + 2); // one sampler per gbuffer image + one view eye/view dir UBO + one for light UBO
    finalBindings.front() = {
      0,
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      1,
      VK_SHADER_STAGE_FRAGMENT_BIT,
      nullptr
    };

    for (uint32_t i = 1; i < finalBindings.size() - 1; ++i) {
      finalBindings[i] = {
        i,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        1,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        nullptr
      };
    }

    finalBindings.back() = {
      4,
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      1,
      VK_SHADER_STAGE_FRAGMENT_BIT,
      nullptr
    };

    VkDescriptorSetLayoutCreateInfo finalLayoutCreate = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      nullptr,
      0,
      static_cast<uint32_t>(finalBindings.size()),
      finalBindings.data()
    };

    if (vkCreateDescriptorSetLayout(*m_device, &finalLayoutCreate, nullptr, &m_finalDescSetLayout) != VK_SUCCESS)
      throw std::runtime_error("could not create final descriptor set");

    uint32_t                          numImages      = static_cast<uint32_t>(m_swapchain->getNumImages());
    std::vector<VkDescriptorPoolSize> finalPoolSizes = {
      {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        numImages * 2
      },
      {
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        numImages * static_cast<uint32_t>(m_gbufferViews.size())
      }
    };

    VkDescriptorPoolCreateInfo finalPoolInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      nullptr,
      0,
      numImages,
      static_cast<uint32_t>(finalPoolSizes.size()),
      finalPoolSizes.data()
    };

    if (vkCreateDescriptorPool(*m_device, &finalPoolInfo, nullptr, &m_finalDescPool) != VK_SUCCESS)
      throw std::runtime_error("could not create final descrition pool");

    std::vector<VkDescriptorSetLayout> finalLayouts = {numImages, m_finalDescSetLayout};
    descSetAllocInfo.pSetLayouts                    = finalLayouts.data();
    descSetAllocInfo.descriptorPool                 = m_finalDescPool;
    descSetAllocInfo.descriptorSetCount             = numImages;

    m_finalDescSets.resize(numImages);
    VkResult result = vkAllocateDescriptorSets(*m_device, &descSetAllocInfo, m_finalDescSets.data());
    switch (result) {
      case VK_ERROR_OUT_OF_HOST_MEMORY:
        throw std::runtime_error("could not allocate descriptor sets (final) (out of host memory)");

      case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        throw std::runtime_error("could not allocate descriptor sets (final) (out of device memory)");

      case VK_ERROR_FRAGMENTED_POOL:
        throw std::runtime_error("could not allocate descriptor sets (final) (fragmented pool)");

      case VK_ERROR_OUT_OF_POOL_MEMORY:
        throw std::runtime_error("could not allocate descriptor sets (final) (out of pool memory)");

      default:
        break;
    }

    // Descriptor sets are updated once the scene is set
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //// PIPELINES SETUP
  /////////////////////////////////////////////////////////////////////////////

  void Renderer::setupPipeline() {
    auto vertexAttributes = Vertex::GetBindingAttributes();
    auto vertexBindings   = Vertex::GetBindingDescriptions();
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
      static_cast<float>(m_surface->getHeight()),
      static_cast<float>(m_surface->getWidth()),
      -static_cast<float>(m_surface->getHeight()),
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
      VK_FRONT_FACE_COUNTER_CLOCKWISE,
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

    VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      nullptr,
      0,
      true,
      true,
      VK_COMPARE_OP_LESS,
      false,
      false,
      {},
      {},
      0.f,
      1.f
    };

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

    std::vector<VkPipelineColorBlendAttachmentState> deferredColorAttachmentInfos(m_gbufferImages.size());
    for (auto& info : deferredColorAttachmentInfos) {
      info = colorAttachmentInfo;
    }

    // I doubt this will ever change
    VkPipelineColorBlendStateCreateInfo colorBlendInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      nullptr,
      0,
      VK_FALSE, // Setting this to true changes color blend to color bitwise combination. Turns blending off for all attachments
      VK_LOGIC_OP_COPY,
      static_cast<uint32_t>(deferredColorAttachmentInfos.size()
      ), // this has to equal the number of color attachments on the subpass this pipeline is used
      deferredColorAttachmentInfos.data(),
      {0, 0, 0, 0}
    };

    // Compiling the shader stages together
    // TODO
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

    // object material/model transform push constants
    static_assert(sizeof(ObjectTransfPushConst) % 4 == 0);
    static_assert(sizeof(ObjectMtlPushConst) % 4 == 0);

    const std::array<VkPushConstantRange, 2> pipePushConsts = {{
      {
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(ObjectTransfPushConst)
      },
      {
        VK_SHADER_STAGE_FRAGMENT_BIT,
        sizeof(ObjectTransfPushConst),
        sizeof(ObjectMtlPushConst)
      }
    }};

    VkPipelineLayoutCreateInfo pipeLayoutInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      nullptr,
      0,
      1,
      &m_deferredDescSetLayout,
      pipePushConsts.size(),
      pipePushConsts.data()
    };
    if (!m_deferredDescSetLayout) {
      pipeLayoutInfo.pSetLayouts    = nullptr;
      pipeLayoutInfo.setLayoutCount = 0;
    }

    vkCreatePipelineLayout(*m_device, &pipeLayoutInfo, nullptr, &m_deferredPipeLayout);
    assert(m_deferredPipeLayout);

    // create final pass pipe layout
    pipeLayoutInfo.setLayoutCount = 1;
    pipeLayoutInfo.pSetLayouts    = &m_finalDescSetLayout;
    pipeLayoutInfo.pushConstantRangeCount = 0;
    pipeLayoutInfo.pPushConstantRanges = nullptr;
    vkCreatePipelineLayout(*m_device, &pipeLayoutInfo, nullptr, &m_finalPipeLayout);
    assert(m_finalPipeLayout);

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
      &depthStencilInfo, // no depth test for now (soon to change)
      &colorBlendInfo,
      nullptr, // no dynamic stages yet
      m_deferredPipeLayout,
      *m_deferredPass,
      0,
      nullptr, // You can create derivative pipelines, maybe this will be good later but imnotsure
      -1
    };

    vkCreateGraphicsPipelines(*m_device, nullptr, 1, &createInfo, nullptr, &m_deferredPipeline);
    assert(m_deferredPipeline);

    stageInfo[0] = m_fsqVertShader->getCreateInfo();
    stageInfo[1] = m_fsqFragShader->getCreateInfo();

    vertInputInfo.pVertexAttributeDescriptions    = nullptr;
    vertInputInfo.pVertexAttributeDescriptions    = nullptr;
    vertInputInfo.vertexAttributeDescriptionCount = 0;
    vertInputInfo.vertexBindingDescriptionCount   = 0;

    depthStencilInfo.depthTestEnable = false;

    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments    = &colorAttachmentInfo;

    createInfo.layout     = m_finalPipeLayout;
    createInfo.renderPass = *m_finalPass;

    VkViewport finalViewport     = {0, 0, m_swapchain->getImageSize().width, m_swapchain->getImageSize().height, 0, 1};
    viewportStateInfo.pViewports = &finalViewport;

    rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

    vkCreateGraphicsPipelines(*m_device, nullptr, 1, &createInfo, nullptr, &m_finalPipeline);
  }
}
