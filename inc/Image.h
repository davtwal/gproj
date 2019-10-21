// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : Image.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 16d
// * Last Altered: 2019y 09m 16d
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

#ifndef DW_VK_IMAGE_H
#define DW_VK_IMAGE_H

#include "LogicalDevice.h"

namespace dw {
  class DependentImage;
  class IndependentImage;

  CREATE_DEVICE_DEPENDENT(ImageView)
  public:
    ImageView(LogicalDevice& device, VkImageView v = nullptr);
    ~ImageView();

    operator VkImageView() const {
      return m_view;
    }

  private:
    friend class Image;
    VkImageView m_view{nullptr};
  };

  class Image {
  public:
    explicit Image(VkImage         i         = nullptr,
                   VkFormat        format    = VK_FORMAT_BEGIN_RANGE,
                   VkImageViewType type      = VK_IMAGE_VIEW_TYPE_BEGIN_RANGE,
                   bool            isMutable = false);

    virtual ~Image() = default;

    Image(Image const&) = delete;
    Image& operator=(Image const&) = delete;

    Image(Image&& o) noexcept;

    static DependentImage Load(LogicalDevice& device, std::string const& filename, uint32_t maxMipLevels = 100);

    // Views
    NO_DISCARD ImageView createView(VkImageAspectFlags        readAspect      = VK_IMAGE_ASPECT_COLOR_BIT,
                                    uint32_t                  startMipLevel   = 0,
                                    uint32_t                  mipLevelCount   = 1,
                                    uint32_t                  startArrayIndex = 0,
                                    uint32_t                  arrayImageCount = 1,
                                    VkComponentMapping const& mapping         = {}) const;

    NO_DISCARD ImageView createView(VkFormat                  format,
                                    VkImageAspectFlags        readAspect      = VK_IMAGE_ASPECT_COLOR_BIT,
                                    uint32_t                  startMipLevel   = 0,
                                    uint32_t                  mipLevelCount   = 1,
                                    uint32_t                  startArrayIndex = 0,
                                    uint32_t                  arrayImageCount = 1,
                                    VkComponentMapping const& mapping         = {}) const;

    NO_DISCARD ImageView createView(VkImageViewType           type,
                                    VkImageAspectFlags        readAspect      = VK_IMAGE_ASPECT_COLOR_BIT,
                                    uint32_t                  startMipLevel   = 0,
                                    uint32_t                  mipLevelCount   = 1,
                                    uint32_t                  startArrayIndex = 0,
                                    uint32_t                  arrayImageCount = 1,
                                    VkComponentMapping const& mapping         = {}) const;

    NO_DISCARD ImageView createView(VkFormat                  format,
                                    VkImageViewType           type            = VK_IMAGE_VIEW_TYPE_2D,
                                    VkImageAspectFlags        readAspect      = VK_IMAGE_ASPECT_COLOR_BIT,
                                    uint32_t                  startMipLevel   = 0,
                                    uint32_t                  mipLevelCount   = 1,
                                    uint32_t                  startArrayIndex = 0,
                                    uint32_t                  arrayImageCount = 1,
                                    VkComponentMapping const& mapping         = {}) const;

    operator VkImage() const;
    
    void initImage(VkImageType       type,     // 1D, 2D, or 3D
                   VkImageViewType   intendedViewType,
                   VkFormat          format,      // format
                   VkExtent3D        extent,    // sizes             
                   VkImageUsageFlags usage, // usage
                   uint32_t          mipLevels,   // mip levels. if this is >1, extents must be a power of 2
                   uint32_t          arrayLayers, // array size
                   bool              isMutable,       // can create views of different format
                   bool              isCubemap,       // enables cubemap / cubemap arrays
                   bool              is2DArray,       // enables 2D and 2D array
                   bool              isMappable       // images will be laid out in linear format if mappable);
    );

    NO_DISCARD VkFormat getFormat() const;
    NO_DISCARD bool isMutable() const;

    NO_DISCARD VkExtent2D getSize() const;
    NO_DISCARD VkExtent3D getExtent() const;

    NO_DISCARD
    VkAttachmentDescription getAttachmentDesc(VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                                              VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                              VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                              VkImageLayout initLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                              VkAttachmentLoadOp stencilLoad = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                              VkAttachmentStoreOp stencilStore = VK_ATTACHMENT_STORE_OP_DONT_CARE) const;

  protected:

    NO_DISCARD virtual LogicalDevice& getDevice() const = 0;

    VkExtent3D      m_extent;
    VkImage         m_image{nullptr};
    VkFormat        m_format;
    VkImageViewType m_type;
    bool            m_mutable{false};
  };

  // a independent image is an image that is independent of a logical device,
  // and as such whoever creates a 'IndependentImage' is responsible for
  // calling vkDestroyImage. This is used by things like the swapchain.
  // The optional device is the device that will be used to create things
  // such as image views.
  class IndependentImage : public Image {
  public:
    IndependentImage(VkImage         image,
                     VkFormat        format,
                     VkImageViewType type,
                     LogicalDevice*  device    = nullptr,
                     bool            isMutable = false);
    // does not override destructor

    IndependentImage& setDevice(LogicalDevice* device);

  private:
    NO_DISCARD LogicalDevice& getDevice() const override;

    LogicalDevice* m_devicePointer;
  };

  class MemoryAllocator;

  // a dependent image is dependent on a logical device, and destroys itself
  // using the destructor.
  CREATE_DEVICE_DEPENDENT_INHERIT(DependentImage, public Image)
  public:
    DependentImage(LogicalDevice& device);

    // overrides destructor to call vkDestroyImage
    ~DependentImage() override;

    void back(MemoryAllocator& allocator, VkMemoryPropertyFlags memFlags);

  protected:
    NO_DISCARD LogicalDevice& getDevice() const override;

    VkDeviceMemory m_memory{ nullptr };
    VkDeviceSize m_memSize{ 0 };
  };
}

#endif
