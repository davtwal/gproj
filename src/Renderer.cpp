// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Renderer.cpp
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
#include "Framebuffer.h"

#include <cassert>
#include <algorithm>

namespace dw {
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  ///////////////////////////// SETUP & SHUTDOWN //////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  void Renderer::initGeneral() {
    GLFWControl::Init();

    openWindow();

    setupInstance();
    setupHelpers();
    setupDevice();
    setupSurface();
    setupSwapChain();
    setupCommandPools();
    setupCommandBuffers();
    transitionSwapChainImages();
  }

  void Renderer::initSpecific() {
    setupShaders();
    setupDescriptors();
    setupRenderSteps();
    setupSwapChainFrameBuffers();
    setupPipeline();
  }

  void Renderer::shutdown() {
    vkDeviceWaitIdle(*m_device);

    m_objList.clear();

    vkDestroyPipeline(*m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(*m_device, m_graphicsPipelineLayout, nullptr);

    m_renderPass.reset();

    vkDestroyDescriptorPool(*m_device, m_descriptorPool, nullptr);
    m_uniformBuffers.clear();
    vkDestroyDescriptorSetLayout(*m_device, m_descriptorSetLayout, nullptr);

    m_triangleFragShader.reset();
    m_triangleVertShader.reset();

    m_commandBuffers.clear();
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
    auto destroyFn = (PFN_vkDestroyDebugUtilsMessengerEXT)glfwGetInstanceProcAddress(*m_control,
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

    delete m_window;
    m_window = nullptr;

    delete m_inputHandler;
    m_inputHandler = nullptr;

    GLFWControl::Shutdown();
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

    uint32_t nextImageIndex = m_swapchain->getNextImageIndex();
    Image const& nextImage = m_swapchain->getNextImage();

    auto& graphicsQueue = *m_graphicsQueue;

    updateUniformBuffers(nextImageIndex);

    graphicsQueue->submitOne(*m_commandBuffers[nextImageIndex],
                             {m_swapchain->getNextImageSemaphore()},
                             {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
                             {m_swapchain->getImageRenderReadySemaphore()});

    graphicsQueue->waitSubmit();
    graphicsQueue->waitIdle();

    m_swapchain->present();
    graphicsQueue->waitIdle();
  }

  void Renderer::uploadMeshes(std::vector<util::ptr<Mesh>> const& meshes) const {
    std::vector<Mesh::StagingBuffs> stagingBuffers;
    stagingBuffers.reserve(meshes.size());
    for(auto& mesh : meshes) {
      stagingBuffers.push_back(mesh->createAllBuffs(*m_device));
      mesh->uploadStaging(stagingBuffers.back());
    }
    
    CommandBuffer& moveBuff = m_transferCmdPool->allocateCommandBuffer();
    
    moveBuff.start(true);
    for(size_t i = 0; i < meshes.size(); ++i) {
      meshes[i]->uploadCmds(moveBuff, stagingBuffers[i]);
    }
    moveBuff.end();
    
    m_transferQueue->ref.submitOne(moveBuff);
    m_transferQueue->ref.waitIdle();

    m_transferCmdPool->freeCommandBuffer(moveBuff);
  }

  void Renderer::setScene(std::vector<Object> const& objects) {
    m_objList = objects;
    writeCommandBuffers();
  }

  struct MVPTransformUniform {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
  };

}

#include <chrono>

namespace dw {

  void Renderer::updateUniformBuffers(uint32_t imageIndex) {
    // for frequently changing values, we don't want to map/unmap every frame. for that we'd want push constants. ill get to those after
    // making UBOs work.
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto  currentTime = std::chrono::high_resolution_clock::now();
    float time        = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    MVPTransformUniform mvp = {
      rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
      lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
      glm::perspective(glm::radians(45.0f),
                       static_cast<float>(m_swapchain->getImageSize().width) / static_cast<float>(m_swapchain->
                                                                                                  getImageSize().height
                       ),
                       0.1f,
                       10.0f)
    };

    // flip y-axis because Vulkan renders upside down compared to OpenGL
    //mvp.proj[1][1] *= -1;

    void* data = m_uniformBuffers[imageIndex].map();
    memcpy(data, &mvp, sizeof(mvp));
    m_uniformBuffers[imageIndex].unMap();
  }

  void Renderer::writeCommandBuffers() {
    if (m_objList.empty())
      return;

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

      vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

      vkCmdBeginRenderPass(*commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineLayout, 0, 1, &m_descriptorSets[i], 0, nullptr);
      util::ptr<Mesh> curMesh = nullptr;
      for (auto& obj : m_objList) {
        if (obj.m_mesh != curMesh || curMesh == nullptr) {
          curMesh = obj.m_mesh;

          const VkBuffer&    buff   = curMesh->getVertexBuffer();
          const VkDeviceSize offset = 0;
          vkCmdBindVertexBuffers(*commandBuffer, 0, 1, &buff, &offset);
          vkCmdBindIndexBuffer(*commandBuffer, curMesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
        }

        vkCmdDrawIndexed(*commandBuffer, curMesh->getNumIndices(), 1, 0, 0, 0);
      }

      vkCmdEndRenderPass(*commandBuffer);

      commandBuffer->end();
    }
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

  void Renderer::openWindow() {
    m_window       = new GLFWWindow(800, 800, "hey lol");
    m_inputHandler = new InputHandler(*m_window);

    m_window->setInputHandler(m_inputHandler);
  }

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
    if (!(*m_graphicsQueue)->isValid())
      throw std::runtime_error("no graphics queue available");

    m_transferQueue = new util::Ref<Queue>(m_device->getBestQueue(VK_QUEUE_TRANSFER_BIT));
    assert(m_transferQueue && (*m_transferQueue)->isValid()); // graphics queues implicitly allow transfer
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //// SURFACE & SWAPCHAIN SETUP
  /////////////////////////////////////////////////////////////////////////////

  void Renderer::setupSurface() {
    m_surface = std::make_unique<Surface>(*m_window, m_device->getOwningPhysical());

    m_presentQueue = new util::Ref<Queue>(m_device->getPresentableQueue(*m_surface));
    if (!(*m_presentQueue)->isValid())
      throw std::runtime_error("no present queue available");
  }

  void Renderer::setupSwapChain() {
    m_swapchain = std::make_unique<Swapchain>(*m_device, *m_surface, *m_presentQueue);
  }

  void Renderer::transitionSwapChainImages() {
    CommandBuffer& transBuff = m_transferCmdPool->allocateCommandBuffer();
    transBuff.start(true);

    for(auto& image : m_swapchain->getImages()) {
      VkImageMemoryBarrier memBarrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        VK_ACCESS_MEMORY_WRITE_BIT,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
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

      vkCmdPipelineBarrier(transBuff,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &memBarrier);
    }

    transBuff.end();

    m_transferQueue->ref.submitOne(transBuff);
    m_transferQueue->ref.waitIdle();

    m_commandPool->freeCommandBuffer(transBuff);
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //// COMMAND POOLS & BUFFER SETUP
  /////////////////////////////////////////////////////////////////////////////

  void Renderer::setupCommandPools() {
    m_commandPool     = util::make_ptr<CommandPool>(*m_device, (*m_graphicsQueue)->getFamily());
    m_transferCmdPool = util::make_ptr<CommandPool>(*m_device, m_transferQueue->ref.getFamily());
  }

  void Renderer::setupCommandBuffers() {
    size_t imageCount = m_swapchain->getNumImages();
    m_commandBuffers.reserve(imageCount);
    for (size_t i = 0; i < imageCount; ++i) {
      m_commandBuffers.emplace_back(m_commandPool->allocateCommandBuffer());
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //// SPECIFIC SETUP /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //// RENDER STEPS & BACK-BUFFERS SETUP
  /////////////////////////////////////////////////////////////////////////////

  void Renderer::setupRenderSteps() {
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

  void Renderer::setupSwapChainFrameBuffers() const {
    m_swapchain->createFramebuffers(*m_renderPass);
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //// SHADER LOADING & DESCRIPTOR SET CREATION
  /////////////////////////////////////////////////////////////////////////////

  void Renderer::setupShaders() {
    m_triangleVertShader = util::make_ptr<Shader<ShaderStage::Vertex>>(ShaderModule::Load(*m_device,
                                                                                          "fromBuffer_transform_vert.spv"));
    m_triangleFragShader = util::make_ptr<Shader<ShaderStage::Fragment>>(ShaderModule::Load(*m_device,
                                                                                            "triangle_frag.spv"));
  }

  void Renderer::setupDescriptors() {
    // MVP matrices for object transformations
    VkDescriptorSetLayoutBinding mvpTransformLayoutBinding = {
      0,
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      1,
      VK_SHADER_STAGE_VERTEX_BIT,
      nullptr
    };

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      nullptr,
      0, // soon-to-be push descriptor
      1,
      &mvpTransformLayoutBinding
    };

    if (vkCreateDescriptorSetLayout(*m_device, &layoutCreateInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS || !
        m_descriptorSetLayout)
      throw std::runtime_error("Could not create descriptor set layout");

    // create uniform buffers - one for each swapchain image
    VkDeviceSize mvpTransformSize = sizeof(MVPTransformUniform);

    m_uniformBuffers.reserve(m_swapchain->getNumImages());

    for (auto& image : m_swapchain->getImages()) {
      m_uniformBuffers.push_back(Buffer::CreateUniform(*m_device, mvpTransformSize));
    }

    VkDescriptorPoolSize poolSize = {
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      m_swapchain->getNumImages()
    };

    VkDescriptorPoolCreateInfo poolCreateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      nullptr,
      0,
      m_swapchain->getNumImages(),
      1,
      &poolSize
    };

    if (vkCreateDescriptorPool(*m_device, &poolCreateInfo, nullptr, &m_descriptorPool) != VK_SUCCESS || !
        m_descriptorPool)
      throw std::runtime_error("Could not create descriptor pool");

    std::vector<VkDescriptorSetLayout> layouts(m_swapchain->getNumImages(), m_descriptorSetLayout);

    VkDescriptorSetAllocateInfo descSetAllocInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      nullptr,
      m_descriptorPool,
      m_swapchain->getNumImages(),
      layouts.data()
    };

    m_descriptorSets.resize(m_swapchain->getNumImages());
    if (vkAllocateDescriptorSets(*m_device, &descSetAllocInfo, m_descriptorSets.data()) != VK_SUCCESS)
      throw std::runtime_error("Could not allocate descriptor sets");

    // Descriptor sets are automatically freed once the pool is freed.
    // They can be individually freed if the pool was created with
    // VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT set.
    for (size_t i = 0; i < m_descriptorSets.size(); ++i) {
      VkDescriptorBufferInfo buffInfo = {
        m_uniformBuffers[i],
        0,
        sizeof(MVPTransformUniform)
      };

      VkWriteDescriptorSet descWrite = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        nullptr,
        m_descriptorSets[i],
        0,
        0,
        1,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        nullptr,
        &buffInfo,
        nullptr
      };

      vkUpdateDescriptorSets(*m_device, 1, &descWrite, 0, nullptr);
    }
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
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      nullptr,
      0,
      1,
      &m_descriptorSetLayout,
      0,
      nullptr
    };

    if(!m_descriptorSetLayout) {
      pipelineLayoutInfo.pSetLayouts = nullptr;
      pipelineLayoutInfo.setLayoutCount = 0;
    }

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
      nullptr, // You can create derivative pipelines, maybe this will be good later but imnotsure
      -1
    };

    vkCreateGraphicsPipelines(*m_device, nullptr, 1, &createInfo, nullptr, &m_graphicsPipeline);
    assert(m_graphicsPipeline);
  }
}
