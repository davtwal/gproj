// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : RenderPass.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 20d
// * Last Altered: 2019y 09m 20d
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

#ifndef DW_RENDER_PASS_H
#define DW_RENDER_PASS_H

#include "LogicalDevice.h"

namespace dw {
  CREATE_DEVICE_DEPENDENT(RenderPass)
  public:
    RenderPass(LogicalDevice& device);
    ~RenderPass();

    void startConstruction();
    void reserveAttachmentRefs(uint32_t num);
    void reserveAttachments(uint32_t num);
    void reserveSubpasses(uint32_t num);
    void reserveSubpassDependencies(uint32_t num);

    // SETUP FUNCTIONS
    // 0: Registering color attachments
    void addAttachment(VkAttachmentDescription const& desc);
    // 1: Setting up a subpass

    enum AttachmentRefFlags {
      arfColor = 1 << 0,

      arfResolve = 1 << 1,  // multi-sampling
      // must also have arfColor flagged. with an extension can also be used to resolve depth/stencil?

      arfDepthStencil = 1 << 2,
    };

    //All valid color attachments must have the same sample count
    //All valid color attachments must have a format whose features contain VK_FORMAT_FEATURE_COLOR_ATTACHMENT
    //All valid resolve attachments must have a format whose features contain VK_FORMAT_FEATURE_COLOR_ATTACHMENT
    //All valid resolve attachments must also be valid in the color attachment list
    //All valid depth-stencil attachments must have a format whose features contain VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    void addAttachmentRef(uint32_t index, VkImageLayout layout, AttachmentRefFlags refType);

    //All valid input attachments must have formats whose features contain at least one of VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT or VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    void addInputRef(uint32_t index, VkImageLayout layout);
    void addPreserveRef(uint32_t index);
    void finishSubpass();



    // 2: Finishing render pass
    // Dependency:
    // Subpass 0: The first subpass in the dependency
    // Subpass 1: The second subpass in the dependency. Must be >= subpass0.
    void addSubpassDependency(VkSubpassDependency const& dep);

    void finishRenderPass();

    NO_DISCARD bool isConstructed() const;

    operator VkRenderPass() const;

  private:
    void destroy();

    struct RenderPassConstruction {
      std::vector<VkAttachmentDescription> attachments;
      std::vector<VkSubpassDescription> subpasses;
      std::vector<VkSubpassDependency> dependencies;

      struct SubpassConstruction {
        std::vector<VkAttachmentReference> inputRefs,
          colorRefs,
          resolveRefs,
          depthStencilRefs;
        std::vector<uint32_t> preserveRefs;
      };

      std::vector<SubpassConstruction> sbc;
    };

    VkRenderPass m_pass { nullptr };
    RenderPassConstruction* m_conInf { nullptr };
  };
}

#endif
