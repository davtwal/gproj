// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Transform.h
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

#ifndef DW_TRANSFORM_H
#define DW_TRANSFORM_H

#include "obj/Component.h"

#include "util/MyMath.h"

namespace dw::obj {
  class Transform : public ComponentBase<Transform> {
  public:
    NO_DISCARD Type getType() const override { return Type::ctTransform; }
    NO_DISCARD std::string getTypeName() const override { return "Transform"; }

    glm::mat4 const& getMatrix();

    NO_DISCARD glm::vec3 const& getPosition() const;
    NO_DISCARD glm::vec3 const& getScale() const;
    NO_DISCARD glm::quat const& getRotation() const;

    Transform& setPosition(glm::vec3 const& newPos);
    Transform& setScale(glm::vec3 const& newScale);
    Transform& setRotation(glm::quat const& newRot);

    Transform& addPosition(glm::vec3 const& plusPos);
    Transform& addScale(glm::vec3 const& plusScale);
    Transform& addRotation(glm::quat const& plusRot);

    // etc.
  private:
    glm::mat4 m_matrix {};
    glm::quat m_rotation {};
    glm::vec3 m_position{ 0.f };
    glm::vec3 m_scale{ 1.f };

    bool m_dirty{ true };
  };
}

#endif
