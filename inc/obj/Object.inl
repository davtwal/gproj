// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Object.inl
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

#ifndef DW_OBJECT_INL
#define DW_OBJECT_INL

#include "obj/Object.h"

namespace dw::obj {
  /*template <typename ... TComponents>
  Object::Object(std::tuple<TComponents *...> componentList) {
    constexpr size_t size = std::tuple_size<TComponents *...>::value;
    attach(std::get<0>(componentList));
  }*/

  template <typename T>
  std::shared_ptr<T> Object::get() const {
    if (T::GetList().empty())
      return nullptr;

    try {
      return std::reinterpret_pointer_cast<T>(m_components.at(T::GetList().front()->getType()));
    }
    catch (std::exception&) {
      return nullptr;
    }
  }

  template <typename T>
  bool Object::remove() {
    if (T::GetList().empty())
      return false;

    try {
      return m_components.erase(T::GetList().front()->getType()) != 0u;
    }
    catch (std::exception&) {
      return false;
    }
  }
}

#endif
