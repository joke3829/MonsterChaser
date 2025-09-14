#include "UIObject.h"

UIObject::UIObject(unsigned int crIndex, unsigned int trIndex, Mesh* mesh, CTexture* texture)
	: m_nCBRootParameterIndex(crIndex), m_nTextureRootParameterIndex(trIndex), m_pMesh(mesh), m_pTexture(texture)
{
	auto desc = BASIC_BUFFER_DESC;
	desc.Width = sizeof(XMFLOAT4) * 4;
	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_ColorBuffer.GetAddressOf()));
	m_vColor.assign(4, XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));

	desc.Width = Align(sizeof(UIConstant), 256);
	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_CB.GetAddressOf()));

	m_ColorBuffer->Map(0, nullptr, &m_pColorMappedptr);
	m_CB->Map(0, nullptr, reinterpret_cast<void**>(&m_pMap));

	XMStoreFloat4x4(&m_WorldMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_uvTransform, XMMatrixIdentity());
	XMStoreFloat4x4(&m_pMap->worldmatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_pMap->uvTransform, XMMatrixIdentity());
	m_pMap->bHasTexture = (texture) ? 1 : 0;
}
UIObject::~UIObject()
{
	m_ColorBuffer->Unmap(0, nullptr);
	m_CB->Unmap(0, nullptr);

	m_pMesh = nullptr;
	m_pTexture = nullptr;
}

void UIObject::setPositionInViewport(int x, int y)
{
	m_WorldMatrix._41 = static_cast<float>(x) - (DEFINED_UAV_BUFFER_WIDTH / 2);
	m_WorldMatrix._42 = -static_cast<float>(y) + (DEFINED_UAV_BUFFER_HEIGHT / 2);
	XMStoreFloat4x4(&m_pMap->worldmatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_WorldMatrix)));
}
void UIObject::setPositionInWorld(float x, float y)
{
	m_WorldMatrix._41 = x;
	m_WorldMatrix._42 = y;
	XMStoreFloat4x4(&m_pMap->worldmatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_WorldMatrix)));
}

void UIObject::setScale(XMFLOAT3& scale)
{
	m_WorldMatrix._11 = scale.x; m_WorldMatrix._22 = scale.y; m_WorldMatrix._33 = scale.z;
	XMStoreFloat4x4(&m_pMap->worldmatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_WorldMatrix)));
}

void UIObject::setScale(float scale)
{
	m_WorldMatrix._11 = scale; m_WorldMatrix._22 = scale; m_WorldMatrix._33 = scale;
	XMStoreFloat4x4(&m_pMap->worldmatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_WorldMatrix)));
}

void UIObject::setScaleX(float xScale)
{
	m_WorldMatrix._11 = xScale;
	XMStoreFloat4x4(&m_pMap->worldmatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_WorldMatrix)));
}

void UIObject::setScaleXWithUV(float xScale)
{
	m_WorldMatrix._11 = m_uvTransform._11 = xScale;
	XMStoreFloat4x4(&m_pMap->worldmatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_WorldMatrix)));
	XMStoreFloat4x4(&m_pMap->uvTransform, XMMatrixTranspose(XMLoadFloat4x4(&m_uvTransform)));
}

void UIObject::setScaleY(float yScale)
{
	m_WorldMatrix._22 = yScale;
	XMStoreFloat4x4(&m_pMap->worldmatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_WorldMatrix)));
}

void UIObject::setColor(float r, float g, float b, float a)
{
	for (XMFLOAT4& cor : m_vColor) {
		cor.x = r; cor.y = g; cor.z = b; cor.w = a;
	}
	memcpy(m_pColorMappedptr, m_vColor.data(), sizeof(XMFLOAT4) * 4);
}

void UIObject::Animation(float fElapsedTime)
{
	m_uvTransform._41 += 0.1f * fElapsedTime;
	XMStoreFloat4x4(&m_pMap->uvTransform, XMMatrixTranspose(XMLoadFloat4x4(&m_uvTransform)));
}

void UIObject::Render()
{
	if (m_bRenderState) {
		g_DxResource.cmdList->SetGraphicsRootConstantBufferView(m_nCBRootParameterIndex, m_CB->GetGPUVirtualAddress());
		if (m_pTexture) {
			ID3D12DescriptorHeap* tt = m_pTexture->getView();
			g_DxResource.cmdList->SetDescriptorHeaps(1, &tt);
			g_DxResource.cmdList->SetGraphicsRootDescriptorTable(m_nTextureRootParameterIndex, m_pTexture->getView()->GetGPUDescriptorHandleForHeapStart());
		}
		D3D12_VERTEX_BUFFER_VIEW vv[3]{};
		vv[0].BufferLocation = m_pMesh->getVertexBuffer()->GetGPUVirtualAddress();
		vv[0].SizeInBytes = sizeof(XMFLOAT3) * m_pMesh->getVertexCount();
		vv[0].StrideInBytes = sizeof(XMFLOAT3);

		vv[1].BufferLocation = m_pMesh->getTexCoord0Buffer()->GetGPUVirtualAddress();
		vv[1].SizeInBytes = sizeof(XMFLOAT2) * m_pMesh->getTexCoord0Count();
		vv[1].StrideInBytes = sizeof(XMFLOAT2);

		vv[2].BufferLocation = m_ColorBuffer->GetGPUVirtualAddress();
		vv[2].SizeInBytes = sizeof(XMFLOAT4) * 4;
		vv[2].StrideInBytes = sizeof(XMFLOAT4);

		unsigned int indices = m_pMesh->getIndexCount(0);

		D3D12_INDEX_BUFFER_VIEW iv{};
		iv.BufferLocation = m_pMesh->getIndexBuffer(0)->GetGPUVirtualAddress();
		iv.Format = DXGI_FORMAT_R32_UINT;
		iv.SizeInBytes = sizeof(UINT) * indices;

		g_DxResource.cmdList->IASetVertexBuffers(0, 3, vv);
		g_DxResource.cmdList->IASetIndexBuffer(&iv);

		g_DxResource.cmdList->DrawIndexedInstanced(indices, 1, 0, 0, 0);
	}
}