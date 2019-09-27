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
    using MeshKey = uint32_t;

    /* After this call is resolved the following meshes will be available:
     * 0 - Unit Square
     * 1 - Full Screen Quad
     * 2 - Unit Square
     * 3 - Unit Sphere
     */
    void loadBasicMeshes();

    MeshKey addMesh(std::vector<Vertex> verts, std::vector<uint32_t> indices);
    util::Ref<Mesh> getMesh(MeshKey key);

    void uploadMeshes(Renderer& renderer);

    void clear();

  private:
    std::unordered_map<MeshKey, Mesh> m_loadedMeshes;
    MeshKey m_curKey{ 0 };
  };
}

#endif
