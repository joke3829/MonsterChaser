#include "GameObject.h"


CGameObject::CGameObject(const CGameObject& other)
{
	m_strName = other.m_strName;
	m_xmf3Pos = other.m_xmf3Pos;
	m_xmf3Scale = other.m_xmf3Scale;

	m_xmf3Right = other.m_xmf3Right;
	m_xmf3Up = other.m_xmf3Up;
	m_xmf3Look = other.m_xmf3Look;

	//m_xmf4x4LocalMatrix = other.m_xmf4x4LocalMatrix;
	UpdateLocalMatrix();

	m_bUseBoundingInfo = other.m_bUseBoundingInfo;

	m_OBB = other.m_OBB;
	m_BoundingSphere = other.m_BoundingSphere;
	// SBT may have already registered Resources (in the case of not Skinning)
	// Share material if index is the same

	for (int i = 0; i < other.m_vMaterials.size(); ++i)
		m_vMaterials.emplace_back(other.m_vMaterials[i]);

	m_nMeshIndex = other.m_nMeshIndex;
	m_nParentIndex = other.m_nParentIndex;
	m_nHitGroupIndex = other.m_nHitGroupIndex;
}

CGameObject& CGameObject::operator=(const CGameObject& other)
{
	if (this != &other) {
		m_strName = other.m_strName;
		m_xmf3Pos = other.m_xmf3Pos;
		m_xmf3Scale = other.m_xmf3Scale;

		m_xmf3Right = other.m_xmf3Right;
		m_xmf3Up = other.m_xmf3Up;
		m_xmf3Look = other.m_xmf3Look;

		UpdateLocalMatrix();

		m_bUseBoundingInfo = other.m_bUseBoundingInfo;

		m_OBB = other.m_OBB;
		m_BoundingSphere = other.m_BoundingSphere;
		// m_xmf4x4LocalMatrix = other.m_xmf4x4LocalMatrix;
		// SBT may have already registered Resources (in the case of not Skinning)
		// Share material if index is the same

		for (int i = 0; i < other.m_vMaterials.size(); ++i)
			m_vMaterials.emplace_back(other.m_vMaterials[i]);

		m_nMeshIndex = other.m_nMeshIndex;
		m_nParentIndex = other.m_nParentIndex;
		m_nHitGroupIndex = other.m_nHitGroupIndex;
	}
	return *this;
}

bool CGameObject::InitializeObjectFromFile(std::ifstream& inFile)
{
	int temp;	// temp Data
	inFile.read((char*)&temp, sizeof(int));	// Frame Number
	inFile.read((char*)&temp, sizeof(int));	// Child Count

	// Name
	char strLength{};
	inFile.read(&strLength, sizeof(char));
	m_strName.assign(strLength, ' ');
	inFile.read(m_strName.data(), strLength);

	std::string strLabel{};
	auto readLabel = [&]() {
		char nStrLength{};
		inFile.read(&nStrLength, sizeof(char));
		strLabel.assign(nStrLength, ' ');
		inFile.read(strLabel.data(), nStrLength);
		};

	// <Transform>:
	readLabel();
	float dummyFloat[4];
	inFile.read((char*)&m_xmf3Pos, sizeof(XMFLOAT3));
	inFile.read((char*)dummyFloat, sizeof(float) * 3);
	inFile.read((char*)&m_xmf3Scale, sizeof(XMFLOAT3));
	inFile.read((char*)dummyFloat, sizeof(float) * 4);

	// <TransformMatrix>:
	readLabel();
	inFile.read((char*)&m_xmf4x4LocalMatrix, sizeof(XMFLOAT4X4));

	InitializeAxis();

	return true;
}

void CGameObject::UpdateLocalMatrix()
{
	XMFLOAT4X4 tempMatrix{};
	tempMatrix._11 = m_xmf3Right.x; tempMatrix._12 = m_xmf3Right.y; tempMatrix._13 = m_xmf3Right.z; tempMatrix._14 = 0;
	tempMatrix._21 = m_xmf3Up.x; tempMatrix._22 = m_xmf3Up.y; tempMatrix._23 = m_xmf3Up.z; tempMatrix._24 = 0;
	tempMatrix._31 = m_xmf3Look.x; tempMatrix._32 = m_xmf3Look.y; tempMatrix._33 = m_xmf3Look.z; tempMatrix._34 = 0;
	tempMatrix._41 = 0; tempMatrix._42 = 0; tempMatrix._43 = 0; tempMatrix._44 = 1;

	XMStoreFloat4x4(&tempMatrix, XMMatrixScaling(m_xmf3Scale.x, m_xmf3Scale.y, m_xmf3Scale.z) * XMLoadFloat4x4(&tempMatrix));
	tempMatrix._41 = m_xmf3Pos.x; tempMatrix._42 = m_xmf3Pos.y; tempMatrix._43 = m_xmf3Pos.z;

	m_xmf4x4LocalMatrix = tempMatrix;
}
void CGameObject::SetPosition(XMFLOAT3 pos)
{
	m_xmf3Pos = pos;
	UpdateLocalMatrix();
}

void CGameObject::Rotate(XMFLOAT3 rot)
{
	XMMATRIX mtx;
	if (rot.x != 0.0f) {
		mtx = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Right), XMConvertToRadians(rot.x));
		XMStoreFloat3(&m_xmf3Look, XMVector3TransformNormal(XMLoadFloat3(&m_xmf3Look), mtx));
		XMStoreFloat3(&m_xmf3Up, XMVector3TransformNormal(XMLoadFloat3(&m_xmf3Up), mtx));
	}
	if (rot.y != 0.0f) {
		mtx = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Up), XMConvertToRadians(rot.y));
		XMStoreFloat3(&m_xmf3Look, XMVector3TransformNormal(XMLoadFloat3(&m_xmf3Look), mtx));
		XMStoreFloat3(&m_xmf3Right, XMVector3TransformNormal(XMLoadFloat3(&m_xmf3Right), mtx));
	}
	if (rot.z != 0.0f) {
		mtx = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Look), XMConvertToRadians(rot.z));
		XMStoreFloat3(&m_xmf3Right, XMVector3TransformNormal(XMLoadFloat3(&m_xmf3Right), mtx));
		XMStoreFloat3(&m_xmf3Up, XMVector3TransformNormal(XMLoadFloat3(&m_xmf3Up), mtx));
	}
	UpdateLocalMatrix();
}
void CGameObject::SetScale(XMFLOAT3 scale)
{
	m_xmf3Scale = scale;
	UpdateLocalMatrix();
}

void CGameObject::move(float fElapsedTime) {
	XMStoreFloat3(&m_xmf3Pos, XMLoadFloat3(&m_xmf3Pos) + (XMLoadFloat3(&m_xmf3Look) * 30.0f * fElapsedTime));
	UpdateLocalMatrix();
}

std::string CGameObject::getFrameName() const
{
	return m_strName;
}

std::vector<Material>& CGameObject::getMaterials()
{
	return m_vMaterials;
}

int CGameObject::getMeshIndex() const
{
	return m_nMeshIndex;
}

int CGameObject::getHitGroupIndex() const
{
	return m_nHitGroupIndex;
}

int CGameObject::getParentIndex() const
{
	return m_nParentIndex;
}

ID3D12Resource* CGameObject::getCbuffer(int index) const
{
	return m_vCBuffers[index].Get();
}

ID3D12Resource* CGameObject::getMeshCBuffer() const
{
	return m_pd3dMeshCBuffer.Get();
}

XMFLOAT4X4& CGameObject::getWorldMatrix()
{
	return m_xmf4x4WorldMatrix;
}

XMFLOAT4X4& CGameObject::getLocalMatrix()
{
	return m_xmf4x4LocalMatrix;
}

void CGameObject::SetMeshIndex(int index)
{
	m_nMeshIndex = index;
}

void CGameObject::SetParentIndex(int index)
{
	m_nParentIndex = index;
}

void CGameObject::SetHitGroupIndex(int index)
{
	m_nHitGroupIndex = index;
}

void CGameObject::SetWorldMatrix(XMFLOAT4X4& mtx)
{
	m_xmf4x4WorldMatrix = mtx;
}

void CGameObject::InitializeAxis()
{
	auto normalizeFloat3 = [](XMFLOAT3& xmf) {
		XMStoreFloat3(&xmf, XMVector3Normalize(XMLoadFloat3(&xmf)));
		};
	XMVECTOR scale, rotation, position;
	XMMatrixDecompose(&scale, &rotation, &position, XMLoadFloat4x4(&m_xmf4x4LocalMatrix));

	XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotation);

	XMStoreFloat3(&m_xmf3Right, rotationMatrix.r[0]);
	XMStoreFloat3(&m_xmf3Up, rotationMatrix.r[1]);
	XMStoreFloat3(&m_xmf3Look, rotationMatrix.r[2]);

	normalizeFloat3(m_xmf3Right);
	normalizeFloat3(m_xmf3Up);
	normalizeFloat3(m_xmf3Look);
}


// =============================================================

CSkinningInfo::CSkinningInfo(std::ifstream& inFile, UINT nRefMesh)
{
	std::string strLabel{};
	UINT tempInt{};
	auto readLabel = [&]() {
		char nStrLength{};
		inFile.read(&nStrLength, sizeof(char));
		strLabel.assign(nStrLength, ' ');
		inFile.read(strLabel.data(), nStrLength);
		};
	m_nRefMesh = nRefMesh;

	readLabel();
	while (1) {
		readLabel();
		if (strLabel == "</SkinningInfo>")
			break;
		else if ("<BonesPerVertex>:" == strLabel)
			inFile.read((char*)&m_nBonesPerVertex, sizeof(int));
		else if ("<Bounds>:" == strLabel) { // unused bound
			XMFLOAT3 tempData{};
			inFile.read((char*)&tempData, sizeof(XMFLOAT3));
			inFile.read((char*)&tempData, sizeof(XMFLOAT3));
		}
		else if ("<BoneNames>:" == strLabel) {
			inFile.read((char*)&m_nBones, sizeof(int));
			for (int i = 0; i < m_nBones; ++i) {
				readLabel();
				m_vBoneNames.emplace_back(strLabel);
			}
		}
		else if ("<BoneOffsets>:" == strLabel) {
			inFile.read((char*)&m_nBones, sizeof(int));
			m_vOffsetMatrix.assign(m_nBones, XMFLOAT4X4());
			inFile.read((char*)m_vOffsetMatrix.data(), sizeof(XMFLOAT4X4) * m_nBones);
		}
		else if ("<BoneIndices>:" == strLabel) {
			inFile.read((char*)&tempInt, sizeof(UINT));
			m_vBoneIndices.assign(tempInt * m_nBonesPerVertex, 0);
			inFile.read((char*)m_vBoneIndices.data(), sizeof(int) * tempInt * m_nBonesPerVertex);
		}
		else if ("<BoneWeights>:" == strLabel) {
			inFile.read((char*)&tempInt, sizeof(UINT));
			m_nVertexCount = tempInt;
			m_vBoneWeight.assign(tempInt * m_nBonesPerVertex, 0);
			inFile.read((char*)m_vBoneWeight.data(), sizeof(float) * tempInt * m_nBonesPerVertex);
		}
	}
}

CSkinningInfo::CSkinningInfo(const CSkinningInfo& other)
{
	m_nBonesPerVertex = other.m_nBonesPerVertex;
	m_nBones = other.m_nBones;
	m_nVertexCount = other.m_nVertexCount;

	m_nRefMesh = other.m_nRefMesh;
	for (int i = 0; i < other.m_vBoneNames.size(); ++i)
		m_vBoneNames.emplace_back(other.m_vBoneNames[i]);
	for (int i = 0; i < other.m_vOffsetMatrix.size(); ++i) {
		m_vOffsetMatrix.emplace_back(other.m_vOffsetMatrix[i]);
		XMStoreFloat4x4(&m_vOffsetMatrix[i], XMMatrixTranspose(XMLoadFloat4x4(&m_vOffsetMatrix[i])));
	}
	for (int i = 0; i < other.m_vBoneIndices.size(); ++i)
		m_vBoneIndices.emplace_back(other.m_vBoneIndices[i]);
	for (int i = 0; i < other.m_vBoneWeight.size(); ++i)
		m_vBoneWeight.emplace_back(other.m_vBoneWeight[i]);
}

void CSkinningInfo::MakeAnimationMatrixIndex(std::vector<std::string>& vFrameNames)
{
	for (std::string& name : m_vBoneNames) {
		auto p = std::find(vFrameNames.begin(), vFrameNames.end(), name);
		m_vAnimationMatrixIndex.emplace_back(distance(vFrameNames.begin(), p));
	}
}

void CSkinningInfo::MakeBufferAndDescriptorHeap(ComPtr<ID3D12Resource>& pMatrixBuffer, UINT nElements)
{
	D3D12_DESCRIPTOR_HEAP_DESC ddesc{};
	ddesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ddesc.NumDescriptors = 6;
	ddesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	g_DxResource.device->CreateDescriptorHeap(&ddesc, IID_PPV_ARGS(m_pd3dDesciptorHeap.GetAddressOf()));

	void* tempData{};

	// CB
	auto desc = BASIC_BUFFER_DESC;

	desc.Width = Align(sizeof(UINT) * 2, 256);
	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pConstantBuffer.GetAddressOf()));
	m_pConstantBuffer->Map(0, nullptr, &tempData);
	memcpy(tempData, &m_nVertexCount, sizeof(UINT));
	tempData = static_cast<char*>(tempData) + sizeof(UINT);
	memcpy(tempData, &m_nBonesPerVertex, sizeof(UINT));
	m_pConstantBuffer->Unmap(0, nullptr);

	// Offset Matrix
	desc.Width = sizeof(XMFLOAT4X4) * m_vOffsetMatrix.size();
	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pOffsetMatrixBuffer.GetAddressOf()));
	m_pOffsetMatrixBuffer->Map(0, nullptr, &tempData);
	for (int i = 0; i < m_vOffsetMatrix.size(); ++i)
		XMStoreFloat4x4(&m_vOffsetMatrix[i], XMMatrixTranspose(XMLoadFloat4x4(&m_vOffsetMatrix[i])));
	memcpy(tempData, m_vOffsetMatrix.data(), sizeof(XMFLOAT4X4) * m_vOffsetMatrix.size());
	m_pOffsetMatrixBuffer->Unmap(0, nullptr);

	// BoneIndex
	desc.Width = sizeof(UINT) * m_vBoneIndices.size();
	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pBoneIndicesBuffer.GetAddressOf()));
	m_pBoneIndicesBuffer->Map(0, nullptr, &tempData);
	memcpy(tempData, m_vBoneIndices.data(), sizeof(UINT) * m_vBoneIndices.size());
	m_pBoneIndicesBuffer->Unmap(0, nullptr);

	// BoneWeight
	desc.Width = sizeof(float) * m_vBoneWeight.size();
	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pBoneWeightBuffer.GetAddressOf()));
	m_pBoneWeightBuffer->Map(0, nullptr, &tempData);
	memcpy(tempData, m_vBoneWeight.data(), sizeof(float) * m_vBoneWeight.size());
	m_pBoneWeightBuffer->Unmap(0, nullptr);

	// Index of BoneIndex
	desc.Width = sizeof(UINT) * m_vAnimationMatrixIndex.size();
	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pAnimationMatrixIndexBuffer.GetAddressOf()));
	m_pAnimationMatrixIndexBuffer->Map(0, nullptr, &tempData);
	memcpy(tempData, m_vAnimationMatrixIndex.data(), sizeof(UINT) * m_vAnimationMatrixIndex.size());
	m_pAnimationMatrixIndexBuffer->Unmap(0, nullptr);


	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_pd3dDesciptorHeap->GetCPUDescriptorHandleForHeapStart();

	UINT incrementSize = g_DxResource.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CONSTANT_BUFFER_VIEW_DESC cDesc{};
	// CB
	cDesc.BufferLocation = m_pConstantBuffer->GetGPUVirtualAddress();
	cDesc.SizeInBytes = Align(sizeof(UINT) * 2, 256);
	g_DxResource.device->CreateConstantBufferView(&cDesc, handle);
	handle.ptr += incrementSize;

	D3D12_SHADER_RESOURCE_VIEW_DESC vDesc{};
	vDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	vDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	vDesc.Buffer.FirstElement = 0;
	vDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	// OffsetMatrix
	vDesc.Buffer.NumElements = m_vOffsetMatrix.size();
	vDesc.Buffer.StructureByteStride = sizeof(XMFLOAT4X4);
	g_DxResource.device->CreateShaderResourceView(m_pOffsetMatrixBuffer.Get(), &vDesc, handle);
	handle.ptr += incrementSize;

	// BoneIndex
	vDesc.Buffer.NumElements = m_vBoneIndices.size();
	vDesc.Buffer.StructureByteStride = sizeof(UINT);
	g_DxResource.device->CreateShaderResourceView(m_pBoneIndicesBuffer.Get(), &vDesc, handle);
	handle.ptr += incrementSize;

	// BoneWeight
	vDesc.Buffer.NumElements = m_vBoneWeight.size();
	vDesc.Buffer.StructureByteStride = sizeof(float);
	g_DxResource.device->CreateShaderResourceView(m_pBoneWeightBuffer.Get(), &vDesc, handle);
	handle.ptr += incrementSize;

	// Index of BoneIndex
	vDesc.Buffer.NumElements = m_vAnimationMatrixIndex.size();
	vDesc.Buffer.StructureByteStride = sizeof(UINT);
	g_DxResource.device->CreateShaderResourceView(m_pAnimationMatrixIndexBuffer.Get(), &vDesc, handle);
	handle.ptr += incrementSize;

	// Animation Matrix
	vDesc.Buffer.NumElements = nElements;
	vDesc.Buffer.StructureByteStride = sizeof(XMFLOAT4X4);
	g_DxResource.device->CreateShaderResourceView(pMatrixBuffer.Get(), &vDesc, handle);
}

void CSkinningInfo::SetShaderVariables()
{
	g_DxResource.cmdList->SetDescriptorHeaps(1, m_pd3dDesciptorHeap.GetAddressOf());
	g_DxResource.cmdList->SetComputeRootDescriptorTable(0, m_pd3dDesciptorHeap->GetGPUDescriptorHandleForHeapStart());
}

// ============================================================================

CSkinningObject::CSkinningObject()
{
	XMStoreFloat4x4(&m_xmf4x4WorldMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_xmf4x4PreTransformMatrix, XMMatrixIdentity());
	m_xmf3Right = XMFLOAT3(1.0f, 0.0f, 0.0f);
	m_xmf3Up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	m_xmf3Look = XMFLOAT3(0.0f, 0.0f, 1.0f);
}

void CSkinningObject::CopyFromOtherObject(CSkinningObject* other)
{
	m_strObjectName = other->getName();

	for (std::unique_ptr<CGameObject>& Frame : other->getObjects())
		m_vObjects.emplace_back(std::make_unique<CGameObject>(*Frame.get()));
	for (std::shared_ptr<Mesh>& mesh : other->getMeshes())
		m_vMeshes.emplace_back(mesh);
	for (std::shared_ptr<CTexture>& texture : other->getTextures())
		m_vTextures.emplace_back(texture);
	for (std::unique_ptr<CSkinningInfo>& info : other->getSkinningInfo())
		m_vSkinningInfo.emplace_back(std::make_unique<CSkinningInfo>(*info.get()));
}

void CSkinningObject::AddResourceFromFile(std::ifstream& inFile, std::string strFront)
{
	FilePathFront = strFront;
	std::string strLabel{};
	auto readLabel = [&]() {
		char nStrLength{};
		inFile.read(&nStrLength, sizeof(char));
		strLabel.assign(nStrLength, ' ');
		inFile.read(strLabel.data(), nStrLength);
		};
	while (1) {
		readLabel();
		if (strLabel == "</Hierarchy>")
			break;
		else if (strLabel == "<Frame>:")
			AddObjectFromFile(inFile);
	}

	if (m_vObjects.size() != 0)
		m_strObjectName = m_vObjects[0]->getFrameName();
}

void CSkinningObject::AddObjectFromFile(std::ifstream& inFile, int nParentIndex)
{
	UINT nCurrentObjectIndex = m_vObjects.size();
	bool bBoneBound = false;
	bool bOBB = false;
	m_vObjects.emplace_back(std::make_unique<CGameObject>());
	m_vObjects[nCurrentObjectIndex]->InitializeObjectFromFile(inFile);

	if (nParentIndex != -1) {		// Not Has Parent
		m_vObjects[nCurrentObjectIndex]->SetParentIndex(nParentIndex);
	}

	std::string strLabel{};
	auto readLabel = [&]() {
		char nStrLength{};
		inFile.read(&nStrLength, sizeof(char));
		strLabel.assign(nStrLength, ' ');
		inFile.read(strLabel.data(), nStrLength);
		};
	int tempData{};
	while (1) {
		readLabel();
		if (strLabel == "<Mesh>:") {
			if (!g_ShowBoundingBox) {
				inFile.read((char*)&tempData, sizeof(int));
				readLabel();	// Read Mesh Name
				auto p = std::find_if(m_vMeshes.begin(), m_vMeshes.end(), [&](std::shared_ptr<Mesh>& tempMesh) {
					return tempMesh->getName() == strLabel;
					});
				if (p != m_vMeshes.end()) {	// mesh in List
					// Mesh Index Set
					m_vObjects[nCurrentObjectIndex]->SetMeshIndex(std::distance(m_vMeshes.begin(), p));
					// The file has a mesh, so it needs to be removed
					Mesh* tempMesh = new Mesh(inFile, strLabel);	// not used
					delete tempMesh;
				}
				else {	// No mesh in List
					m_vObjects[nCurrentObjectIndex]->SetMeshIndex(m_vMeshes.size());
					m_vMeshes.emplace_back(std::make_shared<Mesh>(inFile, strLabel));
				}
			}
			else {
				if (bBoneBound) {
					inFile.read((char*)&tempData, sizeof(int));
					readLabel();
					Mesh* tempMesh = new Mesh(inFile, strLabel);
					delete tempMesh;
				}
				else {
					inFile.read((char*)&tempData, sizeof(int));
					readLabel();	// Read Mesh Name
					auto p = std::find_if(m_vMeshes.begin(), m_vMeshes.end(), [&](std::shared_ptr<Mesh>& tempMesh) {
						return tempMesh->getName() == strLabel;
						});
					if (p != m_vMeshes.end()) {	// mesh in List
						// Mesh Index Set
						m_vObjects[nCurrentObjectIndex]->SetMeshIndex(std::distance(m_vMeshes.begin(), p));
						// The file has a mesh, so it needs to be removed
						Mesh* tempMesh = new Mesh(inFile, strLabel);	// not used
						delete tempMesh;
					}
					else {	// No mesh in List
						m_vObjects[nCurrentObjectIndex]->SetMeshIndex(m_vMeshes.size());
						m_vMeshes.emplace_back(std::make_shared<Mesh>(inFile, strLabel));
					}
				}
			}
		}
		else if (strLabel == "<SkinningInfo>:") {
			if (g_ShowBoundingBox)	// Fail -> Frame not has Bound
				m_vSkinningInfo.emplace_back(std::make_unique<CSkinningInfo>(inFile, m_vMeshes.size() - 1));
			else
				m_vSkinningInfo.emplace_back(std::make_unique<CSkinningInfo>(inFile, m_vMeshes.size()));
		}
		else if (strLabel == "<Materials>:") {
			inFile.read((char*)&tempData, sizeof(int));
			AddMaterialFromFile(inFile, nCurrentObjectIndex);
			if (g_ShowBoundingBox && bBoneBound) {
				std::vector<Material>& tempV = m_vObjects[nCurrentObjectIndex]->getMaterials();
				tempV.clear();
				Material tempM;
				tempM.m_bHasAlbedoColor = true; tempM.m_xmf4AlbedoColor = XMFLOAT4(g_unorm(g_dre), g_unorm(g_dre), g_unorm(g_dre), 0.5);	// ���� �÷�
				tempV.emplace_back(tempM);
			}
		}
		else if (strLabel == "<Children>:") {
			inFile.read((char*)&tempData, sizeof(int));
			if (tempData > 0) {
				for (int i = 0; i < tempData; ++i) {
					readLabel();	// <Frame>:
					AddObjectFromFile(inFile, nCurrentObjectIndex);
				}
			}
		}
		else if (strLabel == "<Bound>:") {
			XMFLOAT3 center, extent;
			inFile.read((char*)&center, sizeof(XMFLOAT3));
			inFile.read((char*)&extent, sizeof(XMFLOAT3));
			m_vObjects[nCurrentObjectIndex]->SetBoundingOBB(center, extent);
			if (g_ShowBoundingBox && false == bBoneBound) {
				bBoneBound = true;
				bOBB = true;
				m_vObjects[nCurrentObjectIndex]->SetMeshIndex(m_vMeshes.size());
				m_vMeshes.emplace_back(std::make_shared<Mesh>(center, extent));

				std::vector<Material>& tempV = m_vObjects[nCurrentObjectIndex]->getMaterials();
				tempV.clear();
				Material tempM;
				tempM.m_bHasAlbedoColor = true; tempM.m_xmf4AlbedoColor = XMFLOAT4(g_unorm(g_dre), g_unorm(g_dre), g_unorm(g_dre), 0.5);	// ���� �÷�
				tempV.emplace_back(tempM);
			}
		}
		else if (strLabel == "<BoundingSphere>:") {
			XMFLOAT3 center{}; float rad{};
			inFile.read((char*)&center, sizeof(XMFLOAT3));
			inFile.read((char*)&rad, sizeof(float));
			m_vObjects[nCurrentObjectIndex]->SetBoundingSphere(center, rad);
			if (g_ShowBoundingBox) {
				bBoneBound = true;
				if (bOBB) {
					m_vObjects[nCurrentObjectIndex]->SetMeshIndex(m_vMeshes.size());
					m_vMeshes.emplace_back(std::make_shared<Mesh>(center, rad));
				}
				else {
					m_vObjects[nCurrentObjectIndex]->SetMeshIndex(m_vMeshes.size());
					m_vMeshes.emplace_back(std::make_shared<Mesh>(center, rad));

					std::vector<Material>& tempV = m_vObjects[nCurrentObjectIndex]->getMaterials();
					tempV.clear();
					Material tempM;
					tempM.m_bHasAlbedoColor = true; tempM.m_xmf4AlbedoColor = XMFLOAT4(g_unorm(g_dre), g_unorm(g_dre), g_unorm(g_dre), 0.5);	// ���� �÷�
					tempV.emplace_back(tempM);
				}
			}
		}
		else if (strLabel == "</Frame>")
			break;
	}
}

void CSkinningObject::AddMaterialFromFile(std::ifstream& inFile, int nCurrentIndex)
{
	std::string strLabel{};
	auto readLabel = [&]() {
		char nStrLength{};
		inFile.read(&nStrLength, sizeof(char));
		strLabel.assign(nStrLength, ' ');
		inFile.read(strLabel.data(), nStrLength);
		};
	int tempData{};
	std::vector<Material> vMaterials;

	std::string FilePathBack{ ".dds" };				// Use .dss

	int nCurrentMaterial{};
	while (1) {
		readLabel();
		if (strLabel == "<Material>:") {
			nCurrentMaterial = vMaterials.size();
			vMaterials.emplace_back(Material());
			inFile.read((char*)&tempData, sizeof(int));
		}
		else if (strLabel == "<AlbedoColor>:") {
			vMaterials[nCurrentMaterial].m_bHasAlbedoColor = true;
			inFile.read((char*)&vMaterials[nCurrentMaterial].m_xmf4AlbedoColor, sizeof(XMFLOAT4));
		}
		else if (strLabel == "<EmissiveColor>:") {
			vMaterials[nCurrentMaterial].m_bHasEmissiveColor = true;
			inFile.read((char*)&vMaterials[nCurrentMaterial].m_xmf4EmissiveColor, sizeof(XMFLOAT4));
		}
		else if (strLabel == "<SpecularColor>:") {
			vMaterials[nCurrentMaterial].m_bHasSpecularColor = true;
			inFile.read((char*)&vMaterials[nCurrentMaterial].m_xmf4SpecularColor, sizeof(XMFLOAT4));
		}
		else if (strLabel == "<Glossiness>:") {
			vMaterials[nCurrentMaterial].m_bHasGlossiness = true;
			inFile.read((char*)&vMaterials[nCurrentMaterial].m_fGlossiness, sizeof(float));
		}
		else if (strLabel == "<Smoothness>:") {
			vMaterials[nCurrentMaterial].m_bHasSmoothness = true;
			inFile.read((char*)&vMaterials[nCurrentMaterial].m_fSmoothness, sizeof(float));
		}
		else if (strLabel == "<Metallic>:") {
			vMaterials[nCurrentMaterial].m_bHasMetallic = true;
			inFile.read((char*)&vMaterials[nCurrentMaterial].m_fMetallic, sizeof(float));
		}
		else if (strLabel == "<SpecularHighlight>:") {
			vMaterials[nCurrentMaterial].m_bHasSpecularHighlight = true;
			inFile.read((char*)&vMaterials[nCurrentMaterial].m_fSpecularHighlight, sizeof(float));
		}
		else if (strLabel == "<GlossyReflection>:") {
			vMaterials[nCurrentMaterial].m_bHasGlossyReflection = true;
			inFile.read((char*)&vMaterials[nCurrentMaterial].m_fGlossyReflection, sizeof(float));
		}
		else if (strLabel == "<AlbedoMap>:") {
			readLabel();
			if (strLabel[0] == '@') {	// Defined
				strLabel.erase(strLabel.begin());
				auto p = std::find_if(m_vTextures.begin(), m_vTextures.end(), [&](std::shared_ptr<CTexture>& txt) {
					return txt->getName() == strLabel;
					});
				vMaterials[nCurrentMaterial].m_bHasAlbedoMap = true;
				vMaterials[nCurrentMaterial].m_nAlbedoMapIndex = std::distance(m_vTextures.begin(), p);
			}
			else if (strLabel == "null") {	// null is not used Texture
				continue;
			}
			else {	// Create New Texture
				vMaterials[nCurrentMaterial].m_bHasAlbedoMap = true;
				vMaterials[nCurrentMaterial].m_nAlbedoMapIndex = m_vTextures.size();
				std::string FilePath = FilePathFront + strLabel + FilePathBack;
				std::wstring wstr;
				wstr.assign(FilePath.begin(), FilePath.end());
				m_vTextures.emplace_back(std::make_shared<CTexture>(wstr.c_str()));
				m_vTextures[m_vTextures.size() - 1]->SetTextureName(strLabel);
			}
		}
		else if (strLabel == "<SpecularMap>:") {
			readLabel();
			if (strLabel[0] == '@') {	// Defined
				strLabel.erase(strLabel.begin());
				auto p = std::find_if(m_vTextures.begin(), m_vTextures.end(), [&](std::shared_ptr<CTexture>& txt) {
					return txt->getName() == strLabel;
					});
				vMaterials[nCurrentMaterial].m_bHasSpecularMap = true;
				vMaterials[nCurrentMaterial].m_nSpecularMapIndex = std::distance(m_vTextures.begin(), p);	// ����
			}
			else if (strLabel == "null") {	// null is not used Texture
				continue;
			}
			else {	// Create New Texture
				vMaterials[nCurrentMaterial].m_bHasSpecularMap = true;
				vMaterials[nCurrentMaterial].m_nSpecularMapIndex = m_vTextures.size();
				std::string FilePath = FilePathFront + strLabel + FilePathBack;
				std::wstring wstr;
				wstr.assign(FilePath.begin(), FilePath.end());
				m_vTextures.emplace_back(std::make_shared<CTexture>(wstr.c_str()));
				m_vTextures[m_vTextures.size() - 1]->SetTextureName(strLabel);
			}
		}
		else if (strLabel == "<MetallicMap>:") {
			readLabel();
			if (strLabel[0] == '@') {	// Defined
				strLabel.erase(strLabel.begin());
				auto p = std::find_if(m_vTextures.begin(), m_vTextures.end(), [&](std::shared_ptr<CTexture>& txt) {
					return txt->getName() == strLabel;
					});
				vMaterials[nCurrentMaterial].m_bHasMetallicMap = true;
				vMaterials[nCurrentMaterial].m_nMetallicMapIndex = std::distance(m_vTextures.begin(), p);
			}
			else if (strLabel == "null") {	// null is not used Texture
				continue;
			}
			else {	// Create New Texture
				vMaterials[nCurrentMaterial].m_bHasMetallicMap = true;
				vMaterials[nCurrentMaterial].m_nMetallicMapIndex = m_vTextures.size();
				std::string FilePath = FilePathFront + strLabel + FilePathBack;
				std::wstring wstr;
				wstr.assign(FilePath.begin(), FilePath.end());
				m_vTextures.emplace_back(std::make_shared<CTexture>(wstr.c_str()));
				m_vTextures[m_vTextures.size() - 1]->SetTextureName(strLabel);
			}
		}
		else if (strLabel == "<NormalMap>:") {
			readLabel();
			if (strLabel[0] == '@') {	// Defined
				strLabel.erase(strLabel.begin());
				auto p = std::find_if(m_vTextures.begin(), m_vTextures.end(), [&](std::shared_ptr<CTexture>& txt) {
					return txt->getName() == strLabel;
					});
				vMaterials[nCurrentMaterial].m_bHasNormalMap = true;
				vMaterials[nCurrentMaterial].m_nNormalMapIndex = std::distance(m_vTextures.begin(), p);
			}
			else if (strLabel == "null") {	// null is not used Texture
				continue;
			}
			else {	// Create New Texture
				vMaterials[nCurrentMaterial].m_bHasNormalMap = true;
				vMaterials[nCurrentMaterial].m_nNormalMapIndex = m_vTextures.size();
				std::string FilePath = FilePathFront + strLabel + FilePathBack;
				std::wstring wstr;
				wstr.assign(FilePath.begin(), FilePath.end());
				m_vTextures.emplace_back(std::make_shared<CTexture>(wstr.c_str()));
				m_vTextures[m_vTextures.size() - 1]->SetTextureName(strLabel);
			}
		}
		else if (strLabel == "<EmissionMap>:") {
			readLabel();
			if (strLabel[0] == '@') {	// Defined
				strLabel.erase(strLabel.begin());
				auto p = std::find_if(m_vTextures.begin(), m_vTextures.end(), [&](std::shared_ptr<CTexture>& txt) {
					return txt->getName() == strLabel;
					});
				vMaterials[nCurrentMaterial].m_bHasEmissionMap = true;
				vMaterials[nCurrentMaterial].m_nEmissionMapIndex = std::distance(m_vTextures.begin(), p);
			}
			else if (strLabel == "null") {	// null is not used Texture
				continue;
			}
			else {	// Create New Texture
				vMaterials[nCurrentMaterial].m_bHasEmissionMap = true;
				vMaterials[nCurrentMaterial].m_nEmissionMapIndex = m_vTextures.size();
				std::string FilePath = FilePathFront + strLabel + FilePathBack;
				std::wstring wstr;
				wstr.assign(FilePath.begin(), FilePath.end());
				m_vTextures.emplace_back(std::make_shared<CTexture>(wstr.c_str()));
				m_vTextures[m_vTextures.size() - 1]->SetTextureName(strLabel);
			}
		}
		else if (strLabel == "<DetailAlbedoMap>:") {
			readLabel();
			if (strLabel[0] == '@') {	// Defined
				strLabel.erase(strLabel.begin());
				auto p = std::find_if(m_vTextures.begin(), m_vTextures.end(), [&](std::shared_ptr<CTexture>& txt) {
					return txt->getName() == strLabel;
					});
				vMaterials[nCurrentMaterial].m_bHasDetailAlbedoMap = true;
				vMaterials[nCurrentMaterial].m_nDetailAlbedoMapIndex = std::distance(m_vTextures.begin(), p);
			}
			else if (strLabel == "null") {	// null is not used Texture
				continue;
			}
			else {	// Create New Texture
				vMaterials[nCurrentMaterial].m_bHasDetailAlbedoMap = true;
				vMaterials[nCurrentMaterial].m_nDetailAlbedoMapIndex = m_vTextures.size();
				std::string FilePath = FilePathFront + strLabel + FilePathBack;
				std::wstring wstr;
				wstr.assign(FilePath.begin(), FilePath.end());
				m_vTextures.emplace_back(std::make_shared<CTexture>(wstr.c_str()));
				m_vTextures[m_vTextures.size() - 1]->SetTextureName(strLabel);
			}
		}
		else if (strLabel == "<DetailNormalMap>:") {
			readLabel();
			if (strLabel[0] == '@') {	// Defined
				strLabel.erase(strLabel.begin());
				auto p = std::find_if(m_vTextures.begin(), m_vTextures.end(), [&](std::shared_ptr<CTexture>& txt) {
					return txt->getName() == strLabel;
					});
				vMaterials[nCurrentMaterial].m_bHasDetailNormalMap = true;
				vMaterials[nCurrentMaterial].m_nDetailNormalMapIndex = std::distance(m_vTextures.begin(), p);
			}
			else if (strLabel == "null") {	// null is not used Texture
				continue;
			}
			else {	// Create New Texture
				vMaterials[nCurrentMaterial].m_bHasDetailNormalMap = true;
				vMaterials[nCurrentMaterial].m_nDetailNormalMapIndex = m_vTextures.size();
				std::string FilePath = FilePathFront + strLabel + FilePathBack;
				std::wstring wstr;
				wstr.assign(FilePath.begin(), FilePath.end());
				m_vTextures.emplace_back(std::make_shared<CTexture>(wstr.c_str()));
				m_vTextures[m_vTextures.size() - 1]->SetTextureName(strLabel);
			}
		}
		else if (strLabel == "</Materials>")
			break;
	}
	// push Material
	for (int i = 0; i < vMaterials.size(); ++i) {
		m_vObjects[nCurrentIndex]->getMaterials().emplace_back(vMaterials[i]);
	}
}

void CSkinningObject::InitializeGameObjectCBuffer()
{
	for (std::unique_ptr<CGameObject>& object : m_vObjects)
		object->InitializeConstanctBuffer(m_vMeshes);
}

void CSkinningObject::setPreTransform(float scale, XMFLOAT3 rotate, XMFLOAT3 position)
{
	m_bUsePreTransform = true;
	XMMATRIX xmfScale = XMMatrixScaling(scale, scale, scale);
	XMMATRIX xmfRotateX = XMMatrixRotationX(XMConvertToRadians(rotate.x));
	XMMATRIX xmfRotateY = XMMatrixRotationX(XMConvertToRadians(rotate.y));
	XMMATRIX xmfRotateZ = XMMatrixRotationX(XMConvertToRadians(rotate.z));

	XMMATRIX xmfRotation = xmfRotateX * xmfRotateY * xmfRotateZ;

	XMMATRIX xmfTranslation = XMMatrixTranslation(position.x, position.y, position.z);

	XMStoreFloat4x4(&m_xmf4x4PreTransformMatrix, xmfScale * xmfRotation * xmfTranslation);
}

void CSkinningObject::UpdateFrameWorldMatrix()
{
	UpdatePreWorldMatrix();

	for (std::unique_ptr<CGameObject>& object : m_vObjects) {
		if (object->getParentIndex() != -1) {
			XMFLOAT4X4 wmtx = m_vObjects[object->getParentIndex()]->getWorldMatrix();
			XMFLOAT4X4 lmtx = object->getLocalMatrix();
			XMStoreFloat4x4(&lmtx, XMLoadFloat4x4(&lmtx) * XMLoadFloat4x4(&wmtx));
			object->SetWorldMatrix(lmtx);
		}
		else {
			XMFLOAT4X4 lmtx = object->getLocalMatrix();
			//if (m_bUsePreTransform) {
			//	//XMMATRIX mtx = XMLoadFloat4x4(&m_xmf4x4PreTransformMatrix) * XMLoadFloat4x4(&m_xmf4x4WorldMatrix);
			//	XMStoreFloat4x4(&lmtx, XMLoadFloat4x4(&lmtx) * XMLoadFloat4x4(&m_xmf4x4PreWorldMatrix));
			//	object->SetWorldMatrix(lmtx);
			//}
			//else {
			//	XMStoreFloat4x4(&lmtx, XMLoadFloat4x4(&lmtx) * XMLoadFloat4x4(&m_xmf4x4WorldMatrix));
			//	object->SetWorldMatrix(lmtx);
			//}
			XMStoreFloat4x4(&lmtx, XMLoadFloat4x4(&lmtx) * XMLoadFloat4x4(&m_xmf4x4PreWorldMatrix));
			object->SetWorldMatrix(lmtx);
		}
	}
}

void CSkinningObject::UpdateAnimationMatrixes()
{
	for (std::unique_ptr<CGameObject>& object : m_vObjects) {
		int parent = object->getParentIndex();
		if (parent != -1) {
			XMFLOAT4X4 patx = m_vObjects[parent]->getAnimationMatrix();
			XMFLOAT4X4 ltx = object->getLocalMatrix();
			XMStoreFloat4x4(&ltx, XMLoadFloat4x4(&ltx) * XMLoadFloat4x4(&patx));
			object->SetAnimationMatrix(ltx);
		}
		else
			object->SetAnimationMatrix(object->getLocalMatrix());
	}
}

void CSkinningObject::UpdatePreWorldMatrix()
{
	if (m_bUsePreTransform)
		XMStoreFloat4x4(&m_xmf4x4PreWorldMatrix, XMLoadFloat4x4(&m_xmf4x4PreTransformMatrix) * XMLoadFloat4x4(&m_xmf4x4WorldMatrix));
	else
		m_xmf4x4PreWorldMatrix = m_xmf4x4WorldMatrix;
}

void CSkinningObject::UpdateWorldMatrix()
{
	m_xmf4x4WorldMatrix._11 = m_xmf3Right.x; m_xmf4x4WorldMatrix._12 = m_xmf3Right.y; m_xmf4x4WorldMatrix._13 = m_xmf3Right.z; m_xmf4x4WorldMatrix._14 = 0;
	m_xmf4x4WorldMatrix._21 = m_xmf3Up.x; m_xmf4x4WorldMatrix._22 = m_xmf3Up.y; m_xmf4x4WorldMatrix._23 = m_xmf3Up.z; m_xmf4x4WorldMatrix._24 = 0;
	m_xmf4x4WorldMatrix._31 = m_xmf3Look.x; m_xmf4x4WorldMatrix._32 = m_xmf3Look.y; m_xmf4x4WorldMatrix._33 = m_xmf3Look.z; m_xmf4x4WorldMatrix._34 = 0;
	m_xmf4x4WorldMatrix._41 = m_xmf3Position.x; m_xmf4x4WorldMatrix._42 = m_xmf3Position.y; m_xmf4x4WorldMatrix._43 = m_xmf3Position.z; m_xmf4x4WorldMatrix._44 = 1;
}
void CSkinningObject::SetPosition(XMFLOAT3 pos)
{
	m_xmf3Position = pos;
	UpdateWorldMatrix();
}
void CSkinningObject::SetLookDirection(const XMFLOAT3& look, const XMFLOAT3& up)
{
	m_xmf3Look = look; // Set the character look with the camera's direction vector
	m_xmf3Up = up;     // Set to Up vector for camera or frame

	// Normalize Look
	XMVECTOR lookVec = XMLoadFloat3(&m_xmf3Look);
	XMVECTOR upVec = XMLoadFloat3(&m_xmf3Up);
	lookVec = XMVector3Normalize(lookVec);
	XMStoreFloat3(&m_xmf3Look, lookVec);

	// Calculate Right
	XMVECTOR right = XMVector3Cross(upVec, lookVec);
	right = XMVector3Normalize(right);
	XMStoreFloat3(&m_xmf3Right, right);

	// Calculate Up
	upVec = XMVector3Cross(lookVec, right);
	upVec = XMVector3Normalize(upVec);
	XMStoreFloat3(&m_xmf3Up, upVec);

	UpdateWorldMatrix();
}

void CSkinningObject::Rotate(XMFLOAT3 rot)
{
	XMMATRIX mtx;
	if (rot.x != 0.0f) {
		mtx = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Right), XMConvertToRadians(rot.x));
		XMStoreFloat3(&m_xmf3Look, XMVector3TransformNormal(XMLoadFloat3(&m_xmf3Look), mtx));
		XMStoreFloat3(&m_xmf3Up, XMVector3TransformNormal(XMLoadFloat3(&m_xmf3Up), mtx));
	}
	if (rot.y != 0.0f) {
		mtx = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Up), XMConvertToRadians(rot.y));
		XMStoreFloat3(&m_xmf3Look, XMVector3TransformNormal(XMLoadFloat3(&m_xmf3Look), mtx));
		XMStoreFloat3(&m_xmf3Right, XMVector3TransformNormal(XMLoadFloat3(&m_xmf3Right), mtx));
	}
	if (rot.z != 0.0f) {
		mtx = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Look), XMConvertToRadians(rot.z));
		XMStoreFloat3(&m_xmf3Right, XMVector3TransformNormal(XMLoadFloat3(&m_xmf3Right), mtx));
		XMStoreFloat3(&m_xmf3Up, XMVector3TransformNormal(XMLoadFloat3(&m_xmf3Up), mtx));
	}
	UpdateWorldMatrix();
}

void CSkinningObject::Rotation(XMFLOAT3 rot, CGameObject& frame)
{
	XMFLOAT4X4 centerWorldMatrix = frame.getWorldMatrix();
	XMMATRIX centerMatrix = XMLoadFloat4x4(&centerWorldMatrix);

	XMMATRIX centerInverse = XMMatrixInverse(nullptr, centerMatrix);

	XMVECTOR currentPos = XMLoadFloat3(&m_xmf3Position);
	XMVECTOR localPos = XMVector3Transform(currentPos, centerInverse);

	XMFLOAT3 centerUp = frame.getUp();
	XMVECTOR rotationAxis = XMLoadFloat3(&centerUp);
	XMFLOAT3 tempUp;
	XMStoreFloat3(&tempUp, rotationAxis);
	XMMATRIX orbitMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(rot.y));

	XMVECTOR newLocalPos = XMVector3Transform(localPos, orbitMatrix);

	XMVECTOR newPos = XMVector3Transform(newLocalPos, centerMatrix);
	XMStoreFloat3(&m_xmf3Position, newPos);

	XMVECTOR oldLook = XMLoadFloat3(&m_xmf3Look);
	XMVECTOR oldRight = XMLoadFloat3(&m_xmf3Right);
	XMVECTOR newLook = XMVector3TransformNormal(oldLook, orbitMatrix);
	XMVECTOR newRight = XMVector3TransformNormal(oldRight, orbitMatrix);
	XMStoreFloat3(&m_xmf3Look, XMVector3Normalize(newLook));
	XMStoreFloat3(&m_xmf3Right, XMVector3Normalize(newRight));
	m_xmf3Up = tempUp;

	UpdateWorldMatrix();
}

void CSkinningObject::move(float fElapsedTime)
{
	float speed = 5.0f;
	XMStoreFloat3(&m_xmf3Position, XMLoadFloat3(&m_xmf3Position) + (XMLoadFloat3(&m_xmf3Look) * speed * fElapsedTime));

	UpdateWorldMatrix();
}

void CSkinningObject::run(float fElapsedTime)
{
	float speed = 10.0f;
	XMStoreFloat3(&m_xmf3Position, XMLoadFloat3(&m_xmf3Position) + (XMLoadFloat3(&m_xmf3Look) * speed * fElapsedTime));

	UpdateWorldMatrix();
}

void CSkinningObject::sliding(float depth, const XMFLOAT3& normal, float meshHeight)
{
	XMVECTOR normalVec = XMLoadFloat3(&normal);

	// 법선 정규화
	normalVec = XMVector3Normalize(normalVec);

	// y 성분 제거 (x, z 방향으로만 밀어내기)
	XMFLOAT3 normalNoY;
	XMStoreFloat3(&normalNoY, normalVec);
	normalNoY.y = 0.0f;
	normalVec = XMLoadFloat3(&normalNoY);

	// y 성분 제거 후 방향이 0 벡터가 아닌 경우 정규화
	if (XMVectorGetX(XMVector3Length(normalVec)) > 0.01f) {
		normalVec = XMVector3Normalize(normalVec);
	}
	else {
		// 모서리에서 법선이 부정확할 경우, 이전 슬라이딩 방향 유지 또는 이동 방향 사용
		if (XMVectorGetX(XMVector3Length(XMLoadFloat3(&m_xmf3Sliding))) > 0.01f) {
			normalVec = XMVector3Normalize(XMLoadFloat3(&m_xmf3Sliding));
		}
		else {
			normalVec = XMLoadFloat3(&m_xmf3Look); // 이동 방향으로 대체
			XMFLOAT3 lookNoY = m_xmf3Look;
			lookNoY.y = 0.0f;
			normalVec = XMVector3Normalize(XMLoadFloat3(&lookNoY));
		}
	}

	// 침투 깊이로 밀어내기
	XMVECTOR pushOut = normalVec * depth;
	XMMATRIX worldMatrix = XMLoadFloat4x4(&m_xmf4x4WorldMatrix);
	XMVECTOR currentPos = worldMatrix.r[3];
	currentPos += pushOut;

	// 슬라이딩 벡터 업데이트
	XMStoreFloat3(&m_xmf3Sliding, normalVec);

	// 위치 업데이트
	XMFLOAT3 lastPos;
	XMStoreFloat3(&lastPos, currentPos);
	SetPosition(lastPos);

	UpdateWorldMatrix();
}

void CSkinningObject::SetMoveDirection(XMFLOAT3& pos)
{
	m_xmf3MoveDirection = pos;
}

void CSkinningObject::SetDirectionMove(const XMFLOAT3& targetDir, const XMFLOAT3& up, float fElapsedTime)
{
	XMVECTOR currentLook = XMVector3Normalize(XMLoadFloat3(&m_xmf3Look));
	XMFLOAT3 targetDirXZ(targetDir.x, 0.0f, targetDir.z);
	XMVECTOR targetLook = XMVector3Normalize(XMLoadFloat3(&targetDirXZ));

	float rotationSpeed = XMConvertToRadians(1620.0f);
	float t = min(1.0f, rotationSpeed * fElapsedTime);

	float dot = XMVectorGetX(XMVector3Dot(currentLook, targetLook));
	if (dot < -0.8f)
	{
		t = min(1.0f, rotationSpeed * fElapsedTime * 1.5f);
	}

	XMVECTOR newLook = XMVector3Normalize(XMVectorLerp(currentLook, targetLook, t));
	XMStoreFloat3(&m_xmf3Look, newLook);
	m_xmf3Look.y = 0.0f;

	XMVECTOR upVector = XMLoadFloat3(&up);
	XMVECTOR rightVector = XMVector3Normalize(XMVector3Cross(upVector, newLook));
	XMVECTOR recalculatedUp = XMVector3Normalize(XMVector3Cross(newLook, rightVector));

	XMStoreFloat3(&m_xmf3Right, rightVector);
	XMStoreFloat3(&m_xmf3Up, recalculatedUp);

	XMMATRIX worldMatrix = XMMatrixIdentity();
	worldMatrix.r[0] = XMLoadFloat3(&m_xmf3Right);
	worldMatrix.r[1] = XMLoadFloat3(&m_xmf3Up);
	worldMatrix.r[2] = XMLoadFloat3(&m_xmf3Look);
	worldMatrix.r[3] = XMVectorSet(m_xmf3Position.x, m_xmf3Position.y, m_xmf3Position.z, 1.0f);

	XMStoreFloat4x4(&m_xmf4x4WorldMatrix, worldMatrix);
}

void CSkinningObject::SetWorldMatrix(XMFLOAT4X4& mat)
{
	m_xmf4x4WorldMatrix = mat;
	XMFLOAT3 right(mat._11, mat._12, mat._13);
	XMFLOAT3 up(mat._21, mat._22, mat._23);
	XMFLOAT3 look(mat._31, mat._32, mat._33);

	XMStoreFloat3(&m_xmf3Right, XMLoadFloat3(&right));
	XMStoreFloat3(&m_xmf3Up, XMLoadFloat3(&up));
	XMStoreFloat3(&m_xmf3Look, XMLoadFloat3(&look));
}

std::vector<std::unique_ptr<CSkinningInfo>>& CSkinningObject::getSkinningInfo()
{
	return m_vSkinningInfo;
}


// ==================================================================================

void CRayTracingSkinningObject::PrepareObject()
{
	MakeBLAS();
	ReadyOutputVertexBuffer();
}

void CRayTracingSkinningObject::UpdateObject(float fElapsedTime)
{
	if (!g_ShowBoundingBox) {
		for (int i = 0; i < m_vSkinningInfo.size(); ++i) {
			m_vSkinningInfo[i]->SetShaderVariables();
			UINT ref = m_vSkinningInfo[i]->getRefMeshIndex();
			//g_DxResource.cmdList->SetComputeRootShaderResourceView(1, m_vMeshes[ref]->getVertexBuffer()->GetGPUVirtualAddress());
			g_DxResource.cmdList->SetDescriptorHeaps(1, m_vInsertDescriptorHeap[ref].GetAddressOf());
			g_DxResource.cmdList->SetComputeRootDescriptorTable(1, m_vInsertDescriptorHeap[ref]->GetGPUDescriptorHandleForHeapStart());
			g_DxResource.cmdList->SetDescriptorHeaps(1, m_vUAV[ref].GetAddressOf());
			g_DxResource.cmdList->SetComputeRootDescriptorTable(2, m_vUAV[ref]->GetGPUDescriptorHandleForHeapStart());

			UINT dispatch = m_vMeshes[ref]->getVertexCount() / 32 + 1;
			g_DxResource.cmdList->Dispatch(dispatch, 1, 1);
		}
	}
}

void CRayTracingSkinningObject::ReBuildBLAS()
{
	for (std::unique_ptr<CSkinningInfo>& info : m_vSkinningInfo) {
		UINT ref = info->getRefMeshIndex();
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> GeometryDesc{};
		if (m_vMeshes[ref]->getHasSubmesh()) {
			int nSubMesh = m_vMeshes[ref]->getSubMeshCount();
			for (int i = 0; i < nSubMesh; ++i) {
				D3D12_RAYTRACING_GEOMETRY_DESC desc{};
				desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
				desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
				desc.Triangles.Transform3x4 = 0;
				desc.Triangles.VertexBuffer = {
					.StartAddress = (g_ShowBoundingBox) ? m_vMeshes[ref]->getVertexBuffer()->GetGPUVirtualAddress() : m_vOutputVertexBuffer[ref]->GetGPUVirtualAddress(),
					.StrideInBytes = sizeof(float) * 3
				};
				desc.Triangles.VertexCount = m_vMeshes[ref]->getVertexCount();
				desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
				desc.Triangles.IndexBuffer = m_vMeshes[ref]->getIndexBuffer(i)->GetGPUVirtualAddress();
				desc.Triangles.IndexCount = m_vMeshes[ref]->getIndexCount(i);
				desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

				GeometryDesc.emplace_back(desc);
			}
		}
		else {
			D3D12_RAYTRACING_GEOMETRY_DESC desc{};
			desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
			desc.Triangles.Transform3x4 = 0;
			desc.Triangles.VertexBuffer = {
				.StartAddress = (g_ShowBoundingBox) ? m_vMeshes[ref]->getVertexBuffer()->GetGPUVirtualAddress() : m_vOutputVertexBuffer[ref]->GetGPUVirtualAddress(),
				.StrideInBytes = sizeof(float) * 3
			};
			desc.Triangles.VertexCount = m_vMeshes[ref]->getVertexCount();
			desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			desc.Triangles.IndexBuffer = 0;
			desc.Triangles.IndexCount = 0;
			desc.Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN;

			GeometryDesc.emplace_back(desc);
		}

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
		inputs.NumDescs = GeometryDesc.size();
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.pGeometryDescs = GeometryDesc.data();

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC ASDesc{};
		ASDesc.Inputs = inputs;
		ASDesc.DestAccelerationStructureData = m_vBLAS[ref]->GetGPUVirtualAddress();
		ASDesc.ScratchAccelerationStructureData = m_pScratchBuffer->GetGPUVirtualAddress();

		g_DxResource.cmdList->BuildRaytracingAccelerationStructure(&ASDesc, 0, nullptr);
		D3D12_RESOURCE_BARRIER d3dbr{};
		d3dbr.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dbr.UAV.pResource = m_vBLAS[ref].Get();
		g_DxResource.cmdList->ResourceBarrier(1, &d3dbr);
	}
}

void CRayTracingSkinningObject::MakeBLAS()
{
	for (std::shared_ptr<Mesh>& mesh : m_vMeshes) {
		ComPtr<ID3D12Resource> blas{};
		if (mesh->getHasVertex()) {
			InitBLAS(blas, mesh);
		}
		m_vBLAS.emplace_back(blas);
	}
	if (m_nScratchSize != 0) {
		auto desc = BASIC_BUFFER_DESC;
		desc.Width = m_nScratchSize;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		g_DxResource.device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(m_pScratchBuffer.GetAddressOf()));
	}
}

void CRayTracingSkinningObject::InitBLAS(ComPtr<ID3D12Resource>& resource, std::shared_ptr<Mesh>& mesh)
{
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> GeometryDesc{};
	if (mesh->getHasSubmesh()) {
		int nSubMesh = mesh->getSubMeshCount();
		for (int i = 0; i < nSubMesh; ++i) {
			D3D12_RAYTRACING_GEOMETRY_DESC desc{};
			desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
			desc.Triangles.Transform3x4 = 0;
			desc.Triangles.VertexBuffer = {
				.StartAddress = mesh->getVertexBuffer()->GetGPUVirtualAddress(),
				.StrideInBytes = sizeof(float) * 3
			};
			desc.Triangles.VertexCount = mesh->getVertexCount();
			desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			desc.Triangles.IndexBuffer = mesh->getIndexBuffer(i)->GetGPUVirtualAddress();
			desc.Triangles.IndexCount = mesh->getIndexCount(i);
			desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

			GeometryDesc.emplace_back(desc);
		}
	}
	else {
		D3D12_RAYTRACING_GEOMETRY_DESC desc{};
		desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		desc.Triangles.Transform3x4 = 0;
		desc.Triangles.VertexBuffer = {
			.StartAddress = mesh->getVertexBuffer()->GetGPUVirtualAddress(),
			.StrideInBytes = sizeof(float) * 3
		};
		desc.Triangles.VertexCount = mesh->getVertexCount();
		desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		desc.Triangles.IndexBuffer = 0;
		desc.Triangles.IndexCount = 0;
		desc.Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN;

		GeometryDesc.emplace_back(desc);
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
	inputs.NumDescs = GeometryDesc.size();
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.pGeometryDescs = GeometryDesc.data();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO preBuildInfo{};
	g_DxResource.device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &preBuildInfo);
	m_nScratchSize = (preBuildInfo.ScratchDataSizeInBytes > m_nScratchSize) ? preBuildInfo.ScratchDataSizeInBytes : m_nScratchSize;

	// blas
	auto bDesc = BASIC_BUFFER_DESC;
	bDesc.Width = preBuildInfo.ResultDataMaxSizeInBytes;
	bDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	g_DxResource.device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &bDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(resource.GetAddressOf()));

	ComPtr<ID3D12Resource> ScratchBuffer{};
	bDesc.Width = preBuildInfo.ScratchDataSizeInBytes;
	g_DxResource.device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &bDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(ScratchBuffer.GetAddressOf()));

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC ASDesc{};
	ASDesc.Inputs = inputs;
	ASDesc.DestAccelerationStructureData = resource->GetGPUVirtualAddress();
	ASDesc.ScratchAccelerationStructureData = ScratchBuffer->GetGPUVirtualAddress();

	g_DxResource.cmdAlloc->Reset();
	g_DxResource.cmdList->Reset(g_DxResource.cmdAlloc, nullptr);
	g_DxResource.cmdList->BuildRaytracingAccelerationStructure(&ASDesc, 0, nullptr);
	D3D12_RESOURCE_BARRIER d3dbr{};
	d3dbr.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	d3dbr.UAV.pResource = resource.Get();
	g_DxResource.cmdList->ResourceBarrier(1, &d3dbr);
	g_DxResource.cmdList->Close();
	g_DxResource.cmdQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(&g_DxResource.cmdList));
	Flush();
}

void CRayTracingSkinningObject::ReadyOutputVertexBuffer()
{
	auto desc = BASIC_BUFFER_DESC;
	desc.Width = sizeof(XMFLOAT3);
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	g_DxResource.device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(m_pNullResource.GetAddressOf()));


	for (std::unique_ptr<CSkinningInfo>& info : m_vSkinningInfo) {
		int n = info->getRefMeshIndex();
		if (!g_ShowBoundingBox)
			m_vMeshes[n]->setSkinning(true);
	}
	for (std::shared_ptr<Mesh>& mesh : m_vMeshes) {
		ComPtr<ID3D12Resource> constResource{};
		ComPtr<ID3D12Resource> vResource{};
		ComPtr<ID3D12Resource> nResource{};
		ComPtr<ID3D12Resource> tResource{};
		ComPtr<ID3D12Resource> biResource{};

		ComPtr<ID3D12DescriptorHeap> uav{};
		ComPtr<ID3D12DescriptorHeap> insert{};
		if (mesh->getbSkinning()) {
			// ��� ����
			desc.Width = Align(sizeof(XMINT3), 256);
			desc.Flags = D3D12_RESOURCE_FLAG_NONE;
			g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(constResource.GetAddressOf()));
			XMINT3* pMap{};
			constResource->Map(0, nullptr, (void**)&pMap);

			D3D12_DESCRIPTOR_HEAP_DESC hDesc{};
			hDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			hDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			hDesc.NumDescriptors = 4;

			g_DxResource.device->CreateDescriptorHeap(&hDesc, IID_PPV_ARGS(uav.GetAddressOf()));

			hDesc.NumDescriptors = 5;
			g_DxResource.device->CreateDescriptorHeap(&hDesc, IID_PPV_ARGS(insert.GetAddressOf()));

			D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = uav->GetCPUDescriptorHandleForHeapStart();
			D3D12_CPU_DESCRIPTOR_HANDLE insertHandle = insert->GetCPUDescriptorHandleForHeapStart();
			UINT incrementSize = g_DxResource.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cDesc{};
			cDesc.BufferLocation = constResource->GetGPUVirtualAddress(); cDesc.SizeInBytes = Align(sizeof(XMINT3), 256);
			g_DxResource.device->CreateConstantBufferView(&cDesc, insertHandle);
			insertHandle.ptr += incrementSize;

			D3D12_UNORDERED_ACCESS_VIEW_DESC vDesc{};
			vDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

			D3D12_SHADER_RESOURCE_VIEW_DESC sDesc{};
			sDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			sDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			sDesc.Buffer.FirstElement = 0;
			sDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

			{	// vertex
				desc.Width = mesh->getVertexCount() * sizeof(XMFLOAT3);
				desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				g_DxResource.device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(vResource.GetAddressOf()));

				vDesc.Buffer.NumElements = mesh->getVertexCount();
				vDesc.Buffer.StructureByteStride = sizeof(XMFLOAT3);
				g_DxResource.device->CreateUnorderedAccessView(vResource.Get(), nullptr, &vDesc, uavHandle);

				sDesc.Buffer.NumElements = mesh->getVertexCount();
				sDesc.Buffer.StructureByteStride = sizeof(XMFLOAT3);
				g_DxResource.device->CreateShaderResourceView(mesh->getVertexBuffer(), &sDesc, insertHandle);
			}
			uavHandle.ptr += incrementSize;
			insertHandle.ptr += incrementSize;

			if (mesh->getHasNormal()) {	// exist Normal buffer
				pMap->x = 1;

				desc.Width = mesh->getVertexCount() * sizeof(XMFLOAT3);
				desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				g_DxResource.device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(nResource.GetAddressOf()));

				vDesc.Buffer.NumElements = mesh->getVertexCount();
				vDesc.Buffer.StructureByteStride = sizeof(XMFLOAT3);
				g_DxResource.device->CreateUnorderedAccessView(nResource.Get(), nullptr, &vDesc, uavHandle);

				sDesc.Buffer.NumElements = mesh->getVertexCount();
				sDesc.Buffer.StructureByteStride = sizeof(XMFLOAT3);
				g_DxResource.device->CreateShaderResourceView(mesh->getNormalsBuffer(), &sDesc, insertHandle);
			}
			else {
				pMap->x = 0;

				vDesc.Buffer.NumElements = 1;
				vDesc.Buffer.StructureByteStride = sizeof(XMFLOAT3);
				g_DxResource.device->CreateUnorderedAccessView(m_pNullResource.Get(), nullptr, &vDesc, uavHandle);

				sDesc.Buffer.NumElements = 1;
				sDesc.Buffer.StructureByteStride = sizeof(XMFLOAT3);
				g_DxResource.device->CreateShaderResourceView(m_pNullResource.Get(), &sDesc, insertHandle);
			}
			uavHandle.ptr += incrementSize;
			insertHandle.ptr += incrementSize;

			if (mesh->getHasTangent()) {	// exist Tangent buffer
				pMap->y = 1;

				desc.Width = mesh->getVertexCount() * sizeof(XMFLOAT3);
				desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				g_DxResource.device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(tResource.GetAddressOf()));

				vDesc.Buffer.NumElements = mesh->getVertexCount();
				vDesc.Buffer.StructureByteStride = sizeof(XMFLOAT3);
				g_DxResource.device->CreateUnorderedAccessView(tResource.Get(), nullptr, &vDesc, uavHandle);

				sDesc.Buffer.NumElements = mesh->getVertexCount();
				sDesc.Buffer.StructureByteStride = sizeof(XMFLOAT3);
				g_DxResource.device->CreateShaderResourceView(mesh->getTangentsBuffer(), &sDesc, insertHandle);
			}
			else {
				pMap->y = 0;

				vDesc.Buffer.NumElements = 1;
				vDesc.Buffer.StructureByteStride = sizeof(XMFLOAT3);
				g_DxResource.device->CreateUnorderedAccessView(m_pNullResource.Get(), nullptr, &vDesc, uavHandle);

				sDesc.Buffer.NumElements = 1;
				sDesc.Buffer.StructureByteStride = sizeof(XMFLOAT3);
				g_DxResource.device->CreateShaderResourceView(m_pNullResource.Get(), &sDesc, insertHandle);
			}
			uavHandle.ptr += incrementSize;
			insertHandle.ptr += incrementSize;

			if (mesh->getHasBiTangent()) {	// exist BiTangent buffer
				pMap->z = 1;

				desc.Width = mesh->getVertexCount() * sizeof(XMFLOAT3);
				desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				g_DxResource.device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(biResource.GetAddressOf()));

				vDesc.Buffer.NumElements = mesh->getVertexCount();
				vDesc.Buffer.StructureByteStride = sizeof(XMFLOAT3);
				g_DxResource.device->CreateUnorderedAccessView(biResource.Get(), nullptr, &vDesc, uavHandle);

				sDesc.Buffer.NumElements = mesh->getVertexCount();
				sDesc.Buffer.StructureByteStride = sizeof(XMFLOAT3);
				g_DxResource.device->CreateShaderResourceView(mesh->getBiTangentsBuffer(), &sDesc, insertHandle);
			}
			else {
				pMap->z = 0;

				vDesc.Buffer.NumElements = 1;
				vDesc.Buffer.StructureByteStride = sizeof(XMFLOAT3);
				g_DxResource.device->CreateUnorderedAccessView(m_pNullResource.Get(), nullptr, &vDesc, uavHandle);

				sDesc.Buffer.NumElements = 1;
				sDesc.Buffer.StructureByteStride = sizeof(XMFLOAT3);
				g_DxResource.device->CreateShaderResourceView(m_pNullResource.Get(), &sDesc, insertHandle);
			}
			uavHandle.ptr += incrementSize;
			insertHandle.ptr += incrementSize;

			constResource->Unmap(0, nullptr);


		}
		m_vInputConstantBuffer.emplace_back(constResource);
		m_vOutputVertexBuffer.emplace_back(vResource);
		m_vOutputNormalBuffer.emplace_back(nResource);
		m_vOutputTangentsBuffer.emplace_back(tResource);
		m_vOutputBiTangentsBuffer.emplace_back(biResource);

		m_vUAV.emplace_back(uav);
		m_vInsertDescriptorHeap.emplace_back(insert);
	}
}

// ========================================================================================

CProjectile::CProjectile()
{
	XMStoreFloat4x4(&m_xmf4x4WorldMatrix, XMMatrixIdentity());
}

void CProjectile::IsMoving(float fElapsedTime)
{
	if (!m_bActive) return;

	m_xmf3Position.x += m_xmf3MoveDirection.x * m_fSpeed * fElapsedTime;
	m_xmf3Position.y += m_xmf3MoveDirection.y * m_fSpeed * fElapsedTime;
	m_xmf3Position.z += m_xmf3MoveDirection.z * m_fSpeed * fElapsedTime;

	m_Objects->SetPosition(m_xmf3Position);

	m_fElapsedTime += fElapsedTime;
	if (m_fLifetime > 0.0f && m_fElapsedTime >= m_fLifetime)
	{
		m_bActive = false;
		m_fElapsedTime = 0.0f;
		m_Objects->SetRenderState(false);
	}
}

void CProjectile::UpdateWorldMatrix()
{
	XMMATRIX mtxTranslate = XMMatrixTranslation(m_xmf3Position.x, m_xmf3Position.y, m_xmf3Position.z);
	XMStoreFloat4x4(&m_xmf4x4WorldMatrix, mtxTranslate);
}
