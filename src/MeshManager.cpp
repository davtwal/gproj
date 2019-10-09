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
#include <algorithm>

namespace dw {
  void MeshManager::clear() {
    m_loadedMeshes.clear();
    m_curKey = 0;
  }

  MeshManager::MeshMap::reference MeshManager::addMesh(std::vector<Vertex> verts, std::vector<uint32_t> indices) {
    return *m_loadedMeshes.try_emplace(m_curKey++, std::move(verts), std::move(indices)).first;
  }

  util::Ref<Mesh> MeshManager::getMesh(MeshKey key) {
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

    bool computeNormals = attributes.normals.empty();
    bool computeTangents = !attributes.texcoords.empty();

    std::vector<Vertex> vertices;
    const size_t vertexCount = attributes.vertices.size() / 3;
    assert(attributes.vertices.size() % 3 == 0);

    vertices.reserve(vertexCount);
    for(size_t i = 0; i < attributes.vertices.size(); i += 3) {
      Vertex v;

      v.pos = glm::vec3(attributes.vertices[i],  attributes.vertices[i + 1],  attributes.vertices[i + 2]);

      if(i < attributes.normals.size())
        v.normal = glm::vec3(attributes.normals[i],   attributes.normals[i + 1],   attributes.normals[i + 2]);

      if(i < attributes.texcoords.size())
        v.texCoord = glm::vec2(attributes.texcoords[i], attributes.texcoords[i + 1]);

      if(i < attributes.colors.size())
        v.color = glm::vec3(attributes.colors[i],    attributes.colors[i + 1],    attributes.colors[i + 2]);

      vertices.push_back(v);
    }

    size_t faceCount = 0;
    // do per-face fixing (compute normals, compute tangents)
    for(auto& shape : shapes) { // += 3 here because we triangulated.
      for (size_t i = 0; i < shape.mesh.indices.size(); i += 3) {
        ++faceCount;

        // http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/
        auto& index0 = shape.mesh.indices[i];
        auto& index1 = shape.mesh.indices[i + 1];
        auto& index2 = shape.mesh.indices[i + 2];

        auto& v0 = vertices[index0.vertex_index];
        auto& v1 = vertices[index1.vertex_index];
        auto& v2 = vertices[index2.vertex_index];
        glm::vec3 deltaP0 = v1.pos - v0.pos;
        glm::vec3 deltaP1 = v2.pos - v0.pos;

        if (computeNormals) {
          glm::vec3 N = normalize(cross(deltaP0, deltaP1));
          v0.normal += N;
          v1.normal += N;
          v2.normal += N;
        }

        if (computeTangents) {
          if (index0.texcoord_index == -1 || index1.texcoord_index == -1 || index2.texcoord_index == -1)
            continue;

          auto& uv0 = vertices[index0.texcoord_index].texCoord;
          auto& uv1 = vertices[index1.texcoord_index].texCoord;
          auto& uv2 = vertices[index2.texcoord_index].texCoord;

          glm::vec2 deltaUV0 = uv1 - uv0;
          glm::vec2 deltaUV1 = uv2 - uv0;

          float denom = 1.f / (deltaUV0.x * deltaUV1.y - deltaUV0.y * deltaUV1.x);

          // normalize?
          glm::vec3 tangent = (deltaP0 * deltaUV1.y - deltaP1 * deltaUV0.y) * denom;
          glm::vec3 bitangent = (deltaP1 * deltaUV0.x - deltaP0 * deltaUV1.x) * denom;

          vertices[index0.vertex_index].tangent += tangent;
          vertices[index1.vertex_index].tangent += tangent;
          vertices[index2.vertex_index].tangent += tangent;

          vertices[index0.vertex_index].bitangent += bitangent;
          vertices[index1.vertex_index].bitangent += bitangent;
          vertices[index2.vertex_index].bitangent += bitangent;
        }
      }
    }

    // POST-PROCESS STEPS
    // Normalize normals, tangents, and bitangents
    for(auto& vert : vertices) {
      vert.normal = normalize(vert.normal);
      vert.tangent = normalize(vert.tangent);
      vert.bitangent = normalize(vert.bitangent);
    }

    // Center vertices
    glm::vec3 rollingAverage = vertices[0].pos;
    for(size_t i = 1; i < vertices.size(); ++i) {
      rollingAverage = (vertices[i].pos + glm::vec3(i) * rollingAverage) / glm::vec3(i + 1);
    }

    float biggestExtent = glm::length2(vertices[0].pos - rollingAverage);
    for(auto& vert : vertices) {
      vert.pos -= rollingAverage;
      biggestExtent = std::max(biggestExtent, glm::length2(vert.pos));
    }

    biggestExtent = 1.f / sqrt(biggestExtent);
    for(auto& vert : vertices) {
      vert.pos *= biggestExtent;
    }

    // form index vector
    std::vector<uint32_t> indices;
    indices.reserve(faceCount * 3); // *3 because triangulated

    for(auto& shape : shapes) {
      for(size_t i = 0; i < shape.mesh.indices.size(); i += 3) {
        // This reverses the winding of the faces.
        indices.push_back(shape.mesh.indices[i].vertex_index);
        if (flipWinding) {
          indices.push_back(shape.mesh.indices[i + 2].vertex_index);
          indices.push_back(shape.mesh.indices[i + 1].vertex_index);
        } else {
          indices.push_back(shape.mesh.indices[i + 1].vertex_index);
          indices.push_back(shape.mesh.indices[i + 2].vertex_index);
        }
      }
    }

    indices.shrink_to_fit();

    return addMesh(vertices, indices).first;
  }
}
