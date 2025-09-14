#pragma once
#include "stdafx.h"
#include "protocol.h"
#include "GameObject.h"
//#include "ObjectManager.h"
#include "Scene.h"
#include <mutex>

// 07.25 ===========================================
extern std::array<bool, 3>	g_PlayerBuffState;
extern std::array<float, 3> g_maxHPs;
extern std::array<float, 3> g_maxMPs;
extern std::array<float, 3> g_SkillCost;

extern CParticle* g_pBuff0;
extern CParticle* g_pBuff1;
extern CParticle* g_pBuff2;

extern std::array<bool, 3> g_PlayerDie;
// =================================================

class C_Socket
{
public:
	C_Socket();
	~C_Socket();



	bool Init(const char* ip, int port);	//소켓 초기화												
	void do_recv();		//recv 받는곳																	
	void process_packet(char* ptr);	//패킷 처리														

	void send_packet(void* ptk);	//send하는곳 			
	//--------------------------------------------------------------//	
	void SendLogin();									//Send LoginPacket
	void SendCreateUser(const char* UserID, const char* Userpassword, const char* userNickName);	//Send CreateUserPacket
	void SendEnterRoom(const short RoomNum);														//Send EnterroomPacket
	void SendsetReady(const bool isReady, const int room_num);										//Send Readypakcet if isReday is true->player set ready
	void SendBroadCastRoom();																		
	void SendMovePacket(const float& Time, const UINT State);										//SendMovePacket	
	void SendPickCharacter(const short RoomNum, const short Job);
	void SendPlayerReady(const short Map);																			//Send Player Ready Packet
	void SendMonsterAttack(const int monster_id, const int target_id, const int Atktype);								//Send Monster Attack Packet
	void SendPlayerAttack(const int target_id, const int type);														//Send Player Attack Packet
	void SendUseItem(const unsigned int type);														//Send Item Packet
	
	void SendPriestBUFF(const char SkillNumber);	//Send Priest Buff Packet

	void SendMasterKey();


	//void SendNEXTSTAGEMASTERKEY();	//Send Next Stage Master Key Packet
	//--------------------------------------------------------------//									

	bool getstart() { return InGameStart; }
	void Setstart(const bool& st) { InGameStart = st; }

	void set_id(int id) { my_id = id; }
	int get_id() const { return my_id; }
	//------------------------------------------------------------//									
	std::mutex myMutex;
	SOCKET Getsocket() const { return m_socket; }
private:
	SOCKET m_socket;

	bool running{ true };
	bool InGameStart = false;

	unsigned char remained{ 0 };


	int my_id = -1;


};

