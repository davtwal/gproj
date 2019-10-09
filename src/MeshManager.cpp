// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : MeshManager.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 10m 08d
// * Last Altered: 2019y 10m 08d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include <tiny_obj_loader.h>

#include "MeshManager.h"
#include "Renderer.h"
#include "Trace.h"

namespace dw {
  void MeshManager::clear() {
    m_loadedMeshes.clear();
    m_curKey = 0;
  }

  MeshManager::MeshKey MeshManager::addMesh(std::vector<Vertex> verts, std::vector<uint32_t> indices) {
    m_loadedMeshes.try_emplace(m_curKey, std::move(verts), std::move(indices));
    return m_curKey++;
  }

  util::Ref<Mesh> MeshManager::getMesh(MeshKey key) {
    return m_loadedMeshes.at(key);
  }

  void MeshManager::uploadMeshes(Renderer& renderer) {
    renderer.uploadMeshes(m_loadedMeshes);
  }

  MeshManager::MeshKey MeshManager::load(std::string const& filename) {
    using namespace tinyobj;
    /* Attributes:
     *  - Contains vertices, normals, texture coords, (colors)
     *
     * Shapes:
     *  - Contain mesh, lines, points. ONLY MESHES SUPPORTED
     *
     * Mesh:
     *  - Contain face indices (into the attribute arrays),
     *  - Contain the number of vertices on a specific face (Will be 3 when triangulated, which we do)
     *  - Contain PER-FACE material IDs
     *  - Contain PER-FACE smoothing group IDs
     *
     * Material:
     *  - Ambient, Diffuse, Specular, Transmittance, Emission (all vec3)
     *  - Shininess, Refraction Index, Dissolve/transparency
     *  - Illumination model (ignored)
     *  TEXTURES:
     *    - Ambient texture
     *    - Diffuse texture (Albedo)
     *    - Specular texture
     *    - Specular highlight texture (i.e. specular exponent), affects shininess
     *    - Bump map
     *    - Displacement map
     *    - Alpha map! ok damn that's cool I guess?
     *    - Reflectivity map . _. Affects refraction index?
     *
     *  EACH TEXTURE HAS A texture_option_t ASSOCIATED WITH IT:
     *    - Sharpness, brightness, contrast, "origin offset", scale, "turbulence"??, clamp,
     *       imfchan !?!, blend u and v, bump multiplier, and colorspace (e.g. sRGB or linear)
     *
     *  PBR EXTENSION:
     *    - Roughness, Metallic, Sheen, Clear coat rough/thickness, anisotropy, aniso rotation
     *    - Also includes maps for rough/metallic/sheen, and emissive maps/normal maps
     *    - And texture options
     */
    attrib_t attributes;
    std::vector<shape_t> shapes;
    std::vector<material_t> materials;
    std::string warnString;
    std::string errString;

    bool worked = LoadObj(&attributes, &shapes, &materials, &warnString, &errString, filename.c_str());

    if (!errString.empty())
      Trace::Error << "Model Loading Error: " << errString << Trace::Stop;
    if (!warnString.empty())
      Trace::Warn << "Model Loading Warning: " << warnString << Trace::Stop;

    if(!worked) {
      return std::numeric_limits<MeshKey>::max();
    }

    // Vertices, normals, texcoords
    return m_curKey++;
  }
}
