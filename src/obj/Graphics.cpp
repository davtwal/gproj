// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Graphics.cpp
// * Copyright (C) DigiPen Institute of Technology 2020
// * 
// * Created     : 2020y 02m 12d
// * Last Altered: 2020y 02m 12d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "obj/Object.h"
#include "obj/Graphics.h"

namespace dw::obj {
  Graphics::Graphics(util::ptr<Mesh> mesh)
    : m_mesh(std::move(mesh)) {
  }

  Graphics& Graphics::setMesh(util::ptr<Mesh> newMesh) {
    m_mesh = std::move(newMesh);
    return *this;
  }

  util::ptr<Mesh> Graphics::getMesh() const {
    return m_mesh;
  }
}
