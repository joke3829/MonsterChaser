#pragma once

#define NOMINMAX

#include <iostream>
#include <WS2tcpip.h>
#include <d3d12.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <WinSock2.h>
#include <thread>
#include <algorithm>
#include <DirectXMath.h>
#include <mutex>
#include <windows.h>
#include <concurrent_unordered_map.h>
#include <MSWSock.h>
#include <chrono>
#include <fstream>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
#pragma comment(lib, "d3d12.lib")

using namespace DirectX;
using namespace std;
using namespace concurrency;
using namespace chrono;




// define Scene
constexpr unsigned short SCENE_TITLE = 0;
constexpr unsigned short SCENE_PLAIN = 1;
constexpr unsigned short SCENE_CAVE = 2;
constexpr unsigned short SCENE_WINTERLAND = 3;

#define MAX_USER 5000


enum MoveAnimationState
{
	IDLE = 0,
	WALK_FORWARD = 5,
	WALK_LEFT_UP = 6,
	WALK_RIGHT_UP = 7,
	WALK_LEFT = 8,
	WALK_RIGHT = 9,
	WALK_BACKWARD = 10,
	WALK_LEFT_DOWN = 11,
	WALK_RIGHT_DOWN = 12,
	RUN_FORWARD = 13,
	RUN_LEFT_UP = 14,
	RUN_RIGHT_UP = 15,
	RUN_LEFT = 16,
	RUN_RIGHT = 17,
	RUN_BACKWARD = 18,
	RUN_LEFT_DOWN = 19,
	RUN_RIGHT_DOWN = 20,
};
enum Character:char {
	None = 0,
	Wizard,
	Warrior,
	Priest
};


enum class MonsterType {
	None = 0,
	Feroptere,
	Pistiripere,
	RostrokarackLarvae,
	Xenokarce,
	Occisodonte,
	Limadon,
	Fulgurodonte,
	Crassorrid,
	Gorhorrid,
};

enum class ItemType : unsigned char {
	HP_POTION = 0,
	MP_POTION,
	ATK_BUFF,
	DEF_BUFF,
	ITEM_COUNT
};