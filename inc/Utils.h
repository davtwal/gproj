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
#include "Vulkan.h"

#ifndef NO_DISCARD
#define NO_DISCARD [[nodiscard]]
#endif

enum VkFormat;

namespace dw::util {
  template <typename T>
  using ptr = std::shared_ptr<T>;

  template<typename T, typename... Args>
  ptr<T> make_ptr(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
  }

  constexpr bool IsFormatStencil(VkFormat f) {
    return f >= VK_FORMAT_S8_UINT && f <= VK_FORMAT_D32_SFLOAT_S8_UINT;
  }

  constexpr bool IsFormatDepthOrStencil(VkFormat f) {
    return f >= VK_FORMAT_D16_UNORM && f <= VK_FORMAT_D32_SFLOAT_S8_UINT;
  }

  constexpr uint32_t BitsPerPixelFormat(VkFormat f) {
    if (f == VK_FORMAT_R4G4_UNORM_PACK8
      || f >= VK_FORMAT_R8_UNORM && f <= VK_FORMAT_R8_SRGB
      || f == VK_FORMAT_S8_UINT)
      return 8;

    if (f >= VK_FORMAT_R4G4B4A4_UNORM_PACK16 && f <= VK_FORMAT_A1R5G5B5_UNORM_PACK16
      || f == VK_FORMAT_D16_UNORM
      || f >= VK_FORMAT_R8G8_UNORM && f <= VK_FORMAT_R8G8_SRGB
      || f >= VK_FORMAT_R16_UNORM && f <= VK_FORMAT_R16_SFLOAT)
      return 16;

    if (f >= VK_FORMAT_R8G8B8_UNORM && f <= VK_FORMAT_B8G8R8_SRGB
      || f == VK_FORMAT_D16_UNORM_S8_UINT)
      return 24;

    if (f >= VK_FORMAT_R8G8B8A8_UNORM && f <= VK_FORMAT_A2B10G10R10_SINT_PACK32
      || f >= VK_FORMAT_R16G16_UNORM && f <= VK_FORMAT_R16G16_SFLOAT
      || f >= VK_FORMAT_R32_UINT && f <= VK_FORMAT_R32_SFLOAT
      || f == VK_FORMAT_B10G11R11_UFLOAT_PACK32
      || f == VK_FORMAT_E5B9G9R9_UFLOAT_PACK32
      || f == VK_FORMAT_X8_D24_UNORM_PACK32
      || f == VK_FORMAT_D32_SFLOAT
      || f == VK_FORMAT_D24_UNORM_S8_UINT)
      return 32;

    if (f == VK_FORMAT_D32_SFLOAT_S8_UINT)
      return 40;

    if (f >= VK_FORMAT_R16G16B16_UNORM && f <= VK_FORMAT_R16G16B16_SFLOAT)
      return 48;

    if (f >= VK_FORMAT_R16G16B16A16_UNORM && f <= VK_FORMAT_R16G16B16A16_SFLOAT
      || f >= VK_FORMAT_R32G32_UINT && f <= VK_FORMAT_R32G32_SFLOAT
      || f >= VK_FORMAT_R64_UINT && f <= VK_FORMAT_R64_SFLOAT)
      return 64;

    if (f >= VK_FORMAT_R32G32B32_UINT && f <= VK_FORMAT_R32G32B32_SFLOAT)
      return 96;

    if (f >= VK_FORMAT_R32G32B32A32_UINT && f <= VK_FORMAT_R32G32B32A32_SFLOAT
      || f >= VK_FORMAT_R64G64_UINT && f <= VK_FORMAT_R64G64_SFLOAT)
      return 128;

    if (f >= VK_FORMAT_R64G64B64_UINT && f <= VK_FORMAT_R64G64B64_SFLOAT)
      return 160;

    if (f >= VK_FORMAT_R64G64B64A64_UINT && f <= VK_FORMAT_R64G64B64A64_SFLOAT)
      return 192;

    //if (f >= VK_FORMAT_BC1_RGB_UNORM_BLOCK) < VK_FORMAT_ASTC_12x12_SRGB_BLOCK

    //if (f >=
    return std::numeric_limits<uint32_t>::max();
  }

  template <typename T>
  using Ref = std::reference_wrapper<T>;

  /*class Ref {
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
    }*/
  //};
}

#endif
