#include "TextManager.h"

CTextLayer::CTextLayer(ComPtr<IDWriteTextFormat>& dwformat, ComPtr<ID2D1SolidColorBrush>& brush, const wchar_t* str, float x, float y)
	: m_format(dwformat), m_Brush(brush), m_str(str)
{
	m_layout = { x, y, DEFINED_UAV_BUFFER_WIDTH, DEFINED_UAV_BUFFER_HEIGHT };
}

// =============================================================

void CTextManager::InitManager(ComPtr<ID3D12Resource>& renderTarget)
{
	ID3D12Device* pDevice = g_DxResource.device;
	ID3D12CommandQueue* pCmdQueue = g_DxResource.cmdQueue;
	ComPtr<ID3D11Device> d3d11device{};
	ComPtr<ID2D1Factory3> d2d1factory{};
	ComPtr<IDXGIDevice> dxgiDevice{};

	D3D11On12CreateDevice(pDevice, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0,
		reinterpret_cast<IUnknown**>(&pCmdQueue), 1, 0, d3d11device.GetAddressOf(),
		m_pd3d11DeviceContext.GetAddressOf(), nullptr);

	d3d11device->QueryInterface(IID_PPV_ARGS(m_pd3d11on12Device.GetAddressOf()));

	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(d2d1factory.GetAddressOf()));
	m_pd3d11on12Device->QueryInterface(IID_PPV_ARGS(dxgiDevice.GetAddressOf()));

	d2d1factory->CreateDevice(dxgiDevice.Get(), m_pd2d1Device.GetAddressOf());

	m_pd2d1Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_pd2d1DeviceContext.GetAddressOf());

	m_pd2d1DeviceContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

	DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(m_pdwFactory.GetAddressOf()));

	D2D1_BITMAP_PROPERTIES1 d2d1bp = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

	D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
	m_pd3d11on12Device->CreateWrappedResource(renderTarget.Get(), &d3d11Flags, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT, IID_PPV_ARGS(m_WrapResource.GetAddressOf()));
	ComPtr<IDXGISurface> dxgiSurface{};
	m_WrapResource->QueryInterface(IID_PPV_ARGS(dxgiSurface.GetAddressOf()));
	m_pd2d1DeviceContext->CreateBitmapFromDxgiSurface(dxgiSurface.Get(), &d2d1bp, m_RenderTarget.GetAddressOf());
}

void CTextManager::AddText(const wchar_t* fontName, float fontsize, D2D1::ColorF color, const wchar_t* str, float x, float y)
{
	ComPtr<IDWriteTextFormat> tFormat{};
	m_pdwFactory->CreateTextFormat(fontName, nullptr, 
		DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
		fontsize, L"ko_kr", tFormat.GetAddressOf());

	tFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

	ComPtr<ID2D1SolidColorBrush> colorbrush{};
	m_pd2d1DeviceContext->CreateSolidColorBrush(color, colorbrush.GetAddressOf());
	
	m_TextList.emplace_back(std::make_shared<CTextLayer>(tFormat, colorbrush, str, x, y));
}

void CTextManager::Render()
{
	m_pd2d1DeviceContext->SetTarget(m_RenderTarget.Get());
	m_pd3d11on12Device->AcquireWrappedResources(m_WrapResource.GetAddressOf(), 1);

	m_pd2d1DeviceContext->BeginDraw();
	for (size_t i = 0; i < m_TextList.size(); i++)
	{
		m_pd2d1DeviceContext->DrawText(m_TextList[i]->m_str.c_str(), m_TextList[i]->m_str.length(), m_TextList[i]->m_format.Get(), m_TextList[i]->m_layout, m_TextList[i]->m_Brush.Get());
	}
	m_pd2d1DeviceContext->EndDraw();

	m_pd3d11on12Device->ReleaseWrappedResources(m_WrapResource.GetAddressOf(), 1);
	m_pd3d11DeviceContext->Flush();
}