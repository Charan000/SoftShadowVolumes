#pragma once
#include "Global.h"
#include "Storage.h"
#include <vector>
#include <assert.h>
#include <algorithm>

struct Light
{
	float radius;
	float range;
	float linearAttenuation;
	D3DXVECTOR4 position;
	D3DXVECTOR4 color;
};

struct Camera
{
	float yaw;
	float pitch;
	float radius;
	D3DXVECTOR3 eyePt;
	D3DXVECTOR3 up;
};

struct TextureData
{
	LPDIRECT3DTEXTURE9 pTexture;
	TextureData() : pTexture(NULL) {}
	~TextureData() { if (pTexture) pTexture->Release(); }
};
typedef Utils::Storage<TextureData, std::string> TextureStorage;
typedef TextureStorage::handle Texture;

struct ShadowVert
{
	D3DXVECTOR3 vertex;
	D3DXVECTOR3 vertNormal0;
	D3DXVECTOR3 vertNormal1;
	D3DXVECTOR4 normal;
	D3DXVECTOR3 backNormal;
	D3DXVECTOR4 edge;

	const static D3DVERTEXELEMENT9 Decl[7];
	static LPDIRECT3DVERTEXDECLARATION9 pVertexDecl;
};

struct Face
{
	int v0, v1, v2;
	bool re0, re1, re2;
	int e0, e1, e2;
	D3DXVECTOR3 normal;
};

struct Edge
{
	int v0, v1;
	int f0, f1;
};
typedef std::pair<int, int>	int_pair;
typedef std::pair<int_pair, Edge> edge_pair;
typedef std::map<int_pair, Edge> EdgeMap;

struct ShadowVolume
{
	std::vector<ShadowVert> vertices;
	std::vector<int> umbraIndices;
	std::vector<int> penumbraIndices;

	IDirect3DVertexBuffer9* pVertexBuffer;
	IDirect3DIndexBuffer9*  pUmbraIndexBuffer;
	IDirect3DIndexBuffer9*  pPenumbraIndexBuffer;
	D3DXVECTOR4 silhouettePlane; // plane containing silhouette
	D3DXVECTOR4 silhouetteCenter; // center of the silhouette
	int penumbraIboSize;
	int umbraIboSize;

	ShadowVolume() :
		pVertexBuffer(NULL),
		pUmbraIndexBuffer(NULL),
		pPenumbraIndexBuffer(NULL),
		penumbraIboSize(0),
		umbraIboSize(0)
	{
	}

	~ShadowVolume()
	{
		if (pVertexBuffer) pVertexBuffer->Release();
		if (pUmbraIndexBuffer) pUmbraIndexBuffer->Release();
		if (pPenumbraIndexBuffer) pPenumbraIndexBuffer->Release();
	}
};

class ScreenQuad {
private:
	static  ScreenQuad*         instance;
	D3DXVECTOR3                 vertices[6];
	IDirect3DVertexBuffer9*     pVertexBuffer;

	ScreenQuad();
	~ScreenQuad();

public:
	static ScreenQuad*  Instance();
	void                Init();
	void                Render();
	static void         Free();
};