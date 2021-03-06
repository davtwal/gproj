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

#include "render/Image.h"
#include "render/Buffer.h"
#include "render/Texture.h"
#include "util/Utils.h"
#include "util/MyMath.h"
#include "tiny_obj_loader.h"

#include <unordered_map>
#include <array>

namespace dw {
  class Renderer;
  class CommandBuffer;

  class Material {
  public:
    Material() = default;
    ~Material() = default;

    MOVE_CONSTRUCT_ONLY(Material);

    static constexpr unsigned MTL_MAP_COUNT = 4;

    NO_DISCARD uint32_t getID() const;

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
        m_useMap[0],//m_textures[0] != nullptr,
        m_useMap[1],//m_textures[1] != nullptr,
        m_useMap[2],//m_textures[2] != nullptr,
        m_useMap[3]//m_textures[3] != nullptr,
      };
    }

    NO_DISCARD std::array<util::ptr<Texture>, MTL_MAP_COUNT> const& getTextures() const;
    NO_DISCARD util::ptr<Texture> getTexture(size_t i) const;

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

    //std::vector<RawImage> m_raws;
    //std::vector<DependentImage> m_images;
    //std::vector<ImageView> m_views;
    std::array<util::ptr<Texture>, MTL_MAP_COUNT> m_textures;
    std::array<bool, MTL_MAP_COUNT> m_useMap;
    glm::vec3 m_kd {1};
    glm::vec3 m_ks { 1 };
    float m_metallic{ 1 };
    float m_roughness{ 1 };

    uint32_t m_id{ 0 };
  };

  class MaterialManager {
  public:
    static constexpr const char* DEFAULT_MTL_NAME = "default";
    static constexpr const char* SKYBOX_MTL_NAME = "skybox";

    MaterialManager(TextureManager& textureStorage);

    using MtlKey = std::string;
    using MtlMap = std::unordered_map<MtlKey, util::ptr<Material>>;

    MtlKey load(tinyobj::material_t const& mtl);

    NO_DISCARD util::ptr<Material> getMtl(MtlKey key);
    NO_DISCARD util::ptr<Material> getDefaultMtl();
    NO_DISCARD util::ptr<Material> getSkyboxMtl();

    void clear();

    void uploadMaterials(Renderer& renderer);

    MtlMap const& getMaterials() const;
  private:
    TextureManager& m_textureStorage;

    MtlMap m_loadedMtls;
    uint32_t m_curID {0};
  };

}

#endif
