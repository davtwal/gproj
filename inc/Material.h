// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Material.h
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

#ifndef DW_MATERIAL_H
#define DW_MATERIAL_H

#include "Image.h"
#include "Buffer.h"
#include "Utils.h"
#include "MyMath.h"
#include "tiny_obj_loader.h"

#include <unordered_map>
#include <array>
//namespace tinyobj {
//  typedef struct material_t material_t;
//}

namespace dw {
  class Renderer;
  class CommandBuffer;

  class Material {
  public:
    Material() = default;

    MOVE_CONSTRUCT_ONLY(Material);

    static constexpr unsigned MTL_MAP_COUNT = 4;

    NO_DISCARD bool hasTexturesToLoad() const;
    NO_DISCARD uint32_t getID() const;

    struct StagingBuffs {
      util::ptr<Buffer> buffs[MTL_MAP_COUNT];
    };

    void createBuffers(LogicalDevice& device);
    NO_DISCARD StagingBuffs createStaging(LogicalDevice& device);

    NO_DISCARD StagingBuffs createAllBuffs(LogicalDevice& device) {
      createBuffers(device);
      return createStaging(device);
    }

    void uploadStaging(StagingBuffs& buffs);
    void uploadCmds(CommandBuffer& cmdBuff, StagingBuffs& staging);

    //
    struct MaterialUBO {
      alignas(16) glm::vec3 kd;
      alignas(16) glm::vec3 ks;
      alignas(04) float metallic;
      alignas(04) float roughness;
      alignas(04) int hasAlbedo;
      alignas(04) int hasNormal;
      alignas(04) int hasMetallic;
      alignas(04) int hasRoughness;
    };

    NO_DISCARD MaterialUBO getAsUBO() const {
      return { m_kd, m_ks, m_metallic, m_roughness,
        m_useMap[0],
        m_useMap[1],
        m_useMap[2],
        m_useMap[3],
      };
    }

    NO_DISCARD std::vector<DependentImage> const& getImages() const;
    NO_DISCARD std::vector<ImageView> const& getViews() const;

  private:
    friend class MaterialManager;

    class RawImage {
    public:
      RawImage() = default;
      ~RawImage();

      MOVE_CONSTRUCT_ONLY(RawImage);
      
      void Load(std::string const& filename);

      uint64_t m_width{ 0 };
      uint64_t m_height{ 0 };
      uint64_t m_channels{ 0 };
      uint64_t m_bitsPerChannel{ 0 };
      unsigned char* m_raw{ nullptr };
    };

    std::vector<RawImage> m_raws;
    std::vector<DependentImage> m_images;
    std::vector<ImageView> m_views;
    glm::vec3 m_kd {1};
    glm::vec3 m_ks { 1 };
    float m_metallic{ 1 };
    float m_roughness{ 1 };

    std::array<bool, MTL_MAP_COUNT> m_useMap{ true };

    uint32_t m_id{ 0 };

  };

  class MaterialManager {
  public:
    static constexpr const char* DEFAULT_MTL_NAME = "default";

    using MtlKey = std::string;
    using MtlMap = std::unordered_map<MtlKey, util::ptr<Material>>;

    MtlKey load(tinyobj::material_t const& mtl);

    NO_DISCARD util::ptr<Material> getMtl(MtlKey key);
    NO_DISCARD util::ptr<Material> getDefaultMtl();

    void clear();

    void uploadMaterials(Renderer& renderer);

    MtlMap const& getMaterials() const;
  private:
    MtlMap m_loadedMtls;
    uint32_t m_curID {0};
  };

}

#endif
