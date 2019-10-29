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
#include "Light.h"
#include "RenderSteps.h"

#include "ImGui.h"

#include <array>
#include <cassert>
#include <algorithm>


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

static void transitionImageLayout(dw::CommandBuffer& cmdBuff,
                                  dw::Image const&   image,
                                  VkImageLayout      oldLayout,
                                  VkImageLayout      newLayout) {
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
  Renderer::ShadowMappedLight::ShadowMappedLight(ShadowedLight const& light)
    : m_light(light) {
  }


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
    VkSemaphoreCreateInfo semaphoreCreateInfo = {
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      nullptr,
      0
    };

    // TODO:: make better
    if (vkCreateSemaphore(*m_device, &semaphoreCreateInfo, nullptr, &m_deferredSemaphore) != VK_SUCCESS)
      throw std::runtime_error("Could not create deferred rendering semaphore");

    if (vkCreateSemaphore(*m_device, &semaphoreCreateInfo, nullptr, &m_shadowSemaphore) != VK_SUCCESS)
      throw std::runtime_error("Could not create shadow map rendering semaphore");

    if (vkCreateSemaphore(*m_device, &semaphoreCreateInfo, nullptr, &m_blurSemaphore) != VK_SUCCESS)
      throw std::runtime_error("Could not create shadow map rendering semaphore");

    if (vkCreateSemaphore(*m_device, &semaphoreCreateInfo, nullptr, &m_globalLightSemaphore) != VK_SUCCESS)
      throw std::runtime_error("Could not create global lighting rendering semaphore");

    if (vkCreateSemaphore(*m_device, &semaphoreCreateInfo, nullptr, &m_localLitSemaphore) != VK_SUCCESS)
      throw std::runtime_error("Could not create local lighting semaphore");

    setupSamplers();
    setupUniformBuffers();
    setupFrameBufferImages();
    setupRenderSteps();
    setupFrameBuffers();
    transitionRenderImages();

#ifdef DW_USE_IMGUI
    setupImGui();
#endif
  }

  void Renderer::shutdown() {
    vkDeviceWaitIdle(*m_device);

#ifdef DW_USE_IMGUI
    shutdownImGui();
#endif

    vkDestroySemaphore(*m_device, m_deferredSemaphore, nullptr);
    vkDestroySemaphore(*m_device, m_shadowSemaphore, nullptr);
    vkDestroySemaphore(*m_device, m_blurSemaphore, nullptr);
    vkDestroySemaphore(*m_device, m_globalLightSemaphore, nullptr);
    vkDestroySemaphore(*m_device, m_localLitSemaphore, nullptr);
    m_deferredSemaphore     = nullptr;
    m_shadowSemaphore       = nullptr;
    m_blurSemaphore         = nullptr;
    m_globalLightSemaphore  = nullptr;
    m_localLitSemaphore     = nullptr;

    m_globalLights.clear();
    m_objList.clear();

    if (m_modelUBOdata)
      alignedFree(m_modelUBOdata);

    m_modelUBOdata = nullptr;

    vkDestroySampler(*m_device, m_sampler, nullptr);

    m_gbuffer.reset();
    m_globalLitFrameBuffer.reset();
    m_blurIntermediateView.reset();
    m_blurIntermediate.reset();

    m_globalLightsUBO.reset();
    m_localLightsUBO.reset();
    m_modelUBO.reset();
    m_cameraUBO.reset();
    m_shaderControlBuffer.reset();

    m_shaderControl = nullptr;

    m_geometryStep.reset();
    m_shadowMapStep.reset();
    m_blurStep.reset();
    m_globalLightStep.reset();
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

  // IMGUI

#ifdef DW_USE_IMGUI
  void Renderer::setupImGui() {
    const auto checkResultFn = [](VkResult err) {
      if (err != VK_SUCCESS) {
        Trace::Error << "ImGui VK Error: " << err << Trace::Stop;
      }
    };

    VkDescriptorPoolSize poolSize = {
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      1
    };
    VkDescriptorPoolCreateInfo poolCreateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      nullptr,
      0,
      1,
      1,
      &poolSize
    };

    if (vkCreateDescriptorPool(*m_device, &poolCreateInfo, nullptr, &m_imguiDescriptorPool) != VK_SUCCESS)
      throw std::runtime_error("Could not create ImGui descriptor pool");

    ImGui_ImplVulkan_InitInfo initInfo = {
      *m_control,
      m_device->getOwningPhysical(),
      *m_device,
      m_graphicsQueue->get().getFamily(),
      m_graphicsQueue->get(),
      nullptr,
      m_imguiDescriptorPool,
      m_surface->getCapabilities().minImageCount + 1, // TODO make this easier / based on swapchain
      m_swapchain->getNumImages(),
      VK_SAMPLE_COUNT_1_BIT,
      nullptr,
      checkResultFn
    };

    // Need VkRenderPass
    // Need descriptor pool - can allocate 1 combined image sampler
    ImGui_ImplVulkan_Init(&initInfo, m_finalStep->getRenderPass());

    // this should be a command buffer that is already started
    CommandBuffer& cmdBuff = (m_commandPool->allocateCommandBuffer());
    cmdBuff.start(true);

    ImGui_ImplVulkan_CreateFontsTexture(cmdBuff);

    cmdBuff.end();

    VkCommandBuffer vkCmdBuff = cmdBuff;
    VkSubmitInfo submitInfo = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO,
      nullptr,
      0,
      nullptr,
      0,
      1,
      &vkCmdBuff,
      0,
      nullptr
    };

    vkQueueSubmit(m_graphicsQueue->get(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue->get()); // TODO: not this
  }

  void Renderer::shutdownImGui() {
    vkDestroyDescriptorPool(*m_device, m_imguiDescriptorPool, nullptr);

    ImGui_ImplVulkan_DestroyFontUploadObjects();
    ImGui_ImplVulkan_Shutdown();
  }
#endif


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

    auto&           graphicsQueue      = m_graphicsQueue->get();
    VkCommandBuffer deferredCmdBuff    = m_geometryStep->getCommandBuffer();
    VkCommandBuffer shadowCmdBuff      = m_shadowMapStep->getCommandBuffer();
    VkCommandBuffer blurCmdBuff        = m_blurStep->getCommandBuffer();
    VkCommandBuffer globalLightCmdBuff = m_globalLightStep->getCommandBuffer();
    VkCommandBuffer localLightCmdBuff  = m_finalStep->getCommandBuffer(nextImageIndex);

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
    submitInfo.pSignalSemaphores = &m_shadowSemaphore;
    submitInfo.pCommandBuffers   = &shadowCmdBuff;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);

    submitInfo.pWaitSemaphores   = &m_shadowSemaphore;
    submitInfo.pSignalSemaphores = &m_blurSemaphore;
    submitInfo.pCommandBuffers   = &blurCmdBuff;
    
    // Note: This is assuming that the graphics queue supports compute!
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);

    submitInfo.pWaitSemaphores   = &m_blurSemaphore;
    submitInfo.pSignalSemaphores = &m_globalLightSemaphore;
    submitInfo.pCommandBuffers   = &globalLightCmdBuff;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);

#ifdef DW_USE_IMGUI
    // this updates the second subpass that is defined for imgui rendering
    m_finalStep->writeCmdBuff(m_swapchain->getFrameBuffers(), m_globalLitFrameBuffer->getImages().front(), {}, nextImageIndex);
#endif

    submitInfo.pWaitSemaphores   = &m_globalLightSemaphore;
    submitInfo.pSignalSemaphores = &m_swapchain->getImageRenderReadySemaphore();
    submitInfo.pCommandBuffers   = &localLightCmdBuff;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);

    m_swapchain->present();
    graphicsQueue.waitIdle(); // todo: not wait

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
    // NOTE: Global lights are NOT dynamic

    CameraUniform cam = {
      m_camera.get().getView(),
      m_camera.get().getProj(),
      m_camera.get().getEyePos(),
      m_camera.get().getViewDir(),
      m_camera.get().getFarDist(),
      m_camera.get().getNearDist()
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

    data                   = m_localLightsUBO->map();
    LightUBO* lightUBOdata = reinterpret_cast<LightUBO*>(data);
    for (size_t i     = 0; i < m_lights.size(); ++i)
      lightUBOdata[i] = m_lights[i].get().getAsUBO();
    m_localLightsUBO->unMap();

    // shader control:
    assert(m_shaderControl);
    data = m_shaderControlBuffer->map();
    *reinterpret_cast<ShaderControl*>(data) = *m_shaderControl;
    m_shaderControlBuffer->unMap();

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

  void Renderer::setShaderControl(ShaderControl* control) {
    m_shaderControl = std::move(control);
  }

  void Renderer::setCamera(util::Ref<Camera> camera) {
    m_camera = camera;
  }

  void Renderer::setLocalLights(LightContainer const& lights) {
    m_lights = lights;
    m_localLightsUBO.reset();

    VkDeviceSize lightUniformSize = sizeof(LightUBO);
    m_localLightsUBO              = util::make_ptr<Buffer>(Buffer::CreateUniform(*m_device,
                                                                                 lightUniformSize * lights.size()));

    m_finalStep->updateDescriptorSets(m_gbuffer->getImageViews(), m_globalLitFrameBuffer->getImageViews().front(), *m_cameraUBO, *m_localLightsUBO, m_sampler);
    m_finalStep->writeCmdBuff(m_swapchain->getFrameBuffers(), m_globalLitFrameBuffer->getImages().front());
  }

  void Renderer::setGlobalLights(GlobalLightContainer lights) {
    size_t uboSize = sizeof(ShadowedUBO) * lights.size();
    m_globalLightsUBO.reset();
    m_globalLightsUBO = util::make_ptr<Buffer>(Buffer::CreateUniform(*m_device, uboSize, true));

    // Stage memory
    {
      // NOTE: Global lights are NOT dynamic, hence they are updated here.
      Buffer staging = Buffer::CreateStaging(*m_device, uboSize);
      auto   data    = reinterpret_cast<ShadowedUBO*>(staging.map());
      for (size_t i = 0; i < lights.size(); ++i)
        data[i]     = lights[i].getAsShadowUBO();
      staging.unMap();

      CommandBuffer& cmdBuff = m_transferCmdPool->allocateCommandBuffer();
      cmdBuff.start(true);

      VkBufferCopy copy = {0, 0, uboSize};
      vkCmdCopyBuffer(cmdBuff, staging, *m_globalLightsUBO, 1, &copy);

      cmdBuff.end();
      m_transferQueue->get().submitOne(cmdBuff);
      m_transferQueue->get().waitIdle();

      m_transferCmdPool->freeCommandBuffer(cmdBuff);
    }

    m_globalLights.clear();
    m_globalLights.reserve(lights.size());

    // TODO: instead of remaking depth buffers from scratch, instead reuse them and only allocate new ones
    // TODO: as needed

    for (auto& light : lights) {
      util::ptr<Framebuffer> depthBuff = util::make_ptr<Framebuffer>(*m_device, SHADOW_DEPTH_MAP_EXTENT);

      depthBuff->addImage(VK_IMAGE_ASPECT_COLOR_BIT,
                          VK_IMAGE_TYPE_2D,
                          VK_IMAGE_VIEW_TYPE_2D,
                          VK_FORMAT_R32G32B32A32_SFLOAT,
                          SHADOW_DEPTH_MAP_EXTENT,
                          VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                          1,
                          1,
                          false,
                          false,
                          false,
                          false);

      depthBuff->addImage(VK_IMAGE_ASPECT_DEPTH_BIT,
                          VK_IMAGE_TYPE_2D,
                          VK_IMAGE_VIEW_TYPE_2D,
                          VK_FORMAT_D24_UNORM_S8_UINT,
                          SHADOW_DEPTH_MAP_EXTENT,
                          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                          1,
                          1,
                          false,
                          false,
                          false,
                          false);

      depthBuff->finalize(m_shadowMapStep->getRenderPass());

      m_globalLights.emplace_back(light).m_depthBuffer = depthBuff;
    }

    if (m_modelUBO) {
      m_shadowMapStep->updateDescriptorSets(*m_modelUBO, *m_globalLightsUBO);
      m_shadowMapStep->writeCmdBuff(m_globalLights, m_objList, m_modelUBOdynamicAlignment);
    }

    m_blurStep->writeCmdBuff(m_globalLights, *m_blurIntermediate, *m_blurIntermediateView);

    m_globalLightStep->updateDescriptorSets(m_gbuffer->getImageViews(),
                                            m_globalLights,
                                            *m_cameraUBO,
                                            *m_globalLightsUBO,
                                            *m_shaderControlBuffer,
                                            m_sampler);

    m_globalLightStep->writeCmdBuff(*m_globalLitFrameBuffer);
  }

  void Renderer::setScene(std::vector<util::Ref<Object>> const& objects) {
    m_objList = objects;

    if (m_modelUBOdata)
      alignedFree(m_modelUBOdata);

    prepareDynamicUniformBuffers();

    m_geometryStep->updateDescriptorSets(*m_modelUBO, *m_cameraUBO);
    m_geometryStep->writeCmdBuff(*m_gbuffer, m_objList, m_modelUBOdynamicAlignment);

    if (m_globalLightsUBO) {
      m_shadowMapStep->updateDescriptorSets(*m_modelUBO, *m_globalLightsUBO);
      m_shadowMapStep->writeCmdBuff(m_globalLights, m_objList, m_modelUBOdynamicAlignment);
    }
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
      VK_FILTER_LINEAR,//NEAREST,
      VK_FILTER_LINEAR,//NEAREST,
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
    m_shaderControlBuffer = util::make_ptr<Buffer>(Buffer::CreateUniform(*m_device, sizeof(ShaderControl)));
  }

  void Renderer::setupFrameBufferImages() {
    VkExtent3D gbuffExtent = {m_swapchain->getImageSize().width, m_swapchain->getImageSize().height, 1};

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

    m_globalLitFrameBuffer = util::make_ptr<Framebuffer>(*m_device, gbuffExtent);

    m_globalLitFrameBuffer->addImage(VK_IMAGE_ASPECT_COLOR_BIT,
                                     VK_IMAGE_TYPE_2D,
                                     VK_IMAGE_VIEW_TYPE_2D,
                                     VK_FORMAT_R8G8B8A8_UNORM,
                                     gbuffExtent,
                                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                     1,
                                     1,
                                     false,
                                     false,
                                     false,
                                     false);

    MemoryAllocator allocator(m_device->getOwningPhysical());
    m_blurIntermediate = util::make_ptr<DependentImage>(*m_device);
    m_blurIntermediate->initImage(VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT,
      SHADOW_DEPTH_MAP_EXTENT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 1, 1, false, false, false, false);

    m_blurIntermediate->back(allocator, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    m_blurIntermediateView = util::make_ptr<ImageView>(m_blurIntermediate->createView());

    /*m_localLitFramebuffer = util::make_ptr<Framebuffer>(*m_device, gbuffExtent);

    m_localLitFramebuffer->addImage(VK_IMAGE_ASPECT_COLOR_BIT,
      VK_IMAGE_TYPE_2D,
      VK_IMAGE_VIEW_TYPE_2D,
      VK_FORMAT_R8G8B8A8_UNORM,
      gbuffExtent,
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      1,
      1,
      false,
      false,
      false,
      false);*/
  }

  void Renderer::setupRenderSteps() {
    m_geometryStep = util::make_ptr<GeometryStep>(*m_device, *m_commandPool);

    m_geometryStep->setupShaders();
    m_geometryStep->setupDescriptors();
    m_geometryStep->setupRenderPass({m_gbuffer->getImages().begin(), m_gbuffer->getImages().end()});
    m_geometryStep->setupPipelineLayout();
    m_geometryStep->setupPipeline(m_swapchain->getImageSize());

    m_shadowMapStep = util::make_ptr<ShadowMapStep>(*m_device, *m_commandPool);

    m_shadowMapStep->setupShaders();
    m_shadowMapStep->setupDescriptors();
    m_shadowMapStep->setupRenderPass({}); // no images on purpose
    m_shadowMapStep->setupPipelineLayout();
    m_shadowMapStep->setupPipeline({SHADOW_DEPTH_MAP_EXTENT.width, SHADOW_DEPTH_MAP_EXTENT.height});

    // Note: Assuming that command pool can do compute!
    m_blurStep = util::make_ptr<BlurStep>(*m_device, *m_commandPool);

    m_blurStep->setupShaders();
    m_blurStep->setupDescriptors();
    m_blurStep->setupPipelineLayout();
    m_blurStep->setupPipeline({});

    m_globalLightStep = util::make_ptr<GlobalLightStep>(*m_device, *m_commandPool);

    m_globalLightStep->setupShaders();
    m_globalLightStep->setupDescriptors();
    m_globalLightStep->setupRenderPass({m_globalLitFrameBuffer->getImages()[0]});
    m_globalLightStep->setupPipelineLayout();
    m_globalLightStep->setupPipeline(m_globalLitFrameBuffer->getExtent());

    m_finalStep = util::make_ptr<FinalStep>(*m_device, *m_commandPool, m_swapchain->getNumImages());

    m_finalStep->setupShaders();
    m_finalStep->setupDescriptors();
    m_finalStep->setupRenderPass({m_swapchain->getImages()[0]});
    m_finalStep->setupPipelineLayout();
    m_finalStep->setupPipeline(m_swapchain->getImageSize());
  }

  void Renderer::setupFrameBuffers() const {
    m_gbuffer->finalize(m_geometryStep->getRenderPass());
    m_globalLitFrameBuffer->finalize(m_globalLightStep->getRenderPass());
    //m_localLitFramebuffer->finalize(m_finalStep->getRenderPass());

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
                                VkExtent3D{m_surface->getWidth(), m_surface->getHeight(), 1});
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
    transitionImageLayout(graphicsBuff,
                          depthImage,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    graphicsBuff.end();
    m_graphicsQueue->get().submitOne(graphicsBuff);
    m_graphicsQueue->get().waitIdle();

    m_commandPool->freeCommandBuffer(graphicsBuff);
  }
}
