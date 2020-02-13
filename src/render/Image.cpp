// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : Image.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 17d
// * Last Altered: 2019y 09m 17d
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

#include "render/Image.h"
#include "render/MemoryAllocator.h"

#include <cassert>
#include "util/Trace.h"
#include "util/Utils.h"

namespace dw {
  ImageView::ImageView(LogicalDevice& device, VkImageView v)
    : m_device(device),
      m_view(v) {

  }

  ImageView::~ImageView() {
    if (m_view) {
      vkDestroyImageView(m_device, m_view, nullptr);
      m_view = nullptr;
    }
  }

  ImageView::ImageView(ImageView&& o) noexcept
    : m_device(o.m_device),
      m_view(std::move(o.m_view)) {
    o.m_view = nullptr;
  }

  Image::Image(VkImage i, VkFormat format, VkImageViewType type, bool isMutable)
    : m_extent({0, 0, 0}),
      m_image(i),
      m_format(format),
      m_type(type),
      m_mutable(isMutable) {
  }

  Image::Image(Image&& o) noexcept
    : m_extent(o.m_extent),
      m_image(o.m_image),
      m_format(o.m_format),
      m_type(o.m_type),
      m_mipLevels(o.m_mipLevels),
      m_mutable(o.m_mutable) {
  }

  Image::operator VkImage() const {
    return m_image;
  }

  VkFormat Image::getFormat() const {
    return m_format;
  }

  bool Image::isMutable() const {
    return m_mutable;
  }

  VkExtent3D Image::getExtent() const {
    return m_extent;
  }

  VkExtent2D Image::getSize() const {
    return { m_extent.width, m_extent.height };
  }

  VkAttachmentDescription Image::getAttachmentDesc(VkAttachmentLoadOp loadOp,
                                                   VkAttachmentStoreOp storeOp,
                                                   VkImageLayout finalLayout,
                                                   VkImageLayout initLayout,
                                                   VkAttachmentLoadOp stencilLoad,
                                                   VkAttachmentStoreOp stencilStore) const {
    VkAttachmentDescription ret = {
      0,
      m_format,
      VK_SAMPLE_COUNT_1_BIT, // TODO: Multisampling support
      loadOp,
      storeOp,
      stencilLoad,
      stencilStore,
      initLayout,
      finalLayout
    };

    return ret;
  }

  void Image::initImage(
    VkImageType       type,     // 1D, 2D, or 3D
    VkImageViewType   intendedViewType,
    VkFormat          format,      // format
    VkExtent3D        extent,    // sizes             
    VkImageUsageFlags usage, // usage
    uint32_t          mipLevels,   // mip levels. if this is >1, extents must be a power of 2
    uint32_t          arrayLayers, // array size
    bool              isMutable,       // can create views of different format
    bool              isCubemap,       // enables cubemap / cubemap arrays
    bool              is2DArray,       // enables 2D and 2D array
    bool              isMappable       // images will be laid out in linear format if mappable
  ) {
    assert(extent.width > 0);
    assert(extent.height > 0);
    assert(extent.depth > 0);
    assert(mipLevels > 0);
    assert(arrayLayers > 0);

#ifdef _DEBUG
    // check the image against the limits
    auto& limits = getDevice().getOwningPhysical().getLimits();
    if (arrayLayers > limits.maxImageArrayLayers
        || extent.width > limits.maxImageDimension1D
        || extent.width > limits.maxImageDimension2D
        || extent.width > limits.maxImageDimension3D
        || extent.width > limits.maxImageDimensionCube
        || extent.height > limits.maxImageDimension2D
        || extent.height > limits.maxImageDimension3D
        || extent.height > limits.maxImageDimensionCube
        || extent.depth > limits.maxImageDimension3D) {
      return;
    }
#endif

    if (!getDevice())
      return;

    if (is2DArray) {
      if (isCubemap
          || type != VK_IMAGE_TYPE_3D)
        return;
    }

    // Asses limitations
    if (isCubemap) {
      if (type != VK_IMAGE_TYPE_2D
          || arrayLayers % 6 != 0) {
        Trace::Error << "Attempted to create invalid cubemap image" << Trace::Stop;
        return;
      }
    }

    // TODO: check if this check is still needed
    //if (mipLevels > 1) {
    //  // check for powers of two
    //  if ((extent.depth & (extent.depth - 1)) != 0
    //      || (extent.height & (extent.height - 1)) != 0
    //      || (extent.width & (extent.width - 1)) != 0) {
    //    Trace::Error << "Attempted to create an image with >1 mip levels but not a power of two image" << Trace::Stop;
    //    return;
    //  }
    //}

    /* From the vulkan spec:
      Creation of images with tiling VK_IMAGE_TILING_LINEAR may not be supported unless
      other parameters meet all of the constraints :
      - imageType is VK_IMAGE_TYPE_2D
      - format is not a depth / stencil format
      - mipLevels is 1
      - arrayLayers is 1
      - samples is VK_SAMPLE_COUNT_1_BIT
      - usage only includes VK_IMAGE_USAGE_TRANSFER_SRC_BIT and /or VK_IMAGE_USAGE_TRANSFER_DST_BIT
    */
    if (isMappable) {
      if (type != VK_IMAGE_TYPE_2D
          || util::IsFormatDepthOrStencil(format)
          || mipLevels != 1
          || arrayLayers != 1
          // sampleCount != VK_SAMPLE_COUNT_1_BIT
          || (usage & ~VK_IMAGE_USAGE_TRANSFER_DST_BIT & ~VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0) {
        Trace::Error << "Attempted to create invalid mappable image" << Trace::Stop;
        return;
      }
    }

    // TODO support multi-plane formats e.g. YCBCR conversion
    // TODO support compressed formats
    // TODO allow for multisampling

    // maybe TODO support sharing mode concurrent images

    VkFlags flags = (isMutable ? VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT : 0)
                    | (isCubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0)
                    | (is2DArray ? VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT : 0);

    VkImageTiling tiling = isMappable ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;

    VkImageFormatProperties properties;
    vkGetPhysicalDeviceImageFormatProperties(getDevice().getOwningPhysical(), format, type, tiling, usage, flags, &properties);

    VkImageCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      nullptr,
      flags,
      type,
      format,
      extent,
      mipLevels,
      arrayLayers,
      VK_SAMPLE_COUNT_1_BIT,
      tiling,
      usage,
      VK_SHARING_MODE_EXCLUSIVE,
      VK_QUEUE_FAMILY_IGNORED,
      nullptr,
      VK_IMAGE_LAYOUT_UNDEFINED
    };

    vkCreateImage(getDevice(), &createInfo, nullptr, &m_image);

    if (!m_image)
      throw std::runtime_error("could not create image");

    m_format = format;
    m_mutable = isMutable;
    m_type = intendedViewType;
    m_mipLevels = mipLevels;
    m_extent = extent;
  }

  // CREATE VIEWS
  ImageView Image::createView(VkImageAspectFlags        readAspect,
                              uint32_t                  startMipLevel,
                              uint32_t                  mipLevelCount,
                              uint32_t                  startArrayIndex,
                              uint32_t                  arrayImageCount,
                              VkComponentMapping const& mapping) const {
    return createView(m_format,
                      m_type,
                      readAspect,
                      startMipLevel,
                      mipLevelCount,
                      startArrayIndex,
                      arrayImageCount,
                      mapping);
  }


  ImageView Image::createView(VkFormat                  format,
                              VkImageAspectFlags        readAspect,
                              uint32_t                  startMipLevel,
                              uint32_t                  mipLevelCount,
                              uint32_t                  startArrayIndex,
                              uint32_t                  arrayImageCount,
                              VkComponentMapping const& mapping) const {
    return createView(format,
                      m_type,
                      readAspect,
                      startMipLevel,
                      mipLevelCount,
                      startArrayIndex,
                      arrayImageCount,
                      mapping);
  }

  ImageView Image::createView(VkImageViewType           type,
                              VkImageAspectFlags        readAspect,
                              uint32_t                  startMipLevel,
                              uint32_t                  mipLevelCount,
                              uint32_t                  startArrayIndex,
                              uint32_t                  arrayImageCount,
                              VkComponentMapping const& mapping) const {
    return createView(m_format,
                      type,
                      readAspect,
                      startMipLevel,
                      mipLevelCount,
                      startArrayIndex,
                      arrayImageCount,
                      mapping);
  }


  ImageView Image::createView(VkFormat                  format,
                              VkImageViewType           type,
                              VkImageAspectFlags        readAspect,
                              uint32_t                  startMipLevel,
                              uint32_t                  mipLevelCount,
                              uint32_t                  startArrayIndex,
                              uint32_t                  arrayImageCount,
                              VkComponentMapping const& mapping) const {

    // Format: The format must be compatible with the format of the image.
    // In general: The number of bits per pixel should match
    // If neither format is compressed  : The bits per texel must match
    // If both formats are compressed   : The bits per block must match
    // If only one format is compressed :
    //  The bits per block of the compressed must match the bits per texel of the uncompressed
    if (!getDevice() || util::BitsPerPixelFormat(format) != util::BitsPerPixelFormat(m_format))
      return ImageView(getDevice());

    // Aspect:
    //  Refers to a specific part of the image. You cannot create a view with
    //  multiple aspects, such as reading both depth and stencil.

    if (mipLevelCount == ALL_MIP_LEVELS)
      mipLevelCount = m_mipLevels - startMipLevel;

    assert(mipLevelCount + startMipLevel <= m_mipLevels);

    VkImageViewCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      nullptr,
      0,
      m_image,
      type,
      format,
      mapping,
      {
        readAspect,
        startMipLevel,
        mipLevelCount,
        startArrayIndex,
        arrayImageCount
      }
    };

    VkImageView view;
    vkCreateImageView(getDevice(), &createInfo, nullptr, &view);

    if (!view)
      throw std::runtime_error("could not create image view");

    return ImageView(getDevice(), view);
  }

  // INDEPENDENT IMAGE

  IndependentImage::IndependentImage(VkImage         image,
                                     VkFormat        format,
                                     VkImageViewType type,
                                     LogicalDevice*  device,
                                     bool            isMutable)
    : Image(image, format, type, isMutable),
      m_devicePointer(device) {
  }


  IndependentImage& IndependentImage::setDevice(LogicalDevice* device) {
    m_devicePointer = device;
    return *this;
  }

  LogicalDevice& IndependentImage::getDevice() const {
    return *m_devicePointer;
  }

  uint32_t Image::getMipLevels() const {
    return m_mipLevels;
  }

  // DEPENDENT IMAGE

  DependentImage::DependentImage(LogicalDevice& device)
    : m_device(device) {
  }

  DependentImage::DependentImage(DependentImage&& o) noexcept
    : Image(o.m_image, o.m_format, o.m_type, o.m_mutable), m_device(o.m_device), m_memory(o.m_memory), m_memSize(o.m_memSize) {
    o.m_image = nullptr;
    o.m_memory = nullptr;
  }

  DependentImage::~DependentImage() {
    if (m_image)
      vkDestroyImage(getOwningDevice(), m_image, nullptr);

    if (m_memory)
      vkFreeMemory(getOwningDevice(), m_memory, nullptr);
  }

  LogicalDevice& DependentImage::getDevice() const {
    return getOwningDevice();
  }

  void DependentImage::back(MemoryAllocator& allocator, VkMemoryPropertyFlags memFlags) {
    if (!m_image) return;

    VkMemoryRequirements requirements = {};
    vkGetImageMemoryRequirements(m_device, m_image, &requirements);

    VkMemoryAllocateInfo allocInfo = {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      nullptr,
      requirements.size,
      allocator.GetAppropriateMemType(requirements.memoryTypeBits, memFlags)
    };

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS || !m_memory)
      throw std::runtime_error("could not allocate image memory");

    m_memSize = requirements.size;

    if (vkBindImageMemory(m_device, m_image, m_memory, 0) != VK_SUCCESS)
      throw std::runtime_error("Could not bind device memory to image");
  }

  void* DependentImage::map() {
    void* data = nullptr;
    if(m_memory && !m_isMapped) {
      if (vkMapMemory(getDevice(), m_memory, 0, m_memSize, 0, &data) != VK_SUCCESS) {
        throw std::runtime_error("could not map image memory");
      }

      m_isMapped = true;
    }
    return data;
  }

  void DependentImage::unMap() {
    if(m_isMapped) {
      vkUnmapMemory(m_device, m_memory);
      m_isMapped = false;
    }
  }

}
