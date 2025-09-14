// Room.h
#pragma once
#include <array>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include "Player.h"
#include "Monster.h"

enum Stage {
    Stage1 = 1,
    Stage2 = 2,
    Stage3 = 3
};
class Monster;  // Forward declaration of Monster class

class Room {
public:
    std::vector<int> id; 
    std::unordered_map<int, std::shared_ptr<Monster>> monsters;
    std::unordered_map<int, short> selected_characters;

    std::mutex RoomMutex;
    std::thread monsterThread;

    std::array<bool, 3> player_ready = { false, false, false };
    std::array<std::chrono::steady_clock::time_point, 3> lastHitTime;
 

    std::array<int, 3> Ready_user{ false,false,false };
	bool InGameStart = false; // 게임 시작 여부
    bool bStageActive = false;
    bool bMonsterThreadRunning = false;
    short currentStage = SCENE_TITLE;

    Room();
    ~Room();
    bool IsAllReady();
	void SetReady_User(int local_id, bool state);


    void setReady(int local_id, bool state);
    bool isAllGameStartReady();
    void InitailizeReadyingame();

   
 
    int GetPlayerCount();
    bool IsAddPlayer();

    void AddPlayer(int id);
    void RemovePlayer(int id);

	void setStage(short stage);
    void SpawnMonsters();
    void StartGame();
    void StopGame();
    void MonsterThreadFunction();
    void ResolveMonsterSeparation();


    void ResetGame();
};