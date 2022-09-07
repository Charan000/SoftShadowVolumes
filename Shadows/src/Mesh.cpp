#include "Mesh.h"
#include <string>
#include <stdexcept>
#include <iostream>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

using namespace std;
using namespace boost::lambda;

void Mesh::SetShaderConstants0(const D3DXMATRIX& world, const Light& light, const bool objSpace) const {
    D3DXMATRIX   normalMatrix;
    D3DXMATRIX   worldViewMatrix;
    D3DXMATRIX   worldViewProjMatrix;
    D3DXMATRIX   projMatrix;
    D3DXVECTOR4  lightPosition;

    D3DXMatrixMultiply(&worldViewMatrix, &transform, &world);
    normalMatrix = worldViewMatrix;
    normalMatrix._41 = normalMatrix._42 = normalMatrix._43 = 0.0f;

    pd3dDevice->GetTransform(D3DTS_PROJECTION, &projMatrix);
    D3DXMatrixMultiply(&worldViewProjMatrix, &worldViewMatrix, &projMatrix);

    if (objSpace) {
        D3DXMATRIX   invTransform;

        // From world space to object space
        D3DXMatrixInverse(&invTransform, NULL, &transform);
        D3DXVec4Transform(&lightPosition, &light.position, &invTransform);
    }
    else
        D3DXVec4Transform(&lightPosition, &light.position, &world);

    // Setup variables
    pLightingEffect->SetMatrix("normalMatrix", &normalMatrix);
    pLightingEffect->SetMatrix("worldViewMatrix", &worldViewMatrix);
    pLightingEffect->SetMatrix("worldViewProjMatrix", &worldViewProjMatrix);
    pLightingEffect->SetVector("lightPosition", &lightPosition);
    pLightingEffect->SetFloat("linearAttenuation", light.linearAttenuation);
    pLightingEffect->SetFloat("lightRange", light.range);
    pLightingEffect->SetFloat("lightRadius", light.radius);
}

void Mesh::SetShaderConstants1(const Light& light, const D3DMATERIAL9& material, const Texture& texture ) const {
    D3DXVECTOR4   diffProduct;
    D3DXVECTOR4   specProduct;

    diffProduct = D3DXVECTOR4( light.color.x * material.Diffuse.r,
                               light.color.y * material.Diffuse.g,
                               light.color.z * material.Diffuse.b,
                               light.color.w * material.Diffuse.a );

    specProduct = D3DXVECTOR4( light.color.x * material.Specular.r,
                               light.color.y * material.Specular.g,
                               light.color.z * material.Specular.b,
                               light.color.w * material.Specular.a );

    // Setup variables
    pLightingEffect->SetVector("diffuseProduct", &diffProduct);
    pLightingEffect->SetVector("specularProduct", &specProduct);

    if ( texture.Exist() ) 
        pLightingEffect->SetTexture("textureDiffuseColor", texture->pTexture);
}


// Setup constants for shadow technique
void Mesh::SetShadowConstants(const D3DXMATRIX& world, const Light& light) const {
    D3DXMATRIX   projMatrix;
    D3DXMATRIX   worldViewMatrix;
    D3DXMATRIX   worldViewProjMatrix;
    D3DXVECTOR4  lightPosition;
    D3DXMATRIX   invTransform;

    D3DXMatrixMultiply(&worldViewMatrix, &transform, &world);
    pd3dDevice->GetTransform(D3DTS_PROJECTION, &projMatrix);
    D3DXMatrixMultiply(&worldViewProjMatrix, &worldViewMatrix, &projMatrix);

    // From world space to object space
    D3DXMatrixInverse(&invTransform, NULL, &transform);
    D3DXVec4Transform(&lightPosition, &light.position, &invTransform);
    D3DXMatrixInverse(&invTransform, NULL, &worldViewProjMatrix);

    // Setup variables
    pLightingEffect->SetMatrix("invTransform", &invTransform);
    pLightingEffect->SetMatrix("normalMatrix", &worldViewMatrix);
    pLightingEffect->SetMatrix("worldViewMatrix", &worldViewMatrix);
    pLightingEffect->SetMatrix("worldViewProjMatrix", &worldViewProjMatrix);
    pLightingEffect->SetVector("lightPosition", &lightPosition);
    pLightingEffect->SetFloat("lightRange", light.range);
    pLightingEffect->SetFloat("lightRadius", light.radius);
    pLightingEffect->SetTexture("zTexture", ZTexture::Instance()->GetZTexture());
}

// Make vbo/ibo for rendering
void Mesh::PrepareShadowVolumes() {
    void*       copyData;
	int      bufferSize;   

	// Create vertex buffer from our device
    bufferSize = shadowVolume.vertices.size() * sizeof(ShadowVert);
    pd3dDevice->CreateVertexBuffer(bufferSize, 0, NULL, D3DPOOL_MANAGED, &shadowVolume.pVertexBuffer, NULL);
	
	shadowVolume.pVertexBuffer->Lock(0, 0, &copyData, 0);
	memcpy(copyData, (void*)&shadowVolume.vertices[0], bufferSize);
	shadowVolume.pVertexBuffer->Unlock();
	
	if (!ShadowVert::pVertexDecl)
    {
		// New vertex declaration
        pd3dDevice->CreateVertexDeclaration(ShadowVert::Decl, &ShadowVert::pVertexDecl);
    }
    UpdateShadowVolumes();
}

// Make vbo/ibo for rendering
void Mesh::UpdateShadowVolumes() {
    void*       copyData;
	int      bufferSize;

    // Umbra
	bufferSize = shadowVolume.umbraIndices.size() * sizeof(int);
    if (bufferSize > shadowVolume.umbraIboSize)
	{
		if (shadowVolume.pUmbraIndexBuffer) 
            shadowVolume.pUmbraIndexBuffer->Release();

		// Create index buffer from our device
        pd3dDevice->CreateIndexBuffer(bufferSize, 0, D3DFMT_INDEX32, D3DPOOL_MANAGED, &shadowVolume.pUmbraIndexBuffer, NULL );
		
		// new size
		shadowVolume.umbraIboSize = bufferSize;
	}

	// Copying indices
    shadowVolume.pUmbraIndexBuffer->Lock(0, 0, &copyData, 0);
    memcpy(copyData, (void*)&shadowVolume.umbraIndices[0], bufferSize);
    shadowVolume.pUmbraIndexBuffer->Unlock();
	
    // Penumbra
    // Don't recreate ibo if it is smaller than existing
	bufferSize = shadowVolume.penumbraIndices.size() * sizeof(int);
    if (bufferSize > shadowVolume.penumbraIboSize) {
		if (shadowVolume.pPenumbraIndexBuffer) 
            shadowVolume.pPenumbraIndexBuffer->Release();

		// Create index buffer from our device
        pd3dDevice->CreateIndexBuffer(bufferSize, 0, D3DFMT_INDEX32, D3DPOOL_MANAGED, &shadowVolume.pPenumbraIndexBuffer, NULL );
		
		// new size
		shadowVolume.penumbraIboSize = bufferSize;
	}

	// Copying indices
    shadowVolume.pPenumbraIndexBuffer->Lock(0, 0, &copyData, 0);
	memcpy(copyData, (void*)&shadowVolume.penumbraIndices[0], bufferSize);
	shadowVolume.pPenumbraIndexBuffer->Unlock();
}

void Mesh::AddEdge(EdgeMap& edgeMap, int v0, int v1, int face) {
	bool rev = false;

	// from min to max
	if (v0 > v1) {
		std::swap(v0, v1);
		rev = true;
	}

	// check whether we already have this face
	EdgeMap::iterator i = edgeMap.find( int_pair(v0, v1) );
	if (i != edgeMap.end()) 
		i->second.f1 = face;
	else {
		// insert new edge
		Edge edge;

        if (rev) {
		    edge.v0 = v1;
		    edge.v1 = v0;
        }
        else {
		    edge.v0 = v0;
		    edge.v1 = v1;
        }
		edge.f0	= face;
		edge.f1 = -1;

		edgeMap.insert(edge_pair(int_pair(v0,v1), edge));
	}
}

void Mesh::MakeEdges() {
    int j = 0;
    EdgeMap edgeMap;

    // Add edge from each face
	for (int i = 0; i<faces.size(); ++i) {
		AddEdge(edgeMap, faces[i].v0, faces[i].v1, i);
		AddEdge(edgeMap, faces[i].v1, faces[i].v2, i);
		AddEdge(edgeMap, faces[i].v2, faces[i].v0, i);
	}

    // Check closed edges and copy edges to vector
    edges.reserve(edgeMap.size());
    for(EdgeMap::iterator i = edgeMap.begin(); i != edgeMap.end(); ++i, ++j) {
        if (i->second.f0 == -1 || i->second.f1 == -1) {
            edges.clear();
            return;
        }
        else {
		    // copy
            if (faces[i->second.f0].v0 == i->second.v0 && faces[i->second.f0].v1 == i->second.v1) {
                faces[i->second.f0].re0 = false;
                faces[i->second.f0].e0 = j;
            }
            else if (faces[i->second.f0].v1 == i->second.v0 && faces[i->second.f0].v2 == i->second.v1) {
                faces[i->second.f0].re1 = false;
                faces[i->second.f0].e1 = j;
            }
            else if (faces[i->second.f0].v2 == i->second.v0 && faces[i->second.f0].v0 == i->second.v1) {
                faces[i->second.f0].re2 = false;
                faces[i->second.f0].e2 = j;
            }

            if (faces[i->second.f1].v0 == i->second.v1 && faces[i->second.f1].v1 == i->second.v0) {
                faces[i->second.f1].re0 = true;
                faces[i->second.f1].e0 = j;
            }
            else if (faces[i->second.f1].v1 == i->second.v1 && faces[i->second.f1].v2 == i->second.v0) {
                faces[i->second.f1].re1 = true;
                faces[i->second.f1].e1 = j;
            }
            else if (faces[i->second.f1].v2 == i->second.v1 && faces[i->second.f1].v0 == i->second.v0) {
                faces[i->second.f1].re2 = true;
                faces[i->second.f1].e2 = j;
            }
            edges.push_back(i->second);
        }
    }
}

Mesh::Mesh():pMesh(NULL) {
    D3DXMatrixIdentity(&transform);
}

Mesh::~Mesh(void) {
    if (pMesh) 
		pMesh->Release();
}

// Setup mesh transformation matrix
void Mesh::SetTransform(const D3DXMATRIX& matrix)
{
    transform = matrix;
}

// Transform mesh transformation matrix
void Mesh::Transform(const D3DXMATRIX& matrix)
{
    D3DXMatrixMultiply(&transform, &transform, &matrix);
}

void Mesh::Load(const char* name) {

    ID3DXBuffer*     pD3DXMtrlBuffer;
    D3DXMATERIAL*    d3dxMaterials;
    DWORD            numMaterials;
    string           folder;

    // Load the mesh from the specified file
    D3DXLoadMeshFromXA(name, D3DXMESH_SYSTEMMEM, pd3dDevice, NULL, &pD3DXMtrlBuffer, NULL, &numMaterials, &pMesh);
    
    // Get folder of the path
    string path(name);
    int pos = path.rfind("/");
    if (pos == string::npos) {
        pos = path.rfind("\\");
        if (pos == string::npos) 
            pos = 0;
    }
    if (pos != string::npos)
        folder = path.substr(0, pos + 1);

    // Load materials & textures
    d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();        
    materials.resize(numMaterials);
    textures.resize(numMaterials);
    
    // Load materials
    for(int i = 0; i<numMaterials; ++i) {
        materials[i] = d3dxMaterials[i].MatD3D;
        materials[i].Ambient = materials[i].Diffuse;

        // Create the texture
        if (d3dxMaterials[i].pTextureFilename) {
            string          fullName = folder + d3dxMaterials[i].pTextureFilename;

            textures[i] = TextureStorage::Instance()->Get(fullName);
            if ( textures[i] == TextureStorage::Instance()->End() )
            {
                TextureData* texture = new TextureData();
                D3DXCreateTextureFromFileA( pd3dDevice, 
                                                 fullName.c_str(), 
                                                 &texture->pTexture );
                textures[i] = TextureStorage::Instance()->Add(fullName, texture);
                
            }
        }
        else
        {
            textures[i] = TextureStorage::Instance()->End();
        }
    }

    // No more need
    pD3DXMtrlBuffer->Release();

    // Misc
    PrepareShadowGeometry();
    if (shadowVolume.vertices.size() > 0)
        PrepareShadowVolumes();
}

// Get normal of the plane
D3DXVECTOR3 ComputeNormal(const D3DXVECTOR3& v0, const D3DXVECTOR3& v1, const D3DXVECTOR3& v2) {
    D3DXVECTOR3 normal;

    D3DXVec3Cross(&normal, &(v1 - v0), &(v2 - v0));
    D3DXVec3Normalize(&normal, &normal);

    return normal;
}

bool operator < (const D3DXVECTOR3& left, const D3DXVECTOR3& right) {
    float dx = left.x - right.x;
    float dy = left.y - right.y;
    float dz = left.z - right.z;

    if (dx < -eps)
        return true;
    else if (fabs(dx) <= eps && dy < -eps)
        return true;
    else if (fabs(dx) <= eps && fabs(dy) < eps && dz < -eps)
        return true;

    return false;
}

// Copy vertices, etc...
void Mesh::PrepareShadowGeometry() {
    
	typedef map<D3DXVECTOR3, int> VertexMap;
    char* pData;
	D3DVERTEXELEMENT9 decl[MAX_FVF_DECL_SIZE];
    VertexMap vertexMap;
	int positionStride;
	int elemSize;
	int size;
    bool ind32;

	pMesh->GetDeclaration(decl);
	
	// get vertices
	pMesh->LockVertexBuffer( D3DLOCK_READONLY, (LPVOID*)&pData );
	
	// Find position decl. Determine vertex size
	positionStride = find_if( decl, decl + MAX_FVF_DECL_SIZE, bind(&D3DVERTEXELEMENT9::Usage, _1) == D3DDECLUSAGE_POSITION )->Offset;

	// Copy vertices
    size = pMesh->GetNumVertices();
    elemSize = pMesh->GetNumBytesPerVertex();
    vertices.resize(size); 
    for(int i = 0; i<vertices.size(); ++i) {
        memcpy(&vertices[i], pData + i * elemSize + positionStride, sizeof(D3DXVECTOR3));
        vertexMap.insert( pair<D3DXVECTOR3, int>( vertices[i], vertexMap.size() ) );
    }
	
    pMesh->UnlockVertexBuffer();

    // get faces
	pMesh->LockIndexBuffer( D3DLOCK_READONLY, (LPVOID*)&pData );
	
    // Copy faces
    size = pMesh->GetNumVertices();
    ind32 = pMesh->GetOptions() & D3DXMESH_32BIT; // check size of indices
    faces.resize( pMesh->GetNumFaces() );
    pData -= ind32 ? 12 : 2;
    for(int i = 0; i<faces.size(); ++i)
    {
        // Get data
        if (ind32)
            memcpy(&faces[i], pData += 12, 12);
        else
        {
            faces[i].v0 = *((unsigned short*)(pData += 2));
            faces[i].v1 = *((unsigned short*)(pData += 2));
            faces[i].v2 = *((unsigned short*)(pData += 2));
        }
        faces[i].normal = ComputeNormal(vertices[faces[i].v0], vertices[faces[i].v1], vertices[faces[i].v2]);

        faces[i].v0 = vertexMap.find( vertices[faces[i].v0] )->second;
        faces[i].v1 = vertexMap.find( vertices[faces[i].v1] )->second;
        faces[i].v2 = vertexMap.find( vertices[faces[i].v2] )->second;
    }   

    pMesh->UnlockIndexBuffer();

    // Reorder vertices
    vertices.resize( vertexMap.size() );
    for(VertexMap::iterator i = vertexMap.begin(); i != vertexMap.end(); ++i)
        vertices[i->second] = i->first;

    // Compute vertex normals
    normals.resize( vertices.size() );
    fill( normals.begin(), normals.end(), D3DXVECTOR3(0, 0, 0) );
    for(int i = 0; i<faces.size(); ++i)
    {
        normals[ faces[i].v0 ] += faces[i].normal;
        normals[ faces[i].v1 ] += faces[i].normal;
        normals[ faces[i].v2 ] += faces[i].normal;
    }
    for(int i = 0; i<vertices.size(); ++i)
    {
        D3DXVec3Normalize(&normals[i], &normals[i]);
    }

    // Edges
    MakeEdges();
    if (edges.size() == 0)
        return;

    // find center of the mesh
    D3DXVECTOR3 meshCenter3 = D3DXVECTOR3(0.0, 0.0, 0.0);
    for(int i = 0; i<vertices.size(); ++i)
    {
        meshCenter3 += vertices[i];
    }
    meshCenter3 /= static_cast<float>( vertices.size() );
    meshCenter = D3DXVECTOR4(meshCenter3, 1.0f);

    meshRadius = 0.0f;
    for(int i = 0; i<vertices.size(); ++i)
    {
        meshRadius = max( meshRadius, D3DXVec3Length( &(meshCenter3 - vertices[i]) ) );
    }

	// copy each vertex twice 
	//  first extruded/second not
    size = edges.size();
	shadowVolume.vertices.resize( 6 * size );
	for(int i = 0; i<size; ++i)
	{
        D3DXVECTOR3 edge = vertices[ edges[i].v1 ] - vertices[ edges[i].v0 ];
        //D3DXVec3Normalize(&edge, &edge);

        // v0
        shadowVolume.vertices[i].vertex = vertices[ edges[i].v0 ];
        shadowVolume.vertices[i].vertNormal0 = normals[ edges[i].v0 ];
        shadowVolume.vertices[i].vertNormal1 = normals[ edges[i].v1 ];
        shadowVolume.vertices[i].normal = D3DXVECTOR4(faces[ edges[i].f0 ].normal, 0.0f);
        shadowVolume.vertices[i].backNormal = faces[ edges[i].f1 ].normal;
        shadowVolume.vertices[i].edge = D3DXVECTOR4(edge, 1.0f);

		shadowVolume.vertices[i + size].vertex = vertices[ edges[i].v0 ];
        shadowVolume.vertices[i + size].vertNormal0 = normals[ edges[i].v0 ];
        shadowVolume.vertices[i + size].vertNormal1 = normals[ edges[i].v1 ];
		shadowVolume.vertices[i + size].normal = D3DXVECTOR4(faces[ edges[i].f1 ].normal, 1.0f);
		shadowVolume.vertices[i + size].backNormal = faces[ edges[i].f0 ].normal;
        shadowVolume.vertices[i + size].edge = D3DXVECTOR4(edge, 1.0f);

		shadowVolume.vertices[i + 2*size].vertex = vertices[ edges[i].v0 ];
        shadowVolume.vertices[i + 2*size].vertNormal0 = normals[ edges[i].v0 ];
        shadowVolume.vertices[i + 2*size].vertNormal1 = normals[ edges[i].v1 ];
		shadowVolume.vertices[i + 2*size].normal = D3DXVECTOR4(faces[ edges[i].f1 ].normal, -1.0f);
		shadowVolume.vertices[i + 2*size].backNormal = faces[ edges[i].f0 ].normal;
        shadowVolume.vertices[i + 2*size].edge = D3DXVECTOR4(edge, 1.0f);

        // v1
		shadowVolume.vertices[i + 3*size].vertex = vertices[ edges[i].v1 ];
        shadowVolume.vertices[i + 3*size].vertNormal0 = normals[ edges[i].v1 ];
        shadowVolume.vertices[i + 3*size].vertNormal1 = normals[ edges[i].v0 ];
		shadowVolume.vertices[i + 3*size].normal = D3DXVECTOR4(faces[ edges[i].f0 ].normal, 0.0f);
		shadowVolume.vertices[i + 3*size].backNormal = faces[ edges[i].f1 ].normal;
        shadowVolume.vertices[i + 3*size].edge = D3DXVECTOR4(-edge, -1.0f);

		shadowVolume.vertices[i + 4*size].vertex = vertices[ edges[i].v1 ];
        shadowVolume.vertices[i + 4*size].vertNormal0 = normals[ edges[i].v1 ];
        shadowVolume.vertices[i + 4*size].vertNormal1 = normals[ edges[i].v0 ];
		shadowVolume.vertices[i + 4*size].normal = D3DXVECTOR4(faces[ edges[i].f1 ].normal, 1.0f);
		shadowVolume.vertices[i + 4*size].backNormal = faces[ edges[i].f0 ].normal;
        shadowVolume.vertices[i + 4*size].edge = D3DXVECTOR4(-edge, -1.0f);

		shadowVolume.vertices[i + 5*size].vertex = vertices[ edges[i].v1 ];
        shadowVolume.vertices[i + 5*size].vertNormal0 = normals[ edges[i].v1 ];
        shadowVolume.vertices[i + 5*size].vertNormal1 = normals[ edges[i].v0 ];
		shadowVolume.vertices[i + 5*size].normal = D3DXVECTOR4(faces[ edges[i].f1 ].normal, -1.0f);
		shadowVolume.vertices[i + 5*size].backNormal = faces[ edges[i].f0 ].normal;
        shadowVolume.vertices[i + 5*size].edge = D3DXVECTOR4(-edge, -1.0f);
	}

    // make umbra
    shadowVolume.umbraIndices.resize( faces.size()*3 + edges.size()*6 );
    // add faces
	for(int i = 0; i<faces.size(); ++i)
	{
        if ( faces[i].re0 )
            shadowVolume.umbraIndices[i*3] = faces[i].e0 + 4*size;
        else
            shadowVolume.umbraIndices[i*3] = faces[i].e0;
 
        if ( faces[i].re1 )
            shadowVolume.umbraIndices[i*3 + 1] = faces[i].e1 + 4*size;
        else
            shadowVolume.umbraIndices[i*3 + 1] = faces[i].e1;

        if ( faces[i].re2 )
            shadowVolume.umbraIndices[i*3 + 2] = faces[i].e2 + 4*size;
        else
            shadowVolume.umbraIndices[i*3 + 2] = faces[i].e2;
    }

    InitShadowForGPU();
}

void Mesh::AddEdgeToVolume(const int i)
{
    int size = edges.size();
    int j    = shadowVolume.umbraIndices.size();
    
    shadowVolume.umbraIndices.resize(j+6);

    // front
    shadowVolume.umbraIndices[j] = i + 3*size;
    shadowVolume.umbraIndices[j+1] = i;
    shadowVolume.umbraIndices[j+2] = i + 5*size;

    shadowVolume.umbraIndices[j+3] = i + 5*size;
    shadowVolume.umbraIndices[j+4] = i;
    shadowVolume.umbraIndices[j+5] = i + 2*size;

    j = shadowVolume.penumbraIndices.size();
    
    
    shadowVolume.penumbraIndices.resize(j+24);

    // inner
    shadowVolume.penumbraIndices[j] = i + 3*size;
    shadowVolume.penumbraIndices[j+1] = i;
    shadowVolume.penumbraIndices[j+2] = i + size;

    shadowVolume.penumbraIndices[j+3] = i + size;
    shadowVolume.penumbraIndices[j+4] = i + 4*size;
    shadowVolume.penumbraIndices[j+5] = i + 3*size;

    // left
    shadowVolume.penumbraIndices[j+6] = i + size;
    shadowVolume.penumbraIndices[j+7] = i;
    shadowVolume.penumbraIndices[j+8] = i + 2*size;

    // right
    shadowVolume.penumbraIndices[j+9] = i + 5*size;
    shadowVolume.penumbraIndices[j+10] = i + 3*size;
    shadowVolume.penumbraIndices[j+11] = i + 4*size;

    // front
    shadowVolume.penumbraIndices[j+12] = i;
    shadowVolume.penumbraIndices[j+13] = i + 3*size;
    shadowVolume.penumbraIndices[j+14] = i + 5*size;

    shadowVolume.penumbraIndices[j+15] = i + 5*size;
    shadowVolume.penumbraIndices[j+16] = i + 2*size;
    shadowVolume.penumbraIndices[j+17] = i;

    
	// back
	shadowVolume.penumbraIndices[j + 18] = i + size;
	shadowVolume.penumbraIndices[j + 19] = i + 2 * size;
	shadowVolume.penumbraIndices[j + 20] = i + 4 * size;

	shadowVolume.penumbraIndices[j + 21] = i + 4 * size;
	shadowVolume.penumbraIndices[j + 22] = i + 2 * size;
	shadowVolume.penumbraIndices[j + 23] = i + 5 * size;
    
}

// Make penumbra for rendering fully on gpu
void Mesh::InitShadowForGPU() {
    int size = edges.size();

    // add edges
    shadowVolume.umbraIndices.resize( faces.size()*3 + size*6 );
    for(int i = 0; i<edges.size(); ++i)
	{   
        int j = faces.size()*3 + i*6;

        shadowVolume.umbraIndices[j] = i + 3*size; // 3 0 5
        shadowVolume.umbraIndices[j+1] = i;
        shadowVolume.umbraIndices[j+2] = i + 5*size;

        // to extrude
        shadowVolume.umbraIndices[j+3] = i + 5*size;//5 0 2
        shadowVolume.umbraIndices[j+4] = i;
        shadowVolume.umbraIndices[j+5] = i + 2*size;
    }

    // make penumbra
    shadowVolume.penumbraIndices.resize( size*24 );
	for(int i = 0; i<edges.size(); ++i)
	{
        int j = i*24;

        // inner
        shadowVolume.penumbraIndices[j] = i + 3*size;
        shadowVolume.penumbraIndices[j+1] = i;
        shadowVolume.penumbraIndices[j+2] = i + size;

        shadowVolume.penumbraIndices[j+3] = i + size;
        shadowVolume.penumbraIndices[j+4] = i + 4*size;
        shadowVolume.penumbraIndices[j+5] = i + 3*size;

        // left
        shadowVolume.penumbraIndices[j+6] = i + size;
        shadowVolume.penumbraIndices[j+7] = i;
        shadowVolume.penumbraIndices[j+8] = i + 2*size;

        // right
        shadowVolume.penumbraIndices[j+9] = i + 5*size;
        shadowVolume.penumbraIndices[j+10] = i + 3*size;
        shadowVolume.penumbraIndices[j+11] = i + 4*size;

        // front
        shadowVolume.penumbraIndices[j+12] = i;
        shadowVolume.penumbraIndices[j+13] = i + 3*size;
        shadowVolume.penumbraIndices[j+14] = i + 5*size;

        shadowVolume.penumbraIndices[j+15] = i + 5*size;
        shadowVolume.penumbraIndices[j+16] = i + 2*size;
        shadowVolume.penumbraIndices[j+17] = i;

        // back
        shadowVolume.penumbraIndices[j+18] = i + size;
        shadowVolume.penumbraIndices[j+19] = i + 2*size;
        shadowVolume.penumbraIndices[j+20] = i + 4*size;

        shadowVolume.penumbraIndices[j+21] = i + 4*size;
        shadowVolume.penumbraIndices[j+22] = i + 2*size;
        shadowVolume.penumbraIndices[j+23] = i + 5*size;
    }

    UpdateShadowVolumes();
}

// Compute volumes to render shadows
void Mesh::ComputeShadowVolumes(const Light& light) {  
    D3DXMATRIX      invTransform;
    D3DXVECTOR3     lightPos;
    D3DXVECTOR4     tmp;

    D3DXMatrixInverse(&invTransform, NULL, &transform);
    D3DXVec4Transform(&tmp, &light.position, &invTransform);
    lightPos = D3DXVECTOR3(tmp.x, tmp.y, tmp.z);

    shadowVolume.silhouettePlane = D3DXVECTOR4(lightPos, 0.0f);
    D3DXVec4Normalize(&shadowVolume.silhouettePlane, &shadowVolume.silhouettePlane);
    shadowVolume.silhouettePlane.w = -D3DXVec4Dot(&shadowVolume.silhouettePlane, &light.position);

    // From world space to object space
    D3DXMatrixInverse(&invTransform, NULL, &transform);
    D3DXVec4Transform(&tmp, &light.position, &invTransform);
    lightPos = D3DXVECTOR3(tmp.x, tmp.y, tmp.z);

    
        vector<bool> frontFace;
        int offs = vertices.size();

	    // Check front or back faces
        shadowVolume.umbraIndices.resize( faces.size() * 3 );
        shadowVolume.penumbraIndices.clear();
        frontFace.resize(faces.size());
	    for(int i=0; i<faces.size(); ++i) {
		    frontFace[i] = D3DXVec3Dot(&faces[i].normal, &(lightPos - vertices[faces[i].v0])) > 0.0f;
        }

	    // Add silhouette indices
	    for(int i = 0; i<edges.size(); ++i) {
		    Edge edge = edges[i];
            Face f0 = faces[edge.f0];
            Face f1 = faces[edge.f1];

		    // Check silhouette edge
		    if (!frontFace[edge.f0] && frontFace[edge.f1]) {
                AddEdgeToVolume(i);
            }
            else if (frontFace[edge.f0] && !frontFace[edge.f1]) {
                AddEdgeToVolume(i);
            }
	    }
        UpdateShadowVolumes();
    
}

// Render ambient part
void Mesh::RenderAmbient(const D3DXMATRIX& world) const {
    D3DXMATRIX   result;

    // World transform
    D3DXMatrixMultiply(&result, &transform, &world);
    pd3dDevice->SetTransform(D3DTS_WORLD, &result);

    // Render subsets
    for(int i = 0; i<materials.size(); ++i)
    {
        pd3dDevice->SetMaterial(&materials[i]);

        if ( textures[i] != TextureStorage::Instance()->End() )
            pd3dDevice->SetTexture(0, textures[i]->pTexture);
        else
            pd3dDevice->SetTexture(0, 0);


        pMesh->DrawSubset(i);
    }
}

// Render pass to fill depth texture
void Mesh::RenderZF(const D3DXMATRIX& world) const {
    // World transform
    D3DXMATRIX   worldViewMatrix;
    D3DXMATRIX   worldViewProjMatrix;
    D3DXMATRIX   projMatrix;

    D3DXMatrixMultiply(&worldViewMatrix, &transform, &world);
    pd3dDevice->GetTransform(D3DTS_PROJECTION, &projMatrix);
    D3DXMatrixMultiply(&worldViewProjMatrix, &worldViewMatrix, &projMatrix);

    // Setup variables
    pLightingEffect->SetMatrix("projMatrix", &projMatrix);
    pLightingEffect->SetMatrix("worldViewMatrix", &worldViewMatrix);
    pLightingEffect->SetMatrix("worldViewProjMatrix", &worldViewProjMatrix);

    // Render subsets
    pLightingEffect->BeginPass(0);
    for(int i = 0; i<materials.size(); ++i)
        pMesh->DrawSubset(i);
    pLightingEffect->EndPass();
}

// Check when mesh faces are closed
bool Mesh::IsClosed() const {
    return edges.size() > 0;
}

// Render
void Mesh::Render(const D3DXMATRIX& world, const Light& light) const {
    SetShaderConstants0(world, light);

    // Render subsets
    for(int i = 0; i<materials.size(); ++i) {
        if (!textures[i].Exist()) {
            SetShaderConstants1(light, materials[i], textures[i]);
            pLightingEffect->BeginPass(0);
            pMesh->DrawSubset(i);
            pLightingEffect->EndPass();
        }
    }
 }

// Render
void Mesh::RenderTextured(const D3DXMATRIX& world, const Light& light) const {
    // Render subsets
    for(int i = 0; i<materials.size(); ++i) {
        if (textures[i].Exist()) {
            SetShaderConstants1(light, materials[i], textures[i]);
            pLightingEffect->BeginPass(1);
            pMesh->DrawSubset(i);
            pLightingEffect->EndPass();
        }
    }
 }

// Render umbra volume
void Mesh::RenderUmbra(int pass) const {
    // Set source
    pd3dDevice->SetVertexDeclaration(ShadowVert::pVertexDecl);
	pd3dDevice->SetStreamSource(0, shadowVolume.pVertexBuffer, 0, sizeof(ShadowVert));
	pd3dDevice->SetIndices(shadowVolume.pUmbraIndexBuffer);

    // draw
    pLightingEffect->BeginPass(pass);
	pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, shadowVolume.vertices.size(), 0, shadowVolume.umbraIndices.size()/3);
    pLightingEffect->EndPass();
}

// Render penumbra volume
void Mesh::RenderPenumbra(int pass) const
{
    // Set source
    pd3dDevice->SetVertexDeclaration(ShadowVert::pVertexDecl);
	pd3dDevice->SetStreamSource(0, shadowVolume.pVertexBuffer, 0, sizeof(ShadowVert));
	pd3dDevice->SetIndices(shadowVolume.pPenumbraIndexBuffer);

    // draw
    pLightingEffect->BeginPass(pass);
	pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, shadowVolume.vertices.size(), 0, shadowVolume.penumbraIndices.size()/3 );
    pLightingEffect->EndPass();
}

// Release mesh resources
void Mesh::Clear()
{
    textures.clear();
    if (pMesh) pMesh->Release();
    pMesh = NULL;
}