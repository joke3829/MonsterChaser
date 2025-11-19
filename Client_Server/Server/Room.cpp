// Room.cpp
#include "stdafx.h"
#include "Room.h"
#include "Network.h"

extern Network g_server;

Room::Room() {}

Room::~Room() {
	StopGame();
}

bool Room::IsAllReady()
{
	for (int i = 0; i < id.size(); ++i) {
		if (i >= Ready_user.size() || Ready_user[i] == false)
			return false; // 누군가 준비 안함
	}
	return true; // 모두 준비 완료
}
void Room::SetReady_User(int local_id, bool state)
{
	Ready_user[local_id] = state;

};
	


void Room::setReady(int local_id, bool state) {
	player_ready[local_id] = state;
}

bool Room::isAllGameStartReady() {
	int count = GetPlayerCount();
	for (int i = 0; i < count; ++i)
		if (!player_ready[i]) return false;
	return true;
}

void Room::InitailizeReadyingame() {
	for (auto& r : player_ready) r = false;
}



int Room::GetPlayerCount() {
	return static_cast<int>(id.size());
}

bool Room::IsAddPlayer() {
	return id.size() < MAX_ROOM_MEMBER;
}

void Room::AddPlayer(int pid) {
	id.push_back(pid);
}

void Room::RemovePlayer(int pid) {
	id.erase(std::remove(id.begin(), id.end(), pid), id.end());
}

void Room::StartGame() {
	
	if (!bMonsterThreadRunning) {
		bMonsterThreadRunning = true;
		monsterThread = std::thread(&Room::MonsterThreadFunction, this);
	}
}

void Room::StopGame() {
	bMonsterThreadRunning = false;
	if (monsterThread.joinable())
		monsterThread.join();
	bStageActive = false;
}

void Room::MonsterThreadFunction() {
	auto lastUpdateTime = std::chrono::steady_clock::now();

	while (bMonsterThreadRunning) {
		if (!bStageActive) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		auto now = std::chrono::steady_clock::now();
		float deltaTime = std::chrono::duration<float>(now - lastUpdateTime).count();
		lastUpdateTime = now;

		// 몬스터 업데이트
		for (auto& [id, monster] : monsters)
			//monster->Update(deltaTime, *this, g_server.playerManager);
			monster->Update(0.016, *this, g_server.playerManager);

		ResolveMonsterSeparation();  // <- 충돌 방지 처리도 함께

		// 플레이어 MP 회복 및 버프 상태 갱신
		for (int i = 0; i < GetPlayerCount(); ++i) {
			auto now = std::chrono::steady_clock::now();
			auto player = g_server.playerManager.GetPlayer(id[i]);
			if (!player) continue;
			lastHitTime[i] = player->GetLastHitTime();
			float elapsed = std::chrono::duration<float>(now - lastHitTime[i]).count();
			player->TryRespawn(); // 플레이어가 사망했으면 리스폰 시도

			// 아직 피격 후 10초가 지나지 않았으면 회복 차단
			if (elapsed < 10.0f)
				continue;
			if (elapsed >= 0.016f) {
				player->RecoverSkillCost(0.1); // 초당 1 회복

			}

			player->UpdateBuffStatesIfChanged(false);  // ✅ 버프 상태 변경 감지 및 패킷 전송
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 약 60fps 주기
	}
}
void Room::setStage(short stage)
{
	currentStage = stage;

}
void Room::SpawnMonsters()
{
	// 예시로 1마리 생성
	int new_id = 0;
	/*XMFLOAT3 spawnPos = { -28.0f, 0.0f, -245.0f };
	monsters[new_id] = std::make_shared<Monster>(new_id, spawnPos);*/

	if (!monsters.empty()) return;

	// 예: 1스테이지 기준
	switch (currentStage)
	{
	case 1:
	{
		// Feroptere - 2마리
		monsters[new_id++] = std::make_shared<Monster>(new_id, XMFLOAT3(-280.0f,
			getHeight(g_pStage1Height.get(), -280.0f, 215.4f, -512.0f, 0.0f, -512.0f, SCENE_PLAIN)
			, 215.4f), MonsterType::Feroptere);
		monsters[new_id++] = std::make_shared<Monster>(new_id, XMFLOAT3(-246.3,
			getHeight(g_pStage1Height.get(), -246.3, 15.1, -512.0f, 0.0f, -512.0f, SCENE_PLAIN)
			, 15.1), MonsterType::Feroptere);

		// Pistiripere - 2마리
		monsters[new_id++] = std::make_shared<Monster>(new_id, XMFLOAT3(-240, getHeight(g_pStage1Height.get(), -240.0f, 149.8f, -512.0f, 0.0f, -512.0f, SCENE_PLAIN), 149.8), MonsterType::Pistiripere);
		monsters[new_id++] = std::make_shared<Monster>(new_id, XMFLOAT3(-351.1, getHeight(g_pStage1Height.get(), -351.1f, 26.7f, -512.0f, 0.0f, -512.0f, SCENE_PLAIN), 26.7), MonsterType::Pistiripere);

		// RostrokarackLarvae - 2마리

		monsters[new_id++] = std::make_shared<Monster>(new_id, XMFLOAT3(-150.5, getHeight(g_pStage1Height.get(), -150.5f, 85.7f, -512.0f, 0.0f, -512.0f, SCENE_PLAIN), 85.7), MonsterType::RostrokarackLarvae);
		monsters[new_id++] = std::make_shared<Monster>(new_id, XMFLOAT3(-164.7, getHeight(g_pStage1Height.get(), -164.7f, 66.0f, -512.0f, 0.0f, -512.0f, SCENE_PLAIN), 66), MonsterType::RostrokarackLarvae);

		// Boss - Xenokarce
		monsters[new_id++] = std::make_shared<Monster>(new_id, XMFLOAT3(-306.7, getHeight(g_pStage1Height.get(), -306.7f, -150.8f, -512.0f, 0.0f, -512.0f, SCENE_PLAIN), -150.8), MonsterType::Xenokarce);




		break;
	}
	case 2:
	{
		monsters[new_id++] = std::make_shared<Monster>(new_id, XMFLOAT3(-18.0f, getHeight(g_pStage2Height.get(), -18.0f, -15.5f,
			-200.0f, -10.0f, -66.5f, SCENE_CAVE), -15.5f), MonsterType::Limadon);

		monsters[new_id++] = std::make_shared<Monster>(new_id, XMFLOAT3(-152.1f,getHeight(g_pStage2Height.get(), -152.1f, 246.3f,
			-200.0f, -10.0f, -66.5f, SCENE_CAVE), 246.3f), MonsterType::Limadon);

		monsters[new_id++] = std::make_shared<Monster>(new_id, XMFLOAT3(-160.0f,getHeight(g_pStage2Height.get(), -160.0f, 78.6f,
			-200.0f, -10.0f, -66.5f, SCENE_CAVE), 78.6f), MonsterType::Fulgurodonte);

		monsters[new_id++] = std::make_shared<Monster>(new_id, XMFLOAT3(-128.6f, getHeight(g_pStage2Height.get(), -128.6f, 272.8f,
			-200.0f, -10.0f, -66.5f, SCENE_CAVE), 272.8f), MonsterType::Fulgurodonte);

		monsters[new_id++] = std::make_shared<Monster>(new_id, XMFLOAT3(-92.0f, getHeight(g_pStage2Height.get(), -92.0f, 376.5f,
			-200.0f, -10.0f, -66.5f, SCENE_CAVE), 376.5f), MonsterType::Fulgurodonte);

		monsters[new_id++] = std::make_shared<Monster>(new_id, XMFLOAT3(-30.0f, getHeight(g_pStage2Height.get(), -30.0f, 21.0f,
			-200.0f, -10.0f, -66.5f, SCENE_CAVE), 21.0f), MonsterType::Occisodonte);

		monsters[new_id++] = std::make_shared<Monster>(new_id, XMFLOAT3(-97.0f, getHeight(g_pStage2Height.get(), -97.0f, 315.3f,
			-200.0f, -10.0f, -66.5f, SCENE_CAVE), 315.3f), MonsterType::Occisodonte);


		// Boss: Crassorrid (추가 MonsterType 필요 시 정의해둬야 함)
		monsters[new_id++] = std::make_shared<Monster>(new_id, XMFLOAT3(0.5f, getHeight(g_pStage2Height.get(), 0.5f, 362.8f,
			-200.0f, -10.0f, -66.5f, SCENE_CAVE), 362.8f), MonsterType::Crassorrid);

		break;
	}
	case 3:
	{
		// Final Boss: Gorhorrid
		monsters[new_id++] = std::make_shared<Monster>(new_id, XMFLOAT3(-86.3f, getHeight(g_pStage3Height.get(), -86.3f, -301.1f,
			-1024.0f, 0.0f, -1024.0f, SCENE_WINTERLAND), -301.1f), MonsterType::Gorhorrid);
		break;
	}
	default:
		break;
	}
	for (auto& [monster_id, monster] : monsters) {
		// 몬스터 스폰 패킷 전송
		sc_packet_monster_spawn pkt;
		pkt.size = sizeof(pkt);
		pkt.type = S2C_P_MONSTER_SPAWN;
		pkt.monster_id = monster_id;
		XMStoreFloat4x4(&pkt.pos, XMMatrixTranslation(monster->GetPosition().x, monster->GetPosition().y, monster->GetPosition().z));

		for (int pid : id) {
			g_server.users[pid]->do_send(&pkt);
		}
	}

}

void Room::ResolveMonsterSeparation() {
	const float MIN_SEPARATION = 5.0f;
	const float PUSH_STRENGTH = 5.0f;

	for (auto& [idA, monA] : monsters) {
		//if (monA->GetState() == MonsterState::Attack) continue; // 공격 중인 몬스터는 예외

		for (auto& [idB, monB] : monsters) {
			if (idA == idB) continue;
			//if (monB->GetState() == MonsterState::Attack) continue; // 상대가 공격 중이면 예외

			XMFLOAT3 posA = monA->GetPosition();
			XMFLOAT3 posB = monB->GetPosition();

			float dx = posA.x - posB.x;
			float dz = posA.z - posB.z;
			float distSq = dx * dx + dz * dz;

			if (distSq < MIN_SEPARATION * MIN_SEPARATION && distSq > 0.01f) {
				float dist = sqrtf(distSq);
				float push = (MIN_SEPARATION - dist) * 0.5f;

				dx /= dist;
				dz /= dist;

				monA->MovePosition(dx * push * PUSH_STRENGTH, dz * push * PUSH_STRENGTH);
				monB->MovePosition(-dx * push * PUSH_STRENGTH, -dz * push * PUSH_STRENGTH);
			}
		}
	}
}

void Room::ResetGame()
{
	// 플레이어 준비 상태 초기화
	Ready_user = { false, false, false };
	InGameStart = false;
	// 플레이어가 있으면 각자의 상태도 리셋
	for (auto& player_id : id) {
		if (player_id != -1) {
			auto player = g_server.playerManager.GetPlayer(player_id);
			if (player) {
				player->isReady = false;
				//player->inGame = false;
				//player->local_id = -1;
				//player->room_num = -1;
			}
		}
	}

	// 플레이어 리스트 초기화
	id.clear();

	// 몬스터 / 스테이지 관련
	monsters.clear();
	setStage(SCENE_TITLE);
	bStageActive = false;
	bMonsterThreadRunning = false;
	selected_characters.clear();
	// 필요 시 추가 초기화
	// 예: 보스 클리어 여부, 스코어, 타이머 등
}







