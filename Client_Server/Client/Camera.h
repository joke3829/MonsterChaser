#pragma once

#include "stdafx.h"
#include "GameObject.h"

extern DXResources g_DxResource;

struct CB_CAMERA_INFO {
	XMFLOAT4X4 xmf4x4ViewProj;
	XMFLOAT4X4 xmf4x4InverseViewProj;
	XMFLOAT3 xmf3Eye;
	float fElapsedTime;
	XMFLOAT4X4 particleTarget;			// test
	int bNormalMapping;	// front 2byte normal, back 2byte albedo
	int bReflection;
	int nMapNumber;
};

class CCamera {
public:
	~CCamera();

	void Setup(int nRootParameterIndex);

	void Rotate(int cxDelta, int cyDelta);
	void Move(int arrow, float fElapsedTime, bool shift = false);

	void UpdateViewMatrix(float height = 0.0f);
	void SetShaderVariable();

	XMFLOAT3& getEye() { return m_xmf3Eye; }
	XMFLOAT3& getEyeCalculateOffset();
	bool getThirdPersonState() const { return m_bThirdPerson; }

	void SetTarget(CGameObject* target);
	void SetThirdPersonMode(bool bThirdPerson);
	void SetCameraLength(float fLength) { m_fCameraLength = fLength; }
	void SetHOffset(float height) { m_xmf3hOffset.y = height; }
	void SetElapsedTimeAndShader(float fElapsedTime, UINT rootParameter);

	void SetMapNumber(int num);

	void toggleNormalMapping()
	{
		unsigned int fBytes = m_pCameraInfo->bNormalMapping & 0xFFFF0000;
		unsigned int bBytes = m_pCameraInfo->bNormalMapping & 0x0000FFFF;
		fBytes = ~fBytes & 0XFFFF0000;
		m_pCameraInfo->bNormalMapping = fBytes | bBytes;
		//m_pCameraInfo->bNormalMapping = ~m_pCameraInfo->bNormalMapping; 
	}
	void toggleAlbedoColor()
	{
		unsigned int fByte = m_pCameraInfo->bNormalMapping & 0xFFFF0000;
		unsigned int bByte = ~(m_pCameraInfo->bNormalMapping & 0x0000FFFF) & 0x0000FFFF;

		m_pCameraInfo->bNormalMapping = fByte | bByte;
		//m_pCameraInfo->bNormalMapping |= 0x0000; 
	}
	void toggleReflection() { m_pCameraInfo->bReflection = ~m_pCameraInfo->bReflection; }
	XMFLOAT3 getDir() const { return m_xmf3Dir; }
	XMFLOAT3 getUp() const { return m_xmf3Up; }

	void ChangeLength(short arrow);		// 0: closer, 1: far
protected:
	bool m_bThirdPerson = false;
	CGameObject* m_pTarget = nullptr;


	ComPtr<ID3D12Resource> m_pd3dCameraBuffer{};
	CB_CAMERA_INFO* m_pCameraInfo{};

	XMFLOAT3 m_xmf3Eye{ 0.0f, 0.0f, 0.0f };
	XMFLOAT3 m_xmf3At;
	XMFLOAT3 m_xmf3Up{ 0.0f, 1.0f, 0.0f };

	XMFLOAT3 m_xmf3Dir{ 0.0f, 0.0f, 1.0f };
	XMFLOAT3 m_xmf3Offset{ 0.0f, 0.0f, -1.0f };
	XMFLOAT3 m_xmf3hOffset{ 0.0f, 0.0f, 0.0f };

	XMFLOAT4X4 m_xmf4x4View;
	XMFLOAT4X4 m_xmf4x4Proj;

	float m_fFOV = 60.0f;
	float m_fAspect = 960.0f / 540.0f;
	float m_fNear = 0.1f;
	float m_fFar = 1000.0f;

	float m_fLimitcy{};
	float m_fCameraLength{ 30.0f };		// ī�޶� �Ÿ�

	int m_nRootParameterIndex{};
};

