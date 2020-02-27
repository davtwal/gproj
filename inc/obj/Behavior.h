// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Behavior.h
// * Copyright (C) DigiPen Institute of Technology 2020
// * 
// * Created     : 2020y 02m 13d
// * Last Altered: 2020y 02m 13d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#ifndef DW_BEHAVIOR_CMP_H
#define DW_BEHAVIOR_CMP_H

#include "obj/Component.h"

namespace dw::obj {
  class Behavior : public ComponentBase<Behavior> {
  public:
                            // parent, curTime, dt
    using BehaviorFn = void(*)(Object*, float, float);
    Behavior(BehaviorFn fn = nullptr);

    NO_DISCARD Type getType() const override { return Type::ctBehavior; }
    NO_DISCARD std::string getTypeName() const override { return "Behavior"; }

    Behavior& setFunction(BehaviorFn fn);
    BehaviorFn getFunction() const;

    void call(float, float);
  private:
    BehaviorFn m_behaviorFn{ nullptr };
  };
}

#endif
