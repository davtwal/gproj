// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : Queue.h
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

#ifndef DW_VK_QUEUE_H
#define DW_VK_QUEUE_H

#include "LogicalDevice.h"
#include "util/Utils.h"

namespace dw {
  class CommandBuffer;

  class Queue {
  public:
    Queue(uint32_t family);

    operator VkQueue() const;

    // wait until the queue is idle
    void waitIdle();

    // waits until the current submission is done
    void waitSubmit();

    void submitOne(CommandBuffer const&                     buffer,
                   std::vector<VkSemaphore> const&          waitSemaphores   = {},
                   std::vector<VkPipelineStageFlags> const& waitSemStages    = {},
                   std::vector<VkSemaphore> const&          signalSemaphores = {});

    void submitOne(VkSubmitInfo const& info);
    void submitOne(VkSubmitInfo const& info, VkFence fence);

    // this is kinda slow as i need to make a new vector of just
    // VkCommandBuffers in order to build a single submit info
    //void submitMulti(std::vector<CommandBuffer*> const& buffers);

    void submitMulti(std::vector<VkSubmitInfo> const& infos);

    //void present();

    NO_DISCARD VkQueueFlags getFlags() const;
    NO_DISCARD uint32_t     getFamily() const;
    NO_DISCARD bool         isLocked() const;
    NO_DISCARD bool         isValid() const;

    NO_DISCARD bool isSubmitting() const;
    NO_DISCARD bool isSubmitFinished();

    // locks usage so the logical device cannot give out this queue
    // anymore to be used by anything else
    void lockUsage();
    void unlockUsage();

  private:
    friend class LogicalDevice;
    void init(LogicalDevice* dev);
    void stop(const VkAllocationCallbacks* callbacks = nullptr);
    void reset();

    LogicalDevice* m_device{nullptr};
    VkQueue  m_queue{nullptr};
    VkFence  m_submitFence{nullptr};
    uint32_t m_family{std::numeric_limits<uint32_t>::max()};
    bool     m_locked{false};
    bool     m_submitting{false};
  };
}

#endif
