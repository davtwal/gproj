// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Object.cpp
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

#include "obj/Object.h"
#include "obj/Graphics.h"
#include <stdarg.h>
#include <tuple>

namespace dw::obj {
  /*Object::Object(bool addGraphics) {
    attach(new Transform);

    if (addGraphics)
      attach(new Graphics);
  }*/

  Object::Object(int c, util::ptr<IComponent>...) {
    va_list list;
    va_start(list, c);

    for(int i = 0; i < c; ++i) {
      attach(va_arg(list, util::ptr<IComponent>));
    }

    va_end(list);

    if (m_components.find(IComponent::Type::ctTransform) == m_components.end())
      attach(util::make_ptr<Transform>());
  }

  Object::Object(Object&& obj) noexcept {
    m_components = std::move(obj.m_components);
    obj.m_components.clear();
  }

  Object& Object::operator=(Object&& obj) noexcept {
    m_components = std::move(obj.m_components);
    return *this;
  }


  bool Object::attach(util::ptr<IComponent> component) {
    if (!component)
      return false;

    component->m_parent = this;

    // .second is true if insertion took place, false if assignment took place
    // we return the opposite of that
    return !m_components.insert_or_assign(component->getType(),
                                          util::ptr<IComponent>(component)
      ).second;
  }

  std::shared_ptr<Transform> Object::getTransform() const {
    return get<Transform>();
  }

}