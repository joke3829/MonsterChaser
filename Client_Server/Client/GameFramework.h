// ===================================================
// GameFramework.h
// 설명 추가
// ===================================================
#pragma once
#include "stdafx.h"
#include "Scene.h"
#include "Timer.h"
#include "Camera.h"
#include "SoundManager.h"

extern DXResources g_DxResource;
extern InGameState g_InGameState;

class CGameFramework {
public:
	~CGameFramework();

	bool OnInit(HWND hWnd, HINSTANCE hInstance);

	void InitDevice();
	void InitCommand();
	void InitSwapChain();

	// Rasterization
	//void InitRTVDSV();

	// DXR
	void InitOutputBuffer();

	// device의 RayTracing 지원 확인
	void CheckRayTracingSupport();
	void CreateRootSignature();
	void SetUpRayTracingPipeline();

	void InitScene();

	void ChangeScreenStateWindow();
	void ChangeFullScreenState();

	LRESULT CALLBACK WMMessageProcessing(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam);
	void KeyboardProcessing(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam);
	void MouseProcessing(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam);

	void CheckSceneFinish();
	void ChangeScene(short definedScene);

	void ProcessInput(float fElapsedTime);

	void Render();
private:
	HWND m_hWnd{};
	HINSTANCE m_hInstance{};

	ComPtr<IDXGIFactory4> m_pdxgiFactory{};
	ComPtr<IDXGISwapChain3> m_pdxgiSwapChain{};
	ComPtr<ID3D12Device5> m_pd3dDevice{};

	ComPtr<ID3D12CommandAllocator> m_pd3dCommandAllocator{};
	ComPtr<ID3D12CommandQueue> m_pd3dCommandQueue{};
	ComPtr<ID3D12GraphicsCommandList4> m_pd3dCommandList{};

	ComPtr<ID3D12Fence> m_pd3dFence{};
	HANDLE m_hFenceHandle{};

	bool m_bRayTracingSupport{};	// RayTracing 지원 여부
	bool m_bRaster{};				// Rendering 방식 결정 값

	// Rasterization
	/*ComPtr<ID3D12Resource> m_pd3dBackBuffer[2]{};
	ComPtr<ID3D12DescriptorHeap> m_pd3dRenderTargetView{};
	UINT m_nRTVIncrementSize{};

	ComPtr<ID3D12Resource> m_pd3dDepthStencilBuffer{};
	ComPtr<ID3D12DescriptorHeap> m_pd3dDepthStencilView{};*/

	// DXR
	ComPtr<ID3D12Resource> m_pd3dOutputBuffer{};
	ComPtr<ID3D12DescriptorHeap> m_pd3dOutputBufferView{};

	//ComPtr<ID3D12Resource> m_pTextRenderTarget{};

	_TCHAR m_pszFrameRate[50];
	CGameTimer m_Timer;

	std::unique_ptr<CScene> m_pScene{};

	std::shared_ptr<CCamera> m_pCamera{};
	//std::shared_ptr<CTextManager> m_pTextManager{};

	ComPtr<ID3D12RootSignature> m_pGlobalRootSignature;
	ComPtr<ID3D12RootSignature> m_pLocalRootSignature;
	std::shared_ptr<CRayTracingPipeline> m_pRaytracingPipeline;

	bool bIngame = false;
};

