// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : CommandBuffer.h
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

#ifndef DW_COMMAND_BUFFER_H
#define DW_COMMAND_BUFFER_H

#include "LogicalDevice.h"
#include <list>
#include "Utils.h"

namespace dw {
  class CommandBuffer;

  CREATE_DEVICE_DEPENDENT(CommandPool)
  public:
    CommandPool(LogicalDevice& device,
                uint32_t       queueFamilyIndex,
                bool           indivCmdBfrResetable = true,
                bool           frequentRecording    = false);

    ~CommandPool();

    NO_DISCARD CommandBuffer& allocateCommandBuffer();

    void freeCommandBuffer(CommandBuffer& buffer);
    operator VkCommandPool() const;

    NO_DISCARD bool resetPool(bool release = true);
    NO_DISCARD bool supportsIndividualReset() const;

    bool operator==(CommandPool const& rhs) const {
      return m_pool == rhs.m_pool;
    }
    
  private:
    VkCommandPool m_pool;
    uint32_t      m_qFamily;
    bool          m_individualReset;

    std::list<CommandBuffer> m_commandBuffers;
  };

  using CmdBuffContainer = util::Ref<CommandBuffer>;

  class CommandBuffer {
  public:
    CommandBuffer(CommandPool& pool, VkCommandBuffer buff = nullptr);
    ~CommandBuffer();

    operator VkCommandBuffer() const;

    NO_DISCARD bool canBeReset() const;      // if the individual cmd buffer can be reset
    NO_DISCARD bool isFresh() const;
    NO_DISCARD bool isStarted() const;      // if the cmd buffer has been 'begun'
    NO_DISCARD bool isInRenderPass() const;  // if the cmd buffer has started a renderpass
    NO_DISCARD bool isReady() const;
    NO_DISCARD bool isPending() const;

    void reset(bool release = true);        // can only be called if the owning pool supports
    void start(bool oneTime);
    void startRenderpass();
    void endRenderpass();
    void end();

    // singular submit to a queue.
    void submit(Queue& q);

    // gets the submit info for the queue,
    // meant for submission of multiple submit infos
    NO_DISCARD VkSubmitInfo getSubmitInfo() const;

    bool operator==(CommandBuffer const& o) const {
      return m_cmdBuffer == o.m_cmdBuffer && m_pool == o.m_pool;
    }

    CommandBuffer& operator=(CommandBuffer const&) = delete;
    CommandBuffer(CommandBuffer const& o)          = delete;

    CommandPool& getOwningPool() const {
      return m_pool;
    }

  private:
    friend class CommandPool;
    friend class Queue;

    CommandPool&    m_pool;
    VkCommandBuffer m_cmdBuffer;

    enum class State {
      Fresh,
      Recording,
      RenderPass,
      Executable,
      Pending
    };

    State m_state{State::Fresh};
  };
}

#endif
