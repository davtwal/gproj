// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Texture.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 11m 15d
// * Last Altered: 2019y 11m 15d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "Texture.h"

#include "CommandBuffer.h"
#include "MemoryAllocator.h"
#include "Trace.h"
#include "Renderer.h"

#include "stb_image.h"

#include <filesystem>
namespace fs = std::filesystem;

namespace dw {
  // Texture Manager
  void TextureManager::clear() {
    m_loadedTextures.clear();
  }

  util::ptr<Texture> TextureManager::getTexture(TexKey key) {
    try {
      return m_loadedTextures.at(key);
    } catch (std::out_of_range& e) {
      return nullptr;
    }
  }

  TextureManager::TexMap const& TextureManager::getTextures() const {
    return m_loadedTextures;
  }

  void TextureManager::uploadTextures(Renderer& renderer) {
    renderer.uploadTextures(m_loadedTextures);
  }

  TextureManager::TexMap::iterator TextureManager::load(std::string const& filename) {
    fs::path filepath{ filename };

    if (getTexture(filepath.filename().generic_string()) == nullptr) {
      util::ptr<Texture> tex = util::make_ptr<Texture>();
      tex->m_raw = util::make_ptr<Texture::RawImage>();
      tex->m_raw->Load(filename);

      return m_loadedTextures.insert_or_assign(filepath.filename().generic_string(), tex).first;
    }

    return m_loadedTextures.find(filepath.filename().generic_string());
  }
  
  // Raw Image
  namespace {
    VkFormat PickFormat(uint8_t bitsPerChannel, uint8_t channels) {
      switch (bitsPerChannel) {
      case 8:
        switch (channels) {
        case 1: return VK_FORMAT_R8_UNORM;
        case 2: return VK_FORMAT_R8G8_UNORM;
        case 3: return VK_FORMAT_R8G8B8_UNORM;
        case 4: return VK_FORMAT_R8G8B8A8_UNORM;
        default: return VK_FORMAT_UNDEFINED;
        }
      case 16:
        switch (channels) {
        case 1: return VK_FORMAT_R16_UNORM;
        case 2: return VK_FORMAT_R16G16_UNORM;
        case 3: return VK_FORMAT_R16G16B16_UNORM;
        case 4: return VK_FORMAT_R16G16B16A16_UNORM;
        default: return VK_FORMAT_UNDEFINED;
        }
      case 32:
        switch(channels) {
        case 1: return VK_FORMAT_R32_SFLOAT;
        case 2: return VK_FORMAT_R32G32_SFLOAT;
        case 3: return VK_FORMAT_R32G32B32_SFLOAT;
        case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
        default: return VK_FORMAT_UNDEFINED;
        }
      default: return VK_FORMAT_UNDEFINED;
      }
    }
  }

  Texture::RawImage::~RawImage() {
    if (m_raw) {
      stbi_image_free(m_raw);
      m_raw = nullptr;
    }

    else if(m_rawf) {
      stbi_image_free(m_rawf);
      m_rawf = nullptr;
    }
  }

  void Texture::RawImage::Load(std::string const& filename) {
    if (!m_raw) {
      // Supported file types from STB_IMAGE (STBI):
      /* PNG
       * JPG
       * HDR
       * ... a bunch others ...
       */
      stbi_set_flip_vertically_on_load(true);
      
      int w, h, c;

      if (stbi_is_hdr(filename.c_str())) {
        m_rawf = stbi_loadf(filename.c_str(), &w, &h, &c, STBI_rgb_alpha);
        if (m_rawf) {
          m_width = static_cast<uint64_t>(w);
          m_height = static_cast<uint64_t>(h);
          m_channels = 4;
          m_bitsPerChannel = sizeof(float) * 8;
        }
        else
          Trace::Error << "Could not load " << filename << " [HDR]: " << stbi_failure_reason() << Trace::Stop;
      }
      else {
        m_raw = stbi_load(filename.c_str(), &w, &h, &c, STBI_rgb_alpha);
        if (m_raw) {
          m_width = static_cast<uint64_t>(w);
          m_height = static_cast<uint64_t>(h);
          m_channels = 4;
          m_bitsPerChannel = 8;
        }
        else
          Trace::Error << "Could not load " << filename << " [LDR]: " << stbi_failure_reason() << Trace::Stop;
      }
    }
  }

  Texture::RawImage::RawImage(RawImage&& o) noexcept
    : m_width(o.m_width),
    m_height(o.m_height),
    m_channels(o.m_channels),
    m_bitsPerChannel(o.m_channels),
    m_raw(o.m_raw),
    m_rawf(o.m_rawf) {
    o.m_raw = nullptr;
    o.m_rawf = nullptr;
  }

  // Texture
  Texture::Texture(Texture&& o) noexcept
    : m_raw(std::move(o.m_raw)),
      m_image(std::move(o.m_image)),
      m_view(std::move(o.m_view)){
    m_raw.reset();
    m_image.reset();
    m_view.reset();
  }

  util::ptr<DependentImage> Texture::getImage() const {
    return m_image;
  }

  util::ptr<ImageView> Texture::getView() const {
    return m_view;
  }

  bool Texture::isLoaded() const {
    return m_raw == nullptr;
  }

  void Texture::createBuffers(LogicalDevice& device) {
    MemoryAllocator allocator(device.getOwningPhysical());

    VkExtent3D extent = { m_raw->m_width, m_raw->m_height, 1 };
    static constexpr auto DstImgUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VkFlags usageFlags = DstImgUsage;

    auto& img = *m_raw;

    VkFormat format = PickFormat(img.m_bitsPerChannel, img.m_channels);

    uint32_t mipLevels = 1;
    if (m_raw->m_rawf) {// if HDR
      mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_raw->m_width, m_raw->m_height)))) + 1;
      usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    m_image = util::make_ptr<DependentImage>(device);
    m_image->initImage(VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, format, extent, usageFlags, mipLevels, 1, false, false, false, false);
    m_image->back(allocator, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    m_view = util::make_ptr<ImageView>(m_image->createView());
  }

  Texture::StagingBuffs Texture::createStaging(LogicalDevice& device) {
    VkDeviceSize size = m_raw->m_width * m_raw->m_height * m_raw->m_channels * (m_raw->m_bitsPerChannel / 8);
    return util::make_ptr<Buffer>(Buffer::CreateStaging(device, size));
  }

  void Texture::uploadStaging(StagingBuffs& buffs) {
    VkDeviceSize size = m_raw->m_width * m_raw->m_height * m_raw->m_channels * (m_raw->m_bitsPerChannel / 8);
    void* data = buffs->map();
    if (m_raw->m_raw)
      memcpy(data, m_raw->m_raw, size);
    else
      memcpy(data, m_raw->m_rawf, size);
    buffs->unMap();
  }

  void Texture::uploadCmds(CommandBuffer& cmdBuff, StagingBuffs& staging) {
    VkExtent3D extent = { m_raw->m_width, m_raw->m_height, 1 };

    VkImageMemoryBarrier barrier = {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      nullptr,
      0,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_QUEUE_FAMILY_IGNORED,
      VK_QUEUE_FAMILY_IGNORED,
      *m_image,
      {
        VK_IMAGE_ASPECT_COLOR_BIT,
        0,
        m_image->getMipLevels(),
        0,
        1
      },
    };

    vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
      0,
      0, nullptr,
      0, nullptr,
      1, &barrier);

    barrier.subresourceRange.levelCount = 1;

    VkBufferImageCopy copy = {
      0,
      static_cast<uint32_t>(m_raw->m_width),
      static_cast<uint32_t>(m_raw->m_height),
      {
        VK_IMAGE_ASPECT_COLOR_BIT,
        0,
        0,
        1
      },
      {0, 0, 0},
      extent
    };

    vkCmdCopyBufferToImage(cmdBuff, *staging, *m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

    // Generate mipmaps if they exist:
    auto imageExtent = m_image->getExtent();
    for(uint32_t i = 1; i < m_image->getMipLevels(); ++i) {

      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      barrier.subresourceRange.baseMipLevel = i - 1;

      vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

      VkImageBlit blit = {
        {
          VK_IMAGE_ASPECT_COLOR_BIT,
          i - 1,
          0,
          1
        },
        {{0,0,0}, {imageExtent.width, imageExtent.height, 1}},
        {
          VK_IMAGE_ASPECT_COLOR_BIT,
          i,
          0,
          1
        },
        {{0,0,0},
          {imageExtent.width > 1 ? imageExtent.width / 2 : 1, imageExtent.height > 1 ? imageExtent.height / 2 : 1, 1}
        }
      };

      vkCmdBlitImage(cmdBuff, *m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

      if (imageExtent.width > 1) imageExtent.width /= 2;
      if (imageExtent.height > 1) imageExtent.height /= 2;
    }

    barrier.subresourceRange.baseMipLevel = m_image->getMipLevels() - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
      0, nullptr,
      0, nullptr,
      1, &barrier);

    // unload raw images
    // here instead of etc because i need the size for the copy commands
    m_raw.reset();
  }
}
