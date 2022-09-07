#pragma once
#include "ScreenQuad.h"

// Stores z-buffer data in texture
class ZTexture
{
private:
	static  ZTexture*   instance;
	IDirect3DTexture9*  pZTexture;
	IDirect3DSurface9*  pBackBuffer;

	ZTexture();
	~ZTexture();

public:
	static ZTexture* Instance();
	void Init(int width, int height);
	void SetAsTarget();
	void RestoreTarget();
	IDirect3DTexture9* GetZTexture();
	static void Free();
};
