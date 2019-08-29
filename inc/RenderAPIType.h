#ifndef DW_RENDERAPI_TYPE_H
#define DW_RENDERAPI_TYPE_H

enum RenderAPIType {
    API_INVALID,
    API_GL,
    API_VK,
    API_VKR
#ifdef _WIN32
    ,
    API_DX11,
    API_DX12,
    API_DXR
#endif
};

#endif
