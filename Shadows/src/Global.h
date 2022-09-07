#pragma once

#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>

#pragma comment(lib, "d3d9.lib")

extern  HWND                hWnd;
extern  LPDIRECT3D9         pD3D;
extern  LPDIRECT3DDEVICE9   pd3dDevice;
extern  LPD3DXEFFECT        pLightingEffect;
extern  LPD3DXFONT          pFont;      
#define eps 0.0001f