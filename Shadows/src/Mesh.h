#pragma once
#include "ZTexture.h"

class Mesh {
private:
    LPD3DXMESH pMesh;
    D3DXVECTOR4 meshCenter;
    float meshRadius;
    std::vector<D3DMATERIAL9> materials;
    std::vector<Texture> textures;

    // Shadow data
    std::vector<D3DXVECTOR3> vertices;
    std::vector<D3DXVECTOR3> normals;
    std::vector<Face> faces;
    std::vector<Edge> edges;
    ShadowVolume shadowVolume;

    D3DXMATRIX transform;

    // Setup shader variables
    void    SetShaderConstants0(const D3DXMATRIX& world, const Light& light, const bool objSpace = false) const;
    void    SetShaderConstants1(const Light& light, const D3DMATERIAL9& material, const Texture& texture) const;

    // Make vbo/ibo for rendering
    void PrepareShadowVolumes();

    // Make vbo/ibo for rendering
    void UpdateShadowVolumes();

	// Copy vertices, etc...
	void PrepareShadowGeometry();

    // Add edge if it is unique
    void AddEdge(EdgeMap& edgeMap, int v0, int v1, int face);

    // Find mesh edges
    void MakeEdges();

    // Add edge to penumbra & umbra volume
    void AddEdgeToVolume(const int i);

public:
    Mesh();
    ~Mesh(void);

    void SetTransform(const D3DXMATRIX& matrix);
    void SetShadowConstants(const D3DXMATRIX& world, const Light& light) const;
    void Transform(const D3DXMATRIX& matrix);
    void Load(const char* name);
    void InitShadowForGPU();
    void ComputeShadowVolumes(const Light& light);
    bool IsClosed() const;
    void RenderAmbient(const D3DXMATRIX& world) const;
    void RenderZF(const D3DXMATRIX& world) const;
    // Render textured part
    void Render(const D3DXMATRIX& world, const Light& light) const;
    // Render untextured part
    void RenderTextured(const D3DXMATRIX& world, const Light& light) const;
    void RenderUmbra(int pass) const;
    void RenderPenumbra(int pass) const;
    void Clear();
};
