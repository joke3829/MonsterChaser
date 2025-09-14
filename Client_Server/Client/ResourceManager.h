#pragma once

//#include "GameObject.h"
#include "AnimationManager.h"
#include "Particle.h"

struct Light
{
	UINT    Type;
	XMFLOAT3  Position;
	float   Intensity;
	XMFLOAT3  Direction;
	float   Range;
	float   SpotAngle;
	XMFLOAT2 padding;
	XMFLOAT4  Color;
};

struct Lights
{
	UINT numLights;
	XMFLOAT3 padding;
	Light lights[MAX_LIGHTS];
};

extern DXResources g_DxResource;
extern std::default_random_engine g_dre;
extern std::uniform_real_distribution<float> g_unorm;

class CResourceManager {
public:
	void SetUp(unsigned int nLightRootParameterIndex);		// Light Buffer Ready
	bool AddResourceFromFile(wchar_t* FilePath, std::string textureFilePathFront);
	bool AddSkinningResourceFromFile(wchar_t* FilePath, std::string textureFilePathFront, unsigned short job = JOB_NOTHING);

	void AddGameObjectFromFile(std::ifstream& inFile, int nParentIndex = -1);
	void AddMaterialFromFile(std::ifstream& inFile, int nCurrentIndex);
	void InitializeGameObjectCBuffer();

	void PrepareObject();

	void UpdateSkinningMesh(float fElapsedTime);
	void UpdatePosition(float fElapsedTime);
	void ReBuildBLAS();
	void UpdateWorldMatrix();	// UpdateWorldMatrix
	void UpdateParticles(float fElapsedTime);

	void AddLightsFromFile(wchar_t* FilePath);
	void AddLightsFromFileRecursion(std::ifstream& inFile);
	void ReadyLightBufferContent();
	inline void SetLights() { g_DxResource.cmdList->SetComputeRootConstantBufferView(m_nLightRootParameterIndex, m_pLights->GetGPUVirtualAddress()); }

	void PostProcess();

	void WinterLand_LightSetup();
	// getter
	std::vector<std::unique_ptr<CGameObject>>& getGameObjectList();
	std::vector<CGameObject*> getGameObjectPtrList();
	std::vector<Mesh*> getMeshPtrList();
	std::vector<std::unique_ptr<Mesh>>& getMeshList();
	std::vector<std::unique_ptr<CTexture>>& getTextureList();
	std::vector<std::unique_ptr<CParticle>>& getParticleList() { return m_vParticleList; }

	std::vector<std::unique_ptr<CSkinningObject>>& getSkinningObjectList() { return m_vSkinningObject; }
	std::vector<std::unique_ptr<CAnimationManager>>& getAnimationManagers() { return m_vAnimationManager; }

	ComPtr<ID3D12Resource>& getLightBuffer() { return m_pLights; }

	// ================================================
private:
	std::string FilePathFront{};

	std::vector<std::unique_ptr<CGameObject>> m_vGameObjectList;
	std::vector<std::unique_ptr<Mesh>> m_vMeshList;
	std::vector<std::unique_ptr<CTexture>> m_vTextureList;
	std::vector<std::unique_ptr<CParticle>> m_vParticleList;

	// Lights
	ComPtr<ID3D12Resource> m_pLights;
	std::vector<Light> m_vLights{};
	UINT m_nLightRootParameterIndex{};

	// Skinning animation
	std::vector<std::unique_ptr<CSkinningObject>> m_vSkinningObject;
	std::vector<std::unique_ptr<CAnimationManager>> m_vAnimationManager;
};

