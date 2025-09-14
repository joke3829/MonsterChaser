
#include "C_Socket.h"

extern C_Socket Client;
extern std::unordered_map<int, Player> Players;
extern std::unordered_map<int, std::unique_ptr<Monster>> Monsters;
extern InGameState g_InGameState;
extern std::array<short, 10>	 userPerRoom;
extern std::vector<std::unique_ptr<CSkinningObject>>& skinned;
extern bool allready;
extern TitleState g_state;
extern std::unique_ptr<CMonsterChaserSoundManager> g_pSoundManager;

extern std::vector<std::unique_ptr<CPlayableCharacter>>	m_vMonsters;
extern std::vector<std::unique_ptr<CPlayableCharacter>>	m_vPlayers;



C_Socket::C_Socket() : InGameStart(false), running(true), remained(0), m_socket(INVALID_SOCKET) {}


C_Socket::~C_Socket()
{

	closesocket(m_socket);
}

bool C_Socket::Init(const char* ip, int port)
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &serverAddr.sin_addr);

	if (connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {

		closesocket(m_socket);
		WSACleanup();
		return false;
	}


	return true;
}


void C_Socket::send_packet(void* pkt)
{
	int ret = send(m_socket, reinterpret_cast<CHAR*>(pkt), reinterpret_cast<unsigned char*>(pkt)[0], 0);
	if (ret == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK) {
			std::cerr << "send 블로킹 발생!" << std::endl;
		}
	}
}

void C_Socket::SendLogin()
{
	cs_packet_login p;
	p.size = sizeof(p);
	p.type = C2S_P_LOGIN;
	
	Client.send_packet(&p);
}

void C_Socket::SendCreateUser(const char* UserID, const char* Userpassword, const char* userNickName)
{
}

void C_Socket::SendEnterRoom(const short RoomNum)
{
	cs_packet_enter_room p;
	p.size = sizeof(p);
	p.type = C2S_P_ENTER_ROOM;
	p.room_number = static_cast<char>(RoomNum);
	Client.send_packet(&p);
}

void C_Socket::SendPickCharacter(const short RoomNum, const short Job)
{
	cs_packet_pickcharacter p;
	p.size = sizeof(p);
	p.type = C2S_P_PICKCHARACTER;
	p.room_number = static_cast<char>(RoomNum);
	p.C_type = Job;
	Client.send_packet(&p);
}

void C_Socket::SendPlayerReady(const short Map)
{
	cs_packet_readytoIngame pkt;
	pkt.size = sizeof(pkt);
	pkt.type = C2S_P_READYINGAME;
	pkt.Map = Map;			//어떤 맵 준비완료됐는지
	Client.send_packet(&pkt);
}

void C_Socket::SendMonsterAttack(const int monster_id, const int target_id,const int Atktype)
{
	cs_packet_monster_attack pkt;
	pkt.size = sizeof(pkt);
	pkt.type = C2S_P_MONSTER_ATTACK;
	pkt.attacker_id = monster_id; // 공격하는 몬스터 ID
	pkt.target_player_id = target_id; // 공격 대상 플레이어 ID
	pkt.attack_type = Atktype; // 0: 일반 공격, 1: 스킬 공격
	Client.send_packet(&pkt);
}

void C_Socket::SendPlayerAttack(const int target_id,const int type)
{
	cs_packet_player_attack pkt;
	pkt.size = sizeof(pkt);
	pkt.type = C2S_P_PLAYERATTACK;
	pkt.target_monster_id = target_id; // 공격 대상 몬스터 ID		// 스킬 쓴거임 -1
	pkt.attack_type = type; // 0: 일반 공격, 1: 스킬 공격			// 1: 1번 스킬 2 번 3 번
	Client.send_packet(&pkt);
}

void C_Socket::SendUseItem(const unsigned int type)
{
	
	cs_packet_item_use pkt;
	pkt.size = sizeof(pkt);
	pkt.type = C2S_P_USE_ITEM;
	pkt.item_type = type;
	Client.send_packet(&pkt);
}

void C_Socket::SendPriestBUFF(const char SkillNumber)
{
	cs_packet_skill_use pkt;
	pkt.size = sizeof(pkt);
	pkt.type = C2S_P_USE_SKILL;
	pkt.skillNumber = SkillNumber; // 0이 체력 회복, 1이 공격력 증가 + 방어력 감소, 2가 스킬게이지 최대치
	Client.send_packet(&pkt);
}

void C_Socket::SendMasterKey()
{
	cs_packet_next_stage_master_key pkt;
	pkt.size = sizeof(pkt);
	pkt.type = C2S_P_MASTERKEY;
	Client.send_packet(&pkt);
}

void C_Socket::SendsetReady(const bool isReady, const int room_num)
{
	cs_packet_getready rp;
	rp.size = sizeof(rp);
	rp.type = C2S_P_GETREADY;
	rp.room_number = room_num;
	rp.isReady = isReady;
	Client.send_packet(&rp);
}

void C_Socket::SendBroadCastRoom()
{
	cs_packet_room_refresh rp;
	rp.size = sizeof(rp);
	rp.type = C2S_P_ROOM_UPDATE;
	Client.send_packet(&rp);
}

void C_Socket::SendMovePacket(const float& Time, const UINT State)
{
	cs_packet_move mp;
	mp.size = sizeof(mp);
	mp.type = C2S_P_MOVE;
	mp.pos = Players[Client.get_id()].getRenderingObject()->getWorldMatrix();
	mp.time = Time;
	mp.state = State;
	Client.send_packet(&mp);

}





void C_Socket::process_packet(char* ptr)
{
	char type = ptr[1];
	switch (type)
	{

	case S2C_P_ENTER:			//입장
	{
		sc_packet_enter* p = reinterpret_cast<sc_packet_enter*>(ptr);
		

		break;
	}
	case S2C_P_UPDATEROOM: {

		sc_packet_room_info* p = reinterpret_cast<sc_packet_room_info*>(ptr);
		short room_in_Players[10];
		memcpy(room_in_Players, p->room_info, sizeof(room_in_Players));
		//memcpy(userperroom, p->room_info, sizeof(userperroom));		이것만 해주면 끝

		memcpy(userPerRoom.data(), room_in_Players, sizeof(room_in_Players));



		break;
	}
	case S2C_P_SELECT_ROOM: {

		sc_packet_select_room* p = reinterpret_cast<sc_packet_select_room*>(ptr);

		int room_num = static_cast<int>(p->room_number);
		int local_id = p->Local_id;
		if (!Players.contains(local_id)) {
			Player newPlayer(local_id); // 명시적 생성자 사용
			Players.emplace(local_id, std::move(newPlayer));

			//  is_self가 true일 때만 내 로컬 ID 설정
			if (Client.get_id() == -1) {
				if (p->is_self) {
					Client.set_id(local_id);
					g_state = InRoom;
				}

			}

		}

		break;
	}
	case S2C_P_PICKCHARACTER:
	{
		sc_packet_pickcharacter* p = reinterpret_cast<sc_packet_pickcharacter*>(ptr);
		short CT = p->C_type;
		int local_id = p->Local_id;
		Players[local_id].setCharacterType(CT);

		Players[local_id].SetMaxHP(p->Max_HP);
		Players[local_id].SetHP(p->Max_HP);

		Players[local_id].SetMaxMP(p->Max_MP);
		Players[local_id].SetMP(p->Max_MP);

		// 07.25
		g_maxHPs[local_id] = p->Max_HP;
		g_maxMPs[local_id] = p->Max_MP;
		break;
	}
	case S2C_P_SETREADY:
	{

		sc_packet_set_ready* p = reinterpret_cast<sc_packet_set_ready*>(ptr);

		int id = p->Local_id;
		int room_num = static_cast<int>(p->room_number);//이미 방 선택할떄 room_num이 Players[id]안에 들어감
		bool ready = p->is_ready;

		Players[id].setReady(ready);
		//Players[p->id]->setReady(ready);
		//Players[p->id]->setRoomNumber(room_num);// 방 선택했을떄 해당 방 유저 수 나타내는 맴버 변수 
		//p->id = id;

		break;
	}
	case S2C_P_ALLREADY:
	{

		sc_packet_Ingame_start* p = reinterpret_cast<sc_packet_Ingame_start*>(ptr);
		Setstart(true);		//맴버 변수 InGameStart true로 바꿔주기
		//g_state = GoLoading;
		g_pSoundManager->AllStop();
		g_pSoundManager->StartFx(ESOUND::SOUND_START);
		break;
		//4 7 9
	}

	case S2C_P_MOVE: {
		sc_packet_move* p = reinterpret_cast<sc_packet_move*>(ptr);
		//로컬ID
		int local_id = p->Local_id;
		float time = p->time;
		unsigned int state = p->state;
		Players[local_id].SetMP(p->mp);
		if (local_id == Client.get_id()) {
			return;
		}
		XMFLOAT4X4 position = p->pos;

		

		Players[local_id].getRenderingObject()->SetWorldMatrix(position);
		Players[local_id].getAnimationManager()->ChangeAnimation(state, true);
		Players[local_id].getAnimationManager()->UpdateAnimation(time);		// 받은 시간과 현재 시간이 얼마 차이 안나면 바꾸지 않도록 추가하자


		break;
	}

	case S2C_P_MONSTER_SPAWN: {
		sc_packet_monster_spawn* pkt = reinterpret_cast<sc_packet_monster_spawn*>(ptr);

		int id = pkt->monster_id;
		MonsterType type = pkt->monster_type;

		auto& monster = Monsters[pkt->monster_id];
		// 이미 있으면 덮어쓰기 방지
		if (Monsters.find(id) != Monsters.end()) {
			Monsters[pkt->monster_id]->getRenderingObject()->SetWorldMatrix(pkt->pos);
			Monsters[pkt->monster_id]->getAnimationManager()->ChangeAnimation(2, false);
		}
		else {

	
		}
		break;
	}
	case S2C_P_MONSTER_ATTACK://몬스터가 공격 상태일 떄
	{
		sc_packet_monster_attack* pkt = reinterpret_cast<sc_packet_monster_attack*>(ptr);
		int attack_type = pkt->attack_type; // 공격 타입 (0: 1번 공격 모양, 1: 2번 공격모양, 2: 3번 공격모양)	char형태
		int monster_id = pkt->monster_id; // 몬스터 ID
	
		Monsters[monster_id]->setCurrentAttackType(attack_type); // 몬스터의 현재 공격 타입 설정 attack_type이 1이면 Skill1 , 2면 Skill2
		int a = Monsters[monster_id]->getCurrentAttackType();
		Monsters[monster_id]->getAnimationManager()->ChangeAnimation(a, true); // 몬스터 애니메이션 변경
		
		switch (a) {
		case 6:
			m_vMonsters[monster_id]->Skill1();
			break;
		case 7:
			m_vMonsters[monster_id]->Skill2();
			break;
		case 8:
			m_vMonsters[monster_id]->Skill3();
			break;
		}
		
		//Monsters[monster_id]->getAnimationManager()->
		//pkt->monster_id; // 몬스터 ID		//이걸로 공격 애니메이션 셋 

		break;
	}
	case S2C_P_MONSTER_DIE: {
		sc_packet_monster_die* pkt = reinterpret_cast<sc_packet_monster_die*>(ptr);
		int monster_id = pkt->monster_id;
		int gold = pkt->gold;
		if (Monsters.find(monster_id) != Monsters.end()) {
			auto& monster = Monsters[monster_id];
			monster->setHP(0); // 몬스터 HP를 0으로 설정
			monster->getAnimationManager()->ChangeAnimation(0, false);
		}
		break;
		
	}

	

	case S2C_P_MONSTER_RESPAWN: {
		sc_packet_monster_respawn* pkt = reinterpret_cast<sc_packet_monster_respawn*>(ptr);

		int id = pkt->monster_id;


		if (Monsters.find(id) != Monsters.end()) {
			auto& m = Monsters[id]; // Use auto& to correctly reference the unique_ptr
			m->getRenderingObject()->SetWorldMatrix(pkt->pos);
			m->getAnimationManager()->ChangeAnimation(2, false);

			//m->setVisible(true);														  // doyoung's turn
			//m->playIdleAnim();															  // doyoung's turn
		}
		else {
			// 존재하지 않는 경우 새로 생성  
		/*	auto newMonster = std::make_unique<Monster>(id);
			newMonster->setPosition(pkt->pos);
			newMonster->setVisible(true);
			newMonster->playIdleAnim();
			Monsters[id] = std::move(newMonster);*/
		}
		break;
	}

	case S2C_P_MONSTER_HIT:
	{
		sc_packet_monster_hit* pkt = reinterpret_cast<sc_packet_monster_hit*>(ptr);

		int id = pkt->monster_id; // 몬스터 ID
		if (Monsters.contains(id)) {
			Monsters[id]->setHP(pkt->hp); // 몬스터 HP 업데이트
		}
		break;

	}

	case S2C_P_MONSTER_MOVE: {
		sc_packet_monster_move* pkt = reinterpret_cast<sc_packet_monster_move*>(ptr);
		int id = pkt->monster_id;
		
		if (Monsters.contains(id)) {
			auto& monster = Monsters[id];
			auto* ap = dynamic_cast<CMonsterManager*>(monster->getAnimationManager());
			if (!ap->getSkillnum()) {
				monster->getRenderingObject()->SetWorldMatrix(pkt->pos);
				monster->getAnimationManager()->ChangeAnimation(4, false);
			}

			
			//monster->setVisible(true);
		}

		break;
	}
	case S2C_P_PLAYER_HIT:
	{		
		sc_packet_player_hit* pkt = reinterpret_cast<sc_packet_player_hit*>(ptr);
		Players[pkt->local_id].SetHP(pkt->hp); // 플레이어가 데미지를 받았을 때 HP 감소 처리
		break;
	}
	case S2C_P_NEXTSTAGE:
	{
		g_InGameState = IS_FINISH; // 게임 상태를 완료로 변경
		break;
	}
	case S2C_P_CHANGEHP:
	{
		auto* pkt = reinterpret_cast<sc_packet_change_hp*>(ptr);
		int local_id = static_cast<int>(pkt->local_id);
		if (local_id == Client.get_id() && Players[local_id].GetHP() < pkt->hp) {
			g_pSoundManager->StartFx(ESOUND::SOUND_HEALING);
			g_pBuff2->Start();
		}
		Players[local_id].SetHP(pkt->hp); // 플레이어의 HP 변경 처리
		break;
	}
	case S2C_P_CHANGEMP:
	{
		auto* pkt = reinterpret_cast<sc_packet_change_mp*>(ptr);
		int local_id = static_cast<int>(pkt->local_id);
		Players[local_id].SetMP(pkt->mp); // 플레이어의 HP 변경 처리
		break;
	}
	case S2C_P_BOSS_ROAR:
	{
		auto* pkt = reinterpret_cast<sc_packet_boss_roar*>(ptr);
		int boss_id = pkt->monster_id;

		if (Monsters.count(boss_id)) {
			Monsters[boss_id]->getAnimationManager()->ChangeAnimation(3, true);
			g_pSoundManager->StartFx(ESOUND::SOUND_ROAR);
			//Monsters[boss_id]->playRoarAnimation();  //  울부짖는 애니메이션 재생 함수
		}
		break;
	}
	case S2C_P_BUFFCHANGE:
	{
		auto* pkt = reinterpret_cast<sc_packet_buff_change*>(ptr);
		char buffType = pkt->bufftype;
		char state = pkt->state;

		switch (buffType)
		{
		case 0: // 공격력 증가
			if(state == 1) {
				g_PlayerBuffState[0] = true;
				g_pBuff0->Start();
				//공격력 버프 켜짐
			} else {
				g_PlayerBuffState[0] = false;
				//공격력 버프 꺼짐
			}
			break;
		case 1: // 방어력 증가
			if (state == 1) {
				g_PlayerBuffState[1] = true;
				g_pBuff1->Start();
				//방어력 상승 버프 켜짐
			}
			else {
				g_PlayerBuffState[1] = false;
				//방어력 상승 버프 꺼짐
			}
			break;
		case 2: // 방어력 감소
			if (state == 1) {
				g_PlayerBuffState[2] = true;
				//방어력 감소 버프 켜짐
			}
			else {
				g_PlayerBuffState[2] = false;
				//방어력 감소 버프 꺼짐
			}
			break;
		default:
			break;
		}

		//std::cout << "[버프 변경] 타입: " << (int)buffType << ", 상태: " << (int)state << std::endl;
		break;
	}
	case S2C_P_LEAVE:
	{
		sc_packet_leave* pkt = reinterpret_cast<sc_packet_leave*>(ptr);

		int local_id = pkt->Local_id;

		if (Players.find(local_id) != Players.end()) {
			Players.erase(local_id);
			if (local_id == Client.get_id()) {
				Client.set_id(-1); // 클라이언트 ID 초기화
				g_state = Title; // 타이틀 상태로 변경
			}
		}
		break;
	}
	case S2C_P_MONSTERIDLE:
	{
		sc_packet_monster_idle* pkt = reinterpret_cast<sc_packet_monster_idle*>(ptr);
		int monster_id = pkt->monster_id;
		if (Monsters.find(monster_id) != Monsters.end()) {
			auto& monster = Monsters[monster_id];
			monster->getRenderingObject()->SetWorldMatrix(pkt->pos);
			monster->getAnimationManager()->ChangeAnimation(2, false); // 몬스터 애니메이션을 Idle 상태로 변경
		}
		break;
	}
	case S2C_P_PlAYER_DIE:
	{
		// 플레이어가 죽었을 때 처리
		sc_packet_player_die* pkt = reinterpret_cast<sc_packet_player_die*>(ptr);
		g_PlayerDie[pkt->Local_id] = true;
		auto p = reinterpret_cast<CPlayableCharacterAnimationManager*>(Players[pkt->Local_id].getAnimationManager());
		p->setDie(true);
		p->ChangeDie();
		g_pSoundManager->StartFx(ESOUND::SOUND_PLAYER_DEAD);
		break;
	}
	case S2C_P_PlAYER_RESPAWN:
	{
		// 플레이어가 부활했을 때 처리
		sc_packet_respawn* pkt = reinterpret_cast<sc_packet_respawn*>(ptr);
		g_PlayerDie[pkt->Local_id] = false;
		auto p = reinterpret_cast<CPlayableCharacterAnimationManager*>(Players[pkt->Local_id].getAnimationManager());
		p->setDie(false);
		p->ChangeAlive();
		Players[pkt->Local_id].SetHP(pkt->hp);
		Players[pkt->Local_id].SetMP(pkt->mp);
		m_vPlayers[pkt->Local_id]->InitComboState();
		break;
	}
	
	default:
		break;
	}

}

void C_Socket::do_recv()
{
	//std::lock_guard<std::mutex>lock(myMutex);

	char buffer[BUF_SIZE] = {};
	while (running) {
		int processed = 0;

		int io_byte = recv(m_socket, buffer + remained, BUF_SIZE - remained, 0);      //io_byte


		if (io_byte == 0) {
			MessageBoxA(nullptr, "IO_BYTE 크기가 0입니다.", "수신 에러", MB_ICONERROR);

			break;
		}
		if (io_byte == SOCKET_ERROR) {
			int err = WSAGetLastError();
			MessageBoxW(NULL, L"서버와의 연결이 끊어졌습니다.", L"연결 종료", MB_OK | MB_ICONERROR);
			PostQuitMessage(0);  // 윈도우 루프 종료
			return;

		}
		char* ptr = buffer;
		io_byte += remained;
		while (processed < io_byte) {
			int size = ptr[0];
			if (size > io_byte - processed)
				break;


			process_packet(ptr);

			ptr += size;
			processed += size;
		}
		remained = io_byte - processed;

		if (remained > 0) {
			memcpy(buffer, ptr, remained);
		}
	}
}




