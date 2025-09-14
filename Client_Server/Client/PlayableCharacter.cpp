#include "PlayableCharacter.h"
#include "ObjectManager.h"
#include "C_Socket.h"

CPlayableCharacter::CPlayableCharacter(CSkinningObject* object, CAnimationManager* aManager)
	: m_Object(object)
{
	m_AManager = dynamic_cast<CPlayableCharacterAnimationManager*>(aManager);
}

// =======================================================================================

void CPlayerMage::Skill1()
{
	XMFLOAT3 cameraDir = m_pCamera->getDir();
	XMFLOAT3 characterDir = cameraDir;
	characterDir.y = 0.0f; // delete y value
	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_SKILL1), true);
	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_AManager->UpdateAniPosition(0.0f, m_Object);
	for (int i = 0; i < 5; ++i)
	{
		m_bBulletFired[i] = false;
	}
	m_bSkillActive = true;
	m_CurrentSkill = 1;
	m_Damage = 500.0f;
}

void CPlayerMage::Skill2()
{
	XMFLOAT3 cameraDir = m_pCamera->getDir();
	XMFLOAT3 characterDir = cameraDir;
	characterDir.y = 0.0f; // delete y value
	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_SKILL2), true);
	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_AManager->UpdateAniPosition(0.0f, m_Object);
	for (int i = 0; i < 5; ++i)
	{
		m_bBulletFired[i] = false;
	}
	m_bSkillActive = true;
	m_CurrentSkill = 2;
	m_Damage = 3000.0f;
}

void CPlayerMage::Skill3()
{
	XMFLOAT3 cameraDir = m_pCamera->getDir();
	XMFLOAT3 characterDir = cameraDir;
	characterDir.y = 0.0f; // delete y value
	m_AManager->OnKey3Input();
	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_AManager->UpdateAniPosition(0.0f, m_Object);
	for (int i = 0; i < 5; ++i)
	{
		m_bBulletFired[i] = false;
	}
	m_bSkillActive = true;
	m_CurrentSkill = 3;
	m_Damage = 4000.0f;
}

bool CPlayerMage::Attacked(float damage)
{
	if (!CanBeAttacked()) {
		return false;
	}
	//// m_JP -= damage;
	m_LastHit = m_GameTime;
	m_bAttacked = true;
	/*if (m_HP > 0.0f)
	{
		if (!m_bSkillActive && !m_bDoingCombo) {
			m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_HIT), true);
		}
	}
	else
	{
		m_bLive = false;
		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_HIT_DEATH), true);
	}*/
	g_pSoundManager->StartFx(ESOUND::SOUND_HIT);
	return true;
}

CPlayerMage::CPlayerMage(CSkinningObject* object, CAnimationManager* aManager)
	: CPlayableCharacter(object, aManager)
{
	m_HP = 800.0f;
	m_MP = 100.0f;
	m_GameTime = 0.0f;
}

void CPlayerMage::MouseProcess(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	if (!mouseIsInitialize) {
		ShowCursor(FALSE);  // hide cursor
		RECT clientRect;
		GetClientRect(hWnd, &clientRect);
		POINT center;
		center.x = (clientRect.right - clientRect.left) / 2;
		center.y = (clientRect.bottom - clientRect.top) / 2;
		oldCursor = center;
		ClientToScreen(hWnd, &oldCursor);
		SetCursorPos(oldCursor.x, oldCursor.y);
		mouseIsInitialize = true;
	}
	POINT currentPos;
	GetCursorPos(&currentPos);
	XMFLOAT3 cameraDir = m_pCamera->getDir();
	switch (nMessage) {
	case WM_LBUTTONDOWN:
	{
		if (!m_bSkillActive && !g_PlayerDie[Client.get_id()]) {
			XMFLOAT3 characterDir = cameraDir;
			characterDir.y = 0.0f; // delete y value
			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
			m_AManager->UpdateAniPosition(0.0f, m_Object);
			m_AManager->OnAttackInput();
			m_bDoingCombo = true;
			m_Damage = 800.0f;
			MakeBullet(50.0f,1);
		}
		break;
	}
	case WM_MOUSEMOVE:
	{
		// mouse moving amount
		float deltaX = static_cast<float>(currentPos.x - oldCursor.x);
		float deltaY = static_cast<float>(currentPos.y - oldCursor.y);

		if (deltaX != 0.0f || deltaY != 0.0f) {
			if (m_pCamera->getThirdPersonState()) {
				m_pCamera->Rotate(deltaX * 1.5f, -deltaY * 1.5f);
				CGameObject* frame = m_AManager->getFrame()[0];
				if (!m_bSkillActive && !m_bDoingCombo && !m_AManager->IsInCombo() && !m_AManager->IsAnimationFinished() && !g_PlayerDie[Client.get_id()]) {
					m_Object->Rotation(XMFLOAT3(0.0f, deltaX * 0.5f, 0.0f), *frame);
					XMFLOAT3 characterDir = cameraDir;
					characterDir.y = 0.0f; // delete y value
					//m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
				}
			}
			else
				m_pCamera->Rotate(deltaX * 1.5f, deltaY * 1.5f);

			SetCursorPos(oldCursor.x, oldCursor.y);
			break;
		}
	}
	}
}

KeyInputRet CPlayerMage::ProcessInput(UCHAR* keyBuffer, float fElapsedTime)
{
	XMFLOAT3 cameraDir = m_pCamera->getDir();
	XMFLOAT3 characterDir = cameraDir;
	characterDir.y = 0.0f; // delete y value
	m_bMoving = false;
	XMFLOAT3 normalizedCharacterDir = characterDir;
	XMStoreFloat3(&normalizedCharacterDir, XMVector3Normalize(XMLoadFloat3(&normalizedCharacterDir)));
	XMFLOAT3 moveDir{};

	KeyInputRet ret = KEY_NOTHING;

	if (m_bSkillActive || m_bDoingCombo) {
		memset(m_PrevKeyBuffer, 0, sizeof(m_PrevKeyBuffer));
		return ret;
	}

	XMVECTOR lookDir = XMLoadFloat3(&normalizedCharacterDir);
	XMVECTOR rightDir = XMVector3Normalize(XMVector3Cross(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), lookDir));

	if (keyBuffer['W'] & 0x80 && keyBuffer['A'] & 0x80) {
		XMVECTOR dir = XMVectorSubtract(lookDir, rightDir);
		XMStoreFloat3(&moveDir, XMVector3Normalize(dir));
		m_bMoving = true;
	}
	else if (keyBuffer['W'] & 0x80 && keyBuffer['D'] & 0x80) {
		XMVECTOR dir = XMVectorAdd(lookDir, rightDir);
		XMStoreFloat3(&moveDir, XMVector3Normalize(dir));
		m_bMoving = true;
	}
	else if (keyBuffer['S'] & 0x80 && keyBuffer['A'] & 0x80) {
		XMVECTOR dir = XMVectorSubtract(XMVectorNegate(lookDir), rightDir);
		XMStoreFloat3(&moveDir, XMVector3Normalize(dir));
		m_bMoving = true;
	}
	else if (keyBuffer['S'] & 0x80 && keyBuffer['D'] & 0x80) {
		XMVECTOR dir = XMVectorAdd(XMVectorNegate(lookDir), rightDir);
		XMStoreFloat3(&moveDir, XMVector3Normalize(dir));
		m_bMoving = true;
	}
	else if (keyBuffer['W'] & 0x80) {
		moveDir = normalizedCharacterDir;
		m_bMoving = true;
	}
	else if (keyBuffer['S'] & 0x80) {
		XMStoreFloat3(&moveDir, XMVectorNegate(lookDir));
		m_bMoving = true;
	}
	else if (keyBuffer['A'] & 0x80) {
		XMStoreFloat3(&moveDir, XMVectorNegate(rightDir));
		m_bMoving = true;
	}
	else if (keyBuffer['D'] & 0x80) {
		XMStoreFloat3(&moveDir, rightDir);
		m_bMoving = true;
	}

	if (m_bMoving) {
		m_Object->SetMoveDirection(moveDir);
		m_Object->SetDirectionMove(moveDir, XMFLOAT3(0.0f, 1.0f, 0.0f), fElapsedTime);

		if (keyBuffer[VK_LSHIFT] & 0x80) {
			m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_FORWARD), true);
			m_Object->run(fElapsedTime);
		}
		else {
			m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_FORWARD), true);
			m_Object->move(fElapsedTime);
		}
	}
	else {
		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_IDLE), false);
	}

	//// W -> IDLE while Shift held
	//if ((m_PrevKeyBuffer['W'] & 0x80) && !(keyBuffer['W'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_IDLE), false); // IDLE
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// A -> IDLE while Shift held
	//else if ((m_PrevKeyBuffer['A'] & 0x80) && !(keyBuffer['A'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_IDLE), false); // IDLE
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S -> IDLE while Shift held
	//else if ((m_PrevKeyBuffer['S'] & 0x80) && !(keyBuffer['S'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_IDLE), false); // IDLE
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// D -> IDLE while Shift held
	//else if ((m_PrevKeyBuffer['D'] & 0x80) && !(keyBuffer['D'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_IDLE), false); // IDLE
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W + A + Shift -> Run Left Up
	//else if ((keyBuffer['W'] & 0x80) && (keyBuffer['A'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['W'] & 0x80) || !(m_PrevKeyBuffer['A'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_LEFT_UP), true); // Run Left Up
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_LEFT_UP), true); // Maintain Run
	//	}
	//}
	//// W + A + Shift -> Walk Left Up
	//else if ((keyBuffer['W'] & 0x80) && (keyBuffer['A'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_LEFT_UP), true); // Walk Left UP
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W + A -> Walk Left Up
	//else if ((keyBuffer['W'] & 0x80) && (keyBuffer['A'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['W'] & 0x80) || !(m_PrevKeyBuffer['A'] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_LEFT_UP), true); // Walk Left Up
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_LEFT_UP), true); // Maintain Walk
	//	}
	//}
	//// W + A + Shift, A -> Run Forward
	//else if ((keyBuffer['W'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && (m_PrevKeyBuffer['A'] & 0x80) && !(keyBuffer['A'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_FORWARD), true); // Run Forward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W + A, A -> Walk Forward
	//else if ((keyBuffer['W'] & 0x80) && (m_PrevKeyBuffer['A'] & 0x80) && !(keyBuffer['A'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_FORWARD), true); // Walk Forward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W + A, W -> Walk Left
	//else if ((keyBuffer['A'] & 0x80) && (m_PrevKeyBuffer['W'] & 0x80) && !(keyBuffer['W'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_LEFT), true); // Walk Left
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W + D + Shift -> Run Right Up
	//else if ((keyBuffer['W'] & 0x80) && (keyBuffer['D'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['W'] & 0x80) || !(m_PrevKeyBuffer['D'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_RIGHT_UP), true); // Run Right Up
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_RIGHT_UP), true); // Maintain Run
	//	}
	//}
	//// W + D + Shift -> Walk Right Up
	//else if ((keyBuffer['W'] & 0x80) && (keyBuffer['D'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_RIGHT_UP), true); // Walk Right Up
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W + D -> Walk Right Up
	//else if ((keyBuffer['W'] & 0x80) && (keyBuffer['D'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['W'] & 0x80) || !(m_PrevKeyBuffer['D'] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_RIGHT_UP), true); // Walk Right Up
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_RIGHT_UP), true); // Maintain Walk
	//	}
	//}
	//// W + D + Shift, D -> Run Forward
	//else if ((keyBuffer['W'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && (m_PrevKeyBuffer['D'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_FORWARD), true); // Run Forward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W + D, D -> Walk Forward
	//else if ((keyBuffer['W'] & 0x80) && (m_PrevKeyBuffer['D'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_FORWARD), true); // Walk Forward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W + D, W -> Walk Right
	//else if ((keyBuffer['D'] & 0x80) && (m_PrevKeyBuffer['W'] & 0x80) && !(keyBuffer['W'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_RIGHT), true); // Walk Right
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S + A + Shift -> Run Left Down
	//else if ((keyBuffer['S'] & 0x80) && (keyBuffer['A'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['S'] & 0x80) || !(m_PrevKeyBuffer['A'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_LEFT_DOWN), true); // Run Left Down
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_LEFT_DOWN), true); // Maintain Run
	//	}
	//}
	//// S + A + Shift -> Walk Left Down
	//else if ((keyBuffer['S'] & 0x80) && (keyBuffer['A'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_LEFT_DOWN), true); // Walk Left Down
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S + A -> Walk Left Down
	//else if ((keyBuffer['S'] & 0x80) && (keyBuffer['A'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['S'] & 0x80) || !(m_PrevKeyBuffer['A'] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_LEFT_DOWN), true); // Walk Left Down
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_LEFT_DOWN), true); // Maintain Walk
	//	}
	//}
	//// S + A + Shift, A -> Run Backward
	//else if ((keyBuffer['S'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && (m_PrevKeyBuffer['A'] & 0x80) && !(keyBuffer['A'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_BACKWARD), true); // Run Backward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S + A, A -> Walk Backward
	//else if ((keyBuffer['S'] & 0x80) && (m_PrevKeyBuffer['A'] & 0x80) && !(keyBuffer['A'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_BACKWARD), true); // Walk Backward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S + A, S -> Walk Left
	//else if ((keyBuffer['A'] & 0x80) && (m_PrevKeyBuffer['S'] & 0x80) && !(keyBuffer['S'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_LEFT), true); // Walk Left
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S + D + Shift -> Run Right Down
	//else if ((keyBuffer['S'] & 0x80) && (keyBuffer['D'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['S'] & 0x80) || !(m_PrevKeyBuffer['D'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_RIGHT_DOWN), true); // Run Right Down
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_RIGHT_DOWN), true); // Maintain Run
	//	}
	//}
	//// S + D + Shift -> Walk Right Down
	//else if ((keyBuffer['S'] & 0x80) && (keyBuffer['D'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_RIGHT_DOWN), true); // Walk Right Down
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S + D -> Walk Right Down
	//else if ((keyBuffer['S'] & 0x80) && (keyBuffer['D'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['S'] & 0x80) || !(m_PrevKeyBuffer['D'] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_RIGHT_DOWN), true); // Walk Right Down
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_RIGHT_DOWN), true); // Maintain Walk
	//	}
	//}
	//// S + D + Shift, D -> Run Backward
	//else if ((keyBuffer['S'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && (m_PrevKeyBuffer['D'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_BACKWARD), true); // Run Backward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S + D, D -> Walk Backward
	//else if ((keyBuffer['S'] & 0x80) && (m_PrevKeyBuffer['D'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_BACKWARD), true); // Walk Backward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S + D, S -> Walk Right
	//else if ((keyBuffer['D'] & 0x80) && (m_PrevKeyBuffer['S'] & 0x80) && !(keyBuffer['S'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_RIGHT), true); // Walk Right
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W + Shift -> Run Forward
	//else if ((keyBuffer['W'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['W'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_FORWARD), true); // Run Forward
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_FORWARD), true); // Maintain Run
	//	}
	//}
	//// W + Shift -> Walk Forward
	//else if ((keyBuffer['W'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_FORWARD), true); // Walk Forward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W -> Walk Forward
	//else if ((keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['W'] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_FORWARD), true); // Maintain Walk
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_FORWARD), true); // Maintain Walk
	//	}
	//}
	//// S + Shift -> Run Backward
	//else if ((keyBuffer['S'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['S'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_BACKWARD), true); // Run Backward
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_BACKWARD), true); // Maintain Run
	//	}
	//}
	//// S + Shift -> Walk Backward
	//else if ((keyBuffer['S'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_BACKWARD), true); // Walk Backward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S -> Walk Backward
	//else if ((keyBuffer['S'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['S'] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_BACKWARD), true); // Walk Backward
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_BACKWARD), true); // Maintain Walk
	//	}
	//}
	//// A + Shift -> Run Left
	//else if ((keyBuffer['A'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['A'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_LEFT), true); // Run Left
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_LEFT), true); // Maintain Run
	//	}
	//}
	//// A + Shift -> Walk Left
	//else if ((keyBuffer['A'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_LEFT), true); // Walk Left
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// A -> Walk Left
	//else if ((keyBuffer['A'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['A'] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_LEFT), true); // Walk Left
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_LEFT), true); // Maintain Walk
	//	}
	//}
	//// D + Shift -> Run Right
	//else if ((keyBuffer['D'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['D'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_RIGHT), true); // Run Right
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_RUN_RIGHT), true); // Maintain Run
	//	}
	//}
	//// D + Shift -> Walk Right
	//else if ((keyBuffer['D'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_RIGHT), true); // Walk Right
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// D -> Walk Right
	//else if ((keyBuffer['D'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['D'] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_RIGHT), true); // Walk Right
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_WALK_RIGHT), true); // Maintain Walk
	//	}
	//}
	//// W -> IDLE
	//else if ((m_PrevKeyBuffer['W'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_IDLE), false);
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S -> IDLE
	//else if ((m_PrevKeyBuffer['S'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_IDLE), false); // IDLE
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// A -> IDLE
	//else if ((m_PrevKeyBuffer['A'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_IDLE), false); // IDLE
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// D -> IDLE
	//else if ((m_PrevKeyBuffer['D'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_IDLE), false); // IDLE
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}

	if (!m_bSkillActive && !m_bDoingCombo) {
		if ((keyBuffer['J'] & 0x80) && !(m_PrevKeyBuffer['J'] & 0x80)) {
			m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_HIT), true);
			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
			m_AManager->UpdateAniPosition(0.0f, m_Object);
			m_bSkillActive = true;
		}
		if ((keyBuffer['K'] & 0x80) && !(m_PrevKeyBuffer['K'] & 0x80)) {
			m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_HIT), true);
			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
			m_AManager->UpdateAniPosition(0.0f, m_Object);
			m_bSkillActive = true;
		}
		if ((keyBuffer[VK_SPACE] & 0x80) && !(m_PrevKeyBuffer[VK_SPACE] & 0x80)) {
			XMFLOAT3 dodgeDir = characterDir;

			if (keyBuffer['W'] & 0x80 && keyBuffer['A'] & 0x80) {
				dodgeDir = XMFLOAT3(characterDir.x - characterDir.z, 0.0f, characterDir.z + characterDir.x);
			}
			else if (keyBuffer['W'] & 0x80 && keyBuffer['D'] & 0x80) {
				dodgeDir = XMFLOAT3(characterDir.x + characterDir.z, 0.0f, characterDir.z - characterDir.x);
			}
			else if (keyBuffer['S'] & 0x80 && keyBuffer['A'] & 0x80) {
				dodgeDir = XMFLOAT3(-characterDir.x - characterDir.z, 0.0f, -characterDir.z + characterDir.x);
			}
			else if (keyBuffer['S'] & 0x80 && keyBuffer['D'] & 0x80) {
				dodgeDir = XMFLOAT3(-characterDir.x + characterDir.z, 0.0f, -characterDir.z - characterDir.x);
			}
			else if (keyBuffer['W'] & 0x80) {
				dodgeDir = characterDir;
			}
			else if (keyBuffer['S'] & 0x80) {
				dodgeDir = XMFLOAT3(-characterDir.x, 0.0f, -characterDir.z);
			}
			else if (keyBuffer['A'] & 0x80) {
				dodgeDir = XMFLOAT3(-characterDir.z, 0.0f, characterDir.x);
			}
			else if (keyBuffer['D'] & 0x80) {
				dodgeDir = XMFLOAT3(characterDir.z, 0.0f, -characterDir.x);
			}

			XMStoreFloat3(&dodgeDir, XMVector3Normalize(XMLoadFloat3(&dodgeDir)));

			m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_DODGE), true);
			m_Object->SetLookDirection(dodgeDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
			m_AManager->UpdateAniPosition(0.0f, m_Object);
			m_bSkillActive = true;
			m_bDodged = true;
		}
		if ((keyBuffer['L'] & 0x80) && !(m_PrevKeyBuffer['L'] & 0x80)) {
			m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_BIGHIT), true);
			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
			m_AManager->UpdateAniPosition(0.0f, m_Object);
			m_bSkillActive = true;
		}
		if ((keyBuffer['U'] & 0x80) && !(m_PrevKeyBuffer['U'] & 0x80)) {
			m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_BIGHIT_DEATH), true);
			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
			m_AManager->UpdateAniPosition(0.0f, m_Object);
			m_bSkillActive = true;
		}
		if ((keyBuffer['Q'] & 0x80) && !(m_PrevKeyBuffer['Q'] & 0x80) &&
			Players[Client.get_id()].GetMP() >= g_SkillCost[0] && g_SkillCurCTime[0] <= 0) {	// Mp검사 추가해서 
			Skill1();
			ret = KEY_SKILL1;
		}
		if ((keyBuffer['E'] & 0x80) && !(m_PrevKeyBuffer['E'] & 0x80) &&
			Players[Client.get_id()].GetMP() >= g_SkillCost[1] && g_SkillCurCTime[1] <= 0) {
			Skill2();
			ret = KEY_SKILL2;
		}
		if ((keyBuffer['R'] & 0x80) && !(m_PrevKeyBuffer['R'] & 0x80) &&
			Players[Client.get_id()].GetMP() >= g_SkillCost[2] && g_SkillCurCTime[2] <= 0) {
			Skill3();
			ret = KEY_SKILL3;
		}
	}
	memcpy(m_PrevKeyBuffer, keyBuffer, sizeof(m_PrevKeyBuffer));
	return ret;
}

void CPlayerMage::UpdateObject(float fElapsedTime)
{
	bool test = false;
	m_bCheckAC = false;
	m_GameTime += fElapsedTime;
	if (m_bLive) {
		m_AManager->UpdateCombo(fElapsedTime);
		if (!m_AManager->IsInCombo() && m_AManager->IsAnimationFinished()) {
			m_AManager->ChangeAnimation(static_cast<int>(MageAni::ANI_IDLE), false);
			test = true;
			m_bSkillActive = false;
			m_bDoingCombo = false;
			m_bAttacked = false;
			m_CurrentSkill = 0;
			m_Skill3Time = 0.0f;
			m_bDodged = false;
		}
		if (m_AManager->IsComboInterrupted()) {
			test = true;
			m_AManager->ClearComboInterrupted();
			m_bSkillActive = false;
			m_bDoingCombo = false;
			m_bAttacked = false;
			m_bDodged = false;
		}

		if (test) {
			m_AManager->UpdateAniPosition(fElapsedTime, m_Object);
		}

		for (auto& bulletPtr : bullet) {
			if (bulletPtr->getActive()) {
				bulletPtr->IsMoving(fElapsedTime);
			}
		}

		switch (getCurrentSkill())
		{
		case 1: //0.5~0.6, 1.2~1.3, 2.0~2.1, 2.2~2.3, 3.1~3.2
			if (getAniManager()->IsAnimationInTimeRange(0.5f, 0.6f) && !m_bBulletFired[0])
			{
				MakeBullet(80.0f);
				m_bBulletFired[0] = true;
			}
			else if (getAniManager()->IsAnimationInTimeRange(1.2f, 1.3f) && !m_bBulletFired[1])
			{
				MakeBullet(80.0f);
				m_bBulletFired[1] = true;
			}
			else if (getAniManager()->IsAnimationInTimeRange(2.0f, 2.1f) && !m_bBulletFired[2])
			{
				MakeBullet(80.0f);
				m_bBulletFired[2] = true;
			}
			else if (getAniManager()->IsAnimationInTimeRange(2.2f, 2.3f) && !m_bBulletFired[3])
			{
				MakeBullet(80.0f);
				m_bBulletFired[3] = true;
			}
			else if (getAniManager()->IsAnimationInTimeRange(3.1f, 3.2f) && !m_bBulletFired[4])
			{
				MakeBullet(80.0f);
				m_bBulletFired[4] = true;
			}
			break;
		case 2:
			if (!m_bBulletFired[0]) {
				MakeBullet(80.0f, 2);
				m_bBulletFired[0] = true;
			}
			break;
		case 3:
			m_Skill3Time += fElapsedTime;
			if (1.1f <= m_Skill3Time && m_Skill3Time <= 1.2f && !m_bBulletFired[0])
			{
				MakeBullet(80.0f, 3);
				m_bBulletFired[0] = true;
			}
			else if (1.7f <= m_Skill3Time && m_Skill3Time <= 1.8f && !m_bBulletFired[1])
			{
				MakeBullet(80.0f, 3);
				m_bBulletFired[1] = true;
			}
			else if (2.3f <= m_Skill3Time && m_Skill3Time <= 2.4f && !m_bBulletFired[2])
			{
				MakeBullet(80.0f, 3);
				m_bBulletFired[2] = true;
			}
			else if (2.9f <= m_Skill3Time && m_Skill3Time <= 3.0f && !m_bBulletFired[3])
			{
				MakeBullet(80.0f, 3);
				m_bBulletFired[3] = true;
			}
			else if (3.5f <= m_Skill3Time && m_Skill3Time <= 3.6f && !m_bBulletFired[4])
			{
				MakeBullet(80.0f, 3);
				m_bBulletFired[4] = true;
			}
			break;
		}
	}

}

void CPlayerMage::MakeBullet(float speed, int skill)
{
	if (bullet.empty()) {
		return;
	}

	size_t startIndex = currentBullet;
	CProjectile* projectile = nullptr;
	do {
		projectile = bullet[currentBullet].get();
		if (!projectile->getActive()) {
			break;
		}
	} while (currentBullet != startIndex);

	projectile->setSpeed(speed);
	projectile->setLifetime(3.0f);
	projectile->setTime(0.0f);
	XMFLOAT3 pos = XMFLOAT3(m_Head->getWorldMatrix()._41, m_Head->getWorldMatrix()._42, m_Head->getWorldMatrix()._43);

	if (skill == 1) {
		projectile->setPosition(pos);
		projectile->setMoveDirection(m_AutoDirect);
		projectile->getObjects().SetScale({ 1.0f, 1.0f, 1.0f });
		projectile->getObjects().SetRenderState(true);
	}
	else if (skill == 2) {
		pos.y += 30.0f;
		projectile->setPosition(pos);
		projectile->setMoveDirection({ m_AutoDirect.x, -0.6f, m_AutoDirect.z });
		projectile->getObjects().SetScale({ 10.0f, 10.0f, 10.0f });
		projectile->getObjects().SetRenderState(true);
	}
	else if (skill == 3) {
		projectile->setPosition(pos);
		projectile->setMoveDirection(m_AutoDirect);
		projectile->getObjects().SetScale({ 1.0f, 1.0f, 1.0f });
		projectile->getObjects().SetRenderState(false);
	}
	projectile->setActive(true);
	currentBullet = (currentBullet + 1) % bullet.size();
}

// =======================================================================================

void CPlayerWarrior::Skill1()
{
	XMFLOAT3 cameraDir = m_pCamera->getDir();
	XMFLOAT3 characterDir = cameraDir;
	characterDir.y = 0.0f; // delete y value
	m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_SKILL1), true);
	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_AManager->UpdateAniPosition(0.0f, m_Object);
	m_bSkillActive = true;
	m_CurrentSkill = 1;
	m_Damage = 1800.0f;
}

void CPlayerWarrior::Skill2()
{
	XMFLOAT3 cameraDir = m_pCamera->getDir();
	XMFLOAT3 characterDir = cameraDir;
	characterDir.y = 0.0f; // delete y value
	m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_SKILL2), true);
	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_AManager->UpdateAniPosition(0.0f, m_Object);
	m_bSkillActive = true;
	m_CurrentSkill = 2;
	m_Damage = 2000.0f;
}

void CPlayerWarrior::Skill3()
{
	XMFLOAT3 cameraDir = m_pCamera->getDir();
	XMFLOAT3 characterDir = cameraDir;
	characterDir.y = 0.0f; // delete y value
	m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_SKILL3_1), true);
	//m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_SKILL3_2), true);
	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_AManager->UpdateAniPosition(0.0f, m_Object);
	m_bSkillActive = true;
	m_CurrentSkill = 3;
	m_Damage = 3200.0f; //1
	//m_Damage = 600.0f; //2
}

bool CPlayerWarrior::Attacked(float damage)
{
	if (m_bSkillActive && m_CurrentSkill == 2)
	{
		g_pSoundManager->StartFx(ESOUND::SOUND_SKILL_PARRY);
		return false;
	}
	if (!CanBeAttacked()) {
		return false;
	}
	//// m_JP -= damage;
	m_LastHit = m_GameTime;
	//// m_JP -= damage;
	m_bAttacked = true;
	/*if (m_HP > 0.0f)
	{
		if (!m_bSkillActive && !m_bDoingCombo) {
			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_HIT), true);
		}
	}
	else
	{
		m_bLive = false;
		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_DEATH), true);
	}*/
	g_pSoundManager->StartFx(ESOUND::SOUND_HIT);
	return true;
}

CPlayerWarrior::CPlayerWarrior(CSkinningObject* object, CAnimationManager* aManager)
	: CPlayableCharacter(object, aManager)
{
	m_HP = 1200.0f;
	m_MP = 100.0f;
	m_GameTime = 0.0f;
}

void CPlayerWarrior::MouseProcess(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	if (!mouseIsInitialize) {
		ShowCursor(FALSE);  // hide cursor
		RECT clientRect;
		GetClientRect(hWnd, &clientRect);
		POINT center;
		center.x = (clientRect.right - clientRect.left) / 2;
		center.y = (clientRect.bottom - clientRect.top) / 2;
		oldCursor = center;
		ClientToScreen(hWnd, &oldCursor);
		SetCursorPos(oldCursor.x, oldCursor.y);
		mouseIsInitialize = true;
	}
	POINT currentPos;
	GetCursorPos(&currentPos);
	XMFLOAT3 cameraDir = m_pCamera->getDir();
	switch (nMessage) {
	case WM_LBUTTONDOWN:
	{
		if (!m_bSkillActive && !g_PlayerDie[Client.get_id()]) {
			XMFLOAT3 characterDir = cameraDir;
			characterDir.y = 0.0f; // delete y value
			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
			m_AManager->UpdateAniPosition(0.0f, m_Object);
			m_AManager->OnAttackInput();
			m_bDoingCombo = true;
			m_Damage = 600.0f;
		}
		break;
	}
	case WM_MOUSEMOVE:
	{
		// mouse moving amount
		float deltaX = static_cast<float>(currentPos.x - oldCursor.x);
		float deltaY = static_cast<float>(currentPos.y - oldCursor.y);

		if (deltaX != 0.0f || deltaY != 0.0f) {
			if (m_pCamera->getThirdPersonState()) {
				m_pCamera->Rotate(deltaX * 1.5f, -deltaY * 1.5f);
				CGameObject* frame = m_AManager->getFrame()[0];
				if (!m_bSkillActive && !m_bDoingCombo && !m_AManager->IsInCombo() && !m_AManager->IsAnimationFinished() && !g_PlayerDie[Client.get_id()]) {
					m_Object->Rotation(XMFLOAT3(0.0f, deltaX * 0.5f, 0.0f), *frame);
					XMFLOAT3 characterDir = cameraDir;
					characterDir.y = 0.0f; // delete y value
					//m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
				}
			}
			else
				m_pCamera->Rotate(deltaX * 1.5f, deltaY * 1.5f);

			SetCursorPos(oldCursor.x, oldCursor.y);
			break;
		}
	}
	}
}

KeyInputRet CPlayerWarrior::ProcessInput(UCHAR* keyBuffer, float fElapsedTime)
{
	XMFLOAT3 cameraDir = m_pCamera->getDir();
	XMFLOAT3 characterDir = cameraDir;
	characterDir.y = 0.0f; // delete y value
	m_bMoving = false;
	XMFLOAT3 normalizedCharacterDir = characterDir;
	XMStoreFloat3(&normalizedCharacterDir, XMVector3Normalize(XMLoadFloat3(&normalizedCharacterDir)));
	XMFLOAT3 moveDir{};

	KeyInputRet ret = KEY_NOTHING;

	if (m_bSkillActive || m_bDoingCombo) {
		memset(m_PrevKeyBuffer, 0, sizeof(m_PrevKeyBuffer));
		return ret;
	}

	XMVECTOR lookDir = XMLoadFloat3(&normalizedCharacterDir);
	XMVECTOR rightDir = XMVector3Normalize(XMVector3Cross(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), lookDir));

	if (keyBuffer['W'] & 0x80 && keyBuffer['A'] & 0x80) {
		XMVECTOR dir = XMVectorSubtract(lookDir, rightDir);
		XMStoreFloat3(&moveDir, XMVector3Normalize(dir));
		m_bMoving = true;
	}
	else if (keyBuffer['W'] & 0x80 && keyBuffer['D'] & 0x80) {
		XMVECTOR dir = XMVectorAdd(lookDir, rightDir);
		XMStoreFloat3(&moveDir, XMVector3Normalize(dir));
		m_bMoving = true;
	}
	else if (keyBuffer['S'] & 0x80 && keyBuffer['A'] & 0x80) {
		XMVECTOR dir = XMVectorSubtract(XMVectorNegate(lookDir), rightDir);
		XMStoreFloat3(&moveDir, XMVector3Normalize(dir));
		m_bMoving = true;
	}
	else if (keyBuffer['S'] & 0x80 && keyBuffer['D'] & 0x80) {
		XMVECTOR dir = XMVectorAdd(XMVectorNegate(lookDir), rightDir);
		XMStoreFloat3(&moveDir, XMVector3Normalize(dir));
		m_bMoving = true;
	}
	else if (keyBuffer['W'] & 0x80) {
		moveDir = normalizedCharacterDir;
		m_bMoving = true;
	}
	else if (keyBuffer['S'] & 0x80) {
		XMStoreFloat3(&moveDir, XMVectorNegate(lookDir));
		m_bMoving = true;
	}
	else if (keyBuffer['A'] & 0x80) {
		XMStoreFloat3(&moveDir, XMVectorNegate(rightDir));
		m_bMoving = true;
	}
	else if (keyBuffer['D'] & 0x80) {
		XMStoreFloat3(&moveDir, rightDir);
		m_bMoving = true;
	}

	if (m_bMoving) {
		m_Object->SetMoveDirection(moveDir);
		m_Object->SetDirectionMove(moveDir, XMFLOAT3(0.0f, 1.0f, 0.0f), fElapsedTime);

		if (keyBuffer[VK_LSHIFT] & 0x80) {
			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_FORWARD), true);
			m_Object->run(fElapsedTime);
		}
		else {
			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_FORWARD), true);
			m_Object->move(fElapsedTime);
		}
	}
	else {
		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_IDLE), false);
	}

//// W -> IDLE while Shift held
//	if ((m_PrevKeyBuffer['W'] & 0x80) && !(keyBuffer['W'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_IDLE), false); // IDLE
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// A -> IDLE while Shift held
//	else if ((m_PrevKeyBuffer['A'] & 0x80) && !(keyBuffer['A'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_IDLE), false); // IDLE
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// S -> IDLE while Shift held
//	else if ((m_PrevKeyBuffer['S'] & 0x80) && !(keyBuffer['S'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_IDLE), false); // IDLE
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// D -> IDLE while Shift held
//	else if ((m_PrevKeyBuffer['D'] & 0x80) && !(keyBuffer['D'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_IDLE), false); // IDLE
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// W + A + Shift -> Run Left Up
//	else if ((keyBuffer['W'] & 0x80) && (keyBuffer['A'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80)) {
//		if (!(m_PrevKeyBuffer['W'] & 0x80) || !(m_PrevKeyBuffer['A'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_LEFT_UP), true); // Run Left Up
//			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//			m_AManager->UpdateAniPosition(0.0f, m_Object);
//		}
//		else {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_LEFT_UP), true); // Maintain Run
//		}
//	}
//	// W + A + Shift -> Walk Left Up
//	else if ((keyBuffer['W'] & 0x80) && (keyBuffer['A'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_LEFT_UP), true); // Walk Left UP
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// W + A -> Walk Left Up
//	else if ((keyBuffer['W'] & 0x80) && (keyBuffer['A'] & 0x80)) {
//		if (!(m_PrevKeyBuffer['W'] & 0x80) || !(m_PrevKeyBuffer['A'] & 0x80)) {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_LEFT_UP), true); // Walk Left Up
//			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//			m_AManager->UpdateAniPosition(0.0f, m_Object);
//		}
//		else {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_LEFT_UP), true); // Maintain Walk
//		}
//	}
//	// W + A + Shift, A -> Run Forward
//	else if ((keyBuffer['W'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && (m_PrevKeyBuffer['A'] & 0x80) && !(keyBuffer['A'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_FORWARD), true); // Run Forward
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// W + A, A -> Walk Forward
//	else if ((keyBuffer['W'] & 0x80) && (m_PrevKeyBuffer['A'] & 0x80) && !(keyBuffer['A'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_FORWARD), true); // Walk Forward
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// W + A, W -> Walk Left
//	else if ((keyBuffer['A'] & 0x80) && (m_PrevKeyBuffer['W'] & 0x80) && !(keyBuffer['W'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_LEFT), true); // Walk Left
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// W + D + Shift -> Run Right Up
//	else if ((keyBuffer['W'] & 0x80) && (keyBuffer['D'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80)) {
//		if (!(m_PrevKeyBuffer['W'] & 0x80) || !(m_PrevKeyBuffer['D'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_RIGHT_UP), true); // Run Right Up
//			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//			m_AManager->UpdateAniPosition(0.0f, m_Object);
//		}
//		else {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_RIGHT_UP), true); // Maintain Run
//		}
//	}
//	// W + D + Shift -> Walk Right Up
//	else if ((keyBuffer['W'] & 0x80) && (keyBuffer['D'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_RIGHT_UP), true); // Walk Right Up
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// W + D -> Walk Right Up
//	else if ((keyBuffer['W'] & 0x80) && (keyBuffer['D'] & 0x80)) {
//		if (!(m_PrevKeyBuffer['W'] & 0x80) || !(m_PrevKeyBuffer['D'] & 0x80)) {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_RIGHT_UP), true); // Walk Right Up
//			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//			m_AManager->UpdateAniPosition(0.0f, m_Object);
//		}
//		else {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_RIGHT_UP), true); // Maintain Walk
//		}
//	}
//	// W + D + Shift, D -> Run Forward
//	else if ((keyBuffer['W'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && (m_PrevKeyBuffer['D'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_FORWARD), true); // Run Forward
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// W + D, D -> Walk Forward
//	else if ((keyBuffer['W'] & 0x80) && (m_PrevKeyBuffer['D'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_FORWARD), true); // Walk Forward
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// W + D, W -> Walk Right
//	else if ((keyBuffer['D'] & 0x80) && (m_PrevKeyBuffer['W'] & 0x80) && !(keyBuffer['W'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_RIGHT), true); // Walk Right
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// S + A + Shift -> Run Left Down
//	else if ((keyBuffer['S'] & 0x80) && (keyBuffer['A'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80)) {
//		if (!(m_PrevKeyBuffer['S'] & 0x80) || !(m_PrevKeyBuffer['A'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_LEFT_DOWN), true); // Run Left Down
//			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//			m_AManager->UpdateAniPosition(0.0f, m_Object);
//		}
//		else {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_LEFT_DOWN), true); // Maintain Run
//		}
//	}
//	// S + A + Shift -> Walk Left Down
//	else if ((keyBuffer['S'] & 0x80) && (keyBuffer['A'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_LEFT_DOWN), true); // Walk Left Down
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// S + A -> Walk Left Down
//	else if ((keyBuffer['S'] & 0x80) && (keyBuffer['A'] & 0x80)) {
//		if (!(m_PrevKeyBuffer['S'] & 0x80) || !(m_PrevKeyBuffer['A'] & 0x80)) {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_LEFT_DOWN), true); // Walk Left Down
//			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//			m_AManager->UpdateAniPosition(0.0f, m_Object);
//		}
//		else {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_LEFT_DOWN), true); // Maintain Walk
//		}
//	}
//	// S + A + Shift, A -> Run Backward
//	else if ((keyBuffer['S'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && (m_PrevKeyBuffer['A'] & 0x80) && !(keyBuffer['A'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_BACKWARD), true); // Run Backward
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// S + A, A -> Walk Backward
//	else if ((keyBuffer['S'] & 0x80) && (m_PrevKeyBuffer['A'] & 0x80) && !(keyBuffer['A'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_BACKWARD), true); // Walk Backward
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// S + A, S -> Walk Left
//	else if ((keyBuffer['A'] & 0x80) && (m_PrevKeyBuffer['S'] & 0x80) && !(keyBuffer['S'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_LEFT), true); // Walk Left
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// S + D + Shift -> Run Right Down
//	else if ((keyBuffer['S'] & 0x80) && (keyBuffer['D'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80)) {
//		if (!(m_PrevKeyBuffer['S'] & 0x80) || !(m_PrevKeyBuffer['D'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_RIGHT_DOWN), true); // Run Right Down
//			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//			m_AManager->UpdateAniPosition(0.0f, m_Object);
//		}
//		else {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_RIGHT_DOWN), true); // Maintain Run
//		}
//	}
//	// S + D + Shift -> Walk Right Down
//	else if ((keyBuffer['S'] & 0x80) && (keyBuffer['D'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_RIGHT_DOWN), true); // Walk Right Down
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// S + D -> Walk Right Down
//	else if ((keyBuffer['S'] & 0x80) && (keyBuffer['D'] & 0x80)) {
//		if (!(m_PrevKeyBuffer['S'] & 0x80) || !(m_PrevKeyBuffer['D'] & 0x80)) {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_RIGHT_DOWN), true); // Walk Right Down
//			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//			m_AManager->UpdateAniPosition(0.0f, m_Object);
//		}
//		else {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_RIGHT_DOWN), true); // Maintain Walk
//		}
//	}
//	// S + D + Shift, D -> Run Backward
//	else if ((keyBuffer['S'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && (m_PrevKeyBuffer['D'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_BACKWARD), true); // Run Backward
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// S + D, D -> Walk Backward
//	else if ((keyBuffer['S'] & 0x80) && (m_PrevKeyBuffer['D'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_BACKWARD), true); // Walk Backward
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// S + D, S -> Walk Right
//	else if ((keyBuffer['D'] & 0x80) && (m_PrevKeyBuffer['S'] & 0x80) && !(keyBuffer['S'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_RIGHT), true); // Walk Right
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// W + Shift -> Run Forward
//	else if ((keyBuffer['W'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
//		if (!(m_PrevKeyBuffer['W'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_FORWARD), true); // Run Forward
//			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//			m_AManager->UpdateAniPosition(0.0f, m_Object);
//		}
//		else {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_FORWARD), true); // Maintain Run
//		}
//	}
//	// W + Shift -> Walk Forward
//	else if ((keyBuffer['W'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_FORWARD), true); // Walk Forward
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// W -> Walk Forward
//	else if ((keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
//		if (!(m_PrevKeyBuffer['W'] & 0x80)) {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_FORWARD), true); // Maintain Walk
//			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//			m_AManager->UpdateAniPosition(0.0f, m_Object);
//		}
//		else {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_FORWARD), true); // Maintain Walk
//		}
//	}
//	// S + Shift -> Run Backward
//	else if ((keyBuffer['S'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
//		if (!(m_PrevKeyBuffer['S'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_BACKWARD), true); // Run Backward
//			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//			m_AManager->UpdateAniPosition(0.0f, m_Object);
//		}
//		else {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_BACKWARD), true); // Maintain Run
//		}
//	}
//	// S + Shift -> Walk Backward
//	else if ((keyBuffer['S'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_BACKWARD), true); // Walk Backward
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// S -> Walk Backward
//	else if ((keyBuffer['S'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
//		if (!(m_PrevKeyBuffer['S'] & 0x80)) {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_BACKWARD), true); // Walk Backward
//			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//			m_AManager->UpdateAniPosition(0.0f, m_Object);
//		}
//		else {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_BACKWARD), true); // Maintain Walk
//		}
//	}
//	// A + Shift -> Run Left
//	else if ((keyBuffer['A'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
//		if (!(m_PrevKeyBuffer['A'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_LEFT), true); // Run Left
//			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//			m_AManager->UpdateAniPosition(0.0f, m_Object);
//		}
//		else {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_LEFT), true); // Maintain Run
//		}
//	}
//	// A + Shift -> Walk Left
//	else if ((keyBuffer['A'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_LEFT), true); // Walk Left
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// A -> Walk Left
//	else if ((keyBuffer['A'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
//		if (!(m_PrevKeyBuffer['A'] & 0x80)) {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_LEFT), true); // Walk Left
//			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//			m_AManager->UpdateAniPosition(0.0f, m_Object);
//		}
//		else {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_LEFT), true); // Maintain Walk
//		}
//	}
//	// D + Shift -> Run Right
//	else if ((keyBuffer['D'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80)) {
//		if (!(m_PrevKeyBuffer['D'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_RIGHT), true); // Run Right
//			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//			m_AManager->UpdateAniPosition(0.0f, m_Object);
//		}
//		else {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_RUN_RIGHT), true); // Maintain Run
//		}
//	}
//	// D + Shift -> Walk Right
//	else if ((keyBuffer['D'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_RIGHT), true); // Walk Right
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// D -> Walk Right
//	else if ((keyBuffer['D'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80)) {
//		if (!(m_PrevKeyBuffer['D'] & 0x80)) {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_RIGHT), true); // Walk Right
//			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//			m_AManager->UpdateAniPosition(0.0f, m_Object);
//		}
//		else {
//			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_WALK_RIGHT), true); // Maintain Walk
//		}
//	}
//	// W -> IDLE
//	else if ((m_PrevKeyBuffer['W'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_IDLE), false);
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// S -> IDLE
//	else if ((m_PrevKeyBuffer['S'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_IDLE), false); // IDLE
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// A -> IDLE
//	else if ((m_PrevKeyBuffer['A'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_IDLE), false); // IDLE
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}
//	// D -> IDLE
//	else if ((m_PrevKeyBuffer['D'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
//		m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_IDLE), false); // IDLE
//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
//		m_AManager->UpdateAniPosition(0.0f, m_Object);
//	}

	if (!m_bSkillActive && !m_bDoingCombo) {
		if ((keyBuffer['J'] & 0x80) && !(m_PrevKeyBuffer['J'] & 0x80)) {
			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_HIT), true);
			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
			m_AManager->UpdateAniPosition(0.0f, m_Object);
			m_bSkillActive = true;
		}
		if ((keyBuffer['K'] & 0x80) && !(m_PrevKeyBuffer['K'] & 0x80)) {
			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_HIT), true);
			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
			m_AManager->UpdateAniPosition(0.0f, m_Object);
			m_bSkillActive = true;
		}
		if ((keyBuffer[VK_SPACE] & 0x80) && !(m_PrevKeyBuffer[VK_SPACE] & 0x80)) {
			XMFLOAT3 dodgeDir = characterDir;

			if (keyBuffer['W'] & 0x80 && keyBuffer['A'] & 0x80) {
				dodgeDir = XMFLOAT3(characterDir.x - characterDir.z, 0.0f, characterDir.z + characterDir.x);
			}
			else if (keyBuffer['W'] & 0x80 && keyBuffer['D'] & 0x80) {
				dodgeDir = XMFLOAT3(characterDir.x + characterDir.z, 0.0f, characterDir.z - characterDir.x);
			}
			else if (keyBuffer['S'] & 0x80 && keyBuffer['A'] & 0x80) {
				dodgeDir = XMFLOAT3(-characterDir.x - characterDir.z, 0.0f, -characterDir.z + characterDir.x);
			}
			else if (keyBuffer['S'] & 0x80 && keyBuffer['D'] & 0x80) {
				dodgeDir = XMFLOAT3(-characterDir.x + characterDir.z, 0.0f, -characterDir.z - characterDir.x);
			}
			else if (keyBuffer['W'] & 0x80) {
				dodgeDir = characterDir;
			}
			else if (keyBuffer['S'] & 0x80) {
				dodgeDir = XMFLOAT3(-characterDir.x, 0.0f, -characterDir.z);
			}
			else if (keyBuffer['A'] & 0x80) {
				dodgeDir = XMFLOAT3(-characterDir.z, 0.0f, characterDir.x);
			}
			else if (keyBuffer['D'] & 0x80) {
				dodgeDir = XMFLOAT3(characterDir.z, 0.0f, -characterDir.x);
			}

			XMStoreFloat3(&dodgeDir, XMVector3Normalize(XMLoadFloat3(&dodgeDir)));

			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_DODGE), true);
			m_Object->SetLookDirection(dodgeDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
			m_AManager->UpdateAniPosition(0.0f, m_Object);
			m_bSkillActive = true;
			m_bDodged = true;
		}
		if ((keyBuffer['L'] & 0x80) && !(m_PrevKeyBuffer['L'] & 0x80)) {
			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_BIGHIT), true);
			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
			m_AManager->UpdateAniPosition(0.0f, m_Object);
			m_bSkillActive = true;
		}
		if ((keyBuffer['U'] & 0x80) && !(m_PrevKeyBuffer['U'] & 0x80)) {
			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_DEATH), true);
			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
			m_AManager->UpdateAniPosition(0.0f, m_Object);
			m_bSkillActive = true;
		}
		if ((keyBuffer['Q'] & 0x80) && !(m_PrevKeyBuffer['Q'] & 0x80) &&
			Players[Client.get_id()].GetMP() >= g_SkillCost[0] && g_SkillCurCTime[0] <= 0) {
			Skill1();
			ret = KEY_SKILL1;
		}
		if ((keyBuffer['E'] & 0x80) && !(m_PrevKeyBuffer['E'] & 0x80) &&
			Players[Client.get_id()].GetMP() >= g_SkillCost[1] && g_SkillCurCTime[1] <= 0) {
			Skill2();
			ret = KEY_SKILL2;
		}
		if ((keyBuffer['R'] & 0x80) && !(m_PrevKeyBuffer['R'] & 0x80) &&
			Players[Client.get_id()].GetMP() >= g_SkillCost[2] && g_SkillCurCTime[2] <= 0) {
			Skill3();
			ret = KEY_SKILL3;
		}
	}
	memcpy(m_PrevKeyBuffer, keyBuffer, sizeof(m_PrevKeyBuffer));
	return ret;
}

void CPlayerWarrior::UpdateObject(float fElapsedTime)
{
	bool test = false;
	m_bCheckAC = false;
	m_GameTime += fElapsedTime;
	if (m_bLive) {
		m_AManager->UpdateCombo(fElapsedTime);
		if (!m_AManager->IsInCombo() && m_AManager->IsAnimationFinished()) {
			m_AManager->ChangeAnimation(static_cast<int>(WarriorAni::ANI_IDLE), false);
			test = true;
			m_bSkillActive = false;
			m_bDoingCombo = false;
			m_bAttacked = false;
			m_CurrentSkill = 0;
			m_bDodged = false;
		}
		if (m_AManager->IsComboInterrupted()) {
			test = true;
			m_AManager->ClearComboInterrupted();
			m_bSkillActive = false;
			m_bDoingCombo = false;
			m_bAttacked = false;
			m_CurrentSkill = 0;
			m_bDodged = false;
		}

		if (test) {
			m_AManager->UpdateAniPosition(fElapsedTime, m_Object);
		}

		if (m_bDoingCombo)
		{
			m_bCheckAC = true;
		}

		switch (getCurrentSkill())
		{
		case 1:
			if (getAniManager()->IsAnimationInTimeRange(0.4f, 0.6f))
			{
				m_bCheckAC = true;
			}
			break;
		case 2:
			if (getAniManager()->IsAnimationInTimeRange(0.5f, 0.6f))
			{
				m_bCheckAC = true;
			}
			break;
		case 3:
			// 스킬-1
			if (getAniManager()->IsAnimationInTimeRange(0.5f, 0.6f))
			{
				m_bCheckAC = true;
			}
			break;
		}
	}
}

// =======================================================================================

void CPlayerPriest::Skill1()
{
	XMFLOAT3 cameraDir = m_pCamera->getDir();
	XMFLOAT3 characterDir = cameraDir;
	characterDir.y = 0.0f; // delete y value
	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_SKILL1), true);
	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_AManager->UpdateAniPosition(0.0f, m_Object);
	m_bSkillActive = true;
	m_CurrentSkill = 1;
}

void CPlayerPriest::Skill2()
{
	XMFLOAT3 cameraDir = m_pCamera->getDir();
	XMFLOAT3 characterDir = cameraDir;
	characterDir.y = 0.0f; // delete y value
	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_SKILL2), true);
	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_AManager->UpdateAniPosition(0.0f, m_Object);
	m_bSkillActive = true;
	m_CurrentSkill = 2;
}

void CPlayerPriest::Skill3()
{
	XMFLOAT3 cameraDir = m_pCamera->getDir();
	XMFLOAT3 characterDir = cameraDir;
	characterDir.y = 0.0f; // delete y value
	m_AManager->OnKey3Input();
	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_AManager->UpdateAniPosition(0.0f, m_Object);
	m_bSkillActive = true;
	m_CurrentSkill = 3;
}

bool CPlayerPriest::Attacked(float damage)
{
	//// m_JP -= damage;

	if (!CanBeAttacked()) {
		return false;
	}
	//// m_JP -= damage;
	m_LastHit = m_GameTime;

	m_bAttacked = true;
	/*if (m_HP > 0.0f)
	{
		if (!m_bSkillActive && !m_bDoingCombo) {
			m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_HIT), true);
		}
	}
	else
	{
		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_HIT_DEATH), true);
		m_bLive = false;
	}*/
	g_pSoundManager->StartFx(ESOUND::SOUND_HIT);
	return true;
}

CPlayerPriest::CPlayerPriest(CSkinningObject* object, CAnimationManager* aManager)
	: CPlayableCharacter(object, aManager)
{
	m_HP = 1000.0f;
	m_MP = 100.0f;
	m_Damage = 800.0f;
	m_GameTime = 0.0f;
}

void CPlayerPriest::MouseProcess(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	if (!mouseIsInitialize) {
		ShowCursor(FALSE);  // hide cursor
		RECT clientRect;
		GetClientRect(hWnd, &clientRect);
		POINT center;
		center.x = (clientRect.right - clientRect.left) / 2;
		center.y = (clientRect.bottom - clientRect.top) / 2;
		oldCursor = center;
		ClientToScreen(hWnd, &oldCursor);
		SetCursorPos(oldCursor.x, oldCursor.y);
		mouseIsInitialize = true;
	}
	POINT currentPos;
	GetCursorPos(&currentPos);
	XMFLOAT3 cameraDir = m_pCamera->getDir();
	switch (nMessage) {
	case WM_LBUTTONDOWN:
	{
		if (!m_bSkillActive && !g_PlayerDie[Client.get_id()]) {
			XMFLOAT3 characterDir = cameraDir;
			characterDir.y = 0.0f; // delete y value
			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
			m_AManager->UpdateAniPosition(0.0f, m_Object);
			m_AManager->OnAttackInput();
			m_bDoingCombo = true;
			MakeBullet(50.0f, 1);
		}
		break;
	}
	case WM_MOUSEMOVE:
	{
		// mouse moving amount
		float deltaX = static_cast<float>(currentPos.x - oldCursor.x);
		float deltaY = static_cast<float>(currentPos.y - oldCursor.y);

		if (deltaX != 0.0f || deltaY != 0.0f) {
			if (m_pCamera->getThirdPersonState()) {
				m_pCamera->Rotate(deltaX * 1.5f, -deltaY * 1.5f);
				CGameObject* frame = m_AManager->getFrame()[0];
				if (!m_bSkillActive && !m_bDoingCombo && !m_AManager->IsInCombo() && !m_AManager->IsAnimationFinished() && !g_PlayerDie[Client.get_id()]) {
					m_Object->Rotation(XMFLOAT3(0.0f, deltaX * 0.5f, 0.0f), *frame);
					XMFLOAT3 characterDir = cameraDir;
					characterDir.y = 0.0f; // delete y value
					//m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
				}
			}
			else
				m_pCamera->Rotate(deltaX * 1.5f, deltaY * 1.5f);

			SetCursorPos(oldCursor.x, oldCursor.y);
			break;
		}
	}
	}
}

KeyInputRet CPlayerPriest::ProcessInput(UCHAR* keyBuffer, float fElapsedTime)
{
	XMFLOAT3 cameraDir = m_pCamera->getDir();
	XMFLOAT3 characterDir = cameraDir;
	characterDir.y = 0.0f; // delete y value
	m_bMoving = false;
	XMFLOAT3 normalizedCharacterDir = characterDir;
	XMStoreFloat3(&normalizedCharacterDir, XMVector3Normalize(XMLoadFloat3(&normalizedCharacterDir)));
	XMFLOAT3 moveDir{};

	KeyInputRet ret = KEY_NOTHING;

	if (m_bSkillActive || m_bDoingCombo) {
		memset(m_PrevKeyBuffer, 0, sizeof(m_PrevKeyBuffer));
		return ret;
	}

	XMVECTOR lookDir = XMLoadFloat3(&normalizedCharacterDir);
	XMVECTOR rightDir = XMVector3Normalize(XMVector3Cross(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), lookDir));

	if (keyBuffer['W'] & 0x80 && keyBuffer['A'] & 0x80) {
		XMVECTOR dir = XMVectorSubtract(lookDir, rightDir);
		XMStoreFloat3(&moveDir, XMVector3Normalize(dir));
		m_bMoving = true;
	}
	else if (keyBuffer['W'] & 0x80 && keyBuffer['D'] & 0x80) {
		XMVECTOR dir = XMVectorAdd(lookDir, rightDir);
		XMStoreFloat3(&moveDir, XMVector3Normalize(dir));
		m_bMoving = true;
	}
	else if (keyBuffer['S'] & 0x80 && keyBuffer['A'] & 0x80) {
		XMVECTOR dir = XMVectorSubtract(XMVectorNegate(lookDir), rightDir);
		XMStoreFloat3(&moveDir, XMVector3Normalize(dir));
		m_bMoving = true;
	}
	else if (keyBuffer['S'] & 0x80 && keyBuffer['D'] & 0x80) {
		XMVECTOR dir = XMVectorAdd(XMVectorNegate(lookDir), rightDir);
		XMStoreFloat3(&moveDir, XMVector3Normalize(dir));
		m_bMoving = true;
	}
	else if (keyBuffer['W'] & 0x80) {
		moveDir = normalizedCharacterDir;
		m_bMoving = true;
	}
	else if (keyBuffer['S'] & 0x80) {
		XMStoreFloat3(&moveDir, XMVectorNegate(lookDir));
		m_bMoving = true;
	}
	else if (keyBuffer['A'] & 0x80) {
		XMStoreFloat3(&moveDir, XMVectorNegate(rightDir));
		m_bMoving = true;
	}
	else if (keyBuffer['D'] & 0x80) {
		XMStoreFloat3(&moveDir, rightDir);
		m_bMoving = true;
	}

	if (m_bMoving) {
		m_Object->SetMoveDirection(moveDir);
		m_Object->SetDirectionMove(moveDir, XMFLOAT3(0.0f, 1.0f, 0.0f), fElapsedTime);

		if (keyBuffer[VK_LSHIFT] & 0x80) {
			m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_FORWARD), true);
			m_Object->run(fElapsedTime);
		}
		else {
			m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_FORWARD), true);
			m_Object->move(fElapsedTime);
		}
	}
	else {
		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_IDLE), false);
	}

	//// W -> IDLE while Shift held
	//if ((m_PrevKeyBuffer['W'] & 0x80) && !(keyBuffer['W'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_IDLE), false); // IDLE
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// A -> IDLE while Shift held
	//else if ((m_PrevKeyBuffer['A'] & 0x80) && !(keyBuffer['A'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_IDLE), false); // IDLE
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S -> IDLE while Shift held
	//else if ((m_PrevKeyBuffer['S'] & 0x80) && !(keyBuffer['S'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_IDLE), false); // IDLE
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// D -> IDLE while Shift held
	//else if ((m_PrevKeyBuffer['D'] & 0x80) && !(keyBuffer['D'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_IDLE), false); // IDLE
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W + A + Shift -> Run Left Up
	//else if ((keyBuffer['W'] & 0x80) && (keyBuffer['A'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['W'] & 0x80) || !(m_PrevKeyBuffer['A'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_LEFT_UP), true); // Run Left Up
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_LEFT_UP), true); // Maintain Run
	//	}
	//}
	//// W + A + Shift -> Walk Left Up
	//else if ((keyBuffer['W'] & 0x80) && (keyBuffer['A'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_LEFT_UP), true); // Walk Left UP
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W + A -> Walk Left Up
	//else if ((keyBuffer['W'] & 0x80) && (keyBuffer['A'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['W'] & 0x80) || !(m_PrevKeyBuffer['A'] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_LEFT_UP), true); // Walk Left Up
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_LEFT_UP), true); // Maintain Walk
	//	}
	//}
	//// W + A + Shift, A -> Run Forward
	//else if ((keyBuffer['W'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && (m_PrevKeyBuffer['A'] & 0x80) && !(keyBuffer['A'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_FORWARD), true); // Run Forward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W + A, A -> Walk Forward
	//else if ((keyBuffer['W'] & 0x80) && (m_PrevKeyBuffer['A'] & 0x80) && !(keyBuffer['A'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_FORWARD), true); // Walk Forward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W + A, W -> Walk Left
	//else if ((keyBuffer['A'] & 0x80) && (m_PrevKeyBuffer['W'] & 0x80) && !(keyBuffer['W'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_LEFT), true); // Walk Left
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W + D + Shift -> Run Right Up
	//else if ((keyBuffer['W'] & 0x80) && (keyBuffer['D'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['W'] & 0x80) || !(m_PrevKeyBuffer['D'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_RIGHT_UP), true); // Run Right Up
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_RIGHT_UP), true); // Maintain Run
	//	}
	//}
	//// W + D + Shift -> Walk Right Up
	//else if ((keyBuffer['W'] & 0x80) && (keyBuffer['D'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_RIGHT_UP), true); // Walk Right Up
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W + D -> Walk Right Up
	//else if ((keyBuffer['W'] & 0x80) && (keyBuffer['D'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['W'] & 0x80) || !(m_PrevKeyBuffer['D'] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_RIGHT_UP), true); // Walk Right Up
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_RIGHT_UP), true); // Maintain Walk
	//	}
	//}
	//// W + D + Shift, D -> Run Forward
	//else if ((keyBuffer['W'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && (m_PrevKeyBuffer['D'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_FORWARD), true); // Run Forward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W + D, D -> Walk Forward
	//else if ((keyBuffer['W'] & 0x80) && (m_PrevKeyBuffer['D'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_FORWARD), true); // Walk Forward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W + D, W -> Walk Right
	//else if ((keyBuffer['D'] & 0x80) && (m_PrevKeyBuffer['W'] & 0x80) && !(keyBuffer['W'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_RIGHT), true); // Walk Right
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S + A + Shift -> Run Left Down
	//else if ((keyBuffer['S'] & 0x80) && (keyBuffer['A'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['S'] & 0x80) || !(m_PrevKeyBuffer['A'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_LEFT_DOWN), true); // Run Left Down
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_LEFT_DOWN), true); // Maintain Run
	//	}
	//}
	//// S + A + Shift -> Walk Left Down
	//else if ((keyBuffer['S'] & 0x80) && (keyBuffer['A'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_LEFT_DOWN), true); // Walk Left Down
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S + A -> Walk Left Down
	//else if ((keyBuffer['S'] & 0x80) && (keyBuffer['A'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['S'] & 0x80) || !(m_PrevKeyBuffer['A'] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_LEFT_DOWN), true); // Walk Left Down
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_LEFT_DOWN), true); // Maintain Walk
	//	}
	//}
	//// S + A + Shift, A -> Run Backward
	//else if ((keyBuffer['S'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && (m_PrevKeyBuffer['A'] & 0x80) && !(keyBuffer['A'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_BACKWARD), true); // Run Backward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S + A, A -> Walk Backward
	//else if ((keyBuffer['S'] & 0x80) && (m_PrevKeyBuffer['A'] & 0x80) && !(keyBuffer['A'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_BACKWARD), true); // Walk Backward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S + A, S -> Walk Left
	//else if ((keyBuffer['A'] & 0x80) && (m_PrevKeyBuffer['S'] & 0x80) && !(keyBuffer['S'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_LEFT), true); // Walk Left
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S + D + Shift -> Run Right Down
	//else if ((keyBuffer['S'] & 0x80) && (keyBuffer['D'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['S'] & 0x80) || !(m_PrevKeyBuffer['D'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_RIGHT_DOWN), true); // Run Right Down
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_RIGHT_DOWN), true); // Maintain Run
	//	}
	//}
	//// S + D + Shift -> Walk Right Down
	//else if ((keyBuffer['S'] & 0x80) && (keyBuffer['D'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_RIGHT_DOWN), true); // Walk Right Down
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S + D -> Walk Right Down
	//else if ((keyBuffer['S'] & 0x80) && (keyBuffer['D'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['S'] & 0x80) || !(m_PrevKeyBuffer['D'] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_RIGHT_DOWN), true); // Walk Right Down
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_RIGHT_DOWN), true); // Maintain Walk
	//	}
	//}
	//// S + D + Shift, D -> Run Backward
	//else if ((keyBuffer['S'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && (m_PrevKeyBuffer['D'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_BACKWARD), true); // Run Backward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S + D, D -> Walk Backward
	//else if ((keyBuffer['S'] & 0x80) && (m_PrevKeyBuffer['D'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_BACKWARD), true); // Walk Backward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S + D, S -> Walk Right
	//else if ((keyBuffer['D'] & 0x80) && (m_PrevKeyBuffer['S'] & 0x80) && !(keyBuffer['S'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_RIGHT), true); // Walk Right
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W + Shift -> Run Forward
	//else if ((keyBuffer['W'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['W'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_FORWARD), true); // Run Forward
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_FORWARD), true); // Maintain Run
	//	}
	//}
	//// W + Shift -> Walk Forward
	//else if ((keyBuffer['W'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_FORWARD), true); // Walk Forward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// W -> Walk Forward
	//else if ((keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['W'] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_FORWARD), true); // Maintain Walk
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_FORWARD), true); // Maintain Walk
	//	}
	//}
	//// S + Shift -> Run Backward
	//else if ((keyBuffer['S'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['S'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_BACKWARD), true); // Run Backward
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_BACKWARD), true); // Maintain Run
	//	}
	//}
	//// S + Shift -> Walk Backward
	//else if ((keyBuffer['S'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_BACKWARD), true); // Walk Backward
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S -> Walk Backward
	//else if ((keyBuffer['S'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['S'] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_BACKWARD), true); // Walk Backward
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_BACKWARD), true); // Maintain Walk
	//	}
	//}
	//// A + Shift -> Run Left
	//else if ((keyBuffer['A'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['A'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_LEFT), true); // Run Left
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_LEFT), true); // Maintain Run
	//	}
	//}
	//// A + Shift -> Walk Left
	//else if ((keyBuffer['A'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_LEFT), true); // Walk Left
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// A -> Walk Left
	//else if ((keyBuffer['A'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['A'] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_LEFT), true); // Walk Left
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_LEFT), true); // Maintain Walk
	//	}
	//}
	//// D + Shift -> Run Right
	//else if ((keyBuffer['D'] & 0x80) && (keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['D'] & 0x80) || !(m_PrevKeyBuffer[VK_LSHIFT] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_RIGHT), true); // Run Right
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_RUN_RIGHT), true); // Maintain Run
	//	}
	//}
	//// D + Shift -> Walk Right
	//else if ((keyBuffer['D'] & 0x80) && (m_PrevKeyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_RIGHT), true); // Walk Right
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// D -> Walk Right
	//else if ((keyBuffer['D'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80)) {
	//	if (!(m_PrevKeyBuffer['D'] & 0x80)) {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_RIGHT), true); // Walk Right
	//		m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//		m_AManager->UpdateAniPosition(0.0f, m_Object);
	//	}
	//	else {
	//		m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_WALK_RIGHT), true); // Maintain Walk
	//	}
	//}
	//// W -> IDLE
	//else if ((m_PrevKeyBuffer['W'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_IDLE), false);
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// S -> IDLE
	//else if ((m_PrevKeyBuffer['S'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_IDLE), false); // IDLE
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// A -> IDLE
	//else if ((m_PrevKeyBuffer['A'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_IDLE), false); // IDLE
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}
	//// D -> IDLE
	//else if ((m_PrevKeyBuffer['D'] & 0x80) && !(keyBuffer['W'] & 0x80) && !(keyBuffer['A'] & 0x80) && !(keyBuffer['S'] & 0x80) && !(keyBuffer['D'] & 0x80) && !(keyBuffer[VK_LSHIFT] & 0x80)) {
	//	m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_IDLE), false); // IDLE
	//	m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
	//	m_AManager->UpdateAniPosition(0.0f, m_Object);
	//}

	if (!m_bSkillActive && !m_bDoingCombo) {
		if ((keyBuffer['J'] & 0x80) && !(m_PrevKeyBuffer['J'] & 0x80)) {
			m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_HIT), true);
			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
			m_AManager->UpdateAniPosition(0.0f, m_Object);
			m_bSkillActive = true;
		}
		if ((keyBuffer['K'] & 0x80) && !(m_PrevKeyBuffer['K'] & 0x80)) {
			m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_HIT), true);
			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
			m_AManager->UpdateAniPosition(0.0f, m_Object);
			m_bSkillActive = true;
		}
		if ((keyBuffer[VK_SPACE] & 0x80) && !(m_PrevKeyBuffer[VK_SPACE] & 0x80)) {
			XMFLOAT3 dodgeDir = characterDir;

			if (keyBuffer['W'] & 0x80 && keyBuffer['A'] & 0x80) {
				dodgeDir = XMFLOAT3(characterDir.x - characterDir.z, 0.0f, characterDir.z + characterDir.x);
			}
			else if (keyBuffer['W'] & 0x80 && keyBuffer['D'] & 0x80) {
				dodgeDir = XMFLOAT3(characterDir.x + characterDir.z, 0.0f, characterDir.z - characterDir.x);
			}
			else if (keyBuffer['S'] & 0x80 && keyBuffer['A'] & 0x80) {
				dodgeDir = XMFLOAT3(-characterDir.x - characterDir.z, 0.0f, -characterDir.z + characterDir.x);
			}
			else if (keyBuffer['S'] & 0x80 && keyBuffer['D'] & 0x80) {
				dodgeDir = XMFLOAT3(-characterDir.x + characterDir.z, 0.0f, -characterDir.z - characterDir.x);
			}
			else if (keyBuffer['W'] & 0x80) {
				dodgeDir = characterDir;
			}
			else if (keyBuffer['S'] & 0x80) {
				dodgeDir = XMFLOAT3(-characterDir.x, 0.0f, -characterDir.z);
			}
			else if (keyBuffer['A'] & 0x80) {
				dodgeDir = XMFLOAT3(-characterDir.z, 0.0f, characterDir.x);
			}
			else if (keyBuffer['D'] & 0x80) {
				dodgeDir = XMFLOAT3(characterDir.z, 0.0f, -characterDir.x);
			}

			XMStoreFloat3(&dodgeDir, XMVector3Normalize(XMLoadFloat3(&dodgeDir)));

			m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_DODGE), true);
			m_Object->SetLookDirection(dodgeDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
			m_AManager->UpdateAniPosition(0.0f, m_Object);
			m_bSkillActive = true;
			m_bDodged = true;
		}
		if ((keyBuffer['L'] & 0x80) && !(m_PrevKeyBuffer['L'] & 0x80)) {
			m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_BIGHIT), true);
			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
			m_AManager->UpdateAniPosition(0.0f, m_Object);
			m_bSkillActive = true;
		}
		if ((keyBuffer['U'] & 0x80) && !(m_PrevKeyBuffer['U'] & 0x80)) {
			m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_BIGHIT_DEATH), true);
			m_Object->SetLookDirection(characterDir, XMFLOAT3(0.0f, 1.0f, 0.0f));
			m_AManager->UpdateAniPosition(0.0f, m_Object);
			m_bSkillActive = true;
		}
		if ((keyBuffer['Q'] & 0x80) && !(m_PrevKeyBuffer['Q'] & 0x80) &&
			Players[Client.get_id()].GetMP() >= g_SkillCost[0] && g_SkillCurCTime[0] <= 0) {
			Skill1();
			ret = KEY_SKILL1;
		}
		if ((keyBuffer['E'] & 0x80) && !(m_PrevKeyBuffer['E'] & 0x80) &&
			Players[Client.get_id()].GetMP() >= g_SkillCost[1] && g_SkillCurCTime[1] <= 0) {
			Skill2();
			ret = KEY_SKILL2;
		}
		if ((keyBuffer['R'] & 0x80) && !(m_PrevKeyBuffer['R'] & 0x80) &&
			Players[Client.get_id()].GetMP() >= g_SkillCost[2] && g_SkillCurCTime[2] <= 0) {
			Skill3();
			ret = KEY_SKILL3;
		}
	}
	memcpy(m_PrevKeyBuffer, keyBuffer, sizeof(m_PrevKeyBuffer));
	return ret;
}

void CPlayerPriest::UpdateObject(float fElapsedTime)
{
	bool test = false;
	m_GameTime += fElapsedTime;
	if (m_bLive) {
		m_AManager->UpdateCombo(fElapsedTime);
		if (!m_AManager->IsInCombo() && m_AManager->IsAnimationFinished()) {
			m_AManager->ChangeAnimation(static_cast<int>(PriestAni::ANI_IDLE), false);
			test = true;
			m_bSkillActive = false;
			m_bDoingCombo = false;
			m_bAttacked = false;
			m_CurrentSkill = 0;
			m_bDodged = false;
		}
		if (m_AManager->IsComboInterrupted()) {
			test = true;
			m_AManager->ClearComboInterrupted();
			m_bSkillActive = false;
			m_bDoingCombo = false;
			m_bAttacked = false;
			m_bDodged = false;
		}

		if (test) {
			m_AManager->UpdateAniPosition(fElapsedTime, m_Object);
		}

		for (auto& bulletPtr : bullet) {
			if (bulletPtr->getActive()) {
				bulletPtr->IsMoving(fElapsedTime);
			}
		}
	}
}

void CPlayerPriest::MakeBullet(float speed, int skill)
{
	if (bullet.empty()) {
		return;
	}

	size_t startIndex = currentBullet;
	CProjectile* projectile = nullptr;
	do {  
		projectile = bullet[currentBullet].get();
		if (!projectile->getActive()) {
			break;
		}
		currentBullet = (currentBullet + 1) % bullet.size();
	} while (currentBullet != startIndex);

	XMFLOAT3 pos = XMFLOAT3(m_Head->getWorldMatrix()._41, m_Head->getWorldMatrix()._42, m_Head->getWorldMatrix()._43);
	projectile->setSpeed(speed);
	projectile->setLifetime(3.0f);
	projectile->setTime(0.0f);
	projectile->setPosition(pos);
	projectile->setMoveDirection(m_AutoDirect);
	projectile->setActive(true);
	projectile->getObjects().SetRenderState(true);

	currentBullet = (currentBullet + 1) % bullet.size();
}

// =======================================================================================

CPlayer::CPlayer(CPlayableCharacter* playerObject, std::shared_ptr<CCamera>& camera)
	: m_pPlayerObject(playerObject), m_pCamera(camera)
{
	m_pPlayerObject->SetCamera(camera);
}

void CPlayer::MouseProcess(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	m_pPlayerObject->MouseProcess(hWnd, nMessage, wParam, lParam);
}

KeyInputRet CPlayer::ProcessInput(UCHAR* keyBuffer, float fElapsedTime)
{
	return m_pPlayerObject->ProcessInput(keyBuffer, fElapsedTime);
}

void CPlayer::HeightCheck(CHeightMapImage* heightmap, float fElapsedTime, float offsetx, float offsety, float offsetz, short mapNum)
{
	CSkinningObject* p = m_pPlayerObject->getObject();
	XMFLOAT4X4& playerWorld = p->getWorldMatrix();
	XMFLOAT4X4& playerPreWorld = p->getPreWorldMatrix();
	XMFLOAT4X4& objectWorld = p->getObjects()[0]->getWorldMatrix();
	float fy = objectWorld._42 - (30 * fElapsedTime);

	float terrainHeight = heightmap->GetHeightinWorldSpace(objectWorld._41 - offsetx, objectWorld._43 - offsetz);
	switch (mapNum) {
	case SCENE_WINTERLAND:
		if (objectWorld._43 >= -500.0f) {
			if (terrainHeight < 10.0f) {
				terrainHeight = 10.0f;
			}
		}
		break;
	}
	if (fy < terrainHeight + offsety)
		playerWorld._42 = terrainHeight + offsety;
	else
		playerWorld._42 -= (30 * fElapsedTime);
	p->SetPosition(XMFLOAT3(playerWorld._41, playerWorld._42, playerWorld._43));
	playerPreWorld._42 = playerWorld._42;
}

void CPlayer::CollisionCheck(CHeightMapImage* heightmap, float fElapsedTime, float offsetx, float offsety, float offsetz, short mapNum)
{
	CSkinningObject* p = m_pPlayerObject->getObject();
	XMFLOAT4X4& playerWorld = p->getWorldMatrix();
	XMFLOAT4X4& playerPreWorld = p->getPreWorldMatrix();
	XMFLOAT4X4& objectWorld = p->getObjects()[0]->getWorldMatrix();

	float terrainHeight = heightmap->GetHeightinWorldSpace(objectWorld._41 - offsetx, objectWorld._43 - offsetz);

	if (terrainHeight > 0.0) {
		XMFLOAT3 pushdir(m_xmf2PrevPos.x - objectWorld._41, 0.0, m_xmf2PrevPos.y - objectWorld._43);
		playerWorld._41 += pushdir.x; playerWorld._43 += pushdir.z;
		playerPreWorld._41 += pushdir.x; playerPreWorld._43 += pushdir.z;
		objectWorld._41 += pushdir.x; objectWorld._43 += pushdir.z;
	}
	else
		m_xmf2PrevPos.x = objectWorld._41; m_xmf2PrevPos.y = objectWorld._43;

	p->SetPosition(XMFLOAT3(playerWorld._41, playerWorld._42, playerWorld._43));
}

void CPlayer::CollisionCheck(CHeightMapImage* heightmap, CHeightMapImage* CollisionMap, float fElapsedTime, float offsetx, float offsety, float offsetz, short mapNum)
{
	CSkinningObject* p = m_pPlayerObject->getObject();
	XMFLOAT4X4& playerWorld = p->getWorldMatrix();
	XMFLOAT4X4& playerPreWorld = p->getPreWorldMatrix();
	XMFLOAT4X4& objectWorld = p->getObjects()[0]->getWorldMatrix();

	float colHeight = CollisionMap->GetHeightinWorldSpace(objectWorld._41 - offsetx, objectWorld._43 - offsetz);
	float terrainHeight = heightmap->GetHeightinWorldSpace(objectWorld._41 - offsetx, objectWorld._43 - offsetz);

	if (colHeight - terrainHeight >= 0.1f) {
		XMFLOAT3 pushdir(m_xmf2PrevPos.x - objectWorld._41, 0.0, m_xmf2PrevPos.y - objectWorld._43);
		playerWorld._41 += pushdir.x; playerWorld._43 += pushdir.z;
		playerPreWorld._41 += pushdir.x; playerPreWorld._43 += pushdir.z;
		objectWorld._41 += pushdir.x; objectWorld._43 += pushdir.z;
	}
	else
		m_xmf2PrevPos.x = objectWorld._41; m_xmf2PrevPos.y = objectWorld._43;

	p->SetPosition(XMFLOAT3(playerWorld._41, playerWorld._42, playerWorld._43));
}

// =======================================================================================
