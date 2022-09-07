#include "ScreenQuad.h"
#include <string>
#include <stdexcept>
#include <iostream>

using namespace std;

const D3DVERTEXELEMENT9 ShadowVert::Decl[7] =
{
	{ 0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
	{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
	{ 0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
	{ 0, 36, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0 },
	{ 0, 52, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2 },
	{ 0, 64, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3 },
	D3DDECL_END()
};
LPDIRECT3DVERTEXDECLARATION9 ShadowVert::pVertexDecl = NULL;

ScreenQuad* ScreenQuad::instance;

ScreenQuad::ScreenQuad() : pVertexBuffer(NULL) {
	vertices[0] = D3DXVECTOR3(-1.0f, -1.0f, 0.0f);
	vertices[1] = D3DXVECTOR3(-1.0f, 1.0f, 0.0f);
	vertices[2] = D3DXVECTOR3(1.0f, -1.0f, 0.0f);

	vertices[3] = D3DXVECTOR3(1.0f, -1.0f, 0.0f);
	vertices[4] = D3DXVECTOR3(-1.0f, 1.0f, 0.0f);
	vertices[5] = D3DXVECTOR3(1.0f, 1.0f, 0.0f);
}

ScreenQuad::~ScreenQuad() {
}

ScreenQuad* ScreenQuad::Instance() {
	if (!instance)
		instance = new ScreenQuad();

	return instance;
}

void ScreenQuad::Init() {
	void*       copyData;

	pd3dDevice->CreateVertexBuffer(sizeof(vertices), 0, D3DFVF_XYZ, D3DPOOL_DEFAULT, &pVertexBuffer, NULL);

	pVertexBuffer->Lock(0, 0, &copyData, 0);
	memcpy(copyData, (void*)&vertices[0], sizeof(vertices));
	pVertexBuffer->Unlock();
};

void ScreenQuad::Render() {
	pd3dDevice->SetStreamSource(0, pVertexBuffer, 0, sizeof(D3DXVECTOR3));
	pd3dDevice->SetFVF(D3DFVF_XYZ);

	pLightingEffect->BeginPass(0);
	pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);
	pLightingEffect->EndPass();
}

void ScreenQuad::Free() {
	delete instance;
};

