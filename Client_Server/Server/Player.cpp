#include "stdafx.h"
#include "Player.h"
#include "Network.h"

extern Network g_server;

Player::Player(int id, const std::string& nickname, int room)
	: local_id(id), name(nickname), room_num(room)
{
	lastSentBuffState[BuffType::ATKUP] = false;
	lastSentBuffState[BuffType::DEFUP] = false;
	lastSentBuffState[BuffType::DEF_DOWN] = false;
	position = XMFLOAT4X4(); // 초기 위치 0
}

void Player::SetPosition(const XMFLOAT4X4& pos) {
	position = pos;
}

const XMFLOAT4X4& Player::GetPosition() const {
	return position;
}

bool  Player::TakeDamage(int dmg) {
	std::lock_guard<std::mutex> lock(playerMutex);
 	hp -= dmg;
	if (hp <= 0) {
		Death();
		hp = 0;
	}
	return hp <= 0; // 0이 되면 죽음
}

void Player::Move(float dx, float dy, float dz) {
	position._41 += dx;
	position._42 += dy;
	position._43 += dz;
}


void Player::PlaySkill(const int attacktype)
{
	switch (type)
	{
	case None:
		break;
	case Wizard:
	{
		std::lock_guard<std::mutex> lock(playerMutex);
		if (attacktype == 1) // 스킬 공격
			skill_cost -= 25; // 스킬 공격은 20 스킬 비용
		else if (attacktype == 2) // 메테오
			skill_cost -= 40; // 메테오 스킬은 50 스킬 비용
		else if (attacktype == 3) // 궁극기
			skill_cost -= 70; // 궁극기는 100 스킬 비용
		break;
	}
	case Warrior:
	{
		std::lock_guard<std::mutex> lock(playerMutex);
		if (attacktype == 1) // 스킬 공격
			skill_cost -= 30; // 스킬 공격은 20 스킬 비용
		if (attacktype == 2) // 메테오
			skill_cost -= 20; // 메테오 스킬은 50 스킬 비용
		else if (attacktype == 3) // 궁극기
			skill_cost -= 40; // 궁극기는 100 스킬 비용
		break;
	}
	case Priest:
	{
		std::lock_guard<std::mutex> lock(playerMutex);
		if (attacktype == 1) // 스킬 공격
			skill_cost -= 30; // 스킬 공격은 20 스킬 비용
		else if (attacktype == 2) // 메테오
			skill_cost -= 40; // 메테오 스킬은 50 스킬 비용
		else if (attacktype == 3) // 궁극기
			skill_cost -= 60; // 궁극기는 100 스킬 비용
		break;
	}
	default:
		break;
	}
}

void Player::RecoverSkillCost(float amount)
{
	skill_cost += amount;
	if (skill_cost > max_skill_cost) {
		skill_cost = max_skill_cost;
	}
}

void Player::Updatestatus(Character t)
{
	type = t;
	switch (type)
	{
	case None:
		break;
	case Wizard:
	{
		hp = 800;
		max_hp = hp; // 최대 HP 설정
		skill_cost = 100; // 스킬 사용 비용
		max_skill_cost = skill_cost; // 스킬 사용 비용
		attack = 800; // 공격력
		defense = 30; // 방어력
		break;
	}
	case Warrior:
	{
		hp = 1200;
		max_hp = hp; // 최대 HP 설정
		skill_cost = 100; // 스킬 사용 비용
		max_skill_cost = skill_cost; // 스킬 사용 비용
		attack = 600; // 공격력
		defense = 70; // 방어력
		break;
	}
	case Priest:
	{
		hp = 1000;
		max_hp = hp; // 최대 HP 설정
		skill_cost = 100; // 스킬 사용 비용
		max_skill_cost = skill_cost; // 스킬 사용 비용
		attack = 700; // 공격력
		defense = 30; // 방어력
		break;
	}
	default:
		break;
	}
}

void Player::AddATKBuff_Potion(float value, float duration_sec)
{
	atk_buff_potion = value;
	atk_buff_potion_end = std::chrono::steady_clock::now() + std::chrono::seconds((int)duration_sec);
}

void Player::AddATKBuff_Skill(float value, float duration_sec)
{
	atk_buff_skill = value;
	atk_buff_skill_end = std::chrono::steady_clock::now() + std::chrono::seconds((int)duration_sec);
}





void Player::AddATKBuff(float value, float duration_sec)
{
	atk_buff = value;
	atk_buff_skill_end = std::chrono::steady_clock::now() + std::chrono::seconds((int)duration_sec);
	std::cout << "[공격력 버프 적용] +" << value << " for " << duration_sec << " seconds\n";

}
void Player::AddDEFBuff(float value, float duration_sec)
{
	def_buff = value;
	def_buff_end = std::chrono::steady_clock::now() + std::chrono::seconds((int)duration_sec);
	std::cout << "[방어력 버프 적용] +" << value << " for " << duration_sec << " seconds\n";

}
void Player::AddDEFDEBuff(float value, float duration_sec)
{
	def_debuff = value;
	def_debuff_end = std::chrono::steady_clock::now() + std::chrono::seconds((int)duration_sec);
	std::cout << "[방어력 버프 적용] +" << value << " for " << duration_sec << " seconds\n";
}

float Player::GetDamage(int attacktype)
{


	float attack = GetATK();
	switch (type)
	{
	case None:
		break;
	case Wizard:
	{
		if (attacktype == 0) // 일반 공격
			return attack; // 일반 공격은 기본 공격력						800

		else if (attacktype == 1) // 스킬 공격
			return  attack * 0.5f; // 5발 쏘니까 다 맞추면 2000

		else if (attacktype == 2) // 메테오
			return attack * 4.0f; // 강력한 스킬 공격은 4.0배

		else if (attacktype == 3) // 궁극기
			return attack * 4.0f; // 궁극기는 4배
	}
	break;
	case Warrior:
	{
		if (attacktype == 0) // 일반 공격
			return attack; // 일반 공격은 기본 공격력						600

		else if (attacktype == 1) // 스킬 공격
			return  attack * 3.0f; // 스킬 공격은 3배

		else if (attacktype == 2) // 강력한 스킬 공격
			return attack * 3.3f; // 강력한 스킬 공격은 3.3배

		else if (attacktype == 3) // 궁극기
			return attack * 4.0f; // 궁극기는 4배
	}
	break;
	case Priest:
		return attack; // 프리스트는 일반 공격만 사용						700
		break;
	default:
		break;
	}

}

float Player::GetATK() {
	
	return attack + atk_buff_potion + atk_buff_skill;
}


float Player::GetDEF() {
	return defense + def_buff - def_debuff;
}

void Player::UpdateBuffStatesIfChanged(bool broadcastAll)
{
	auto now = std::chrono::steady_clock::now();

	if (now >= atk_buff_potion_end)
	{
		atk_buff_potion = 0.0f;
		//cout << "[공격력 버프 종료] 포션 버프가 만료되었습니다.\n";
	}
	if (now >= atk_buff_skill_end) {
		atk_buff_skill = 0.0f;
		//cout << "[공격력 버프 종료] 스킬 버프가 만료되었습니다.\n";
	}
	if (now >= def_buff_end) {
		def_buff = 0.f;
		//	cout << "[방어력 버프 종료] 방어력 버프가 만료되었습니다.\n";
	}
	if (now >= def_debuff_end)
	{
		def_debuff = 0.f;
		//cout << "[방어력 감소 버프 종료] 방어력 감소 버프가 만료되었습니다.\n";
	}
	// 현재 버프 상태 계산
	bool atkActive = (now < atk_buff_potion_end) || (now < atk_buff_skill_end);
	bool defActive = (now < def_buff_end);
	bool defDownActive = (now < def_debuff_end);

	// 각 상태 비교 및 전송
	SendBuffPacketIfChanged(BuffType::ATKUP, atkActive, broadcastAll);
	SendBuffPacketIfChanged(BuffType::DEFUP, defActive, broadcastAll);
	SendBuffPacketIfChanged(BuffType::DEF_DOWN, defDownActive, broadcastAll);
}

void Player::SendBuffPacketIfChanged(BuffType type, bool currentState, bool broadcastAll)
{
	// 이전에 전송한 상태와 다르면 새로 전송
	if (lastSentBuffState[type] != currentState) {
		lastSentBuffState[type] = currentState;

		sc_packet_buff_change pkt;
		pkt.size = sizeof(pkt);
		pkt.type = S2C_P_BUFFCHANGE;
		pkt.bufftype = static_cast<char>(type);
		pkt.state = currentState ? 1 : 0;

		if (broadcastAll) {
			for (int pid : g_server.rooms[room_num].id)
				g_server.users[pid]->do_send(&pkt);
		}
		else {
			auto& roomIds = g_server.rooms[room_num].id;
			if (roomIds.size() <= local_id) {//로컬 아이디가 vector out of range 방어
				return;
			}
			g_server.users[roomIds[local_id]]->do_send(&pkt); // 본인에게만
		}

		std::cout << "[버프 상태 변경] 플레이어 " << local_id
			<< " | 타입: " << static_cast<int>(type)
			<< " | 상태: " << static_cast<int>(pkt.state)
			<< " | ATK: " << GetATK() << ", DEF: " << GetDEF() << "\n";
	}
}


void Player::Death()
{

	if (isDead)return;

	isDead = true;
	isRespawning = true;
	respawnTime = std::chrono::steady_clock::now() + std::chrono::seconds(10);
}

void Player::TryRespawn()
{
	std::lock_guard<std::mutex> lock(playerMutex);
	if (!isDead || !isRespawning) return;

	auto now = std::chrono::steady_clock::now();	
	if(now>=respawnTime) {
		hp = max_hp; // 부활 시 최대 HP로 초기화
		skill_cost = max_skill_cost; // 스킬 비용 초기화
		isDead = false;
		isRespawning = false;
		
		
		sc_packet_respawn pkt;
		pkt.size = sizeof(pkt);
		pkt.type = S2C_P_PlAYER_RESPAWN;
		pkt.Local_id = local_id;
		pkt.hp = max_hp;
		pkt.mp = 100;
		XMStoreFloat4x4(&pkt.pos, XMLoadFloat4x4(&position));
		
		for (int pid : g_server.rooms[room_num].id)
			g_server.users[pid]->do_send(&pkt);
		
		std::cout << "[플레이어 부활] 플레이어 " << local_id << "이(가) 부활했습니다.\n";
	}
}

