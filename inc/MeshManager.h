// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : MeshManager.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 09m 26d
// * Last Altered: 2019y 09m 26d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#ifndef DW_MESH_MANAGER_H
#define DW_MESH_MANAGER_H

#include "Mesh.h"

#include <unordered_map>

namespace dw {
  class Renderer;

  class MeshManager {
  public:
    MeshManager(MaterialManager& mtlLoader);

    using MeshKey = uint32_t;
    using MeshMap = std::unordered_map<MeshKey, Mesh>;

    /* After this call is resolved the following meshes will be available:
     * 0 - Unit Square
     * 1 - Full Screen Quad
     * 2 - Unit Square
     * 3 - Unit Sphere
     */
    void loadBasicMeshes();

    MeshMap::reference addMesh(std::vector<Vertex> verts, std::vector<uint32_t> indices, util::ptr<Material> mtl = nullptr);
    void uploadMeshes(Renderer& renderer);

    util::Ref<Mesh> getMesh(MeshKey key);

    MeshKey load(std::string const& filename, bool flipWinding = false);

    void clear();

  private:
    util::Ref<MaterialManager> m_materialLoader;
    MeshMap m_loadedMeshes;
    MeshKey m_curKey{ 0 };
  };
}

#endif
