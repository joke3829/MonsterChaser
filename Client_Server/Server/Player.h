#pragma once
#include "protocol.h"


class Network;
using namespace DirectX;



enum class BuffType : char {
	ATKUP = 0,
	DEFUP = 1,
	DEF_DOWN = 2
};

class Player {
public:

	int gold = 0; // 플레이어가 획득한 골드
	int local_id;
	int room_num;
	bool isReady = false;


	std::mutex playerMutex;

	Player(int id, const std::string& nickname, int room);

	void SetPosition(const XMFLOAT4X4& pos);
	const XMFLOAT4X4& GetPosition() const;

	bool  TakeDamage(int dmg);
	//void GetDamage(int dmg) {
	//	std::lock_guard<std::mutex> lock(playerMutex);
	//	int damage = dmg - GetDEF(); // 방어력 적용
	//	hp -= damage;
	//	if (hp < 0) hp = 0; // HP가 0 이하로 떨어지지 않도록 처리
	//}

	void Move(float dx, float dy, float dz); // 이동처리 예시

	float GetHP() { return hp; }
	float GetMaxHP() { return max_hp; } // 최대 HP 반환
	void SetHP(float newHP) { hp = newHP; } // Add a setter method for HP

	void PlusHP(int new_hp) {
		std::lock_guard<std::mutex> lock(playerMutex);
		hp += new_hp;
		if (hp > max_hp) hp = max_hp; // HP가 최대치를 넘지 않도록 처리
	}


	float GetMP() { return skill_cost; } // 스킬 사용 비용 반환
	void SetMP(float newMP) { skill_cost = newMP; } // Add a setter method for MP
	void PlusMP(float new_mp) {
		std::lock_guard<std::mutex> lock(playerMutex);
		skill_cost += new_mp;
	}
	void PlaySkill(const int attacktype);
	void setSkillCost(float new_cost) {
		std::lock_guard<std::mutex> lock(playerMutex);
		skill_cost = new_cost;
		if (skill_cost > max_skill_cost) skill_cost = max_skill_cost; // 스킬 사용 비용이 최대치를 넘지 않도록
	}
	// MP 자동 회복 관련 함수
	void RecoverSkillCost(float amount);
	// MP 자동 회복 관련 함수


	void Updatestatus(Character t);

	void AddATKBuff_Potion(float value, float duration_sec);
	void AddATKBuff_Skill(float value, float duration_sec);



	void AddATKBuff(float value, float duration_sec);
	void AddDEFBuff(float value, float duration_sec);
	void AddDEFDEBuff(float value, float duration_sec);


	bool IsATKBuffActive() const {
		return atk_buff_potion > 0 || atk_buff_skill > 0;
	}

	bool IsDEFBuffActive() const {
		return def_buff > 0;
	}

	bool IsDEFDeBuffActive() const {
		return def_debuff > 0;
	}


	float GetDamage(int attacktype);

	bool TakeDamage(int dmg, int type) {
		std::lock_guard<std::mutex> lock(playerMutex);
		int damage = dmg - GetDEF(); // 방어력 적용
		if (damage < 0) damage = 0; // 방어력으로 인해 피해가 0 이하가 되지 않도록 처리
		hp -= damage;

		if (hp < 0)
			hp = 0; // HP가 0 이하로 떨어지지 않도록 처리

		return hp == 0; // HP가 0이 되면 true 반환
	}

	float GetATK();
	float GetDEF();
	


	void SendBuffPacketIfChanged(BuffType type, bool currentState,bool broadcastAll);


	const auto& GetLastHitTime() const { return lastHitTime; }
	const auto& GetLastRecoverTime() const { return lastRecoverTime; }
	void SetLastHitTime() { lastHitTime = std::chrono::steady_clock::now(); }
	void SetLastRecoverTime(std::chrono::steady_clock::time_point t) { lastRecoverTime = t; }

	void setBoanPosition(const XMFLOAT3& pos) {
		Bogan_position = pos;
	}
	const XMFLOAT3& GetBoanPosition() const {
		return Bogan_position;
	}

	void UpdateBuffStatesIfChanged(bool broadcastAll);
	void Death();
	void TryRespawn();
	bool ISDead() const { return isDead; } // 플레이어가 죽었는지 확인하는 함수
	
private:
	Player() = default; // 기본 생성자는 private로 설정하여 사용하지 못하게 함



	std::string name;

	XMFLOAT4X4 position;
	XMFLOAT3 Bogan_position;
	float hp = 100;
	float max_hp = 100; // 최대 HP
	float skill_cost = 100; // 스킬 사용 비용
	float max_skill_cost = 100; // 스킬 사용 비용
	Character type = Character::None; // 캐릭터 타입 (예: 전사, 마법사 등)

	int attack = 10; // 공격력
	float atk_buff = 0.f;   // 추가 공격력

	int defense = 5; // 방어력

	bool isDead = false;
	bool isRespawning = false; // 플레이어가 부활 중인지 여부
	std::chrono::steady_clock::time_point respawnTime; // 부활 시간


	// 공격력 버프
	float atk_buff_potion = 0.f;
	float atk_buff_skill = 0.f;
	std::chrono::steady_clock::time_point atk_buff_potion_end{};
	std::chrono::steady_clock::time_point atk_buff_skill_end{};

	// 추가 방어력
	float def_buff = 0.f;  
	std::chrono::steady_clock::time_point def_buff_end{};

	// 방어력 디버프 (감소)
	float def_debuff = 0.f;
	std::chrono::steady_clock::time_point def_debuff_end{};


	std::unordered_map<BuffType, bool> lastSentBuffState; // 클라에 마지막으로 전송된 버프 상태



	std::chrono::steady_clock::time_point lastHitTime;
	std::chrono::steady_clock::time_point lastRecoverTime;
};
