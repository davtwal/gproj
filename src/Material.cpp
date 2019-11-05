// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Material.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 11m 04d
// * Last Altered: 2019y 11m 04d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "Material.h"
#include "MemoryAllocator.h"
#include "CommandBuffer.h"
#include "Renderer.h"
#include "Trace.h"
#include "tiny_obj_loader.h"
#include "stb_image.h"
#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;

namespace dw {
  // MANAGER

  util::ptr<Material> MaterialManager::getMtl(MtlKey key) {
    try {
      return m_loadedMtls.at(key);
    } catch(std::out_of_range& e) {
      return nullptr;
    }
  }

  util::ptr<Material> MaterialManager::getDefaultMtl() {
    auto ret = getMtl(DEFAULT_MTL_NAME);

    if(ret == nullptr) {
      auto iter = m_loadedMtls.try_emplace(DEFAULT_MTL_NAME, util::make_ptr<Material>());
      auto& mtl = *iter.first->second;

      mtl.m_id = m_curID++;

      mtl.m_kd = { 1, 1, 1 };
      mtl.m_ks = { 1, 1, 1 };

      ret = iter.first->second;
    }

    return ret;
  }

  void MaterialManager::uploadMaterials(Renderer& renderer) {
    renderer.uploadMaterials(m_loadedMtls);
  }

  MaterialManager::MtlKey MaterialManager::load(tinyobj::material_t const& mtl) {
    auto iter = m_loadedMtls.try_emplace(mtl.name, util::make_ptr<Material>());

    if (iter.second) {
      auto& material = *iter.first->second;
      material.m_kd = glm::vec3(mtl.diffuse[0], mtl.diffuse[1], mtl.diffuse[2]);
      material.m_ks = glm::vec3(mtl.specular[0], mtl.specular[1], mtl.specular[2]);
      material.m_metallic = mtl.metallic;
      material.m_roughness = mtl.roughness;
      material.m_id = m_curID++;
      // load textures
      assert(Material::MTL_MAP_COUNT == 4);
      material.m_raws.resize(Material::MTL_MAP_COUNT);

      fs::path mtlPath = fs::current_path() / "data" / "materials";

      if(!mtl.diffuse_texname.empty())
        material.m_raws[0].Load((mtlPath / fs::path(mtl.diffuse_texname)).generic_string());

      if(!mtl.normal_texname.empty())
        material.m_raws[1].Load((mtlPath / fs::path(mtl.normal_texname)).generic_string());

      if(!mtl.metallic_texname.empty())
        material.m_raws[2].Load((mtlPath / fs::path(mtl.metallic_texname)).generic_string());

      if(!mtl.roughness_texname.empty())
        material.m_raws[3].Load((mtlPath / fs::path(mtl.roughness_texname)).generic_string());
    }

    return iter.first->first;
  }

  void MaterialManager::clear() {
    m_loadedMtls.clear();
  }

  MaterialManager::MtlMap const& MaterialManager::getMaterials() const {
    return m_loadedMtls;
  }


  // MTL
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
      default: return VK_FORMAT_UNDEFINED;
      }
    }
  }

  Material::RawImage::~RawImage() {
    if (m_raw) {
      stbi_image_free(m_raw);
      m_raw = nullptr;
    }
  }

  void Material::RawImage::Load(std::string const& filename) {
    if (!m_raw) {
      stbi_set_flip_vertically_on_load(true);
      int w, h, c;

      m_raw = stbi_load(filename.c_str(), &w, &h, &c, STBI_rgb_alpha);
      if (m_raw) {
        m_width = static_cast<uint64_t>(w);
        m_height = static_cast<uint64_t>(h);
        m_channels = 4;// static_cast<uint64_t>(c);
        m_bitsPerChannel = 8;
      }
      else
        Trace::Error << "Could not load " << filename << ": " << stbi_failure_reason() << Trace::Stop;
    }
  }

  Material::RawImage::RawImage(RawImage&& o) noexcept
    : m_width(o.m_width),
      m_height(o.m_height),
      m_channels(o.m_channels),
      m_bitsPerChannel(o.m_channels),
      m_raw(o.m_raw) {
    o.m_raw = nullptr;
  }

  Material::Material(Material&& o) noexcept
    : m_kd(o.m_kd),
      m_ks(o.m_ks),
      m_metallic(o.m_metallic),
      m_roughness(o.m_roughness),
      m_raws(std::move(o.m_raws)),
      m_images(std::move(o.m_images)),
      m_views(std::move(o.m_views)){
  }

  std::vector<DependentImage> const& Material::getImages() const {
    return m_images;
  }

  std::vector<ImageView> const& Material::getViews() const {
    return m_views;
  }

  uint32_t Material::getID() const {
    return m_id;
  }

  bool Material::hasTexturesToLoad() const {
    return !m_raws.empty() && std::find_if(m_raws.begin(), m_raws.end(),
          [](RawImage const& r) { return r.m_raw != nullptr; }
        ) != m_raws.end();
  }

  void Material::createBuffers(LogicalDevice& device) {
    MemoryAllocator allocator(device.getOwningPhysical());

    VkExtent3D extent = { m_raws[0].m_width, m_raws[0].m_height, 1 };
    VkDeviceSize size = extent.width * extent.height * m_raws[0].m_channels;
    static constexpr auto DstImgUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    for (size_t i = 0; i < m_raws.size(); ++i) {
      auto& img = m_raws[i];

      VkFormat format = PickFormat(img.m_bitsPerChannel, img.m_channels);

      m_images.emplace_back(device);
      m_images.back().initImage(VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, format, extent, DstImgUsage, 1, 1, false, false, false, false);
      m_images.back().back(allocator, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

      m_views.emplace_back(m_images.back().createView());
    }
  }

  Material::StagingBuffs Material::createStaging(LogicalDevice& device) {
    StagingBuffs ret;

    for(size_t i = 0; i < MTL_MAP_COUNT; ++i) {
      VkDeviceSize size = m_raws[i].m_width * m_raws[i].m_height * m_raws[i].m_channels * (m_raws[i].m_bitsPerChannel / 8);
      ret.buffs[i] = m_raws[i].m_raw ? util::make_ptr<Buffer>(Buffer::CreateStaging(device, size)) : nullptr;
    }

    return ret;
  }

  void Material::uploadStaging(StagingBuffs& buffs) {
    for(size_t i = 0; i < MTL_MAP_COUNT; ++i) {
      VkDeviceSize size = m_raws[i].m_width * m_raws[i].m_height * m_raws[i].m_channels * (m_raws[i].m_bitsPerChannel / 8);
      void* data = buffs.buffs[i]->map();
      memcpy(data, m_raws[i].m_raw, size);
      buffs.buffs[i]->unMap();
    }
  }

  void Material::uploadCmds(CommandBuffer& cmdBuff, StagingBuffs& staging) {
    for (size_t i = 0; i < MTL_MAP_COUNT; ++i) {
      VkExtent3D extent = { m_raws[i].m_width, m_raws[i].m_height, 1 };

      VkImageMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        0,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        m_images[i],
        {
          VK_IMAGE_ASPECT_COLOR_BIT,
          0,
          1,
          0,
          1
        },
      };

      vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

      VkBufferImageCopy copy = {
        0,
        static_cast<uint32_t>(m_raws[i].m_width),
        static_cast<uint32_t>(m_raws[i].m_height),
        {
          VK_IMAGE_ASPECT_COLOR_BIT,
          0,
          0,
         1
        },
        {0, 0, 0},
        extent
      };

      vkCmdCopyBufferToImage(cmdBuff, *staging.buffs[i], m_images[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
    }

    // unload raw images
    // here instead of etc because i need the size for the copy commands
    m_raws.clear();
  }
}
