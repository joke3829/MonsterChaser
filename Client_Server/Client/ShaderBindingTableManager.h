#pragma once

#include "RayTracingPipeline.h"
#include "ResourceManager.h"

extern DXResources g_DxResource;

class CShaderBindingTableManager {
public:
	void Setup(CRayTracingPipeline* pipeline, CResourceManager* manager);

	void CreateSBT();

	ID3D12Resource* getRayGenTable() const;
	ID3D12Resource* getMissTable() const;
	ID3D12Resource* getHitGroupTable() const;

	UINT64 getHitGroupSize() const;
	UINT64 getHitGroupStride() const;
	UINT64 getMissSize() const;
private:
	CRayTracingPipeline* m_pRaytracingPipeline{};
	CResourceManager* m_pResourceManager{};
	ComPtr<ID3D12Resource> m_pRayGenTable{};

	ComPtr<ID3D12Resource> m_pMissTable{};
	UINT64 m_nMissSize{};

	ComPtr<ID3D12Resource> m_pHitGroupTable{};
	UINT64 m_nHitGroupSize{};
	UINT64 m_nHitGroupStride{};

	//ComPtr<ID3D12Resource> m_pd3dNullBuffer{};
	//ComPtr<ID3D12Resource> m_pd3dNullTexture{};
	ComPtr<ID3D12DescriptorHeap> m_pd3dNullBufferView;
};

