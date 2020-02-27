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

#include "render/MeshManager.h"
#include "render/Renderer.h"
#include "util/Trace.h"
#include <algorithm>
#include <filesystem>
#include <unordered_set>
namespace fs = std::filesystem;

bool operator==(tinyobj::index_t const& a, tinyobj::index_t const& b) {
  return a.vertex_index == b.vertex_index && a.texcoord_index == b.texcoord_index && a.normal_index == b.normal_index;
}

namespace std {
  template <>
  struct equal_to<tinyobj::index_t> { // functor for operator==
    _CXX17_DEPRECATE_ADAPTOR_TYPEDEFS typedef tinyobj::index_t first_argument_type;
    _CXX17_DEPRECATE_ADAPTOR_TYPEDEFS typedef tinyobj::index_t second_argument_type;
    _CXX17_DEPRECATE_ADAPTOR_TYPEDEFS typedef bool             result_type;

    constexpr bool operator()(const tinyobj::index_t& _Left, const tinyobj::index_t& _Right) const {
      return _Left.vertex_index == _Right.vertex_index && _Left.texcoord_index == _Right.texcoord_index && _Left.
             normal_index == _Right.normal_index;
    }
  };

  template <>
  struct hash<tinyobj::index_t> {
    size_t operator()(const tinyobj::index_t& i) const noexcept {
      return (hash<int>()(i.vertex_index) << 32)
             ^ (hash<int>()(i.texcoord_index) << 16)
             ^ (hash<int>()(i.normal_index));
    }
  };
}

namespace dw {
  MeshManager::MeshManager(MaterialManager& mtlLoader)
    : m_materialLoader(mtlLoader){
  }

  void MeshManager::clear() {
    m_loadedMeshes.clear();
    m_curKey = 0;
  }

  MeshManager::MeshMap::reference MeshManager::addMesh(std::vector<Vertex> verts, std::vector<uint32_t> indices, util::ptr<Material> mtl) {
    // give it the default material if there is no material requested
    if (mtl == nullptr)
      mtl = m_materialLoader.get().getDefaultMtl();

    auto iter = m_loadedMeshes.try_emplace(m_curKey++, util::make_ptr<Mesh>(std::move(verts), std::move(indices))).first;
    iter->second->setMaterial(mtl);
    return *iter;
  }

  util::ptr<Mesh> MeshManager::getMesh(MeshKey key) {
    return m_loadedMeshes.at(key);
  }

  void MeshManager::uploadMeshes(Renderer& renderer) {
    renderer.uploadMeshes(m_loadedMeshes);
  }

  MeshManager::MeshKey MeshManager::load(std::string const& filename, bool flipWinding) {
    using namespace tinyobj;
    /* Attributes:
     *  - Contains vertices, normals, texture coords, (colors)
     *    ALL IN SEPERATE VECTORS
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
    attrib_t                attributes;
    std::vector<shape_t>    shapes;
    std::vector<material_t> materials;
    std::string             warnString;
    std::string             errString;

    fs::path mtlPath = fs::current_path() / "data" / "materials";
    bool     worked  = LoadObj(&attributes,
                               &shapes,
                               &materials,
                               &warnString,
                               &errString,
                               filename.c_str(),
                               mtlPath.generic_string().c_str());

    if (!errString.empty())
      Trace::Error << "Model Loading Error: " << errString << Trace::Stop;
    if (!warnString.empty())
      Trace::Warn << "Model Loading Warning: " << warnString << Trace::Stop;

    if (!worked) {
      return std::numeric_limits<MeshKey>::max();
    }

    bool computeNormals  = attributes.normals.empty();
    bool computeTangents = !attributes.texcoords.empty();

    assert(attributes.vertices.size() % 3 == 0);
    const size_t                        vertexCount = attributes.vertices.size() / 3;
    std::vector<Vertex>                 vertices;
    std::vector<uint32_t>               indices;
    std::unordered_map<index_t, size_t> vertexIndexCombos;

    // Getting a vertex of index i : attributes.vertices[i * 3] attributes.vertices[i * 3 + 1] attributes.vertices[i * 3 + 2]
    // Vertex info:
    //  - Position
    //  - Normal
    //  - Tangent
    //  - Bitangent
    //  - Texcoord
    //  - Color
    // A vertex is created for each UNIQUE COMBINATION of data
    // If a face has 8/2 3/1 4/2 as its vi/ti, and another has 8/1 3/1 4/2, then we'll end up with
    // four total vertices: 8/1, 8/2, 3/1, 4/2.
    size_t duplicates_saved = 0;
    util::ptr<Material> loadedMtl = nullptr;
    for (auto& shape : shapes) {
      auto& mesh = shape.mesh;
      if (mesh.material_ids.front() > 0) {
        loadedMtl = m_materialLoader.get().getMtl(m_materialLoader.get().load(materials[mesh.material_ids.front()]));
      }

      for (uint32_t i = 0; i < mesh.indices.size(); ++i) {
        auto& index = mesh.indices[i];

        // if this combination is already a part of the data
        auto val = vertexIndexCombos.find(index);
        if (val != vertexIndexCombos.end()) {
          indices.push_back(val->second);
          ++duplicates_saved;
        }
        else {
          // new unique combination
          auto& vi = index.vertex_index;
          auto& ti = index.texcoord_index;
          auto& ni = index.normal_index;

          Vertex v;
          v.pos = {attributes.vertices[vi * 3], attributes.vertices[vi * 3 + 1], attributes.vertices[vi * 3 + 2]};

          if (!attributes.normals.empty())
            v.normal = {attributes.normals[ni * 3], attributes.normals[ni * 3 + 1], attributes.normals[ni * 3 + 2]};

          if (!attributes.texcoords.empty())
            v.texCoord = glm::vec2{attributes.texcoords[ti * 2], attributes.texcoords[ti * 2 + 1]};

          if (!attributes.colors.empty())
            v.color = {attributes.colors[vi * 3], attributes.colors[vi * 3 + 1], attributes.colors[vi * 3 + 2]};

          indices.push_back(vertices.size());
          vertexIndexCombos.try_emplace(index, vertices.size());

          vertices.push_back(v);
        }
      }
    }

    vertices.shrink_to_fit();
    indices.shrink_to_fit();

    Trace::All << "Mesh Loading Duplicates (" << filename << "): " << duplicates_saved << Trace::Stop;

    // We now have a complete list of VERTICES and INDICES for a complete mesh.
    // Compute normals / tangents / bitangents

    // for each triangle
    assert(indices.size() % 3 == 0);
    for (size_t i = 0; i < indices.size(); i += 3) {
      if(flipWinding) {
        std::swap(indices[i + 1], indices[i + 2]);
      }
      auto& v0 = vertices[indices[i]];
      auto& v1 = vertices[indices[i + 1]];
      auto& v2 = vertices[indices[i + 2]];

      glm::vec3 deltaP0 = v1.pos - v0.pos;
      glm::vec3 deltaP1 = v2.pos - v0.pos;


      if (computeNormals) {
        glm::vec3 N = normalize(cross(deltaP0, deltaP1));
        v0.normal += N;
        v1.normal += N;
        v2.normal += N;
      }

      if (computeTangents) {
        glm::vec2 deltaUV0 = v1.texCoord - v0.texCoord;
        glm::vec2 deltaUV1 = v2.texCoord - v0.texCoord;

        float denom = 1.f / (deltaUV0.x * deltaUV1.y - deltaUV0.y * deltaUV1.x);

        glm::vec3 tan  = (deltaP0 * deltaUV1.y - deltaP1 * deltaUV0.y) * denom;
        glm::vec3 btan = (deltaP1 * deltaUV0.x - deltaP0 * deltaUV1.x) * denom;

        v0.tangent += tan;
        v1.tangent += tan;
        v2.tangent += tan;

        v0.bitangent += btan;
        v1.bitangent += btan;
        v2.bitangent += btan;
      }
    }

    // POST-PROCESS STEPS
    // Normalize normals, tangents, and bitangents
    for (auto& vert : vertices) {
      vert.normal    = normalize(vert.normal);
      vert.tangent   = normalize(vert.tangent);
      vert.bitangent = normalize(vert.bitangent);
    }

    // Center vertices
    glm::vec3 rollingAverage = vertices[0].pos;
    for (size_t i = 1; i < vertices.size(); ++i) {
      rollingAverage = (vertices[i].pos + glm::vec3(i) * rollingAverage) / glm::vec3(i + 1);
    }

    float biggestExtent = length2(vertices[0].pos - rollingAverage);
    for (auto& vert : vertices) {
      vert.pos -= rollingAverage;
      biggestExtent = std::max(biggestExtent, length2(vert.pos));
    }

    biggestExtent = 1.f / sqrt(biggestExtent);
    for (auto& vert : vertices) {
      vert.pos *= biggestExtent;
    }

    return addMesh(vertices, indices, loadedMtl).first;
  }
}
