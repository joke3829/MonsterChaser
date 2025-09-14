#include "AnimationManager.h"
#include "algorithm"

CAnimationSet::CAnimationSet(std::ifstream& inFile, UINT nBones)
{
	UINT tempInt{};
	float tempFloat{};
	std::string strLabel{};
	auto readLabel = [&]() {
		char nStrLength{};
		inFile.read(&nStrLength, sizeof(char));
		strLabel.assign(nStrLength, ' ');
		inFile.read(strLabel.data(), nStrLength);
		};

	readLabel();	// <AnimationSet>:
	inFile.read((char*)&tempInt, sizeof(UINT));

	readLabel();
	m_AnimationName = strLabel;

	inFile.read((char*)&m_fLength, sizeof(float));
	inFile.read((char*)&m_nFramePerSec, sizeof(int));
	inFile.read((char*)&m_nKeyFrame, sizeof(int));

	m_vTransforms.assign(m_nKeyFrame, std::vector<XMFLOAT4X4>(nBones));
	for (int i = 0; i < m_nKeyFrame; ++i) {
		std::vector<XMFLOAT4X4>& test = m_vTransforms[i];
		readLabel();		// <Transforms>:
		inFile.read((char*)&tempInt, sizeof(UINT));
		inFile.read((char*)&tempFloat, sizeof(float));
		m_vKeyTime.emplace_back(tempFloat);
		inFile.read((char*)test.data(), sizeof(XMFLOAT4X4) * nBones);
	}
}

void CAnimationSet::UpdateAnimationMatrix(std::vector<CGameObject*>& vMatrixes, float fElapsedTime)
{
	/*auto p = std::find_if(m_vKeyTime.begin(), m_vKeyTime.end(), [&](float& time) {return time >= fElapsedTime; });
	if (p == m_vKeyTime.end())
		p -= 1;
	else if (p == m_vKeyTime.begin())
		p += 1;
	UINT index = std::distance(m_vKeyTime.begin(), p) - 1;*/
	// 수정 제안
	auto p = std::lower_bound(m_vKeyTime.begin(), m_vKeyTime.end(), fElapsedTime);

	// 애니메이션 시간 끝에 도달했을 때
	if (p == m_vKeyTime.end()) {
		// 마지막에서 두 번째 프레임을 index로 사용해 보간하거나, 그냥 마지막 프레임으로 고정
		p--; // 마지막 요소로 이동
	}
	// 애니메이션 시간 시작 전일 때
	if (p == m_vKeyTime.begin()) {
		p++; // 두 번째 요소로 이동해야 아래 로직이 안전함
	}

	UINT index = std::distance(m_vKeyTime.begin(), p) - 1;

	// index가 유효한 범위인지 최종 확인
	if (index >= m_vKeyTime.size() - 1) {
		index = m_vKeyTime.size() - 2; // 마지막 보간 가능한 인덱스로 클램핑
		if (m_vKeyTime.size() < 2) index = 0; // 프레임이 2개 미만일 때 예외 처리
	}
	// t (0 ~ 1)
	//float t = (fElapsedTime - m_vKeyTime[index]) / (m_vKeyTime[index + 1] - m_vKeyTime[index]);
	// 수정 제안
	float frameDuration = m_vKeyTime[index + 1] - m_vKeyTime[index];
	float t = 0.0f;
	if (frameDuration > 1e-6f) { // 매우 작은 값(epsilon)보다 클 때만 계산
		t = (fElapsedTime - m_vKeyTime[index]) / frameDuration;
	}
	// 행렬 보간
	for (int i = 0; i < vMatrixes.size(); ++i) {
		XMFLOAT4X4 xmf{};
		XMVECTOR S0, R0, T0, S1, R1, T1;
		XMMatrixDecompose(&S0, &R0, &T0, XMLoadFloat4x4(&m_vTransforms[index][i]));
		XMMatrixDecompose(&S1, &R1, &T1, XMLoadFloat4x4(&m_vTransforms[index + 1][i]));

		XMVECTOR defaultVec = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

		R0 = XMVectorSelect(R0, defaultVec, XMVectorIsNaN(R0));
		R1 = XMVectorSelect(R1, defaultVec, XMVectorIsNaN(R1));
		T0 = XMVectorSelect(T0, defaultVec, XMVectorIsNaN(T0));
		T1 = XMVectorSelect(T1, defaultVec, XMVectorIsNaN(T1));
		S0 = XMVectorSelect(S0, XMVectorReplicate(1.0f), XMVectorIsNaN(S0));
		S1 = XMVectorSelect(S1, XMVectorReplicate(1.0f), XMVectorIsNaN(S1));

		XMVECTOR S = XMVectorLerp(S0, S1, t);
		XMVECTOR T = XMVectorLerp(T0, T1, t);
		XMVECTOR R = XMQuaternionSlerp(R0, R1, t);
		//XMStoreFloat4x4(&xmf, XMMatrixTranspose(XMMatrixAffineTransformation(S, XMVectorZero(), R, T)));
		XMStoreFloat4x4(&xmf, XMMatrixAffineTransformation(S, XMVectorZero(), R, T));
		vMatrixes[i]->SetLocalMatrix(xmf);
	}
}

void CAnimationSet::BlendAnimationMatrix(std::vector<CGameObject*>& vMatrixes, float fElapsedTime, std::vector<XMFLOAT4X4>& outMatrices)
{
	/*auto p = std::find_if(m_vKeyTime.begin(), m_vKeyTime.end(), [&](float& time) { return time >= fElapsedTime; });
	if (p == m_vKeyTime.end())
		p -= 1;
	else if (p == m_vKeyTime.begin())
		p += 1;
	UINT index = std::distance(m_vKeyTime.begin(), p) - 1;*/
	// 수정 제안
	auto p = std::lower_bound(m_vKeyTime.begin(), m_vKeyTime.end(), fElapsedTime);

	// 애니메이션 시간 끝에 도달했을 때
	if (p == m_vKeyTime.end()) {
		// 마지막에서 두 번째 프레임을 index로 사용해 보간하거나, 그냥 마지막 프레임으로 고정
		p--; // 마지막 요소로 이동
	}
	// 애니메이션 시간 시작 전일 때
	if (p == m_vKeyTime.begin()) {
		p++; // 두 번째 요소로 이동해야 아래 로직이 안전함
	}

	UINT index = std::distance(m_vKeyTime.begin(), p) - 1;

	// index가 유효한 범위인지 최종 확인
	if (index >= m_vKeyTime.size() - 1) {
		index = m_vKeyTime.size() - 2; // 마지막 보간 가능한 인덱스로 클램핑
		if (m_vKeyTime.size() < 2) index = 0; // 프레임이 2개 미만일 때 예외 처리
	}
	// t (0 ~ 1)
	//float t = (fElapsedTime - m_vKeyTime[index]) / (m_vKeyTime[index + 1] - m_vKeyTime[index]);
	// 수정 제안
	float frameDuration = m_vKeyTime[index + 1] - m_vKeyTime[index];
	float t = 0.0f;
	if (frameDuration > 1e-6f) { // 매우 작은 값(epsilon)보다 클 때만 계산
		t = (fElapsedTime - m_vKeyTime[index]) / frameDuration;
	}
	// 행렬 보간
	for (int i = 0; i < vMatrixes.size(); ++i) {
		XMFLOAT4X4 xmf{};
		XMVECTOR S0, R0, T0, S_forest, S1, R1, T1;
		XMMatrixDecompose(&S0, &R0, &T0, XMLoadFloat4x4(&m_vTransforms[index][i]));
		XMMatrixDecompose(&S1, &R1, &T1, XMLoadFloat4x4(&m_vTransforms[index + 1][i]));

		XMVECTOR defaultVec = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		R0 = XMVectorSelect(R0, defaultVec, XMVectorIsNaN(R0));
		R1 = XMVectorSelect(R1, defaultVec, XMVectorIsNaN(R1));
		T0 = XMVectorSelect(T0, defaultVec, XMVectorIsNaN(T0));
		T1 = XMVectorSelect(T1, defaultVec, XMVectorIsNaN(T1));
		S0 = XMVectorSelect(S0, XMVectorReplicate(1.0f), XMVectorIsNaN(S0));
		S1 = XMVectorSelect(S1, XMVectorReplicate(1.0f), XMVectorIsNaN(S1));

		XMVECTOR S = XMVectorLerp(S0, S1, t);
		XMVECTOR T = XMVectorLerp(T0, T1, t);
		XMVECTOR R = XMQuaternionSlerp(R0, R1, t);
		XMStoreFloat4x4(&xmf, XMMatrixAffineTransformation(S, XMVectorZero(), R, T));
		outMatrices[i] = xmf;
	}
}

// ====================================================================================

CAnimationManager::CAnimationManager(std::ifstream& inFile)
{
	UINT tempInt{};
	std::string strLabel{};
	auto readLabel = [&]() {
		char nStrLength{};
		inFile.read(&nStrLength, sizeof(char));
		strLabel.assign(nStrLength, ' ');
		inFile.read(strLabel.data(), nStrLength);
		};

	while (1) {
		readLabel();
		if ("</Animation>" == strLabel)
			break;
		else if ("<AnimationSets>:" == strLabel) {
			inFile.read((char*)&m_nAnimationSets, sizeof(UINT));
			// <FrameNames>:
			readLabel();
			inFile.read((char*)&tempInt, sizeof(UINT));
			for (int i = 0; i < tempInt; ++i) {
				readLabel();
				m_vFrameNames.emplace_back(strLabel);
			}
			for (int i = 0; i < m_nAnimationSets; ++i)
				m_vAnimationSets.emplace_back(std::make_shared<CAnimationSet>(inFile, m_vFrameNames.size()));
		}
	}
	m_vMatrixes.assign(m_vFrameNames.size(), XMFLOAT4X4());
	auto desc = BASIC_BUFFER_DESC;
	desc.Width = sizeof(XMFLOAT4X4) * m_vFrameNames.size();
	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pMatrixBuffer.GetAddressOf()));
	m_pMatrixBuffer->Map(0, nullptr, &m_pMappedPointer);
}

CAnimationManager::CAnimationManager(const CAnimationManager& other)
{
	m_nAnimationSets = other.m_nAnimationSets;
	for (int i = 0; i < other.m_vFrameNames.size(); ++i)
		m_vFrameNames.emplace_back(other.m_vFrameNames[i]);
	for (int i = 0; i < m_nAnimationSets; ++i)
		m_vAnimationSets.emplace_back(other.m_vAnimationSets[i]);

	m_vMatrixes.assign(m_vFrameNames.size(), XMFLOAT4X4());
	auto desc = BASIC_BUFFER_DESC;
	desc.Width = sizeof(XMFLOAT4X4) * m_vFrameNames.size();
	g_DxResource.device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pMatrixBuffer.GetAddressOf()));
	m_pMatrixBuffer->Map(0, nullptr, &m_pMappedPointer);
}

void CAnimationManager::SetFramesPointerFromSkinningObject(std::vector<std::unique_ptr<CGameObject>>& vObjects)
{
	for (std::string& name : m_vFrameNames) {
		auto p = std::find_if(vObjects.begin(), vObjects.end(), [&](std::unique_ptr<CGameObject>& frame) {
			return frame->getFrameName() == name;
			});
		if (p != vObjects.end()) {
			m_vFrames.emplace_back((*p).get());
		}
	}
}

void CAnimationManager::MakeAnimationMatrixIndex(CSkinningObject* pSkinningObject)
{
	std::vector<std::unique_ptr<CSkinningInfo>>& skinningInfo = pSkinningObject->getSkinningInfo();
	for (std::unique_ptr<CSkinningInfo>& p : skinningInfo) {
		p->MakeAnimationMatrixIndex(m_vFrameNames);
		p->MakeBufferAndDescriptorHeap(m_pMatrixBuffer, m_vMatrixes.size());
	}
}

void CAnimationManager::TimeIncrease(float fElapsedTime)
{
	/*m_fElapsedTime += fElapsedTime;
	float length = m_vAnimationSets[m_nCurrnetSet]->getLength();
	while (m_fElapsedTime > length)
		m_fElapsedTime -= length;
	m_vAnimationSets[m_nCurrnetSet]->UpdateAnimationMatrix(m_vFrames, m_fElapsedTime);*/
	m_fElapsedTime += fElapsedTime; // 시간 누적
	float length = m_vAnimationSets[m_nCurrentSet]->getLength();

	if (m_bIsBlending) {
		m_fBlendTime += fElapsedTime;
		if (m_fBlendTime >= m_fBlendDuration) {
			m_bIsBlending = false; // end blending
		}
	}

	if (m_bPlayOnce) {
		if (m_fElapsedTime >= length) {
			m_fElapsedTime = length; // 한 번 재생 시 종료
		}
	}
	else {
		if (m_fElapsedTime > length) {
			m_fElapsedTime -= length;
		}
	}

	if (m_bIsBlending) {
		// calculate animation matrix
		std::vector<XMFLOAT4X4> prevMatrices(m_vFrames.size());
		std::vector<XMFLOAT4X4> currMatrices(m_vFrames.size());
		//---------------------------------------------------------------------------------------------------
		m_vAnimationSets[m_nPrevSet]->BlendAnimationMatrix(m_vFrames, m_fElapsedTime, prevMatrices);	// Doyuoug you can do it
		//---------------------------------------------------------------------------------------------------
		m_vAnimationSets[m_nCurrentSet]->BlendAnimationMatrix(m_vFrames, m_fElapsedTime, currMatrices);

		// calculate animation blend weight
		float blendWeight = m_fBlendTime / m_fBlendDuration;

		// 행렬 보간
		for (size_t i = 0; i < m_vFrames.size(); ++i) {
			XMFLOAT4X4 blendedMatrix;
			XMVECTOR S0, R0, T0, S1, R1, T1;
			XMMatrixDecompose(&S0, &R0, &T0, XMLoadFloat4x4(&prevMatrices[i]));
			XMMatrixDecompose(&S1, &R1, &T1, XMLoadFloat4x4(&currMatrices[i]));

			XMVECTOR defaultVec = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
			R0 = XMVectorSelect(R0, defaultVec, XMVectorIsNaN(R0));
			R1 = XMVectorSelect(R1, defaultVec, XMVectorIsNaN(R1));
			T0 = XMVectorSelect(T0, defaultVec, XMVectorIsNaN(T0));
			T1 = XMVectorSelect(T1, defaultVec, XMVectorIsNaN(T1));
			S0 = XMVectorSelect(S0, XMVectorReplicate(1.0f), XMVectorIsNaN(S0));
			S1 = XMVectorSelect(S1, XMVectorReplicate(1.0f), XMVectorIsNaN(S1));

			XMVECTOR S = XMVectorLerp(S0, S1, blendWeight);
			XMVECTOR T = XMVectorLerp(T0, T1, blendWeight);
			XMVECTOR R = XMQuaternionSlerp(R0, R1, blendWeight);

			XMStoreFloat4x4(&blendedMatrix, XMMatrixAffineTransformation(S, XMVectorZero(), R, T));
			m_vFrames[i]->SetLocalMatrix(blendedMatrix);
		}
	}
	else {
		m_vAnimationSets[m_nCurrentSet]->UpdateAnimationMatrix(m_vFrames, m_fElapsedTime);
	}
}

void CAnimationManager::UpdateAnimation(float fElapsedTime)
{
	/*m_fElapsedTime = fElapsedTime;
	float length = m_vAnimationSets[m_nCurrnetSet]->getLength();
	while (m_fElapsedTime > length)
		m_fElapsedTime -= length;
	m_vAnimationSets[m_nCurrnetSet]->UpdateAnimationMatrix(m_vFrames, m_fElapsedTime);*/
	m_fElapsedTime = fElapsedTime;
	float length = m_vAnimationSets[m_nCurrentSet]->getLength();
	if (m_bPlayOnce) {
		if (m_fElapsedTime >= length) {
			m_fElapsedTime = length;
		}
	}
	else {
		while (m_fElapsedTime > length) {
			m_fElapsedTime -= length;
		}
	}
	m_vAnimationSets[m_nCurrentSet]->UpdateAnimationMatrix(m_vFrames, m_fElapsedTime);
}

void CAnimationManager::UpdateAnimationMatrix()
{
	for (int i = 0; i < m_vMatrixes.size(); ++i)
		XMStoreFloat4x4(&m_vMatrixes[i], XMMatrixTranspose(XMLoadFloat4x4(&m_vFrames[i]->getAnimationMatrix())));
	memcpy(m_pMappedPointer, m_vMatrixes.data(), sizeof(XMFLOAT4X4) * m_vMatrixes.size());
}

void CAnimationManager::ChangeAnimation(UINT nSet)
{
	if (nSet >= m_nAnimationSets)
		nSet = 0;
	if (nSet != m_nCurrentSet) {
		m_nCurrentSet = nSet;
		m_fElapsedTime = 0.0f;
	}
}

void CAnimationManager::ChangeAnimation(UINT nSet, bool playOnce)
{
	if (nSet >= m_nAnimationSets)
		nSet = 0;
	if (nSet != m_nCurrentSet || m_bPlayOnce != playOnce) { // playOnce 변경도 반영
		m_nPrevSet = m_nCurrentSet;
		m_nCurrentSet = nSet;
		m_bIsBlending = true;
		m_fBlendTime = 0.0f;
		m_fElapsedTime = 0.0f;
		m_bPlayOnce = playOnce;
	}
}

// =====================================================================================

void CPlayableCharacterAnimationManager::TimeIncrease(float fElapsedTime)
{
	/*m_fElapsedTime += fElapsedTime;
	float length = m_vAnimationSets[m_nCurrnetSet]->getLength();
	while (m_fElapsedTime > length)
		m_fElapsedTime -= length;
	m_vAnimationSets[m_nCurrnetSet]->UpdateAnimationMatrix(m_vFrames, m_fElapsedTime);*/
	m_fElapsedTime += fElapsedTime; // 시간 누적
	float length = m_vAnimationSets[m_nCurrentSet]->getLength();

	if (m_bIsBlending) {
		m_fBlendTime += fElapsedTime;
		if (m_fBlendTime >= m_fBlendDuration) {
			m_bIsBlending = false; // end blending
		}
	}

	if (m_bPlayOnce) {
		if (m_fElapsedTime >= length) {
			m_fElapsedTime = length; // 한 번 재생 시 종료
		}
	}
	else {
		if (m_fElapsedTime > length) {
			if (m_bDie) {
				m_fElapsedTime = length;
			}
			else
				m_fElapsedTime -= length;
		}
	}

	if (m_bIsBlending) {
		// calculate animation matrix
		std::vector<XMFLOAT4X4> prevMatrices(m_vFrames.size());
		std::vector<XMFLOAT4X4> currMatrices(m_vFrames.size());
		//---------------------------------------------------------------------------------------------------
		m_vAnimationSets[m_nPrevSet]->BlendAnimationMatrix(m_vFrames, m_fElapsedTime, prevMatrices);	// Doyuoug you can do it
		//---------------------------------------------------------------------------------------------------
		m_vAnimationSets[m_nCurrentSet]->BlendAnimationMatrix(m_vFrames, m_fElapsedTime, currMatrices);

		// calculate animation blend weight
		float blendWeight = m_fBlendTime / m_fBlendDuration;

		// 행렬 보간
		for (size_t i = 0; i < m_vFrames.size(); ++i) {
			XMFLOAT4X4 blendedMatrix;
			XMVECTOR S0, R0, T0, S1, R1, T1;
			XMMatrixDecompose(&S0, &R0, &T0, XMLoadFloat4x4(&prevMatrices[i]));
			XMMatrixDecompose(&S1, &R1, &T1, XMLoadFloat4x4(&currMatrices[i]));

			XMVECTOR defaultVec = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
			R0 = XMVectorSelect(R0, defaultVec, XMVectorIsNaN(R0));
			R1 = XMVectorSelect(R1, defaultVec, XMVectorIsNaN(R1));
			T0 = XMVectorSelect(T0, defaultVec, XMVectorIsNaN(T0));
			T1 = XMVectorSelect(T1, defaultVec, XMVectorIsNaN(T1));
			S0 = XMVectorSelect(S0, XMVectorReplicate(1.0f), XMVectorIsNaN(S0));
			S1 = XMVectorSelect(S1, XMVectorReplicate(1.0f), XMVectorIsNaN(S1));

			XMVECTOR S = XMVectorLerp(S0, S1, blendWeight);
			XMVECTOR T = XMVectorLerp(T0, T1, blendWeight);
			XMVECTOR R = XMQuaternionSlerp(R0, R1, blendWeight);

			XMStoreFloat4x4(&blendedMatrix, XMMatrixAffineTransformation(S, XMVectorZero(), R, T));
			m_vFrames[i]->SetLocalMatrix(blendedMatrix);
		}
	}
	else {
		m_vAnimationSets[m_nCurrentSet]->UpdateAnimationMatrix(m_vFrames, m_fElapsedTime);
	}
}

// =====================================================================================

void CMageManager::StartCombo()
{
	m_bInCombo = true;
	m_CurrentComboStep = 0;
	m_fComboTimer = 0.0f;
	m_bWaitingForNextInput = false;
	m_bNextAttack = false;

	m_vComboAnimationSets = { 22,23,24,25 };
	m_vSkillAnimationSets.clear();
	ChangeAnimation(m_vComboAnimationSets[m_CurrentComboStep], true);
}

void CMageManager::OnAttackInput()
{
	if (!m_bInCombo) {
		StartCombo();
		g_pSoundManager->StartFx(ESOUND::SOUND_WANDSWING);
		return;
	}

	// 콤보 진행
	if (m_vComboAnimationSets.size() > 0 && m_vComboAnimationSets[0] == 22) {
		m_bNextAttack = true;
		//if (!m_bWaitingForNextInput) {
		//	m_bNextAttack = true; // 다음 공격 대기
		//}
		//else {
		//	m_CurrentComboStep = (m_CurrentComboStep + 1) % m_vComboAnimationSets.size();
		//	ChangeAnimation(m_vComboAnimationSets[m_CurrentComboStep], true);
		//	m_bWaitingForNextInput = false;
		//	m_fComboTimer = 0.0f;
		//	m_bNextAttack = false;
		//}
	}
}

void CMageManager::UpdateCombo(float fElapsedTime)
{
	if (!m_bInCombo) return;

	/*if (m_vComboAnimationSets.size() > 0 && m_vComboAnimationSets[0] == 22) {
		if (m_bNextAttack) {
			m_CurrentComboStep = (m_CurrentComboStep + 1) % m_vComboAnimationSets.size();
			ChangeAnimation(m_vComboAnimationSets[m_CurrentComboStep], true);
			m_bNextAttack = false;
			m_fComboTimer = 0.0f;
		}
		else {
			m_bWaitingForNextInput = true;
			m_fComboTimer += fElapsedTime;
			if (m_fComboTimer >= m_fComboWaitTime) {
				ResetCombo();
			}
		}
	}
	else if (m_vSkillAnimationSets.size() > 0 && m_vSkillAnimationSets[0] == 26) {
		m_CurrentComboStep++;
		if (m_CurrentComboStep < m_vSkillAnimationSets.size()) {
			ChangeAnimation(m_vSkillAnimationSets[m_CurrentComboStep], true);
		}
		else {
			ResetCombo();
		}
		m_fComboTimer = 0.0f;
	}
	else {
		ResetCombo();
	}*/
	m_fComboTimer += fElapsedTime; // 타이머 증가

	if (m_vComboAnimationSets.size() > 0 && m_vComboAnimationSets[0] == 22) {
		if (m_bNextAttack && m_bWaitingForNextInput) {
			const float fComboCooldown = 0.4f;
			if (m_fComboTimer >= fComboCooldown) {
				m_CurrentComboStep = (m_CurrentComboStep + 1) % m_vComboAnimationSets.size();
				ChangeAnimation(m_vComboAnimationSets[m_CurrentComboStep], true);
				m_bNextAttack = false;
				m_bWaitingForNextInput = false;
				m_fComboTimer = 0.0f; // 타이머 초기화
				g_pSoundManager->StartFx(ESOUND::SOUND_WANDSWING);
			}
		}
		else {
			m_bWaitingForNextInput = true;
			if (m_fComboTimer >= m_fComboWaitTime) {
				ResetCombo();
			}
		}
	}
	else if (m_vSkillAnimationSets.size() > 0 && m_vSkillAnimationSets[0] == 26) {
		if (m_fComboTimer >= m_fComboWaitTime) {
			m_CurrentComboStep++;
			if (m_CurrentComboStep < m_vSkillAnimationSets.size()) {
				ChangeAnimation(m_vSkillAnimationSets[m_CurrentComboStep], true);
			}
			else {
				ResetCombo();
			}
			m_fComboTimer = 0.0f;
		}
	}
	else {
		ResetCombo();
	}
}

void CMageManager::ResetCombo()
{
	m_bComboEnd = (m_fComboTimer >= m_fComboWaitTime); // 중단 감지
	m_bInCombo = false;
	m_CurrentComboStep = 0;
	m_fComboTimer = 0.0f;
	m_bWaitingForNextInput = false;
	m_bNextAttack = false;
	m_vComboAnimationSets.clear();
	m_vSkillAnimationSets.clear();
	setTimeZero();
	ChangeAnimation(0, false); // idle로 전환
}

void CMageManager::StartSkill3()
{
	m_bInCombo = true;
	m_CurrentComboStep = 0;
	m_fComboTimer = 0.0f;
	m_bWaitingForNextInput = false;
	m_bNextAttack = false;

	m_vSkillAnimationSets = { 26, 27, 28, 29 , 30 };
	m_vComboAnimationSets.clear();
	ChangeAnimation(m_vSkillAnimationSets[m_CurrentComboStep], true);
}

void CMageManager::OnKey3Input()
{
	if (!m_bInCombo) {
		StartSkill3();
	}
}

void CMageManager::UpdateAniPosition(float fElapsedTime, CSkinningObject* player)
{
	if (m_vFrames[0]) {
		XMFLOAT3 targetPosition = m_vFrames[0]->getPositionFromWMatrix();
		player->SetPosition(targetPosition);
	}
}

void CMageManager::ChangeDie()
{
	ChangeAnimation(2, false);
}

void CMageManager::ChangeAlive()
{
	ChangeAnimation(0, false);
}

// ===============================================================================================

void CWarriorManager::StartCombo()
{
	m_bInCombo = true;
	m_CurrentComboStep = 0;
	m_fComboTimer = 0.0f;
	m_bWaitingForNextInput = false;
	m_bNextAttack = false;

	m_vComboAnimationSets = { 23,24,25 };
	ChangeAnimation(m_vComboAnimationSets[m_CurrentComboStep], true);
}

void CWarriorManager::OnAttackInput()
{
	if (!m_bInCombo) {
		StartCombo();
		g_pSoundManager->StartFx(ESOUND::SOUND_SLASH);
		return;
	}

	// 콤보 진행
	if (m_vComboAnimationSets.size() > 0 && m_vComboAnimationSets[0] == 23) {
		m_bNextAttack = true;
		//if (!m_bWaitingForNextInput) {
		//	m_bNextAttack = true; // 다음 공격 대기
		//}
		//else {
		//	m_CurrentComboStep = (m_CurrentComboStep + 1) % m_vComboAnimationSets.size();
		//	ChangeAnimation(m_vComboAnimationSets[m_CurrentComboStep], true);
		//	m_bWaitingForNextInput = false;
		//	m_fComboTimer = 0.0f;
		//	m_bNextAttack = false;
		//}
	}
}

void CWarriorManager::UpdateCombo(float fElapsedTime)
{
	if (!m_bInCombo) return;

	m_fComboTimer += fElapsedTime; // 타이머 증가

	if (m_vComboAnimationSets.size() > 0 && m_vComboAnimationSets[0] == 23) {
		if (m_bNextAttack && m_bWaitingForNextInput) {
			const float fComboCooldown = 0.1f;
			if (m_fComboTimer >= fComboCooldown) {
				m_CurrentComboStep = (m_CurrentComboStep + 1) % m_vComboAnimationSets.size();
				ChangeAnimation(m_vComboAnimationSets[m_CurrentComboStep], true);
				m_bNextAttack = false;
				m_bWaitingForNextInput = false;
				m_fComboTimer = 0.0f; // 타이머 초기화
				g_pSoundManager->StartFx(ESOUND::SOUND_SLASH);
			}
		}
		else {
			m_bWaitingForNextInput = true;
			if (m_fComboTimer >= m_fComboWaitTime) {
				ResetCombo();
			}
		}
	}
	else {
		ResetCombo();
	}
}

void CWarriorManager::ResetCombo()
{
	m_bComboEnd = (m_fComboTimer >= m_fComboWaitTime); // 중단 감지
	m_bInCombo = false;
	m_CurrentComboStep = 0;
	m_fComboTimer = 0.0f;
	m_bWaitingForNextInput = false;
	m_bNextAttack = false;
	m_vComboAnimationSets.clear();
	setTimeZero();
	ChangeAnimation(0, false); // idle로 전환
}

void CWarriorManager::UpdateAniPosition(float fElapsedTime, CSkinningObject* player)
{
	if (m_vFrames[0]) {
		XMFLOAT3 targetPosition = m_vFrames[0]->getPositionFromWMatrix();
		player->SetPosition(targetPosition);
	}
}

void CWarriorManager::ChangeDie()
{
	ChangeAnimation(3, false);
}

void CWarriorManager::ChangeAlive()
{
	ChangeAnimation(0, false);
}

// ==================================================================================

void CPriestManager::StartCombo()
{
	m_bInCombo = true;
	m_CurrentComboStep = 0;
	m_fComboTimer = 0.0f;
	m_bWaitingForNextInput = false;
	m_bNextAttack = false;

	m_vComboAnimationSets = { 22,23,24,25 };
	ChangeAnimation(m_vComboAnimationSets[m_CurrentComboStep], true);
}

void CPriestManager::OnAttackInput()
{
	if (!m_bInCombo) {
		StartCombo();
		g_pSoundManager->StartFx(ESOUND::SOUND_WANDSWING);
		return;
	}

	// 콤보 진행
	if (m_vComboAnimationSets.size() > 0 && m_vComboAnimationSets[0] == 22) {
		m_bNextAttack = true;
	}
}

void CPriestManager::UpdateCombo(float fElapsedTime)
{
	if (!m_bInCombo) return;

	m_fComboTimer += fElapsedTime; // 타이머 증가

	if (m_vComboAnimationSets.size() > 0 && m_vComboAnimationSets[0] == 22) {
		if (m_bNextAttack && m_bWaitingForNextInput) {
			const float fComboCooldown = 0.4f;
			if (m_fComboTimer >= fComboCooldown) {
				m_CurrentComboStep = (m_CurrentComboStep + 1) % m_vComboAnimationSets.size();
				ChangeAnimation(m_vComboAnimationSets[m_CurrentComboStep], true);
				m_bNextAttack = false;
				m_bWaitingForNextInput = false;
				m_fComboTimer = 0.0f; // 타이머 초기화
				g_pSoundManager->StartFx(ESOUND::SOUND_WANDSWING);
			}
		}
		else {
			m_bWaitingForNextInput = true;
			if (m_fComboTimer >= m_fComboWaitTime) {
				ResetCombo();
			}
		}
	}
	else if (m_vSkillAnimationSets.size() > 0 && m_vSkillAnimationSets[0] == 28) {
		if (m_fComboTimer >= m_fComboWaitTime) {
			m_CurrentComboStep++;
			if (m_CurrentComboStep < m_vSkillAnimationSets.size()) {
				ChangeAnimation(m_vSkillAnimationSets[m_CurrentComboStep], true);
			}
			else {
				ResetCombo();
			}
			m_fComboTimer = 0.0f;
		}
	}
	else {
		ResetCombo();
	}
}

void CPriestManager::ResetCombo()
{
	m_bComboEnd = (m_fComboTimer >= m_fComboWaitTime); // 중단 감지
	m_bInCombo = false;
	m_CurrentComboStep = 0;
	m_fComboTimer = 0.0f;
	m_bWaitingForNextInput = false;
	m_bNextAttack = false;
	m_vComboAnimationSets.clear();
	m_vSkillAnimationSets.clear();
	setTimeZero();
	ChangeAnimation(0, false); // idle로 전환
}

void CPriestManager::StartSkill3()
{
	m_bInCombo = true;
	m_CurrentComboStep = 0;
	m_fComboTimer = 0.0f;
	m_bWaitingForNextInput = false;
	m_bNextAttack = false;

	m_vSkillAnimationSets = { 28, 29 , 30 };
	m_vComboAnimationSets.clear();
	ChangeAnimation(m_vSkillAnimationSets[m_CurrentComboStep], true);
}

void CPriestManager::OnKey3Input()
{
	if (!m_bInCombo) {
		StartSkill3();
	}
}

void CPriestManager::UpdateAniPosition(float fElapsedTime, CSkinningObject* player)
{
	if (m_vFrames[0]) {
		XMFLOAT3 targetPosition = m_vFrames[0]->getPositionFromWMatrix();
		player->SetPosition(targetPosition);
	}
}
void CPriestManager::ChangeDie()
{
	ChangeAnimation(2, false);
}

void CPriestManager::ChangeAlive()
{
	ChangeAnimation(0, false);
}


void CMonsterManager::TimeIncrease(float fElapsedTime)
{
	m_fElapsedTime += fElapsedTime; // 시간 누적
	float length = m_vAnimationSets[m_nCurrentSet]->getLength();

	if (m_bIsBlending) {
		m_fBlendTime += fElapsedTime;
		if (m_fBlendTime >= m_fBlendDuration) {
			m_bIsBlending = false; // end blending
		}
	}

	if (m_bPlayOnce) {
		if (m_fElapsedTime >= length) {
			m_fElapsedTime = length; // 한 번 재생 시 종료
		}
	}
	else {
		if (m_fElapsedTime > length) {
			if (m_nCurrentSet == 0) 
				m_fElapsedTime = length;
			else
				m_fElapsedTime -= length;
		}
	}

	if (m_bIsBlending) {
		// calculate animation matrix
		std::vector<XMFLOAT4X4> prevMatrices(m_vFrames.size());
		std::vector<XMFLOAT4X4> currMatrices(m_vFrames.size());
		//---------------------------------------------------------------------------------------------------
		m_vAnimationSets[m_nPrevSet]->BlendAnimationMatrix(m_vFrames, m_fElapsedTime, prevMatrices);	// Doyuoug you can do it
		//---------------------------------------------------------------------------------------------------
		m_vAnimationSets[m_nCurrentSet]->BlendAnimationMatrix(m_vFrames, m_fElapsedTime, currMatrices);

		// calculate animation blend weight
		float blendWeight = m_fBlendTime / m_fBlendDuration;

		// 행렬 보간
		for (size_t i = 0; i < m_vFrames.size(); ++i) {
			XMFLOAT4X4 blendedMatrix;
			XMVECTOR S0, R0, T0, S1, R1, T1;
			XMMatrixDecompose(&S0, &R0, &T0, XMLoadFloat4x4(&prevMatrices[i]));
			XMMatrixDecompose(&S1, &R1, &T1, XMLoadFloat4x4(&currMatrices[i]));

			XMVECTOR defaultVec = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
			R0 = XMVectorSelect(R0, defaultVec, XMVectorIsNaN(R0));
			R1 = XMVectorSelect(R1, defaultVec, XMVectorIsNaN(R1));
			T0 = XMVectorSelect(T0, defaultVec, XMVectorIsNaN(T0));
			T1 = XMVectorSelect(T1, defaultVec, XMVectorIsNaN(T1));
			S0 = XMVectorSelect(S0, XMVectorReplicate(1.0f), XMVectorIsNaN(S0));
			S1 = XMVectorSelect(S1, XMVectorReplicate(1.0f), XMVectorIsNaN(S1));

			XMVECTOR S = XMVectorLerp(S0, S1, blendWeight);
			XMVECTOR T = XMVectorLerp(T0, T1, blendWeight);
			XMVECTOR R = XMQuaternionSlerp(R0, R1, blendWeight);

			XMStoreFloat4x4(&blendedMatrix, XMMatrixAffineTransformation(S, XMVectorZero(), R, T));
			m_vFrames[i]->SetLocalMatrix(blendedMatrix);
		}
	}
	else {
		m_vAnimationSets[m_nCurrentSet]->UpdateAnimationMatrix(m_vFrames, m_fElapsedTime);
	}
}