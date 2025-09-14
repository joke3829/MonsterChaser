// ==================================================
// AccelerationStructureManager.h
// 하나의 Scene마다 해당 AS 가지게 한다.
// ==================================================

#pragma once

#include "ResourceManager.h"

extern DXResources g_DxResource;

// AS를 한번에 관리하는 클래스
class CAccelerationStructureManager {
public:
	void Setup(CResourceManager* resourceManager, UINT parameterIndex);

	void SetScene();				// 셰이더에 TLAS를 넘겨준다.
	void UpdateScene(XMFLOAT3& cameraEye);				// instance 정보를 받아 행렬및 장면을 업데이트 해주자

	// BLASList에 BLAS를 추가한다.
	/*void AddBLAS(ID3D12Resource* vertexBuffer, UINT vertexcount, UINT64 vertexStride, DXGI_FORMAT vertexFormat,
		ID3D12Resource* indexBuffer = nullptr, UINT indices = 0, DXGI_FORMAT indexFormat = DXGI_FORMAT_UNKNOWN);*/
	void InitBLAS();
	// BLAS를 생성한다.
	/*void MakeBLAS(ComPtr<ID3D12Resource>& asResource,
		ID3D12Resource* vertexBuffer, UINT vertexCount, UINT64 vertexStride, DXGI_FORMAT vertexFormat,
		ID3D12Resource* indexBuffer = nullptr, UINT indices = 0, DXGI_FORMAT indexFormat = DXGI_FORMAT_UNKNOWN, bool bOpaque = true);*/
	void MakeBLAS(ComPtr<ID3D12Resource>& resource, std::unique_ptr<Mesh>& mesh);
	// TLAS를 생성한다.
	void InitTLAS();
	// AccelerationStructure를 만든다.
	void MakeAccelerationStructure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs,
		ComPtr<ID3D12Resource>& asResource, UINT64* updateScratchSize = nullptr, bool allowUpdate = false);

private:
	CResourceManager* m_pResourceManager{};

	ComPtr<ID3D12Resource> m_TLAS{};						// TLAS는 하나만 사용, BLASList를 토대로 만든다.
	ComPtr<ID3D12Resource> m_tlasUpdataeScratch{};
	ComPtr<ID3D12Resource> m_InstanceBuffer{};				// TLAS에 들어갈 instance 버퍼
	D3D12_RAYTRACING_INSTANCE_DESC* m_pInstanceData{};		// 

	std::vector<ComPtr<ID3D12Resource>> m_vBLASList;		// BLAS

	UINT m_nValidObject{};			// 유효한 오브젝트(인스턴스) 개수
	UINT m_nRootParameterIndex{};	// 루트 파라미터 인덱스
	UINT m_nStaticMesh{};
	bool m_bFirst = true;
};

