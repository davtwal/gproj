// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Light.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 10m 04d
// * Last Altered: 2019y 10m 04d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#ifndef DW_LIGHT_H
#define DW_LIGHT_H

#include "MyMath.h"

#ifndef NO_DISCARD
#define NO_DISCARD [[nodiscard]]
#endif

namespace dw {
  struct LightUBO{
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 dir;
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec3 atten;
    alignas(04) float radius;
    alignas(04) int type; // 0, 1, 2
  };

  struct ShadowedUBO {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 dir;
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec3 atten;
    alignas(04) float radius;
    alignas(04) int type; // 0, 1, 2
  };

  class Light {
  public:
    static inline const glm::vec3 UP_DIR = { 0, 0.f, 1.f };

    enum class Type {
      Point,        // does not use direction
      Spot,         // 
      Directional   // does not use position/radius
    };

    virtual Light& setPosition(glm::vec3 const& pos);
    virtual Light& setDirection(glm::vec3 const& dir);
    virtual Light& setColor(glm::vec3 const& color);
    virtual Light& setAttenuation(glm::vec3 const& atten);
    virtual Light& setLocalRadius(float rad);
    virtual Light& setType(Type t);

    NO_DISCARD glm::vec3 const& getPosition() const;
    NO_DISCARD glm::vec3 const& getDirection() const;
    NO_DISCARD glm::vec3 const& getColor() const;
    NO_DISCARD glm::vec3 const& getAttenuation() const;
    NO_DISCARD float getLocalRadius() const;
    NO_DISCARD Type getType() const;

    NO_DISCARD LightUBO getAsUBO() const {
      return { m_position, m_direction, m_color, m_attenuation, m_localRadius, (int)m_type };
    }

  protected:
    glm::vec3 m_position    {0};        // world coordinates
    glm::vec3 m_direction   {0, 0, 1};  // should be unit vector
    glm::vec3 m_color       {1, 1, 1};  // should be 0..1
    glm::vec3 m_attenuation {0, 0, 0};  // x + yt + zt^2
    float m_localRadius     {5};              // world coordinates
    Type m_type             { Type::Point };
  };

  class ShadowedLight : public Light {
  public:
    Light& setPosition(glm::vec3 const& pos) override;
    Light& setDirection(glm::vec3 const& dir) override;

    // TODO: set FOV, aspect, near, far for projection matrix

    glm::mat4 const& getView();
    glm::mat4 const& getProj();

    ShadowedUBO getAsShadowUBO() {
      return { getView(), getProj(), m_position, m_direction, m_color, m_attenuation, m_localRadius, (int)m_type };
    }

  private:
    glm::mat4 m_view {1};
    glm::mat4 m_proj {1};
    bool m_isViewDirty{ true };
    bool m_isProjDirty{ true };
  };
}

#endif
