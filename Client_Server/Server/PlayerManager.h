#pragma once
#include "Player.h"

class PlayerManager {
public:
    // 플레이어 추가 및 제거
    void AddPlayer(int id, std::shared_ptr<Player> player);
    void RemovePlayer(int id);

    // 데미지 적용
    void ApplyDamage(int id, int dmg);

    // 위치 설정
    void SetPosition(int id, const XMFLOAT4X4& pos);
    void SetBoganPostion(int id, const XMFLOAT4X4& pos);
    std::shared_ptr<Player> GetPlayer(int id) const;

private:
    std::unordered_map<int, std::shared_ptr<Player>> players;
    mutable  std::mutex managerMutex; // 전체 player map 보호 const맴버 함수에서도 뮤텍스처럼 사용 가능 
};

