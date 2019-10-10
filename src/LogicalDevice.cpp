// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : LogicalDevice.cpp
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

#include "LogicalDevice.h"
#include "Queue.h"
#include "CommandBuffer.h"
#include "Surface.h"

#include "Trace.h"

#include <cassert>

namespace dw {
  Queue LogicalDevice::m_badQueue = Queue(std::numeric_limits<uint32_t>::max());

  namespace {
    constexpr uint32_t NumFeatures = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);

    VkBool32 GetFeature(VkPhysicalDeviceFeatures const& f, uint32_t i) {
      return *(reinterpret_cast<const VkBool32*>(&f) + i);
    }
    VkBool32& GetFeature(VkPhysicalDeviceFeatures& f, uint32_t i) {
      return *(reinterpret_cast<VkBool32*>(&f) + i);
    }
  }

  LogicalDevice::~LogicalDevice() {
    for(auto& queueFam : m_queues) {
      for(auto& queue : queueFam ) {
        queue.stop(nullptr);
      }
    }

    vkDeviceWaitIdle(m_device);
    vkDestroyDevice(m_device, nullptr);
  }

  LogicalDevice::LogicalDevice(LogicalDevice&& o) noexcept
    : m_physical(o.m_physical),
      m_device(o.m_device),
      m_queues(std::move(o.m_queues)) {
  }

  LogicalDevice::LogicalDevice(PhysicalDevice&                 physical,
                               std::vector<const char*> const& layers,
                               std::vector<const char*> const& extensions,
                               QueueList const&                queues,
                               VkPhysicalDeviceFeatures        requiredFeatures,
                               VkPhysicalDeviceFeatures const& requestedFeatures,
                               bool                            useRequested)
    : m_physical(physical){
    Trace::Info << "Creating new logical device with " << queues.size() << " queue families in use " << Trace::Stop;
    
    // family, vector of priorities whose size is the number to make
    // reminder: queue priority is [0..1] where 1 is highest priority
    if (useRequested) {
      for (uint32_t i = 0; i < NumFeatures; ++i) {
        GetFeature(requiredFeatures, i) = GetFeature(requiredFeatures, i) ||
                                          GetFeature(requestedFeatures, i) && GetFeature(m_physical.m_features, i);
      }
    }

#ifdef _DEBUG
    for (auto& q : queues) {
      assert(m_physical.m_queueFamilies.size() > q.first);
      assert(m_physical.m_queueFamilies[q.first].queueCount >= q.second.size());
    }
#endif

    std::vector<VkDeviceQueueCreateInfo> devQCreates;
    devQCreates.resize(queues.size());
    for (uint32_t i = 0; i < queues.size(); ++i) {
      devQCreates[i] = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        nullptr,
        0,
        queues[i].first,
        static_cast<uint32_t>(queues[i].second.size()),
        queues[i].second.data()
      };
    }

    VkDeviceCreateInfo devCreate = {
      VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      nullptr,
      0,
      static_cast<uint32_t>(devQCreates.size()),
      devQCreates.data(),
      static_cast<uint32_t>(layers.size()),
      layers.data(),
      static_cast<uint32_t>(extensions.size()),
      extensions.data(),
      &requiredFeatures
    };

    VkResult res = vkCreateDevice(m_physical, &devCreate, nullptr, &m_device);
    if (!m_device) {
      Trace::Error << "ERROR: Could not create device #" << res << Trace::Stop;
      std::abort();
    }

    m_queues.resize(queues.size());
    for (uint32_t i = 0; i < queues.size(); ++i) {
      m_queues[i].reserve(queues[i].second.size());
      for (uint32_t j = 0; j < queues[i].second.size(); ++j) {
        m_queues[i].emplace_back(queues[i].first);
        vkGetDeviceQueue(m_device, queues[i].first, j, &m_queues[i][j].m_queue);
        m_queues[i][j].init(this);
      }
    }
  }

  Queue& LogicalDevice::getBestQueue(VkQueueFlagBits flag) {
    // find specialized first
    for(auto& qfam : m_queues) {
      for(auto& q : qfam) {
        if (!q.isLocked() && q.getFlags() == flag)
          return q;
      }
    }

    // find non-specialized if no specialized exists/are available
    for (auto& qfam : m_queues) {
      for (auto& q : qfam) {
        if (!q.isLocked() && (q.getFlags() & flag) == flag)
          return q;
      }
    }

    return m_badQueue;
  }

  Queue& LogicalDevice::getPresentableQueue(Surface& surface) {
    for (auto& qfam : m_queues) {
      for (auto& q : qfam) {
        if (!q.isLocked()) {
          VkBool32 supported = false;
          vkGetPhysicalDeviceSurfaceSupportKHR(getOwningPhysical(),
            q.getFamily(),
            surface,
            &supported);

          if(supported)
            return q;
        }
      }
    }

    return m_badQueue;
  }

}
