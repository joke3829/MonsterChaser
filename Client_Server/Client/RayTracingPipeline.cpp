#include "RayTracingPipeline.h"

CRayTracingPipeline::~CRayTracingPipeline()
{
	for (int i = 0; i < m_nSubObjects; ++i) {
		if (m_pSubobjects[i].pDesc)
			delete m_pSubobjects[i].pDesc;
	}
}

void CRayTracingPipeline::Setup(UINT nSubObject)
{
	m_nSubObjects = nSubObject;
	m_nCurrentIndex = 0;
	m_pSubobjects = std::make_unique<D3D12_STATE_SUBOBJECT[]>(m_nSubObjects);
}

void CRayTracingPipeline::AddLibrarySubObject(const unsigned char* compiledShader, size_t shaderSize)
{
	D3D12_DXIL_LIBRARY_DESC* lib = new D3D12_DXIL_LIBRARY_DESC;
	lib->DXILLibrary.pShaderBytecode = compiledShader;
	lib->DXILLibrary.BytecodeLength = shaderSize;
	lib->NumExports = 0;
	lib->pExports = nullptr;

	/*D3D12_STATE_SUBOBJECT temp{};
	temp.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	temp.pDesc = lib;

	m_vSubobjects.emplace_back(temp);*/
	m_pSubobjects[m_nCurrentIndex].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	m_pSubobjects[m_nCurrentIndex].pDesc = lib;
	++m_nCurrentIndex;
}

// 사용할 것만 넣기
void CRayTracingPipeline::AddHitGroupSubObject(wchar_t* exportName, wchar_t* ClosestHit, wchar_t* AnyHit, wchar_t* Intersect)
{
	D3D12_HIT_GROUP_DESC* hitGroup = new D3D12_HIT_GROUP_DESC{};
	hitGroup->Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;	// 바뀔 수도 있음
	hitGroup->ClosestHitShaderImport = ClosestHit;
	if (AnyHit)
		hitGroup->AnyHitShaderImport = AnyHit;
	if (Intersect)
		hitGroup->IntersectionShaderImport = Intersect;
	hitGroup->HitGroupExport = exportName;

	// export를 저장한다.
	m_exports.emplace_back(exportName);

	/*D3D12_STATE_SUBOBJECT temp{};
	temp.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	temp.pDesc = hitGroup;
	m_vSubobjects.emplace_back(temp);*/
	m_pSubobjects[m_nCurrentIndex].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	m_pSubobjects[m_nCurrentIndex].pDesc = hitGroup;
	++m_nCurrentIndex;
}

void CRayTracingPipeline::AddShaderConfigSubObject(UINT nMaxAttributeSize, UINT nMaxPayloadSize)
{
	D3D12_RAYTRACING_SHADER_CONFIG* shaderCfg = new D3D12_RAYTRACING_SHADER_CONFIG;
	shaderCfg->MaxAttributeSizeInBytes = nMaxAttributeSize;
	shaderCfg->MaxPayloadSizeInBytes = nMaxPayloadSize;

	/*D3D12_STATE_SUBOBJECT temp{};
	temp.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	temp.pDesc = shaderCfg;
	m_vSubobjects.emplace_back(temp);*/
	m_pSubobjects[m_nCurrentIndex].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	m_pSubobjects[m_nCurrentIndex].pDesc = shaderCfg;
	++m_nCurrentIndex;
}

void CRayTracingPipeline::AddLocalRootAndAsoociationSubObject(ID3D12RootSignature* pLocalRootSignature)
{
	D3D12_LOCAL_ROOT_SIGNATURE* localrootsig = new D3D12_LOCAL_ROOT_SIGNATURE;
	localrootsig->pLocalRootSignature = pLocalRootSignature;

	/*D3D12_STATE_SUBOBJECT localrootSubObject{};
	localrootSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	localrootSubObject.pDesc = localrootsig;
	m_vSubobjects.emplace_back(localrootSubObject);*/
	m_pSubobjects[m_nCurrentIndex].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	m_pSubobjects[m_nCurrentIndex].pDesc = localrootsig;
	++m_nCurrentIndex;

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION* exportsAss = new D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	exportsAss->pSubobjectToAssociate = &m_pSubobjects[m_nCurrentIndex - 1];
	exportsAss->NumExports = m_exports.size();
	exportsAss->pExports = m_exports.data();

	/*D3D12_STATE_SUBOBJECT temp{};
	temp.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	temp.pDesc = exportsAss;
	m_vSubobjects.emplace_back(temp);*/
	m_pSubobjects[m_nCurrentIndex].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	m_pSubobjects[m_nCurrentIndex].pDesc = exportsAss;
	++m_nCurrentIndex;
}

void CRayTracingPipeline::AddGlobalRootSignatureSubObject(ID3D12RootSignature* pGlobalRootSignature)
{
	D3D12_GLOBAL_ROOT_SIGNATURE* globalSig = new D3D12_GLOBAL_ROOT_SIGNATURE;
	globalSig->pGlobalRootSignature = pGlobalRootSignature;

	/*D3D12_STATE_SUBOBJECT temp{};
	temp.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	temp.pDesc = globalSig;
	m_vSubobjects.emplace_back(temp);*/
	m_pSubobjects[m_nCurrentIndex].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	m_pSubobjects[m_nCurrentIndex].pDesc = globalSig;
	++m_nCurrentIndex;
}

void CRayTracingPipeline::AddPipelineConfigSubObject(UINT nMaxTraceDepth)
{
	D3D12_RAYTRACING_PIPELINE_CONFIG* pipelineCfg = new D3D12_RAYTRACING_PIPELINE_CONFIG;
	pipelineCfg->MaxTraceRecursionDepth = nMaxTraceDepth;

	/*D3D12_STATE_SUBOBJECT temp{};
	temp.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	temp.pDesc = pipelineCfg;
	m_vSubobjects.emplace_back(temp);*/
	m_pSubobjects[m_nCurrentIndex].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	m_pSubobjects[m_nCurrentIndex].pDesc = pipelineCfg;
	++m_nCurrentIndex;
}

void CRayTracingPipeline::MakePipelineState()
{
	D3D12_STATE_OBJECT_DESC desc{};
	desc.NumSubobjects = m_nSubObjects;
	desc.pSubobjects = m_pSubobjects.get();
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	g_DxResource.device->CreateStateObject(&desc, IID_PPV_ARGS(m_pRaytracingPipelineState.GetAddressOf()));
}

ID3D12StateObject* CRayTracingPipeline::getPipelineState()
{
	return m_pRaytracingPipelineState.Get();
}

std::vector<LPCWSTR>& CRayTracingPipeline::getExports()
{
	return m_exports;
}