#pragma once
#include "stdafx.h"

extern DXResources g_DxResource;

class CTextLayer {
public:
	CTextLayer(ComPtr<IDWriteTextFormat>& dwformat, ComPtr<ID2D1SolidColorBrush>& brush, const wchar_t* str, float x, float y);

	ComPtr<IDWriteTextFormat>		m_format;
	ComPtr<ID2D1SolidColorBrush>	m_Brush;
	std::wstring					m_str;
	D2D1_RECT_F						m_layout;
	bool							m_bRender = true;
};

class CTextManager {
public:
	void InitManager(ComPtr<ID3D12Resource>& renderTarget);

	//void AddFont(const wchar_t* filePath);
	void AddText(const wchar_t* fontName, float fontsize, D2D1::ColorF color, const wchar_t* str, float x, float y);

	void Render();
protected:
	ComPtr<ID2D1Device2> m_pd2d1Device;
	ComPtr<ID2D1DeviceContext2> m_pd2d1DeviceContext{};
	ComPtr<ID3D11On12Device>	m_pd3d11on12Device{};
	ComPtr<ID3D11DeviceContext>	m_pd3d11DeviceContext{};

	ComPtr<ID3D11Resource>	m_WrapResource{};
	ComPtr<ID2D1Bitmap1>	m_RenderTarget{};

	ComPtr<IDWriteFactory>	m_pdwFactory{};
	std::vector<std::shared_ptr<CTextLayer>>	m_TextList{};

	std::vector<std::wstring>	m_UserFont{};
};

