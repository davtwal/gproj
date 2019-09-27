// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Utils.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 09m 23d
// * Last Altered: 2019y 09m 23d
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

#ifndef DW_UTILS_H
#define DW_UTILS_H

#include <memory>

#ifndef NO_DISCARD
#define NO_DISCARD [[nodiscard]]
#endif

namespace dw::util {
  template <typename T>
  using ptr = std::shared_ptr<T>;

  template<typename T, typename... Args>
  ptr<T> make_ptr(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
  }

  template <typename T>
  class Ref {
  public:
    using type = T;

    Ref(type& t)
      : ref(t) {
    }

    Ref<T> operator=(T const& o) {
      return Ref<T>(o);
    }

    Ref<T> operator=(Ref<T> const& o) {
      return Ref<T>(o.ref);
    }

    type& ref;

    bool operator==(Ref<T> const& o) {
      return ref == o.ref;
    }

    bool operator!=(Ref<T> const& o) {
      return !(ref == o.ref);
    }

    operator type() const {
      return ref;
    }

    NO_DISCARD type* operator->() const {
      return &ref;
    }

    NO_DISCARD type& operator*() const {
      return ref;
    }
  };
}

#endif
