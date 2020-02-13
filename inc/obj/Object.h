// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Object.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 09m 25d
// * Last Altered: 2019y 09m 25d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * Description :
// *
// *
// *
// *
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

#ifndef DW_OBJECT_H
#define DW_OBJECT_H

#include "render/Mesh.h"
#include "util/Utils.h"
#include "util/MyMath.h"
#include <functional>

namespace dw {
  class Object {
  public:
    Object(util::Ref<Mesh> const& mesh);
    //Object(util::ptr<Mesh> mesh, glm::vec3 pos, glm::vec3 scale, glm::quat rot);

    glm::mat4 const& getTransform();
    NO_DISCARD glm::vec3 const& getPosition() const;
    NO_DISCARD glm::vec3 const& getScale() const;
    NO_DISCARD glm::quat const& getRotation() const;

    Object& setPosition(glm::vec3 const& pos);
    Object& setScale(glm::vec3 const& scale);
    Object& setRotation(glm::quat const& rot);

    // graphics
    util::Ref<Mesh> m_mesh;

    // behavior
    using BehaviorFn = std::function<void(Object &, float, float)>;

    BehaviorFn m_behavior;

    void callBehavior(float curTime, float dt);

  private:

    // transform
    glm::mat4 m_transform {1.f};

    glm::vec3 m_position {0 };
    glm::vec3 m_scale {1 };
    glm::quat m_rotation{glm::identity<glm::quat>()};

    bool m_isDirty { true };
  };
}

#endif
