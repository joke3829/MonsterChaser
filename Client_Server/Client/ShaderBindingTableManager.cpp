#include "ShaderBindingTableManager.h"

// 여기에 셰이더 이름을 사용할 개수 만큼 쓴다.
const wchar_t* RayGenShaderNames[] = { L"RayGenShader" };
const wchar_t* MissShaderNames[] = { L"RadianceMiss", L"ShadowMiss" };

struct LocalRootArg {
	D3D12_GPU_VIRTUAL_ADDRESS CBufferGPUVirtualAddress;		// Material상수버퍼
	D3D12_GPU_VIRTUAL_ADDRESS MeshCBufferGPUVirtualAddress;	// Mesh 상수버퍼	
	// Mesh 정보
	D3D12_GPU_VIRTUAL_ADDRESS VertexBuffer;
	D3D12_GPU_VIRTUAL_ADDRESS ColorsBuffer;
	D3D12_GPU_VIRTUAL_ADDRESS TexCoord0Buffer;
	D3D12_GPU_VIRTUAL_ADDRESS TexCoord1Buffer;
	D3D12_GPU_VIRTUAL_ADDRESS NormalsBuffer;
	D3D12_GPU_VIRTUAL_ADDRESS TangentBuffer;
	D3D12_GPU_VIRTUAL_ADDRESS BiTangentBuffer;
	D3D12_GPU_VIRTUAL_ADDRESS IndexBuffer;

	D3D12_GPU_DESCRIPTOR_HANDLE AlbedoMap;
	D3D12_GPU_DESCRIPTOR_HANDLE SpecularMap;
	D3D12_GPU_DESCRIPTOR_HANDLE NormalMap;
	D3D12_GPU_DESCRIPTOR_HANDLE MetallicMap;
	D3D12_GPU_DESCRIPTOR_HANDLE EmissionMap;
	D3D12_GPU_DESCRIPTOR_HANDLE DetailAlbedoMap;
	D3D12_GPU_DESCRIPTOR_HANDLE DetailNormalMap;
};

void CShaderBindingTableManager::Setup(CRayTracingPipeline* pipeline, CResourceManager* manager)
{
	m_pRaytracingPipeline = pipeline;
	m_pResourceManager = manager;

	// null 버퍼와 nulltexture
	//auto desc = BASIC_BUFFER_DESC;
	//desc.Width = 1;			// 여기 의심

	//g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pd3dNullBuffer.GetAddressOf()));

	//desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	//desc.Width = desc.Height = 1;
	//desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	//desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	//g_DxResource.device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pd3dNullTexture.GetAddressOf()));

	D3D12_DESCRIPTOR_HEAP_DESC descriptorDesc{};
	descriptorDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorDesc.NumDescriptors = 1;
	descriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	g_DxResource.device->CreateDescriptorHeap(&descriptorDesc, IID_PPV_ARGS(m_pd3dNullBufferView.GetAddressOf()));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;

	g_DxResource.device->CreateShaderResourceView(g_DxResource.nullTexture.Get(), &srvDesc, m_pd3dNullBufferView->GetCPUDescriptorHandleForHeapStart());
}

void CShaderBindingTableManager::CreateSBT()
{
	ID3D12StateObjectProperties* properties{};
	m_pRaytracingPipeline->getPipelineState()->QueryInterface(&properties);

	std::vector<void*> raygenIDs;		// Raygen은 하나만 쓰는거라 생각하지만 일단은 vector로 사용
	std::vector<void*> MissIDs;
	std::vector<void*> HitGroupIDs;

	std::vector<LPCWSTR>& exports = m_pRaytracingPipeline->getExports();
	// Identifier 사용 준비
	for (int i = 0; i < std::size(RayGenShaderNames); ++i) {
		void* raygenID = properties->GetShaderIdentifier(RayGenShaderNames[i]);
		raygenIDs.emplace_back(raygenID);
	}

	for (int i = 0; i < std::size(MissShaderNames); ++i) {
		void* MissID = properties->GetShaderIdentifier(MissShaderNames[i]);
		MissIDs.emplace_back(MissID);
	}

	for (int i = 0; i < exports.size(); ++i) {
		void* HitGroupID = properties->GetShaderIdentifier(exports[i]);
		HitGroupIDs.emplace_back(HitGroupID);
	}

	auto makeBuffer = [&](ComPtr<ID3D12Resource>& buffer, UINT size)
		{
			auto desc = BASIC_BUFFER_DESC;
			desc.Width = size;
			g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer));
		};

	// RaygenTable
	{
		void* tempdata;
		makeBuffer(m_pRayGenTable, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT * raygenIDs.size());
		m_pRayGenTable->Map(0, nullptr, &tempdata);
		for (int i = 0; i < raygenIDs.size(); ++i) {
			memcpy(tempdata, raygenIDs[i], D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			tempdata = static_cast<char*>(tempdata) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		}
		m_pRayGenTable->Unmap(0, nullptr);
	}

	// MissTable
	{
		void* tempdata;
		makeBuffer(m_pMissTable, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT * MissIDs.size());
		m_nMissSize = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT * MissIDs.size();
		m_pMissTable->Map(0, nullptr, &tempdata);
		for (int i = 0; i < MissIDs.size(); ++i) {
			memcpy(tempdata, MissIDs[i], D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			tempdata = static_cast<char*>(tempdata) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		}
		m_pMissTable->Unmap(0, nullptr);
	}

	// HitGroupTable
	{
		// 테스트용
		/*
		void* tempdata;
		makeBuffer(m_pHitGroupTable, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT * HitGroupIDs.size());
		m_nHitGroupSize = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT * HitGroupIDs.size();
		m_pHitGroupTable->Map(0, nullptr, &tempdata);
		for (int i = 0; i < HitGroupIDs.size(); ++i) {
			memcpy(tempdata, HitGroupIDs[i], D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			tempdata = static_cast<char*>(tempdata) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		}
		m_pHitGroupTable->Unmap(0, nullptr);
		*/

		m_nHitGroupSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeof(LocalRootArg);
		m_nHitGroupStride = Align(m_nHitGroupSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

		UINT nVaildMeshes{};
		std::vector<std::unique_ptr<CGameObject>>& vObjects = m_pResourceManager->getGameObjectList();
		std::vector<std::unique_ptr<Mesh>>& vMeshes = m_pResourceManager->getMeshList();
		std::vector<std::unique_ptr<CTexture>>& vTextures = m_pResourceManager->getTextureList();
		// Skinning
		std::vector<std::unique_ptr<CSkinningObject>>& vSkinnings = m_pResourceManager->getSkinningObjectList();
		std::vector<std::unique_ptr<CParticle>>& vParticles = m_pResourceManager->getParticleList();

		for (std::unique_ptr<CGameObject>& object : vObjects) {
			int n = object->getMeshIndex();
			if (n != -1) {
				if (vMeshes[n]->getHasVertex()) {
					if (vMeshes[n]->getHasSubmesh())
						nVaildMeshes += vMeshes[n]->getSubMeshCount();
					else
						nVaildMeshes += 1;
				}
			}
		}
		// Skinning
		for (std::unique_ptr<CSkinningObject>& SkinningObject : vSkinnings) {
			std::vector<std::shared_ptr<Mesh>>& sMeshes = SkinningObject->getMeshes();
			for (std::unique_ptr<CGameObject>& object : SkinningObject->getObjects()) {
				int n = object->getMeshIndex();
				if (n != -1) {
					if (sMeshes[n]->getHasVertex()) {
						if (sMeshes[n]->getHasSubmesh())
							nVaildMeshes += sMeshes[n]->getSubMeshCount();
						else
							nVaildMeshes += 1;
					}
				}
			}
		}
		// Particle
		nVaildMeshes += vParticles.size();

		makeBuffer(m_pHitGroupTable, m_nHitGroupStride * nVaildMeshes * exports.size());
		m_nHitGroupSize = m_pHitGroupTable.Get()->GetDesc().Width;

		void* ptrStride{};
		void* ptrtemp{};
		m_pHitGroupTable->Map(0, nullptr, &ptrStride);
		ptrtemp = ptrStride;
		int nRecords{};
		for (std::unique_ptr<CGameObject>& object : vObjects) {
			int n = object->getMeshIndex();
			if (n != -1) {
				std::vector<Material>& vMaterials = object->getMaterials();
				if (vMeshes[n]->getHasVertex()) {
					object->SetHitGroupIndex(nRecords);	// object의 hitgroup index 설정
					if (vMeshes[n]->getHasSubmesh()) {	// 서브 메시를 가질 때(인덱스를 가질 때)
						for (int i = 0; i < vMeshes[n]->getSubMeshCount(); ++i) {	// i == object의 mesh 인덱스
							for (int j = 0; j < HitGroupIDs.size(); ++j) {			// j == IDs 인덱스
								LocalRootArg args{};
								{
									args.CBufferGPUVirtualAddress = object->getCbuffer(i)->GetGPUVirtualAddress();
									args.MeshCBufferGPUVirtualAddress = object->getMeshCBuffer()->GetGPUVirtualAddress();
									// 정점
									args.VertexBuffer = vMeshes[n]->getVertexBuffer()->GetGPUVirtualAddress();
									// 컬러
									if (vMeshes[n]->getHasColor())
										args.ColorsBuffer = vMeshes[n]->getColorsBuffer()->GetGPUVirtualAddress();
									else
										args.ColorsBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
									// Tex0
									if (vMeshes[n]->getHasTex0())
										args.TexCoord0Buffer = vMeshes[n]->getTexCoord0Buffer()->GetGPUVirtualAddress();
									else
										args.TexCoord0Buffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
									// Tex1
									if (vMeshes[n]->getHasTex1())
										args.TexCoord1Buffer = vMeshes[n]->getTexCoord1Buffer()->GetGPUVirtualAddress();
									else
										args.TexCoord1Buffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
									// Normal
									if (vMeshes[n]->getHasNormal())
										args.NormalsBuffer = vMeshes[n]->getNormalsBuffer()->GetGPUVirtualAddress();
									else
										args.NormalsBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
									// Tangent
									if (vMeshes[n]->getHasTangent())
										args.TangentBuffer = vMeshes[n]->getTangentsBuffer()->GetGPUVirtualAddress();
									else
										args.TangentBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
									// BiTangent
									if (vMeshes[n]->getHasBiTangent())
										args.BiTangentBuffer = vMeshes[n]->getBiTangentsBuffer()->GetGPUVirtualAddress();
									else
										args.BiTangentBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
									// Index
									args.IndexBuffer = vMeshes[n]->getIndexBuffer(i)->GetGPUVirtualAddress();

									// AlbedoMap
									if (vMaterials[i].m_bHasAlbedoMap)
										args.AlbedoMap = vTextures[vMaterials[i].m_nAlbedoMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
									else
										args.AlbedoMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
									// SpecularMap
									if (vMaterials[i].m_bHasSpecularMap)
										args.SpecularMap = vTextures[vMaterials[i].m_nSpecularMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
									else
										args.SpecularMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
									// NormalMap
									if (vMaterials[i].m_bHasNormalMap)
										args.NormalMap = vTextures[vMaterials[i].m_nNormalMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
									else
										args.NormalMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
									// MetallicMap
									if (vMaterials[i].m_bHasMetallicMap)
										args.MetallicMap = vTextures[vMaterials[i].m_nMetallicMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
									else
										args.MetallicMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
									// EmissionMap
									if (vMaterials[i].m_bHasEmissionMap)
										args.EmissionMap = vTextures[vMaterials[i].m_nEmissionMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
									else
										args.EmissionMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
									// DetailAlbedoMap
									if (vMaterials[i].m_bHasDetailAlbedoMap)
										args.DetailAlbedoMap = vTextures[vMaterials[i].m_nDetailAlbedoMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
									else
										args.DetailAlbedoMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
									// DetailNormalMap
									if (vMaterials[i].m_bHasDetailNormalMap)
										args.DetailNormalMap = vTextures[vMaterials[i].m_nDetailNormalMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
									else
										args.DetailNormalMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
								}
								memcpy(ptrtemp, HitGroupIDs[j], D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
								ptrtemp = static_cast<char*>(ptrtemp) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
								memcpy(ptrtemp, &args, sizeof(LocalRootArg));
								ptrStride = static_cast<char*>(ptrStride) + m_nHitGroupStride;
								ptrtemp = ptrStride;

								++nRecords;
							}
						}
					}
					else {	// 정점 정보는 있으나 인덱스가 없을 때
						for (int j = 0; j < HitGroupIDs.size(); ++j) {
							LocalRootArg args{};
							{
								args.CBufferGPUVirtualAddress = object->getCbuffer(0)->GetGPUVirtualAddress();
								args.MeshCBufferGPUVirtualAddress = object->getMeshCBuffer()->GetGPUVirtualAddress();
								// 정점
								args.VertexBuffer = vMeshes[n]->getVertexBuffer()->GetGPUVirtualAddress();
								// 컬러
								if (vMeshes[n]->getHasColor())
									args.ColorsBuffer = vMeshes[n]->getColorsBuffer()->GetGPUVirtualAddress();
								else
									args.ColorsBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
								// Tex0
								if (vMeshes[n]->getHasTex0())
									args.TexCoord0Buffer = vMeshes[n]->getTexCoord0Buffer()->GetGPUVirtualAddress();
								else
									args.TexCoord0Buffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
								// Tex1
								if (vMeshes[n]->getHasTex1())
									args.TexCoord1Buffer = vMeshes[n]->getTexCoord1Buffer()->GetGPUVirtualAddress();
								else
									args.TexCoord1Buffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
								// Normal
								if (vMeshes[n]->getHasNormal())
									args.NormalsBuffer = vMeshes[n]->getNormalsBuffer()->GetGPUVirtualAddress();
								else
									args.NormalsBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
								// Tangent
								if (vMeshes[n]->getHasTangent())
									args.TangentBuffer = vMeshes[n]->getTangentsBuffer()->GetGPUVirtualAddress();
								else
									args.TangentBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
								// BiTangent
								if (vMeshes[n]->getHasBiTangent())
									args.BiTangentBuffer = vMeshes[n]->getBiTangentsBuffer()->GetGPUVirtualAddress();
								else
									args.BiTangentBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
								// Index
								args.IndexBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();

								// AlbedoMap
								if (vMaterials[0].m_bHasAlbedoMap)
									args.AlbedoMap = vTextures[vMaterials[0].m_nAlbedoMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
								else
									args.AlbedoMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
								// SpecularMap
								if (vMaterials[0].m_bHasSpecularMap)
									args.SpecularMap = vTextures[vMaterials[0].m_nSpecularMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
								else
									args.SpecularMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
								// NormalMap
								if (vMaterials[0].m_bHasNormalMap)
									args.NormalMap = vTextures[vMaterials[0].m_nNormalMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
								else
									args.NormalMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
								// MetallicMap
								if (vMaterials[0].m_bHasMetallicMap)
									args.MetallicMap = vTextures[vMaterials[0].m_nMetallicMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
								else
									args.MetallicMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
								// EmissionMap
								if (vMaterials[0].m_bHasEmissionMap)
									args.EmissionMap = vTextures[vMaterials[0].m_nEmissionMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
								else
									args.EmissionMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
								// DetailAlbedoMap
								if (vMaterials[0].m_bHasDetailAlbedoMap)
									args.DetailAlbedoMap = vTextures[vMaterials[0].m_nDetailAlbedoMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
								else
									args.DetailAlbedoMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
								// DetailNormalMap
								if (vMaterials[0].m_bHasDetailNormalMap)
									args.DetailNormalMap = vTextures[vMaterials[0].m_nDetailNormalMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
								else
									args.DetailNormalMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
							}
							memcpy(ptrtemp, HitGroupIDs[j], D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
							ptrtemp = static_cast<char*>(ptrtemp) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
							memcpy(ptrtemp, &args, sizeof(LocalRootArg));
							ptrStride = static_cast<char*>(ptrStride) + m_nHitGroupStride;
							ptrtemp = ptrStride;

							++nRecords;
						}
					}
				}
			}
		}
		for (std::unique_ptr<CSkinningObject>& SkinningObject : vSkinnings) {
			std::vector<std::shared_ptr<Mesh>>& sMeshes = SkinningObject->getMeshes();
			std::vector<std::shared_ptr<CTexture>>& vSTexture = SkinningObject->getTextures();
			for (std::unique_ptr<CGameObject>& object : SkinningObject->getObjects()) {
				int n = object->getMeshIndex();
				if (n != -1) {
					std::vector<Material>& vMaterials = object->getMaterials();
					if (sMeshes[n]->getHasVertex()) {
						object->SetHitGroupIndex(nRecords);	// object의 hitgroup index 설정
						if (sMeshes[n]->getHasSubmesh()) {	// 서브 메시를 가질 때(인덱스를 가질 때)
							for (int i = 0; i < sMeshes[n]->getSubMeshCount(); ++i) {	// i == object의 mesh 인덱스
								for (int j = 0; j < HitGroupIDs.size(); ++j) {			// j == IDs 인덱스
									LocalRootArg args{};
									{
										args.CBufferGPUVirtualAddress = object->getCbuffer(i)->GetGPUVirtualAddress();
										args.MeshCBufferGPUVirtualAddress = object->getMeshCBuffer()->GetGPUVirtualAddress();
										// 정점
										if (SkinningObject->getVertexOutputBuffer(n))	// 레이트레이싱 Skinning이 아닐때를 확인 못함
											args.VertexBuffer = SkinningObject->getVertexOutputBuffer(n)->GetGPUVirtualAddress();
										else
											args.VertexBuffer = sMeshes[n]->getVertexBuffer()->GetGPUVirtualAddress();
										// 컬러
										if (sMeshes[n]->getHasColor())
											args.ColorsBuffer = sMeshes[n]->getColorsBuffer()->GetGPUVirtualAddress();
										else
											args.ColorsBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
										// Tex0
										if (sMeshes[n]->getHasTex0())
											args.TexCoord0Buffer = sMeshes[n]->getTexCoord0Buffer()->GetGPUVirtualAddress();
										else
											args.TexCoord0Buffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
										// Tex1
										if (sMeshes[n]->getHasTex1())
											args.TexCoord1Buffer = sMeshes[n]->getTexCoord1Buffer()->GetGPUVirtualAddress();
										else
											args.TexCoord1Buffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
										// Normal
										if (sMeshes[n]->getHasNormal()) {
											if (SkinningObject->getNormalOutputBuffer(n))
												args.NormalsBuffer = SkinningObject->getNormalOutputBuffer(n)->GetGPUVirtualAddress();
											else
												args.NormalsBuffer = sMeshes[n]->getNormalsBuffer()->GetGPUVirtualAddress();
										}
										else
											args.NormalsBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
										// Tangent
										if (sMeshes[n]->getHasTangent()) {
											if (SkinningObject->getTangentOutputBuffer(n))
												args.TangentBuffer = SkinningObject->getTangentOutputBuffer(n)->GetGPUVirtualAddress();
											else
												args.TangentBuffer = sMeshes[n]->getTangentsBuffer()->GetGPUVirtualAddress();
										}
										else
											args.TangentBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
										// BiTangent
										if (sMeshes[n]->getHasBiTangent()) {
											if (SkinningObject->getBiTangentOutputBuffer(n))
												args.BiTangentBuffer = SkinningObject->getBiTangentOutputBuffer(n)->GetGPUVirtualAddress();
											else
												args.BiTangentBuffer = sMeshes[n]->getBiTangentsBuffer()->GetGPUVirtualAddress();
										}
										else
											args.BiTangentBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
										// Index
										args.IndexBuffer = sMeshes[n]->getIndexBuffer(i)->GetGPUVirtualAddress();

										// AlbedoMap
										if (vMaterials[i].m_bHasAlbedoMap)
											args.AlbedoMap = vSTexture[vMaterials[i].m_nAlbedoMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
										else
											args.AlbedoMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
										// SpecularMap
										if (vMaterials[i].m_bHasSpecularMap)
											args.SpecularMap = vSTexture[vMaterials[i].m_nSpecularMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
										else
											args.SpecularMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
										// NormalMap
										if (vMaterials[i].m_bHasNormalMap)
											args.NormalMap = vSTexture[vMaterials[i].m_nNormalMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
										else
											args.NormalMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
										// MetallicMap
										if (vMaterials[i].m_bHasMetallicMap)
											args.MetallicMap = vSTexture[vMaterials[i].m_nMetallicMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
										else
											args.MetallicMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
										// EmissionMap
										if (vMaterials[i].m_bHasEmissionMap)
											args.EmissionMap = vSTexture[vMaterials[i].m_nEmissionMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
										else
											args.EmissionMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
										// DetailAlbedoMap
										if (vMaterials[i].m_bHasDetailAlbedoMap)
											args.DetailAlbedoMap = vSTexture[vMaterials[i].m_nDetailAlbedoMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
										else
											args.DetailAlbedoMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
										// DetailNormalMap
										if (vMaterials[i].m_bHasDetailNormalMap)
											args.DetailNormalMap = vSTexture[vMaterials[i].m_nDetailNormalMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
										else
											args.DetailNormalMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
									}
									memcpy(ptrtemp, HitGroupIDs[j], D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
									ptrtemp = static_cast<char*>(ptrtemp) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
									memcpy(ptrtemp, &args, sizeof(LocalRootArg));
									ptrStride = static_cast<char*>(ptrStride) + m_nHitGroupStride;
									ptrtemp = ptrStride;

									++nRecords;
								}
							}
						}
						else {	// 정점 정보는 있으나 인덱스가 없을 때
							for (int j = 0; j < HitGroupIDs.size(); ++j) {
								LocalRootArg args{};
								{
									args.CBufferGPUVirtualAddress = object->getCbuffer(0)->GetGPUVirtualAddress();
									args.MeshCBufferGPUVirtualAddress = object->getMeshCBuffer()->GetGPUVirtualAddress();
									// 정점
									if (SkinningObject->getVertexOutputBuffer(n))	// 레이트레이싱 Skinning이 아닐때를 확인 못함
										args.VertexBuffer = SkinningObject->getVertexOutputBuffer(n)->GetGPUVirtualAddress();
									else
										args.VertexBuffer = sMeshes[n]->getVertexBuffer()->GetGPUVirtualAddress();
									// 컬러
									if (sMeshes[n]->getHasColor())
										args.ColorsBuffer = sMeshes[n]->getColorsBuffer()->GetGPUVirtualAddress();
									else
										args.ColorsBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
									// Tex0
									if (sMeshes[n]->getHasTex0())
										args.TexCoord0Buffer = sMeshes[n]->getTexCoord0Buffer()->GetGPUVirtualAddress();
									else
										args.TexCoord0Buffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
									// Tex1
									if (sMeshes[n]->getHasTex1())
										args.TexCoord1Buffer = sMeshes[n]->getTexCoord1Buffer()->GetGPUVirtualAddress();
									else
										args.TexCoord1Buffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
									// Normal
									if (sMeshes[n]->getHasNormal()) {
										if (SkinningObject->getNormalOutputBuffer(n))
											args.NormalsBuffer = SkinningObject->getNormalOutputBuffer(n)->GetGPUVirtualAddress();
										else
											args.NormalsBuffer = sMeshes[n]->getNormalsBuffer()->GetGPUVirtualAddress();
									}
									else
										args.NormalsBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
									// Tangent
									if (sMeshes[n]->getHasTangent()) {
										if (SkinningObject->getTangentOutputBuffer(n))
											args.TangentBuffer = SkinningObject->getTangentOutputBuffer(n)->GetGPUVirtualAddress();
										else
											args.TangentBuffer = sMeshes[n]->getTangentsBuffer()->GetGPUVirtualAddress();
									}
									else
										args.TangentBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
									// BiTangent
									if (sMeshes[n]->getHasBiTangent()) {
										if (SkinningObject->getBiTangentOutputBuffer(n))
											args.BiTangentBuffer = SkinningObject->getBiTangentOutputBuffer(n)->GetGPUVirtualAddress();
										else
											args.BiTangentBuffer = sMeshes[n]->getBiTangentsBuffer()->GetGPUVirtualAddress();
									}
									else
										args.BiTangentBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
									// Index
									args.IndexBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();

									// AlbedoMap
									if (vMaterials[0].m_bHasAlbedoMap)
										args.AlbedoMap = vSTexture[vMaterials[0].m_nAlbedoMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
									else
										args.AlbedoMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
									// SpecularMap
									if (vMaterials[0].m_bHasSpecularMap)
										args.SpecularMap = vSTexture[vMaterials[0].m_nSpecularMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
									else
										args.SpecularMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
									// NormalMap
									if (vMaterials[0].m_bHasNormalMap)
										args.NormalMap = vSTexture[vMaterials[0].m_nNormalMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
									else
										args.NormalMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
									// MetallicMap
									if (vMaterials[0].m_bHasMetallicMap)
										args.MetallicMap = vSTexture[vMaterials[0].m_nMetallicMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
									else
										args.MetallicMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
									// EmissionMap
									if (vMaterials[0].m_bHasEmissionMap)
										args.EmissionMap = vSTexture[vMaterials[0].m_nEmissionMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
									else
										args.EmissionMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
									// DetailAlbedoMap
									if (vMaterials[0].m_bHasDetailAlbedoMap)
										args.DetailAlbedoMap = vSTexture[vMaterials[0].m_nDetailAlbedoMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
									else
										args.DetailAlbedoMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
									// DetailNormalMap
									if (vMaterials[0].m_bHasDetailNormalMap)
										args.DetailNormalMap = vSTexture[vMaterials[0].m_nDetailNormalMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
									else
										args.DetailNormalMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
								}
								memcpy(ptrtemp, HitGroupIDs[j], D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
								ptrtemp = static_cast<char*>(ptrtemp) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
								memcpy(ptrtemp, &args, sizeof(LocalRootArg));
								ptrStride = static_cast<char*>(ptrStride) + m_nHitGroupStride;
								ptrtemp = ptrStride;

								++nRecords;
							}
						}
					}
				}
			}
		}

		for (std::unique_ptr<CParticle>& particle : vParticles) {
			CRaytracingParticle* p = dynamic_cast<CRaytracingParticle*>(particle.get());
			Material& myMaterial = p->getMaterial();
			p->setHitGroupIndex(nRecords);
			for (int j = 0; j < HitGroupIDs.size(); ++j) {
				LocalRootArg args{};
				{
					args.CBufferGPUVirtualAddress = p->getMaterialCB()->GetGPUVirtualAddress();
					args.MeshCBufferGPUVirtualAddress = p->getMeshCB()->GetGPUVirtualAddress();
					// 정점
					args.VertexBuffer = p->getVertexBuffer()->GetGPUVirtualAddress();
					// 컬러
					args.ColorsBuffer = p->getColorBuffer()->GetGPUVirtualAddress();
					// Tex0
					args.TexCoord0Buffer = p->getTexCoordBuffer()->GetGPUVirtualAddress();
					// Tex1
					args.TexCoord1Buffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
					// Normal
					args.NormalsBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
					// Tangent
					args.TangentBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
					// BiTangent
					args.BiTangentBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();
					// Index
					args.IndexBuffer = g_DxResource.nullBuffer->GetGPUVirtualAddress();

					// AlbedoMap
					if (myMaterial.m_bHasAlbedoMap)
						args.AlbedoMap = vTextures[myMaterial.m_nAlbedoMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
					else
						args.AlbedoMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
					// SpecularMap
					if (myMaterial.m_bHasSpecularMap)
						args.SpecularMap = vTextures[myMaterial.m_nSpecularMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
					else
						args.SpecularMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
					// NormalMap
					if (myMaterial.m_bHasNormalMap)
						args.NormalMap = vTextures[myMaterial.m_nNormalMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
					else
						args.NormalMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
					// MetallicMap
					if (myMaterial.m_bHasMetallicMap)
						args.MetallicMap = vTextures[myMaterial.m_nMetallicMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
					else
						args.MetallicMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
					// EmissionMap
					if (myMaterial.m_bHasEmissionMap)
						args.EmissionMap = vTextures[myMaterial.m_nEmissionMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
					else
						args.EmissionMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
					// DetailAlbedoMap
					if (myMaterial.m_bHasDetailAlbedoMap)
						args.DetailAlbedoMap = vTextures[myMaterial.m_nDetailAlbedoMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
					else
						args.DetailAlbedoMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
					// DetailNormalMap
					if (myMaterial.m_bHasDetailNormalMap)
						args.DetailNormalMap = vTextures[myMaterial.m_nDetailNormalMapIndex]->getView()->GetGPUDescriptorHandleForHeapStart();
					else
						args.DetailNormalMap = m_pd3dNullBufferView->GetGPUDescriptorHandleForHeapStart();
				}
				memcpy(ptrtemp, HitGroupIDs[j], D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
				ptrtemp = static_cast<char*>(ptrtemp) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
				memcpy(ptrtemp, &args, sizeof(LocalRootArg));
				ptrStride = static_cast<char*>(ptrStride) + m_nHitGroupStride;
				ptrtemp = ptrStride;

				++nRecords;
			}
		}
		m_pHitGroupTable->Unmap(0, nullptr);



	}
}

UINT64 CShaderBindingTableManager::getMissSize() const
{
	return m_nMissSize;
}

ID3D12Resource* CShaderBindingTableManager::getRayGenTable() const
{
	return m_pRayGenTable.Get();
}
ID3D12Resource* CShaderBindingTableManager::getMissTable() const
{
	return m_pMissTable.Get();
}
ID3D12Resource* CShaderBindingTableManager::getHitGroupTable() const
{
	return m_pHitGroupTable.Get();
}

UINT64 CShaderBindingTableManager::getHitGroupSize() const
{
	return m_nHitGroupSize;
}
UINT64 CShaderBindingTableManager::getHitGroupStride() const
{
	return m_nHitGroupStride;
}