// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : LogicalDevice.h
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
#pragma once

#ifndef DW_LOGICAL_DEVICE_H
#define DW_LOGICAL_DEVICE_H

#include "PhysicalDevice.h"

#define DEVICE_DEPENDENT_FUNCTION(varName, x) \
  NO_DISCARD LogicalDevice& getOwningDevice() const {return (varName).getOwningDevice();} \
  PHYSICAL_DEPENDENT_FUNCTION(varName, x)

#define CREATE_DEVICE_DEPENDENT(x)                                                  \
  class x {                                                                         \
    LogicalDevice& m_device;                                                         \
  public:                                                                           \
    NO_DISCARD LogicalDevice& getOwningDevice() const {return m_device;}                        \
    PHYSICAL_DEPENDENT_FUNCTION(m_device, x) \
  private:

#define CREATE_DEVICE_DEPENDENT_INHERIT(x, ...)                                     \
  class x : __VA_ARGS__ {                                                           \
    LogicalDevice& m_device;                                                         \
  public:                                                                           \
    NO_DISCARD LogicalDevice& getOwningDevice() const {return m_device;}                        \
    PHYSICAL_DEPENDENT_FUNCTION(m_device, x) \
  private:

namespace dw {
  class CommandPool;
  class CommandBuffer;
  class Queue;

  CREATE_PHYSICAL_DEPENDENT(LogicalDevice)
  public:
    using QueueList = std::vector<std::pair<uint32_t, std::vector<float>>>;

    /******************************************!
     * \param physical
     *    The physical device that owns this logical device
     *
     * \param layers
     *    The names of layers to enable. Layers can be found by
     *    checking in the physical device.
     *
     * \param extensions
     *    The names of extensions to enable. Extensions can also be
     *    found by checking in the physical device.
     *
     * \param queues
     *    A vector of information about creation of queues.
     *    In the pair:
     *        uint32_t        : The queue family index
     *        vector<float>   : Vector of queue priorities. The size of this vector
     *                          is taken to be the number of queues to create for this
     *                          family. Each priority should be [0..1] where 1 is high.
     *
     * \param requiredFeatures
     *    The features that absolutely need to be enabled for the device
     *    to properly function.
     *
     * \param requestedFeatures
     *    The features that will be taken advantage of of they are available
     *    but are not needed to function.
     *
     * \param useRequested
     *    If this is false, then requestedFeatures will be ignored, skipping
     *    a for-loop. Set to false if there are no optional features.
     */
    LogicalDevice(PhysicalDevice&                 physical,
                  std::vector<const char*> const& layers,
                  std::vector<const char*> const& extensions,
                  QueueList const&                queues,
                  VkPhysicalDeviceFeatures        requiredFeatures,
                  VkPhysicalDeviceFeatures const& requestedFeatures,
                  bool                            useRequested = true);

    ~LogicalDevice();

    operator VkDevice() const {
      return m_device;
    }

    NO_DISCARD Queue& getBestQueue(VkQueueFlagBits flag);

    //NO_DISCARD CommandPool* createCommandPool(uint32_t queueFamilyIndex,
    //                                          bool     indivCmdBfrResetable = true,
    //                                          bool     frequentRecording    = false) const;

  private:
    VkDevice                        m_device;
    std::vector<std::vector<Queue>> m_queues;
    static Queue m_badQueue;
    //uint32_t                        m_graphicsQFamily;
    //uint32_t                        m_presentQFamily;
    //uint32_t                        m_transferQFamily;
    //uint32_t                        m_computeQFamily;
    //uint32_t                        m_sparseBidingQFamily;
  };
}

#endif
