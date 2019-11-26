// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Texture.h
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

#ifndef DW_TEXTURE_H
#define DW_TEXTURE_H

#include "Image.h"
#include "Utils.h"
#include "Buffer.h"
#include <unordered_map>

namespace dw {
  class Renderer;
  class CommandBuffer;

  class Texture {
  public:
    using StagingBuffs = util::ptr<Buffer>;

    Texture() = default;
    ~Texture() = default;

    MOVE_CONSTRUCT_ONLY(Texture);

    NO_DISCARD bool isLoaded() const;

    void createBuffers(LogicalDevice& device);
    NO_DISCARD StagingBuffs createStaging(LogicalDevice& device);

    NO_DISCARD StagingBuffs createAllBuffs(LogicalDevice& device) {
      createBuffers(device);
      return createStaging(device);
    }

    void uploadStaging(StagingBuffs& buffs);
    void uploadCmds(CommandBuffer& cmdBuff, StagingBuffs& staging);

    NO_DISCARD util::ptr<DependentImage> getImage() const;
    NO_DISCARD util::ptr<ImageView> getView() const;

  private:
    friend class TextureManager;

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
      float* m_rawf{ nullptr };


    };

    util::ptr<RawImage>        m_raw;
    util::ptr<DependentImage>  m_image;
    util::ptr<ImageView>       m_view;
  };

  class TextureManager {
  public:
    using TexKey = std::string;
    using TexMap = std::unordered_map<TexKey, util::ptr<Texture>>;

    TexMap::iterator load(std::string const& filename);

    NO_DISCARD util::ptr<Texture> getTexture(TexKey);

    void clear();
    void uploadTextures(Renderer& renderer);

    TexMap const& getTextures() const;

  private:
    TexMap m_loadedTextures;
  };
}

#endif
