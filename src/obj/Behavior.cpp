// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Behavior.cpp
// * Copyright (C) DigiPen Institute of Technology 2020
// * 
// * Created     : 2020y 02m 26d
// * Last Altered: 2020y 02m 26d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "obj/Behavior.h"

namespace dw::obj {
  Behavior::Behavior(BehaviorFn fn)
    : m_behaviorFn(fn) {
  }

  void Behavior::call(float t, float dt) {
    if (m_behaviorFn)
      m_behaviorFn(this->getParent(), t, dt);
  }

  Behavior::BehaviorFn Behavior::getFunction() const {
    return m_behaviorFn;
  }

  Behavior& Behavior::setFunction(BehaviorFn fn) {
    m_behaviorFn = fn;
    return *this;
  }
}