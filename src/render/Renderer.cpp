// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Renderer.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 09m 26d
// * Last Altered: 2020y 02m 26d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "render/Renderer.h"
#include "render/VulkanControl.h"
#include "render/GLFWControl.h"
#include "render/GLFWWindow.h"
#include "render/Surface.h"
#include "render/Swapchain.h"
#include "render/Queue.h"
#include "render/CommandBuffer.h"
#include "render/RenderPass.h"
#include "render/Shader.h"
#include "render/Vertex.h"
#include "render/Buffer.h"
#include "render/Mesh.h"
#include "render/Framebuffer.h"  // ReSharper likes to think this isn't used. IT IS!!!
#include "render/MemoryAllocator.h"
#include "render/Image.h"
#include "render/RenderSteps.h"

#include "obj/Light.h"
#include "obj/Camera.h"

#include "util/Trace.h"
#include "app/ImGui.h"

#include <array>
#include <cassert>
#include <algorithm>
#include "obj/Graphics.h"


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

  obj::Camera Renderer::s_defaultCamera;

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  ///////////////////////////// SETUP & SHUTDOWN //////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  void Renderer::init(GLFWWindow* window, bool startImgui) {
    assert(window);
    m_window = window;
    setupInstance();
    setupHelpers();
    setupDevice();

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
      throw std::runtime_error("Could not create blur semaphore");

    if (vkCreateSemaphore(*m_device, &semaphoreCreateInfo, nullptr, &m_globalLightSemaphore) != VK_SUCCESS)
      throw std::runtime_error("Could not create global lighting rendering semaphore");

    if (vkCreateSemaphore(*m_device, &semaphoreCreateInfo, nullptr, &m_localLightSemaphore) != VK_SUCCESS)
      throw std::runtime_error("Could not create local lighting semaphore");

    if (vkCreateSemaphore(*m_device, &semaphoreCreateInfo, nullptr, &m_ambientSemaphore) != VK_SUCCESS)
      throw std::runtime_error("Could not create ambient pass semaphore");

    if (vkCreateSemaphore(*m_device, &semaphoreCreateInfo, nullptr, &m_finalSemaphore) != VK_SUCCESS)
      throw std::runtime_error("Could not create post process semaphore");

    setupCommandPools();
    setupUniformBuffers();
    setupSamplers();

    setupWindow();

#ifdef DW_USE_IMGUI
    if (startImgui)
      setupImGui();
#endif
  }

  void Renderer::setupWindow() {
    setupSurface();
    setupSwapChain();
    setupFrameBufferImages();
    setupRenderSteps();
    setupFrameBuffers();
    transitionRenderImages();
  }

  void Renderer::shutdownWindow() {
    m_gbuffer.reset();
    m_blurIntermediateView.reset();
    m_blurIntermediate.reset();
    m_globalLitFrameBuffer.reset();
    m_localLitFramebuffer.reset();
    m_ambientFramebuffer.reset();

    m_splashScreenStep.reset();
    m_geometryStep.reset();
    m_shadowMapStep.reset();
    m_blurStep.reset();
    m_globalLightStep.reset();
    m_localLightStep.reset();
    m_ambientStep.reset();
    m_finalStep.reset();

    delete m_presentQueue;
    m_presentQueue = nullptr;

    m_surface.reset();
    m_swapchain.reset();
  }

  void Renderer::restartWindow() {
    vkDeviceWaitIdle(*m_device);

    shutdownWindow();
    setupWindow();
    setScene(m_scene);
  }

  void Renderer::shutdown(bool shutdownImgui) {
    vkDeviceWaitIdle(*m_device);

    m_materials = nullptr;

#ifdef DW_USE_IMGUI
    if (shutdownImgui)
      shutdownImGui();
#endif

    shutdownWindow();

    vkDestroySemaphore(*m_device, m_deferredSemaphore, nullptr);
    vkDestroySemaphore(*m_device, m_shadowSemaphore, nullptr);
    vkDestroySemaphore(*m_device, m_blurSemaphore, nullptr);
    vkDestroySemaphore(*m_device, m_globalLightSemaphore, nullptr);
    vkDestroySemaphore(*m_device, m_localLightSemaphore, nullptr);
    vkDestroySemaphore(*m_device, m_ambientSemaphore, nullptr);
    vkDestroySemaphore(*m_device, m_finalSemaphore, nullptr);
    m_deferredSemaphore    = nullptr;
    m_shadowSemaphore      = nullptr;
    m_blurSemaphore        = nullptr;
    m_globalLightSemaphore = nullptr;
    m_localLightSemaphore  = nullptr;
    m_ambientSemaphore     = nullptr;
    m_finalSemaphore       = nullptr;

    vkDestroySampler(*m_device, m_sampler, nullptr);
    m_sampler = nullptr;

    m_globalLights.clear();
    m_scene.reset();

    if (m_modelUBOdata) {
      alignedFree(m_modelUBOdata);
      m_modelUBOdata = nullptr;
    }

    m_modelUBOdata = nullptr;

    vkDestroySampler(*m_device, m_sampler, nullptr);
    m_modelUBO.reset();
    m_cameraUBO.reset();
    m_globalLightsUBO.reset();
    m_localLightsUBO.reset();
    m_globalImportanceUBO.reset();
    m_materialsUBO.reset();

    m_shaderControlBuffer.reset();
    m_shaderControl = nullptr;

    m_graphicsCmdPool.reset();
    m_transferCmdPool.reset();
    m_computeCmdPool.reset();

    delete m_graphicsQueue;
    m_graphicsQueue = nullptr;

    delete m_transferQueue;
    m_transferQueue = nullptr;

    delete m_computeQueue;
    m_computeQueue = nullptr;

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
    CommandBuffer& cmdBuff = (m_graphicsCmdPool->allocateCommandBuffer());
    cmdBuff.start(true);

    ImGui_ImplVulkan_CreateFontsTexture(cmdBuff);

    cmdBuff.end();

    VkCommandBuffer vkCmdBuff  = cmdBuff;
    VkSubmitInfo    submitInfo = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO,
      nullptr,
      0,
      nullptr,
      nullptr,
      1,
      &vkCmdBuff,
      0,
      nullptr
    };

    vkQueueSubmit(m_graphicsQueue->get(), 1, &submitInfo, nullptr);
    vkQueueWaitIdle(m_graphicsQueue->get()); // TODO: not this
  }

  void Renderer::shutdownImGui() const {
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

  void Renderer::drawFrame() const {
    assert(m_swapchain->isPresentReady());
    if (!m_scene || m_scene->getObjects().empty())
      return;

    uint32_t     nextImageIndex = m_swapchain->getNextImageIndex();
    Image const& nextImage      = m_swapchain->getNextImage();

    auto&           graphicsQueue      = m_graphicsQueue->get();
    auto&           computeQueue       = m_computeQueue->get();
    VkCommandBuffer deferredCmdBuff    = m_geometryStep->getCommandBuffer();
    VkCommandBuffer shadowCmdBuff      = m_shadowMapStep->getCommandBuffer();
    VkCommandBuffer blurCmdBuff        = m_blurStep->getCommandBuffer();
    VkCommandBuffer globalLightCmdBuff = m_globalLightStep->getCommandBuffer();
    VkCommandBuffer localLightCmdBuff  = m_localLightStep->getCommandBuffer();
    VkCommandBuffer ambientCmdBuff     = m_ambientStep->getCommandBuffer();
    VkCommandBuffer finalCmdBuff       = m_finalStep->getCommandBuffer(nextImageIndex);

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

    submitInfo.pWaitSemaphores = &m_shadowSemaphore;

    if (m_blurEnabled) {
      submitInfo.pSignalSemaphores = &m_blurSemaphore;
      submitInfo.pCommandBuffers   = &blurCmdBuff;

      vkQueueSubmit(computeQueue, 1, &submitInfo, nullptr);

      submitInfo.pWaitSemaphores = &m_blurSemaphore;
    }

    if (m_globalLightEnabled) {
      submitInfo.pSignalSemaphores = &m_globalLightSemaphore;
      submitInfo.pCommandBuffers   = &globalLightCmdBuff;

      vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);

      submitInfo.pWaitSemaphores = &m_globalLightSemaphore;
    }

    submitInfo.pSignalSemaphores = &m_localLightSemaphore;
    submitInfo.pCommandBuffers   = &localLightCmdBuff;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);

    submitInfo.pWaitSemaphores = &m_localLightSemaphore;

    submitInfo.pSignalSemaphores = &m_ambientSemaphore;
    submitInfo.pCommandBuffers   = &ambientCmdBuff;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);

    submitInfo.pWaitSemaphores = &m_ambientSemaphore;

#ifdef DW_USE_IMGUI
    // this updates the second subpass that is defined for imgui rendering
    m_finalStep->writeCmdBuff(m_swapchain->getFrameBuffers(),
                              m_globalLitFrameBuffer->getImages().front(),
                              {},
                              nextImageIndex);
#endif

    submitInfo.pSignalSemaphores = &m_swapchain->getImageRenderReadySemaphore();
    submitInfo.pCommandBuffers   = &finalCmdBuff;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);

    m_swapchain->present();
    graphicsQueue.waitIdle(); // todo: not wait

  }

  void Renderer::displayLogo(util::ptr<ImageView> logoView) const {
    assert(m_swapchain->isPresentReady());
    uint32_t     nextImageIndex = m_swapchain->getNextImageIndex();
    Image const& nextImage      = m_swapchain->getNextImage();

    auto& graphicsQueue = m_graphicsQueue->get();

    m_splashScreenStep->updateDescriptorSets(nextImageIndex, *logoView, m_sampler);
    m_splashScreenStep->writeCmdBuff(nextImageIndex, m_swapchain->getFrameBuffers()[nextImageIndex]);

    VkCommandBuffer splashCmdBuff = m_splashScreenStep->getCommandBuffer(nextImageIndex);

    VkPipelineStageFlags semaphoreWaitFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo         submitInfo        = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO,
      nullptr,
      1,
      &m_swapchain->getNextImageSemaphore(),
      &semaphoreWaitFlag,
      1,
      &splashCmdBuff,
      1,
      &m_swapchain->getImageRenderReadySemaphore()
    };

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);

    m_swapchain->present();
    graphicsQueue.waitIdle(); // todo: not wait
  }

  void Renderer::uploadMeshes(MeshManager::MeshMap& meshes) const {
    std::vector<Mesh::StagingBuffs> stagingBuffers;

    stagingBuffers.reserve(meshes.size());
    for (auto& mesh : meshes) {
      stagingBuffers.push_back(mesh.second->createAllBuffs(*m_device));
      mesh.second->uploadStaging(stagingBuffers.back());
    }

    CommandBuffer& moveBuff = m_transferCmdPool->allocateCommandBuffer();

    moveBuff.start(true);
    for (size_t i = 0; i < meshes.size(); ++i) {
      meshes[i]->uploadCmds(moveBuff, stagingBuffers[i]);
    }
    moveBuff.end();

    m_transferQueue->get().submitOne(moveBuff);
    m_transferQueue->get().waitIdle();

    m_transferCmdPool->freeCommandBuffer(moveBuff);
  }

  void Renderer::uploadTextures(TextureManager::TexMap& textures) const {
    std::unordered_map<TextureManager::TexMap::key_type, Texture::StagingBuffs> stagingBuffers;

    for (auto& tex : textures) {
      if (!tex.second->isLoaded()) {
        tex.second->uploadStaging(
                                  stagingBuffers.try_emplace(tex.first, tex.second->createAllBuffs(*m_device))
                                                .first->second
                                 );
      }
    }

    CommandBuffer& moveBuff = m_graphicsCmdPool->allocateCommandBuffer();

    moveBuff.start(true);
    for (auto& staging : stagingBuffers) {
      textures.at(staging.first)->uploadCmds(moveBuff, staging.second);
    }
    moveBuff.end();

    m_graphicsQueue->get().submitOne(moveBuff);
    m_graphicsQueue->get().waitIdle();
    m_graphicsCmdPool->freeCommandBuffer(moveBuff);
  }

  void Renderer::uploadMaterials(MaterialManager::MtlMap& materials) {
    //std::unordered_map<MaterialManager::MtlMap::key_type, Material::StagingBuffs> stagingBuffers;

    m_materials = &materials;

    if (m_materialsUBO)
      m_materialsUBO.reset();

    m_materialsUBO = util::make_ptr<Buffer>(Buffer::CreateUniform(*m_device,
                                                                  materials.size() * sizeof(Material::MaterialUBO)));

    auto data = reinterpret_cast<Material::MaterialUBO*>(m_materialsUBO->map());
    for (auto& mtl : materials) {
      data[mtl.second->getID()] = mtl.second->getAsUBO();
    }
    m_materialsUBO->unMap();
  }

  void Renderer::updateUniformBuffers(uint32_t imageIndex) const {
    // NOTE: Global lights are NOT dynamic
    auto camera = m_scene->getCamera();
    CameraUniform cam    = {
      camera->worldToCamera(),
      camera->cameraToNDC(),
      camera->getWorldPos(),
      camera->getForward(),
      camera->getFar(),
      camera->getNear()
    };

    void* data = m_cameraUBO->map();
    memcpy(data, &cam, sizeof(cam));
    m_cameraUBO->unMap();

    assert(m_modelUBOdata);

    for (uint32_t i = 0; i < m_scene->getObjects().size(); ++i) {
      auto objData = reinterpret_cast<ObjectUniform*>(
        reinterpret_cast<char*>(m_modelUBOdata) + i * m_modelUBOdynamicAlignment);

      auto obj = m_scene->getObjects()[i];

      objData->model    = m_scene->getObjects()[i]->getTransform()->getMatrix();
      objData->mtlIndex = obj->get<Graphics>() ? obj->get<obj::Graphics>()->getMesh()->getMaterial()->getID() : 0;
    }

    data = m_modelUBO->map();
    memcpy(data, m_modelUBOdata, m_modelUBO->getSize());
    m_modelUBO->unMap();

    data                   = m_localLightsUBO->map();
    LightUBO* lightUBOdata = reinterpret_cast<LightUBO*>(data);
    for (size_t i     = 0; i < m_scene->getLights().size(); ++i)
      lightUBOdata[i] = m_scene->getLights()[i]->getAsUBO();

    *reinterpret_cast<int32_t*>(lightUBOdata + LocalLightingStep::MAX_LOCAL_LIGHTS) = static_cast<uint32_t>(m_scene->
                                                                                                            getLights().
                                                                                                            size());
    m_localLightsUBO->unMap();

    // shader control:
    assert(m_shaderControl);
    data                                    = m_shaderControlBuffer->map();
    *reinterpret_cast<ShaderControl*>(data) = *m_shaderControl;
    m_shaderControlBuffer->unMap();

    // generate new random samples:
    data = m_globalImportanceUBO->map();

    auto                      vec2data   = reinterpret_cast<GlobalLightStep::ImportanceSampleUBO*>(data);
    static constexpr uint32_t numSamples = 10;

    int pos = 0;
    for (uint32_t i = 0; i < numSamples; ++i) {
      int   kk = i;
      float u  = 0.f;
      for (float p = 0.5f; kk; p *= .5f, kk >>= 1)
        if (kk & 1)
          u += p;

      vec2data->samples[pos++] = u;
      vec2data->samples[pos++] = (i + .5f) / numSamples;
    }

    //vec2data->numSamples = numSamples;

    VkMappedMemoryRange importanceRange = {
      VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
      nullptr,
      m_globalImportanceUBO->getMemory(),
      0,
      VK_WHOLE_SIZE
    };

    vkFlushMappedMemoryRanges(*m_device, 1, &importanceRange);
    m_globalImportanceUBO->unMap();

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

  void Renderer::setScene(util::ptr<Scene> scene) {
    if (!scene)
      return;

    if (m_scene) {
      m_geometryStep->getCommandBuffer().reset();
      m_shadowMapStep->getCommandBuffer().reset();
      m_blurStep->getCommandBuffer().reset();
      m_globalLightStep->getCommandBuffer().reset();
      m_localLightStep->getCommandBuffer().reset();
      m_ambientStep->getCommandBuffer().reset();

      for (uint32_t i = 0; i < m_swapchain->getNumImages(); ++i)
        m_finalStep->getCommandBuffer(i).reset();
      //vkResetCommandPool(*m_device, *m_graphicsCmdPool, 0/*VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT*/);
    }

    m_scene = scene;

    Scene::LightContainer const& lights       = scene->getLights();
    std::vector<ShadowedLight>   shadowLights = scene->getGlobalLights();

    // Local lights
    if (!m_localLightsUBO) {
      VkDeviceSize lightUniformSize = sizeof(LightUBO);
      m_localLightsUBO              = util::make_ptr<Buffer>(Buffer::CreateUniform(*m_device,
                                                                                   lightUniformSize * LocalLightingStep
                                                                                   ::
                                                                                   MAX_LOCAL_LIGHTS + sizeof(uint32_t
                                                                                   ))); // the light count is at the very end of the buffer
    }

    // Global lights
    size_t uboSize = sizeof(ShadowedUBO) * GlobalLightStep::MAX_GLOBAL_LIGHTS + sizeof(uint32_t);
    if (!m_globalLightsUBO) {
      // +sizeof(uint32_t) because light count 
      m_globalLightsUBO = util::make_ptr<Buffer>(Buffer::CreateUniform(*m_device, uboSize, true));
    }

    // Stage memory
    {
      // NOTE: Global lights are NOT dynamic, hence they are updated here.
      Buffer staging = Buffer::CreateStaging(*m_device, uboSize);
      auto   data    = reinterpret_cast<ShadowedUBO*>(staging.map());
      for (size_t i = 0; i < shadowLights.size(); ++i)
        data[i]     = shadowLights[i].getAsShadowUBO();

      *reinterpret_cast<uint32_t*>(data + GlobalLightStep::MAX_GLOBAL_LIGHTS) = shadowLights.size();
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
    m_globalLights.reserve(shadowLights.size());

    // TODO: instead of remaking depth buffers from scratch, instead reuse them and only allocate new ones
    // TODO: as needed

    for (auto& light : shadowLights) {
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

    // Object list
    if (m_modelUBOdata)
      alignedFree(m_modelUBOdata);

    prepareDynamicUniformBuffers();

    // Descriptors
    m_geometryStep->updateDescriptorSets(*m_modelUBO,
                                         *m_cameraUBO,
                                         *m_materialsUBO,
                                         *m_shaderControlBuffer,
                                         *m_materials,
                                         m_sampler);
    m_geometryStep->writeCmdBuff(*m_gbuffer, scene->getObjects(), m_modelUBOdynamicAlignment);

    m_shadowMapStep->updateDescriptorSets(*m_modelUBO, *m_globalLightsUBO);
    m_shadowMapStep->writeCmdBuff(m_globalLights, scene->getObjects(), m_modelUBOdynamicAlignment);

    m_blurStep->writeCmdBuff(m_globalLights, *m_blurIntermediate, *m_blurIntermediateView);

    m_globalLightStep->updateDescriptorSets(m_gbuffer->getImageViews(),
                                            m_globalLights,
                                            *m_scene->getBackground()->getView(),
                                            *m_scene->getIrradiance()->getView(),
                                            *m_cameraUBO,
                                            *m_globalLightsUBO,
                                            *m_globalImportanceUBO,
                                            *m_shaderControlBuffer,
                                            m_sampler);

    m_globalLightStep->writeCmdBuff(*m_globalLitFrameBuffer);

    m_localLightStep->updateDescriptorSets(m_gbuffer->getImageViews(),
                                           m_globalLitFrameBuffer->getImageViews().front(),
                                           *m_cameraUBO,
                                           *m_localLightsUBO,
                                           *m_shaderControlBuffer,
                                           m_sampler);
    m_localLightStep->writeCmdBuff(*m_localLitFramebuffer, m_globalLitFrameBuffer->getImages().front());

    m_ambientStep->updateDescriptorSets(m_gbuffer->getImageViews()[0], m_gbuffer->getImageViews()[1], m_sampler);
    m_ambientStep->writeCmdBuff(*m_ambientFramebuffer);

    m_finalStep->updateDescriptorSets(m_localLitFramebuffer->getImageViews().front(), m_sampler);
    m_finalStep->writeCmdBuff(m_swapchain->getFrameBuffers(), m_localLitFramebuffer->getImages().front());
  }

  void Renderer::prepareDynamicUniformBuffers() {
    const size_t minUboAlignment = m_device->getOwningPhysical().getLimits().minUniformBufferOffsetAlignment;

    m_modelUBOdynamicAlignment = sizeof(ObjectUniform);
    if (minUboAlignment > 0) {
      m_modelUBOdynamicAlignment = (m_modelUBOdynamicAlignment + (minUboAlignment - 1)) & ~(minUboAlignment - 1);
    }

    size_t modelUBOsize = m_modelUBOdynamicAlignment * m_scene->getObjects().size();

    Trace::Warn << "Minimum UBO Align  : " << minUboAlignment << Trace::Stop;
    Trace::Warn << "Size of Object UBO : " << sizeof(ObjectUniform) << Trace::Stop;
    Trace::Warn << "Model UBO Alignment: " << m_modelUBOdynamicAlignment << Trace::Stop;
    Trace::Warn << "(2^N) Alignment    : " << (m_modelUBOdynamicAlignment & -m_modelUBOdynamicAlignment) << Trace::Stop;

    m_modelUBOdata = static_cast<ObjectUniform*>(alignedAlloc(modelUBOsize,
                                                              m_modelUBOdynamicAlignment & -m_modelUBOdynamicAlignment)
    );
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
    uint32_t computeFamily  = physical.pickQueueFamily(VK_QUEUE_COMPUTE_BIT);

    LogicalDevice::QueueList queueList;
    queueList.push_back(std::make_pair(graphicsFamily, std::vector<float>({1})));

    if (graphicsFamily != transferFamily)
      queueList.push_back(std::make_pair(transferFamily, std::vector<float>({1})));

    if (computeFamily != graphicsFamily && computeFamily != transferFamily)
      queueList.push_back(std::make_pair(computeFamily, std::vector<float>({1})));

    m_device = new LogicalDevice(physical, deviceLayers, deviceExtensions, queueList, features, features, false);

    m_graphicsQueue = new util::Ref<Queue>(m_device->getBestQueue(VK_QUEUE_GRAPHICS_BIT));
    if (!m_graphicsQueue->get().isValid())
      throw std::runtime_error("no graphics queue available");

    m_transferQueue = new util::Ref<Queue>(m_device->getBestQueue(VK_QUEUE_TRANSFER_BIT));
    assert(m_transferQueue && m_transferQueue->get().isValid()); // graphics queues implicitly allow transfer

    m_computeQueue = new util::Ref<Queue>(m_device->getBestQueue(VK_QUEUE_COMPUTE_BIT));
    if (!m_computeQueue->get().isValid())
      throw std::runtime_error("no compute queue available");
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
    m_graphicsCmdPool = util::make_ptr<CommandPool>(*m_device, m_graphicsQueue->get().getFamily());
    m_transferCmdPool = util::make_ptr<CommandPool>(*m_device, m_transferQueue->get().getFamily());
    m_computeCmdPool  = util::make_ptr<CommandPool>(*m_device, m_computeQueue->get().getFamily());
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
      16.f,
      VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
      false
    };

    if (vkCreateSampler(*m_device, &createInfo, nullptr, &m_sampler) != VK_SUCCESS || !m_sampler)
      throw std::runtime_error("Could not create sampler");
  }

  void Renderer::setupUniformBuffers() {
    VkDeviceSize cameraUniformSize = sizeof(CameraUniform);

    m_cameraUBO           = util::make_ptr<Buffer>(Buffer::CreateUniform(*m_device, cameraUniformSize));
    m_shaderControlBuffer = util::make_ptr<Buffer>(Buffer::CreateUniform(*m_device, sizeof(ShaderControl)));
    m_globalImportanceUBO = util::make_ptr<Buffer>(Buffer::CreateUniform(*m_device,
                                                                         sizeof(GlobalLightStep::ImportanceSampleUBO)));
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
                                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                     1,
                                     1,
                                     false,
                                     false,
                                     false,
                                     false);

    m_localLitFramebuffer = util::make_ptr<Framebuffer>(*m_device, gbuffExtent);
    m_localLitFramebuffer->addImage(VK_IMAGE_ASPECT_COLOR_BIT,
                                    VK_IMAGE_TYPE_2D,
                                    VK_IMAGE_VIEW_TYPE_2D,
                                    VK_FORMAT_R8G8B8A8_UNORM,
                                    gbuffExtent,
                                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                    1,
                                    1,
                                    false,
                                    false,
                                    false,
                                    false);

    m_ambientFramebuffer = util::make_ptr<Framebuffer>(*m_device, gbuffExtent);
    m_ambientFramebuffer->addImage(VK_IMAGE_ASPECT_COLOR_BIT,
                                   VK_IMAGE_TYPE_2D,
                                   VK_IMAGE_VIEW_TYPE_2D,
                                   VK_FORMAT_R8G8B8A8_UNORM,
                                   gbuffExtent,
                                   VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                   1,
                                   1,
                                   false,
                                   false,
                                   false,
                                   false);

    MemoryAllocator allocator(m_device->getOwningPhysical());
    m_blurIntermediate = util::make_ptr<DependentImage>(*m_device);
    m_blurIntermediate->initImage(VK_IMAGE_TYPE_2D,
                                  VK_IMAGE_VIEW_TYPE_2D,
                                  VK_FORMAT_R32G32B32A32_SFLOAT,
                                  SHADOW_DEPTH_MAP_EXTENT,
                                  VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                  1,
                                  1,
                                  false,
                                  false,
                                  false,
                                  false);

    m_blurIntermediate->back(allocator, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    m_blurIntermediateView = util::make_ptr<ImageView>(m_blurIntermediate->createView());
  }

  void Renderer::setupRenderSteps() {
    m_splashScreenStep = util::make_ptr<SplashScreenStep>(*m_device, *m_graphicsCmdPool, m_swapchain->getNumImages());

    m_splashScreenStep->setupShaders();
    m_splashScreenStep->setupDescriptors();
    m_splashScreenStep->setupRenderPass({m_swapchain->getImages()[0]});
    m_splashScreenStep->setupPipelineLayout();
    m_splashScreenStep->setupPipeline(m_swapchain->getImageSize());

    m_geometryStep = util::make_ptr<GeometryStep>(*m_device, *m_graphicsCmdPool);

    m_geometryStep->setupShaders();
    m_geometryStep->setupDescriptors();
    m_geometryStep->setupRenderPass({m_gbuffer->getImages().begin(), m_gbuffer->getImages().end()});
    m_geometryStep->setupPipelineLayout();
    m_geometryStep->setupPipeline(m_swapchain->getImageSize());

    m_shadowMapStep = util::make_ptr<ShadowMapStep>(*m_device, *m_graphicsCmdPool);

    m_shadowMapStep->setupShaders();
    m_shadowMapStep->setupDescriptors();
    m_shadowMapStep->setupRenderPass({}); // no images on purpose
    m_shadowMapStep->setupPipelineLayout();
    m_shadowMapStep->setupPipeline({SHADOW_DEPTH_MAP_EXTENT.width, SHADOW_DEPTH_MAP_EXTENT.height});

    m_blurStep = util::make_ptr<BlurStep>(*m_device, *m_computeCmdPool);

    m_blurStep->setupShaders();
    m_blurStep->setupDescriptors();
    m_blurStep->setupPipelineLayout();
    m_blurStep->setupPipeline({});

    m_globalLightStep = util::make_ptr<GlobalLightStep>(*m_device, *m_graphicsCmdPool);

    m_globalLightStep->setupShaders();
    m_globalLightStep->setupDescriptors();
    m_globalLightStep->setupRenderPass({m_globalLitFrameBuffer->getImages()[0]});
    m_globalLightStep->setupPipelineLayout();
    m_globalLightStep->setupPipeline(m_globalLitFrameBuffer->getExtent());

    m_localLightStep = util::make_ptr<LocalLightingStep>(*m_device, *m_graphicsCmdPool);

    m_localLightStep->setupShaders();
    m_localLightStep->setupDescriptors();
    m_localLightStep->setupRenderPass({m_localLitFramebuffer->getImages()[0]});
    m_localLightStep->setupPipelineLayout();
    m_localLightStep->setupPipeline(m_localLitFramebuffer->getExtent());

    m_ambientStep = util::make_ptr<AmbientStep>(*m_device, *m_graphicsCmdPool);

    m_ambientStep->setupShaders();
    m_ambientStep->setupDescriptors();
    m_ambientStep->setupRenderPass({m_ambientFramebuffer->getImages()[0]});
    m_ambientStep->setupPipelineLayout();
    m_ambientStep->setupPipeline(m_ambientFramebuffer->getExtent());

    m_finalStep = util::make_ptr<FinalStep>(*m_device, *m_graphicsCmdPool, m_swapchain->getNumImages());

    m_finalStep->setupShaders();
    m_finalStep->setupDescriptors();
    m_finalStep->setupRenderPass({m_swapchain->getImages()[0]});
    m_finalStep->setupPipelineLayout();
    m_finalStep->setupPipeline(m_swapchain->getImageSize());
  }

  void Renderer::setupFrameBuffers() const {
    m_gbuffer->finalize(m_geometryStep->getRenderPass());
    m_globalLitFrameBuffer->finalize(m_globalLightStep->getRenderPass());
    m_localLitFramebuffer->finalize(m_localLightStep->getRenderPass());
    m_ambientFramebuffer->finalize(m_ambientStep->getRenderPass());

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

    CommandBuffer& graphicsBuff = m_graphicsCmdPool->allocateCommandBuffer();
    graphicsBuff.start(true);

    const Image& depthImage = dynamic_cast<const Image&>(m_gbuffer->getImages().back());
    transitionImageLayout(graphicsBuff,
                          depthImage,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    graphicsBuff.end();
    m_graphicsQueue->get().submitOne(graphicsBuff);
    m_graphicsQueue->get().waitIdle();

    m_graphicsCmdPool->freeCommandBuffer(graphicsBuff);
  }
}
