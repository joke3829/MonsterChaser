#include "Scene.h"
#include "C_Socket.h"
#include "protocol.h"
extern C_Socket Client;
extern std::unordered_map<int, Player> Players;
extern std::unordered_map<int, std::unique_ptr<Monster>> Monsters;
extern std::array<short, 10>	 userPerRoom;
extern TitleState g_state;
extern InGameState g_InGameState;
constexpr unsigned short NUM_G_ROOTPARAMETER = 6;

std::vector<std::unique_ptr<CPlayableCharacter>>	m_vMonsters{};
std::vector<std::unique_ptr<CPlayableCharacter>>	m_vPlayers{};

CParticle* g_pBuff0{};
CParticle* g_pBuff1{};
CParticle* g_pBuff2{};

void CScene::CreateRTVDSV()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.NumDescriptors = 1;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	ID3D12Device5* device = g_DxResource.device;
	device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_RTV.GetAddressOf()));

	device->CreateRenderTargetView(m_pOutputBuffer.Get(), nullptr, m_RTV->GetCPUDescriptorHandleForHeapStart());

	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_DSV.GetAddressOf()));

	D3D12_RESOURCE_DESC resourceDesc = BASIC_BUFFER_DESC;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.Width = DEFINED_UAV_BUFFER_WIDTH;		// Subject to change
	resourceDesc.Height = DEFINED_UAV_BUFFER_HEIGHT;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, nullptr, IID_PPV_ARGS(m_pDepthStencilBuffer.GetAddressOf()));
	device->CreateDepthStencilView(m_pDepthStencilBuffer.Get(), nullptr, m_DSV->GetCPUDescriptorHandleForHeapStart());
}

void CScene::CreateOrthoMatrixBuffer()
{
	auto desc = BASIC_BUFFER_DESC;
	desc.Width = Align(sizeof(XMFLOAT4X4), 256);
	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_cameraCB.GetAddressOf()));

	XMFLOAT4X4 ortho{};
	XMStoreFloat4x4(&ortho, XMMatrixTranspose(XMMatrixOrthographicLH(DEFINED_UAV_BUFFER_WIDTH, DEFINED_UAV_BUFFER_HEIGHT, -1, 1)));

	void* temp{};
	m_cameraCB->Map(0, nullptr, &temp);
	memcpy(temp, &ortho, sizeof(XMFLOAT4X4));
	m_cameraCB->Unmap(0, nullptr);
}
// ==================================================================================

void TitleScene::SetUp(ComPtr<ID3D12Resource>& outputBuffer, std::shared_ptr<CRayTracingPipeline> pipeline)
{
	m_pOutputBuffer = outputBuffer;

	CreateRTVDSV();
	CreateRootSignature();
	CreatePipelineState();

	CreateOrthoMatrixBuffer();

	m_pResourceManager = std::make_unique<CResourceManager>();

	std::vector<std::unique_ptr<CTexture>>& textures = m_pResourceManager->getTextureList();
	std::vector<std::unique_ptr<Mesh>>& meshes = m_pResourceManager->getMeshList();

	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0, 0.0, 0.0), DEFINED_UAV_BUFFER_WIDTH, DEFINED_UAV_BUFFER_HEIGHT));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\Title\\title1.dds"));
	m_vTitleUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get(), textures[textures.size() - 1].get()));
	m_vTitleUIs[m_vTitleUIs.size() - 1]->setPositionInViewport(0, 0);

	m_vTitleUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get()));
	m_vTitleUIs[m_vTitleUIs.size() - 1]->setPositionInViewport(0, 0);
	m_vTitleUIs[m_vTitleUIs.size() - 1]->setColor(0.0, 0.0, 0.0, 1.0);

	// Room Select =======================================================================================

	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0, 0.0, 0.0), DEFINED_UAV_BUFFER_WIDTH, DEFINED_UAV_BUFFER_HEIGHT));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\RoomSelect\\background1.dds"));
	m_vRoomSelectUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get(), textures[textures.size() - 1].get()));
	m_vRoomSelectUIs[m_vRoomSelectUIs.size() - 1]->setPositionInViewport(0, 0);

	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0, 0.0, 0.0), 440, 84));
	//textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\RoomSelect\\people.dds"));
	for (int i = 0; i < 10; ++i) {
		int j = i % 2;
		if (j == 0) {
			m_vRoomSelectUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get()));
			m_vRoomSelectUIs[m_vRoomSelectUIs.size() - 1]->setPositionInViewport(20 + 160, i / 2 * 100 + 20 + 180);
			m_vRoomSelectUIs[m_vRoomSelectUIs.size() - 1]->setColor(1.0, 1.0, 1.0, 0.5);
		}
		else {
			m_vRoomSelectUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get()));
			m_vRoomSelectUIs[m_vRoomSelectUIs.size() - 1]->setPositionInViewport(500 + 160, i / 2 * 100 + 20 + 180);
			m_vRoomSelectUIs[m_vRoomSelectUIs.size() - 1]->setColor(1.0, 1.0, 1.0, 0.5);
		}
	}
	peopleindex = m_vRoomSelectUIs.size();

	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0, 0.0, 0.0), 30, 84));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\RoomSelect\\people.dds"));
	for (int i = 0; i < 10; ++i) {
		for (int j = 0; j < 3; ++j) {
			int k = i % 2;
			if (k == 0) {
				m_vRoomSelectUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get(), textures[textures.size() - 1].get()));
				m_vRoomSelectUIs[m_vRoomSelectUIs.size() - 1]->setPositionInViewport(350 + (j * 40) + 160, i / 2 * 100 + 20 + 180);
			}
			else {
				m_vRoomSelectUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get(), textures[textures.size() - 1].get()));
				m_vRoomSelectUIs[m_vRoomSelectUIs.size() - 1]->setPositionInViewport(830 + (j * 40) + 160, i / 2 * 100 + 20 + 180);
			}
		}
	}

	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0, 0.0, 0.0), DEFINED_UAV_BUFFER_WIDTH, DEFINED_UAV_BUFFER_HEIGHT));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\RoomSelect\\SelectRoomText.dds"));
	m_vRoomSelectUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get(), textures[textures.size() - 1].get()));
	m_vRoomSelectUIs[m_vRoomSelectUIs.size() - 1]->setPositionInViewport(0, 0);

	// InRoom=======================================================================================

	int tempIndex = meshes.size();
	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0, 0.0, 0.0), DEFINED_UAV_BUFFER_WIDTH, DEFINED_UAV_BUFFER_HEIGHT));
	int temptxt = textures.size();
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\RoomSelect\\background1.dds"));
	m_vInRoomUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get(), textures[textures.size() - 1].get()));
	m_vInRoomUIs[m_vInRoomUIs.size() - 1]->setPositionInViewport(0, 0);

	backUIIndex = m_vInRoomUIs.size();
	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0, 0.0, 0.0), 296, 484));
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			textures.emplace_back(std::make_unique<CTexture>(std::format(L"src\\texture\\UI\\InRoom\\CharacterStanding{}.dds", j).data()));
			m_vInRoomUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get(), textures[textures.size() - 1].get()));
			m_vInRoomUIs[m_vInRoomUIs.size() - 1]->setPositionInViewport((i * 296) + (18 * (i + 1)), 18);
			//m_vInRoomUIs[m_vInRoomUIs.size() - 1]->setColor(0.0, 0.0, 0.0, 0.5);
		}
	}

	readyUIIndex = m_vInRoomUIs.size();
	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0, 0.0, 0.0), 175, 65));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InRoom\\ReadyText.dds"));
	for (int i = 0; i < 3; ++i) {
		m_vInRoomUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get(), textures[textures.size() - 1].get()));
		m_vInRoomUIs[m_vInRoomUIs.size() - 1]->setPositionInViewport((i * 296) + (18 * (i + 1)) + 130, 437);
	}

	// select button
	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0, 0.0, 0.0), 300, 90));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InRoom\\SelectJobText.dds"));
	m_vInRoomUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get(), textures[textures.size() - 1].get()));
	m_vInRoomUIs[m_vInRoomUIs.size() - 1]->setPositionInViewport(18, 610);

	// curtain
	m_vInRoomUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[tempIndex].get()));
	m_vInRoomUIs[m_vInRoomUIs.size() - 1]->setPositionInViewport(0, 0);
	m_vInRoomUIs[m_vInRoomUIs.size() - 1]->setColor(0.0, 0.0, 0.0, 0.0);

	// ==========================================================================

	m_vSelectCUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[tempIndex].get(), textures[temptxt].get()));
	m_vSelectCUIs[m_vSelectCUIs.size() - 1]->setPositionInViewport(0, 0);

	// arrow
	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0, 0.0, 0.0), 100, 100));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\SelectC\\ArrowLeft.dds"));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\SelectC\\ArrowRight.dds"));
	m_vSelectCUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get(), textures[textures.size() - 2].get()));
	m_vSelectCUIs[m_vSelectCUIs.size() - 1]->setPositionInViewport(20, 310);
	m_vSelectCUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get(), textures[textures.size() - 1].get()));
	m_vSelectCUIs[m_vSelectCUIs.size() - 1]->setPositionInViewport(1150, 310);

	// back
	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0, 0.0, 0.0), 300, 90));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\SelectC\\BackText.dds"));
	m_vSelectCUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get(), textures[textures.size() - 1].get()));
	m_vSelectCUIs[m_vSelectCUIs.size() - 1]->setPositionInViewport(18, 610);

	// ok
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\SelectC\\SelectText.dds"));
	m_vSelectCUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get(), textures[textures.size() - 1].get()));
	m_vSelectCUIs[m_vSelectCUIs.size() - 1]->setPositionInViewport(962, 610);

	CUIindex = m_vSelectCUIs.size();
	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0, 0.0, 0.0), 800, 550));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\SelectC\\CharacterInfo1.dds"));
	m_vSelectCUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get(), textures[textures.size() - 1].get()));
	m_vSelectCUIs[m_vSelectCUIs.size() - 1]->setPositionInViewport(240, 40);

	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\SelectC\\CharacterInfo2.dds"));
	m_vSelectCUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get(), textures[textures.size() - 1].get()));
	m_vSelectCUIs[m_vSelectCUIs.size() - 1]->setPositionInViewport(240, 40);

	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\SelectC\\CharacterInfo3.dds"));
	m_vSelectCUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get(), textures[textures.size() - 1].get()));
	m_vSelectCUIs[m_vSelectCUIs.size() - 1]->setPositionInViewport(240, 40);

	g_pSoundManager->StartBGM(ESOUND::SOUND_TITLE_BGM);
}

void TitleScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	switch (nMessage) {
	case WM_KEYDOWN: {
		switch (g_state) {
		case Title:
			g_state = RoomSelect;
			Client.SendBroadCastRoom();
			break;
		case RoomSelect:
			switch (wParam) {
			case 'R':
				Client.SendBroadCastRoom();
				break;
			}
			break;
		case InRoom: {
			switch (wParam) {
			case 'R':
			{
				if (Players[Client.get_id()].getCharacterType() != JOB_NOTHING)	//if pick non character
				{
					bool currentReady = Players[Client.get_id()].getReady();
					if (!currentReady)
						g_pSoundManager->StartFx(ESOUND::SOUND_READY);
					Players[Client.get_id()].setReady(!currentReady);
					Client.SendsetReady(Players[Client.get_id()].getReady(), currentRoom);
				}
				break;
			}
			case VK_BACK:		// 방에서 나가는거(서버에서 인원관리 해야함)
				--userPerRoom[currentRoom];
				g_state = RoomSelect;
				break;
			}
			break;
		}
		}
		break;
	}
	case WM_KEYUP:
		break;
	}
}

void TitleScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	switch (nMessage) {
	case WM_LBUTTONDOWN: {
		int mx = LOWORD(lParam);
		int my = HIWORD(lParam);
		if (g_state != GoLoading)
			g_pSoundManager->StartFx(ESOUND::SOUND_CLICK);
		switch (g_state) {
		case Title:
			g_state = RoomSelect;
			Client.SendBroadCastRoom();
			break;
		case RoomSelect:
			for (int i = 0; i < 10; ++i) {
				int j = i % 2;
				if (j == 0) {
					int x1 = 20 + 160, x2 = 460 + 160;
					int y1 = i / 2 * 100 + 20 + 180, y2 = i / 2 * 100 + 20 + 84 + 180;
					if (mx >= x1 && mx <= x2 && my >= y1 && my <= y2) {
						if (userPerRoom[i] < 3) {
							//local_uid = userPerRoom[i]++;
							currentRoom = i;
							Client.SendEnterRoom(currentRoom);
							//g_state = InRoom;
							break;
						}
					}
				}
				else {
					int x1 = 500 + 160, x2 = 940 + 160;
					int y1 = i / 2 * 100 + 20 + 180, y2 = i / 2 * 100 + 20 + 84 + 180;
					if (mx >= x1 && mx <= x2 && my >= y1 && my <= y2) {
						if (userPerRoom[i] < 3) {
							//local_uid = userPerRoom[i]++;
							currentRoom = i;
							Client.SendEnterRoom(currentRoom);
							//g_state = InRoom;

							break;
						}
					}
				}
			}

			break;
		case InRoom:
			if (mx >= 18 && mx < 318) {
				if (my >= 610 && my < 700) {
					if (!Players[Client.get_id()].getReady()) {
						g_state = SelectC;			// change g_state
					}
					//prevJob = userJob[local_uid];
					//	Players[local_uid].getCharacterType()
					//	Players[local_uid].setCharacterType(prevJob);
					prevJob = Players[Client.get_id()].getCharacterType();
				}
			}
			break;
		case SelectC:
			short currentJob = Players[Client.get_id()].getCharacterType();
			if (mx >= 18 && mx < 318) {
				if (my >= 610 && my < 700) {
					Players[Client.get_id()].setCharacterType(prevJob);
					g_state = InRoom;		// change g_state
					Client.SendPickCharacter(currentRoom, (int)Players[Client.get_id()].getCharacterType());
				}
			}
			if (mx >= 20 && mx < 120) {
				if (my >= 310 && my < 410) {
					int newJob = (int)currentJob - 1;
					if (newJob < 1)
						newJob = 3;
					Players[Client.get_id()].setCharacterType(newJob);
				}
			}
			if (mx >= 1150 && mx < 1250) {
				if (my >= 310 && my < 410) {
					int newJob = (int)currentJob + 1;
					if (newJob > 3)
						newJob = 1;
					Players[Client.get_id()].setCharacterType(newJob);
				}
			}
			if (mx >= 962 && mx < 1262) {
				if (my >= 610 && my < 700) {
					Client.SendPickCharacter(currentRoom, (int)Players[Client.get_id()].getCharacterType());
					g_state = InRoom;		// change g_state
				}
			}
			break;
		}
		break;
	}
	case WM_LBUTTONUP: {
		break;
	}
	case WM_MOUSEMOVE: {
		break;
	}
	}
}
void TitleScene::CreateRootSignature()
{
	D3D12_DESCRIPTOR_RANGE tRange{};
	tRange.BaseShaderRegister = 0;
	tRange.NumDescriptors = 1;
	tRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

	D3D12_ROOT_PARAMETER params[3]{};
	params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[0].Descriptor.RegisterSpace = 0;
	params[0].Descriptor.ShaderRegister = 0;

	params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[1].Descriptor.RegisterSpace = 0;
	params[1].Descriptor.ShaderRegister = 1;

	params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	params[2].DescriptorTable.NumDescriptorRanges = 1;
	params[2].DescriptorTable.pDescriptorRanges = &tRange;

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
	rtDesc.NumParameters = 3;
	rtDesc.NumStaticSamplers = 1;
	rtDesc.pParameters = params;
	rtDesc.pStaticSamplers = &samplerDesc;
	rtDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* pBlob{};
	D3D12SerializeRootSignature(&rtDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &pBlob, nullptr);
	g_DxResource.device->CreateRootSignature(0, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), IID_PPV_ARGS(m_pGlobalRootSignature.GetAddressOf()));
	pBlob->Release();
}
void TitleScene::CreatePipelineState()
{
	ID3DBlob* pd3dVBlob{ nullptr };
	ID3DBlob* pd3dPBlob{ nullptr };
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineState{};
	d3dPipelineState.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	d3dPipelineState.pRootSignature = m_pGlobalRootSignature.Get();

	D3D12_INPUT_ELEMENT_DESC ldesc[3]{};
	ldesc[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	ldesc[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	ldesc[2] = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	d3dPipelineState.InputLayout.pInputElementDescs = ldesc;
	d3dPipelineState.InputLayout.NumElements = 3;

	d3dPipelineState.DepthStencilState.DepthEnable = FALSE;
	d3dPipelineState.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	d3dPipelineState.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	d3dPipelineState.DepthStencilState.StencilEnable = FALSE;

	d3dPipelineState.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	d3dPipelineState.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	d3dPipelineState.RasterizerState.AntialiasedLineEnable = FALSE;
	d3dPipelineState.RasterizerState.FrontCounterClockwise = FALSE;
	d3dPipelineState.RasterizerState.MultisampleEnable = FALSE;
	d3dPipelineState.RasterizerState.DepthClipEnable = FALSE;

	d3dPipelineState.BlendState.AlphaToCoverageEnable = FALSE;
	d3dPipelineState.BlendState.IndependentBlendEnable = FALSE;
	d3dPipelineState.BlendState.RenderTarget[0].BlendEnable = TRUE;
	d3dPipelineState.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	d3dPipelineState.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	d3dPipelineState.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	d3dPipelineState.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	d3dPipelineState.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	d3dPipelineState.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	d3dPipelineState.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
	d3dPipelineState.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	d3dPipelineState.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dPipelineState.NumRenderTargets = 1;
	d3dPipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	d3dPipelineState.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dPipelineState.SampleDesc.Count = 1;
	d3dPipelineState.SampleMask = UINT_MAX;

	D3DCompileFromFile(L"UIShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", 0, 0, &pd3dVBlob, nullptr);
	d3dPipelineState.VS.BytecodeLength = pd3dVBlob->GetBufferSize();
	d3dPipelineState.VS.pShaderBytecode = pd3dVBlob->GetBufferPointer();

	D3DCompileFromFile(L"UIShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", 0, 0, &pd3dPBlob, nullptr);
	d3dPipelineState.PS.BytecodeLength = pd3dPBlob->GetBufferSize();
	d3dPipelineState.PS.pShaderBytecode = pd3dPBlob->GetBufferPointer();

	g_DxResource.device->CreateGraphicsPipelineState(&d3dPipelineState, IID_PPV_ARGS(m_UIPipelineState.GetAddressOf()));

	if (pd3dVBlob)
		pd3dVBlob->Release();
	if (pd3dPBlob)
		pd3dPBlob->Release();
}

void TitleScene::UpdateObject(float fElapsedTime)
{
	switch (g_state) {
	case Title:
		if (startTime < 3.0f)
			startTime += fElapsedTime;
		else {
			wOpacity -= 0.1f * fElapsedTime;
			if (wOpacity < 0.0f)
				wOpacity = 0.0f;
			m_vTitleUIs[1]->setColor(0.0, 0.0, 0.0, wOpacity);
		}
		break;
	case RoomSelect: {
		for (int i = 0; i < 10; ++i) {
			for (int j = 0; j < 3; ++j) {
				if (j < userPerRoom[i]) {
					m_vRoomSelectUIs[(i * 3) + j + peopleindex]->setRenderState(true);
				}
				else
					m_vRoomSelectUIs[(i * 3) + j + peopleindex]->setRenderState(false);
			}
		}
		break;
	}
	case InRoom: {
		for (int i = 0; i < 3; ++i) {
			if (i < userPerRoom[currentRoom]) {
				if (!Players.contains(i))
					continue;
				for (int j = 0; j < 3; ++j) {
					if (j == (int)Players[i].getCharacterType() - 1)
						m_vInRoomUIs[backUIIndex + (i * 3) + j]->setRenderState(true);
					else
						m_vInRoomUIs[backUIIndex + (i * 3) + j]->setRenderState(false);
				}
				if (Players[i].getReady())
					//userReadyState
					m_vInRoomUIs[readyUIIndex + i]->setRenderState(true);
				else {
					m_vInRoomUIs[readyUIIndex + i]->setRenderState(false);
					Client.Setstart(false);
				}
			}
			else {
				for (int j = 0; j < 3; ++j)
					m_vInRoomUIs[backUIIndex + (i * 3) + j]->setRenderState(false);
				m_vInRoomUIs[readyUIIndex + i]->setRenderState(false);
			}
		}
		if (Client.getstart()) {
			wOpacity = 0.0f;
			g_state = GoLoading;
			//m_nNextScene = SCENE_WINTERLAND;	// 한번만 테스트 성공 해봤지만 계속 터져서 이거 넣어봄
		}
		break;
	}
	case GoLoading: {
		wOpacity += 0.35f * fElapsedTime;
		if (wOpacity > 1.0f) {
			wOpacity = 1.0f;
			m_nNextScene = SCENE_PLAIN;
		}
		m_vInRoomUIs[m_vInRoomUIs.size() - 1]->setColor(0.0, 0.0, 0.0, wOpacity);
		break;
	}
	case SelectC: {
		int currentJob = (int)Players[Client.get_id()].getCharacterType();
		for (int i = CUIindex; i < CUIindex + 3; ++i) {
			if (currentJob == i - CUIindex + 1)		//check
				m_vSelectCUIs[i]->setRenderState(true);
			else
				m_vSelectCUIs[i]->setRenderState(false);
		}
		break;
	}
	}
}
void TitleScene::Render()
{
	ID3D12GraphicsCommandList4* cmdList = g_DxResource.cmdList;
	auto barrier = [&](ID3D12Resource* pResource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
		{
			D3D12_RESOURCE_BARRIER resBarrier{};
			resBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			resBarrier.Transition.pResource = pResource;
			resBarrier.Transition.StateBefore = before;
			resBarrier.Transition.StateAfter = after;
			resBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			cmdList->ResourceBarrier(1, &resBarrier);
		};

	barrier(m_pOutputBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_VIEWPORT vv{};
	vv.Width = DEFINED_UAV_BUFFER_WIDTH; vv.Height = DEFINED_UAV_BUFFER_HEIGHT; vv.MinDepth = 0.0f; vv.MaxDepth = 1.0f;
	cmdList->RSSetViewports(1, &vv);
	D3D12_RECT ss{ 0, 0, DEFINED_UAV_BUFFER_WIDTH, DEFINED_UAV_BUFFER_HEIGHT };
	cmdList->RSSetScissorRects(1, &ss);
	cmdList->OMSetRenderTargets(1, &m_RTV->GetCPUDescriptorHandleForHeapStart(), FALSE, &m_DSV->GetCPUDescriptorHandleForHeapStart());
	cmdList->SetGraphicsRootSignature(m_pGlobalRootSignature.Get());
	cmdList->SetPipelineState(m_UIPipelineState.Get());
	cmdList->SetGraphicsRootConstantBufferView(0, m_cameraCB->GetGPUVirtualAddress());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	switch (g_state) {
	case Title:
		for (auto& p : m_vTitleUIs)
			p->Render();
		break;
	case RoomSelect:
		for (auto& p : m_vRoomSelectUIs)
			p->Render();
		break;
	case InRoom:
	case GoLoading:
		for (auto& p : m_vInRoomUIs)
			p->Render();
		break;
	case SelectC:
		for (auto& p : m_vSelectCUIs)
			p->Render();
		break;
	}

	barrier(m_pOutputBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

// ==================================================================================

void CRaytracingScene::UpdateObject(float fElapsedTime)
{
	// compute shader & rootSignature set
	g_DxResource.cmdList->SetPipelineState(m_pAnimationComputeShader.Get());
	g_DxResource.cmdList->SetComputeRootSignature(m_pComputeRootSignature.Get());

	m_pResourceManager->UpdateSkinningMesh(fElapsedTime);
	Flush();
	// BLAS RebBuild
	m_pResourceManager->ReBuildBLAS();

	m_pResourceManager->UpdateWorldMatrix();

	m_pCamera->getEyeCalculateOffset();
	m_pCamera->UpdateViewMatrix();
	m_pAccelerationStructureManager->UpdateScene(m_pCamera->getEye());
}

void CRaytracingScene::PrepareRender()
{
	g_DxResource.cmdList->SetPipelineState1(m_pRaytracingPipeline->getPipelineState());
	g_DxResource.cmdList->SetComputeRootSignature(m_pGlobalRootSignature.Get());
}

void CRaytracingScene::Render()
{
	m_pCamera->SetShaderVariable();
	m_pAccelerationStructureManager->SetScene();
	m_pResourceManager->SetLights();
	std::vector<std::unique_ptr<CTexture>>& textures = m_pResourceManager->getTextureList();
	g_DxResource.cmdList->SetComputeRootDescriptorTable(4, textures[textures.size() - 1]->getView()->GetGPUDescriptorHandleForHeapStart());

	D3D12_DISPATCH_RAYS_DESC raydesc{};
	raydesc.Depth = 1;
	raydesc.Width = DEFINED_UAV_BUFFER_WIDTH;
	raydesc.Height = DEFINED_UAV_BUFFER_HEIGHT;

	raydesc.RayGenerationShaderRecord.StartAddress = m_pShaderBindingTable->getRayGenTable()->GetGPUVirtualAddress();
	raydesc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	raydesc.MissShaderTable.StartAddress = m_pShaderBindingTable->getMissTable()->GetGPUVirtualAddress();
	raydesc.MissShaderTable.SizeInBytes = m_pShaderBindingTable->getMissSize();
	raydesc.MissShaderTable.StrideInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

	raydesc.HitGroupTable.StartAddress = m_pShaderBindingTable->getHitGroupTable()->GetGPUVirtualAddress();
	raydesc.HitGroupTable.SizeInBytes = m_pShaderBindingTable->getHitGroupSize();
	raydesc.HitGroupTable.StrideInBytes = m_pShaderBindingTable->getHitGroupStride();

	g_DxResource.cmdList->DispatchRays(&raydesc);
}

void CRaytracingScene::CreateComputeRootSignature()
{
	D3D12_DESCRIPTOR_RANGE skinningRange[6]{};
	{
		skinningRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		skinningRange[0].NumDescriptors = 1;
		skinningRange[0].BaseShaderRegister = 0;
		skinningRange[0].RegisterSpace = 0;
		skinningRange[0].OffsetInDescriptorsFromTableStart = 0;

		skinningRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		skinningRange[1].NumDescriptors = 1;
		skinningRange[1].BaseShaderRegister = 0;
		skinningRange[1].RegisterSpace = 0;
		skinningRange[1].OffsetInDescriptorsFromTableStart = 1;

		skinningRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		skinningRange[2].NumDescriptors = 1;
		skinningRange[2].BaseShaderRegister = 1;
		skinningRange[2].RegisterSpace = 0;
		skinningRange[2].OffsetInDescriptorsFromTableStart = 2;

		skinningRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		skinningRange[3].NumDescriptors = 1;
		skinningRange[3].BaseShaderRegister = 2;
		skinningRange[3].RegisterSpace = 0;
		skinningRange[3].OffsetInDescriptorsFromTableStart = 3;

		skinningRange[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		skinningRange[4].NumDescriptors = 1;
		skinningRange[4].BaseShaderRegister = 3;
		skinningRange[4].RegisterSpace = 0;
		skinningRange[4].OffsetInDescriptorsFromTableStart = 4;

		skinningRange[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		skinningRange[5].NumDescriptors = 1;
		skinningRange[5].BaseShaderRegister = 4;
		skinningRange[5].RegisterSpace = 0;
		skinningRange[5].OffsetInDescriptorsFromTableStart = 5;
	}

	D3D12_DESCRIPTOR_RANGE insertRange[5]{};
	{
		insertRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		insertRange[0].NumDescriptors = 1;
		insertRange[0].BaseShaderRegister = 1;
		insertRange[0].RegisterSpace = 0;
		insertRange[0].OffsetInDescriptorsFromTableStart = 0;

		insertRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		insertRange[1].NumDescriptors = 1;
		insertRange[1].BaseShaderRegister = 5;
		insertRange[1].RegisterSpace = 0;
		insertRange[1].OffsetInDescriptorsFromTableStart = 1;

		insertRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		insertRange[2].NumDescriptors = 1;
		insertRange[2].BaseShaderRegister = 6;
		insertRange[2].RegisterSpace = 0;
		insertRange[2].OffsetInDescriptorsFromTableStart = 2;

		insertRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		insertRange[3].NumDescriptors = 1;
		insertRange[3].BaseShaderRegister = 7;
		insertRange[3].RegisterSpace = 0;
		insertRange[3].OffsetInDescriptorsFromTableStart = 3;

		insertRange[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		insertRange[4].NumDescriptors = 1;
		insertRange[4].BaseShaderRegister = 8;
		insertRange[4].RegisterSpace = 0;
		insertRange[4].OffsetInDescriptorsFromTableStart = 4;
	}

	D3D12_DESCRIPTOR_RANGE uavRange[4]{};
	{
		uavRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		uavRange[0].NumDescriptors = 1;
		uavRange[0].BaseShaderRegister = 0;
		uavRange[0].RegisterSpace = 0;
		uavRange[0].OffsetInDescriptorsFromTableStart = 0;

		uavRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		uavRange[1].NumDescriptors = 1;
		uavRange[1].BaseShaderRegister = 1;
		uavRange[1].RegisterSpace = 0;
		uavRange[1].OffsetInDescriptorsFromTableStart = 1;

		uavRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		uavRange[2].NumDescriptors = 1;
		uavRange[2].BaseShaderRegister = 2;
		uavRange[2].RegisterSpace = 0;
		uavRange[2].OffsetInDescriptorsFromTableStart = 2;

		uavRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		uavRange[3].NumDescriptors = 1;
		uavRange[3].BaseShaderRegister = 3;
		uavRange[3].RegisterSpace = 0;
		uavRange[3].OffsetInDescriptorsFromTableStart = 3;
	}

	// 0 - ani info, 1 - InputVertex, 2 - OutputVertex
	D3D12_ROOT_PARAMETER params[3]{};
	params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	params[0].DescriptorTable.NumDescriptorRanges = 6;
	params[0].DescriptorTable.pDescriptorRanges = skinningRange;

	params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	params[1].DescriptorTable.NumDescriptorRanges = 5;
	params[1].DescriptorTable.pDescriptorRanges = insertRange;

	params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	params[2].DescriptorTable.NumDescriptorRanges = 4;
	params[2].DescriptorTable.pDescriptorRanges = uavRange;

	//D3D12_ROOT_PARAMETER params[9]{};
	//params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	//params[0].Descriptor.ShaderRegister = 0;

	//params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	//params[1].Descriptor.ShaderRegister = 0;

	//params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	//params[2].Descriptor.ShaderRegister = 1;

	//params[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	//params[3].Descriptor.ShaderRegister = 2;

	//params[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	//params[4].Descriptor.ShaderRegister = 3;

	//params[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	//params[5].Descriptor.ShaderRegister = 4;

	//params[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	//params[6].Descriptor.ShaderRegister = 5;

	//params[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//params[7].DescriptorTable.NumDescriptorRanges = 1;
	//params[7].DescriptorTable.pDescriptorRanges = &uavRange;

	//// test
	//params[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	//params[8].Descriptor.ShaderRegister = 6;



	D3D12_ROOT_SIGNATURE_DESC desc{};
	desc.NumParameters = 3;
	desc.pParameters = params;
	desc.NumStaticSamplers = 0;
	desc.pStaticSamplers = nullptr;

	ID3DBlob* pBlob{};
	D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &pBlob, nullptr);
	g_DxResource.device->CreateRootSignature(0, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), IID_PPV_ARGS(m_pComputeRootSignature.GetAddressOf()));
	pBlob->Release();
}

template<typename T, typename U>
	requires (HasGameObjectInterface<T> || HasSkinningObjectInterface<T>) && HasSkinningObjectInterface<U>
inline bool CRaytracingScene::CheckSphereCollision(const std::vector<std::unique_ptr<T>>& object1, const std::vector<std::unique_ptr<U>>& object2)
{
	bool collisionDetected = false;
	auto& meshes = m_pResourceManager->getMeshList();

	for (const std::unique_ptr<T>& obj1 : object1) {
		if constexpr (HasGameObjectInterface<T>) {
			int meshIndex = obj1->getMeshIndex();
			if (meshIndex != -1 && meshIndex < meshes.size() && meshes[meshIndex]->getHasVertex() && meshes[meshIndex]->getHasBoundingBox()) {
				DirectX::BoundingOrientedBox mapOBB;
				meshes[meshIndex]->getOBB().Transform(mapOBB, DirectX::XMLoadFloat4x4(&obj1->getWorldMatrix()));

				for (const auto& character : object2) {
					for (const auto& bone : character->getObjects()) {
						if (bone->getBoundingInfo() & 0x1100) {
							DirectX::BoundingSphere boneSphere = bone->getObjectSphere();
							bone->getObjectSphere().Transform(boneSphere, DirectX::XMLoadFloat4x4(&bone->getWorldMatrix()));
							if (mapOBB.Intersects(boneSphere)) {
								collisionDetected = true;
							}
						}
					}
				}
			}
		}

		if constexpr (HasSkinningObjectInterface<T>) {
			for (const auto& bone1 : obj1->getObjects()) {
				if (bone1->getBoundingInfo() & 0x1100) { 
					DirectX::BoundingSphere boneSphere1 = bone1->getObjectSphere();
					bone1->getObjectSphere().Transform(boneSphere1, DirectX::XMLoadFloat4x4(&bone1->getWorldMatrix()));

					for (const auto& character : object2) {
						if (obj1 != character) { 
							for (const auto& bone2 : character->getObjects()) {
								if (bone2->getBoundingInfo() & 0x1100) { 
									DirectX::BoundingSphere boneSphere2 = bone2->getObjectSphere();
									bone2->getObjectSphere().Transform(boneSphere2, DirectX::XMLoadFloat4x4(&bone2->getWorldMatrix()));
									if (boneSphere1.Intersects(boneSphere2)) {
										collisionDetected = true;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return collisionDetected;
}

template<typename T, typename U>
	requires (HasGameObjectInterface<T> || HasSkinningObjectInterface<T>) && HasSkinningObjectInterface<U>
inline void CRaytracingScene::CheckOBBCollisions(const std::vector<std::unique_ptr<T>>& object1, const std::vector<std::unique_ptr<U>>& object2)
{
	auto& meshes = m_pResourceManager->getMeshList();

	for (const std::unique_ptr<T>& obj1 : object1) {
		if constexpr (HasGameObjectInterface<T>) {
			int meshIndex = obj1->getMeshIndex();
			if (meshIndex != -1 && meshIndex < meshes.size() && meshes[meshIndex]->getHasVertex() && meshes[meshIndex]->getHasBoundingBox()) {
				DirectX::BoundingOrientedBox mapOBB;
				meshes[meshIndex]->getOBB().Transform(mapOBB, DirectX::XMLoadFloat4x4(&obj1->getWorldMatrix()));

				for (const auto& character : object2) {
					for (const auto& bone : character->getObjects()) {
						if (bone->getBoundingInfo() & 0x0011) { 
							DirectX::BoundingOrientedBox boneOBB;
							bone->getObjectOBB().Transform(boneOBB, DirectX::XMLoadFloat4x4(&bone->getWorldMatrix()));
							if (mapOBB.Intersects(boneOBB)) {
							}
						}
					}
				}
			}
		}

		if constexpr (HasSkinningObjectInterface<T>) {
			for (const auto& bone1 : obj1->getObjects()) {
				if (bone1->getBoundingInfo() & 0x0011) { 
					DirectX::BoundingOrientedBox boneOBB1;
					bone1->getObjectOBB().Transform(boneOBB1, DirectX::XMLoadFloat4x4(&bone1->getWorldMatrix()));

					for (const auto& character : object2) {
						if (obj1 != character) {
							for (const auto& bone2 : character->getObjects()) {
								if (bone2->getBoundingInfo() & 0x0011) { 
									DirectX::BoundingOrientedBox boneOBB2;
									bone2->getObjectOBB().Transform(boneOBB2, DirectX::XMLoadFloat4x4(&bone2->getWorldMatrix()));
									if (boneOBB1.Intersects(boneOBB2)) {
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void CRaytracingScene::TestCollision(const std::vector<std::unique_ptr<CGameObject>>& mapObjects, const std::vector<std::unique_ptr<CSkinningObject>>& characters)
{
	auto& meshes = m_pResourceManager->getMeshList();

	struct CollisionInfo {
		XMFLOAT3 normal;
		float depth;
		float meshHeight;
	};

	for (const auto& character : characters) {
		std::vector<CollisionInfo> collisions;

		for (const auto& mapObj : mapObjects) {
			int meshIndex = mapObj->getMeshIndex();
			if (meshIndex == -1 || meshIndex >= meshes.size()) continue;

			auto& mesh = meshes[meshIndex];
			if (!mesh->getHasVertex() || !mesh->getHasBoundingBox()) continue;

			BoundingOrientedBox mapOBB;
			mesh->getOBB().Transform(mapOBB, XMLoadFloat4x4(&mapObj->getWorldMatrix()));

			float meshHeight = mapOBB.Extents.y * 2.0f;

			for (const auto& bone : character->getObjects()) {
				if (bone->getBoundingInfo() & 0x1100) { // Sphere
					BoundingSphere boneSphere = bone->getObjectSphere();
					boneSphere.Transform(boneSphere, XMLoadFloat4x4(&bone->getWorldMatrix()));
					if (mapOBB.Intersects(boneSphere)) {
						XMFLOAT3 norm = CalculateCollisionNormal(mapOBB, boneSphere);
						float depth = CalculateDepth(mapOBB, boneSphere);
						//collisions.push_back({ norm, depth, meshHeight });
					}
				}
			}
		}

		if (!collisions.empty()) {

			auto maxCollision = std::max_element(collisions.begin(), collisions.end(),[](const CollisionInfo& a, const CollisionInfo& b) { return a.depth < b.depth; });

			XMFLOAT3 norm = maxCollision->normal;
			float depth = maxCollision->depth;
			float meshHeight = maxCollision->meshHeight;

			XMVECTOR moveDir = XMLoadFloat3(&character->getMoveDirection());
			XMVECTOR normal = XMLoadFloat3(&norm);
			float dotProduct = XMVectorGetX(XMVector3Dot(moveDir, normal));
			if (dotProduct < 0.0f) { 
				character->sliding(depth, norm, meshHeight);
			}
		}
	}
}

void CRaytracingScene::TestShootCollision(const std::vector<std::unique_ptr<CProjectile>>& Objects, const std::vector<std::unique_ptr<CSkinningObject>>& characters)
{
	for (const auto& projectile : Objects) {
		BoundingOrientedBox projectileOBB = projectile->getObjects().getObjectOBB();
		projectileOBB.Transform(projectileOBB, XMLoadFloat4x4(&projectile->getObjects().getWorldMatrix()));

		for (size_t i = 1; i < characters.size(); ++i) {
			const auto& character = characters[i];
			for (const auto& bone : character->getObjects()) {
				if (bone->getBoundingInfo() & 0x1100) { // Sphere
					BoundingSphere boneSphere = bone->getObjectSphere();
					boneSphere.Transform(boneSphere, XMLoadFloat4x4(&bone->getWorldMatrix()));
					if (projectileOBB.Intersects(boneSphere)) {
						m_pResourceManager->getAnimationManagers()[i]->ChangeAnimation(0, true);
						m_pResourceManager->getAnimationManagers()[i]->IsCollision();
						projectile->setActive(false);
						projectile->setTime(0.0f);
					}
				}
			}
		}
	}
}

XMFLOAT3 CRaytracingScene::CalculateCollisionNormal(const BoundingOrientedBox& obb, const BoundingSphere& sphere)
{

	XMVECTOR sphereCenter = XMLoadFloat3(&sphere.Center);
	XMVECTOR obbCenter = XMLoadFloat3(&obb.Center);
	XMVECTOR orientation = XMLoadFloat4(&obb.Orientation);
	XMMATRIX rotation = XMMatrixRotationQuaternion(orientation);

	XMVECTOR axes[3] = {
		rotation.r[0],
		rotation.r[1],
		rotation.r[2]
	};

	XMVECTOR closestPoint = obbCenter;
	XMVECTOR d = sphereCenter - obbCenter;

	for (int i = 0; i < 3; ++i) {
		float distance = XMVectorGetX(XMVector3Dot(d, axes[i]));
		float extent = (&obb.Extents.x)[i];
		distance = std::clamp(distance, -extent, extent);
		closestPoint += axes[i] * distance;
	}

	XMVECTOR normalVec = XMVector3Normalize(sphereCenter - closestPoint);
	XMFLOAT3 normal;
	XMStoreFloat3(&normal, normalVec);
	return normal;
}

float CRaytracingScene::CalculateDepth(const BoundingOrientedBox& obb, const BoundingSphere& sphere)
{
	XMVECTOR sphereCenter = XMLoadFloat3(&sphere.Center);
	XMVECTOR obbCenter = XMLoadFloat3(&obb.Center);
	XMVECTOR orientation = XMLoadFloat4(&obb.Orientation);
	XMMATRIX rotation = XMMatrixRotationQuaternion(orientation);

	XMVECTOR axes[3] = {
		rotation.r[0],
		rotation.r[1],
		rotation.r[2]
	};

	XMVECTOR closestPoint = obbCenter;
	XMVECTOR d = sphereCenter - obbCenter;

	for (int i = 0; i < 3; ++i) {
		float distance = XMVectorGetX(XMVector3Dot(d, axes[i]));
		float extent = (&obb.Extents.x)[i];
		distance = std::clamp(distance, -extent, extent);
		closestPoint += axes[i] * distance;
	}

	float dist = XMVectorGetX(XMVector3Length(sphereCenter - closestPoint));
	return sphere.Radius - dist;
}

void CRaytracingScene::CreateComputeShader()
{
	ID3DBlob* pBlob{};
	ID3DBlob* errorb{};
	D3DCompileFromFile(L"AnimationComputeShader.hlsl", nullptr, nullptr, "main", "cs_5_1", 0, 0, &pBlob, &errorb);
	if (errorb)
		OutputDebugStringA((char*)errorb->GetBufferPointer());


	//D3D12_CACHED_PIPELINE_STATE tempState{};
	D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
	desc.pRootSignature = m_pComputeRootSignature.Get();
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	desc.CS.BytecodeLength = pBlob->GetBufferSize();
	desc.CS.pShaderBytecode = pBlob->GetBufferPointer();

	g_DxResource.device->CreateComputePipelineState(&desc, IID_PPV_ARGS(m_pAnimationComputeShader.GetAddressOf()));

	pBlob->Release();
}

// =====================================================================================

void CRaytracingGameScene::CreateUIRootSignature()
{
	D3D12_DESCRIPTOR_RANGE tRange{};
	tRange.BaseShaderRegister = 0;
	tRange.NumDescriptors = 1;
	tRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

	D3D12_ROOT_PARAMETER params[3]{};
	params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[0].Descriptor.RegisterSpace = 0;
	params[0].Descriptor.ShaderRegister = 0;

	params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[1].Descriptor.RegisterSpace = 0;
	params[1].Descriptor.ShaderRegister = 1;

	params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	params[2].DescriptorTable.NumDescriptorRanges = 1;
	params[2].DescriptorTable.pDescriptorRanges = &tRange;

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
	rtDesc.NumParameters = 3;
	rtDesc.NumStaticSamplers = 1;
	rtDesc.pParameters = params;
	rtDesc.pStaticSamplers = &samplerDesc;
	rtDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* pBlob{};
	D3D12SerializeRootSignature(&rtDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &pBlob, nullptr);
	g_DxResource.device->CreateRootSignature(0, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), IID_PPV_ARGS(m_UIRootSignature.GetAddressOf()));
	pBlob->Release();
}
void CRaytracingGameScene::CreateUIPipelineState()
{
	ID3DBlob* pd3dVBlob{ nullptr };
	ID3DBlob* pd3dPBlob{ nullptr };
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineState{};
	d3dPipelineState.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	d3dPipelineState.pRootSignature = m_UIRootSignature.Get();

	D3D12_INPUT_ELEMENT_DESC ldesc[3]{};
	ldesc[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	ldesc[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	ldesc[2] = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	d3dPipelineState.InputLayout.pInputElementDescs = ldesc;
	d3dPipelineState.InputLayout.NumElements = 3;

	d3dPipelineState.DepthStencilState.DepthEnable = FALSE;
	d3dPipelineState.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	d3dPipelineState.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	d3dPipelineState.DepthStencilState.StencilEnable = FALSE;

	d3dPipelineState.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	d3dPipelineState.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	d3dPipelineState.RasterizerState.AntialiasedLineEnable = FALSE;
	d3dPipelineState.RasterizerState.FrontCounterClockwise = FALSE;
	d3dPipelineState.RasterizerState.MultisampleEnable = FALSE;
	d3dPipelineState.RasterizerState.DepthClipEnable = FALSE;

	d3dPipelineState.BlendState.AlphaToCoverageEnable = FALSE;
	d3dPipelineState.BlendState.IndependentBlendEnable = FALSE;
	d3dPipelineState.BlendState.RenderTarget[0].BlendEnable = TRUE;
	d3dPipelineState.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	d3dPipelineState.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	d3dPipelineState.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	d3dPipelineState.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	d3dPipelineState.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	d3dPipelineState.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	d3dPipelineState.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
	d3dPipelineState.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	d3dPipelineState.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dPipelineState.NumRenderTargets = 1;
	d3dPipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	d3dPipelineState.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dPipelineState.SampleDesc.Count = 1;
	d3dPipelineState.SampleMask = UINT_MAX;

	D3DCompileFromFile(L"UIShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", 0, 0, &pd3dVBlob, nullptr);
	d3dPipelineState.VS.BytecodeLength = pd3dVBlob->GetBufferSize();
	d3dPipelineState.VS.pShaderBytecode = pd3dVBlob->GetBufferPointer();

	D3DCompileFromFile(L"UIShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", 0, 0, &pd3dPBlob, nullptr);
	d3dPipelineState.PS.BytecodeLength = pd3dPBlob->GetBufferSize();
	d3dPipelineState.PS.pShaderBytecode = pd3dPBlob->GetBufferPointer();

	g_DxResource.device->CreateGraphicsPipelineState(&d3dPipelineState, IID_PPV_ARGS(m_UIPipelineState.GetAddressOf()));

	if (pd3dVBlob)
		pd3dVBlob->Release();
	if (pd3dPBlob)
		pd3dPBlob->Release();
}

void CRaytracingGameScene::AttackCollision(const std::vector<std::unique_ptr<CPlayableCharacter>>& targets, const std::vector<std::unique_ptr<CPlayableCharacter>>& attackers, int flag)
{
	if (flag == 1) {
		for (int i = 0; i < targets.size(); i++) {

			//for (const auto& target : targets) {
			if (targets[i]->IsDodge())continue;
			if (!targets[i]->CanBeAttacked())continue;
			for (const auto& targetBone : targets[i]->getObject()->getObjects()) {
				if (!(targetBone->getBoundingInfo() & 0x1100)) continue;
				BoundingSphere targetSphere = targetBone->getObjectSphere();
				BoundingSphere transformedTargetSphere;
				targetSphere.Transform(transformedTargetSphere, XMLoadFloat4x4(&targetBone->getWorldMatrix()));
				int attacker_id = m_local_id;
				//	for (const auto& attacker : attackers) {
				if (!attackers[attacker_id]->IsAttacking() && !attackers[attacker_id]->IsCombo()) return;
				for (const auto& attackerBone : attackers[attacker_id]->getObject()->getObjects()) {
					if (!(attackerBone->getBoundingInfo() & 0x1000)) continue;
					BoundingSphere attackerSphere = attackerBone->getObjectSphere();
					BoundingSphere transformedAttackerSphere;
					attackerSphere.Transform(transformedAttackerSphere, XMLoadFloat4x4(&attackerBone->getWorldMatrix()));
					if (transformedAttackerSphere.Intersects(transformedTargetSphere)) {
						/*float damage = 0.0f;
						damage = attackers[attacker_id]->getCurrentDamage();*/
						bool realHit = targets[i]->Attacked();
						if (realHit) {
							if (flag == 1)
							{
								Client.SendPlayerAttack(i, attackers[attacker_id]->getCurrentSkill());
								//플레이어가 공격할 때
							}
							else
							{
								Client.SendMonsterAttack(attacker_id, i, attackers[attacker_id]->getCurrentSkill());
								//몬스터가 공격할 때

							}
						}
						return;
					}
				}
			}
		}
	}
	else {
		//스피어-스피어
		int i = m_local_id;
		if (targets[i]->IsDodge())return;
		if (!targets[i]->CanBeAttacked())return;
		for (const auto& targetBone : targets[i]->getObject()->getObjects()) {
			if (!(targetBone->getBoundingInfo() & 0x1100)) continue;
			BoundingSphere targetSphere = targetBone->getObjectSphere();
			BoundingSphere transformedTargetSphere;
			targetSphere.Transform(transformedTargetSphere, XMLoadFloat4x4(&targetBone->getWorldMatrix()));
			for (int attacker_id = 0; attacker_id < attackers.size(); attacker_id++) {
				//	for (const auto& attacker : attackers) {
				if (!attackers[attacker_id]->IsAttacking() && !attackers[attacker_id]->IsCombo()) continue;
				for (const auto& attackerBone : attackers[attacker_id]->getObject()->getObjects()) {
					if (!(attackerBone->getBoundingInfo() & 0x1000)) continue;
					BoundingSphere attackerSphere = attackerBone->getObjectSphere();
					BoundingSphere transformedAttackerSphere;
					attackerSphere.Transform(transformedAttackerSphere, XMLoadFloat4x4(&attackerBone->getWorldMatrix()));
					if (transformedAttackerSphere.Intersects(transformedTargetSphere)) {
						/*float damage = 0.0f;
						damage = attackers[attacker_id]->getCurrentDamage();*/
						bool realHit = targets[i]->Attacked();
						if (realHit) {
							if (flag == 1)
							{
								Client.SendPlayerAttack(i, attackers[attacker_id]->getCurrentSkill());
								//플레이어가 공격할 때
							}
							else
							{
								Client.SendMonsterAttack(attacker_id, i, attackers[attacker_id]->getCurrentSkill());
								//몬스터가 공격할 때

							}
						}
						return;
					}
				}
			}
		}
	}
}

void CRaytracingGameScene::ShootCollision(const std::vector<std::unique_ptr<CPlayableCharacter>>& targets, const std::vector<std::unique_ptr<CPlayableCharacter>>& attackers, int flag)
{
	if (flag == 1) {
		for (int i = 0; i < targets.size(); i++) {
			if (targets[i]->IsDodge())continue;
			//for (const auto& target : targets) {
			for (const auto& targetBone : targets[i]->getObject()->getObjects()) {
				if (!(targetBone->getBoundingInfo() & 0x1100)) continue;
				BoundingSphere targetSphere = targetBone->getObjectSphere();
				BoundingSphere transformedTargetSphere;
				targetSphere.Transform(transformedTargetSphere, XMLoadFloat4x4(&targetBone->getWorldMatrix()));
				int attacker_id = m_local_id;
				//	for (const auto& attacker : attackers) {
				auto& bullets = attackers[attacker_id]->GetBullets();
				if (bullets.empty()) return;
				for (const auto& bullet : bullets) {
					if (!bullet || !bullet->getActive()) continue;
					BoundingOrientedBox bulletSphere = bullet->getObjects().getObjectOBB();
					BoundingOrientedBox transformedBulletBox;
					bulletSphere.Transform(transformedBulletBox, XMLoadFloat4x4(&bullet->getObjects().getWorldMatrix()));
					if (transformedBulletBox.Intersects(transformedTargetSphere)) {
						//float damage = attackers[attacker_id]->getCurrentDamage();
						bool realHit = targets[i]->Attacked();
						bullet->getObjects().SetPosition(attackers[attacker_id]->getObject()->getPosition());
						bullet->getObjects().SetRenderState(false);
						bullet->setActive(false);
						if (realHit) {
							if (flag == 1)
							{
								Client.SendPlayerAttack(i, attackers[attacker_id]->getCurrentSkill());
								//플레이어가 공격할 때
							}
							else
							{
								//몬스터가 공격할 때
								Client.SendMonsterAttack(attacker_id, i, attackers[attacker_id]->getCurrentSkill());
							}
						}
					}
				}

			}
		}
	}
	else {
		int i = m_local_id;
		if (targets[i]->IsDodge()) return;
		//for (const auto& target : targets) {
		for (const auto& targetBone : targets[i]->getObject()->getObjects()) {
			if (!(targetBone->getBoundingInfo() & 0x1100)) continue;
			BoundingSphere targetSphere = targetBone->getObjectSphere();
			BoundingSphere transformedTargetSphere;
			targetSphere.Transform(transformedTargetSphere, XMLoadFloat4x4(&targetBone->getWorldMatrix()));
			for (int attacker_id = 0; attacker_id < attackers.size(); attacker_id++) {
				//	for (const auto& attacker : attackers) {
				auto& bullets = attackers[attacker_id]->GetBullets();
				if (bullets.empty()) continue;
				for (const auto& bullet : bullets) {
					if (!bullet || !bullet->getActive()) continue;
					BoundingOrientedBox bulletSphere = bullet->getObjects().getObjectOBB();
					BoundingOrientedBox transformedBulletBox;
					bulletSphere.Transform(transformedBulletBox, XMLoadFloat4x4(&bullet->getObjects().getWorldMatrix()));
					if (transformedBulletBox.Intersects(transformedTargetSphere)) {
						//float damage = attackers[attacker_id]->getCurrentDamage();
						bool realHit = targets[i]->Attacked();
						bullet->getObjects().SetPosition(attackers[attacker_id]->getObject()->getPosition());
						bullet->getObjects().SetRenderState(false);
						bullet->setActive(false);
						if (realHit) {
							if (flag == 1)
							{
								Client.SendPlayerAttack(i, attackers[attacker_id]->getCurrentSkill());
								//플레이어가 공격할 때
							}
							else
							{
								//몬스터가 공격할 때
								Client.SendMonsterAttack(attacker_id, i, attackers[attacker_id]->getCurrentSkill());
							}
						}
					}
				}
			}
		}
	}

}

void CRaytracingGameScene::AutoDirection(const std::vector<std::unique_ptr<CPlayableCharacter>>& attacker, const std::vector<std::unique_ptr<CPlayableCharacter>>& targets)
{
	auto& attackerPtr = attacker[m_local_id];
		if (attackerPtr->GetBullets().empty()) return;
		XMFLOAT3 attackerPos = attackerPtr->getObject()->getPosition();
		XMFLOAT3 attackerDir = attackerPtr->getObject()->getLook();
		float fov = 90.0f * (3.14159f / 180.0f);
		float cosFov = std::cos(fov / 2.0f);
		float maxDistance = 120.0f;
		float minDistance = 120.0f;
		XMFLOAT3 directionToTarget = { 0.0f, 0.0f, 0.0f };
		bool targetFound = false;
		XMVECTOR vAttackerDir = XMLoadFloat3(&attackerDir);
		XMVECTOR vAttackerPos = XMLoadFloat3(&attackerPos);
		for (const auto& target : targets) {
			if (!target) continue;
			XMFLOAT3 targetPos = target->getObject()->getPosition();
			targetPos.y += 1.0f;
			XMVECTOR vTargetPos = XMLoadFloat3(&targetPos);
			XMVECTOR vRelativeDir = XMVectorSubtract(vTargetPos, vAttackerPos);
			XMVECTOR vDistance = XMVector3Length(vRelativeDir);
			float distance;
			XMStoreFloat(&distance, vDistance);
			if (distance > 0.0f && distance <= maxDistance) {
				XMVECTOR vNormRelativeDir = XMVector3Normalize(vRelativeDir);
				XMVECTOR vDot = XMVector3Dot(vAttackerDir, vNormRelativeDir);
				float dot;
				XMStoreFloat(&dot, vDot);
				if (dot >= cosFov && distance < minDistance) {
					minDistance = distance;
					XMStoreFloat3(&directionToTarget, vNormRelativeDir);
					targetFound = true;

				}
			}
		}

		if (targetFound) {
			attackerPtr->SetAutoDirect(directionToTarget);
		}
		else {
			XMFLOAT3 dir = m_pCamera->getDir();
			attackerPtr->SetAutoDirect({ dir.x,0.0f,dir.z });
		}
}

void CRaytracingGameScene::BulletCheck(const std::vector<std::unique_ptr<CPlayableCharacter>>& shoot, CHeightMapImage* terrain, CHeightMapImage* collisionMap, float fElapsedTime, float offsetX, float offsetY, float offsetZ, int sceneType)
{
	for (int attacker_id = 0; attacker_id < shoot.size(); attacker_id++) {
		auto& bullets = shoot[attacker_id]->GetBullets();
		if (bullets.empty()) continue;
		for (const auto& bullet : bullets) {
			if (!bullet || !bullet->getActive()) continue;
			XMFLOAT4X4& projWorld = bullet->getObjects().getWorldMatrix();
			float colHeight = collisionMap->GetHeightinWorldSpace(projWorld._41 - offsetX, projWorld._43 - offsetZ);
			float terrainHeight = terrain->GetHeightinWorldSpace(projWorld._41 - offsetX, projWorld._43 - offsetZ);
			if (colHeight - terrainHeight >= 0.1f)
			{
				bullet->getObjects().SetPosition(shoot[attacker_id]->getObject()->getPosition());
				bullet->getObjects().SetRenderState(false);
				bullet->setActive(false);
			}
		}
	}
}

void CRaytracingGameScene::BulletCheck(const std::vector<std::unique_ptr<CPlayableCharacter>>& shoot, CHeightMapImage* terrain, float fElapsedTime, float offsetX, float offsetY, float offsetZ, int sceneType)
{
	for (int attacker_id = 0; attacker_id < shoot.size(); attacker_id++) {
		auto& bullets = shoot[attacker_id]->GetBullets();
		if (bullets.empty()) continue;
		for (const auto& bullet : bullets) {
			if (!bullet || !bullet->getActive()) continue;
			XMFLOAT4X4& projWorld = bullet->getObjects().getWorldMatrix();
			float terrainHeight = terrain->GetHeightinWorldSpace(projWorld._41 - offsetX, projWorld._43 - offsetZ);
			if (terrainHeight > 0.0f)
			{
				bullet->getObjects().SetPosition(shoot[attacker_id]->getObject()->getPosition());
				bullet->getObjects().SetRenderState(false);
				bullet->setActive(false);
			}
		}
	}
}

void CRaytracingGameScene::CreateMageCharacter()
{
	m_pResourceManager->AddSkinningResourceFromFile(L"src\\model\\Greycloak_33.bin", "src\\texture\\Greycloak\\", JOB_MAGE);
	m_vPlayers.emplace_back(std::make_unique<CPlayerMage>(
		m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 1].get(),
		m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get()));

	// 07.24 preTransform Set
	auto& sv = m_pResourceManager->getSkinningObjectList();
	sv.back()->setPreTransform(2.5f, XMFLOAT3(), XMFLOAT3());

	for (auto& s : m_vPlayers.back()->getObject()->getObjects())
	{
		if (s->getFrameName() == "Bip001-Spine1")
		{
			m_vPlayers.back()->SetHead(s.get());
			break;
		}
	}
	// Create Mage's own objects and Set
	// ex) bullet, particle, barrier  etc...
	//m_pResourceManager->getMeshList().emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0f, 0.0f, 0.0f), 1.0f, "sphere"));
	m_pResourceManager->AddResourceFromFile(L"src\\model\\ETP_Rock_Small03.bin", "src\\texture\\Map\\");
	size_t meshIndex = m_pResourceManager->getMeshList().size() - 1;
	CPlayerMage* mage = dynamic_cast<CPlayerMage*>(m_vPlayers.back().get());
	Material sharedMaterial;

	for (int i = 0; i < 20; ++i) {
		m_pResourceManager->getGameObjectList().emplace_back(std::make_unique<CGameObject>());
		m_pResourceManager->getGameObjectList().back()->SetMeshIndex(meshIndex);
		m_pResourceManager->getGameObjectList().back()->getMaterials().push_back(sharedMaterial);

		auto projectile = std::make_unique<CProjectile>();
		projectile->setGameObject(m_pResourceManager->getGameObjectList().back().get());

		mage->GetBullets().push_back(std::move(projectile));
	}
}
void CRaytracingGameScene::CreateWarriorCharacter()
{
	m_pResourceManager->AddSkinningResourceFromFile(L"src\\model\\swordman_lv1.bin", "src\\texture\\Swordman\\", JOB_WARRIOR);
	m_vPlayers.emplace_back(std::make_unique<CPlayerWarrior>(
		m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 1].get(),
		m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get()));

	auto& sv = m_pResourceManager->getSkinningObjectList();
	sv.back()->getTextures().emplace_back(std::make_shared<CTexture>(L"src\\texture\\Swordman\\@Dex Studio_soullike_style.dds"));
	for (auto& p : sv.back()->getObjects()) {
		auto& v = p->getMaterials();
		if (v.size()) {
			v[0].m_bHasAlbedoMap = true;
			v[0].m_nAlbedoMapIndex = 0;
		}
	}

	sv.back()->setPreTransform(2.8f, XMFLOAT3(), XMFLOAT3());
}
void CRaytracingGameScene::CreatePriestCharacter()
{
	m_pResourceManager->AddSkinningResourceFromFile(L"src\\model\\Luna_Firemantle_33.bin", "src\\texture\\Luna\\", JOB_HEALER);
	m_vPlayers.emplace_back(std::make_unique<CPlayerPriest>(
		m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 1].get(),
		m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get()));

	auto& sv = m_pResourceManager->getSkinningObjectList();
	sv.back()->setPreTransform(2.7f, XMFLOAT3(), XMFLOAT3());

	for (auto& s : m_vPlayers.back()->getObject()->getObjects())
	{
		if (s->getFrameName() == "Bip001-Spine1")
		{
			m_vPlayers.back()->SetHead(s.get());
			break;
		}
	}

	//m_pResourceManager->getMeshList().emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0f, 0.0f, 0.0f), 1.0f, "sphere"));
	m_pResourceManager->AddResourceFromFile(L"src\\model\\ETP_Rock_Small03.bin", "src\\texture\\Map\\");
	size_t meshIndex = m_pResourceManager->getMeshList().size() - 1;
	CPlayerPriest* priest = dynamic_cast<CPlayerPriest*>(m_vPlayers.back().get());
	Material sharedMaterial;

	for (int i = 0; i < 20; ++i) {
		m_pResourceManager->getGameObjectList().emplace_back(std::make_unique<CGameObject>());
		m_pResourceManager->getGameObjectList().back()->SetMeshIndex(meshIndex);
		m_pResourceManager->getGameObjectList().back()->getMaterials().push_back(sharedMaterial);

		auto projectile = std::make_unique<CProjectile>();
		projectile->setGameObject(m_pResourceManager->getGameObjectList().back().get());

		priest->GetBullets().push_back(std::move(projectile));
	}
}
void CRaytracingGameScene::CreateParticle(short job)
{
	std::vector<std::unique_ptr<CSkinningObject>>& skinned = m_pResourceManager->getSkinningObjectList();
	std::vector<std::unique_ptr<CTexture>>& textures = m_pResourceManager->getTextureList();
	std::vector<std::unique_ptr<CParticle>>& particles = m_pResourceManager->getParticleList();

	ComPtr<ID3D12PipelineState> OnePath;
	ComPtr<ID3D12PipelineState> TwoPath;
	switch (job) {
	case JOB_MAGE: {
		CreateOnePath(OnePath, "GS_M_Laser_OnePath");
		CreateTwoPath(TwoPath, "GS_M_Laser_TwoPath");

		particles.emplace_back(std::make_unique<CRaytracingParticle>());
		particles[0]->setOnePathPipeline(OnePath);
		particles[0]->setTwoPathPipeline(TwoPath);
		particles[0]->ParticleSetting(0.0f, 7.0f, XMFLOAT3(0.0, 1.0, 0.0));
		Material pmaterial{};
		pmaterial.m_bHasAlbedoColor = pmaterial.m_bHasAlbedoMap = true;
		pmaterial.m_xmf4AlbedoColor = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
		pmaterial.m_nAlbedoMapIndex = textures.size();
		textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Particle\\laser.dds"));
		particles[0]->setMaterial(pmaterial);
	}
		break;
	case JOB_WARRIOR: {
		CreateOnePath(OnePath, "GS_Boom_OnePath");
		CreateTwoPath(TwoPath, "GS_Boom_TwoPath");

		particles.emplace_back(std::make_unique<CRaytracingParticle>());
		particles[0]->setOnePathPipeline(OnePath);
		particles[0]->setTwoPathPipeline(TwoPath);
		particles[0]->ParticleSetting(0.0f, 3.0f, XMFLOAT3(0.0, 1.0, 0.0));
		Material pmaterial{};
		pmaterial.m_bHasAlbedoColor = pmaterial.m_bHasAlbedoMap = true;
		pmaterial.m_xmf4AlbedoColor = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
		pmaterial.m_nAlbedoMapIndex = textures.size();
		textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Particle\\Particle.dds"));
		particles[0]->setMaterial(pmaterial);
	}
		break;
	case JOB_HEALER: {
		CreateOnePath(OnePath, "GS_Buff_OnePath");
		CreateTwoPath(TwoPath, "GS_Buff_TwoPath");

		particles.emplace_back(std::make_unique<CRaytracingParticle>());
		particles[0]->setOnePathPipeline(OnePath);
		particles[0]->setTwoPathPipeline(TwoPath);
		particles[0]->ParticleSetting(0.0f, 3.0f);
		Material pmaterial{};
		pmaterial.m_bHasAlbedoColor = pmaterial.m_bHasAlbedoMap = true;
		pmaterial.m_xmf4AlbedoColor = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
		pmaterial.m_nAlbedoMapIndex = textures.size();
		textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Particle\\mana.dds"));
		particles[0]->setMaterial(pmaterial);
	}
		break;
	}

	CreateOnePath(OnePath, "GS_Buff_OnePath");
	CreateTwoPath(TwoPath, "GS_Buff_TwoPath");

	particles.emplace_back(std::make_unique<CRaytracingParticle>());
	particles[1]->setOnePathPipeline(OnePath);
	particles[1]->setTwoPathPipeline(TwoPath);
	particles[1]->ParticleSetting(0.0f, 3.0f);
	Material pmaterial{};
	pmaterial.m_bHasAlbedoColor = pmaterial.m_bHasAlbedoMap = true;
	pmaterial.m_xmf4AlbedoColor = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
	pmaterial.m_nAlbedoMapIndex = textures.size();
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Particle\\UI_Buff0.dds"));
	particles[1]->setMaterial(pmaterial);

	particles.emplace_back(std::make_unique<CRaytracingParticle>());
	particles[2]->setOnePathPipeline(OnePath);
	particles[2]->setTwoPathPipeline(TwoPath);
	particles[2]->ParticleSetting(0.0f, 3.0f);
	pmaterial.m_nAlbedoMapIndex = textures.size();
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Particle\\UI_Buff1.dds"));
	particles[2]->setMaterial(pmaterial);

	particles.emplace_back(std::make_unique<CRaytracingParticle>());
	particles[3]->setOnePathPipeline(OnePath);
	particles[3]->setTwoPathPipeline(TwoPath);
	particles[3]->ParticleSetting(0.0f, 3.0f);
	pmaterial.m_nAlbedoMapIndex = textures.size();
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Particle\\GreenCross.dds"));
	particles[3]->setMaterial(pmaterial);

	g_pBuff0 = particles[1].get();
	g_pBuff1 = particles[2].get();
	g_pBuff2 = particles[3].get();
}

void CRaytracingGameScene::CreateParticleRS()
{
	D3D12_ROOT_PARAMETER rp{};
	rp.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rp.Descriptor.ShaderRegister = 0;
	rp.Descriptor.RegisterSpace = 0;

	D3D12_ROOT_SIGNATURE_DESC desc{};
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT;
	desc.NumParameters = 1;
	desc.NumStaticSamplers = 0;
	desc.pParameters = &rp;
	desc.pStaticSamplers = nullptr;

	ID3DBlob* pBlob{};
	D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &pBlob, nullptr);
	g_DxResource.device->CreateRootSignature(0, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), IID_PPV_ARGS(m_ParticleRS.GetAddressOf()));
	pBlob->Release();
}

void CRaytracingGameScene::CreateOnePath(ComPtr<ID3D12PipelineState>& res, const char* entry)
{
	ID3DBlob* pd3dVBlob{ nullptr };
	ID3DBlob* pd3dGBlob{ nullptr };
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineState{};
	d3dPipelineState.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	d3dPipelineState.pRootSignature = m_ParticleRS.Get();

	D3D12_INPUT_ELEMENT_DESC ldesc[4]{};
	ldesc[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	ldesc[1] = { "DIRECTION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	ldesc[2] = { "LIFETIME", 0, DXGI_FORMAT_R32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	ldesc[3] = { "TYPE", 0, DXGI_FORMAT_R32_UINT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	d3dPipelineState.InputLayout.pInputElementDescs = ldesc;
	d3dPipelineState.InputLayout.NumElements = 4;

	d3dPipelineState.DepthStencilState.DepthEnable = FALSE;
	d3dPipelineState.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	d3dPipelineState.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	d3dPipelineState.DepthStencilState.StencilEnable = FALSE;

	d3dPipelineState.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	d3dPipelineState.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	d3dPipelineState.RasterizerState.AntialiasedLineEnable = FALSE;
	d3dPipelineState.RasterizerState.FrontCounterClockwise = FALSE;
	d3dPipelineState.RasterizerState.MultisampleEnable = FALSE;
	d3dPipelineState.RasterizerState.DepthClipEnable = FALSE;

	d3dPipelineState.BlendState.AlphaToCoverageEnable = FALSE;
	d3dPipelineState.BlendState.IndependentBlendEnable = FALSE;
	d3dPipelineState.BlendState.RenderTarget[0].BlendEnable = TRUE;
	d3dPipelineState.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	d3dPipelineState.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	d3dPipelineState.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	d3dPipelineState.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	d3dPipelineState.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	d3dPipelineState.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	d3dPipelineState.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
	d3dPipelineState.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	d3dPipelineState.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dPipelineState.NumRenderTargets = 0;
	d3dPipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	d3dPipelineState.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	d3dPipelineState.SampleDesc.Count = 1;
	d3dPipelineState.SampleMask = UINT_MAX;

	//ID3DBlob* errorb;
	D3DCompileFromFile(L"ParticleShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", 0, 0, &pd3dVBlob, nullptr);
	//OutputDebugStringA((char*)errorb->GetBufferPointer());
	d3dPipelineState.VS.BytecodeLength = pd3dVBlob->GetBufferSize();
	d3dPipelineState.VS.pShaderBytecode = pd3dVBlob->GetBufferPointer();

	D3DCompileFromFile(L"ParticleShader.hlsl", nullptr, nullptr, entry, "gs_5_1", 0, 0, &pd3dGBlob, nullptr);
	d3dPipelineState.GS.BytecodeLength = pd3dGBlob->GetBufferSize();
	d3dPipelineState.GS.pShaderBytecode = pd3dGBlob->GetBufferPointer();

	D3D12_SO_DECLARATION_ENTRY soEntry[4]{};
	soEntry[0] = { 0, "POSITION", 0, 0, 3, 0 };
	soEntry[1] = { 0, "DIRECTION", 0, 0, 3, 0 };
	soEntry[2] = { 0, "LIFETIME", 0, 0, 1, 0 };
	soEntry[3] = { 0, "TYPE", 0, 0, 1, 0 };

	UINT stride[1] = { sizeof(ParticleVertex) };

	d3dPipelineState.StreamOutput.NumEntries = 4;
	d3dPipelineState.StreamOutput.pSODeclaration = soEntry;
	d3dPipelineState.StreamOutput.NumStrides = 1;
	d3dPipelineState.StreamOutput.pBufferStrides = stride;
	d3dPipelineState.StreamOutput.RasterizedStream = D3D12_SO_NO_RASTERIZED_STREAM;

	g_DxResource.device->CreateGraphicsPipelineState(&d3dPipelineState, IID_PPV_ARGS(res.GetAddressOf()));

	if (pd3dVBlob)
		pd3dVBlob->Release();
	if (pd3dGBlob)
		pd3dGBlob->Release();
}
void CRaytracingGameScene::CreateTwoPath(ComPtr<ID3D12PipelineState>& res, const char* entry)
{
	ID3DBlob* pd3dVBlob{ nullptr };
	ID3DBlob* pd3dGBlob{ nullptr };
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineState{};
	d3dPipelineState.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	d3dPipelineState.pRootSignature = m_ParticleRS.Get();

	D3D12_INPUT_ELEMENT_DESC ldesc[4]{};
	ldesc[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	ldesc[1] = { "DIRECTION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	ldesc[2] = { "LIFETIME", 0, DXGI_FORMAT_R32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	ldesc[3] = { "TYPE", 0, DXGI_FORMAT_R32_UINT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	d3dPipelineState.InputLayout.pInputElementDescs = ldesc;
	d3dPipelineState.InputLayout.NumElements = 4;

	d3dPipelineState.DepthStencilState.DepthEnable = FALSE;
	d3dPipelineState.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	d3dPipelineState.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	d3dPipelineState.DepthStencilState.StencilEnable = FALSE;

	d3dPipelineState.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	d3dPipelineState.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	d3dPipelineState.RasterizerState.AntialiasedLineEnable = FALSE;
	d3dPipelineState.RasterizerState.FrontCounterClockwise = FALSE;
	d3dPipelineState.RasterizerState.MultisampleEnable = FALSE;
	d3dPipelineState.RasterizerState.DepthClipEnable = FALSE;

	d3dPipelineState.BlendState.AlphaToCoverageEnable = FALSE;
	d3dPipelineState.BlendState.IndependentBlendEnable = FALSE;
	d3dPipelineState.BlendState.RenderTarget[0].BlendEnable = TRUE;
	d3dPipelineState.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	d3dPipelineState.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	d3dPipelineState.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	d3dPipelineState.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	d3dPipelineState.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	d3dPipelineState.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	d3dPipelineState.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
	d3dPipelineState.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	d3dPipelineState.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dPipelineState.NumRenderTargets = 0;
	d3dPipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	d3dPipelineState.SampleDesc.Count = 1;
	d3dPipelineState.SampleMask = UINT_MAX;

	D3DCompileFromFile(L"ParticleShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", 0, 0, &pd3dVBlob, nullptr);
	d3dPipelineState.VS.BytecodeLength = pd3dVBlob->GetBufferSize();
	d3dPipelineState.VS.pShaderBytecode = pd3dVBlob->GetBufferPointer();

	D3DCompileFromFile(L"ParticleShader.hlsl", nullptr, nullptr, entry, "gs_5_1", 0, 0, &pd3dGBlob, nullptr);
	d3dPipelineState.GS.BytecodeLength = pd3dGBlob->GetBufferSize();
	d3dPipelineState.GS.pShaderBytecode = pd3dGBlob->GetBufferPointer();

	D3D12_SO_DECLARATION_ENTRY soEntry[3]{};
	soEntry[0] = { 0, "POSITION", 0, 0, 3, 0 };
	soEntry[1] = { 1, "TEXCOORD", 0, 0, 2, 1 };
	soEntry[2] = { 2, "COLOR", 0, 0, 4, 2 };

	UINT stride[3] = { sizeof(XMFLOAT3), sizeof(XMFLOAT2), sizeof(XMFLOAT4) };

	d3dPipelineState.StreamOutput.NumEntries = 3;
	d3dPipelineState.StreamOutput.pSODeclaration = soEntry;
	d3dPipelineState.StreamOutput.NumStrides = 3;
	d3dPipelineState.StreamOutput.pBufferStrides = stride;
	d3dPipelineState.StreamOutput.RasterizedStream = D3D12_SO_NO_RASTERIZED_STREAM;

	g_DxResource.device->CreateGraphicsPipelineState(&d3dPipelineState, IID_PPV_ARGS(res.GetAddressOf()));

	if (pd3dVBlob)
		pd3dVBlob->Release();
	if (pd3dGBlob)
		pd3dGBlob->Release();
}

void CRaytracingGameScene::PostProcess()
{
	m_pResourceManager->PostProcess();
}

void CRaytracingGameScene::PlayerUISetup(short job)
{
	size_t mindex{};
	size_t tindex{};
	size_t uindex{};

	std::vector<std::unique_ptr<CTexture>>& textures = m_pResourceManager->getTextureList();
	std::vector<std::unique_ptr<Mesh>>& meshes = m_pResourceManager->getMeshList();

	// status UI ===================================================================

	m_numUser = Players.size();
	m_local_id = Client.get_id();
	m_myJob = job;

	mindex = meshes.size();
	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(), 30, 30));		// buff icon
	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(), 340, 28));		// hp/mp bar
	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(), 330, 18));		// hp/mp

	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(), 264, 14.4));		// coop hp/mp 
	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(), 60, 60));		// coop player face

	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(), 520, 35));
	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(), 510, 25));

	tindex = textures.size();
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_HPbar.dds"));	// HPbar
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_MPbar.dds"));	// MPbar
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_MP.dds"));	// MP

	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_Buff0.dds"));	// buff0
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_Buff1.dds"));	// buff1
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_Buff2.dds"));	// buff2

	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_MiniPlayer0.dds"));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_MiniPlayer1.dds"));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_MiniPlayer2.dds"));

	for (auto& p : g_PlayerBuffState) p = false;

	int otherPlayer = 0;
	for (int i = 0; i < m_numUser; ++i) {
		if (i == m_local_id) {
			uindex = m_vPlayersStatUI[i].size();			// 0 - hpbar
			m_vPlayersStatUI[i].emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex + 1].get(), textures[tindex].get()));
			m_vPlayersStatUI[i][uindex]->setPositionInViewport(20, 20);

			uindex = m_vPlayersStatUI[i].size();			// 1 - hp
			m_vPlayersStatUI[i].emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex + 2].get()));
			m_vPlayersStatUI[i][uindex]->setColor(1.0, 0.0, 0.0, 1.0);
			m_vPlayersStatUI[i][uindex]->setPositionInViewport(25, 25);

			uindex = m_vPlayersStatUI[i].size();			// 2 - mp bar
			m_vPlayersStatUI[i].emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex + 1].get(), textures[tindex + 1].get()));
			m_vPlayersStatUI[i][uindex]->setPositionInViewport(20, 60);

			uindex = m_vPlayersStatUI[i].size();			// 2 - mp
			m_vPlayersStatUI[i].emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex + 2].get(), textures[tindex + 2].get()));
			m_vPlayersStatUI[i][uindex]->setPositionInViewport(25, 65);

			uindex = m_vPlayersStatUI[i].size();			// 3 ~ 5 buff
			m_vPlayersStatUI[i].emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex].get(), textures[tindex + 3].get()));
			m_vPlayersStatUI[i][uindex]->setPositionInViewport(20, 100);
			uindex = m_vPlayersStatUI[i].size();			// 3 ~ 5 buff
			m_vPlayersStatUI[i].emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex].get(), textures[tindex + 4].get()));
			m_vPlayersStatUI[i][uindex]->setPositionInViewport(20, 100);
			uindex = m_vPlayersStatUI[i].size();			// 3 ~ 5 buff
			m_vPlayersStatUI[i].emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex].get(), textures[tindex + 5].get()));
			m_vPlayersStatUI[i][uindex]->setPositionInViewport(20, 100);
		}
		else {
			uindex = m_vPlayersStatUI[i].size();			// 0 - hpbar
			m_vPlayersStatUI[i].emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex + 1].get(), textures[tindex].get()));
			m_vPlayersStatUI[i][uindex]->setScale(0.8);
			m_vPlayersStatUI[i][uindex]->setPositionInViewport(88, 100 + 50 + (otherPlayer * 80));

			uindex = m_vPlayersStatUI[i].size();			// 1 - hp
			m_vPlayersStatUI[i].emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex + 3].get()));
			m_vPlayersStatUI[i][uindex]->setColor(1.0, 0.0, 0.0, 1.0);
			m_vPlayersStatUI[i][uindex]->setPositionInViewport(92, 100 + 50 + (otherPlayer * 80) + 4);

			uindex = m_vPlayersStatUI[i].size();			// 2 - mp bar
			m_vPlayersStatUI[i].emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex + 1].get(), textures[tindex + 1].get()));
			m_vPlayersStatUI[i][uindex]->setScale(0.8);
			m_vPlayersStatUI[i][uindex]->setPositionInViewport(88, 138 + 50 + (otherPlayer * 80));

			uindex = m_vPlayersStatUI[i].size();			// 2 - mp
			m_vPlayersStatUI[i].emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex + 3].get(), textures[tindex + 2].get()));
			m_vPlayersStatUI[i][uindex]->setPositionInViewport(92, 138 + 50 + (otherPlayer * 80) + 4);

			uindex = m_vPlayersStatUI[i].size();
			m_vPlayersStatUI[i].emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex + 4].get(), textures[tindex + Players[i].getCharacterType() + 5].get()));
			m_vPlayersStatUI[i][uindex]->setPositionInViewport(20, 100 + 50 + (otherPlayer * 80));
			m_vPlayersStatUI[i][uindex]->setColor(1.0, 1.0, 1.0, 0.5);
			++otherPlayer;
		}
	}

	m_vBossUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex + 5].get(), textures[tindex].get()));
	m_vBossUIs[0]->setPositionInViewport(380, 10);

	m_vBossUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex + 6].get()));
	m_vBossUIs[1]->setColor(1.0, 0.0, 0.0, 1.0);
	m_vBossUIs[1]->setPositionInViewport(385, 15);
	// =============================================================================

	// item ========================================================================
	mindex = meshes.size();
	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(), 140, 175));

	tindex = textures.size();
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_Item0.dds"));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_Item1.dds"));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_Item2.dds"));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_Item3.dds"));

	for (int i = 0; i < 5; ++i) {
		if (i < 4) {
			uindex = m_vItemUIs.size();
			m_vItemUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex].get(), textures[tindex + i].get()));
			m_vItemUIs[uindex]->setPositionInViewport(20, 525);
			m_vItemUIs[uindex]->setRenderState(false);
		}
		else {
			uindex = m_vItemUIs.size();
			m_vItemUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex].get()));
			m_vItemUIs[uindex]->setColor(0.0, 0.0, 0.0, 0.5);
			m_vItemUIs[uindex]->setPositionInViewport(20, 525);
			m_vItemUIs[uindex]->setRenderState(true);
		}
	}
	m_vItemUIs[0]->setRenderState(true);

	// =============================================================================

	// skills ======================================================================

	mindex = meshes.size();
	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(), 100, 100));

	tindex = textures.size();
	switch (job) {
	case JOB_MAGE:
		textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_Skill_Magician0.dds"));
		textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_Skill_Magician1.dds"));
		textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_Skill_Magician2.dds"));
		g_SkillCoolTime[0] = 10.0f; g_SkillCoolTime[1] = 20.0f; g_SkillCoolTime[2] = 30.0f;
		g_SkillCost[0] = 25.0f; g_SkillCost[1] = 40.0f; g_SkillCost[2] = 70.0f;
		break;
	case JOB_WARRIOR:
		textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_Skill_Warrior0.dds"));
		textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_Skill_Warrior1.dds"));
		textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_Skill_Warrior2.dds"));
		g_SkillCoolTime[0] = 10.0f; g_SkillCoolTime[1] = 5.0f; g_SkillCoolTime[2] = 25.0f;
		g_SkillCost[0] = 30.0f; g_SkillCost[1] = 20.0f; g_SkillCost[2] = 40.0f;
		break;
	case JOB_HEALER:
		textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_Skill_Buffer0.dds"));
		textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_Skill_Buffer1.dds"));
		textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_Skill_Buffer2.dds"));
		g_SkillCoolTime[0] = 8.0f; g_SkillCoolTime[1] = 20.0f; g_SkillCoolTime[2] = 40.0f;
		g_SkillCost[0] = 30.0f; g_SkillCost[1] = 40.0f; g_SkillCost[2] = 60.0f;
		break;
	}
	for (auto& p : g_SkillCurCTime) p = 0;
	
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\UI\\InGame\\UI_Skill_MP_Less.dds"));
	for (int i = 0; i < 3; ++i) {
		uindex = m_vSkillUIs.size();
		m_vSkillUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex].get(), textures[tindex + i].get()));
		m_vSkillUIs[uindex]->setPositionInViewport(i * 110 + 940, 600);
	}

	for (int i = 0; i < 3; ++i) {
		uindex = m_vSkillUIs.size();
		m_vSkillUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex].get()));
		m_vSkillUIs[uindex]->setColor(0.0, 0.0, 0.0, 0.5);
		m_vSkillUIs[uindex]->setPositionInViewport(i * 110 + 940, 600);
	}

	for (int i = 0; i < 3; ++i) {
		uindex = m_vSkillUIs.size();
		m_vSkillUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[mindex].get(), textures[tindex + 3].get()));
		m_vSkillUIs[uindex]->setPositionInViewport(i * 110 + 940, 600);
	}
}

void CRaytracingGameScene::UIUseSkill(KeyInputRet input)
{
	if (m_myJob == JOB_HEALER) {
		switch (input) {
		case KEY_SKILL1:
			g_SkillCurCTime[0] = g_SkillCoolTime[0];
			Client.SendPriestBUFF(0);
			g_pSoundManager->StartFx(ESOUND::SOUND_SKILL_MAGIC1);
			break;
		case KEY_SKILL2:
			g_SkillCurCTime[1] = g_SkillCoolTime[1];
			Client.SendPriestBUFF(1);
			g_pSoundManager->StartFx(ESOUND::SOUND_SKILL_MAGIC2);
			break;
		case KEY_SKILL3:
			g_SkillCurCTime[2] = g_SkillCoolTime[2];
			Client.SendPriestBUFF(2);
			g_pSoundManager->StartFx(ESOUND::SOUND_SKILL_PRIEST3);
			// Send Skill use Packet
			break;
		}
	}
	else if(m_myJob == JOB_MAGE) {
		switch (input) {
		case KEY_SKILL1:
			g_SkillCurCTime[0] = g_SkillCoolTime[0];
			Client.SendPlayerAttack(-1, 1);
			g_pSoundManager->StartFx(ESOUND::SOUND_SKILL_MAGIC1);
			break;
		case KEY_SKILL2:
			g_SkillCurCTime[1] = g_SkillCoolTime[1];
			Client.SendPlayerAttack(-1, 2);
			g_pSoundManager->StartFx(ESOUND::SOUND_SKILL_MAGIC2);
			break;
		case KEY_SKILL3:
			g_SkillCurCTime[2] = g_SkillCoolTime[2];
			Client.SendPlayerAttack(-1, 3);
			g_pSoundManager->StartFx(ESOUND::SOUND_SKILL_LASER);
			break;
		}
	}
	else if (m_myJob == JOB_WARRIOR) {
		switch (input) {
		case KEY_SKILL1:
			g_SkillCurCTime[0] = g_SkillCoolTime[0];
			Client.SendPlayerAttack(-1, 1);
			g_pSoundManager->StartFx(ESOUND::SOUND_SLASH);
			break;
		case KEY_SKILL2:
			g_SkillCurCTime[1] = g_SkillCoolTime[1];
			Client.SendPlayerAttack(-1, 2);
			g_pSoundManager->StartFx(ESOUND::SOUND_WANDSWING);
			break;
		case KEY_SKILL3:
			g_SkillCurCTime[2] = g_SkillCoolTime[2];
			Client.SendPlayerAttack(-1, 3);
			g_pSoundManager->StartFx(ESOUND::SOUND_SHIELD_ATTACK);
			break;
		}
	}
}

void CRaytracingGameScene::SkillParticleStart(KeyInputRet input)
{
	switch (m_myJob) {
	case JOB_MAGE:
		if (input == KEY_SKILL3)
			m_pResourceManager->getParticleList()[0]->Start();
		break;
	case JOB_WARRIOR:
		if (input == KEY_SKILL3)
			m_pResourceManager->getParticleList()[0]->Start();
		break;
	case JOB_HEALER:
		if (input == KEY_SKILL3)
			m_pResourceManager->getParticleList()[0]->Start();
		break;
	}
}

// =====================================================================================

void CRaytracingWinterLandScene::SetUp(ComPtr<ID3D12Resource>& outputBuffer, std::shared_ptr<CRayTracingPipeline> pipeline)
{
	g_InGameState = IS_LOADING;
	wOpacity = 1.0f;
	m_vMonsters.clear();
	m_vPlayers.clear();
	Monsters.clear();

	m_pOutputBuffer = outputBuffer;
	// CreateUISetup
	CreateOrthoMatrixBuffer();
	CreateRTVDSV();
	CreateUIRootSignature();
	CreateUIPipelineState();

	// ParticleRS
	CreateParticleRS();

	// animation Pipeline Ready
	CreateComputeRootSignature();
	CreateComputeShader();

	// Create And Set up PipelineState
	m_pRaytracingPipeline = pipeline;

	// Resource Ready
	m_pResourceManager = std::make_unique<CResourceManager>();
	m_pResourceManager->SetUp(3);

	std::vector<std::unique_ptr<CGameObject>>& normalObjects = m_pResourceManager->getGameObjectList();
	std::vector<std::unique_ptr<CSkinningObject>>& skinned = m_pResourceManager->getSkinningObjectList();
	std::vector<std::unique_ptr<Mesh>>& meshes = m_pResourceManager->getMeshList();
	std::vector<std::unique_ptr<CTexture>>& textures = m_pResourceManager->getTextureList();
	std::vector<std::unique_ptr<CAnimationManager>>& aManagers = m_pResourceManager->getAnimationManagers();

	// Object File Read ======================================================================
	m_pResourceManager->AddResourceFromFile(L"src\\model\\Map\\WinterLand\\WinterLand_Final.bin", "src\\texture\\Map\\");

	// Players Create ========================================================================
	for (int i = 0; i < Players.size(); ++i) {
		// player job check
		// short player_job = Players[i].getJobInfo();
		short player_job = Players[i].getCharacterType();
		switch (player_job) {
		case JOB_MAGE:
			CreateMageCharacter();
			break;
		case JOB_WARRIOR:
			CreateWarriorCharacter();
			break;
		case JOB_HEALER:
			CreatePriestCharacter();
			break;
		}
		Players[i].setRenderingObject(skinned[skinned.size() - 1].get());
		Players[i].setAnimationManager(aManagers[aManagers.size() - 1].get());
		if (i == Client.get_id()) {
			m_pPlayer = std::make_unique<CPlayer>(m_vPlayers[m_vPlayers.size() - 1].get(), m_pCamera);
			CreateParticle(player_job);
		}
	}

	Create_Gorhorrid();

	// Light Read
	m_pResourceManager->AddLightsFromFile(L"src\\Light\\WinterLand_Light_Final.bin");
	m_pResourceManager->ReadyLightBufferContent();
	m_pResourceManager->WinterLand_LightSetup();
	// =========================================================

	UINT finalindex = normalObjects.size();
	UINT finalmesh = meshes.size();

	// terrian
	m_pHeightMap = std::make_unique<CHeightMapImage>(L"src\\model\\Map\\WinterLand\\Terrain_Final.raw", 2049, 2049, XMFLOAT3(1.0f, 0.0312f, 1.0f));
	meshes.emplace_back(std::make_unique<Mesh>(m_pHeightMap.get(), "terrain"));
	normalObjects.emplace_back(std::make_unique<CGameObject>());
	normalObjects[normalObjects.size() - 1]->SetMeshIndex(meshes.size() - 1);

	normalObjects[normalObjects.size() - 1]->SetInstanceID(10);
	normalObjects[normalObjects.size() - 1]->getMaterials().emplace_back();
	normalObjects[normalObjects.size() - 1]->SetPosition(XMFLOAT3(-1024.0, 0.0, -1024.0));
	m_pRoadTerrain = std::make_unique<CHeightMapImage>(L"src\\model\\Map\\WinterLand\\Terrain_Road.raw", 2049, 2049, XMFLOAT3(1.0f, 0.0312f, 1.0f));
	m_pCollisionHMap = std::make_unique<CHeightMapImage>(L"src\\model\\Map\\WinterLand\\Terrain_Collision.raw", 2049, 2049, XMFLOAT3(1.0f, 0.0312f, 1.0f));


	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Map\\FrozenWater02_NORM.dds"));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Map\\FrozenWater02_MNS.dds"));
	auto p = std::find_if(normalObjects.begin(), normalObjects.end(), [](std::unique_ptr<CGameObject>& p) {
		return p->getFrameName() == "Water";
		});
	if (p != normalObjects.end()) {
		(*p)->SetInstanceID(2);
		(*p)->getMaterials().emplace_back();
		Material& mt = (*p)->getMaterials()[0];
		mt.m_bHasAlbedoColor = true; mt.m_xmf4AlbedoColor = XMFLOAT4(0.1613118, 0.2065666, 0.2358491, 0.2);
		mt.m_bHasMetallicMap = true; mt.m_nMetallicMapIndex = textures.size() - 1;
		mt.m_bHasNormalMap = true; mt.m_nNormalMapIndex = textures.size() - 2;

		void* tempptr{};
		std::vector<XMFLOAT2> tex0 = meshes[(*p)->getMeshIndex()]->getTex0();
		for (XMFLOAT2& xmf : tex0) {
			xmf.x *= 10.0f; xmf.y *= 10.0f;
		}
		meshes[(*p)->getMeshIndex()]->getTexCoord0Buffer()->Map(0, nullptr, &tempptr);
		memcpy(tempptr, tex0.data(), sizeof(XMFLOAT2) * tex0.size());
		meshes[(*p)->getMeshIndex()]->getTexCoord0Buffer()->Unmap(0, nullptr);
	}

	PrepareTerrainTexture();

	// cubeMap Ready
	m_nSkyboxIndex = textures.size();
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\WinterLandSky2.dds", true));
	// ===========================================================================================
	m_pResourceManager->InitializeGameObjectCBuffer();
	m_pResourceManager->PrepareObject();	// Ready OutputBuffer to  SkinningObject


	// ShaderBindingTable
	m_pShaderBindingTable = std::make_unique<CShaderBindingTableManager>();
	m_pShaderBindingTable->Setup(m_pRaytracingPipeline.get(), m_pResourceManager.get());
	m_pShaderBindingTable->CreateSBT();

	// Copy(normalObject) & SetPreMatrix ===============================

	for (int i = 0; i < Players.size(); ++i) {
		skinned[i]->SetPosition(XMFLOAT3(-72.5f, 0.0f, -987.8f));
		//skinned[i]->SetPosition(XMFLOAT3(0.0, 0.0f, 0.0));
	}

	// ==============================================================================

	// Camera Setting ==============================================================
	m_pCamera->SetTarget(m_pPlayer->getObject()->getObject()->getObjects()[0].get());
	m_pCamera->SetHOffset(3.5f);
	m_pCamera->SetCameraLength(15.0f);
	m_pCamera->SetMapNumber(SCENE_WINTERLAND);
	// ==========================================================================

	// AccelerationStructure
	m_pAccelerationStructureManager = std::make_unique<CAccelerationStructureManager>();
	m_pAccelerationStructureManager->Setup(m_pResourceManager.get(), 1);
	m_pAccelerationStructureManager->InitBLAS();
	m_pAccelerationStructureManager->InitTLAS();

	// UISetup ========================================================================
	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0, 0.0, 0.0), DEFINED_UAV_BUFFER_WIDTH, DEFINED_UAV_BUFFER_HEIGHT));
	m_vUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get()));
	m_vUIs[m_vUIs.size() - 1]->setPositionInViewport(0, 0);
	m_vUIs[m_vUIs.size() - 1]->setColor(0.0, 0.0, 0.0, 1.0);

	m_vUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get()));
	m_vUIs[m_vUIs.size() - 1]->setPositionInViewport(0, 0);
	m_vUIs[m_vUIs.size() - 1]->setColor(0.5, 0.5, 0.5, 0.0);

	PlayerUISetup(Players[Client.get_id()].getCharacterType());

	g_pSoundManager->StartAMB(ESOUND::SOUND_STAGE3_AMB);

	Client.SendPlayerReady(SCENE_WINTERLAND);
}

void CRaytracingWinterLandScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	switch (nMessage) {
	case WM_KEYDOWN:
		switch (wParam) {
		case 'N':
			m_pCamera->toggleNormalMapping();
			break;
		case 'M':
			m_pCamera->toggleAlbedoColor();
			break;
		case 'B':
			m_pCamera->toggleReflection();
			break;
		case '9':
			m_pCamera->SetThirdPersonMode(false);
			break;
		case '0':
			m_pCamera->SetThirdPersonMode(true);
			break;
		case '7':
			Client.SendMasterKey();
			break;
		case 'Z':
			m_vItemUIs[cItem]->setRenderState(false);
			--cItem;
			if (cItem < 0) cItem = 3;
			m_vItemUIs[cItem]->setRenderState(true);
			break;
		case 'C':
			m_vItemUIs[cItem]->setRenderState(false);
			++cItem;
			if (cItem > 3) cItem = 0;
			m_vItemUIs[cItem]->setRenderState(true);
			break;
		case 'X':
			if (m_fItemCurTime <= 0.0f && !g_PlayerDie[m_local_id]) {
				Client.SendUseItem(cItem);
				m_fItemCurTime = m_fItemCoolTime;
			}
			break;
		case 'P':
			m_bUIOnOff = !m_bUIOnOff;
			break;
		}
		break;
	case WM_KEYUP:
		break;
	}
}

void CRaytracingWinterLandScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	if (g_InGameState == IS_GAMING) {
		m_pPlayer->MouseProcess(hWnd, nMessage, wParam, lParam);
		switch (nMessage) {
		case WM_MOUSEWHEEL:
			short delta = GET_WHEEL_DELTA_WPARAM(wParam);
			if (delta > 0)
				m_pCamera->ChangeLength(1);
			else if (delta < 0)
				m_pCamera->ChangeLength(0);
			break;
		}
	}
}

void CRaytracingWinterLandScene::Create_Gorhorrid()
{
	m_nMonsterNum = m_vMonsters.size();

	m_pResourceManager->AddSkinningResourceFromFile(L"src\\model\\Gorhorrid.bin", "src\\texture\\Gorhorrid\\", MONSTER);
	m_vMonsters.emplace_back(std::make_unique<Gorhorrid>(
		m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 1].get(),
		m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get()));

	for (auto& o : m_pResourceManager->getSkinningObjectList().back()->getObjects()) {	// emissive map
		for (auto& ma : o->getMaterials())
			ma.m_bHasEmissiveColor = false;
	}

	m_vMonsters[m_nMonsterNum]->getObject()->setPreTransform(5.0f, XMFLOAT3(), XMFLOAT3());
	m_vMonsters[m_nMonsterNum]->getObject()->SetPosition(XMFLOAT3(-86.3f, 0.0f, -301.1f));
	//m_vMonsters[m_nMonsterNum]->getObject()->Rotate(XMFLOAT3(0.0f, 0.0f, 0.0f));

	m_pMonsters.emplace_back(std::make_unique<CMonster>(m_vMonsters[m_vMonsters.size() - 1].get()));


	// server object add ==============================================
	auto newMonster = std::make_unique<Monster>(m_nMonsterNum, MonsterType::Gorhorrid);
	newMonster->setRenderingObject(m_vMonsters.back()->getObject());
	newMonster->setAnimationManager(m_vMonsters.back()->getAniManager());
	Monsters[m_nMonsterNum] = std::move(newMonster);
	// =================================================================

	for (auto& s : m_vMonsters[m_nMonsterNum]->getObject()->getObjects())
	{
		if (s->getFrameName() == "Gorhorrid_Tongue_8")
		{
			m_vMonsters[m_nMonsterNum]->SetHead(s.get());
			break;
		}
	}

	m_pResourceManager->getMeshList().emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0f, 0.0f, 0.0f), 1.0f, "sphere"));
	size_t meshIndex = m_pResourceManager->getMeshList().size() - 1;
	Gorhorrid* monster = dynamic_cast<Gorhorrid*>(m_vMonsters.back().get());
	Material sharedMaterial;

	for (int i = 0; i < 15; ++i) {
		m_pResourceManager->getGameObjectList().emplace_back(std::make_unique<CGameObject>());
		m_pResourceManager->getGameObjectList().back()->SetMeshIndex(meshIndex);
		m_pResourceManager->getGameObjectList().back()->getMaterials().push_back(sharedMaterial);

		auto projectile = std::make_unique<CProjectile>();
		projectile->setGameObject(m_pResourceManager->getGameObjectList().back().get());

		monster->GetBullets().push_back(std::move(projectile));
	}
}

void CRaytracingWinterLandScene::ProcessInput(float fElapsedTime)
{
	UCHAR keyBuffer[256];
	GetKeyboardState(keyBuffer);

	if (g_InGameState == IS_GAMING) {
		if (!m_pCamera->getThirdPersonState()) {
			bool shiftDown = false;
			if (keyBuffer[VK_SHIFT] & 0x80)
				shiftDown = true;
			if (keyBuffer['W'] & 0x80)
				m_pCamera->Move(0, fElapsedTime, shiftDown);
			if (keyBuffer['S'] & 0x80)
				m_pCamera->Move(5, fElapsedTime, shiftDown);
			if (keyBuffer['D'] & 0x80)
				m_pCamera->Move(3, fElapsedTime, shiftDown);
			if (keyBuffer['A'] & 0x80)
				m_pCamera->Move(4, fElapsedTime, shiftDown);
			if (keyBuffer[VK_SPACE] & 0x80)
				m_pCamera->Move(1, fElapsedTime, shiftDown);
			if (keyBuffer[VK_CONTROL] & 0x80)
				m_pCamera->Move(2, fElapsedTime, shiftDown);
		}
		else {
			if (!g_PlayerDie[m_local_id]) {
				KeyInputRet ret = m_pPlayer->ProcessInput(keyBuffer, fElapsedTime);
				UIUseSkill(ret);
				SkillParticleStart(ret);
				CAnimationManager* myManager = m_pPlayer->getAniManager();
				Client.SendMovePacket(myManager->getElapsedTime(), myManager->getCurrentSet());	// Check
			}
		}
	}
}

void CRaytracingWinterLandScene::PrepareTerrainTexture()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NumDescriptors = 14;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	g_DxResource.device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_pTerrainDescriptor.GetAddressOf()));

	struct alignas(16) terrainINFO {
		int numLayer{};
		float padding[3]{};
		int bHasDiffuse[4]{};
		int bHasNormal[4]{};
		int bHasMask[4]{};
	};

	auto rdesc = BASIC_BUFFER_DESC;
	rdesc.Width = Align(sizeof(terrainINFO), 256);

	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &rdesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(m_pTerrainCB.GetAddressOf()));

	terrainINFO* pMap{};
	m_pTerrainCB->Map(0, nullptr, reinterpret_cast<void**>(&pMap));
	pMap->numLayer = 4;
	pMap->bHasDiffuse[0] = pMap->bHasDiffuse[1] = pMap->bHasDiffuse[2] = pMap->bHasDiffuse[3] = 1;
	pMap->bHasNormal[0] = pMap->bHasNormal[1] = pMap->bHasNormal[2] = pMap->bHasNormal[3] = 0;
	pMap->bHasMask[0] = pMap->bHasMask[2] = pMap->bHasMask[3] = 0;
	m_pTerrainCB->Unmap(0, nullptr);

	std::vector<std::unique_ptr<CTexture>>& textures = m_pResourceManager->getTextureList();
	size_t textureIndex = textures.size();
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Terrain_WinterLands_splatmap.dds"));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Map\\FrozenWater02.dds"));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Map\\RockStalagmites00_terrain2.dds"));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Map\\Stonerock03_Metallic.dds"));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Map\\SnowGround00_Albedo.dds"));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	D3D12_RESOURCE_DESC d3dRD;

	UINT increment = g_DxResource.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE  handle = m_pTerrainDescriptor->GetCPUDescriptorHandleForHeapStart();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cdesc{};
	cdesc.BufferLocation = m_pTerrainCB->GetGPUVirtualAddress();
	cdesc.SizeInBytes = rdesc.Width;

	g_DxResource.device->CreateConstantBufferView(&cdesc, handle);
	handle.ptr += increment;

	// splat
	d3dRD = textures[textureIndex]->getTexture()->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;

	g_DxResource.device->CreateShaderResourceView(textures[textureIndex]->getTexture(), &srvDesc, handle);
	handle.ptr += increment;

	// layer 0 ===============================================================

	d3dRD = textures[textureIndex + 1]->getTexture()->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(textures[textureIndex + 1]->getTexture(), &srvDesc, handle);
	handle.ptr += increment;

	d3dRD = g_DxResource.nullTexture->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(g_DxResource.nullTexture.Get(), &srvDesc, handle);
	handle.ptr += increment;

	d3dRD = g_DxResource.nullTexture->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(g_DxResource.nullTexture.Get(), &srvDesc, handle);
	handle.ptr += increment;

	// layer 1 ===============================================================

	d3dRD = textures[textureIndex + 2]->getTexture()->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(textures[textureIndex + 2]->getTexture(), &srvDesc, handle);
	handle.ptr += increment;

	d3dRD = g_DxResource.nullTexture->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(g_DxResource.nullTexture.Get(), &srvDesc, handle);
	handle.ptr += increment;

	d3dRD = g_DxResource.nullTexture->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(g_DxResource.nullTexture.Get(), &srvDesc, handle);
	handle.ptr += increment;

	// layer 2 ===============================================================

	d3dRD = textures[textureIndex + 3]->getTexture()->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(textures[textureIndex + 3]->getTexture(), &srvDesc, handle);
	handle.ptr += increment;

	d3dRD = g_DxResource.nullTexture->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(g_DxResource.nullTexture.Get(), &srvDesc, handle);
	handle.ptr += increment;

	d3dRD = g_DxResource.nullTexture->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(g_DxResource.nullTexture.Get(), &srvDesc, handle);
	handle.ptr += increment;

	// layer 3 ===============================================================

	d3dRD = textures[textureIndex + 4]->getTexture()->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(textures[textureIndex + 4]->getTexture(), &srvDesc, handle);
	handle.ptr += increment;

	d3dRD = g_DxResource.nullTexture->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(g_DxResource.nullTexture.Get(), &srvDesc, handle);
	handle.ptr += increment;

	d3dRD = g_DxResource.nullTexture->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(g_DxResource.nullTexture.Get(), &srvDesc, handle);
	handle.ptr += increment;
}

void CRaytracingWinterLandScene::UpdateObject(float fElapsedTime)
{
	// compute shader & rootSignature set
	g_DxResource.cmdList->SetPipelineState(m_pAnimationComputeShader.Get());
	g_DxResource.cmdList->SetComputeRootSignature(m_pComputeRootSignature.Get());

	m_pResourceManager->UpdateSkinningMesh(fElapsedTime);
	Flush();
	// Skinning Object BLAS ReBuild
	m_pResourceManager->ReBuildBLAS();

	m_pResourceManager->UpdateWorldMatrix();

	for (auto& m : m_vPlayers)
	{
		m->UpdateObject(fElapsedTime);

		if (m->CheckAC())
		{
			AttackCollision(m_vMonsters, m_vPlayers, 1);
		}
		if (m->HasActiveBullet())
		{
			ShootCollision(m_vMonsters, m_vPlayers, 1);
		}
	}

	for (auto& m : m_vMonsters)
	{
		m->UpdateObject(fElapsedTime);

		if (m->CheckAC())
		{
			AttackCollision(m_vPlayers, m_vMonsters, 0);
		}
		if (m->HasActiveBullet())
		{
			ShootCollision(m_vPlayers, m_vMonsters, 0);
		}
	}

	m_pPlayer->CollisionCheck(m_pRoadTerrain.get(), m_pCollisionHMap.get(), fElapsedTime, -1024.0f, 0.0f, -1024.0f, SCENE_WINTERLAND);
	m_pPlayer->HeightCheck(m_pRoadTerrain.get(), fElapsedTime, -1024.0f, 0.0f, -1024.0f, SCENE_WINTERLAND);

	BulletCheck(m_vPlayers, m_pRoadTerrain.get(), m_pCollisionHMap.get(), fElapsedTime, -1024.0f, 0.0f, -1024.0f, SCENE_WINTERLAND);

	for (auto& p : m_pMonsters) {
		p->CollisionCheck(m_pRoadTerrain.get(), m_pCollisionHMap.get(), fElapsedTime, -1024.0f, 0.0f, -1024.0f, SCENE_WINTERLAND);
		p->HeightCheck(m_pRoadTerrain.get(), fElapsedTime, -1024.0f, 0.0f, -1024.0f, SCENE_WINTERLAND);
	}

	BulletCheck(m_vMonsters, m_pRoadTerrain.get(), m_pCollisionHMap.get(), fElapsedTime, -1024.0f, 0.0f, -1024.0f, SCENE_WINTERLAND);

	AutoDirection(m_vPlayers, m_vMonsters);
	
	if (m_pCamera->getThirdPersonState()) {
		XMFLOAT3& EYE = m_pCamera->getEyeCalculateOffset();
		float cHeight = m_pRoadTerrain->GetHeightinWorldSpace(EYE.x + 1024.0f, EYE.z + 1024.0f);
		if (EYE.z >= -500.0f) {
			if (cHeight < 10.5f)
				cHeight = 10.5f;
		}
		if (EYE.y < cHeight + 0.5f) {
			m_pCamera->UpdateViewMatrix(cHeight + 0.5f);
		}
		else
			m_pCamera->UpdateViewMatrix();
	}
	else
		m_pCamera->UpdateViewMatrix();

	g_DxResource.cmdList->SetGraphicsRootSignature(m_ParticleRS.Get());
	m_pCamera->SetElapsedTimeAndShader(fElapsedTime, 0);
	m_pResourceManager->UpdateParticles(fElapsedTime);

	m_pAccelerationStructureManager->UpdateScene(m_pCamera->getEye());

	// Player UI ==================================================
	int buffstart = 20; int bstride = 40;
	float cMP = Players[m_local_id].GetMP();

	for (int i = 0; i < m_numUser; ++i) {
		int t{};
		// hp/mp
		m_vPlayersStatUI[i][1]->setScaleX(Players[i].GetHP() / g_maxHPs[i]);
		m_vPlayersStatUI[i][3]->setScaleXWithUV(Players[i].GetMP() / g_maxMPs[i]);
		if (i == m_local_id) {
			for (int j = 0; j < 3; ++j) {
				if (g_PlayerBuffState[j]) {
					m_vPlayersStatUI[i][j + 4]->setRenderState(true);
					m_vPlayersStatUI[i][j + 4]->setPositionInViewport(buffstart + (t * bstride), 100);
					++t;
				}
				else {
					m_vPlayersStatUI[i][j + 4]->setRenderState(false);
				}
			}
			m_vBossUIs[1]->setScaleX(Monsters[0]->getHP() / Monsters[0]->getMaxHP());
			XMFLOAT3 mpos = m_pMonsters[0]->getObject()->getObject()->getPosition(); mpos.y = 0;
			XMFLOAT3 ppos = Players[m_local_id].getRenderingObject()->getPosition(); ppos.y = 0;
			float distance;
			XMStoreFloat(&distance, XMVector3Length(XMLoadFloat3(&mpos) - XMLoadFloat3(&ppos)));
			if (distance <= 100.0f) {
				m_bBossBattle = true;
				if (m_bPrevBossBattle != m_bBossBattle) {
					g_pSoundManager->StartBGM(ESOUND::SOUND_STAGE3_BOSS);
				}
			}
			else {
				m_bBossBattle = false;
				if (m_bPrevBossBattle != m_bBossBattle) {
					g_pSoundManager->StopBGM();
				}
			}
			m_bPrevBossBattle = m_bBossBattle;
		}
	}
	m_fItemCurTime -= fElapsedTime;
	if (m_fItemCurTime < 0.0)
		m_fItemCurTime = 0.0f;
	m_vItemUIs[4]->setScaleY(m_fItemCurTime / m_fItemCoolTime);

	{
		if (cMP < g_SkillCost[0])
			m_vSkillUIs[6]->setRenderState(true);
		else
			m_vSkillUIs[6]->setRenderState(false);

		if (cMP < g_SkillCost[1])
			m_vSkillUIs[7]->setRenderState(true);
		else
			m_vSkillUIs[7]->setRenderState(false);

		if (cMP < g_SkillCost[2])
			m_vSkillUIs[8]->setRenderState(true);
		else
			m_vSkillUIs[8]->setRenderState(false);

		for (int i = 0; i < 3; ++i) {
			g_SkillCurCTime[i] -= fElapsedTime;
			if (g_SkillCurCTime[i] < 0) g_SkillCurCTime[i] = 0.0f;
			m_vSkillUIs[i + 3]->setScaleY(g_SkillCurCTime[i] / g_SkillCoolTime[i]);
		}
	}
	// =================================================================

	if (g_PlayerDie[m_local_id])
		m_vUIs.back()->setColor(0.5, 0.5, 0.5, 0.5);
	else
		m_vUIs.back()->setColor(0.5, 0.5, 0.5, 0.0);

	switch (g_InGameState) {
	case IS_LOADING: {
		wOpacity -= 0.5 * fElapsedTime;
		if (wOpacity < 0.0f) {
			g_InGameState = IS_GAMING;
			wOpacity = 0.0f;
		}
		m_vUIs[0]->setColor(0.0, 0.0, 0.0, wOpacity);
		break;
	}
	case IS_GAMING: {
		break;
	}
	case IS_FINISH: {
		wOpacity += 0.2 * fElapsedTime;
		if (wOpacity > 1.0f) {
			ShowCursor(TRUE);
			g_pSoundManager->AllStop();
			m_nNextScene = SCENE_TITLE;
			Players.clear();
			Client.set_id(-1);
			g_state = Title;
			wOpacity = 1.0f;
		}
		m_vUIs[0]->setColor(0.0, 0.0, 0.0, wOpacity);
		break;
	}
	}
}

void CRaytracingWinterLandScene::Render()
{
	m_pCamera->SetShaderVariable();
	m_pAccelerationStructureManager->SetScene();
	m_pResourceManager->SetLights();
	std::vector<std::unique_ptr<CTexture>>& textures = m_pResourceManager->getTextureList();
	g_DxResource.cmdList->SetComputeRootDescriptorTable(4, textures[m_nSkyboxIndex]->getView()->GetGPUDescriptorHandleForHeapStart());
	g_DxResource.cmdList->SetComputeRootDescriptorTable(5, m_pTerrainDescriptor->GetGPUDescriptorHandleForHeapStart());

	D3D12_DISPATCH_RAYS_DESC raydesc{};
	raydesc.Depth = 1;
	raydesc.Width = DEFINED_UAV_BUFFER_WIDTH;
	raydesc.Height = DEFINED_UAV_BUFFER_HEIGHT;

	raydesc.RayGenerationShaderRecord.StartAddress = m_pShaderBindingTable->getRayGenTable()->GetGPUVirtualAddress();
	raydesc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	raydesc.MissShaderTable.StartAddress = m_pShaderBindingTable->getMissTable()->GetGPUVirtualAddress();
	raydesc.MissShaderTable.SizeInBytes = m_pShaderBindingTable->getMissSize();
	raydesc.MissShaderTable.StrideInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

	raydesc.HitGroupTable.StartAddress = m_pShaderBindingTable->getHitGroupTable()->GetGPUVirtualAddress();
	raydesc.HitGroupTable.SizeInBytes = m_pShaderBindingTable->getHitGroupSize();
	raydesc.HitGroupTable.StrideInBytes = m_pShaderBindingTable->getHitGroupStride();

	g_DxResource.cmdList->DispatchRays(&raydesc);

	// UI Render ==================================================================================

	ID3D12GraphicsCommandList4* cmdList = g_DxResource.cmdList;
	auto barrier = [&](ID3D12Resource* pResource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
		{
			D3D12_RESOURCE_BARRIER resBarrier{};
			resBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			resBarrier.Transition.pResource = pResource;
			resBarrier.Transition.StateBefore = before;
			resBarrier.Transition.StateAfter = after;
			resBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			cmdList->ResourceBarrier(1, &resBarrier);
		};

	barrier(m_pOutputBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_VIEWPORT vv{};
	vv.Width = DEFINED_UAV_BUFFER_WIDTH; vv.Height = DEFINED_UAV_BUFFER_HEIGHT; vv.MinDepth = 0.0f; vv.MaxDepth = 1.0f;
	cmdList->RSSetViewports(1, &vv);
	D3D12_RECT ss{ 0, 0, DEFINED_UAV_BUFFER_WIDTH, DEFINED_UAV_BUFFER_HEIGHT };
	cmdList->RSSetScissorRects(1, &ss);
	cmdList->OMSetRenderTargets(1, &m_RTV->GetCPUDescriptorHandleForHeapStart(), FALSE, &m_DSV->GetCPUDescriptorHandleForHeapStart());
	cmdList->SetGraphicsRootSignature(m_UIRootSignature.Get());
	cmdList->SetPipelineState(m_UIPipelineState.Get());
	cmdList->SetGraphicsRootConstantBufferView(0, m_cameraCB->GetGPUVirtualAddress());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// player UI ====================================
	if (m_bUIOnOff) {
		for (int i = 0; i < m_numUser; ++i) {
			for (auto& p : m_vPlayersStatUI[i])
				p->Render();
		}

		if (m_bBossBattle)
			for (auto& p : m_vBossUIs)
				p->Render();

		for (auto& p : m_vItemUIs)
			p->Render();

		for (auto& p : m_vSkillUIs)
			p->Render();
	}
	// ===============================================

	for (auto& p : m_vUIs)	// black plane
		p->Render();

	barrier(m_pOutputBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

// =================================================================

void CRaytracingCaveScene::SetUp(ComPtr<ID3D12Resource>& outputBuffer, std::shared_ptr<CRayTracingPipeline> pipeline)
{
	g_InGameState = IS_LOADING;
	wOpacity = 1.0f;
	m_vMonsters.clear();
	m_vPlayers.clear();
	Monsters.clear();

	m_pOutputBuffer = outputBuffer;
	// CreateUISetup
	CreateOrthoMatrixBuffer();
	CreateRTVDSV();
	CreateUIRootSignature();
	CreateUIPipelineState();

	// ParticleRS
	CreateParticleRS();

	// animation Pipeline Ready
	CreateComputeRootSignature();
	CreateComputeShader();

	// Create And Set up PipelineState
	m_pRaytracingPipeline = pipeline;

	// Resource Ready
	m_pResourceManager = std::make_unique<CResourceManager>();
	m_pResourceManager->SetUp(3);

	std::vector<std::unique_ptr<CGameObject>>& normalObjects = m_pResourceManager->getGameObjectList();
	std::vector<std::unique_ptr<CSkinningObject>>& skinned = m_pResourceManager->getSkinningObjectList();
	std::vector<std::unique_ptr<Mesh>>& meshes = m_pResourceManager->getMeshList();
	std::vector<std::unique_ptr<CTexture>>& textures = m_pResourceManager->getTextureList();
	std::vector<std::unique_ptr<CAnimationManager>>& aManagers = m_pResourceManager->getAnimationManagers();

	// Object File Read ========================================	! !
	m_pResourceManager->AddResourceFromFile(L"src\\model\\Map\\Cave\\Cave.bin", "src\\texture\\Map\\");
	
	// Players Create ========================================================================
	for (int i = 0; i < Players.size(); ++i) {
		// player job check
		// short player_job = Players[i].getJobInfo();
		short player_job = Players[i].getCharacterType();
		switch (player_job) {
		case JOB_MAGE:
			CreateMageCharacter();
			break;
		case JOB_WARRIOR:
			CreateWarriorCharacter();
			break;
		case JOB_HEALER:
			CreatePriestCharacter();
			break;
		}
		Players[i].setRenderingObject(skinned[skinned.size() - 1].get());
		Players[i].setAnimationManager(aManagers[aManagers.size() - 1].get());
		if (i == Client.get_id()) {
			m_pPlayer = std::make_unique<CPlayer>(m_vPlayers[m_vPlayers.size() - 1].get(), m_pCamera);
			CreateParticle(player_job);
		}
	}

	Create_Limadon();
	Create_Fulgurodonte();
	Create_Occisodonte();
	Create_Crassorrid();

	m_pResourceManager->AddLightsFromFile(L"src\\Light\\Light_Cave.bin");
	m_pResourceManager->ReadyLightBufferContent();
	// =========================================================

	UINT finalindex = normalObjects.size();
	UINT finalmesh = meshes.size();

	// terrian
	m_pHeightMap = std::make_unique<CHeightMapImage>(L"src\\model\\Map\\Cave\\CaveHeightMap.raw", 512, 512, XMFLOAT3(500.0f / 512.0f, 0.0092f, 500.0f / 512.0f));
	m_pCollisionHMap = std::make_unique<CHeightMapImage>(L"src\\model\\Map\\Cave\\CaveCollisionHMap.raw", 512, 512, XMFLOAT3(500.0f / 512.0f, 1.0f, 500.0f / 512.0f));

	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Map\\dlnk_Water_01_nrm.dds"));
	std::for_each(normalObjects.begin(), normalObjects.end(), [&](std::unique_ptr<CGameObject>& p) {
		if (p->getFrameName().contains("Plane")) {
			p->SetInstanceID(1);
			p->getMaterials().emplace_back();
			Material& mt = p->getMaterials()[0];
			mt.m_bHasAlbedoColor = true; mt.m_xmf4AlbedoColor = XMFLOAT4(0.1613118, 0.2065666, 0.2358491, 0.2);
			mt.m_bHasNormalMap = true; mt.m_nNormalMapIndex = textures.size() - 1;

			void* tempptr{};
			std::vector<XMFLOAT2> tex0 = meshes[p->getMeshIndex()]->getTex0();
			for (XMFLOAT2& xmf : tex0) {
				xmf.x *= 10.0f; xmf.y *= 10.0f;
			}
			meshes[p->getMeshIndex()]->getTexCoord0Buffer()->Map(0, nullptr, &tempptr);
			memcpy(tempptr, tex0.data(), sizeof(XMFLOAT2) * tex0.size());
			meshes[p->getMeshIndex()]->getTexCoord0Buffer()->Unmap(0, nullptr);
		}
		});


	// cubeMap Ready
	m_nSkyboxIndex = textures.size();
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\WinterLandSky2.dds", true));
	// ===========================================================================================
	m_pResourceManager->InitializeGameObjectCBuffer();
	m_pResourceManager->PrepareObject();	// Ready OutputBuffer to  SkinningObject


	// ShaderBindingTable
	m_pShaderBindingTable = std::make_unique<CShaderBindingTableManager>();
	m_pShaderBindingTable->Setup(m_pRaytracingPipeline.get(), m_pResourceManager.get());
	m_pShaderBindingTable->CreateSBT();

	// Copy(normalObject) & SetPreMatrix ===============================

	for (int i = 0; i < Players.size(); ++i) {
		skinned[i]->SetPosition(XMFLOAT3(94.0f, 0.0f, 84.0f));
	}

	// ==============================================================================

	// Camera Setting ==============================================================
	m_pCamera->SetTarget(m_pPlayer->getObject()->getObject()->getObjects()[0].get());
	m_pCamera->SetHOffset(3.5f);
	m_pCamera->SetCameraLength(15.0f);
	m_pCamera->SetMapNumber(SCENE_CAVE);
	// ==========================================================================

	// AccelerationStructure
	m_pAccelerationStructureManager = std::make_unique<CAccelerationStructureManager>();
	m_pAccelerationStructureManager->Setup(m_pResourceManager.get(), 1);
	m_pAccelerationStructureManager->InitBLAS();
	m_pAccelerationStructureManager->InitTLAS();

	// UISetup ========================================================================
	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0, 0.0, 0.0), DEFINED_UAV_BUFFER_WIDTH, DEFINED_UAV_BUFFER_HEIGHT));
	m_vUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get()));
	m_vUIs[m_vUIs.size() - 1]->setPositionInViewport(0, 0);
	m_vUIs[m_vUIs.size() - 1]->setColor(0.0, 0.0, 0.0, 1.0);

	m_vUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get()));
	m_vUIs[m_vUIs.size() - 1]->setPositionInViewport(0, 0);
	m_vUIs[m_vUIs.size() - 1]->setColor(0.5, 0.5, 0.5, 0.0);

	PlayerUISetup(Players[Client.get_id()].getCharacterType());

	Client.SendPlayerReady(SCENE_CAVE);

	g_pSoundManager->StartAMB(ESOUND::SOUND_STAGE2_AMB);
}

void CRaytracingCaveScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	switch (nMessage) {
	case WM_KEYDOWN:
		switch (wParam) {
		case 'N':
			m_pCamera->toggleNormalMapping();
			break;
		case 'M':
			m_pCamera->toggleAlbedoColor();
			break;
		case 'B':
			m_pCamera->toggleReflection();
			break;
		case '9':
			m_pCamera->SetThirdPersonMode(false);
			break;
		case '0':
			m_pCamera->SetThirdPersonMode(true);
			break;
		case '7':
			Client.SendMasterKey();
			break;
		case 'Z':
			m_vItemUIs[cItem]->setRenderState(false);
			--cItem;
			if (cItem < 0) cItem = 3;
			m_vItemUIs[cItem]->setRenderState(true);
			break;
		case 'C':
			m_vItemUIs[cItem]->setRenderState(false);
			++cItem;
			if (cItem > 3) cItem = 0;
			m_vItemUIs[cItem]->setRenderState(true);
			break;
		case 'X':
			if (m_fItemCurTime <= 0.0f && !g_PlayerDie[m_local_id]) {
				Client.SendUseItem(cItem);
				m_fItemCurTime = m_fItemCoolTime;
			}
			break;
		case 'P':
			m_bUIOnOff = !m_bUIOnOff;
			break;
		}
		break;
	case WM_KEYUP:
		break;
	}
}

void CRaytracingCaveScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	if (g_InGameState == IS_GAMING) {
		m_pPlayer->MouseProcess(hWnd, nMessage, wParam, lParam);
		switch (nMessage) {
		case WM_MOUSEWHEEL:
			short delta = GET_WHEEL_DELTA_WPARAM(wParam);
			if (delta > 0)
				m_pCamera->ChangeLength(1);
			else if (delta < 0)
				m_pCamera->ChangeLength(0);
			break;
		}
	}
}

void CRaytracingCaveScene::Create_Limadon()
{
	m_nMonsterNum = m_vMonsters.size();

	m_pResourceManager->AddSkinningResourceFromFile(L"src\\model\\Limadon.bin", "src\\texture\\Limadon\\", MONSTER);
	m_vMonsters.emplace_back(std::make_unique<Limadon>(
		m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 1].get(),
		m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get()));

	for (auto& o : m_pResourceManager->getSkinningObjectList().back()->getObjects()) {
		for (auto& ma : o->getMaterials())
			ma.m_bHasEmissiveColor = false;
	}

	m_vMonsters[m_vMonsters.size() - 1]->getObject()->setPreTransform(3.0f, XMFLOAT3(), XMFLOAT3());
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->SetPosition(XMFLOAT3(-18.0f, 0.0f, -15.5f));
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->Rotate(XMFLOAT3(00.0f, 70.0f, 00.0f));

	m_pMonsters.emplace_back(std::make_unique<CMonster>(m_vMonsters[m_vMonsters.size() - 1].get()));

	{ // server object
		auto newMonster = std::make_unique<Monster>(m_nMonsterNum, MonsterType::Limadon);
		newMonster->setRenderingObject(m_vMonsters.back()->getObject());
		newMonster->setAnimationManager(m_vMonsters.back()->getAniManager());
		Monsters[m_nMonsterNum] = std::move(newMonster);
	}

	m_nMonsterNum = m_vMonsters.size();

	m_pResourceManager->getSkinningObjectList().emplace_back(std::make_unique<CRayTracingSkinningObject>());
	auto& newSkinningObject = m_pResourceManager->getSkinningObjectList().back();
	newSkinningObject->CopyFromOtherObject(m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 2].get());

	auto* sourceManager = m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get();
	auto* monsterManager = dynamic_cast<CMonsterManager*>(sourceManager);
	m_pResourceManager->getAnimationManagers().emplace_back(std::make_unique<CMonsterManager>(*monsterManager));
	auto& newAnimationManager = m_pResourceManager->getAnimationManagers().back();

	newAnimationManager->SetFramesPointerFromSkinningObject(newSkinningObject->getObjects());
	newAnimationManager->MakeAnimationMatrixIndex(newSkinningObject.get());
	
	m_vMonsters.emplace_back(std::make_unique<Limadon>(newSkinningObject.get(), newAnimationManager.get()));

	m_vMonsters[m_vMonsters.size() - 1]->getObject()->setPreTransform(3.0f, XMFLOAT3(), XMFLOAT3());
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->SetPosition(XMFLOAT3(-152.1f, 0.0f, 246.3f));
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->Rotate(XMFLOAT3(0.0f, 130.0f, 0.0f));

	m_pMonsters.emplace_back(std::make_unique<CMonster>(m_vMonsters[m_vMonsters.size() - 1].get()));

	{ // server object
		auto newMonster = std::make_unique<Monster>(m_nMonsterNum, MonsterType::Limadon);
		newMonster->setRenderingObject(m_vMonsters.back()->getObject());
		newMonster->setAnimationManager(m_vMonsters.back()->getAniManager());
		Monsters[m_nMonsterNum] = std::move(newMonster);
	}

	for (auto& o : newSkinningObject->getObjects()) {
		for (auto& ma : o->getMaterials()) {
			ma.m_bHasEmissiveColor = false;
		}
	}
}

void CRaytracingCaveScene::Create_Fulgurodonte()
{
	m_nMonsterNum = m_vMonsters.size();

	m_pResourceManager->AddSkinningResourceFromFile(L"src\\model\\Fulgurodonte.bin", "src\\texture\\Fulgurodonte\\", MONSTER);
	m_vMonsters.emplace_back(std::make_unique<Fulgurodonte>(
		m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 1].get(),
		m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get()));

	for (auto& o : m_pResourceManager->getSkinningObjectList().back()->getObjects()) {
		for (auto& ma : o->getMaterials())
			ma.m_bHasEmissiveColor = false;
	}

	m_vMonsters[m_vMonsters.size() - 1]->getObject()->setPreTransform(2.0f, XMFLOAT3(), XMFLOAT3());
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->SetPosition(XMFLOAT3(-160.0f, 0.0f, 78.6f));
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->Rotate(XMFLOAT3(0.0f, 180.0f, 0.0f));

	m_pMonsters.emplace_back(std::make_unique<CMonster>(m_vMonsters[m_vMonsters.size() - 1].get()));

	{ // server object
		auto newMonster = std::make_unique<Monster>(m_nMonsterNum, MonsterType::Fulgurodonte);
		newMonster->setRenderingObject(m_vMonsters.back()->getObject());
		newMonster->setAnimationManager(m_vMonsters.back()->getAniManager());
		Monsters[m_nMonsterNum] = std::move(newMonster);
	}

	auto& mon = m_vMonsters[m_vMonsters.size() - 1];

	for (auto& s : mon->getObject()->getObjects())
	{
		if (s->getFrameName() == "Fulgurodonte_BeakLowerLeft")
		{
			mon->SetHead(s.get());
			break;
		}
	}

	m_pResourceManager->getMeshList().emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0f, 0.0f, 0.0f), 0.4f, "sphere"));
	size_t meshIndex = m_pResourceManager->getMeshList().size() - 1;
	Fulgurodonte* monster = dynamic_cast<Fulgurodonte*>(m_vMonsters.back().get());
	Material sharedMaterial;

	for (int i = 0; i < 9; ++i) {
		m_pResourceManager->getGameObjectList().emplace_back(std::make_unique<CGameObject>());
		m_pResourceManager->getGameObjectList().back()->SetMeshIndex(meshIndex);
		m_pResourceManager->getGameObjectList().back()->getMaterials().push_back(sharedMaterial);

		auto projectile = std::make_unique<CProjectile>();
		projectile->setGameObject(m_pResourceManager->getGameObjectList().back().get());

		monster->GetBullets().push_back(std::move(projectile));
	}

	m_pResourceManager->getSkinningObjectList().emplace_back(std::make_unique<CRayTracingSkinningObject>());
	auto& newSkinningObject = m_pResourceManager->getSkinningObjectList().back();
	newSkinningObject->CopyFromOtherObject(m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 2].get());

	auto* sourceManager = m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get();
	auto* monsterManager = dynamic_cast<CMonsterManager*>(sourceManager);
	m_pResourceManager->getAnimationManagers().emplace_back(std::make_unique<CMonsterManager>(*monsterManager));
	auto& newAnimationManager = m_pResourceManager->getAnimationManagers().back();

	newAnimationManager->SetFramesPointerFromSkinningObject(newSkinningObject->getObjects());
	newAnimationManager->MakeAnimationMatrixIndex(newSkinningObject.get());

	m_nMonsterNum = m_vMonsters.size();

	m_vMonsters.emplace_back(std::make_unique<Fulgurodonte>(newSkinningObject.get(), newAnimationManager.get()));

	m_vMonsters[m_vMonsters.size() - 1]->getObject()->setPreTransform(2.0f, XMFLOAT3(), XMFLOAT3());
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->SetPosition(XMFLOAT3(-128.6f, 0.0f, 272.8f));
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->Rotate(XMFLOAT3(0.0f, 220.0f, 0.0f));

	m_pMonsters.emplace_back(std::make_unique<CMonster>(m_vMonsters[m_vMonsters.size() - 1].get()));

	{ // server object
		auto newMonster = std::make_unique<Monster>(m_nMonsterNum, MonsterType::Fulgurodonte);
		newMonster->setRenderingObject(m_vMonsters.back()->getObject());
		newMonster->setAnimationManager(m_vMonsters.back()->getAniManager());
		Monsters[m_nMonsterNum] = std::move(newMonster);
	}

	for (auto& o : newSkinningObject->getObjects()) {
		for (auto& ma : o->getMaterials()) {
			ma.m_bHasEmissiveColor = false;
		}
	}

	//m_pResourceManager->getMeshList().emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0f, 0.0f, 0.0f), 0.4f, "sphere"));
	meshIndex = m_pResourceManager->getMeshList().size() - 1;
	monster = dynamic_cast<Fulgurodonte*>(m_vMonsters.back().get());

	for (int i = 0; i < 9; ++i) {
		m_pResourceManager->getGameObjectList().emplace_back(std::make_unique<CGameObject>());
		m_pResourceManager->getGameObjectList().back()->SetMeshIndex(meshIndex);
		m_pResourceManager->getGameObjectList().back()->getMaterials().push_back(sharedMaterial);

		auto projectile = std::make_unique<CProjectile>();
		projectile->setGameObject(m_pResourceManager->getGameObjectList().back().get());

		monster->GetBullets().push_back(std::move(projectile));
	}

	m_pResourceManager->getSkinningObjectList().emplace_back(std::make_unique<CRayTracingSkinningObject>());
	auto& newSkinningObject1 = m_pResourceManager->getSkinningObjectList().back();
	newSkinningObject1->CopyFromOtherObject(m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 2].get());

	auto* sourceManager1 = m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get();
	auto* monsterManager1 = dynamic_cast<CMonsterManager*>(sourceManager1);
	m_pResourceManager->getAnimationManagers().emplace_back(std::make_unique<CMonsterManager>(*monsterManager1));
	auto& newAnimationManager1 = m_pResourceManager->getAnimationManagers().back();

	newAnimationManager1->SetFramesPointerFromSkinningObject(newSkinningObject1->getObjects());
	newAnimationManager1->MakeAnimationMatrixIndex(newSkinningObject1.get());

	m_nMonsterNum = m_vMonsters.size();

	m_vMonsters.emplace_back(std::make_unique<Fulgurodonte>(newSkinningObject1.get(), newAnimationManager1.get()));

	m_vMonsters[m_vMonsters.size() - 1]->getObject()->setPreTransform(2.0f, XMFLOAT3(), XMFLOAT3());
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->SetPosition(XMFLOAT3(-92.0f, 0.0f, 376.5f));
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->Rotate(XMFLOAT3(0.0f, 180.0f, 0.0f));

	m_pMonsters.emplace_back(std::make_unique<CMonster>(m_vMonsters[m_vMonsters.size() - 1].get()));

	{ // server object
		auto newMonster = std::make_unique<Monster>(m_nMonsterNum, MonsterType::Fulgurodonte);
		newMonster->setRenderingObject(m_vMonsters.back()->getObject());
		newMonster->setAnimationManager(m_vMonsters.back()->getAniManager());
		Monsters[m_nMonsterNum] = std::move(newMonster);
	}

	for (auto& o : newSkinningObject1->getObjects()) {
		for (auto& ma : o->getMaterials()) {
			ma.m_bHasEmissiveColor = false;
		}
	}

	//m_pResourceManager->getMeshList().emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0f, 0.0f, 0.0f), 0.4f, "sphere"));
	meshIndex = m_pResourceManager->getMeshList().size() - 1;
	monster = dynamic_cast<Fulgurodonte*>(m_vMonsters.back().get());

	for (int i = 0; i < 9; ++i) {
		m_pResourceManager->getGameObjectList().emplace_back(std::make_unique<CGameObject>());
		m_pResourceManager->getGameObjectList().back()->SetMeshIndex(meshIndex);
		m_pResourceManager->getGameObjectList().back()->getMaterials().push_back(sharedMaterial);

		auto projectile = std::make_unique<CProjectile>();
		projectile->setGameObject(m_pResourceManager->getGameObjectList().back().get());

		monster->GetBullets().push_back(std::move(projectile));
	}
}

void CRaytracingCaveScene::Create_Occisodonte()
{
	m_nMonsterNum = m_vMonsters.size();

	m_pResourceManager->AddSkinningResourceFromFile(L"src\\model\\Occisodonte.bin", "src\\texture\\Occisodonte\\", MONSTER);
	m_vMonsters.emplace_back(std::make_unique<Occisodonte>(
		m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 1].get(),
		m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get()));

	for (auto& o : m_pResourceManager->getSkinningObjectList().back()->getObjects()) {
		for (auto& ma : o->getMaterials())
			ma.m_bHasEmissiveColor = false;
	}

	m_vMonsters[m_vMonsters.size() - 1]->getObject()->setPreTransform(2.5f, XMFLOAT3(), XMFLOAT3());
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->SetPosition(XMFLOAT3(-30.0f, 0.0f, 21.0f));
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->Rotate(XMFLOAT3(0.0f, 110.0f, 0.0f));

	m_pMonsters.emplace_back(std::make_unique<CMonster>(m_vMonsters[m_vMonsters.size() - 1].get()));

	{ // server object
		auto newMonster = std::make_unique<Monster>(m_nMonsterNum, MonsterType::Occisodonte);
		newMonster->setRenderingObject(m_vMonsters.back()->getObject());
		newMonster->setAnimationManager(m_vMonsters.back()->getAniManager());
		Monsters[m_nMonsterNum] = std::move(newMonster);
	}

	m_pResourceManager->getSkinningObjectList().emplace_back(std::make_unique<CRayTracingSkinningObject>());
	auto& newSkinningObject = m_pResourceManager->getSkinningObjectList().back();
	newSkinningObject->CopyFromOtherObject(m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 2].get());

	auto* sourceManager = m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get();
	auto* monsterManager = dynamic_cast<CMonsterManager*>(sourceManager);
	m_pResourceManager->getAnimationManagers().emplace_back(std::make_unique<CMonsterManager>(*monsterManager));
	auto& newAnimationManager = m_pResourceManager->getAnimationManagers().back();

	newAnimationManager->SetFramesPointerFromSkinningObject(newSkinningObject->getObjects());
	newAnimationManager->MakeAnimationMatrixIndex(newSkinningObject.get());

	m_nMonsterNum = m_vMonsters.size();

	m_vMonsters.emplace_back(std::make_unique<Occisodonte>(newSkinningObject.get(), newAnimationManager.get()));

	m_vMonsters[m_vMonsters.size() - 1]->getObject()->setPreTransform(2.5f, XMFLOAT3(), XMFLOAT3());
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->SetPosition(XMFLOAT3(-97.0f, 0.0f, 315.3f));
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->Rotate(XMFLOAT3(0.0f, 270.0f, 0.0f));

	m_pMonsters.emplace_back(std::make_unique<CMonster>(m_vMonsters[m_vMonsters.size() - 1].get()));

	{ // server object
		auto newMonster = std::make_unique<Monster>(m_nMonsterNum, MonsterType::Occisodonte);
		newMonster->setRenderingObject(m_vMonsters.back()->getObject());
		newMonster->setAnimationManager(m_vMonsters.back()->getAniManager());
		Monsters[m_nMonsterNum] = std::move(newMonster);
	}

	for (auto& o : newSkinningObject->getObjects()) {
		for (auto& ma : o->getMaterials()) {
			ma.m_bHasEmissiveColor = false;
		}
	}
}

void CRaytracingCaveScene::Create_Crassorrid()
{
	m_nMonsterNum = m_vMonsters.size();

	m_pResourceManager->AddSkinningResourceFromFile(L"src\\model\\Crassorrid.bin", "src\\texture\\Crassorrid\\", MONSTER);
	m_vMonsters.emplace_back(std::make_unique<Crassorrid>(
		m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 1].get(),
		m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get()));

	for (auto& o : m_pResourceManager->getSkinningObjectList().back()->getObjects()) {
		for (auto& ma : o->getMaterials())
			ma.m_bHasEmissiveColor = false;
	}

	m_vMonsters[m_vMonsters.size() - 1]->getObject()->setPreTransform(4.0f, XMFLOAT3(), XMFLOAT3());
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->SetPosition(XMFLOAT3(0.5f, 0.0f, 362.8f));
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->Rotate(XMFLOAT3(0.0f, 270.0f, 0.0f));

	m_pMonsters.push_back(std::make_unique<CMonster>(m_vMonsters[m_vMonsters.size() - 1].get()));

	{ // server object
		auto newMonster = std::make_unique<Monster>(m_nMonsterNum, MonsterType::Crassorrid);
		newMonster->setRenderingObject(m_vMonsters.back()->getObject());
		newMonster->setAnimationManager(m_vMonsters.back()->getAniManager());
		Monsters[m_nMonsterNum] = std::move(newMonster);
	}
}

void CRaytracingCaveScene::ProcessInput(float fElapsedTime)
{
	UCHAR keyBuffer[256];
	GetKeyboardState(keyBuffer);

	if (g_InGameState == IS_GAMING) {
		if (!m_pCamera->getThirdPersonState()) {
			bool shiftDown = false;
			if (keyBuffer[VK_SHIFT] & 0x80)
				shiftDown = true;
			if (keyBuffer['W'] & 0x80)
				m_pCamera->Move(0, fElapsedTime, shiftDown);
			if (keyBuffer['S'] & 0x80)
				m_pCamera->Move(5, fElapsedTime, shiftDown);
			if (keyBuffer['D'] & 0x80)
				m_pCamera->Move(3, fElapsedTime, shiftDown);
			if (keyBuffer['A'] & 0x80)
				m_pCamera->Move(4, fElapsedTime, shiftDown);
			if (keyBuffer[VK_SPACE] & 0x80)
				m_pCamera->Move(1, fElapsedTime, shiftDown);
			if (keyBuffer[VK_CONTROL] & 0x80)
				m_pCamera->Move(2, fElapsedTime, shiftDown);
		}
		else {
			if (!g_PlayerDie[m_local_id]) {
				KeyInputRet ret = m_pPlayer->ProcessInput(keyBuffer, fElapsedTime);
				UIUseSkill(ret);
				SkillParticleStart(ret);
				CAnimationManager* myManager = m_pPlayer->getAniManager();
				Client.SendMovePacket(myManager->getElapsedTime(), myManager->getCurrentSet());	// Check
			}
		}
	}
}

void CRaytracingCaveScene::UpdateObject(float fElapsedTime)
{
	// compute shader & rootSignature set
	g_DxResource.cmdList->SetPipelineState(m_pAnimationComputeShader.Get());
	g_DxResource.cmdList->SetComputeRootSignature(m_pComputeRootSignature.Get());

	m_pResourceManager->UpdateSkinningMesh(fElapsedTime);
	// particle update
	// SetRootSignature
	// particle update

	Flush();
	// Skinning Object BLAS ReBuild
	m_pResourceManager->ReBuildBLAS();

	m_pResourceManager->UpdateWorldMatrix();

	for (auto& m : m_vPlayers)
	{
		m->UpdateObject(fElapsedTime);

		if (m->CheckAC())
		{
			AttackCollision(m_vMonsters, m_vPlayers, 1);
		}
		if (m->HasActiveBullet())
		{
			ShootCollision(m_vMonsters, m_vPlayers, 1);
		}
	}

	for (auto& m : m_vMonsters)
	{
		m->UpdateObject(fElapsedTime);

		if (m->CheckAC())
		{
			AttackCollision(m_vPlayers, m_vMonsters, 0);
		}
		if (m->HasActiveBullet())
		{
			ShootCollision(m_vPlayers, m_vMonsters, 0);
		}
	}

	m_pPlayer->CollisionCheck(m_pCollisionHMap.get(), fElapsedTime, -200.0f, 0.0, -66.5f, SCENE_CAVE);
	m_pPlayer->HeightCheck(m_pHeightMap.get(), fElapsedTime, -200.0f, -10.0f, -66.5f, SCENE_CAVE);

	BulletCheck(m_vPlayers, m_pCollisionHMap.get(), fElapsedTime, -200.0f, 0.0, -66.5f, SCENE_CAVE);

	for (auto& p : m_pMonsters) {
		p->CollisionCheck(m_pCollisionHMap.get(), fElapsedTime, -200.0f, 0.0, -66.5f, SCENE_CAVE);
		p->HeightCheck(m_pHeightMap.get(), fElapsedTime, -200.0f, -10.0f, -66.5f, SCENE_CAVE);
	}

	BulletCheck(m_vMonsters, m_pCollisionHMap.get(), fElapsedTime, -200.0f, 0.0, -66.5f, SCENE_CAVE);

	AutoDirection(m_vPlayers, m_vMonsters);

	if (m_pCamera->getThirdPersonState()) {
		XMFLOAT3& EYE = m_pCamera->getEyeCalculateOffset();
		float cHeight = m_pHeightMap->GetHeightinWorldSpace(EYE.x + 200.0f, EYE.z + 66.5f);
		if (EYE.y < cHeight - 10.0f + 0.5f) {
			m_pCamera->UpdateViewMatrix(cHeight - 10.0f + 0.5f);
		}
		else
			m_pCamera->UpdateViewMatrix();
	}
	else
		m_pCamera->UpdateViewMatrix();

	g_DxResource.cmdList->SetGraphicsRootSignature(m_ParticleRS.Get());
	m_pCamera->SetElapsedTimeAndShader(fElapsedTime, 0);
	m_pResourceManager->UpdateParticles(fElapsedTime);

	m_pAccelerationStructureManager->UpdateScene(m_pCamera->getEye());

	switch (g_InGameState) {
	case IS_LOADING: {
		wOpacity -= 0.5 * fElapsedTime;
		if (wOpacity < 0.0f) {
			g_InGameState = IS_GAMING;
			wOpacity = 0.0f;
		}
		m_vUIs[0]->setColor(0.0, 0.0, 0.0, wOpacity);
		break;
	}
	case IS_GAMING: {
		break;
	}
	case IS_FINISH: {
		wOpacity += 0.2 * fElapsedTime;
		if (wOpacity > 1.0f) {
			ShowCursor(TRUE);
			g_pSoundManager->AllStop();
			m_nNextScene = SCENE_WINTERLAND;
			wOpacity = 1.0f;
		}
		m_vUIs[0]->setColor(0.0, 0.0, 0.0, wOpacity);
		break;
	}
	}
	// Player UI ==================================================
	int buffstart = 20; int bstride = 40;
	float cMP = Players[m_local_id].GetMP();

	for (int i = 0; i < m_numUser; ++i) {
		int t{};
		// hp/mp
		m_vPlayersStatUI[i][1]->setScaleX(Players[i].GetHP() / g_maxHPs[i]);
		m_vPlayersStatUI[i][3]->setScaleXWithUV(Players[i].GetMP() / g_maxMPs[i]);
		if (i == m_local_id) {
			for (int j = 0; j < 3; ++j) {
				if (g_PlayerBuffState[j]) {
					m_vPlayersStatUI[i][j + 4]->setRenderState(true);
					m_vPlayersStatUI[i][j + 4]->setPositionInViewport(buffstart + (t * bstride), 100);
					++t;
				}
				else {
					m_vPlayersStatUI[i][j + 4]->setRenderState(false);
				}
			}
			m_vBossUIs[1]->setScaleX(Monsters[7]->getHP() / Monsters[7]->getMaxHP());
			XMFLOAT3 mpos = m_pMonsters.back()->getObject()->getObject()->getPosition(); mpos.y = 0;
			XMFLOAT3 ppos = Players[m_local_id].getRenderingObject()->getPosition(); ppos.y = 0;
			float distance;
			XMStoreFloat(&distance, XMVector3Length(XMLoadFloat3(&mpos) - XMLoadFloat3(&ppos)));
			if (distance <= 100.0f)
				m_bBossBattle = true;
			else
				m_bBossBattle = false;
		}
	}

	m_fItemCurTime -= fElapsedTime;
	if (m_fItemCurTime < 0.0)
		m_fItemCurTime = 0.0f;
	m_vItemUIs[4]->setScaleY(m_fItemCurTime / m_fItemCoolTime);

	{
		if (cMP < g_SkillCost[0])
			m_vSkillUIs[6]->setRenderState(true);
		else
			m_vSkillUIs[6]->setRenderState(false);

		if (cMP < g_SkillCost[1])
			m_vSkillUIs[7]->setRenderState(true);
		else
			m_vSkillUIs[7]->setRenderState(false);

		if (cMP < g_SkillCost[2])
			m_vSkillUIs[8]->setRenderState(true);
		else
			m_vSkillUIs[8]->setRenderState(false);

		for (int i = 0; i < 3; ++i) {
			g_SkillCurCTime[i] -= fElapsedTime;
			if (g_SkillCurCTime[i] < 0) g_SkillCurCTime[i] = 0.0f;
			m_vSkillUIs[i + 3]->setScaleY(g_SkillCurCTime[i] / g_SkillCoolTime[i]);
		}
	}
	// =================================================================

	if (g_PlayerDie[m_local_id])
		m_vUIs.back()->setColor(0.5, 0.5, 0.5, 0.5);
	else
		m_vUIs.back()->setColor(0.5, 0.5, 0.5, 0.0);
}

void CRaytracingCaveScene::Render()
{
	m_pCamera->SetShaderVariable();
	m_pAccelerationStructureManager->SetScene();
	m_pResourceManager->SetLights();

	std::vector<std::unique_ptr<CTexture>>& textures = m_pResourceManager->getTextureList();
	g_DxResource.cmdList->SetComputeRootDescriptorTable(4, textures[m_nSkyboxIndex]->getView()->GetGPUDescriptorHandleForHeapStart());

	D3D12_DISPATCH_RAYS_DESC raydesc{};
	raydesc.Depth = 1;
	raydesc.Width = DEFINED_UAV_BUFFER_WIDTH;
	raydesc.Height = DEFINED_UAV_BUFFER_HEIGHT;

	raydesc.RayGenerationShaderRecord.StartAddress = m_pShaderBindingTable->getRayGenTable()->GetGPUVirtualAddress();
	raydesc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	raydesc.MissShaderTable.StartAddress = m_pShaderBindingTable->getMissTable()->GetGPUVirtualAddress();
	raydesc.MissShaderTable.SizeInBytes = m_pShaderBindingTable->getMissSize();
	raydesc.MissShaderTable.StrideInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

	raydesc.HitGroupTable.StartAddress = m_pShaderBindingTable->getHitGroupTable()->GetGPUVirtualAddress();
	raydesc.HitGroupTable.SizeInBytes = m_pShaderBindingTable->getHitGroupSize();
	raydesc.HitGroupTable.StrideInBytes = m_pShaderBindingTable->getHitGroupStride();

	g_DxResource.cmdList->DispatchRays(&raydesc);

	// UI Render ==================================================================================

	ID3D12GraphicsCommandList4* cmdList = g_DxResource.cmdList;
	auto barrier = [&](ID3D12Resource* pResource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
		{
			D3D12_RESOURCE_BARRIER resBarrier{};
			resBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			resBarrier.Transition.pResource = pResource;
			resBarrier.Transition.StateBefore = before;
			resBarrier.Transition.StateAfter = after;
			resBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			cmdList->ResourceBarrier(1, &resBarrier);
		};

	barrier(m_pOutputBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_VIEWPORT vv{};
	vv.Width = DEFINED_UAV_BUFFER_WIDTH; vv.Height = DEFINED_UAV_BUFFER_HEIGHT; vv.MinDepth = 0.0f; vv.MaxDepth = 1.0f;
	cmdList->RSSetViewports(1, &vv);
	D3D12_RECT ss{ 0, 0, DEFINED_UAV_BUFFER_WIDTH, DEFINED_UAV_BUFFER_HEIGHT };
	cmdList->RSSetScissorRects(1, &ss);
	cmdList->OMSetRenderTargets(1, &m_RTV->GetCPUDescriptorHandleForHeapStart(), FALSE, &m_DSV->GetCPUDescriptorHandleForHeapStart());
	cmdList->SetGraphicsRootSignature(m_UIRootSignature.Get());
	cmdList->SetPipelineState(m_UIPipelineState.Get());
	cmdList->SetGraphicsRootConstantBufferView(0, m_cameraCB->GetGPUVirtualAddress());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// player UI ====================================
	if (m_bUIOnOff) {
		for (int i = 0; i < m_numUser; ++i) {
			for (auto& p : m_vPlayersStatUI[i])
				p->Render();
		}

		if (m_bBossBattle)
			for (auto& p : m_vBossUIs)
				p->Render();

		for (auto& p : m_vItemUIs)
			p->Render();

		for (auto& p : m_vSkillUIs)
			p->Render();
	}
	// ===============================================

	for (auto& p : m_vUIs)
		p->Render();

	barrier(m_pOutputBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

// ============================================================================

void CRaytracingETPScene::SetUp(ComPtr<ID3D12Resource>& outputBuffer, std::shared_ptr<CRayTracingPipeline> pipeline)
{
	g_InGameState = IS_LOADING;
	wOpacity = 1.0f;
	m_vMonsters.clear();
	m_vPlayers.clear();
	Monsters.clear();

	m_pOutputBuffer = outputBuffer;

	// CreateUISetup
	CreateOrthoMatrixBuffer();
	CreateRTVDSV();
	CreateUIRootSignature();
	CreateUIPipelineState();

	// ParticleRS
	CreateParticleRS();

	// animation Pipeline Ready
	CreateComputeRootSignature();
	CreateComputeShader();

	// Create And Set up PipelineState
	m_pRaytracingPipeline = pipeline;

	// Resource Ready
	m_pResourceManager = std::make_unique<CResourceManager>();
	m_pResourceManager->SetUp(3);

	std::vector<std::unique_ptr<CGameObject>>& normalObjects = m_pResourceManager->getGameObjectList();
	std::vector<std::unique_ptr<CSkinningObject>>& skinned = m_pResourceManager->getSkinningObjectList();
	std::vector<std::unique_ptr<Mesh>>& meshes = m_pResourceManager->getMeshList();
	std::vector<std::unique_ptr<CTexture>>& textures = m_pResourceManager->getTextureList();
	std::vector<std::unique_ptr<CAnimationManager>>& aManagers = m_pResourceManager->getAnimationManagers();

	// Object File Read ========================================	! !
	m_pResourceManager->AddResourceFromFile(L"src\\model\\Map\\ETP\\ETP.bin", "src\\texture\\Map\\");
	m_pResourceManager->AddResourceFromFile(L"src\\model\\Map\\ETP\\Water.bin", "src\\texture\\Map\\");

	{		// Water
		textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Map\\WaterTurbulent00_NORM.dds"));
		auto p = normalObjects[normalObjects.size() - 1].get();
		p->SetInstanceID(2);
		p->getMaterials().emplace_back();
		Material& mt = p->getMaterials()[0];
		mt.m_bHasAlbedoColor = true; mt.m_xmf4AlbedoColor = XMFLOAT4(0.1613118, 0.2065666, 0.2358491, 0.2);
		//mt.m_bHasMetallicMap = true; mt.m_nMetallicMapIndex = textures.size() - 1;
		mt.m_bHasNormalMap = true; mt.m_nNormalMapIndex = textures.size() - 1;
	}

	// Players Create ========================================================================
	for (int i = 0; i < Players.size(); ++i) {
		// player job check
		// short player_job = Players[i].getJobInfo();
		short player_job = Players[i].getCharacterType();
		switch (player_job) {
		case JOB_MAGE:
			CreateMageCharacter();
			break;
		case JOB_WARRIOR:
			CreateWarriorCharacter();
			break;
		case JOB_HEALER:
			CreatePriestCharacter();
			break;
		}
		Players[i].setRenderingObject(skinned[skinned.size() - 1].get());
		Players[i].setAnimationManager(aManagers[aManagers.size() - 1].get());
		if (i == Client.get_id()) {
			m_pPlayer = std::make_unique<CPlayer>(m_vPlayers[m_vPlayers.size() - 1].get(), m_pCamera);
			CreateParticle(player_job);
		}
	}

	Create_Feroptere();
	Create_Pistriptere();
	Create_RostrokarckLarvae();
	Create_Xenokarce();

	// Light Read
	m_pResourceManager->AddLightsFromFile(L"src\\Light\\Light_ETP.bin");
	m_pResourceManager->ReadyLightBufferContent();
	// =========================================================

	// Create Normal Object & skinning Object Copy ========================================

	m_pHeightMap = std::make_unique<CHeightMapImage>(L"src\\model\\Map\\ETP\\ETP_Terrain.raw", 1024, 1024, XMFLOAT3(1.0f, 0.0156, 1.0f));
	meshes.emplace_back(std::make_unique<Mesh>(m_pHeightMap.get(), "terrain"));
	normalObjects.emplace_back(std::make_unique<CGameObject>());
	normalObjects[normalObjects.size() - 1]->SetMeshIndex(meshes.size() - 1);

	normalObjects[normalObjects.size() - 1]->SetInstanceID(10);
	normalObjects[normalObjects.size() - 1]->getMaterials().emplace_back();
	normalObjects[normalObjects.size() - 1]->getMaterials()[0].m_bHasMetallic = true;
	normalObjects[normalObjects.size() - 1]->getMaterials()[0].m_bHasGlossiness = true;
	normalObjects[normalObjects.size() - 1]->getMaterials()[0].m_fGlossiness = 0.0f;
	normalObjects[normalObjects.size() - 1]->SetPosition(XMFLOAT3(-512.0, 0.0, -512.0));

	m_pRoadTerrain = std::make_unique<CHeightMapImage>(L"src\\model\\Map\\ETP\\ETP_Terrain_Road.raw", 1024, 1024, XMFLOAT3(1.0f, 0.0156, 1.0f));
	m_pCollisionHMap = std::make_unique<CHeightMapImage>(L"src\\model\\Map\\ETP\\ETP_CollisionMap.raw", 1024, 1024, XMFLOAT3(1.0f, 0.0156, 1.0f));

	PrepareTerrainTexture();

	// cubeMap Ready
	m_nSkyboxIndex = textures.size();
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\ETPSky.dds", true));
	// ===========================================================================================
	m_pResourceManager->InitializeGameObjectCBuffer();
	m_pResourceManager->PrepareObject();	// Ready OutputBuffer to  SkinningObject


	// ShaderBindingTable
	m_pShaderBindingTable = std::make_unique<CShaderBindingTableManager>();
	m_pShaderBindingTable->Setup(m_pRaytracingPipeline.get(), m_pResourceManager.get());
	m_pShaderBindingTable->CreateSBT();

	// Copy(normalObject) & SetPreMatrix ===============================

	for (int i = 0; i < Players.size(); ++i) {
		skinned[i]->SetPosition(XMFLOAT3(-219.0f, 0.0f, 301.0f));
	}

	// ==============================================================================

	// Camera Setting ==============================================================
	m_pCamera->SetTarget(m_pPlayer->getObject()->getObject()->getObjects()[0].get());
	m_pCamera->SetHOffset(3.5f);
	m_pCamera->SetCameraLength(15.0f);
	m_pCamera->SetMapNumber(SCENE_PLAIN);
	// ==========================================================================

	// AccelerationStructure
	m_pAccelerationStructureManager = std::make_unique<CAccelerationStructureManager>();
	m_pAccelerationStructureManager->Setup(m_pResourceManager.get(), 1);
	m_pAccelerationStructureManager->InitBLAS();
	m_pAccelerationStructureManager->InitTLAS();

	// UISetup ========================================================================
	meshes.emplace_back(std::make_unique<Mesh>(XMFLOAT3(0.0, 0.0, 0.0), DEFINED_UAV_BUFFER_WIDTH, DEFINED_UAV_BUFFER_HEIGHT));
	m_vUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get()));
	m_vUIs[m_vUIs.size() - 1]->setPositionInViewport(0, 0);
	m_vUIs[m_vUIs.size() - 1]->setColor(0.0, 0.0, 0.0, 1.0);

	m_vUIs.emplace_back(std::make_unique<UIObject>(1, 2, meshes[meshes.size() - 1].get()));
	m_vUIs[m_vUIs.size() - 1]->setPositionInViewport(0, 0);
	m_vUIs[m_vUIs.size() - 1]->setColor(0.5, 0.5, 0.5, 0.0);

	PlayerUISetup(Players[Client.get_id()].getCharacterType());

	Client.SendPlayerReady(SCENE_PLAIN);

	g_pSoundManager->StartAMB(ESOUND::SOUND_STAGE1_AMB);
}

void CRaytracingETPScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	switch (nMessage) {
	case WM_KEYDOWN:
		switch (wParam) {
		case 'N':
			m_pCamera->toggleNormalMapping();
			break;
		case 'M':
			m_pCamera->toggleAlbedoColor();
			break;
		case 'B':
			m_pCamera->toggleReflection();
			break;
		case '9':
			m_pCamera->SetThirdPersonMode(false);
			break;
		case '0':
			m_pCamera->SetThirdPersonMode(true);
			break;
		case '7':
			Client.SendMasterKey();
			break;
		case 'Z':
			m_vItemUIs[cItem]->setRenderState(false);
			--cItem;
			if (cItem < 0) cItem = 3;
			m_vItemUIs[cItem]->setRenderState(true);
			break;
		case 'C':
			m_vItemUIs[cItem]->setRenderState(false);
			++cItem;
			if (cItem > 3) cItem = 0;
			m_vItemUIs[cItem]->setRenderState(true);
			break;
		case 'X':
			if (m_fItemCurTime <= 0.0f && !g_PlayerDie[m_local_id]) {
				Client.SendUseItem(cItem);
				m_fItemCurTime = m_fItemCoolTime;
			}
			break;
		case 'P':
			m_bUIOnOff = !m_bUIOnOff;
			break;
		}
		break;
	case WM_KEYUP:
		break;
	}
}

void CRaytracingETPScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	if (g_InGameState == IS_GAMING) {
		m_pPlayer->MouseProcess(hWnd, nMessage, wParam, lParam);
		switch (nMessage) {
		case WM_MOUSEWHEEL:
			short delta = GET_WHEEL_DELTA_WPARAM(wParam);
			if (delta > 0)
				m_pCamera->ChangeLength(1);
			else if (delta < 0)
				m_pCamera->ChangeLength(0);
			break;
		}
	}
}

void CRaytracingETPScene::Create_Feroptere()
{
	m_pResourceManager->AddSkinningResourceFromFile(L"src\\model\\Feroptere.bin", "src\\texture\\Feroptere\\", MONSTER);
	m_vMonsters.emplace_back(std::make_unique<Feroptere>(
		m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 1].get(),
		m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get()));

	m_vMonsters[m_vMonsters.size() - 1]->getObject()->setPreTransform(5.0f, XMFLOAT3(), XMFLOAT3());
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->SetPosition(XMFLOAT3(-280.0f, 0.0f, 215.4f));
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->Rotate(XMFLOAT3(0.0f, 290.0f, 0.0f));

	m_pMonsters.push_back(std::make_unique<CMonster>(m_vMonsters[m_vMonsters.size() - 1].get()));

	{ // server object
		auto newMonster = std::make_unique<Monster>(m_nMonsterNum, MonsterType::Feroptere);
		newMonster->setRenderingObject(m_vMonsters.back()->getObject());
		newMonster->setAnimationManager(m_vMonsters.back()->getAniManager());
		Monsters[m_nMonsterNum] = std::move(newMonster);
		++m_nMonsterNum;
	}

	for (auto& o : m_pResourceManager->getSkinningObjectList().back()->getObjects()) {
		for (auto& ma : o->getMaterials())
			ma.m_bHasEmissiveColor = false;
	}

	m_pResourceManager->getSkinningObjectList().emplace_back(std::make_unique<CRayTracingSkinningObject>());
	auto& newSkinningObject = m_pResourceManager->getSkinningObjectList().back();
	newSkinningObject->CopyFromOtherObject(m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 2].get());

	auto* sourceManager = m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get();
	auto* monsterManager = dynamic_cast<CMonsterManager*>(sourceManager);
	m_pResourceManager->getAnimationManagers().emplace_back(std::make_unique<CMonsterManager>(*monsterManager));
	auto& newAnimationManager = m_pResourceManager->getAnimationManagers().back();

	newAnimationManager->SetFramesPointerFromSkinningObject(newSkinningObject->getObjects());
	newAnimationManager->MakeAnimationMatrixIndex(newSkinningObject.get());

	m_vMonsters.emplace_back(std::make_unique<Feroptere>(newSkinningObject.get(), newAnimationManager.get()));

	m_vMonsters.back()->getObject()->setPreTransform(5.0f, XMFLOAT3(), XMFLOAT3());
	m_vMonsters.back()->getObject()->SetPosition(XMFLOAT3(-246.3f, 0.0f, 15.1f));
	m_vMonsters.back()->getObject()->Rotate(XMFLOAT3(0.0f, 20.0f, 0.0f));

	m_pMonsters.push_back(std::make_unique<CMonster>(m_vMonsters[m_vMonsters.size() - 1].get()));

	{ // server object
		auto newMonster = std::make_unique<Monster>(m_nMonsterNum, MonsterType::Feroptere);
		newMonster->setRenderingObject(m_vMonsters.back()->getObject());
		newMonster->setAnimationManager(m_vMonsters.back()->getAniManager());
		Monsters[m_nMonsterNum] = std::move(newMonster);
		++m_nMonsterNum;
	}

	for (auto& o : newSkinningObject->getObjects()) {
		for (auto& ma : o->getMaterials()) {
			ma.m_bHasEmissiveColor = false;
		}
	}
}

void CRaytracingETPScene::Create_Pistriptere()
{
	m_pResourceManager->AddSkinningResourceFromFile(L"src\\model\\Pistriptere.bin", "src\\texture\\Pistriptere\\", MONSTER);
	m_vMonsters.emplace_back(std::make_unique<Pistriptere>(
		m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 1].get(),
		m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get()));

	for (auto& o : m_pResourceManager->getSkinningObjectList().back()->getObjects()) {
		for (auto& ma : o->getMaterials())
			ma.m_bHasEmissiveColor = false;
	}

	m_vMonsters[m_vMonsters.size() - 1]->getObject()->setPreTransform(3.0f, XMFLOAT3(), XMFLOAT3());
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->SetPosition(XMFLOAT3(-240.0f, 0.0f, 149.8f));
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->Rotate(XMFLOAT3(0.0f, 20.0f, 0.0f));

	m_pMonsters.push_back(std::make_unique<CMonster>(m_vMonsters[m_vMonsters.size() - 1].get()));

	{ // server object
		auto newMonster = std::make_unique<Monster>(m_nMonsterNum, MonsterType::Pistiripere);
		newMonster->setRenderingObject(m_vMonsters.back()->getObject());
		newMonster->setAnimationManager(m_vMonsters.back()->getAniManager());
		Monsters[m_nMonsterNum] = std::move(newMonster);
		++m_nMonsterNum;
	}

	m_pResourceManager->getSkinningObjectList().emplace_back(std::make_unique<CRayTracingSkinningObject>());
	auto& newSkinningObject = m_pResourceManager->getSkinningObjectList().back();
	newSkinningObject->CopyFromOtherObject(m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 2].get());

	auto* sourceManager = m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get();
	auto* monsterManager = dynamic_cast<CMonsterManager*>(sourceManager);
	m_pResourceManager->getAnimationManagers().emplace_back(std::make_unique<CMonsterManager>(*monsterManager));
	auto& newAnimationManager = m_pResourceManager->getAnimationManagers().back();

	newAnimationManager->SetFramesPointerFromSkinningObject(newSkinningObject->getObjects());
	newAnimationManager->MakeAnimationMatrixIndex(newSkinningObject.get());

	m_vMonsters.emplace_back(std::make_unique<Pistriptere>(newSkinningObject.get(), newAnimationManager.get()));

	m_vMonsters[m_vMonsters.size() - 1]->getObject()->setPreTransform(3.0f, XMFLOAT3(), XMFLOAT3());
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->SetPosition(XMFLOAT3(-351.1f, 0.0f, 26.7f));
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->Rotate(XMFLOAT3(0.0f, 350.0f, 0.0f));

	m_pMonsters.push_back(std::make_unique<CMonster>(m_vMonsters[m_vMonsters.size() - 1].get()));

	{ // server object
		auto newMonster = std::make_unique<Monster>(m_nMonsterNum, MonsterType::Pistiripere);
		newMonster->setRenderingObject(m_vMonsters.back()->getObject());
		newMonster->setAnimationManager(m_vMonsters.back()->getAniManager());
		Monsters[m_nMonsterNum] = std::move(newMonster);
		++m_nMonsterNum;
	}

	for (auto& o : newSkinningObject->getObjects()) {
		for (auto& ma : o->getMaterials()) {
			ma.m_bHasEmissiveColor = false;
		}
	}
}

void CRaytracingETPScene::Create_RostrokarckLarvae()
{
	m_pResourceManager->AddSkinningResourceFromFile(L"src\\model\\RostrokarckLarvae.bin", "src\\texture\\RostrokarckLarvae\\", MONSTER);
	m_vMonsters.emplace_back(std::make_unique<RostrokarckLarvae>(
		m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 1].get(),
		m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get()));

	for (auto& o : m_pResourceManager->getSkinningObjectList().back()->getObjects()) {
		for (auto& ma : o->getMaterials())
			ma.m_bHasEmissiveColor = false;
	}

	m_vMonsters[m_vMonsters.size() - 1]->getObject()->setPreTransform(10.0f, XMFLOAT3(), XMFLOAT3());
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->SetPosition(XMFLOAT3(-150.5f, 0.0f, 85.7f));
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->Rotate(XMFLOAT3(0.0f, 320.0f, 0.0f));

	m_pMonsters.push_back(std::make_unique<CMonster>(m_vMonsters[m_vMonsters.size() - 1].get()));

	{ // server object
		auto newMonster = std::make_unique<Monster>(m_nMonsterNum, MonsterType::RostrokarackLarvae);
		newMonster->setRenderingObject(m_vMonsters.back()->getObject());
		newMonster->setAnimationManager(m_vMonsters.back()->getAniManager());
		Monsters[m_nMonsterNum] = std::move(newMonster);
		++m_nMonsterNum;
	}

	m_pResourceManager->getSkinningObjectList().emplace_back(std::make_unique<CRayTracingSkinningObject>());
	auto& newSkinningObject = m_pResourceManager->getSkinningObjectList().back();
	newSkinningObject->CopyFromOtherObject(m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 2].get());

	auto* sourceManager = m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get();
	auto* monsterManager = dynamic_cast<CMonsterManager*>(sourceManager);
	m_pResourceManager->getAnimationManagers().emplace_back(std::make_unique<CMonsterManager>(*monsterManager));
	auto& newAnimationManager = m_pResourceManager->getAnimationManagers().back();

	newAnimationManager->SetFramesPointerFromSkinningObject(newSkinningObject->getObjects());
	newAnimationManager->MakeAnimationMatrixIndex(newSkinningObject.get());

	m_vMonsters.emplace_back(std::make_unique<RostrokarckLarvae>(newSkinningObject.get(), newAnimationManager.get()));

	m_vMonsters[m_vMonsters.size() - 1]->getObject()->setPreTransform(10.0f, XMFLOAT3(), XMFLOAT3());
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->SetPosition(XMFLOAT3(-164.7f, 0.0f, 66.0f));
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->Rotate(XMFLOAT3(0.0f, 320.0f, 0.0f));

	m_pMonsters.push_back(std::make_unique<CMonster>(m_vMonsters[m_vMonsters.size() - 1].get()));

	{ // server object
		auto newMonster = std::make_unique<Monster>(m_nMonsterNum, MonsterType::RostrokarackLarvae);
		newMonster->setRenderingObject(m_vMonsters.back()->getObject());
		newMonster->setAnimationManager(m_vMonsters.back()->getAniManager());
		Monsters[m_nMonsterNum] = std::move(newMonster);
		++m_nMonsterNum;
	}

	for (auto& o : newSkinningObject->getObjects()) {
		for (auto& ma : o->getMaterials()) {
			ma.m_bHasEmissiveColor = false;
		}
	}
}

void CRaytracingETPScene::Create_Xenokarce()
{
	m_pResourceManager->AddSkinningResourceFromFile(L"src\\model\\Xenokarce.bin", "src\\texture\\Xenokarce\\", MONSTER);
	m_vMonsters.emplace_back(std::make_unique<Xenokarce>(
		m_pResourceManager->getSkinningObjectList()[m_pResourceManager->getSkinningObjectList().size() - 1].get(),
		m_pResourceManager->getAnimationManagers()[m_pResourceManager->getAnimationManagers().size() - 1].get()));

	for (auto& o : m_pResourceManager->getSkinningObjectList().back()->getObjects()) {
		for (auto& ma : o->getMaterials())
			ma.m_bHasEmissiveColor = false;
	}

	m_vMonsters[m_vMonsters.size() - 1]->getObject()->setPreTransform(3.0f, XMFLOAT3(), XMFLOAT3());
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->SetPosition(XMFLOAT3(-306.7f, 0.0f, -150.8f));
	m_vMonsters[m_vMonsters.size() - 1]->getObject()->Rotate(XMFLOAT3(0.0f, 0.0f, 0.0f));

	m_pMonsters.push_back(std::make_unique<CMonster>(m_vMonsters[m_vMonsters.size() - 1].get()));

	{ // server object
		auto newMonster = std::make_unique<Monster>(m_nMonsterNum, MonsterType::Xenokarce);
		newMonster->setRenderingObject(m_vMonsters.back()->getObject());
		newMonster->setAnimationManager(m_vMonsters.back()->getAniManager());
		Monsters[m_nMonsterNum] = std::move(newMonster);
		++m_nMonsterNum;
	}
}

void CRaytracingETPScene::ProcessInput(float fElapsedTime)
{
	UCHAR keyBuffer[256];
	GetKeyboardState(keyBuffer);

	if (g_InGameState == IS_GAMING) {
		if (!m_pCamera->getThirdPersonState()) {
			bool shiftDown = false;
			if (keyBuffer[VK_SHIFT] & 0x80)
				shiftDown = true;
			if (keyBuffer['W'] & 0x80)
				m_pCamera->Move(0, fElapsedTime, shiftDown);
			if (keyBuffer['S'] & 0x80)
				m_pCamera->Move(5, fElapsedTime, shiftDown);
			if (keyBuffer['D'] & 0x80)
				m_pCamera->Move(3, fElapsedTime, shiftDown);
			if (keyBuffer['A'] & 0x80)
				m_pCamera->Move(4, fElapsedTime, shiftDown);
			if (keyBuffer[VK_SPACE] & 0x80)
				m_pCamera->Move(1, fElapsedTime, shiftDown);
			if (keyBuffer[VK_CONTROL] & 0x80)
				m_pCamera->Move(2, fElapsedTime, shiftDown);
		}
		else {
			if (!g_PlayerDie[m_local_id]) {
				KeyInputRet ret = m_pPlayer->ProcessInput(keyBuffer, fElapsedTime);
				UIUseSkill(ret);
				SkillParticleStart(ret);
				CAnimationManager* myManager = m_pPlayer->getAniManager();
				Client.SendMovePacket(myManager->getElapsedTime(), myManager->getCurrentSet());	// Check
			}
		}
	}
}

void CRaytracingETPScene::PrepareTerrainTexture()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NumDescriptors = 14;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	g_DxResource.device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_pTerrainDescriptor.GetAddressOf()));

	struct alignas(16) terrainINFO {
		int numLayer{};
		float padding[3]{};
		int bHasDiffuse[4]{};
		int bHasNormal[4]{};
		int bHasMask[4]{};
	};

	auto rdesc = BASIC_BUFFER_DESC;
	rdesc.Width = Align(sizeof(terrainINFO), 256);

	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &rdesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(m_pTerrainCB.GetAddressOf()));

	terrainINFO* pMap{};
	m_pTerrainCB->Map(0, nullptr, reinterpret_cast<void**>(&pMap));
	pMap->numLayer = 4;
	pMap->bHasDiffuse[0] = pMap->bHasDiffuse[1] = pMap->bHasDiffuse[2] = pMap->bHasDiffuse[3] = 1;
	pMap->bHasNormal[0] = pMap->bHasNormal[1] = pMap->bHasNormal[2] = pMap->bHasNormal[3] = 0;
	pMap->bHasMask[0] = pMap->bHasMask[2] = pMap->bHasMask[3] = 0;
	m_pTerrainCB->Unmap(0, nullptr);

	std::vector<std::unique_ptr<CTexture>>& textures = m_pResourceManager->getTextureList();
	size_t textureIndex = textures.size();
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Terrain_ETP_splatmap.dds"));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Map\\SandRiver00_Terrain.dds"));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Map\\Grassgreen00_Albedo.dds"));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Map\\GroundSoil00_Terrain.dds"));
	textures.emplace_back(std::make_unique<CTexture>(L"src\\texture\\Map\\dlnk_RockWall_00_Terrain.dds"));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	D3D12_RESOURCE_DESC d3dRD;

	UINT increment = g_DxResource.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE  handle = m_pTerrainDescriptor->GetCPUDescriptorHandleForHeapStart();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cdesc{};
	cdesc.BufferLocation = m_pTerrainCB->GetGPUVirtualAddress();
	cdesc.SizeInBytes = rdesc.Width;

	g_DxResource.device->CreateConstantBufferView(&cdesc, handle);
	handle.ptr += increment;

	// splat
	d3dRD = textures[textureIndex]->getTexture()->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;

	g_DxResource.device->CreateShaderResourceView(textures[textureIndex]->getTexture(), &srvDesc, handle);
	handle.ptr += increment;

	// layer 0 ===============================================================

	d3dRD = textures[textureIndex + 1]->getTexture()->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(textures[textureIndex + 1]->getTexture(), &srvDesc, handle);
	handle.ptr += increment;

	d3dRD = g_DxResource.nullTexture->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(g_DxResource.nullTexture.Get(), &srvDesc, handle);
	handle.ptr += increment;

	d3dRD = g_DxResource.nullTexture->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(g_DxResource.nullTexture.Get(), &srvDesc, handle);
	handle.ptr += increment;

	// layer 1 ===============================================================

	d3dRD = textures[textureIndex + 2]->getTexture()->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(textures[textureIndex + 2]->getTexture(), &srvDesc, handle);
	handle.ptr += increment;

	d3dRD = g_DxResource.nullTexture->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(g_DxResource.nullTexture.Get(), &srvDesc, handle);
	handle.ptr += increment;

	d3dRD = g_DxResource.nullTexture->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(g_DxResource.nullTexture.Get(), &srvDesc, handle);
	handle.ptr += increment;

	// layer 2 ===============================================================

	d3dRD = textures[textureIndex + 3]->getTexture()->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(textures[textureIndex + 3]->getTexture(), &srvDesc, handle);
	handle.ptr += increment;

	d3dRD = g_DxResource.nullTexture->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(g_DxResource.nullTexture.Get(), &srvDesc, handle);
	handle.ptr += increment;

	d3dRD = g_DxResource.nullTexture->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(g_DxResource.nullTexture.Get(), &srvDesc, handle);
	handle.ptr += increment;

	// layer 3 ===============================================================

	d3dRD = textures[textureIndex + 4]->getTexture()->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(textures[textureIndex + 4]->getTexture(), &srvDesc, handle);
	handle.ptr += increment;

	d3dRD = g_DxResource.nullTexture->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(g_DxResource.nullTexture.Get(), &srvDesc, handle);
	handle.ptr += increment;

	d3dRD = g_DxResource.nullTexture->GetDesc();
	srvDesc.Format = d3dRD.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = -1;
	g_DxResource.device->CreateShaderResourceView(g_DxResource.nullTexture.Get(), &srvDesc, handle);
	handle.ptr += increment;
}

void CRaytracingETPScene::UpdateObject(float fElapsedTime)
{
	// compute shader & rootSignature set
	g_DxResource.cmdList->SetPipelineState(m_pAnimationComputeShader.Get());
	g_DxResource.cmdList->SetComputeRootSignature(m_pComputeRootSignature.Get());

	m_pResourceManager->UpdateSkinningMesh(fElapsedTime);
	// particle update
	// SetRootSignature
	// particle update

	Flush();
	// Skinning Object BLAS ReBuild
	m_pResourceManager->ReBuildBLAS();

	m_pResourceManager->UpdateWorldMatrix();

	for (auto& m : m_vPlayers)
	{
		m->UpdateObject(fElapsedTime);

		if (m->CheckAC())
		{
			AttackCollision(m_vMonsters, m_vPlayers, 1);
		}
		if (m->HasActiveBullet())
		{
			ShootCollision(m_vMonsters, m_vPlayers, 1);
		}
	}

	for (auto& m : m_vMonsters)
	{
		m->UpdateObject(fElapsedTime);

		if (m->CheckAC())
		{
			AttackCollision(m_vPlayers, m_vMonsters, 0);
		}
		if (m->HasActiveBullet())
		{
			ShootCollision(m_vPlayers, m_vMonsters, 0);
		}
	}

	m_pPlayer->CollisionCheck(m_pRoadTerrain.get(), m_pCollisionHMap.get(), fElapsedTime, -512.0f, 0.0f, -512.0f, SCENE_PLAIN);
	m_pPlayer->HeightCheck(m_pRoadTerrain.get(), fElapsedTime, -512.0f, 0.0f, -512.0f, SCENE_PLAIN);

	BulletCheck(m_vPlayers, m_pRoadTerrain.get(), m_pCollisionHMap.get(), fElapsedTime, -512.0f, 0.0f, -512.0f, SCENE_PLAIN);

	for (auto& p : m_pMonsters) {
		p->CollisionCheck(m_pRoadTerrain.get(), m_pCollisionHMap.get(), fElapsedTime, -512.0f, 0.0f, -512.0f, SCENE_PLAIN);
		p->HeightCheck(m_pRoadTerrain.get(), fElapsedTime, -512.0f, 0.0f, -512.0f, SCENE_PLAIN);
	}

	BulletCheck(m_vMonsters, m_pRoadTerrain.get(), m_pCollisionHMap.get(), fElapsedTime, -512.0f, 0.0f, -512.0f, SCENE_PLAIN);

	AutoDirection(m_vPlayers, m_vMonsters);

	if (m_pCamera->getThirdPersonState()) {
		XMFLOAT3& EYE = m_pCamera->getEyeCalculateOffset();
		float cHeight = m_pHeightMap->GetHeightinWorldSpace(EYE.x + 512.0f, EYE.z + 512.0f);
		if (EYE.y < cHeight + 0.5f) {
			m_pCamera->UpdateViewMatrix(cHeight + 0.5f);
		}
		else
			m_pCamera->UpdateViewMatrix();
	}
	else
		m_pCamera->UpdateViewMatrix();

	g_DxResource.cmdList->SetGraphicsRootSignature(m_ParticleRS.Get());
	m_pCamera->SetElapsedTimeAndShader(fElapsedTime, 0);
	m_pResourceManager->UpdateParticles(fElapsedTime);

	m_pAccelerationStructureManager->UpdateScene(m_pCamera->getEye());

	switch (g_InGameState) {
	case IS_LOADING: {
		wOpacity -= 0.5 * fElapsedTime;
		if (wOpacity < 0.0f) {
			g_InGameState = IS_GAMING;
			wOpacity = 0.0f;
		}
		m_vUIs[0]->setColor(0.0, 0.0, 0.0, wOpacity);
		break;
	}
	case IS_GAMING: {
		break;
	}
	case IS_FINISH: {
		wOpacity += 0.2 * fElapsedTime;
		if (wOpacity > 1.0f) {
			ShowCursor(TRUE);
			g_pSoundManager->AllStop();
			m_nNextScene = SCENE_CAVE;
			wOpacity = 1.0f;
		}
		m_vUIs[0]->setColor(0.0, 0.0, 0.0, wOpacity);
		break;
	}
	}

	// Player UI ==================================================
	int buffstart = 20; int bstride = 40;
	float cMP = Players[m_local_id].GetMP();

	for (int i = 0; i < m_numUser; ++i) {
		int t{};
		// hp/mp
		m_vPlayersStatUI[i][1]->setScaleX(Players[i].GetHP() / g_maxHPs[i]);
		float  tt = Players[i].GetMP();
		float ttt = g_maxMPs[i];
		m_vPlayersStatUI[i][3]->setScaleXWithUV(Players[i].GetMP() / g_maxMPs[i]);
		if (i == m_local_id) {
			for (int j = 0; j < 3; ++j) {
				if (g_PlayerBuffState[j]) {
					m_vPlayersStatUI[i][j + 4]->setRenderState(true);
					m_vPlayersStatUI[i][j + 4]->setPositionInViewport(buffstart + (t * bstride), 100);
					++t;
				}
				else {
					m_vPlayersStatUI[i][j + 4]->setRenderState(false);
				}
			}
			m_vBossUIs[1]->setScaleX(Monsters[6]->getHP() / Monsters[6]->getMaxHP());
			XMFLOAT3 mpos = m_pMonsters.back()->getObject()->getObject()->getPosition(); mpos.y = 0;
			XMFLOAT3 ppos = Players[m_local_id].getRenderingObject()->getPosition(); ppos.y = 0;
			float distance;
			XMStoreFloat(&distance, XMVector3Length(XMLoadFloat3(&mpos) - XMLoadFloat3(&ppos)));
			if (distance <= 100.0f)
				m_bBossBattle = true;
			else
				m_bBossBattle = false;
		}
	}

	m_fItemCurTime -= fElapsedTime;
	if (m_fItemCurTime < 0.0)
		m_fItemCurTime = 0.0f;
	m_vItemUIs[4]->setScaleY(m_fItemCurTime / m_fItemCoolTime);

	{
		if (cMP < g_SkillCost[0])
			m_vSkillUIs[6]->setRenderState(true);
		else
			m_vSkillUIs[6]->setRenderState(false);

		if (cMP < g_SkillCost[1])
			m_vSkillUIs[7]->setRenderState(true);
		else
			m_vSkillUIs[7]->setRenderState(false);

		if (cMP < g_SkillCost[2])
			m_vSkillUIs[8]->setRenderState(true);
		else
			m_vSkillUIs[8]->setRenderState(false);

		for (int i = 0; i < 3; ++i) {
			g_SkillCurCTime[i] -= fElapsedTime;
			if (g_SkillCurCTime[i] < 0) g_SkillCurCTime[i] = 0.0f;
			m_vSkillUIs[i + 3]->setScaleY(g_SkillCurCTime[i] / g_SkillCoolTime[i]);
		}
	}
	// =================================================================

	if (g_PlayerDie[m_local_id])
		m_vUIs.back()->setColor(0.5, 0.5, 0.5, 0.5);
	else
		m_vUIs.back()->setColor(0.5, 0.5, 0.5, 0.0);
}

void CRaytracingETPScene::Render()
{
	m_pCamera->SetShaderVariable();
	m_pAccelerationStructureManager->SetScene();
	m_pResourceManager->SetLights();
	std::vector<std::unique_ptr<CTexture>>& textures = m_pResourceManager->getTextureList();
	g_DxResource.cmdList->SetComputeRootDescriptorTable(4, textures[m_nSkyboxIndex]->getView()->GetGPUDescriptorHandleForHeapStart());
	g_DxResource.cmdList->SetComputeRootDescriptorTable(5, m_pTerrainDescriptor->GetGPUDescriptorHandleForHeapStart());

	D3D12_DISPATCH_RAYS_DESC raydesc{};
	raydesc.Depth = 1;
	raydesc.Width = DEFINED_UAV_BUFFER_WIDTH;
	raydesc.Height = DEFINED_UAV_BUFFER_HEIGHT;

	raydesc.RayGenerationShaderRecord.StartAddress = m_pShaderBindingTable->getRayGenTable()->GetGPUVirtualAddress();
	raydesc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	raydesc.MissShaderTable.StartAddress = m_pShaderBindingTable->getMissTable()->GetGPUVirtualAddress();
	raydesc.MissShaderTable.SizeInBytes = m_pShaderBindingTable->getMissSize();
	raydesc.MissShaderTable.StrideInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

	raydesc.HitGroupTable.StartAddress = m_pShaderBindingTable->getHitGroupTable()->GetGPUVirtualAddress();
	raydesc.HitGroupTable.SizeInBytes = m_pShaderBindingTable->getHitGroupSize();
	raydesc.HitGroupTable.StrideInBytes = m_pShaderBindingTable->getHitGroupStride();

	g_DxResource.cmdList->DispatchRays(&raydesc);

	// UI Render ==================================================================================

	ID3D12GraphicsCommandList4* cmdList = g_DxResource.cmdList;
	auto barrier = [&](ID3D12Resource* pResource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
		{
			D3D12_RESOURCE_BARRIER resBarrier{};
			resBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			resBarrier.Transition.pResource = pResource;
			resBarrier.Transition.StateBefore = before;
			resBarrier.Transition.StateAfter = after;
			resBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			cmdList->ResourceBarrier(1, &resBarrier);
		};

	barrier(m_pOutputBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_VIEWPORT vv{};
	vv.Width = DEFINED_UAV_BUFFER_WIDTH; vv.Height = DEFINED_UAV_BUFFER_HEIGHT; vv.MinDepth = 0.0f; vv.MaxDepth = 1.0f;
	cmdList->RSSetViewports(1, &vv);
	D3D12_RECT ss{ 0, 0, DEFINED_UAV_BUFFER_WIDTH, DEFINED_UAV_BUFFER_HEIGHT };
	cmdList->RSSetScissorRects(1, &ss);
	cmdList->OMSetRenderTargets(1, &m_RTV->GetCPUDescriptorHandleForHeapStart(), FALSE, &m_DSV->GetCPUDescriptorHandleForHeapStart());
	cmdList->SetGraphicsRootSignature(m_UIRootSignature.Get());
	cmdList->SetPipelineState(m_UIPipelineState.Get());
	cmdList->SetGraphicsRootConstantBufferView(0, m_cameraCB->GetGPUVirtualAddress());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// player UI ====================================
	if (m_bUIOnOff) {
		for (int i = 0; i < m_numUser; ++i) {
			for (auto& p : m_vPlayersStatUI[i])
				p->Render();
		}

		if (m_bBossBattle)
			for (auto& p : m_vBossUIs)
				p->Render();

		for (auto& p : m_vItemUIs)
			p->Render();

		for (auto& p : m_vSkillUIs)
			p->Render();
	}
	// ===============================================


	for (auto& p : m_vUIs)	// black plane
		p->Render();

	barrier(m_pOutputBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}