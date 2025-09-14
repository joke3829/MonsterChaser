#include "Particle.h"

constexpr unsigned int PARTICLE_EMITTER = 0;
constexpr unsigned int PARTICLE_FLAME = 1;

CParticle::CParticle()
{
	m_nMaxVertex = 42;
	m_nCurrentVertex = 1;
	XMStoreFloat4x4(&m_WorldMatrix, XMMatrixIdentity());
	BufferReady();
}

CParticle::CParticle(UINT maxVertex)
	: m_nMaxVertex(maxVertex)
{
	if (m_nMaxVertex > 42)
		m_nMaxVertex = 42;
	m_nCurrentVertex = 1;
	XMStoreFloat4x4(&m_WorldMatrix, XMMatrixIdentity());
	BufferReady();
}

void CParticle::BufferReady()
{
	m_Sphere = BoundingSphere(XMFLOAT3(0.0, 0.0, 0.0), 10.0f);

	XMStoreFloat4x4(&m_WorldMatrix, XMMatrixIdentity());
	auto desc = BASIC_BUFFER_DESC;
	desc.Width = Align(sizeof(XMFLOAT4X4), 256);

	ID3D12Device5* device = g_DxResource.device;

	device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(m_WorldBuffer.GetAddressOf()));

	// Vertex Emitter
	desc.Width = sizeof(ParticleVertex);
	device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(m_VertexBuffer.GetAddressOf()));

	void* tempData{};

	m_VertexBuffer->Map(0, nullptr, &tempData);
	ParticleVertex tempV = { XMFLOAT3(0.0, 0.0, 0.0), XMFLOAT3(0.0, 1.0, 0.0), 0.0, PARTICLE_EMITTER };
	memcpy(tempData, &tempV, sizeof(tempV));
	m_VertexBuffer->Unmap(0, nullptr);

	desc.Width = m_nMaxVertex * sizeof(ParticleVertex);
	device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(m_DrawBuffer.GetAddressOf()));

	device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_STREAM_OUT,
		nullptr, IID_PPV_ARGS(m_StreamOutputBuffer.GetAddressOf()));

	desc.Width = sizeof(UINT64);
	device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_STREAM_OUT,
		nullptr, IID_PPV_ARGS(m_FilledSizeBuffer.GetAddressOf()));
	device->CreateCommittedResource(&READBACK_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr, IID_PPV_ARGS(m_ReadBackBuffer.GetAddressOf()));
	device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(m_UploadBuffer.GetAddressOf()));

	m_UploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_Mappedptr));
}

XMFLOAT3 CParticle::getPosition()
{
	return XMFLOAT3(m_WorldMatrix._41, m_WorldMatrix._42, m_WorldMatrix._43);
}

void CParticle::setPosition(XMFLOAT3& pos)
{
	m_WorldMatrix._41 = pos.x; m_WorldMatrix._42 = pos.y; m_WorldMatrix._43 = pos.z;
}

void CParticle::ParticleSetting(float lifeTime, float endTime, XMFLOAT3 pos)
{
	void* tempData{};

	m_VertexBuffer->Map(0, nullptr, &tempData);
	ParticleVertex tempV = { pos, XMFLOAT3(0.0, 0.0, 0.0), lifeTime, PARTICLE_EMITTER};
	memcpy(tempData, &tempV, sizeof(tempV));
	m_VertexBuffer->Unmap(0, nullptr);

	m_fEndTime = endTime;
}

void CParticle::UpdateObject(float fElapsedTime)
{
	// Not Used
}

void CParticle::PostProcess()
{
	if (m_bRender) {
		UINT64* pFilledSize{};
		m_ReadBackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pFilledSize));
		m_nCurrentVertex = UINT(*pFilledSize) / sizeof(ParticleVertex);
		m_ReadBackBuffer->Unmap(0, nullptr);

		if (m_nCurrentVertex < 1)
			m_bStart = true;

		m_StreamOutputBuffer.Swap(m_DrawBuffer);
	}
}

void CParticle::Start()
{
	m_bStart = m_bRender = true;
	m_fElapsedTime = 0.0f;
}

void CParticle::Stop()
{
	m_bRender = false;
	m_nCurrentVertex = 0;
}

// =====================================================================================

CRaytracingParticle::CRaytracingParticle()
	: CParticle()
{
	BufferReady();
}

CRaytracingParticle::CRaytracingParticle(UINT maxVertex)
	: CParticle(maxVertex)
{
	BufferReady();
}

void CRaytracingParticle::BufferReady()
{
	ID3D12Device5* device = g_DxResource.device;
	auto desc = BASIC_BUFFER_DESC;

	desc.Width = Align(sizeof(HasMesh), 256);
	device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(m_MeshCB.GetAddressOf()));

	HasMesh meshInfo{};
	meshInfo.bHasVertex = meshInfo.bHasTex0 = meshInfo.bHasColor = true; 
	void* tempData{};
	m_MeshCB->Map(0, nullptr, &tempData);
	memcpy(tempData, &meshInfo, sizeof(HasMesh));
	m_MeshCB->Unmap(0, nullptr);

	desc.Width = Align(sizeof(HasMaterial), 256);
	device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(m_MaterialCB.GetAddressOf()));

	desc.Width = m_nMaxVertex * 6 * sizeof(XMFLOAT3);
	device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_STREAM_OUT,
		nullptr, IID_PPV_ARGS(m_BillboardVertex.GetAddressOf()));

	desc.Width = m_nMaxVertex * 6 * sizeof(XMFLOAT2);
	device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_STREAM_OUT,
		nullptr, IID_PPV_ARGS(m_TexCoord0Buffer.GetAddressOf()));

	desc.Width = m_nMaxVertex * 6 * sizeof(XMFLOAT4);
	device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_STREAM_OUT,
		nullptr, IID_PPV_ARGS(m_ColorBuffer.GetAddressOf()));

	InitBLAS();
}

void CRaytracingParticle::InitBLAS()
{
	ID3D12Device5* device = g_DxResource.device;
	ID3D12CommandAllocator* cmdAlloc = g_DxResource.cmdAlloc;
	ID3D12GraphicsCommandList4* cmdList = g_DxResource.cmdList;
	ID3D12CommandQueue* cmdQueue = g_DxResource.cmdQueue;

	D3D12_RAYTRACING_GEOMETRY_DESC desc{};
	desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	desc.Triangles.Transform3x4 = 0;
	desc.Triangles.VertexBuffer = {
		.StartAddress = m_BillboardVertex->GetGPUVirtualAddress(),
		.StrideInBytes = sizeof(float) * 3
	};
	desc.Triangles.VertexCount = m_nMaxVertex * 6;
	desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	desc.Triangles.IndexBuffer = 0;
	desc.Triangles.IndexCount = 0;
	desc.Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
	inputs.NumDescs = 1;
	inputs.pGeometryDescs = &desc;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO preBuildInfo{};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &preBuildInfo);

	auto bdesc = BASIC_BUFFER_DESC;
	bdesc.Width = preBuildInfo.ResultDataMaxSizeInBytes;
	bdesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &bdesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nullptr, IID_PPV_ARGS(m_BLAS.GetAddressOf()));

	bdesc.Width = preBuildInfo.ScratchDataSizeInBytes;
	device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &bdesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nullptr, IID_PPV_ARGS(m_ScratchBuffer.GetAddressOf()));

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC ASDesc{};
	ASDesc.Inputs = inputs;
	ASDesc.DestAccelerationStructureData = m_BLAS->GetGPUVirtualAddress();
	ASDesc.ScratchAccelerationStructureData = m_ScratchBuffer->GetGPUVirtualAddress();

	cmdAlloc->Reset();
	cmdList->Reset(cmdAlloc, nullptr);
	cmdList->BuildRaytracingAccelerationStructure(&ASDesc, 0, nullptr);
	D3D12_RESOURCE_BARRIER d3dbr{};
	d3dbr.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	d3dbr.UAV.pResource = m_BLAS.Get();
	cmdList->ResourceBarrier(1, &d3dbr);
	cmdList->Close();
	cmdQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(&cmdList));
	Flush();
}

void CRaytracingParticle::ReBuildBLAS()
{
	D3D12_RAYTRACING_GEOMETRY_DESC desc{};
	desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	desc.Triangles.Transform3x4 = 0;
	desc.Triangles.VertexBuffer = {
		.StartAddress = m_BillboardVertex->GetGPUVirtualAddress(),
		.StrideInBytes = sizeof(float) * 3
	};
	desc.Triangles.VertexCount = m_nCurrentVertex * 6;
	desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	desc.Triangles.IndexBuffer = 0;
	desc.Triangles.IndexCount = 0;
	desc.Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
	inputs.NumDescs = 1;
	inputs.pGeometryDescs = &desc;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC ASDesc{};
	ASDesc.Inputs = inputs;
	ASDesc.DestAccelerationStructureData = m_BLAS->GetGPUVirtualAddress();
	ASDesc.ScratchAccelerationStructureData = m_ScratchBuffer->GetGPUVirtualAddress();

	g_DxResource.cmdList->BuildRaytracingAccelerationStructure(&ASDesc, 0, nullptr);
	D3D12_RESOURCE_BARRIER d3dbr{};
	d3dbr.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	d3dbr.UAV.pResource = m_BLAS.Get();
	g_DxResource.cmdList->ResourceBarrier(1, &d3dbr);
}

void CRaytracingParticle::setMaterial(Material& material)
{
	m_material = material;
	HasMaterial matInfo{};
	// temporary
	if (m_material.m_bHasAlbedoColor) {
		matInfo.bHasAlbedoColor = 1; matInfo.AlbedoColor = m_material.m_xmf4AlbedoColor;
	}
	if (m_material.m_bHasAlbedoMap) {
		matInfo.bHasAlbedoMap = 1;
	}

	void* tempData{};
	m_MaterialCB->Map(0, nullptr, &tempData);
	memcpy(tempData, &matInfo, sizeof(HasMaterial));
	m_MaterialCB->Unmap(0, nullptr);
}

void CRaytracingParticle::UpdateObject(float fElapsedTime)
{
	if (m_bRender) {
		Render();
		if (m_fEndTime != 0.0f) {
			m_fElapsedTime += fElapsedTime;
			if (m_fElapsedTime >= m_fEndTime)
				Stop();
		}
	}
	ReBuildBLAS();
}

void CRaytracingParticle::Render()
{
	OnePath();
	TwoPath();
}

void CRaytracingParticle::OnePath()
{
	ID3D12GraphicsCommandList4* cmdList = g_DxResource.cmdList;
	// setting geometry shader
	// 1. particle vertex update
	D3D12_VERTEX_BUFFER_VIEW vView{};
	if (m_bStart) {
		vView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
		vView.SizeInBytes = sizeof(ParticleVertex);
		vView.StrideInBytes = sizeof(ParticleVertex);
		m_bStart = false; m_nCurrentVertex = 1;
	}
	else {
		vView.BufferLocation = m_DrawBuffer->GetGPUVirtualAddress();
		vView.SizeInBytes = sizeof(ParticleVertex) * m_nCurrentVertex;
		vView.StrideInBytes = sizeof(ParticleVertex);
	}
	*m_Mappedptr = 0;
	cmdList->CopyResource(m_FilledSizeBuffer.Get(), m_UploadBuffer.Get());

	D3D12_STREAM_OUTPUT_BUFFER_VIEW soView{};
	soView.BufferFilledSizeLocation = m_FilledSizeBuffer->GetGPUVirtualAddress();
	soView.BufferLocation = m_StreamOutputBuffer->GetGPUVirtualAddress();
	soView.SizeInBytes = sizeof(ParticleVertex) * m_nMaxVertex;
	
	cmdList->SetPipelineState(m_OnePathPipeline.Get());
	cmdList->SOSetTargets(0, 1, &soView);

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	cmdList->IASetVertexBuffers(0, 1, &vView);
	cmdList->DrawInstanced(m_nCurrentVertex, 1, 0, 0);
}

void CRaytracingParticle::TwoPath()
{
	// setting geometry shader
	// 2. paritcle vertex -> billboard~~
	// Need camera Setting -> executed Scene? 
	// Use VertexBuffer set in OnePath

	auto barrier = [&](ComPtr<ID3D12Resource>& res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
		D3D12_RESOURCE_BARRIER dbr{};
		dbr.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		dbr.Transition.pResource = res.Get();
		dbr.Transition.StateBefore = before;
		dbr.Transition.StateAfter = after;
		dbr.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		g_DxResource.cmdList->ResourceBarrier(1, &dbr);
		};
	ID3D12GraphicsCommandList4* cmdList = g_DxResource.cmdList;

	barrier(m_FilledSizeBuffer, D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE);
	cmdList->CopyResource(m_ReadBackBuffer.Get(), m_FilledSizeBuffer.Get());
	barrier(m_FilledSizeBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_STREAM_OUT);


	D3D12_STREAM_OUTPUT_BUFFER_VIEW soView[3]{};
	soView[0].BufferLocation = m_BillboardVertex->GetGPUVirtualAddress();
	soView[0].SizeInBytes = m_nMaxVertex * 6 * sizeof(XMFLOAT3);

	soView[1].BufferLocation = m_TexCoord0Buffer->GetGPUVirtualAddress();
	soView[1].SizeInBytes = m_nMaxVertex * 6 * sizeof(XMFLOAT2);

	soView[2].BufferLocation = m_ColorBuffer->GetGPUVirtualAddress();
	soView[2].SizeInBytes = m_nMaxVertex * 6 * sizeof(XMFLOAT4);

	cmdList->SetPipelineState(m_TwoPathPipeline.Get());
	cmdList->SOSetTargets(0, 3, soView);

	cmdList->DrawInstanced(m_nCurrentVertex, 1, 0, 0);

}

