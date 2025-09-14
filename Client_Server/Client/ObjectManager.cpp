#include "ObjectManager.h"
extern std::mutex mtx;
extern std::unordered_map<int, Player> Players;

ObjectManager::ObjectManager(int id)
	: my_id(id) {}

int ObjectManager::getID() const {
	return my_id;
}

void ObjectManager::setMatrix(const XMFLOAT4X4& pos) {
	m_Matrix = pos;
}

XMFLOAT3 ObjectManager::getPosition() const {
	XMFLOAT4X4 tempmat = m_Matrix;
	XMFLOAT3 pos = XMFLOAT3(tempmat._41, tempmat._42, tempmat._43);
	return pos;
}

Monster::Monster(int id, MonsterType t) : ObjectManager(id), type(t)
{

	switch (type)
	{

	case MonsterType::Feroptere:
	case MonsterType::Pistiripere:
	case MonsterType::RostrokarackLarvae:
		hp = 15000;
		max_hp = 15000; // 최대 HP 설정
		break;
	case MonsterType::Xenokarce:
		hp = 40000;
		max_hp = 40000; // 최대 HP 설정
		break;
	case MonsterType::Occisodonte:
	case MonsterType::Limadon:
	case MonsterType::Fulgurodonte:
		hp = 30000;
		max_hp = 30000; // 최대 HP 설정
		break;
	case MonsterType::Crassorrid:
		hp = 80000;
		max_hp = 80000; // 최대 HP 설정
		break;
	case MonsterType::Gorhorrid:
		hp = 200000;
		max_hp = 200000; // 최대 HP 설정
		break;

	default:
		hp = 100;
		break;
	}
}



void Monster::setCurrentAttackType(int attackType)
{
	
	switch (attackType)
	{
	case 1:
		currentAttackType = ANIMATION::ANI_SKILL1;
		break;
	case 2:
		currentAttackType = ANIMATION::ANI_SKILL2;
		break;
	case 3:
		currentAttackType = ANIMATION::ANI_SKILL3;
		break;
	default:
		currentAttackType = ANIMATION::ANI_IDLE; // 기본값으로 IDLE 설정
		break;
	}
	
}
