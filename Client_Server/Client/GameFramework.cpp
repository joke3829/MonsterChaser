// ==============================================
// GameFramework.cpp
// 
// ==============================================

#include "GameFramework.h"

constexpr unsigned short NUM_G_ROOTPARAMETER = 6;

std::unique_ptr<CMonsterChaserSoundManager> g_pSoundManager{};

CGameFramework::~CGameFramework()
{
	::CloseHandle(m_hFenceHandle);
}

bool CGameFramework::OnInit(HWND hWnd, HINSTANCE hInstance)
{
	m_hWnd = hWnd; m_hInstance = hInstance;

	_tcscpy_s(m_pszFrameRate, _T("Client ("));

	// device, fence Create
	InitDevice();

	// RayTracing Support Check
	CheckRayTracingSupport();

	// Command Object Ready
	InitCommand();

	// SwapChain Ready
	InitSwapChain();

	// RTV, DSV
	//InitRTVDSV();
	g_DxResource.device = m_pd3dDevice.Get();
	g_DxResource.cmdAlloc = m_pd3dCommandAllocator.Get();
	g_DxResource.cmdList = m_pd3dCommandList.Get();
	g_DxResource.cmdQueue = m_pd3dCommandQueue.Get();
	g_DxResource.fence = m_pd3dFence.Get();
	g_DxResource.pFenceHandle = &m_hFenceHandle;

	// UAV Buffer
	if (m_bRayTracingSupport) {
		InitOutputBuffer();
		SetUpRayTracingPipeline();
	}

	// Global Variable & Scene Ready


	// Ready NullResource
	auto desc = BASIC_BUFFER_DESC;
	desc.Width = sizeof(XMFLOAT3);
	m_pd3dDevice->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(g_DxResource.nullBuffer.GetAddressOf()));

	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = desc.Height = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	m_pd3dDevice->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(g_DxResource.nullTexture.GetAddressOf()));

	m_pCamera = std::make_shared<CCamera>();
	m_pCamera->Setup(2);

	g_pSoundManager = std::make_unique<CMonsterChaserSoundManager>(64);

	InitScene();

	return true;
}

void CGameFramework::InitDevice()
{
	// Create Factory And Device
	::CreateDXGIFactory(IID_PPV_ARGS(&m_pdxgiFactory));
	ComPtr<IDXGIAdapter> adapter{};

	::D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_pd3dDevice));
	if (!m_pd3dDevice) {
		OutputDebugString(L"Device not Created\n");
		exit(0);
	}

	// Create Fence
	m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pd3dFence));
	m_hFenceHandle = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}

void CGameFramework::CheckRayTracingSupport()
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5{};
	m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
	if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0) {
		OutputDebugString(L"This Device Not Support RayTracing!\n");
		m_bRayTracingSupport = false;
		m_bRaster = true;
	}
	else {
		m_bRayTracingSupport = true;
		m_bRaster = false;
	}
}

void CGameFramework::CreateRootSignature()
{
	{
		// Global Root Signature
		D3D12_DESCRIPTOR_RANGE rootRange{};								// u0
		rootRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		rootRange.NumDescriptors = 1;
		rootRange.BaseShaderRegister = 0;
		rootRange.RegisterSpace = 0;

		D3D12_DESCRIPTOR_RANGE cubeMapRange{};							// t3
		cubeMapRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		cubeMapRange.NumDescriptors = 1;
		cubeMapRange.BaseShaderRegister = 3;
		cubeMapRange.RegisterSpace = 0;

		D3D12_DESCRIPTOR_RANGE terrainTRange[2]{};						// b0, space2
		terrainTRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		terrainTRange[0].NumDescriptors = 1;
		terrainTRange[0].BaseShaderRegister = 2;
		terrainTRange[0].RegisterSpace = 0;

		terrainTRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		terrainTRange[1].NumDescriptors = 13;
		terrainTRange[1].BaseShaderRegister = 4;
		terrainTRange[1].RegisterSpace = 0;
		terrainTRange[1].OffsetInDescriptorsFromTableStart = 1;

		// 0. uavBuffer, 1. AS, 2. camera, 3. Lights, 4. Enviorment(cubeMap), 5. TerrainInfo
		D3D12_ROOT_PARAMETER params[NUM_G_ROOTPARAMETER] = {};
		params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;	// u0
		params[0].DescriptorTable.NumDescriptorRanges = 1;
		params[0].DescriptorTable.pDescriptorRanges = &rootRange;

		params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;				// t0
		params[1].Descriptor.RegisterSpace = 0;
		params[1].Descriptor.ShaderRegister = 0;

		params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;				// b0, space0
		params[2].Descriptor.RegisterSpace = 0;
		params[2].Descriptor.ShaderRegister = 0;

		params[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;				// b0, space1
		params[3].Descriptor.RegisterSpace = 1;
		params[3].Descriptor.ShaderRegister = 0;

		params[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[4].DescriptorTable.NumDescriptorRanges = 1;
		params[4].DescriptorTable.pDescriptorRanges = &cubeMapRange;

		params[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[5].DescriptorTable.NumDescriptorRanges = 2;
		params[5].DescriptorTable.pDescriptorRanges = terrainTRange;

		D3D12_STATIC_SAMPLER_DESC samplerDesc{};								// s0
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		samplerDesc.RegisterSpace = 0;
		samplerDesc.ShaderRegister = 0;
		samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_SIGNATURE_DESC rtDesc{};
		rtDesc.NumParameters = NUM_G_ROOTPARAMETER;
		rtDesc.NumStaticSamplers = 1;
		rtDesc.pParameters = params;
		rtDesc.pStaticSamplers = &samplerDesc;

		ID3DBlob* pBlob{};
		D3D12SerializeRootSignature(&rtDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &pBlob, nullptr);
		m_pd3dDevice->CreateRootSignature(0, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), IID_PPV_ARGS(m_pGlobalRootSignature.GetAddressOf()));
		pBlob->Release();
	}
	{
		// LocalRootSignature
		D3D12_DESCRIPTOR_RANGE srvRange[7] = {};
		srvRange[0].BaseShaderRegister = 2;		// t2, space0
		srvRange[0].NumDescriptors = 1;
		srvRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange[0].RegisterSpace = 0;

		srvRange[1].BaseShaderRegister = 2;		// t2, space1
		srvRange[1].NumDescriptors = 1;
		srvRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange[1].RegisterSpace = 1;

		srvRange[2].BaseShaderRegister = 2;		// t2, space2
		srvRange[2].NumDescriptors = 1;
		srvRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange[2].RegisterSpace = 2;

		srvRange[3].BaseShaderRegister = 2;		// t2, space3
		srvRange[3].NumDescriptors = 1;
		srvRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange[3].RegisterSpace = 3;

		srvRange[4].BaseShaderRegister = 2;		// t2, space4
		srvRange[4].NumDescriptors = 1;
		srvRange[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange[4].RegisterSpace = 4;

		srvRange[5].BaseShaderRegister = 2;		// t2, space5
		srvRange[5].NumDescriptors = 1;
		srvRange[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange[5].RegisterSpace = 5;

		srvRange[6].BaseShaderRegister = 2;		// t2, space6
		srvRange[6].NumDescriptors = 1;
		srvRange[6].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange[6].RegisterSpace = 6;

		D3D12_ROOT_PARAMETER params[17] = {};
		params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// b1, space0
		params[0].Descriptor.RegisterSpace = 0;
		params[0].Descriptor.ShaderRegister = 1;

		params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// b1, spcae1
		params[1].Descriptor.RegisterSpace = 1;
		params[1].Descriptor.ShaderRegister = 1;

		params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;	// t1, space0
		params[2].Descriptor.RegisterSpace = 0;
		params[2].Descriptor.ShaderRegister = 1;

		params[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;	// t1, space1
		params[3].Descriptor.RegisterSpace = 1;
		params[3].Descriptor.ShaderRegister = 1;

		params[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;	// t1, space2
		params[4].Descriptor.RegisterSpace = 2;
		params[4].Descriptor.ShaderRegister = 1;

		params[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;	// t1, space3
		params[5].Descriptor.RegisterSpace = 3;
		params[5].Descriptor.ShaderRegister = 1;

		params[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;	// t1, space4
		params[6].Descriptor.RegisterSpace = 4;
		params[6].Descriptor.ShaderRegister = 1;

		params[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;	// t1, space5
		params[7].Descriptor.RegisterSpace = 5;
		params[7].Descriptor.ShaderRegister = 1;

		params[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;	// t1, space6
		params[8].Descriptor.RegisterSpace = 6;
		params[8].Descriptor.ShaderRegister = 1;

		params[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;	// t1, space7
		params[9].Descriptor.RegisterSpace = 7;
		params[9].Descriptor.ShaderRegister = 1;

		// texture
		params[10].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[10].DescriptorTable.NumDescriptorRanges = 1;
		params[10].DescriptorTable.pDescriptorRanges = &srvRange[0];

		params[11].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[11].DescriptorTable.NumDescriptorRanges = 1;
		params[11].DescriptorTable.pDescriptorRanges = &srvRange[1];

		params[12].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[12].DescriptorTable.NumDescriptorRanges = 1;
		params[12].DescriptorTable.pDescriptorRanges = &srvRange[2];

		params[13].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[13].DescriptorTable.NumDescriptorRanges = 1;
		params[13].DescriptorTable.pDescriptorRanges = &srvRange[3];

		params[14].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[14].DescriptorTable.NumDescriptorRanges = 1;
		params[14].DescriptorTable.pDescriptorRanges = &srvRange[4];

		params[15].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[15].DescriptorTable.NumDescriptorRanges = 1;
		params[15].DescriptorTable.pDescriptorRanges = &srvRange[5];

		params[16].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[16].DescriptorTable.NumDescriptorRanges = 1;
		params[16].DescriptorTable.pDescriptorRanges = &srvRange[6];

		D3D12_ROOT_SIGNATURE_DESC rtDesc{};
		rtDesc.NumParameters = 17;
		rtDesc.NumStaticSamplers = 0;
		rtDesc.pParameters = params;
		rtDesc.pStaticSamplers = nullptr;
		rtDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		ID3DBlob* pBlob{};
		D3D12SerializeRootSignature(&rtDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &pBlob, nullptr);
		m_pd3dDevice->CreateRootSignature(0, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), IID_PPV_ARGS(m_pLocalRootSignature.GetAddressOf()));
		pBlob->Release();
	}
}

void CGameFramework::SetUpRayTracingPipeline()
{
	CreateRootSignature();

	m_pRaytracingPipeline = std::make_unique<CRayTracingPipeline>();
	m_pRaytracingPipeline->Setup(1 + 2 + 1 + 2 + 1 + 1);
	m_pRaytracingPipeline->AddLibrarySubObject(compiledShader, std::size(compiledShader));
	m_pRaytracingPipeline->AddHitGroupSubObject(L"HitGroup", L"RadianceClosestHit", L"RadianceAnyHit");
	m_pRaytracingPipeline->AddHitGroupSubObject(L"ShadowHit", L"ShadowClosestHit", L"ShadowAnyHit");
	m_pRaytracingPipeline->AddShaderConfigSubObject(8, 20);
	m_pRaytracingPipeline->AddLocalRootAndAsoociationSubObject(m_pLocalRootSignature.Get());
	m_pRaytracingPipeline->AddGlobalRootSignatureSubObject(m_pGlobalRootSignature.Get());
	m_pRaytracingPipeline->AddPipelineConfigSubObject(6);
	m_pRaytracingPipeline->MakePipelineState();
}

void CGameFramework::InitCommand()
{
	D3D12_COMMAND_QUEUE_DESC desc{};
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	m_pd3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_pd3dCommandQueue));
	m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pd3dCommandAllocator));
	m_pd3dDevice->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&m_pd3dCommandList));
}

void CGameFramework::InitSwapChain()
{
	DXGI_SWAP_CHAIN_DESC1 desc{};
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	desc.Width = DEFINED_UAV_BUFFER_WIDTH;	// subject to change
	desc.Height = DEFINED_UAV_BUFFER_HEIGHT;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc = NO_AA;
	desc.BufferCount = 2;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	IDXGISwapChain1* swapchain1{};
	m_pdxgiFactory->CreateSwapChainForHwnd(m_pd3dCommandQueue.Get(), m_hWnd, &desc, nullptr, nullptr, &swapchain1);
	swapchain1->QueryInterface(m_pdxgiSwapChain.GetAddressOf());
	swapchain1->Release();

	if (!m_pdxgiSwapChain) {
		OutputDebugString(L"Not SwapChain Created\n");
		exit(0);
	}

	m_pdxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);
}

//void CGameFramework::InitRTVDSV()
//{
//	// RTV
//	D3D12_DESCRIPTOR_HEAP_DESC heapdesc{};
//	heapdesc.NumDescriptors = 2;
//	heapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
//	m_pd3dDevice->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(&m_pd3dRenderTargetView));
//
//	m_nRTVIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
//	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = m_pd3dRenderTargetView->GetCPUDescriptorHandleForHeapStart();
//
//	for (int i = 0; i < 2; ++i) {
//		m_pdxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pd3dBackBuffer[i]));
//		m_pd3dDevice->CreateRenderTargetView(m_pd3dBackBuffer[i].Get(), nullptr, cpuDescriptorHandle);
//		cpuDescriptorHandle.ptr += m_nRTVIncrementSize;
//	}
//
//	heapdesc.NumDescriptors = 1;
//	heapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
//	m_pd3dDevice->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(&m_pd3dDepthStencilView));
//
//	D3D12_RESOURCE_DESC resourceDesc = BASIC_BUFFER_DESC;
//	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
//	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
//	resourceDesc.Width = DEFINED_UAV_BUFFER_WIDTH;		// Subject to change
//	resourceDesc.Height = DEFINED_UAV_BUFFER_HEIGHT;
//	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
//	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
//
//	m_pd3dDevice->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, nullptr, IID_PPV_ARGS(&m_pd3dDepthStencilBuffer));
//
//	cpuDescriptorHandle = m_pd3dDepthStencilView->GetCPUDescriptorHandleForHeapStart();
//	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer.Get(), nullptr, cpuDescriptorHandle);
//}

void CGameFramework::InitOutputBuffer()
{
	D3D12_DESCRIPTOR_HEAP_DESC heapdesc{};
	heapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapdesc.NumDescriptors = 1;
	heapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	m_pd3dDevice->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(&m_pd3dOutputBufferView));

	D3D12_RESOURCE_DESC desc = BASIC_BUFFER_DESC;
	desc.Width = DEFINED_UAV_BUFFER_WIDTH;
	desc.Height = DEFINED_UAV_BUFFER_HEIGHT;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	m_pd3dDevice->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&m_pd3dOutputBuffer));

	//m_pd3dDevice->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(m_pTextRenderTarget.GetAddressOf()));

	D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc{};
	viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	m_pd3dDevice->CreateUnorderedAccessView(m_pd3dOutputBuffer.Get(), nullptr, &viewDesc, m_pd3dOutputBufferView->GetCPUDescriptorHandleForHeapStart());

}

void CGameFramework::ChangeScreenStateWindow()
{
	BOOL fullScreenState{};
	m_pdxgiSwapChain->GetFullscreenState(&fullScreenState, nullptr);
	if (fullScreenState)
		ChangeFullScreenState();
}

void CGameFramework::ChangeFullScreenState()
{
	Flush();
	BOOL bFullScreenState{};
	m_pdxgiSwapChain->GetFullscreenState(&bFullScreenState, nullptr);
	m_pdxgiSwapChain->SetFullscreenState(!bFullScreenState, nullptr);

	DXGI_MODE_DESC mdesc{};
	mdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	mdesc.Width = DEFINED_UAV_BUFFER_WIDTH;
	mdesc.Height = DEFINED_UAV_BUFFER_HEIGHT;
	mdesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	mdesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	m_pdxgiSwapChain->ResizeTarget(&mdesc);

	m_pdxgiSwapChain->ResizeBuffers(0, DEFINED_UAV_BUFFER_WIDTH, DEFINED_UAV_BUFFER_HEIGHT, DXGI_FORMAT_UNKNOWN, 0);
}

void CGameFramework::InitScene()
{
	m_pScene = std::make_unique<TitleScene>();
	m_pScene->SetCamera(m_pCamera);
	m_pScene->SetUp(m_pd3dOutputBuffer, m_pRaytracingPipeline);
	g_InGameState = IS_LOADING;
	//bIngame = true;
}

LRESULT CALLBACK CGameFramework::WMMessageProcessing(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	switch (nMessage) {
	case WM_KEYDOWN:
	case WM_KEYUP:
		KeyboardProcessing(hWnd, nMessage, wParam, lParam);
		break;
	case WM_MOUSEMOVE:
	case WM_MOUSEWHEEL:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		MouseProcessing(hWnd, nMessage, wParam, lParam);
		break;
	}
	return 0;
}

void CGameFramework::KeyboardProcessing(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	switch (nMessage) {
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_ESCAPE: {
			ChangeScreenStateWindow();
			PostQuitMessage(0);
			break;
		}
		case VK_F9:
			ChangeFullScreenState();
			break;
		default:
			m_pScene->OnProcessingKeyboardMessage(hWnd, nMessage, wParam, lParam);
			break;
		}
		break;
	case WM_KEYUP:
		break;
	}
}

void CGameFramework::MouseProcessing(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	switch (nMessage) {
	case WM_LBUTTONDOWN:;
	case WM_LBUTTONUP:
	case WM_MOUSEMOVE:
	case WM_MOUSEWHEEL:
		m_pScene->OnProcessingMouseMessage(hWnd, nMessage, wParam, lParam);
		break;
	}
}

void CGameFramework::CheckSceneFinish()
{
	short state = m_pScene->getNextSceneNumber();
	if (-1 != state)
		ChangeScene(state);
}

void CGameFramework::ChangeScene(short definedScene)
{
	m_pScene.reset();
	switch (definedScene) {
	case SCENE_TITLE:
		bIngame = false;
		m_pScene = std::make_unique<TitleScene>();
		m_pScene->SetCamera(m_pCamera);
		m_pScene->SetUp(m_pd3dOutputBuffer, m_pRaytracingPipeline);
		g_InGameState = IS_LOADING;
		break;
	case SCENE_PLAIN:
		bIngame = true;
		m_pScene = std::make_unique<CRaytracingETPScene>();
		m_pScene->SetCamera(m_pCamera);
		m_pScene->SetUp(m_pd3dOutputBuffer, m_pRaytracingPipeline);
		g_InGameState = IS_LOADING;
		break;
	case SCENE_WINTERLAND:
		bIngame = true;
		m_pScene = std::make_unique<CRaytracingWinterLandScene>();
		m_pScene->SetCamera(m_pCamera);
		m_pScene->SetUp(m_pd3dOutputBuffer, m_pRaytracingPipeline);
		g_InGameState = IS_LOADING;
		break;
	case SCENE_CAVE:
		bIngame = true;
		m_pScene = std::make_unique<CRaytracingCaveScene>();
		m_pScene->SetCamera(m_pCamera);
		m_pScene->SetUp(m_pd3dOutputBuffer, m_pRaytracingPipeline);
		g_InGameState = IS_LOADING;
		break;
	}

	m_Timer.Reset();
}

void CGameFramework::ProcessInput(float fElapsedTime)
{
	m_pScene->ProcessInput(fElapsedTime);
}

void CGameFramework::Render()
{
	CheckSceneFinish();

	auto barrier = [&](ID3D12Resource* pResource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
		{
			D3D12_RESOURCE_BARRIER resBarrier{};
			resBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			resBarrier.Transition.pResource = pResource;
			resBarrier.Transition.StateBefore = before;
			resBarrier.Transition.StateAfter = after;
			resBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			m_pd3dCommandList->ResourceBarrier(1, &resBarrier);
		};
	//m_Timer.Tick(60.0f);
	m_Timer.Tick();
	m_pd3dCommandAllocator->Reset();
	m_pd3dCommandList->Reset(m_pd3dCommandAllocator.Get(), nullptr);

	ProcessInput(m_Timer.GetTimeElapsed());
	// Render Here(Set & Draw) ===================

	m_pScene->UpdateObject(m_Timer.GetTimeElapsed());

	if (bIngame) {
		m_pd3dCommandList->SetPipelineState1(m_pRaytracingPipeline->getPipelineState());
		m_pd3dCommandList->SetComputeRootSignature(m_pGlobalRootSignature.Get());

		m_pd3dCommandList->SetDescriptorHeaps(1, m_pd3dOutputBufferView.GetAddressOf());
		m_pd3dCommandList->SetComputeRootDescriptorTable(0, m_pd3dOutputBufferView->GetGPUDescriptorHandleForHeapStart());
	}
	m_pScene->Render();
	// ===========================================


	ID3D12Resource* backBuffer{};
	m_pdxgiSwapChain->GetBuffer(m_pdxgiSwapChain->GetCurrentBackBufferIndex(), IID_PPV_ARGS(&backBuffer));

	barrier(m_pd3dOutputBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	barrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);

	m_pd3dCommandList->CopyResource(backBuffer, m_pd3dOutputBuffer.Get());

	barrier(m_pd3dOutputBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	barrier(backBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

	backBuffer->Release();

	m_pd3dCommandList->Close();
	m_pd3dCommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(m_pd3dCommandList.GetAddressOf()));

	HRESULT hResult = m_pd3dDevice->GetDeviceRemovedReason();
	if (hResult != S_OK) {
		hResult = m_pd3dDevice->GetDeviceRemovedReason();
	}

	Flush();

	m_pdxgiSwapChain->Present(0, 0);

	m_pScene->PostProcess();

	m_Timer.GetFrameRate(m_pszFrameRate + 8, 37);
	SetWindowText(m_hWnd, m_pszFrameRate);
}