// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Graphics.h
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

#ifndef DW_GRAPHICS_CMP_H
#define DW_GRAPHICS_CMP_H

#include "obj/Component.h"

#include "render/Mesh.h"

namespace dw::obj {
  class Graphics : public ComponentBase<Graphics> {
  public:
    Graphics(util::ptr<Mesh> mesh = nullptr);

    NO_DISCARD Type getType() const override { return Type::ctGraphics; }
    NO_DISCARD std::string getTypeName() const override { return "Graphics"; }

    NO_DISCARD util::ptr<Mesh> getMesh() const;

    Graphics& setMesh(util::ptr<Mesh> newMesh);

  private:
    util::ptr<Mesh> m_mesh;
  };
}

#endif
