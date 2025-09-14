#include "ResourceManager.h"

void CResourceManager::SetUp(unsigned int nLightRootParameterIndex)
{
	m_nLightRootParameterIndex = nLightRootParameterIndex;

	auto desc = BASIC_BUFFER_DESC;
	desc.Width = Align(sizeof(Lights), 256);

	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pLights.GetAddressOf()));
	Lights* mapptr{};
	m_pLights->Map(0, nullptr, reinterpret_cast<void**>(&mapptr));
	mapptr->numLights = 0;
	m_pLights->Unmap(0, nullptr);
}

bool CResourceManager::AddResourceFromFile(wchar_t* FilePath, std::string textureFilePathFront)
{
	FilePathFront = textureFilePathFront;
	std::ifstream inFile{ FilePath, std::ios::binary };
	if (!inFile) {
		OutputDebugString(L"Can't File Open!\n");
		return false;
	}
	std::string strLabel{};
	auto readLabel = [&]() {
		char nStrLength{};
		inFile.read(&nStrLength, sizeof(char));
		strLabel.assign(nStrLength, ' ');
		inFile.read(strLabel.data(), nStrLength);
		};

	while (!inFile.eof()) {
		readLabel();
		if (strLabel == "</Hierarchy>")
			break;
		else if (strLabel == "<Frame>:")
			AddGameObjectFromFile(inFile);
	}
	return true;
}

bool CResourceManager::AddSkinningResourceFromFile(wchar_t* FilePath, std::string textureFilePathFront, unsigned short job)
{
	std::ifstream inFile{ FilePath, std::ios::binary };
	if (!inFile) {
		OutputDebugString(L"Can't File Open!\n");
		return false;
	}
	std::string strLabel{};
	auto readLabel = [&]() {
		char nStrLength{};
		inFile.read(&nStrLength, sizeof(char));
		strLabel.assign(nStrLength, ' ');
		inFile.read(strLabel.data(), nStrLength);
		};
	m_vSkinningObject.emplace_back(std::make_unique<CRayTracingSkinningObject>());
	m_vSkinningObject[m_vSkinningObject.size() - 1]->AddResourceFromFile(inFile, textureFilePathFront);

	if (!inFile.eof()) {
		readLabel();
		if ("<Animation>:" == strLabel) {	// 애니메이션이 있으면 매니저 생성 & 오브젝트 지정
			switch (job) {
			case JOB_NOTHING:
				m_vAnimationManager.emplace_back(std::make_unique<CAnimationManager>(inFile));
				break;
			case JOB_MAGE:
				m_vAnimationManager.emplace_back(std::make_unique<CMageManager>(inFile));
				break;
			case JOB_WARRIOR:
				m_vAnimationManager.emplace_back(std::make_unique<CWarriorManager>(inFile));
				break;
			case JOB_HEALER:
				m_vAnimationManager.emplace_back(std::make_unique<CPriestManager>(inFile));
				break;
			case MONSTER:
				m_vAnimationManager.emplace_back(std::make_unique<CMonsterManager>(inFile));
				break;
			}
			m_vAnimationManager[m_vAnimationManager.size() - 1]->SetFramesPointerFromSkinningObject(m_vSkinningObject[m_vSkinningObject.size() - 1]->getObjects());
			m_vAnimationManager[m_vAnimationManager.size() - 1]->MakeAnimationMatrixIndex(m_vSkinningObject[m_vSkinningObject.size() - 1].get());
		}
	}

	return true;
}

void CResourceManager::AddGameObjectFromFile(std::ifstream& inFile, int nParentIndex)
{
	UINT nCurrentObjectIndex = m_vGameObjectList.size();
	m_vGameObjectList.emplace_back(std::make_unique<CGameObject>());
	m_vGameObjectList[nCurrentObjectIndex]->InitializeObjectFromFile(inFile);

	if (nParentIndex != -1) {		// 부모가 존재한다는 뜻
		m_vGameObjectList[nCurrentObjectIndex]->SetParentIndex(nParentIndex);
	}

	std::string strLabel{};
	auto readLabel = [&]() {
		char nStrLength{};
		inFile.read(&nStrLength, sizeof(char));
		strLabel.assign(nStrLength, ' ');
		inFile.read(strLabel.data(), nStrLength);
		};
	int tempData{};
	while (1) {
		readLabel();
		if (strLabel == "<Mesh>:") {
			inFile.read((char*)&tempData, sizeof(int));
			readLabel();	// 메시의 이름 읽기
			auto p = std::find_if(m_vMeshList.begin(), m_vMeshList.end(), [&](std::unique_ptr<Mesh>& tempMesh) {
				return tempMesh->getName() == strLabel;
				});
			if (p != m_vMeshList.end()) {	// 이미 리스트에 해당 이름을 가진 메시가 존재
				// 주의할게 이름이 아예 중복이 없는지는 아직 확인을 못함
				m_vGameObjectList[nCurrentObjectIndex]->SetMeshIndex(std::distance(m_vMeshList.begin(), p));
				// 중복메시 제거
				Mesh* tempMesh = new Mesh(inFile, strLabel);
				delete tempMesh;
			}
			else {	// 없으면 새로 생성과 동시에 인덱스 지정
				m_vGameObjectList[nCurrentObjectIndex]->SetMeshIndex(m_vMeshList.size());
				m_vMeshList.emplace_back(std::make_unique<Mesh>(inFile, strLabel));
				if (g_ShowBoundingBox) {
					int n = m_vGameObjectList[nCurrentObjectIndex]->getMeshIndex();
					std::unique_ptr<Mesh> boundingbox = std::make_unique<Mesh>(m_vMeshList[n]->getOBB().Center, m_vMeshList[n]->getOBB().Extents, m_vMeshList[n]->getName());
					m_vMeshList.pop_back();
					m_vMeshList.emplace_back(std::move(boundingbox));
				}
			}
		}
		else if (strLabel == "<Materials>:") {
			inFile.read((char*)&tempData, sizeof(int));
			AddMaterialFromFile(inFile, nCurrentObjectIndex);
			if (g_ShowBoundingBox) {
				m_vGameObjectList[nCurrentObjectIndex]->getMaterials().clear();
				Material tempM;
				tempM.m_bHasAlbedoColor = true; tempM.m_xmf4AlbedoColor = XMFLOAT4(g_unorm(g_dre), g_unorm(g_dre), g_unorm(g_dre), 0.5);	// 랜덤 컬러
				m_vGameObjectList[nCurrentObjectIndex]->getMaterials().emplace_back(tempM);
			}
		}
		else if (strLabel == "<Children>:") {
			inFile.read((char*)&tempData, sizeof(int));
			if (tempData > 0) {
				for (int i = 0; i < tempData; ++i) {
					readLabel();	// <Frame>: 부분을 빼준다
					AddGameObjectFromFile(inFile, nCurrentObjectIndex);
				}
			}
		}
		else if (strLabel == "</Frame>")
			break;
	}
}

void CResourceManager::AddMaterialFromFile(std::ifstream& inFile, int nCurrentIndex)
{
	std::string strLabel{};
	auto readLabel = [&]() {
		char nStrLength{};
		inFile.read(&nStrLength, sizeof(char));
		strLabel.assign(nStrLength, ' ');
		inFile.read(strLabel.data(), nStrLength);
		};
	int tempData{};
	std::vector<Material> vMaterials;

	std::string FilePathBack{ ".dds" };				// 포맷도 바뀔 수 있다.

	int nCurrentMaterial{};
	while (1) {
		readLabel();
		if (strLabel == "<Material>:") {
			nCurrentMaterial = vMaterials.size();
			vMaterials.emplace_back(Material());
			inFile.read((char*)&tempData, sizeof(int));
		}
		else if (strLabel == "<AlbedoColor>:") {
			vMaterials[nCurrentMaterial].m_bHasAlbedoColor = true;
			inFile.read((char*)&vMaterials[nCurrentMaterial].m_xmf4AlbedoColor, sizeof(XMFLOAT4));
		}
		else if (strLabel == "<EmissiveColor>:") {
			vMaterials[nCurrentMaterial].m_bHasEmissiveColor = true;
			inFile.read((char*)&vMaterials[nCurrentMaterial].m_xmf4EmissiveColor, sizeof(XMFLOAT4));
		}
		else if (strLabel == "<SpecularColor>:") {
			vMaterials[nCurrentMaterial].m_bHasSpecularColor = true;
			inFile.read((char*)&vMaterials[nCurrentMaterial].m_xmf4SpecularColor, sizeof(XMFLOAT4));
		}
		else if (strLabel == "<Glossiness>:") {
			vMaterials[nCurrentMaterial].m_bHasGlossiness = true;
			inFile.read((char*)&vMaterials[nCurrentMaterial].m_fGlossiness, sizeof(float));
		}
		else if (strLabel == "<Smoothness>:") {
			vMaterials[nCurrentMaterial].m_bHasSmoothness = true;
			inFile.read((char*)&vMaterials[nCurrentMaterial].m_fSmoothness, sizeof(float));
		}
		else if (strLabel == "<Metallic>:") {
			vMaterials[nCurrentMaterial].m_bHasMetallic = true;
			inFile.read((char*)&vMaterials[nCurrentMaterial].m_fMetallic, sizeof(float));
		}
		else if (strLabel == "<SpecularHighlight>:") {
			vMaterials[nCurrentMaterial].m_bHasSpecularHighlight = true;
			inFile.read((char*)&vMaterials[nCurrentMaterial].m_fSpecularHighlight, sizeof(float));
		}
		else if (strLabel == "<GlossyReflection>:") {
			vMaterials[nCurrentMaterial].m_bHasGlossyReflection = true;
			inFile.read((char*)&vMaterials[nCurrentMaterial].m_fGlossyReflection, sizeof(float));
		}
		else if (strLabel == "<AlbedoMap>:") {
			readLabel();
			if (strLabel[0] == '@') {	// 이미 존재함
				strLabel.erase(strLabel.begin());
				auto p = std::find_if(m_vTextureList.begin(), m_vTextureList.end(), [&](std::unique_ptr<CTexture>& txt) {
					return txt->getName() == strLabel;
					});
				vMaterials[nCurrentMaterial].m_bHasAlbedoMap = true;
				vMaterials[nCurrentMaterial].m_nAlbedoMapIndex = std::distance(m_vTextureList.begin(), p);
			}
			else if (strLabel == "null") {	// null이면 사용 안한단 뜻
				continue;
			}
			else {	// 없으니 새로 만들어라
				vMaterials[nCurrentMaterial].m_bHasAlbedoMap = true;
				vMaterials[nCurrentMaterial].m_nAlbedoMapIndex = m_vTextureList.size();
				std::string FilePath = FilePathFront + strLabel + FilePathBack;
				std::wstring wstr;
				wstr.assign(FilePath.begin(), FilePath.end());
				m_vTextureList.emplace_back(std::make_unique<CTexture>(wstr.c_str()));
				m_vTextureList[m_vTextureList.size() - 1]->SetTextureName(strLabel);
			}
		}
		else if (strLabel == "<SpecularMap>:") {
			readLabel();
			if (strLabel[0] == '@') {	// 이미 존재함
				strLabel.erase(strLabel.begin());
				auto p = std::find_if(m_vTextureList.begin(), m_vTextureList.end(), [&](std::unique_ptr<CTexture>& txt) {
					return txt->getName() == strLabel;
					});
				vMaterials[nCurrentMaterial].m_bHasSpecularMap = true;
				vMaterials[nCurrentMaterial].m_nSpecularMapIndex = std::distance(m_vTextureList.begin(), p);	// 여기
			}
			else if (strLabel == "null") {	// null이면 사용 안한단 뜻
				continue;
			}
			else {	// 없으니 새로 만들어라
				vMaterials[nCurrentMaterial].m_bHasSpecularMap = true;	// 여기
				vMaterials[nCurrentMaterial].m_nSpecularMapIndex = m_vTextureList.size();	// 여기
				std::string FilePath = FilePathFront + strLabel + FilePathBack;
				std::wstring wstr;
				wstr.assign(FilePath.begin(), FilePath.end());
				m_vTextureList.emplace_back(std::make_unique<CTexture>(wstr.c_str()));
				m_vTextureList[m_vTextureList.size() - 1]->SetTextureName(strLabel);
			}
		}
		else if (strLabel == "<MetallicMap>:") {
			readLabel();
			if (strLabel[0] == '@') {	// 이미 존재함
				strLabel.erase(strLabel.begin());
				auto p = std::find_if(m_vTextureList.begin(), m_vTextureList.end(), [&](std::unique_ptr<CTexture>& txt) {
					return txt->getName() == strLabel;
					});
				vMaterials[nCurrentMaterial].m_bHasMetallicMap = true;
				vMaterials[nCurrentMaterial].m_nMetallicMapIndex = std::distance(m_vTextureList.begin(), p);
			}
			else if (strLabel == "null") {	// null이면 사용 안한단 뜻
				continue;
			}
			else {	// 없으니 새로 만들어라
				vMaterials[nCurrentMaterial].m_bHasMetallicMap = true;
				vMaterials[nCurrentMaterial].m_nMetallicMapIndex = m_vTextureList.size();
				std::string FilePath = FilePathFront + strLabel + FilePathBack;
				std::wstring wstr;
				wstr.assign(FilePath.begin(), FilePath.end());
				m_vTextureList.emplace_back(std::make_unique<CTexture>(wstr.c_str()));
				m_vTextureList[m_vTextureList.size() - 1]->SetTextureName(strLabel);
			}
		}
		else if (strLabel == "<NormalMap>:") {
			readLabel();
			if (strLabel[0] == '@') {	// 이미 존재함
				strLabel.erase(strLabel.begin());
				auto p = std::find_if(m_vTextureList.begin(), m_vTextureList.end(), [&](std::unique_ptr<CTexture>& txt) {
					return txt->getName() == strLabel;
					});
				vMaterials[nCurrentMaterial].m_bHasNormalMap = true;
				vMaterials[nCurrentMaterial].m_nNormalMapIndex = std::distance(m_vTextureList.begin(), p);
			}
			else if (strLabel == "null") {	// null이면 사용 안한단 뜻
				continue;
			}
			else {	// 없으니 새로 만들어라
				vMaterials[nCurrentMaterial].m_bHasNormalMap = true;
				vMaterials[nCurrentMaterial].m_nNormalMapIndex = m_vTextureList.size();
				std::string FilePath = FilePathFront + strLabel + FilePathBack;
				std::wstring wstr;
				wstr.assign(FilePath.begin(), FilePath.end());
				m_vTextureList.emplace_back(std::make_unique<CTexture>(wstr.c_str()));
				m_vTextureList[m_vTextureList.size() - 1]->SetTextureName(strLabel);
			}
		}
		else if (strLabel == "<EmissionMap>:") {
			readLabel();
			if (strLabel[0] == '@') {	// 이미 존재함
				strLabel.erase(strLabel.begin());
				auto p = std::find_if(m_vTextureList.begin(), m_vTextureList.end(), [&](std::unique_ptr<CTexture>& txt) {
					return txt->getName() == strLabel;
					});
				vMaterials[nCurrentMaterial].m_bHasEmissionMap = true;
				vMaterials[nCurrentMaterial].m_nEmissionMapIndex = std::distance(m_vTextureList.begin(), p);
			}
			else if (strLabel == "null") {	// null이면 사용 안한단 뜻
				continue;
			}
			else {	// 없으니 새로 만들어라
				vMaterials[nCurrentMaterial].m_bHasEmissionMap = true;
				vMaterials[nCurrentMaterial].m_nEmissionMapIndex = m_vTextureList.size();
				std::string FilePath = FilePathFront + strLabel + FilePathBack;
				std::wstring wstr;
				wstr.assign(FilePath.begin(), FilePath.end());
				m_vTextureList.emplace_back(std::make_unique<CTexture>(wstr.c_str()));
				m_vTextureList[m_vTextureList.size() - 1]->SetTextureName(strLabel);
			}
		}
		else if (strLabel == "<DetailAlbedoMap>:") {
			readLabel();
			if (strLabel[0] == '@') {	// 이미 존재함
				strLabel.erase(strLabel.begin());
				auto p = std::find_if(m_vTextureList.begin(), m_vTextureList.end(), [&](std::unique_ptr<CTexture>& txt) {
					return txt->getName() == strLabel;
					});
				vMaterials[nCurrentMaterial].m_bHasDetailAlbedoMap = true;
				vMaterials[nCurrentMaterial].m_nDetailAlbedoMapIndex = std::distance(m_vTextureList.begin(), p);
			}
			else if (strLabel == "null") {	// null이면 사용 안한단 뜻
				continue;
			}
			else {	// 없으니 새로 만들어라
				vMaterials[nCurrentMaterial].m_bHasDetailAlbedoMap = true;
				vMaterials[nCurrentMaterial].m_nDetailAlbedoMapIndex = m_vTextureList.size();
				std::string FilePath = FilePathFront + strLabel + FilePathBack;
				std::wstring wstr;
				wstr.assign(FilePath.begin(), FilePath.end());
				m_vTextureList.emplace_back(std::make_unique<CTexture>(wstr.c_str()));
				m_vTextureList[m_vTextureList.size() - 1]->SetTextureName(strLabel);
			}
		}
		else if (strLabel == "<DetailNormalMap>:") {
			readLabel();
			if (strLabel[0] == '@') {	// 이미 존재함
				strLabel.erase(strLabel.begin());
				auto p = std::find_if(m_vTextureList.begin(), m_vTextureList.end(), [&](std::unique_ptr<CTexture>& txt) {
					return txt->getName() == strLabel;
					});
				vMaterials[nCurrentMaterial].m_bHasDetailNormalMap = true;
				vMaterials[nCurrentMaterial].m_nDetailNormalMapIndex = std::distance(m_vTextureList.begin(), p);
			}
			else if (strLabel == "null") {	// 있더라고 null이면 사용 안한단 뜻
				continue;
			}
			else {	// 없으니 새로 만들어라
				vMaterials[nCurrentMaterial].m_bHasDetailNormalMap = true;
				vMaterials[nCurrentMaterial].m_nDetailNormalMapIndex = m_vTextureList.size();
				std::string FilePath = FilePathFront + strLabel + FilePathBack;
				std::wstring wstr;
				wstr.assign(FilePath.begin(), FilePath.end());
				m_vTextureList.emplace_back(std::make_unique<CTexture>(wstr.c_str()));
				m_vTextureList[m_vTextureList.size() - 1]->SetTextureName(strLabel);
			}
		}
		else if (strLabel == "</Materials>")
			break;
	}
	// 게임 오브젝트에 마테리얼 저장
	for (int i = 0; i < vMaterials.size(); ++i) {
		m_vGameObjectList[nCurrentIndex]->getMaterials().emplace_back(vMaterials[i]);
	}
}

void CResourceManager::InitializeGameObjectCBuffer()
{
	for (int i = 0; i < m_vGameObjectList.size(); ++i)
		m_vGameObjectList[i]->InitializeConstanctBuffer(m_vMeshList);

	for (int i = 0; i < m_vSkinningObject.size(); ++i)
		m_vSkinningObject[i]->InitializeGameObjectCBuffer();
}

void CResourceManager::PrepareObject()
{
	for (std::unique_ptr<CSkinningObject>& object : m_vSkinningObject)
		object->PrepareObject();
}

void CResourceManager::UpdateSkinningMesh(float fElapsedTime)
{
	// 애니메이션이 없는 스키닝 객체면 문제가 생김
	for (int i = 0; i < m_vAnimationManager.size(); ++i) {
		m_vAnimationManager[i]->TimeIncrease(fElapsedTime);
		m_vSkinningObject[i]->UpdateAnimationMatrixes();
		m_vAnimationManager[i]->UpdateAnimationMatrix();
		m_vSkinningObject[i]->UpdateObject(fElapsedTime);
	}
}

void CResourceManager::UpdatePosition(float fElapsedTime)
{
	for (size_t i = 0; i < m_vAnimationManager.size(); ++i) {
		if (m_vAnimationManager[i]) {
			CSkinningObject* skinningObject = getSkinningObjectList()[i].get();
			if (skinningObject) {
				m_vAnimationManager[i]->UpdateAniPosition(fElapsedTime, skinningObject);
			}
		}
	}
}

void CResourceManager::ReBuildBLAS()
{
	for (std::unique_ptr<CSkinningObject>& object : m_vSkinningObject)
		object->ReBuildBLAS();
}

void CResourceManager::UpdateWorldMatrix()
{
	for (std::unique_ptr<CGameObject>& object : m_vGameObjectList) {
		if (object->getParentIndex() != -1) {
			XMFLOAT4X4 wmtx = m_vGameObjectList[object->getParentIndex()]->getWorldMatrix();
			XMFLOAT4X4 lmtx = object->getLocalMatrix();
			XMStoreFloat4x4(&lmtx, XMLoadFloat4x4(&lmtx) * XMLoadFloat4x4(&wmtx));
			object->SetWorldMatrix(lmtx);
		}
		else {
			object->SetWorldMatrix(object->getLocalMatrix());
		}
	}
	for (std::unique_ptr<CSkinningObject>& Skinning : m_vSkinningObject) {
		Skinning->UpdateFrameWorldMatrix();
	}
}

void CResourceManager::UpdateParticles(float fElapsedTime)
{
	for (std::unique_ptr<CParticle>& particle : m_vParticleList)
		particle->UpdateObject(fElapsedTime);
}

std::vector<std::unique_ptr<CGameObject>>& CResourceManager::getGameObjectList()
{
	return m_vGameObjectList;
}
std::vector<CGameObject*> CResourceManager::getGameObjectPtrList()
{
	std::vector<CGameObject*> ptrList;
	for (const auto& obj : m_vGameObjectList) {
		if (obj)ptrList.push_back(obj.get());
	}
	return ptrList;
}
std::vector<Mesh*> CResourceManager::getMeshPtrList()
{
	std::vector<Mesh*> ptrList;
	for (const auto& obj : m_vMeshList) {
		if (obj)ptrList.push_back(obj.get());
	}
	return ptrList;
}
std::vector<std::unique_ptr<Mesh>>& CResourceManager::getMeshList()
{
	return m_vMeshList;
}
std::vector<std::unique_ptr<CTexture>>& CResourceManager::getTextureList()
{
	return m_vTextureList;
}

void CResourceManager::WinterLand_LightSetup()
{
	Lights* mapptr{};
	m_pLights->Map(0, nullptr, reinterpret_cast<void**>(&mapptr));
	//memcpy(mapptr, &testLight, sizeof(Lights));

	mapptr->lights[2].Intensity = 1.0f;
	mapptr->lights[12].Intensity = 1.0f;
	mapptr->lights[13].Intensity = 1.0f;
	mapptr->lights[14].Intensity = 0.5f;
	mapptr->lights[14].Range = 60;
	mapptr->lights[15].Intensity = 1.0f;
	mapptr->lights[15].Range = 60;
	mapptr->lights[16].Intensity = 1.0f;
	mapptr->lights[17].Intensity = 0.6f;
	m_pLights->Unmap(0, nullptr);
}

void CResourceManager::AddLightsFromFile(wchar_t* FilePath)
{
	std::ifstream inFile{ FilePath, std::ios::binary };
	if (!inFile) {
		OutputDebugString(L"Can't File Open!\n");
		return;
	}
	std::string strLabel{};
	auto readLabel = [&]() {
		char nStrLength{};
		inFile.read(&nStrLength, sizeof(char));
		strLabel.assign(nStrLength, ' ');
		inFile.read(strLabel.data(), nStrLength);
		};

	while (!inFile.eof()) {
		readLabel();
		if ("</Hierarchy>:" == strLabel)
			break;
		if ("<Frame>:" == strLabel)
			AddLightsFromFileRecursion(inFile);
	}
}

void CResourceManager::AddLightsFromFileRecursion(std::ifstream& inFile)
{
	std::string strLabel{};
	auto readLabel = [&]() {
		char nStrLength{};
		inFile.read(&nStrLength, sizeof(char));
		strLabel.assign(nStrLength, ' ');
		inFile.read(strLabel.data(), nStrLength);
		};

	int tempData{};
	inFile.read(reinterpret_cast<char*>(&tempData), sizeof(int));
	inFile.read(reinterpret_cast<char*>(&tempData), sizeof(int));
	readLabel();

	while (1) {
		readLabel();
		if ("</Frame>" == strLabel)
			break;
		if ("<Transform>:" == strLabel) {
			for (int i = 0; i < 13; ++i)
				inFile.read(reinterpret_cast<char*>(&tempData), sizeof(int));
		}
		else if ("<TransformMatrix>:" == strLabel) {
			for (int i = 0; i < 16; ++i)
				inFile.read(reinterpret_cast<char*>(&tempData), sizeof(int));
		}
		else if ("<Bound>:" == strLabel) {
			for (int i = 0; i < 6; ++i)
				inFile.read(reinterpret_cast<char*>(&tempData), sizeof(int));
		}
		else if ("<Mesh>:" == strLabel) {
			inFile.read(reinterpret_cast<char*>(&tempData), sizeof(int));
			readLabel();
			Mesh* tempMesh = new Mesh(inFile, "noName");
			delete tempMesh;
		}
		else if ("<Materials>:" == strLabel) {
			inFile.read((char*)&tempData, sizeof(int));
			while (1) {
				readLabel();
				if (strLabel == "<Material>:") {
					inFile.read(reinterpret_cast<char*>(&tempData), sizeof(int));
				}
				else if (strLabel == "<AlbedoColor>:") {
					for (int i = 0; i < 4; ++i)
						inFile.read(reinterpret_cast<char*>(&tempData), sizeof(int));
				}
				else if (strLabel == "<EmissiveColor>:") {
					for (int i = 0; i < 4; ++i)
						inFile.read(reinterpret_cast<char*>(&tempData), sizeof(int));;
				}
				else if (strLabel == "<SpecularColor>:") {
					for (int i = 0; i < 4; ++i)
						inFile.read(reinterpret_cast<char*>(&tempData), sizeof(int));
				}
				else if (strLabel == "<Glossiness>:") {
					inFile.read(reinterpret_cast<char*>(&tempData), sizeof(int));
				}
				else if (strLabel == "<Smoothness>:") {
					inFile.read(reinterpret_cast<char*>(&tempData), sizeof(int));
				}
				else if (strLabel == "<Metallic>:") {
					inFile.read(reinterpret_cast<char*>(&tempData), sizeof(int));
				}
				else if (strLabel == "<SpecularHighlight>:") {
					inFile.read(reinterpret_cast<char*>(&tempData), sizeof(int));
				}
				else if (strLabel == "<GlossyReflection>:") {
					inFile.read(reinterpret_cast<char*>(&tempData), sizeof(int));
				}
				else if (strLabel == "<AlbedoMap>:") {
					readLabel();
				}
				else if (strLabel == "<SpecularMap>:") {
					readLabel();
				}
				else if (strLabel == "<MetallicMap>:") {
					readLabel();
				}
				else if (strLabel == "<NormalMap>:") {
					readLabel();
				}
				else if (strLabel == "<EmissionMap>:") {
					readLabel();
				}
				else if (strLabel == "<DetailAlbedoMap>:") {
					readLabel();
				}
				else if (strLabel == "<DetailNormalMap>:") {
					readLabel();
				}
				else if (strLabel == "</Materials>")
					break;
			}
		}
		else if ("<Children>:" == strLabel) {
			int nChild{};
			inFile.read(reinterpret_cast<char*>(&nChild), sizeof(int));
			if (nChild > 0) {
				for (int i = 0; i < nChild; ++i) {
					readLabel();
					AddLightsFromFileRecursion(inFile);
				}
			}
		}
		else if ("<Light>:" == strLabel) {
			readLabel();
			size_t myIndex = m_vLights.size();
			m_vLights.emplace_back();
			while (1) {
				readLabel();
				if ("</Light>" == strLabel)
					break;
				if ("<Type>:" == strLabel) {
					readLabel();
					if ("Directional" == strLabel)
						m_vLights[myIndex].Type = DIRECTIONAL_LIGHT;
					else if ("Point" == strLabel)
						m_vLights[myIndex].Type = POINT_LIGHT;
					else if ("Spot" == strLabel)
						m_vLights[myIndex].Type = SPOT_LIGHT;
					continue;
				}
				if ("<Position>:" == strLabel) {
					inFile.read(reinterpret_cast<char*>(&m_vLights[myIndex].Position), sizeof(XMFLOAT3));
					continue;
				}
				if ("<Direction>:" == strLabel) {
					inFile.read(reinterpret_cast<char*>(&m_vLights[myIndex].Direction), sizeof(XMFLOAT3));
					continue;
				}
				if ("<Color>:" == strLabel) {
					inFile.read(reinterpret_cast<char*>(&m_vLights[myIndex].Color), sizeof(XMFLOAT4));
					continue;
				}
				if ("<Intensity>:" == strLabel) {
					inFile.read(reinterpret_cast<char*>(&m_vLights[myIndex].Intensity), sizeof(float));
					continue;
				}
				if ("<Range>:" == strLabel) {
					inFile.read(reinterpret_cast<char*>(&m_vLights[myIndex].Range), sizeof(float));
					continue;
				}
				if ("<SpotAngle>:" == strLabel) {
					inFile.read(reinterpret_cast<char*>(&m_vLights[myIndex].SpotAngle), sizeof(float));
					continue;
				}
			}
		}
	}
}

void CResourceManager::ReadyLightBufferContent()
{
	Lights* mapptr{};
	m_pLights->Map(0, nullptr, reinterpret_cast<void**>(&mapptr));
	mapptr->numLights = m_vLights.size();
	memcpy(mapptr->lights, m_vLights.data(), sizeof(Light) * m_vLights.size());
	m_pLights->Unmap(0, nullptr);
}

void CResourceManager::PostProcess()
{
	for (std::unique_ptr<CParticle>& particle : m_vParticleList)
		particle->PostProcess();
}