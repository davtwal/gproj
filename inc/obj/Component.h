// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Component.h
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

#ifndef DW_COMPONENT_H
#define DW_COMPONENT_H

#include "util/Utils.h"

#include <string>
#include <list>

namespace dw::obj {
  class Object;

  class IComponent {
  public:
    IComponent() = default;
    virtual ~IComponent() = default;

    enum class Type {
      ctTransform,
      ctGraphics,
      ctCamera,
      ctBehavior,
      ctLight,
      ctCount
    };

    template <typename T>
    NO_DISCARD T& as();

    template <typename T>
    NO_DISCARD const T& as() const;

    NO_DISCARD virtual Type getType() const = 0;
    NO_DISCARD virtual std::string getTypeName() const = 0;

    NO_DISCARD Object* getParent() const;

  private:
    friend class Object;
    Object* m_parent{ nullptr };
  };

  template <typename T>
  class ComponentBase : public IComponent {
  public:
    ComponentBase();
    virtual ~ComponentBase();

    using RefContainer = std::list<T*>;

    static constexpr size_t DEFAULT_CAPACITY = 128;
    static RefContainer const& GetList();

  private:
    static RefContainer& List();
  };
}

#include "obj/Component.inl"

#endif
