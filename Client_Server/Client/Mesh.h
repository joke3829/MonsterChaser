//-----------------------------------------------------------------------------
// File: Mesh.h
//-----------------------------------------------------------------------------
#pragma once

#include "stdafx.h"

extern DXResources g_DxResource;

class CHeightMapImage {
public:
	CHeightMapImage(const wchar_t* filePath, int nWidth, int nLength, XMFLOAT3& xmf3Scale);
	float GetHeight(int x, int z);
	float GetHeightinWorldSpace(float x, float z);

	std::unique_ptr<WORD[]> m_pHeightMapPixels;

	int m_nWidth;
	int m_nLength;

	XMFLOAT3 m_xmf3Scale;
};

// =============================================================================

constexpr short MESH_PLANE_1QUADRANT = 0;
constexpr short MESH_PLANE_2QUADRANT = 1;
constexpr short MESH_PLANE_3QUADRANT = 2;
constexpr short MESH_PLANE_4QUADRANT = 3;

class Mesh {
public:
	Mesh(std::ifstream& inFile, std::string strMeshName);		// ctor
	Mesh(CHeightMapImage* heightmap, std::string strMeshName);
	Mesh(XMFLOAT3& center, XMFLOAT3& extent, std::string meshName = "noNameMesh");					// boundingOBB ���鶧 ���
	Mesh(XMFLOAT3& center, float radius, std::string meshName = "noNameMesh");
	Mesh(XMFLOAT3& center, float width, float height, short arrow = MESH_PLANE_4QUADRANT);			// Plane

	//void GetMeshNameFromFile(std::ifstream& inFile);
	void GetBoundInfoFromFile(std::ifstream& inFile);
	void GetPositionFromFile(std::ifstream& inFile);
	void GetColorsFromFile(std::ifstream& inFile);
	void GetTexCoord0FromFile(std::ifstream& inFile);
	void GetTexCoord1FromFile(std::ifstream& inFile);
	void GetNormalFromFile(std::ifstream& inFile);
	void GetTangentFromFile(std::ifstream& inFile);
	void GetBiTangentFromFile(std::ifstream& inFile);
	void GetSubMeshesFromFile(std::ifstream& inFile);
	void MakeSubMesh(std::ifstream& inFile);

	void SetMeshName(std::string& name) { m_MeshName = name; }

	std::string getName() const;

	ID3D12Resource* getVertexBuffer() const;
	ID3D12Resource* getColorsBuffer() const;
	ID3D12Resource* getTexCoord0Buffer() const;
	ID3D12Resource* getTexCoord1Buffer() const;
	ID3D12Resource* getNormalsBuffer() const;
	ID3D12Resource* getTangentsBuffer() const;
	ID3D12Resource* getBiTangentsBuffer() const;
	ID3D12Resource* getIndexBuffer(UINT index) const;
	BoundingOrientedBox& getOBB() { return m_OBB; }

	UINT getVertexCount() const;
	UINT getIndexCount(int index) const;
	UINT getTexCoord0Count() const { return m_nTexCoord0Count; }

	bool getHasVertex() const;
	bool getHasColor() const;
	bool getHasTex0() const;
	bool getHasTex1() const;
	bool getHasNormal() const;
	bool getHasTangent() const;
	bool getHasBiTangent() const;
	bool getHasSubmesh() const;
	bool getbSkinning() const { return m_bSkinningMesh; }
	bool getHasBoundingBox() const { return m_bHasBoundingBox; }

	UINT getSubMeshCount() const;

	std::vector<XMFLOAT2>& getTex0() { return m_vTex0; }
	void setSkinning(bool bSkinning) { m_bSkinningMesh = bSkinning; }
protected:
private:
	std::string m_MeshName{};							// Mesh

	bool m_bHasBoundingBox = false;
	BoundingOrientedBox m_OBB{};						// Oriented_Bounding_Box

	UINT m_nVertexCount{};								// <Positions>
	ComPtr<ID3D12Resource> m_pd3dVertexBuffer{};		// DXGI_FORMAT_R32G32B32_FLOAT
	bool m_bHasVertex = false;

	UINT m_nColorCount{};								// <Colors>
	ComPtr<ID3D12Resource> m_pd3dColorsBuffer{};
	bool m_bHasColor = false;

	UINT m_nTexCoord0Count{};							// <TextureCoords0>
	std::vector<XMFLOAT2> m_vTex0{};
	ComPtr<ID3D12Resource> m_pd3dTexCoord0Buffer{};
	bool m_bHasTex0 = false;

	UINT m_nTexCoord1Count{};							// <TextureCoords1>
	ComPtr<ID3D12Resource> m_pd3dTexCoord1Buffer{};
	bool m_bHasTex1 = false;

	UINT m_nNormalsCount{};								// <Normals>
	ComPtr<ID3D12Resource> m_pd3dNormalsBuffer{};
	bool m_bHasNormals = false;

	UINT m_nTangentsCount{};							// <Tangents>
	ComPtr<ID3D12Resource> m_pd3dTangentsBuffer{};
	bool m_bHasTangents = false;

	UINT m_nBiTangentsCount{};							// <BiTangents>
	ComPtr<ID3D12Resource> m_pd3dBiTangentsBuffer{};
	bool m_bHasBiTangents = false;

	UINT m_nSubMeshesCount{};							// SubMesh
	std::vector<ComPtr<ID3D12Resource>> m_vSubMeshes;	// SubMesh(indexbuffer)
	std::vector<UINT> m_vIndices;
	bool m_bHasSubMeshes = false;

	bool m_bSkinningMesh = false;
};