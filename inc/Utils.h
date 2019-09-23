// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : Utils.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 20d
// * Last Altered: 2019y 09m 20d
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

#ifndef NO_DISCARD
#define NO_DISCARD [[nodiscard]]
#endif

namespace dw::util {
  template<typename T>
  class Ref {
  public:
    using type = T;

    Ref(type& t)
      : ref(t) {}

    Ref<T> operator=(T const& o) {
      return Ref<T>(o);
    }

    Ref<T> operator=(Ref<T> const& o) {
      return Ref<T>(o.ref);
    }

    type& ref;

    operator type() const {
      return ref;
    }

    NO_DISCARD type* operator->() {
      return &ref;
    }

    NO_DISCARD type& operator*() const {
      return ref;
    }
  };
}

#endif

