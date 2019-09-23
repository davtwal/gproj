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

#include "Image.h"
#include "Trace.h"
#include <cassert>

namespace dw {
  ImageView::ImageView(LogicalDevice& device, VkImageView v)
    : m_device(device),
      m_view(v) {

  }

  ImageView::~ImageView() {
    if (m_view)
      vkDestroyImageView(m_device, m_view, nullptr);
  }

  ImageView::ImageView(ImageView&& o) noexcept
    : m_device(o.m_device),
      m_view(std::move(o.m_view)) {
    o.m_view = nullptr;
  }

  Image::Image(VkImage i, VkFormat format, VkImageViewType type, bool isMutable)
    : m_image(i),
      m_format(format),
      m_type(type),
      m_mutable(isMutable) {
  }

  Image::Image(Image&& o)
    : m_image(o.m_image),
      m_format(o.m_format),
      m_type(o.m_type),
      m_mutable(o.m_mutable) {
  }


  Image::operator VkImage() const {
    return m_image;
  }

  constexpr bool IsFormatDepthOrStencil(VkFormat f) {
    return f >= VK_FORMAT_D16_UNORM && f <= VK_FORMAT_D32_SFLOAT_S8_UINT;
  }

  constexpr uint32_t BitsPerPixelFormat(VkFormat f) {
    if (f == VK_FORMAT_R4G4_UNORM_PACK8
        || f >= VK_FORMAT_R8_UNORM && f <= VK_FORMAT_R8_SRGB
        || f == VK_FORMAT_S8_UINT)
      return 8;

    if (f >= VK_FORMAT_R4G4B4A4_UNORM_PACK16 && f <= VK_FORMAT_A1R5G5B5_UNORM_PACK16
        || f == VK_FORMAT_D16_UNORM
        || f >= VK_FORMAT_R8G8_UNORM && f <= VK_FORMAT_R8G8_SRGB
        || f >= VK_FORMAT_R16_UNORM && f <= VK_FORMAT_R16_SFLOAT)
      return 16;

    if (f >= VK_FORMAT_R8G8B8_UNORM && f <= VK_FORMAT_B8G8R8_SRGB
        || f == VK_FORMAT_D16_UNORM_S8_UINT)
      return 24;

    if (f >= VK_FORMAT_R8G8B8A8_UNORM && f <= VK_FORMAT_A2B10G10R10_SINT_PACK32
        || f >= VK_FORMAT_R16G16_UNORM && f <= VK_FORMAT_R16G16_SFLOAT
        || f >= VK_FORMAT_R32_UINT && f <= VK_FORMAT_R32_SFLOAT
        || f == VK_FORMAT_B10G11R11_UFLOAT_PACK32
        || f == VK_FORMAT_E5B9G9R9_UFLOAT_PACK32
        || f == VK_FORMAT_X8_D24_UNORM_PACK32
        || f == VK_FORMAT_D32_SFLOAT
        || f == VK_FORMAT_D24_UNORM_S8_UINT)
      return 32;

    if (f == VK_FORMAT_D32_SFLOAT_S8_UINT)
      return 40;

    if (f >= VK_FORMAT_R16G16B16_UNORM && f <= VK_FORMAT_R16G16B16_SFLOAT)
      return 48;

    if (f >= VK_FORMAT_R16G16B16A16_UNORM && f <= VK_FORMAT_R16G16B16A16_SFLOAT
        || f >= VK_FORMAT_R32G32_UINT && f <= VK_FORMAT_R32G32_SFLOAT
        || f >= VK_FORMAT_R64_UINT && f <= VK_FORMAT_R64_SFLOAT)
      return 64;

    if (f >= VK_FORMAT_R32G32B32_UINT && f <= VK_FORMAT_R32G32B32_SFLOAT)
      return 96;

    if (f >= VK_FORMAT_R32G32B32A32_UINT && f <= VK_FORMAT_R32G32B32A32_SFLOAT
        || f >= VK_FORMAT_R64G64_UINT && f <= VK_FORMAT_R64G64_SFLOAT)
      return 128;

    if (f >= VK_FORMAT_R64G64B64_UINT && f <= VK_FORMAT_R64G64B64_SFLOAT)
      return 160;

    if (f >= VK_FORMAT_R64G64B64A64_UINT && f <= VK_FORMAT_R64G64B64A64_SFLOAT)
      return 192;

    //if (f >= VK_FORMAT_BC1_RGB_UNORM_BLOCK) < VK_FORMAT_ASTC_12x12_SRGB_BLOCK

    //if (f >=
    return std::numeric_limits<uint32_t>::max();
  }

  void Image::initImage(
    VkImageType       type,     // 1D, 2D, or 3D
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

    if (mipLevels > 1) {
      // check for powers of two
      if ((extent.depth & (extent.depth - 1)) != 0
          || (extent.height & (extent.height - 1)) != 0
          || (extent.width & (extent.width - 1)) != 0) {
        Trace::Error << "Attempted to create an image with >1 mip levels but not a power of two image" << Trace::Stop;
        return;
      }
    }

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
          || IsFormatDepthOrStencil(format)
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
      isMappable ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL,
      usage,
      VK_SHARING_MODE_EXCLUSIVE,
      0,
      nullptr,
      VK_IMAGE_LAYOUT_UNDEFINED
    };

    vkCreateImage(getDevice(), &createInfo, nullptr, &m_image);

    if (!m_image)
      throw std::bad_alloc();
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
    if (!getDevice() || BitsPerPixelFormat(format) != BitsPerPixelFormat(m_format))
      return ImageView(getDevice());

    // Aspect:
    //  Refers to a specific part of the image. You cannot create a view with
    //  multiple aspects, such as reading both depth and stencil.

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
      throw std::bad_alloc();

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

  // DEPENDENT IMAGE

  DependentImage::~DependentImage() {
    if (m_image)
      vkDestroyImage(getOwningDevice(), m_image, nullptr);
  }

  LogicalDevice& DependentImage::getDevice() const {
    return getOwningDevice();
  }

}
