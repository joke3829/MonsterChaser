#pragma once
#include "stdafx.h"
#include "GameObject.h"
#include "SoundManager.h"

extern DXResources g_DxResource;
extern std::unique_ptr<CMonsterChaserSoundManager> g_pSoundManager;
extern std::array<bool, 3> g_PlayerDie;

class CAnimationSet {
public:
	CAnimationSet(std::ifstream& inFile, UINT nBones);

	void UpdateAnimationMatrix(std::vector<CGameObject*>& vMatrixes, float fElapsedTime);
	void BlendAnimationMatrix(std::vector<CGameObject*>& vMatrixes, float fElapsedTime, std::vector<XMFLOAT4X4>& outMatrices);

	float getLength() const { return m_fLength; }
	int getNumKeyFrame() const { return m_nKeyFrame; }
	std::vector<std::vector<XMFLOAT4X4>>& getTransforms() { return m_vTransforms; }

private:
	std::string m_AnimationName{};
	float m_fLength{};
	int m_nFramePerSec{};
	int m_nKeyFrame{};


	std::vector<std::vector<XMFLOAT4X4>> m_vTransforms{};	// Row : KeyFrame, Col : AnimationMatrix by Frames
	std::vector<float> m_vKeyTime{};	// Time by KeyFrame
};


class CAnimationManager {
public:
	CAnimationManager(std::ifstream& inFile);
	CAnimationManager(const CAnimationManager& other);
	virtual ~CAnimationManager() = default;

	void SetFramesPointerFromSkinningObject(std::vector<std::unique_ptr<CGameObject>>& vObjects);

	// Use Skinning Info to create indexes
	void MakeAnimationMatrixIndex(CSkinningObject* pSkinningObject);

	virtual void TimeIncrease(float fElapsedTime);
	void UpdateAnimation(float fElapsedTime);
	void UpdateAnimationMatrix();
	virtual void UpdateAniPosition(float fElapsedTime, CSkinningObject* player) {}
	void ChangeAnimation(UINT nSet);
	void ChangeAnimation(UINT nSet, bool playOnce = false); // playOnce 
	void setCurrnetSet(UINT n) { m_nCurrentSet = n; }

	std::shared_ptr<CAnimationSet>& getAnimationSet(int index) { return m_vAnimationSets[index]; }
	float getElapsedTime() const { return m_fElapsedTime; }
	UINT getCurrentSet() const { return m_nCurrentSet; }

	void setTimeZero() { m_fElapsedTime = 0.0f; }
	virtual bool IsAnimationFinished() const { return m_bPlayOnce && m_fElapsedTime >= m_vAnimationSets[m_nCurrentSet]->getLength(); }
	bool IsAnimationNearEnd(float margin = 0.05f) const
	{
		float length = m_vAnimationSets[m_nCurrentSet]->getLength();
		float remainingTime = length - m_fElapsedTime;
		return remainingTime <= margin && remainingTime >= 0.0f;
	}
	bool IsAnimationInTimeRange(float startTime, float endTime) const {
		float length = m_vAnimationSets[m_nCurrentSet]->getLength();
		return m_fElapsedTime >= startTime && m_fElapsedTime <= endTime && m_fElapsedTime <= length;
	}
	const std::vector<CGameObject*>& getFrame() const { return m_vFrames; }

	bool CheckCollision() const { return m_bCollision; }
	void IsCollision() { m_bCollision = true; }
protected:
	UINT m_nAnimationSets{};
	std::atomic<UINT> m_nCurrentSet{};
	float m_fElapsedTime{};
	std::vector<std::string> m_vFrameNames{};		// Bone Names
	std::vector<std::shared_ptr<CAnimationSet>> m_vAnimationSets{};

	std::vector<CGameObject*> m_vFrames{};	// Bones

	ComPtr<ID3D12Resource> m_pMatrixBuffer{};		// Matrix Buffer to be passed to shader
	void* m_pMappedPointer{};
	std::vector<XMFLOAT4X4> m_vMatrixes{};			// Matrix Buffer to be passed to shader

	bool m_bPlayOnce = false;
	bool m_bCollision = false;

	bool m_bIsBlending{};
	float m_fBlendTime{};         // blend time
	float m_fBlendDuration{};     // blend during time
	UINT m_nPrevSet = 0;            // previous animation set
};

class CPlayableCharacterAnimationManager : public CAnimationManager {
public:
	CPlayableCharacterAnimationManager(std::ifstream& inFile) : CAnimationManager(inFile) {}
	CPlayableCharacterAnimationManager(const CPlayableCharacterAnimationManager& other) : CAnimationManager(other) {}

	virtual void StartCombo() {}
	virtual void OnAttackInput() {}
	virtual void UpdateCombo(float fElapsedTime) {}
	virtual void ResetCombo() {}
	virtual void UpdateAniPosition(float fElapsedTime, CSkinningObject* player) {}
	virtual bool IsAnimationFinished() const { return m_bPlayOnce && m_fElapsedTime >= m_vAnimationSets[m_nCurrentSet]->getLength(); }

	virtual void StartSkill3() {};
	virtual void OnKey3Input() {};
	virtual int getSkillnum() { return 0; };

	void setDie(bool b) { m_bDie = b; }
	virtual void TimeIncrease(float fElapsedTime);
	virtual void ChangeDie() {}
	virtual void ChangeAlive() {}

	bool IsInCombo() const { return m_bInCombo; }
	bool IsComboInterrupted() const { return m_bComboEnd; }
	void ClearComboInterrupted() { m_bComboEnd = false; }
protected:
	bool m_bInCombo = false;
	int m_CurrentComboStep = 0;
	std::vector<UINT> m_vComboAnimationSets{};
	float m_fComboTimer = 0.0f;
	const float m_fComboWaitTime = 1.0f;
	bool m_bWaitingForNextInput = false;
	bool m_bNextAttack = false;
	bool m_bComboEnd = false;

	std::vector<UINT> m_vSkillAnimationSets{};

	bool m_bDie{};
};

class CMageManager : public CPlayableCharacterAnimationManager {
public:
	CMageManager(std::ifstream& inFile) : CPlayableCharacterAnimationManager(inFile)
	{
		m_vComboAnimationSets = { 22,23,24,25 }; m_vSkillAnimationSets = { 26, 27, 28, 29 , 30 };
	}

	virtual void StartCombo();
	virtual void OnAttackInput();
	virtual void UpdateCombo(float fElapsedTime);
	virtual void ResetCombo();

	virtual void StartSkill3();
	virtual void OnKey3Input();
	virtual void UpdateAniPosition(float fElapsedTime, CSkinningObject* player);

	virtual void ChangeDie();
	virtual void ChangeAlive();
};

class CWarriorManager : public CPlayableCharacterAnimationManager {
public:
	CWarriorManager(std::ifstream& inFile) : CPlayableCharacterAnimationManager(inFile)
	{
		m_vComboAnimationSets = { 23,24,25 }; m_vSkillAnimationSets = {  };
	}

	virtual void StartCombo();
	virtual void OnAttackInput();
	virtual void UpdateCombo(float fElapsedTime);
	virtual void ResetCombo();
	virtual void UpdateAniPosition(float fElapsedTime, CSkinningObject* player);

	virtual void ChangeDie();
	virtual void ChangeAlive();
protected:
	const float m_fComboWaitTime = 0.7f;
};

class CPriestManager : public CPlayableCharacterAnimationManager {
public:
	CPriestManager(std::ifstream& inFile) : CPlayableCharacterAnimationManager(inFile)
	{
		m_vComboAnimationSets = { 22,23,24,25 }; m_vSkillAnimationSets = { 28,29,30 };
	}

	virtual void StartCombo();
	virtual void OnAttackInput();
	virtual void UpdateCombo(float fElapsedTime);
	virtual void ResetCombo();

	virtual void StartSkill3();
	virtual void OnKey3Input();
	virtual void UpdateAniPosition(float fElapsedTime, CSkinningObject* player);

	virtual void ChangeDie();
	virtual void ChangeAlive();
};

class CMonsterManager : public CPlayableCharacterAnimationManager {
public:
	CMonsterManager(std::ifstream& inFile) : CPlayableCharacterAnimationManager(inFile){}
	CMonsterManager(const CMonsterManager& other) : CPlayableCharacterAnimationManager(other) {}

	virtual int getSkillnum() { 
		if (m_nCurrentSet == 3 || m_nCurrentSet == 0)
			return 1;
		if (m_nAnimationSets == 7)		//1 스테이지 잡몹
		{
			if (m_nCurrentSet == 6) {
				return 1;
			}
			else { return 0; }
		}
		else if (m_nAnimationSets == 8)		//1스테이지 보스 + 2스테이지 잡몹
		{
			if (m_nCurrentSet == 6) {
				return 1;
			}
			else if (m_nCurrentSet == 7) {
				return 2;
			}
			else { return 0; }
		}
		else if (m_nAnimationSets >= 9)			// 2,3스테이지 보스
		{
			if (m_nCurrentSet == 6) {
				return 1;
			}
			else if (m_nCurrentSet == 7) {
				return 2;
			}
			else if (m_nCurrentSet == 8) {
				return 3;
			}
			else { return 0; }
		}
	}		// not attack -> return 0
	void TimeIncrease(float fElapsedTime);
	bool IsAnimationFinished() const { return m_bPlayOnce && m_fElapsedTime >= m_vAnimationSets[m_nCurrentSet]->getLength(); }

	virtual void UpdateAniPosition(float fElapsedTime, CSkinningObject* player)
	{
		if (m_vFrames[0]) {
			XMFLOAT3 targetPosition = m_vFrames[0]->getPositionFromWMatrix();
			player->SetPosition(targetPosition);
		}
	}
};