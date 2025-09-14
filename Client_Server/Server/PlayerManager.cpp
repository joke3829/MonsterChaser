#include "stdafx.h"
#include "PlayerManager.h"

#include "PlayerManager.h"

void PlayerManager::AddPlayer(int id, std::shared_ptr<Player> player) {
    std::lock_guard<std::mutex> lock(managerMutex);
    players[id] = player;
}

void PlayerManager::RemovePlayer(int id) {
    std::lock_guard<std::mutex> lock(managerMutex);
    players.erase(id);
}

void PlayerManager::ApplyDamage(int id, int dmg) {  
    std::lock_guard<std::mutex> lock(managerMutex);  
    auto it = players.find(id);  
    if (it != players.end()) {  
        std::lock_guard<std::mutex> playerLock(it->second->playerMutex);  
        int currentHP = it->second->GetHP(); // Use a temporary variable to store HP  
        currentHP -= dmg;  
        if (currentHP < 0) currentHP = 0;  
        it->second->SetHP(currentHP); // Add a setter method to update HP  
    }  
}      
           


void PlayerManager::SetPosition(int id, const XMFLOAT4X4& pos) {
    std::lock_guard<std::mutex> lock(managerMutex);
    auto it = players.find(id);
    if (it != players.end()) {
        std::lock_guard<std::mutex> playerLock(it->second->playerMutex);
        it->second->SetPosition(pos);
    }
}



void PlayerManager::SetBoganPostion(int id, const XMFLOAT4X4& pos)
{
    std::lock_guard<std::mutex> lock(managerMutex);
    auto it = players.find(id);
    if (it != players.end()) {
        std::lock_guard<std::mutex> playerLock(it->second->playerMutex);
        //it->second->setBoanPosition(pos);
    }
}

std::shared_ptr<Player> PlayerManager::GetPlayer(int id) const {
    std::lock_guard<std::mutex> lock(managerMutex);
    auto it = players.find(id);
    if (it != players.end()) {
        return it->second;
    }
    return nullptr;
}
