#include "ZTexture.h"
using namespace std;

ZTexture*   ZTexture::instance;

ZTexture::ZTexture() : pZTexture(NULL), pBackBuffer(NULL) {

}

ZTexture::~ZTexture() {
	if(pZTexture)
		pZTexture->Release();
}

ZTexture* ZTexture::Instance() {
	if(!instance)
		instance = new ZTexture();
	return instance;
}

void ZTexture::Init(int width, int height) {
	D3DXCreateTexture(pd3dDevice, width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &pZTexture);
}

void ZTexture::SetAsTarget() {
	if(pZTexture) {
		LPDIRECT3DSURFACE9 pRenderSurface;
		pZTexture->GetSurfaceLevel(0, &pRenderSurface);
		pd3dDevice->GetRenderTarget(0, &pBackBuffer);
		pd3dDevice->SetRenderTarget(0, pRenderSurface);
	}
}

void ZTexture::RestoreTarget() {
	if (pBackBuffer)
		pd3dDevice->SetRenderTarget(0, pBackBuffer);
}

IDirect3DTexture9* ZTexture::GetZTexture() {
	return pZTexture;
}

void ZTexture::Free() {
	delete instance;
}