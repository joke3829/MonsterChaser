#include "Texture.h"

std::wstring namestr;

CTexture::CTexture(const wchar_t* pszFileName, bool cubeMap, bool bDDS)
{
	g_DxResource.cmdAlloc->Reset();
	g_DxResource.cmdList->Reset(g_DxResource.cmdAlloc, nullptr);

	// 현재는 .dds를 읽는 코드만 사용한다, 필요 시 WIC도 추가하겠다.
	ComPtr<ID3D12Resource> pd3dUploadBuffer{};

	std::unique_ptr<uint8_t[]> decodedData;	// dds 데이터로도 사용
	std::vector<D3D12_SUBRESOURCE_DATA> vSubresources;
	DDS_ALPHA_MODE ddsAlphaMode = DDS_ALPHA_MODE_UNKNOWN;
	bool blsCubeMap = false;
	D3D12_SUBRESOURCE_DATA subResourceData;
	if (bDDS)
		LoadDDSTextureFromFileEx(g_DxResource.device, pszFileName, 0, D3D12_RESOURCE_FLAG_NONE, DDS_LOADER_DEFAULT, m_pd3dTexture.GetAddressOf(), decodedData, vSubresources, &ddsAlphaMode, &blsCubeMap);

	namestr = pszFileName;

	UINT64 nBytes{};
	UINT nSubResource = (UINT)vSubresources.size();
	if (bDDS) {
		nBytes = GetRequiredIntermediateSize(m_pd3dTexture.Get(), 0, nSubResource);
	}
	auto desc = BASIC_BUFFER_DESC;
	desc.Width = nBytes;

	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pd3dUploadBuffer));

	if (bDDS)
		::UpdateSubresources(g_DxResource.cmdList, m_pd3dTexture.Get(), pd3dUploadBuffer.Get(), 0, 0, nSubResource, &vSubresources[0]);

	D3D12_RESOURCE_BARRIER d3dRB;
	::ZeroMemory(&d3dRB, sizeof(D3D12_RESOURCE_BARRIER));
	d3dRB.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dRB.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dRB.Transition.pResource = m_pd3dTexture.Get();
	d3dRB.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	d3dRB.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	d3dRB.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	g_DxResource.cmdList->ResourceBarrier(1, &d3dRB);

	g_DxResource.cmdList->Close();
	g_DxResource.cmdQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(&g_DxResource.cmdList));
	Flush();

	CreateSRV(cubeMap);
}

void CTexture::CreateSRV(bool cubeMap)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NumDescriptors = 1;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	g_DxResource.device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_pd3dShaderResourceView.GetAddressOf()));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	D3D12_RESOURCE_DESC d3dRD = m_pd3dTexture->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = (cubeMap) ? D3D12_SRV_DIMENSION_TEXTURECUBE : D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;

	g_DxResource.device->CreateShaderResourceView(m_pd3dTexture.Get(), &srvDesc, m_pd3dShaderResourceView->GetCPUDescriptorHandleForHeapStart());
}

ID3D12DescriptorHeap* CTexture::getView() const
{
	return m_pd3dShaderResourceView.Get();
}

void CTexture::SetTextureName(std::string name)
{
	m_strName = name;
}

std::string CTexture::getName() const
{
	return m_strName;
}