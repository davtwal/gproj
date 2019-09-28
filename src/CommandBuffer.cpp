// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : CommandBuffer.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 19d
// * Last Altered: 2019y 09m 19d
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

#include "CommandBuffer.h"
#include "Queue.h"

#include <stdexcept>

namespace dw {
  CommandPool::CommandPool(LogicalDevice& device,
                           uint32_t       queueFamilyIndex,
                           bool           indivCmdBfrResetable,
                           bool           frequentRecording)
    : m_device(device),
      m_pool(nullptr),
      m_qFamily(queueFamilyIndex),
      m_individualReset(indivCmdBfrResetable) {

    VkCommandPoolCreateFlags flags = (m_individualReset ? VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : 0) | (
                                       frequentRecording ? VK_COMMAND_POOL_CREATE_TRANSIENT_BIT : 0);

    VkCommandPoolCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      nullptr,
      flags,
      queueFamilyIndex
    };

    vkCreateCommandPool(m_device, &createInfo, nullptr, &m_pool);

    if (!m_pool)
      throw std::bad_alloc();
  }

  CommandPool::~CommandPool() {
    if (m_pool) {
      for (CommandBuffer& buff : m_commandBuffers) {
        if (buff.m_cmdBuffer) {
          vkFreeCommandBuffers(m_device, m_pool, 1, &buff.m_cmdBuffer);
          buff.m_cmdBuffer = nullptr;
        }
      }

      vkDestroyCommandPool(m_device, m_pool, nullptr);
    }
  }

  bool CommandPool::resetPool(bool release) {
    if (m_pool) {
      for (CommandBuffer& buff : m_commandBuffers)
        if (buff.m_state == CommandBuffer::State::Pending)
          return false;

      vkResetCommandPool(m_device, m_pool, release ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0);

      for (CommandBuffer& buff : m_commandBuffers)
        buff.m_state = CommandBuffer::State::Fresh;

      return true;
    }
    return false;
  }

  CommandBuffer& CommandPool::allocateCommandBuffer() {
    VkCommandBufferAllocateInfo allocInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      nullptr,
      m_pool,
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, // TODO for secondary
      1
    };

    VkCommandBuffer buff;
    vkAllocateCommandBuffers(m_device, &allocInfo, &buff);
    if (!buff)
      throw std::bad_alloc();

    return m_commandBuffers.emplace_back(*this, buff);
  }

  void CommandPool::freeCommandBuffer(CommandBuffer& buffer) {
    if (operator==(buffer.m_pool) && buffer.m_cmdBuffer) {
      vkFreeCommandBuffers(m_device, m_pool, 1, &buffer.m_cmdBuffer);
      buffer.m_cmdBuffer = nullptr;

      for (auto i = m_commandBuffers.begin(); i != m_commandBuffers.end(); ++i) {
        if ((*i) == buffer) {
          m_commandBuffers.erase(i);
          return;
        }
      }
    }
  }

  CommandPool::operator VkCommandPool() const {
    return m_pool;
  }

  bool CommandPool::supportsIndividualReset() const {
    return m_individualReset;
  }


  ///////////////
  // COMMAND BUFFER
  CommandBuffer::CommandBuffer(CommandPool& pool, VkCommandBuffer buff)
    : m_pool(pool),
      m_cmdBuffer(buff) {
  }

  CommandBuffer::~CommandBuffer() {
    //if (m_cmdBuffer)
    //m_pool.freeCommandBuffer(*this);
  }

  void CommandBuffer::reset(bool release) {
    if (m_pool.supportsIndividualReset() && m_cmdBuffer) {
      vkResetCommandBuffer(m_cmdBuffer, release ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT : 0);
      m_state = State::Fresh;
    }
  }

  bool CommandBuffer::isFresh() const {
    return m_cmdBuffer && m_state == State::Fresh;
  }


  bool CommandBuffer::canBeReset() const {
    return m_pool.supportsIndividualReset();
  }

  bool CommandBuffer::isStarted() const {
    return m_state == State::Recording || isInRenderPass();
  }

  bool CommandBuffer::isInRenderPass() const {
    return m_state == State::RenderPass;
  }

  bool CommandBuffer::isReady() const {
    return m_state == State::Executable;
  }

  bool CommandBuffer::isPending() const {
    return m_state == State::Pending;
  }

  CommandBuffer::operator VkCommandBuffer() const {
    return m_cmdBuffer;
  }

  void CommandBuffer::start(bool oneTime) {
    // TODO support secondary command buffers
    if (m_cmdBuffer && m_state == State::Fresh) {

      const VkFlags beginFlags = (oneTime ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

      VkCommandBufferBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        beginFlags,
        nullptr // TODO for secondary buffers
      };

      if (vkBeginCommandBuffer(m_cmdBuffer, &beginInfo) != VK_SUCCESS)
        throw std::runtime_error("Could not begin command buffer");

      m_state = State::Recording;
    }
  }

  void CommandBuffer::startRenderpass() {
    if (m_cmdBuffer && m_state == State::Recording) {

      VkRenderPassBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        nullptr,

      };

      vkCmdBeginRenderPass(m_cmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

      m_state = State::RenderPass;
    }
  }

  void CommandBuffer::endRenderpass() {
    if (m_cmdBuffer && isInRenderPass()) {

      //vkCmdEndRenderPass(m_cmdBuffer);

      m_state = State::Recording;
    }
  }

  void CommandBuffer::end() {
    if (m_cmdBuffer && isStarted()) {
      if (isInRenderPass()) {
        endRenderpass();
      }

      vkEndCommandBuffer(m_cmdBuffer);

      m_state = State::Executable;
    }
  }

  VkSubmitInfo CommandBuffer::getSubmitInfo() const {
    VkSubmitInfo ret = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO,
      nullptr,
      0,
      nullptr,
      nullptr,
      1,
      &m_cmdBuffer,
      0,
      nullptr
    };

    return ret;
  }

  void CommandBuffer::submit(Queue& q) {
    if(isReady()) {
      q.submitOne(*this);
    }
  }
}
