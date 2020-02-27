// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Component.inl
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

#ifndef DW_COMPONENT_INL
#define DW_COMPONENT_INL

#include "obj/Component.h"

namespace dw::obj {
  template <typename T>
  T& IComponent::as() {
    return *reinterpret_cast<T*>(this);
  }

  template <typename T>
  T const& IComponent::as() const {
    return *reinterpret_cast<T*>(this);
  }

  ////////////////
  // ComponentBase

  template <typename T>
  typename ComponentBase<T>::RefContainer& ComponentBase<T>::List() {
    static RefContainer list;
    return list;
  }

  template <typename T>
  typename ComponentBase<T>::RefContainer const& ComponentBase<T>::GetList() {
    return List();
  }

  template <typename T>
  ComponentBase<T>::ComponentBase() {
    //if (List().capacity() == 0)
    //  List().reserve(DEFAULT_CAPACITY);

    List().push_back(reinterpret_cast<T*>(this));
  }

  template <typename T>
  ComponentBase<T>::~ComponentBase() {
    List().erase(std::find(List().begin(), List().end(), this));
  }
}

#endif
