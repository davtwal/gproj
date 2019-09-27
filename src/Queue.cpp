// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : Queue.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 15d
// * Last Altered: 2019y 09m 15d
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

#include "Queue.h"
#include "CommandBuffer.h"

#include <stdexcept>
#include "Swapchain.h"
#include "Utils.h"
#include <cassert>

namespace dw {
  Queue::Queue(uint32_t family)
    : m_family(family) {
  }

  Queue::operator VkQueue() const {
    return m_queue;
  }

  VkQueueFlags Queue::getFlags() const {
    return m_device->getOwningPhysical().getQueueFamilyProperties()[m_family].queueFlags;
  }

  uint32_t Queue::getFamily() const {
    return m_family;
  }

  bool Queue::isLocked() const {
    return m_locked;
  }

  bool Queue::isValid() const {
    return m_family != std::numeric_limits<uint32_t>::max();
  }

  void Queue::lockUsage() {
    m_locked = true;
  }

  void Queue::unlockUsage() {
    m_locked = false;
  }

  void Queue::init(LogicalDevice* dev) {
    m_device = dev;

    if (!m_device)
      throw std::runtime_error("QUEUE DOES NOT HAVE OWNING LOGICAL DEVICE");

    VkFenceCreateInfo fenceInfo = {
      VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      nullptr,
      0
    };

    vkCreateFence(*m_device, &fenceInfo, nullptr, &m_submitFence);
  }

  void Queue::stop(const VkAllocationCallbacks* callbacks) {
    if (!m_device)
      throw std::runtime_error("QUEUE DOES NOT HAVE OWNING LOGICAL DEVICE");

    if (m_submitFence) {
      vkDestroyFence(*m_device, m_submitFence, callbacks);
      m_submitFence = nullptr;
    }
  }

  bool Queue::isSubmitting() const {
    return m_submitting;
  }

  bool Queue::isSubmitFinished() {
    if (!m_device)
      throw std::runtime_error("QUEUE DOES NOT HAVE OWNING LOGICAL DEVICE");

    if (m_submitFence && vkGetFenceStatus(*m_device, m_submitFence) == VK_SUCCESS) {
      reset();
      return true;
    }
    return false;
  }

#if 0
  void Queue::submitMulti(std::vector<CommandBuffer*> const& buffers) {
    if (!m_queue || m_submitting)
      return;

    std::vector<VkCommandBuffer> vkBuffs;
    vkBuffs.resize(buffers.size());
    for (CommandBuffer* buff : buffers) {
      if (buff && buff->isReady())
        vkBuffs.emplace_back(buff->m_cmdBuffer);
    }

    VkSubmitInfo submitInfo = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO,
      nullptr,
      0,
      nullptr,
      nullptr,
      static_cast<uint32_t>(vkBuffs.size()),
      vkBuffs.data(),
      0,
      nullptr
    };

    return submitOne(submitInfo);
  }
#endif

  void Queue::submitMulti(std::vector<VkSubmitInfo> const& infos) {
    if (!m_queue || m_submitting)
      return;

    if (vkQueueSubmit(m_queue, infos.size(), infos.data(), m_submitFence) != VK_SUCCESS)
      throw std::runtime_error("Could not multi-submit queue");

    m_submitting = true;
  }

  void Queue::submitOne(VkSubmitInfo const& info, VkFence fence) {
    if (!m_queue || m_submitting)
      return;

    if (vkQueueSubmit(m_queue, 1, &info, fence) != VK_SUCCESS)
      throw std::runtime_error("Could not submit queue");

    m_submitting = true;
  }


  void Queue::submitOne(CommandBuffer const&                     buffer,
                        std::vector<VkSemaphore> const&          waitSemaphores,
                        std::vector<VkPipelineStageFlags> const& waitSemStages,
                        std::vector<VkSemaphore> const&          signalSemaphores
  ) {
    if (!m_queue || m_submitting || !buffer.isReady())
      return;

    assert(waitSemaphores.size() == waitSemStages.size());

    VkSubmitInfo submitInfo = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO,
      nullptr,
      static_cast<uint32_t>(waitSemaphores.size()),
      waitSemaphores.data(),
      waitSemStages.data(),
      1,
      &(buffer.m_cmdBuffer),
      static_cast<uint32_t>(signalSemaphores.size()),
      signalSemaphores.data()
    };

    return submitOne(submitInfo);
  }

  void Queue::submitOne(VkSubmitInfo const& info) {
    submitOne(info, m_submitFence);
  }

  void Queue::waitSubmit() {
    if (!m_device)
      throw std::runtime_error("QUEUE DOES NOT HAVE OWNING LOGICAL DEVICE");

    if (m_submitFence && m_submitting) {
      vkWaitForFences(*m_device, 1, &m_submitFence, true, 100);

      if (vkGetFenceStatus(*m_device, m_submitFence) == VK_SUCCESS)
        reset();
    }
  }

  void Queue::waitIdle() {
    if (!m_device)
      throw std::runtime_error("QUEUE DOES NOT HAVE OWNING LOGICAL DEVICE");

    assert(m_queue);

    vkQueueWaitIdle(m_queue);

    if (m_submitFence) {
      if (vkGetFenceStatus(*m_device, m_submitFence) == VK_SUCCESS)
        reset();
    }
  }

  void Queue::reset() {
    if (!m_device)
      throw std::runtime_error("QUEUE DOES NOT HAVE OWNING LOGICAL DEVICE");

    if (m_submitFence) {
      m_submitting = false;
      vkResetFences(*m_device, 1, &m_submitFence);
    }
  }

  //void Queue::present(Swapchain const& swapchain) {
  //VkPresentInfoKHR vk_present_info = {
  //  VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
  //  nullptr,
  //  0,
  //  nullptr,
  //  1,
  //  &swapchain,
  //  ,
  //  &present_result
  //};
  //
  ////vkWaitForFences(vk_device, 1, &vk_acquire_next_fence, true, ~0ull);
  //vkQueuePresentKHR(m_queue, &vk_present_info);
  //}


}
