#pragma once
#include "stdafx.h"
#include "GameObject.h"

extern DXResources g_DxResource;

struct ParticleVertex {
	XMFLOAT3 position;
	XMFLOAT3 direction;
	float lifeTime;
	UINT particleType;
};

class CParticle {
public:
	CParticle();
	CParticle(UINT maxVertex);

	XMFLOAT3 getPosition();

	virtual UINT getInstanceID() { return 0; }
	virtual Material& getMaterial() { return m_material; }
	BoundingSphere& getBoundingSphere() { return m_Sphere; }
	XMFLOAT4X4& getWorldMatrix() { return m_WorldMatrix; }

	void setTarget(CGameObject* target) { m_pTarget = target; }
	void setPosition(XMFLOAT3& pos);
	virtual void setMaterial(Material& material) { m_material = material; }
	virtual void setNotUseLightCalurate() {}
	virtual void setOnePathPipeline(ComPtr<ID3D12PipelineState>& ps) { m_OnePathPipeline = ps; }
	virtual void setTwoPathPipeline(ComPtr<ID3D12PipelineState>& ps) { m_TwoPathPipeline = ps; }

	virtual void ParticleSetting(float lifeTime, float endTime = 0.0f, XMFLOAT3 pos = XMFLOAT3());

	virtual void UpdateObject(float fElapsedTime);

	virtual void Render() {}
	virtual void PostProcess();
	
	virtual void Start();
	virtual void Stop();
protected:
	virtual void BufferReady();

	virtual void OnePath() {}
	virtual void TwoPath() {}

	XMFLOAT4X4 m_WorldMatrix{};
	ComPtr<ID3D12Resource> m_WorldBuffer{};

	UINT m_nMaxVertex{};
	UINT m_nCurrentVertex{};

	ComPtr<ID3D12Resource> m_DrawBuffer{};
	ComPtr<ID3D12Resource> m_StreamOutputBuffer{};

	ComPtr<ID3D12Resource> m_FilledSizeBuffer{};
	ComPtr<ID3D12Resource> m_ReadBackBuffer{};
	ComPtr<ID3D12Resource> m_UploadBuffer{};
	UINT64* m_Mappedptr{};

	ComPtr<ID3D12Resource> m_VertexBuffer{};

	ComPtr<ID3D12PipelineState> m_OnePathPipeline{};
	ComPtr<ID3D12PipelineState> m_TwoPathPipeline{};
	Material m_material;

	BoundingSphere m_Sphere{};
	CGameObject* m_pTarget{};

	float m_fElapsedTime{};
	float m_fEndTime{};

	bool m_bStart{ true };
	bool m_bRender{ false };
};


// No Light Particle's InstanceID -> 100  xxxxxxxx
// 
// Default is 99

class CRaytracingParticle : public CParticle {
public:
	CRaytracingParticle();
	CRaytracingParticle(UINT maxVertex);

	ID3D12Resource* getVertexBuffer() { return m_BillboardVertex.Get(); }
	ID3D12Resource* getTexCoordBuffer() { return m_TexCoord0Buffer.Get(); }
	ID3D12Resource* getColorBuffer() { return m_ColorBuffer.Get(); }

	ID3D12Resource* getBLAS() { return m_BLAS.Get(); }
	ID3D12Resource* getMeshCB() { return m_MeshCB.Get(); }
	ID3D12Resource* getMaterialCB() { return m_MaterialCB.Get(); }

	UINT getInstanceID() { return m_nInstanceID; }
	UINT getHitGroupIndex() { return m_nHitGroupIndex; }

	void setMaterial(Material& material);
	void setNotUseLightCalurate() { m_nInstanceID = 100; }		// Not Use
	void setInstanceID(UINT id) { m_nInstanceID = id; }
	void setHitGroupIndex(UINT index) { m_nHitGroupIndex = index; }

	void UpdateObject(float fElapsedTime);

	void Render();

protected:
	void BufferReady();

	void OnePath();
	void TwoPath();

	void InitBLAS();
	void ReBuildBLAS();

	ComPtr<ID3D12Resource> m_MeshCB{};
	ComPtr<ID3D12Resource> m_MaterialCB{};

	// use 2 slot
	ComPtr<ID3D12Resource> m_BillboardVertex{};
	ComPtr<ID3D12Resource> m_TexCoord0Buffer{};
	ComPtr<ID3D12Resource> m_ColorBuffer{};

	ComPtr<ID3D12Resource> m_BLAS{};
	ComPtr<ID3D12Resource> m_ScratchBuffer{};

	UINT m_nInstanceID = 99;
	UINT m_nHitGroupIndex{};
};
