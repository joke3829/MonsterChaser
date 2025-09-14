#include "Camera.h"

CCamera::~CCamera()
{
	if (m_pd3dCameraBuffer)
		m_pd3dCameraBuffer->Unmap(0, nullptr);
}

void CCamera::Setup(int nRootParameterIndex)
{
	m_nRootParameterIndex = nRootParameterIndex;

	auto desc = BASIC_BUFFER_DESC;
	desc.Width = Align(sizeof(CB_CAMERA_INFO), 256);
	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pd3dCameraBuffer.GetAddressOf()));
	m_pd3dCameraBuffer->Map(0, nullptr, (void**)&m_pCameraInfo);
	m_pCameraInfo->bNormalMapping = ~0;
	m_pCameraInfo->bReflection = 0;
	m_pCameraInfo->nMapNumber = 0;

	XMStoreFloat4x4(&m_xmf4x4Proj, XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fFOV), m_fAspect, m_fNear, m_fFar));
}

void CCamera::Rotate(int cxDelta, int cyDelta)
{
	float cx = (float)cxDelta / 3.0f;
	float cy = (float)cyDelta / 3.0f;

	m_fLimitcy += cy;
	if (m_fLimitcy > 80.0f) {
		cy -= (m_fLimitcy - 80.0f);
		m_fLimitcy = 80.0f;
	}
	if (m_fLimitcy < -80.0f) {
		cy += -(m_fLimitcy + 80.0f);
		m_fLimitcy = -80.0f;
	}

	XMMATRIX mtxrotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Up), XMConvertToRadians(cx));
	XMStoreFloat3(&m_xmf3Dir, XMVector3TransformNormal(XMLoadFloat3(&m_xmf3Dir), mtxrotate));
	XMStoreFloat3(&m_xmf3Offset, XMVector3TransformNormal(XMLoadFloat3(&m_xmf3Offset), mtxrotate));

	XMVECTOR xmvRight = XMVector3Cross(XMLoadFloat3(&m_xmf3Up), XMLoadFloat3(&m_xmf3Dir));
	mtxrotate = XMMatrixRotationAxis(xmvRight, XMConvertToRadians(cy));
	XMStoreFloat3(&m_xmf3Dir, XMVector3TransformNormal(XMLoadFloat3(&m_xmf3Dir), mtxrotate));

	xmvRight = XMVector3Cross(XMLoadFloat3(&m_xmf3Up), XMLoadFloat3(&m_xmf3Offset));
	mtxrotate = XMMatrixRotationAxis(xmvRight, XMConvertToRadians(cy));
	XMStoreFloat3(&m_xmf3Offset, XMVector3TransformNormal(XMLoadFloat3(&m_xmf3Offset), mtxrotate));
}

void CCamera::Move(int arrow, float fElapsedTime, bool shift)
{
	float speed = (shift) ? 40.0f : 20.0f;

	if (arrow == 0) {	// ��
		XMStoreFloat3(&m_xmf3Eye, XMLoadFloat3(&m_xmf3Eye) + (XMLoadFloat3(&m_xmf3Dir) * speed * fElapsedTime));
	}
	else if (arrow == 1) { // ��
		XMStoreFloat3(&m_xmf3Eye, XMLoadFloat3(&m_xmf3Eye) + (XMLoadFloat3(&m_xmf3Up) * speed * fElapsedTime));
	}
	else if (arrow == 2) {	// �Ʒ�
		XMStoreFloat3(&m_xmf3Eye, XMLoadFloat3(&m_xmf3Eye) + (XMLoadFloat3(&m_xmf3Up) * -speed * fElapsedTime));
	}
	else if (3 == arrow) {	// right
		XMVECTOR right = XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&m_xmf3Up), XMLoadFloat3(&m_xmf3Dir)));
		XMStoreFloat3(&m_xmf3Eye, XMLoadFloat3(&m_xmf3Eye) + (right * speed * fElapsedTime));
	}
	else if (4 == arrow) {	// left
		XMVECTOR right = XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&m_xmf3Up), XMLoadFloat3(&m_xmf3Dir)));
		XMStoreFloat3(&m_xmf3Eye, XMLoadFloat3(&m_xmf3Eye) + (right * -speed * fElapsedTime));
	}
	else   // ��
		XMStoreFloat3(&m_xmf3Eye, XMLoadFloat3(&m_xmf3Eye) + (XMLoadFloat3(&m_xmf3Dir) * -speed * fElapsedTime));
}

XMFLOAT3& CCamera::getEyeCalculateOffset()
{
	XMStoreFloat3(&m_xmf3Eye, XMLoadFloat3(&m_pTarget->getPositionFromWMatrix()) + XMLoadFloat3(&m_xmf3hOffset) + XMLoadFloat3(&m_xmf3Offset) * m_fCameraLength);
	return m_xmf3Eye;
}

void CCamera::UpdateViewMatrix(float height)
{
	if (m_bThirdPerson) {
		//getEyeCalculateOffset();
		m_xmf3At = m_pTarget->getPositionFromWMatrix();
		XMStoreFloat3(&m_xmf3At, XMLoadFloat3(&m_pTarget->getPositionFromWMatrix()) + XMLoadFloat3(&m_xmf3hOffset));
		if (height != 0.0f) {
			m_xmf3Eye.y = height;
		}
	}
	else {
		XMStoreFloat3(&m_xmf3At, (XMLoadFloat3(&m_xmf3Eye) + XMLoadFloat3(&m_xmf3Dir)));
	}
	XMStoreFloat4x4(&m_xmf4x4View, XMMatrixLookAtLH(XMLoadFloat3(&m_xmf3Eye), XMLoadFloat3(&m_xmf3At), XMLoadFloat3(&m_xmf3Up)));

	XMMATRIX viewProj = XMLoadFloat4x4(&m_xmf4x4View) * XMLoadFloat4x4(&m_xmf4x4Proj);
	m_pCameraInfo->xmf3Eye = m_xmf3Eye;
	XMStoreFloat4x4(&m_pCameraInfo->xmf4x4ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&m_pCameraInfo->xmf4x4InverseViewProj, XMMatrixTranspose(XMMatrixInverse(nullptr, viewProj)));

	if (m_pTarget)
		XMStoreFloat4x4(&m_pCameraInfo->particleTarget, XMMatrixTranspose(XMLoadFloat4x4(&m_pTarget->getWorldMatrix())));
}

void CCamera::SetShaderVariable()
{
	g_DxResource.cmdList->SetComputeRootConstantBufferView(m_nRootParameterIndex, m_pd3dCameraBuffer->GetGPUVirtualAddress());
}

void CCamera::SetTarget(CGameObject* target)
{
	if (target) {
		m_pTarget = target;
		m_bThirdPerson = true;
	}
}

void CCamera::SetThirdPersonMode(bool bThirdPerson)
{
	m_bThirdPerson = bThirdPerson;
}

void CCamera::SetElapsedTimeAndShader(float fElapsedTime, UINT rootParameter)
{
	m_pCameraInfo->fElapsedTime = fElapsedTime;
	g_DxResource.cmdList->SetGraphicsRootConstantBufferView(rootParameter, m_pd3dCameraBuffer->GetGPUVirtualAddress());
}

void CCamera::SetMapNumber(int num)
{
	m_pCameraInfo->nMapNumber = num;
}

void CCamera::ChangeLength(short arrow)
{
	switch (arrow) {
	case 0:
		m_fCameraLength += 1.5f;
		if (m_fCameraLength >= 30.0f)
			m_fCameraLength = 30.0f;
		break;
	case 1:
		m_fCameraLength -= 1.5f;
		if (m_fCameraLength <= 6.0f)
			m_fCameraLength = 6.0f;
		break;
	}
}