#pragma once
#include "stdafx.h"
#include <mutex>
#include <DirectXMath.h>
#include "GameObject.h"
#include "AnimationManager.h"

// 07.25 ===========================================
extern std::array<bool, 3>	g_PlayerBuffStat;
extern std::array<float, 3> g_maxHPs;
extern std::array<float, 3> g_maxMPs;
extern std::array<float, 3> g_SkillCoolTime;
extern std::array<float, 3> g_SkillCurCTime;
extern std::array<float, 3> g_SkillCost;
// =================================================

class ObjectManager {
public:
    ObjectManager() {}
    ObjectManager(int id);
    virtual ~ObjectManager() = default;

    int getID() const;

    void setMatrix(const XMFLOAT4X4& pos);
    XMFLOAT3 getPosition() const;

    void setRenderingObject(CSkinningObject* obj) { SkinningObject = obj; }
    void setAnimationManager(CAnimationManager* ani) { AnimationManager = ani; }

    CSkinningObject* getRenderingObject() { return SkinningObject; }
    CAnimationManager* getAnimationManager() { return AnimationManager; }

    void setPosition(const XMFLOAT4X4& pos) {
        setMatrix(pos);
        if (SkinningObject) {
            XMFLOAT3 pos3 = { pos._41, pos._42 ,pos._43 };
            SkinningObject->SetPosition(pos3);
        }
    }

protected:
    /*std::atomic<int> my_id;
    std::atomic<XMFLOAT4X4> m_Matrix;*/
    int my_id;
    XMFLOAT4X4 m_Matrix;
    CSkinningObject* SkinningObject = nullptr;
    CAnimationManager* AnimationManager = nullptr;
};

class Player : public ObjectManager {
public:
    Player() {}
    Player(int id) : ObjectManager(id) {}

    bool getReady() const { return readyToStart; }
    void setReady(const bool& ready) { readyToStart = ready; }

    void setCharacterType(const short t) { type = t; }
    short getCharacterType() { return type; }

    bool TakeDamage(int dmg) {
        hp -= dmg;
        if (hp < 0) hp = 0;
        return hp == 0;
    }

    float GetHP() const {
        return hp;
    }

    int GetMP() const {
        return mp;
    }



    void Plusgold(int amount) {
        gold += amount;
	}
    void SetHP(float newHP) {
        float thp = hp;
        float tmaxhp = maxHP;
        thp = newHP;
        if (thp > tmaxhp) thp = tmaxhp;
        hp = thp;
        //hp = newHP;
        //if (hp > maxHP) hp = maxHP; // HP�� �ִ�ġ�� ���� �ʵ���
	}
    void SetMaxHP(float newMaxHP) {
        maxHP = newMaxHP;
        float tmaxhp = maxHP;
        float thp = hp;
        if (thp > tmaxhp) { thp = tmaxhp; hp = thp; } // ���� HP�� �ִ�ġ���� ũ�� �ִ�ġ�� ����
    }

    void SetMP(float newMP) {
        float tmp = mp;
        float tmaxmp = maxMP;
        tmp = newMP;
        if (tmp > tmaxmp) tmp = tmaxmp;
        mp = tmp;
        //mp = newMP;
        //if (mp > maxMP) mp = maxMP; // MP�� �ִ�ġ�� ���� �ʵ���
	}
    void SetMaxMP(float newMaxMP) {
        maxMP = newMaxMP;
        float tmaxmp = maxMP;
        float tmp = mp;
        if (tmp > tmaxmp) { tmp = tmaxmp; mp = tmp; } // ���� MP�� �ִ�ġ���� ũ�� �ִ�ġ�� ����
    }

private:
    //std::atomic<bool> readyToStart = false;
    //std::atomic<float> hp = 0;
    //std::atomic<float> maxHP = 100; // �ִ� HP

    //std::atomic<float> mp = 100; // �÷��̾ ���� MP
    //std::atomic<float> maxMP = 100; // �ִ� MP

    //std::atomic<float> gold = 0; // �÷��̾ ���� ���    

    //std::atomic<short> type{ JOB_NOTHING };

    bool readyToStart = false;
    float hp = 0;
    float maxHP = 100; // �ִ� HP

    float mp = 100; // �÷��̾ ���� MP
    float maxMP = 100; // �ִ� MP

    float gold = 0; // �÷��̾ ���� ���    

    short type{ JOB_NOTHING };
};

class Monster : public ObjectManager {
public:


    enum class ANIMATION{
        ANI_DEATH,
        ANI_HIT,
        ANI_IDLE,
        ANI_ROAR,
        ANI_FRONT,
        ANI_BACK,
        ANI_SKILL1,
        ANI_SKILL2,
        ANI_SKILL3,
        ANI_RUN
    };
    Monster(int id, MonsterType t);

    void setTargetID(int tid) { m_targetID = tid; }
    int getTargetID() const { return m_targetID; }

	void setMonsterType(MonsterType t) { type = t; }
	MonsterType getMonsterType  () const { return type; }

	void setSpawnPoint(const XMFLOAT3& point) 
    { 
        auto p = point;
        spawnPoint = p; 
    }
	void setHP(int newHP) { hp = newHP; } 

    void setCurrentAttackType(int attackType);
        
    UINT getCurrentAttackType() { return static_cast<UINT>(currentAttackType); }
  
    float getMaxHP() { return max_hp; }
    float getHP() { return hp; }
private:
    //std::atomic<int> m_targetID = -1;
    //std::atomic<float> hp = -1;
    //std::atomic<float> max_hp = -1;
	
    //std::atomic < MonsterType> type;
    //std::atomic < XMFLOAT3> spawnPoint; // ���� ���� ��ġ

    //std::atomic < ANIMATION> currentAttackType;

    int m_targetID = -1;
    float hp = -1;
    float max_hp = -1;

    MonsterType type;
    XMFLOAT3 spawnPoint; // ���� ���� ��ġ

    ANIMATION currentAttackType;
};
