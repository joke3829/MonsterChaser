#include "Mesh.h"

CHeightMapImage::CHeightMapImage(const wchar_t* filePath, int nWidth, int nLength, XMFLOAT3& xmf3Scale)
{
	m_nWidth = nWidth;
	m_nLength = nLength;
	m_xmf3Scale = xmf3Scale;

	std::ifstream inFile{ filePath, std::ios::binary };
	std::vector<WORD> v(m_nWidth * m_nLength);
	inFile.read((char*)v.data(), sizeof(WORD) * m_nLength * m_nWidth);

	m_pHeightMapPixels = std::make_unique<WORD[]>(m_nWidth * m_nLength);
	for (int z = 0; z < m_nLength; ++z) {
		for (int x = 0; x < m_nWidth; ++x) {
			//m_pHeightMapPixels[x + ((m_nLength - 1 - z) * m_nWidth)] = v[x + (z * m_nWidth)];//pHeightMapPixels[x + (z * m_nWidth)];
			m_pHeightMapPixels[x + (z * m_nWidth)] = v[x + (z * m_nWidth)];
		}
	}
}

float CHeightMapImage::GetHeight(int x, int z)
{
	return m_pHeightMapPixels[x + (z * m_nWidth)] * m_xmf3Scale.y;
}

float CHeightMapImage::GetHeightinWorldSpace(float x, float z)
{
	// need to check
	float lx = x / m_xmf3Scale.x;
	float lz = z / m_xmf3Scale.z;

	if (lx < 0 || lx >= m_nWidth || lz < 0 || lz >= m_nLength)
		return 0.0f;

	int ix = static_cast<int>(lx);
	int iz = static_cast<int>(lz);

	float xRatio = lx - ix;
	float zRatio = lz - iz;

	auto myLerp = [](float& f1, float& f2, float ratio) {
		return ((1.0f - ratio) * f1) + ((ratio)*f2);
		};

	float x0z0 = GetHeight(ix, iz); float x1z0 = GetHeight(ix + 1, iz);
	float x0z1 = GetHeight(ix, iz + 1); float x1z1 = GetHeight(ix + 1, iz + 1);

	float lerp1 = myLerp(x0z0, x0z1, zRatio);
	float lerp2 = myLerp(x1z0, x1z1, zRatio);

	return myLerp(lerp1, lerp2, xRatio);
}

Mesh::Mesh(std::ifstream& inFile, std::string strMeshName)
{
	std::string strLabel{};
	auto readLabel = [&]() {
		char nStrLength{};
		inFile.read(&nStrLength, sizeof(char));
		strLabel.assign(nStrLength, ' ');
		inFile.read(strLabel.data(), nStrLength);
		};
	// �̸� �ޱ�
	//GetMeshNameFromFile(inFile);
	m_MeshName = strMeshName;

	while (1) {
		readLabel();
		if (strLabel == "<Bounds>:")
			GetBoundInfoFromFile(inFile);
		else if (strLabel == "<Positions>:")
			GetPositionFromFile(inFile);
		else if (strLabel == "<Colors>:")
			GetColorsFromFile(inFile);
		else if (strLabel == "<TextureCoords0>:")
			GetTexCoord0FromFile(inFile);
		else if (strLabel == "<TextureCoords1>:")
			GetTexCoord1FromFile(inFile);
		else if (strLabel == "<Normals>:")
			GetNormalFromFile(inFile);
		else if (strLabel == "<Tangents>:")
			GetTangentFromFile(inFile);
		else if (strLabel == "<BiTangents>:")
			GetBiTangentFromFile(inFile);
		else if (strLabel == "<SubMeshes>:")
			GetSubMeshesFromFile(inFile);
		else if (strLabel == "</Mesh>")
			break;
	}
}

Mesh::Mesh(CHeightMapImage* heightmap, std::string strMeshName)
{
	XMFLOAT3 xmf3Scale = heightmap->m_xmf3Scale;
	int Width = heightmap->m_nWidth; int Length = heightmap->m_nLength;
	std::vector<XMFLOAT3> vertex{};
	std::vector<XMFLOAT2> tex0{};
	std::vector<XMFLOAT2> tex1{};
	vertex.reserve(Length * Width);
	tex0.reserve(Length * Width);
	tex1.reserve(Length * Width);
	std::vector<XMFLOAT3> normals(Length * Width, { 0.0f, 1.0f, 0.0f });

	auto f = [&](XMVECTOR& n, XMFLOAT3& n1, XMFLOAT3& n2, XMFLOAT3& n3) {
		XMVECTOR v1 = XMVector3Normalize(XMLoadFloat3(&n2) - XMLoadFloat3(&n1));
		XMVECTOR v2 = XMVector3Normalize(XMLoadFloat3(&n3) - XMLoadFloat3(&n1));
		n += XMVector3Cross(v1, v2);
		};

	float max_Value = FLT_MIN;

	float fHeight;
	for (int z = 0; z < Length; ++z) {
		for (int x = 0; x < Width; ++x) {
			fHeight = heightmap->GetHeight(x, z);
			if (max_Value < fHeight)
				max_Value = fHeight;
			vertex.emplace_back(x * xmf3Scale.x, fHeight, z * xmf3Scale.z);
			tex0.emplace_back(float(x) / float(Width - 1), float(Length - 1 - z) / float(Length - 1));
			tex1.emplace_back(float(x) / float(xmf3Scale.x * 10.0f), float(z) / float(xmf3Scale.z * 10.0f));

			if (0 == x || Width - 1 == x || 0 == z || Length - 1 == z)
				continue;

			XMVECTOR N{};
			XMFLOAT3 v1{ x * xmf3Scale.x , heightmap->GetHeight(x, z - 1),  (z - 1) * xmf3Scale.z };
			XMFLOAT3 v2{ (x + 1) * xmf3Scale.x , heightmap->GetHeight(x + 1, z - 1),  (z - 1) * xmf3Scale.z };
			XMFLOAT3 v3{ (x - 1) * xmf3Scale.x , heightmap->GetHeight(x - 1, z),  z * xmf3Scale.z };
			XMFLOAT3 v4{ x * xmf3Scale.x , heightmap->GetHeight(x, z),  z * xmf3Scale.z };
			XMFLOAT3 v5{ (x + 1) * xmf3Scale.x , heightmap->GetHeight(x + 1, z),  z * xmf3Scale.z };
			XMFLOAT3 v6{ (x - 1) * xmf3Scale.x , heightmap->GetHeight(x - 1, z + 1),  (z + 1) * xmf3Scale.z };
			XMFLOAT3 v7{ x * xmf3Scale.x , heightmap->GetHeight(x, z + 1),  (z + 1) * xmf3Scale.z };

			f(N, v3, v4, v1); f(N, v1, v4, v2); f(N, v4, v5, v2);
			f(N, v3, v6, v4); f(N, v6, v7, v4); f(N, v4, v7, v5);

			XMStoreFloat3(&normals[x + z * Length], N);
		}
	}

	XMFLOAT3 center{ ((Width - 1) * xmf3Scale.x) / 2 , max_Value / 2, ((Length - 1) * xmf3Scale.z) / 2 };
	XMFLOAT3 extent{ ((Width - 1) * xmf3Scale.x) / 2 , max_Value / 2, ((Length - 1) * xmf3Scale.z) / 2 };
	m_bHasBoundingBox = true;
	m_OBB = BoundingOrientedBox(center, extent, XMFLOAT4(0.0, 0.0, 0.0, 1.0));

	std::vector<UINT> index{};

	for (int z = 0; z < Length - 1; ++z) {
		for (int x = 0; x < Width - 1; ++x) {
			index.emplace_back(x + (z * Width));
			index.emplace_back(x + (z * Width) + Width);
			index.emplace_back(x + (z * Width) + 1);

			index.emplace_back(x + (z * Width) + 1);
			index.emplace_back(x + (z * Width) + Width);
			index.emplace_back(x + (z * Width) + 1 + Width);
		}
	}

	void* tempData{};
	m_bHasVertex = true; m_nVertexCount = vertex.size();
	auto desc = BASIC_BUFFER_DESC;
	desc.Width = sizeof(XMFLOAT3) * m_nVertexCount;
	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(m_pd3dVertexBuffer.GetAddressOf()));
	m_pd3dVertexBuffer->Map(0, nullptr, &tempData);
	memcpy(tempData, vertex.data(), sizeof(XMFLOAT3) * m_nVertexCount);
	m_pd3dVertexBuffer->Unmap(0, nullptr);

	m_bHasTex0 = true; m_nTexCoord0Count = tex0.size();
	desc.Width = sizeof(XMFLOAT2) * m_nTexCoord0Count;
	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(m_pd3dTexCoord0Buffer.GetAddressOf()));
	m_pd3dTexCoord0Buffer->Map(0, nullptr, &tempData);
	memcpy(tempData, tex0.data(), sizeof(XMFLOAT2) * m_nTexCoord0Count);
	m_pd3dTexCoord0Buffer->Unmap(0, nullptr);

	m_bHasTex1 = true; m_nTexCoord1Count = tex1.size();
	desc.Width = sizeof(XMFLOAT2) * m_nTexCoord1Count;
	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(m_pd3dTexCoord1Buffer.GetAddressOf()));
	m_pd3dTexCoord1Buffer->Map(0, nullptr, &tempData);
	memcpy(tempData, tex1.data(), sizeof(XMFLOAT2) * m_nTexCoord1Count);
	m_pd3dTexCoord1Buffer->Unmap(0, nullptr);

	m_bHasNormals = true; m_nNormalsCount = normals.size();
	desc.Width = sizeof(XMFLOAT3) * m_nNormalsCount;
	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(m_pd3dNormalsBuffer.GetAddressOf()));
	m_pd3dNormalsBuffer->Map(0, nullptr, &tempData);
	memcpy(tempData, normals.data(), sizeof(XMFLOAT3) * m_nNormalsCount);
	m_pd3dNormalsBuffer->Unmap(0, nullptr);

	m_bHasSubMeshes = true;
	ComPtr<ID3D12Resource> tempBuffer{};
	desc.Width = sizeof(UINT) * index.size();
	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(tempBuffer.GetAddressOf()));
	tempBuffer->Map(0, nullptr, &tempData);
	memcpy(tempData, index.data(), sizeof(UINT) * index.size());
	tempBuffer->Unmap(0, nullptr);

	++m_nSubMeshesCount;
	m_vIndices.emplace_back(index.size());
	m_vSubMeshes.emplace_back(tempBuffer);
}

Mesh::Mesh(XMFLOAT3& center, XMFLOAT3& extent, std::string meshName)
{
	m_MeshName = meshName;
	m_bHasBoundingBox = true;
	m_OBB = BoundingOrientedBox(center, extent, XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));

	std::vector<XMFLOAT3> pos(8);
	pos[0] = XMFLOAT3(center.x - extent.x, center.y - extent.y, center.z - extent.z);
	pos[1] = XMFLOAT3(center.x + extent.x, center.y - extent.y, center.z - extent.z);
	pos[2] = XMFLOAT3(center.x + extent.x, center.y - extent.y, center.z + extent.z);
	pos[3] = XMFLOAT3(center.x - extent.x, center.y - extent.y, center.z + extent.z);
	pos[4] = XMFLOAT3(center.x - extent.x, center.y + extent.y, center.z - extent.z);
	pos[5] = XMFLOAT3(center.x + extent.x, center.y + extent.y, center.z - extent.z);
	pos[6] = XMFLOAT3(center.x + extent.x, center.y + extent.y, center.z + extent.z);
	pos[7] = XMFLOAT3(center.x - extent.x, center.y + extent.y, center.z + extent.z);

	std::vector<XMFLOAT3> nor(6);
	nor[0] = XMFLOAT3(0.0f, 0.0f, -1.0f); nor[1] = XMFLOAT3(1.0f, 0.0f, 0.0f); nor[2] = XMFLOAT3(0.0f, 0.0f, 1.0f);
	nor[3] = XMFLOAT3(-1.0f, 0.0f, 0.0f); nor[4] = XMFLOAT3(0.0f, -1.0f, 0.0f); nor[5] = XMFLOAT3(0.0f, 1.0f, 0.0f);


	std::vector<XMFLOAT3> vertex(36);
	vertex[0] = pos[0]; vertex[1] = pos[4]; vertex[2] = pos[1];
	vertex[3] = pos[4]; vertex[4] = pos[5]; vertex[5] = pos[1];

	vertex[6] = pos[1]; vertex[7] = pos[5]; vertex[8] = pos[2];
	vertex[9] = pos[5]; vertex[10] = pos[6]; vertex[11] = pos[2];

	vertex[12] = pos[2]; vertex[13] = pos[6]; vertex[14] = pos[3];
	vertex[15] = pos[6]; vertex[16] = pos[7]; vertex[17] = pos[3];

	vertex[18] = pos[3]; vertex[19] = pos[7]; vertex[20] = pos[0];
	vertex[21] = pos[7]; vertex[22] = pos[4]; vertex[23] = pos[0];

	vertex[24] = pos[3]; vertex[25] = pos[0]; vertex[26] = pos[2];
	vertex[27] = pos[0]; vertex[28] = pos[1]; vertex[29] = pos[2];

	vertex[30] = pos[7]; vertex[31] = pos[6]; vertex[32] = pos[4];
	vertex[33] = pos[4]; vertex[34] = pos[6]; vertex[35] = pos[5];

	std::vector<XMFLOAT3> normals(36);
	normals[0] = nor[0]; normals[1] = nor[0]; normals[2] = nor[0];
	normals[3] = nor[0]; normals[4] = nor[0]; normals[5] = nor[0];

	normals[6] = nor[1]; normals[7] = nor[1]; normals[8] = nor[1];
	normals[9] = nor[1]; normals[10] = nor[1]; normals[11] = nor[1];

	normals[12] = nor[2]; normals[13] = nor[2]; normals[14] = nor[2];
	normals[15] = nor[2]; normals[16] = nor[2]; normals[17] = nor[2];

	normals[18] = nor[3]; normals[19] = nor[3]; normals[20] = nor[3];
	normals[21] = nor[3]; normals[22] = nor[3]; normals[23] = nor[3];

	normals[24] = nor[4]; normals[25] = nor[4]; normals[26] = nor[4];
	normals[27] = nor[4]; normals[28] = nor[4]; normals[29] = nor[4];

	normals[30] = nor[5]; normals[31] = nor[5]; normals[32] = nor[5];
	normals[33] = nor[5]; normals[34] = nor[5]; normals[35] = nor[5];


	void* tempData{};
	m_bHasVertex = true;
	m_nVertexCount = 36;
	auto desc = BASIC_BUFFER_DESC;
	desc.Width = sizeof(XMFLOAT3) * 36;
	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(m_pd3dVertexBuffer.GetAddressOf()));
	m_pd3dVertexBuffer->Map(0, nullptr, &tempData);
	memcpy(tempData, vertex.data(), sizeof(XMFLOAT3) * 36);
	m_pd3dVertexBuffer->Unmap(0, nullptr);

	m_bHasNormals = true;
	m_nNormalsCount = 36;
	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(m_pd3dNormalsBuffer.GetAddressOf()));
	m_pd3dNormalsBuffer->Map(0, nullptr, &tempData);
	memcpy(tempData, normals.data(), sizeof(XMFLOAT3) * 36);
	m_pd3dNormalsBuffer->Unmap(0, nullptr);
}

Mesh::Mesh(XMFLOAT3& center, float radius, std::string meshName)
{
	m_MeshName = meshName;

	m_bHasBoundingBox = true;
	m_OBB = BoundingOrientedBox(center, XMFLOAT3(radius, radius, radius), XMFLOAT4(0.0, 0.0, 0.0, 1.0));

	const UINT sliceCount = 20;
	const UINT stackCount = 20;

	std::vector<XMFLOAT3> positions;
	std::vector<XMFLOAT3> normals;
	std::vector<UINT> indices;

	// Top vertex
	positions.emplace_back(center.x, center.y + radius, center.z);
	normals.emplace_back(0.0f, 1.0f, 0.0f);

	// Generate vertices
	for (UINT stack = 1; stack < stackCount; ++stack)
	{
		float phi = XM_PI * stack / stackCount;
		float y = radius * cosf(phi);
		float r = radius * sinf(phi);

		for (UINT slice = 0; slice <= sliceCount; ++slice)
		{
			float theta = 2.0f * XM_PI * slice / sliceCount;
			float x = r * cosf(theta);
			float z = r * sinf(theta);

			positions.emplace_back(center.x + x, center.y + y, center.z + z);
			normals.emplace_back(x / radius, y / radius, z / radius);
		}
	}

	// Bottom vertex
	positions.emplace_back(center.x, center.y - radius, center.z);
	normals.emplace_back(0.0f, -1.0f, 0.0f);

	// Top cap indices
	for (UINT i = 1; i <= sliceCount; ++i)
	{
		indices.push_back(0);
		indices.push_back(i + 1);
		indices.push_back(i);
	}

	// Middle quads (converted to two triangles)
	UINT baseIndex = 1;
	UINT ringVertexCount = sliceCount + 1;
	for (UINT stack = 0; stack < stackCount - 2; ++stack)
	{
		for (UINT slice = 0; slice < sliceCount; ++slice)
		{
			UINT a = baseIndex + stack * ringVertexCount + slice;
			UINT b = baseIndex + (stack + 1) * ringVertexCount + slice;
			UINT c = baseIndex + (stack + 1) * ringVertexCount + slice + 1;
			UINT d = baseIndex + stack * ringVertexCount + slice + 1;

			// Triangle 1
			indices.push_back(a);
			indices.push_back(c);
			indices.push_back(b);

			// Triangle 2
			indices.push_back(a);
			indices.push_back(d);
			indices.push_back(c);
		}
	}

	// Bottom cap indices
	UINT southPoleIndex = (UINT)positions.size() - 1;
	baseIndex = southPoleIndex - ringVertexCount;
	for (UINT i = 0; i < sliceCount; ++i)
	{
		indices.push_back(southPoleIndex);
		indices.push_back(baseIndex + i);
		indices.push_back(baseIndex + i + 1);
	}

	m_bHasVertex = true;
	m_nVertexCount = static_cast<UINT>(positions.size());

	// Create Vertex Buffer
	{
		D3D12_RESOURCE_DESC desc = BASIC_BUFFER_DESC;
		desc.Width = sizeof(XMFLOAT3) * m_nVertexCount;
		g_DxResource.device->CreateCommittedResource(
			&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(m_pd3dVertexBuffer.GetAddressOf()));

		void* tempData{};
		m_pd3dVertexBuffer->Map(0, nullptr, &tempData);
		memcpy(tempData, positions.data(), sizeof(XMFLOAT3) * m_nVertexCount);
		m_pd3dVertexBuffer->Unmap(0, nullptr);
	}

	m_bHasNormals = true;
	m_nNormalsCount = static_cast<UINT>(normals.size());
	// Create Normal Buffer
	{
		D3D12_RESOURCE_DESC desc = BASIC_BUFFER_DESC;
		desc.Width = sizeof(XMFLOAT3) * m_nVertexCount;
		g_DxResource.device->CreateCommittedResource(
			&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(m_pd3dNormalsBuffer.GetAddressOf()));

		void* tempData{};
		m_pd3dNormalsBuffer->Map(0, nullptr, &tempData);
		memcpy(tempData, normals.data(), sizeof(XMFLOAT3) * m_nVertexCount);
		m_pd3dNormalsBuffer->Unmap(0, nullptr);
	}

	// Create Index Buffer
	m_bHasSubMeshes = true;
	ComPtr<ID3D12Resource> tempBuffer{};
	D3D12_RESOURCE_DESC desc = BASIC_BUFFER_DESC;
	desc.Width = sizeof(UINT) * (UINT)indices.size();
	g_DxResource.device->CreateCommittedResource(
		&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(tempBuffer.GetAddressOf()));

	void* tempData{};
	tempBuffer->Map(0, nullptr, &tempData);
	memcpy(tempData, indices.data(), sizeof(UINT) * indices.size());
	tempBuffer->Unmap(0, nullptr);

	++m_nSubMeshesCount;
	m_vIndices.emplace_back((UINT)indices.size());
	m_vSubMeshes.emplace_back(tempBuffer);
}

Mesh::Mesh(XMFLOAT3& center, float width, float height, short arrow)
{
	//float hw = width / 2;
	//float hh = height / 2;
	std::vector<XMFLOAT3> pos(4);
	/*pos[0] = XMFLOAT3(center.x - hw, center.y - hh, 0.0f);
	pos[1] = XMFLOAT3(center.x + hw, center.y - hh, 0.0f);
	pos[2] = XMFLOAT3(center.x + hw, center.y + hh, 0.0f);
	pos[3] = XMFLOAT3(center.x - hw, center.y + hh, 0.0f);*/

	switch (arrow) {
	case MESH_PLANE_1QUADRANT:
		pos[0] = XMFLOAT3(0.0f, 0.0f, 0.0f);
		pos[1] = XMFLOAT3(width, 0.0f, 0.0f);
		pos[2] = XMFLOAT3(width, height, 0.0f);
		pos[3] = XMFLOAT3(0.0f, height, 0.0f);
		break;
	case MESH_PLANE_2QUADRANT:
		pos[0] = XMFLOAT3(-width, 0.0f, 0.0f);
		pos[1] = XMFLOAT3(0.0f, 0.0f, 0.0f);
		pos[2] = XMFLOAT3(0.0f, height, 0.0f);
		pos[3] = XMFLOAT3(-width, height, 0.0f);
		break;
	case MESH_PLANE_3QUADRANT:
		pos[0] = XMFLOAT3(-width, -height, 0.0f);
		pos[1] = XMFLOAT3(0.0f, -height, 0.0f);
		pos[2] = XMFLOAT3(0.0f, 0.0f, 0.0f);
		pos[3] = XMFLOAT3(-width, 0.0f, 0.0f);
		break;
	case MESH_PLANE_4QUADRANT:
	default:
		pos[0] = XMFLOAT3(0.0f, -height, 0.0f);
		pos[1] = XMFLOAT3(width, -height, 0.0f);
		pos[2] = XMFLOAT3(width, 0.0f, 0.0f);
		pos[3] = XMFLOAT3(0.0f, 0.0f, 0.0f);
		break;
	}

	m_bHasVertex = true;
	m_nVertexCount = 4;

	std::vector<XMFLOAT2> uv(4);
	uv[0] = XMFLOAT2(0.0f, 1.0f);
	uv[1] = XMFLOAT2(1.0f, 1.0f);
	uv[2] = XMFLOAT2(1.0f, 0.0f);
	uv[3] = XMFLOAT2(0.0f, 0.0f);

	m_bHasTex0 = true;
	m_nTexCoord0Count = 4;

	std::vector<UINT> index(6);
	index[0] = 0; index[1] = 3; index[2] = 1;
	index[3] = 3; index[4] = 2; index[5] = 1;
	m_nSubMeshesCount = 1;
	m_bHasSubMeshes = true;
	m_vIndices.emplace_back(6);

	{
		D3D12_RESOURCE_DESC desc = BASIC_BUFFER_DESC;
		desc.Width = sizeof(XMFLOAT3) * m_nVertexCount;
		g_DxResource.device->CreateCommittedResource(
			&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(m_pd3dVertexBuffer.GetAddressOf()));

		void* tempData{};
		m_pd3dVertexBuffer->Map(0, nullptr, &tempData);
		memcpy(tempData, pos.data(), sizeof(XMFLOAT3) * m_nVertexCount);
		m_pd3dVertexBuffer->Unmap(0, nullptr);
	}
	{
		D3D12_RESOURCE_DESC desc = BASIC_BUFFER_DESC;
		desc.Width = sizeof(XMFLOAT2) * m_nTexCoord0Count;
		g_DxResource.device->CreateCommittedResource(
			&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(m_pd3dTexCoord0Buffer.GetAddressOf()));

		void* tempData{};
		m_pd3dTexCoord0Buffer->Map(0, nullptr, &tempData);
		memcpy(tempData, uv.data(), sizeof(XMFLOAT2) * m_nTexCoord0Count);
		m_pd3dTexCoord0Buffer->Unmap(0, nullptr);
	}
	{
		ComPtr<ID3D12Resource> tempBuffer{};
		D3D12_RESOURCE_DESC desc = BASIC_BUFFER_DESC;
		desc.Width = sizeof(UINT) * 6;
		g_DxResource.device->CreateCommittedResource(
			&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(tempBuffer.GetAddressOf()));

		void* tempData{};
		tempBuffer->Map(0, nullptr, &tempData);
		memcpy(tempData, index.data(), sizeof(UINT) * 6);
		tempBuffer->Unmap(0, nullptr);

		m_vSubMeshes.emplace_back(tempBuffer);
	}
}
//void Mesh::GetMeshNameFromFile(std::ifstream& inFile)
//{
//	int temp;	// ������ ��
//	inFile.read((char*)&temp, sizeof(int));
//
//	char nStrLength{};
//	inFile.read((char*)&nStrLength, sizeof(char));
//	m_MeshName.assign(nStrLength, ' ');
//	inFile.read(m_MeshName.data(), nStrLength);
//}

void Mesh::GetBoundInfoFromFile(std::ifstream& inFile)
{
	XMFLOAT3 OBBCenter{}; XMFLOAT3 OBBExtent{};
	inFile.read((char*)&OBBCenter, sizeof(XMFLOAT3));
	inFile.read((char*)&OBBExtent, sizeof(XMFLOAT3));
	m_bHasBoundingBox = true;
	m_OBB = BoundingOrientedBox(OBBCenter, OBBExtent, XMFLOAT4(0.0, 0.0, 0.0, 1.0));
}

// ���� ������ ������ ���ÿ� vertexbuffer ����
void Mesh::GetPositionFromFile(std::ifstream& inFile)
{
	// ���Ͽ��� ���� �б�
	inFile.read((char*)&m_nVertexCount, sizeof(int));	// ������ ���� �б�
	if (m_nVertexCount > 0) {
		m_bHasVertex = true;

		std::vector<XMFLOAT3> vPositions;
		vPositions.assign(m_nVertexCount, XMFLOAT3(0.0, 0.0, 0.0));
		inFile.read((char*)vPositions.data(), sizeof(XMFLOAT3) * m_nVertexCount);

		// vertex buffer ����
		auto desc = BASIC_BUFFER_DESC;
		desc.Width = sizeof(XMFLOAT3) * m_nVertexCount;
		// �ϴ��� UPLOAD�� ����, ���� �� DEFAULT�� ���� ����
		g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pd3dVertexBuffer.GetAddressOf()));

		void* ptr;
		m_pd3dVertexBuffer->Map(0, nullptr, &ptr);
		memcpy(ptr, vPositions.data(), sizeof(XMFLOAT3) * m_nVertexCount);
		m_pd3dVertexBuffer->Unmap(0, nullptr);
	}
}

void Mesh::GetColorsFromFile(std::ifstream& inFile)
{
	inFile.read((char*)&m_nColorCount, sizeof(int));
	if (m_nColorCount > 0) {
		m_bHasColor = true;
		std::vector<XMFLOAT4> vColors;

		vColors.assign(m_nColorCount, XMFLOAT4(0.0, 0.0, 0.0, 1.0));
		inFile.read((char*)vColors.data(), sizeof(XMFLOAT4) * m_nColorCount);

		// CreateBuffer
		auto desc = BASIC_BUFFER_DESC;
		desc.Width = sizeof(XMFLOAT4) * m_nColorCount;
		// �ϴ��� UPLOAD�� ����, ���� �� DEFAULT�� ���� ����
		g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pd3dColorsBuffer.GetAddressOf()));

		void* ptr;
		m_pd3dColorsBuffer->Map(0, nullptr, &ptr);
		memcpy(ptr, vColors.data(), sizeof(XMFLOAT4) * m_nColorCount);
		m_pd3dColorsBuffer->Unmap(0, nullptr);
	}
}

void Mesh::GetTexCoord0FromFile(std::ifstream& inFile)
{
	inFile.read((char*)&m_nTexCoord0Count, sizeof(int));
	if (m_nTexCoord0Count > 0) {
		m_bHasTex0 = true;
		/*std::vector<XMFLOAT2> uv;
		uv.assign(m_nTexCoord0Count, XMFLOAT2(0.0, 0.0));
		inFile.read((char*)uv.data(), sizeof(XMFLOAT2) * m_nTexCoord0Count);*/

		m_vTex0.assign(m_nTexCoord0Count, XMFLOAT2(0.0, 0.0));
		inFile.read((char*)m_vTex0.data(), sizeof(XMFLOAT2) * m_nTexCoord0Count);

		// CreateBuffer
		auto desc = BASIC_BUFFER_DESC;
		desc.Width = sizeof(XMFLOAT2) * m_nTexCoord0Count;
		// �ϴ��� UPLOAD�� ����, ���� �� DEFAULT�� ���� ����
		g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pd3dTexCoord0Buffer.GetAddressOf()));

		void* ptr;
		m_pd3dTexCoord0Buffer->Map(0, nullptr, &ptr);
		memcpy(ptr, m_vTex0.data(), sizeof(XMFLOAT2) * m_nTexCoord0Count);
		m_pd3dTexCoord0Buffer->Unmap(0, nullptr);
	}
}

void Mesh::GetTexCoord1FromFile(std::ifstream& inFile)
{
	inFile.read((char*)&m_nTexCoord1Count, sizeof(int));
	if (m_nTexCoord1Count > 0) {
		m_bHasTex1 = true;
		std::vector<XMFLOAT2> uv;
		uv.assign(m_nTexCoord1Count, XMFLOAT2(0.0, 0.0));
		inFile.read((char*)uv.data(), sizeof(XMFLOAT2) * m_nTexCoord1Count);

		// CreateBuffer
		auto desc = BASIC_BUFFER_DESC;
		desc.Width = sizeof(XMFLOAT2) * m_nTexCoord1Count;
		// �ϴ��� UPLOAD�� ����, ���� �� DEFAULT�� ���� ����
		g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pd3dTexCoord1Buffer.GetAddressOf()));

		void* ptr;
		m_pd3dTexCoord1Buffer->Map(0, nullptr, &ptr);
		memcpy(ptr, uv.data(), sizeof(XMFLOAT2) * m_nTexCoord1Count);
		m_pd3dTexCoord1Buffer->Unmap(0, nullptr);
	}
}
void Mesh::GetNormalFromFile(std::ifstream& inFile)
{
	inFile.read((char*)&m_nNormalsCount, sizeof(int));
	if (m_nNormalsCount > 0) {
		m_bHasNormals = true;
		std::vector<XMFLOAT3> uv;
		uv.assign(m_nNormalsCount, XMFLOAT3(0.0, 0.0, 0.0));
		inFile.read((char*)uv.data(), sizeof(XMFLOAT3) * m_nNormalsCount);

		// CreateBuffer
		auto desc = BASIC_BUFFER_DESC;
		desc.Width = sizeof(XMFLOAT3) * m_nNormalsCount;
		// �ϴ��� UPLOAD�� ����, ���� �� DEFAULT�� ���� ����
		g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pd3dNormalsBuffer.GetAddressOf()));

		void* ptr;
		m_pd3dNormalsBuffer->Map(0, nullptr, &ptr);
		memcpy(ptr, uv.data(), sizeof(XMFLOAT3) * m_nNormalsCount);
		m_pd3dNormalsBuffer->Unmap(0, nullptr);
	}
}

void Mesh::GetTangentFromFile(std::ifstream& inFile)
{
	inFile.read((char*)&m_nTangentsCount, sizeof(int));
	if (m_nTangentsCount > 0) {
		m_bHasTangents = true;
		std::vector<XMFLOAT3> uv;
		uv.assign(m_nTangentsCount, XMFLOAT3(0.0, 0.0, 0.0));
		inFile.read((char*)uv.data(), sizeof(XMFLOAT3) * m_nTangentsCount);

		// CreateBuffer
		auto desc = BASIC_BUFFER_DESC;
		desc.Width = sizeof(XMFLOAT3) * m_nTangentsCount;
		// �ϴ��� UPLOAD�� ����, ���� �� DEFAULT�� ���� ����
		g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pd3dTangentsBuffer.GetAddressOf()));

		void* ptr;
		m_pd3dTangentsBuffer->Map(0, nullptr, &ptr);
		memcpy(ptr, uv.data(), sizeof(XMFLOAT3) * m_nTangentsCount);
		m_pd3dTangentsBuffer->Unmap(0, nullptr);
	}
}

void Mesh::GetBiTangentFromFile(std::ifstream& inFile)
{
	inFile.read((char*)&m_nBiTangentsCount, sizeof(int));
	if (m_nBiTangentsCount > 0) {
		m_bHasBiTangents = true;
		std::vector<XMFLOAT3> uv;
		uv.assign(m_nBiTangentsCount, XMFLOAT3(0.0, 0.0, 0.0));
		inFile.read((char*)uv.data(), sizeof(XMFLOAT3) * m_nBiTangentsCount);

		// CreateBuffer
		auto desc = BASIC_BUFFER_DESC;
		desc.Width = sizeof(XMFLOAT3) * m_nBiTangentsCount;
		// �ϴ��� UPLOAD�� ����, ���� �� DEFAULT�� ���� ����
		g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pd3dBiTangentsBuffer.GetAddressOf()));

		void* ptr;
		m_pd3dBiTangentsBuffer->Map(0, nullptr, &ptr);
		memcpy(ptr, uv.data(), sizeof(XMFLOAT3) * m_nBiTangentsCount);
		m_pd3dBiTangentsBuffer->Unmap(0, nullptr);
	}
}

void Mesh::GetSubMeshesFromFile(std::ifstream& inFile)
{
	inFile.read((char*)&m_nSubMeshesCount, sizeof(int));
	if (m_nSubMeshesCount > 0) {
		m_bHasSubMeshes = true;
		for (int i = 0; i < m_nSubMeshesCount; ++i)
			MakeSubMesh(inFile);
	}
}

void Mesh::MakeSubMesh(std::ifstream& inFile)
{
	std::string label{};
	char nStrLength{};

	ComPtr<ID3D12Resource> indexBuffer{};
	int subMeshIndex{};
	UINT indices{};

	// "<SubMesh>:"
	inFile.read(&nStrLength, sizeof(char));
	label.assign(nStrLength, ' ');
	inFile.read((char*)label.data(), nStrLength);

	inFile.read((char*)&subMeshIndex, sizeof(int));

	inFile.read((char*)&indices, sizeof(int));
	m_vIndices.emplace_back(indices);

	if (indices > 0) {
		std::vector<UINT> index{};
		index.assign(indices, 0);
		inFile.read((char*)index.data(), sizeof(UINT) * indices);

		// CreateBuffer
		auto desc = BASIC_BUFFER_DESC;
		desc.Width = sizeof(UINT) * indices;
		// �ϴ��� UPLOAD�� ����, ���� �� DEFAULT�� ���� ����
		g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(indexBuffer.GetAddressOf()));

		void* ptr;
		indexBuffer->Map(0, nullptr, &ptr);
		memcpy(ptr, index.data(), sizeof(UINT) * indices);
		indexBuffer->Unmap(0, nullptr);
	}
	m_vSubMeshes.emplace_back(indexBuffer);
}

// =============================== getter ====================================
std::string Mesh::getName() const
{
	return m_MeshName;
}
ID3D12Resource* Mesh::getVertexBuffer() const
{
	return m_pd3dVertexBuffer.Get();
}

ID3D12Resource* Mesh::getColorsBuffer() const
{
	return m_pd3dColorsBuffer.Get();
}

ID3D12Resource* Mesh::getTexCoord0Buffer() const
{
	return m_pd3dTexCoord0Buffer.Get();
}
ID3D12Resource* Mesh::getTexCoord1Buffer() const
{
	return m_pd3dTexCoord1Buffer.Get();
}
ID3D12Resource* Mesh::getNormalsBuffer() const
{
	return m_pd3dNormalsBuffer.Get();
}
ID3D12Resource* Mesh::getTangentsBuffer() const
{
	return m_pd3dTangentsBuffer.Get();
}
ID3D12Resource* Mesh::getBiTangentsBuffer() const
{
	return m_pd3dBiTangentsBuffer.Get();
}
ID3D12Resource* Mesh::getIndexBuffer(UINT index) const
{
	return m_vSubMeshes[index].Get();
}

UINT Mesh::getVertexCount() const
{
	return m_nVertexCount;
}
UINT Mesh::getIndexCount(int index) const
{
	return m_vIndices[index];
}

bool Mesh::getHasVertex() const
{
	return m_bHasVertex;
}

bool Mesh::getHasColor() const
{
	return m_bHasColor;
}
bool Mesh::getHasTex0() const
{
	return m_bHasTex0;
}
bool Mesh::getHasTex1() const
{
	return m_bHasTex1;
}
bool Mesh::getHasNormal() const
{
	return m_bHasNormals;
}
bool Mesh::getHasTangent() const
{
	return m_bHasTangents;
}
bool Mesh::getHasBiTangent() const
{
	return m_bHasBiTangents;
}
bool Mesh::getHasSubmesh() const
{
	return m_bHasSubMeshes;
}

UINT Mesh::getSubMeshCount() const
{
	return m_nSubMeshesCount;
}

// ==========================================================================