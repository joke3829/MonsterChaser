#pragma once
#include "stdafx.h"

constexpr int PORT_NUM = 3500;
constexpr int BUF_SIZE = 256;
constexpr int MAX_ROOM = 10;
constexpr int MAX_ROOM_MEMBER = 3;

constexpr char MAX_ID_LEN = 20;

#pragma pack(push, 1)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Server to Client packets
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
constexpr char S2C_P_ENTER = 1;
constexpr char S2C_P_CREATEUSER = 2;
constexpr char S2C_P_SELECT_ROOM = 3;
constexpr char S2C_P_ALLREADY = 4;
constexpr char S2C_P_UPDATEROOM = 5;
constexpr char S2C_P_SETREADY = 6;
constexpr char S2C_P_MOVE = 7;
constexpr char S2C_P_PICKCHARACTER = 8;
constexpr char S2C_P_INGAME_START = 9;
constexpr char S2C_P_MONSTER_SPAWN = 10;
constexpr char S2C_P_MONSTER_ATTACK = 11;  // 몬스터가 플레이어 공격한 상태로 바꾸기   
constexpr char S2C_P_MONSTER_HIT = 12;  // 몬스터가 플레이어 공격한 상태로 바꾸기   
constexpr char S2C_P_MONSTER_DIE = 13;
constexpr char S2C_P_MONSTER_RESPAWN = 14;
constexpr char S2C_P_MONSTER_MOVE = 15;
constexpr char S2C_P_PLAYER_HIT = 16;  // 플레이어가 공격해서 몬스터가 맞았을 때
constexpr char S2C_P_NEXTSTAGE = 17;  //  보스몬스터 처치 후 다음 스테이지로 넘어갈 때
constexpr char S2C_P_CHANGEHP = 18;  // HP 아이템 사용 시
constexpr char S2C_P_CHANGEMP = 19;  // MP 아이템 사용 시
constexpr char S2C_P_BOSS_ROAR = 20; // 다음 패킷 번호
constexpr char S2C_P_BUFFCHANGE = 21;
constexpr char S2C_P_MONSTERIDLE = 22;
constexpr char S2C_P_PlAYER_DIE = 23;
constexpr char S2C_P_PlAYER_RESPAWN = 24;


constexpr char S2C_P_LEAVE = 49; // 플레이어가 방을 나갈 때
struct sc_packet_enter {
	unsigned char size;
	char type;
};

struct sc_packet_createUser {
	unsigned char size;
	char type;
	int id;
	char Nickname[MAX_ID_LEN];
	bool loginSuccess = false;
};

struct sc_packet_select_room {
	unsigned char size;
	char type;
	int Local_id;
	char room_number;
	bool is_self;
};

struct sc_packet_Ingame_start {
	unsigned char size;
	char type;
	int Local_id;
	char room_number;
};

struct sc_packet_room_info {
	unsigned char size;
	char type;
	short room_info[10];
};

struct sc_packet_set_ready {
	unsigned char size;
	char type;
	int Local_id;
	char room_number;
	bool is_ready;
};

struct sc_packet_move {
	unsigned char size;
	char type;
	int Local_id;
	XMFLOAT4X4 pos;
	float time;
	UINT state;
	UINT pingTime;
	float mp;
	float hp;
};

struct sc_packet_pickcharacter {
	unsigned char size;
	char type;
	int Local_id;
	float Max_HP;
	float Max_MP;
	short C_type;
};

struct sc_packet_ingame_start {
	unsigned char size;
	char type;
};

struct sc_packet_monster_spawn {
	unsigned char size;
	char type;
	int monster_id;
	MonsterType monster_type;
	XMFLOAT4X4 pos;
};

struct sc_packet_monster_attack  //몬스터가 공격 상태일 떄
{
	unsigned char size;
	char type;
	int monster_id;
	char attack_type; // Attacktype
};
struct sc_packet_monster_hit  //몬스터가 맞았을때
{
	unsigned char size;
	char type;
	int monster_id;
	float hp;
};
struct sc_packet_monster_die  //몬스터가 죽었을때
{
	unsigned char size;
	char type;
	int monster_id;
	int gold;
};

struct sc_packet_monster_respawn {
	unsigned char size;
	char type;
	int monster_id;
	XMFLOAT4X4 pos;
};

struct sc_packet_monster_move {
	unsigned char size;
	char type;
	int monster_id;
	XMFLOAT4X4 pos;
	int state;  // MonsterState (Idle, Chase, Attack, Return, Dead)

};

struct sc_packet_player_hit {
	unsigned char size;
	char type;
	int local_id;
	float hp;
};

struct sc_packet_NextStage {
	unsigned char size;
	char type;
};

struct sc_packet_leave {
	unsigned char size;
	char type;
	int Local_id;
};

struct sc_packet_change_hp {
	unsigned char size;
	char type;
	float hp;
	char local_id;
};
struct sc_packet_change_mp {
	unsigned char size;
	char type;
	float mp;
	char local_id;
};

struct sc_packet_boss_roar {
	unsigned char size;
	char type;
	int monster_id;
};

struct sc_packet_buff_change {
	unsigned char size;
	char type;
	char bufftype;		// 0: 공격력 증가, 1: 방어력 증가, 2: 방어력 감소
	char state;			// 0: 꺼짐, 1: 켜짐
};

struct sc_packet_monster_idle {
	unsigned char size;
	char type;
	int monster_id;
	XMFLOAT4X4 pos;
};
struct sc_packet_player_die {
	unsigned char size;
	char type;
	int Local_id;
};

struct sc_packet_respawn {
	unsigned char size;
	char type;
	int Local_id;
	XMFLOAT4X4 pos;
	float hp;
	float mp;
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Client to Server packets
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
constexpr char C2S_P_LOGIN = 51;
constexpr char C2S_P_CREATEUSER = 52;
constexpr char C2S_P_ENTER_ROOM = 53;
constexpr char C2S_P_ROOM_UPDATE = 54;
constexpr char C2S_P_PICKCHARACTER = 55;
constexpr char C2S_P_GETREADY = 56;
constexpr char C2S_P_READYINGAME = 57;
constexpr char C2S_P_MOVE = 58;
constexpr char C2S_P_PLAYERATTACK = 59;
constexpr char C2S_P_MONSTER_ATTACK = 60;
constexpr char C2S_P_USE_ITEM = 61;
constexpr char C2S_P_USE_SKILL = 62;
constexpr char C2S_P_MASTERKEY = 63;

struct cs_packet_login {
	unsigned char size;
	char type;
	//char UserID[MAX_ID_LEN];
	//char Userpassword[MAX_ID_LEN];
};

struct cs_packet_createuser {
	unsigned char size;
	char type;
	char UserID[MAX_ID_LEN];
	char Userpassword[MAX_ID_LEN];
	char UserNickName[MAX_ID_LEN];
};

struct cs_packet_enter_room {
	unsigned char size;
	char type;
	char room_number;
};
struct cs_packet_room_refresh
{
	unsigned char size;
	char type;
};

struct cs_packet_pickcharacter {
	unsigned char size;
	char type;
	char room_number;
	short C_type;
};
struct cs_packet_getready {
	unsigned char size;
	char type;
	char room_number;
	bool isReady;
};

struct cs_packet_readytoIngame {
	unsigned char size;
	char type;
	short Map;
};

struct cs_packet_move {
	unsigned char size;
	char type;
	XMFLOAT4X4 pos;
	XMFLOAT3 BOGAN_POS;
	float time;
	UINT state;
};


struct cs_packet_player_attack {
	unsigned char size;
	char type;
	int target_monster_id;
	int attack_type; // Attacktype
};

struct cs_packet_monster_attack{					
	unsigned char size;
	char type;
	int attacker_id;  // 몬스터 ID
	int target_player_id;
	int attack_type; // Attacktype
};

struct cs_packet_item_use {
	unsigned char size;
	char type;
	char item_type;   // enum ItemType
};

struct cs_packet_skill_use {
	unsigned char size;
	char type;
	short job;
	char skillNumber;	// 0 ~ 2		0이 체력 회복, 1이 공격력 증가 + 방어력 감소, 2가 스킬게이지 최대치
};



struct cs_packet_next_stage_master_key {
	unsigned char size;
	char type;

};
#pragma pack(pop)
