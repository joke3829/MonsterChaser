#pragma once
#include "stdafx.h"
#include "ResourceManager.h"

struct UIConstant {
	XMFLOAT4X4 worldmatrix;
	XMFLOAT4X4 uvTransform;
	int bHasTexture;
};

class UIObject {
public:
	UIObject(unsigned int crIndex, unsigned int trIndex, Mesh* mesh, CTexture* texture = nullptr);
	~UIObject();

	void setRenderState(bool b) { m_bRenderState = b; }
	void setPositionInViewport(int x, int y);
	void setPositionInWorld(float x, float y);
	void setScale(XMFLOAT3& scale);
	void setScale(float scale);
	void setScaleX(float xScale);
	void setScaleXWithUV(float xScale);
	void setScaleY(float yScale);

	void setColor(float r, float g, float b, float a);

	void Animation(float fElapsedTime);

	void Render();
protected:
	bool						m_bRenderState = true;

	std::vector<XMFLOAT4>		m_vColor{};
	ComPtr<ID3D12Resource>		m_ColorBuffer{};
	void* m_pColorMappedptr{};

	ComPtr<ID3D12Resource>		m_CB{};
	XMFLOAT4X4					m_WorldMatrix{};
	XMFLOAT4X4					m_uvTransform{};
	UIConstant* m_pMap{};
	unsigned int				m_nCBRootParameterIndex{};
	unsigned int				m_nTextureRootParameterIndex{};

	Mesh* m_pMesh{};
	CTexture* m_pTexture{};
};

