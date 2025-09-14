#include "stdafx.h"
#include "Network.h"


extern Network g_server;
extern mutex myMutex;

EXP_OVER::EXP_OVER(IO_OP op) : io_op(op) {
	ZeroMemory(&over, sizeof(over));
	wsabuf[0].buf = buffer;
	wsabuf[0].len = BUF_SIZE;
	accept_socket = INVALID_SOCKET;
}

SESSION::SESSION(int Num, SOCKET s) : m_uniqueNo(Num), socket(s) {
	recv_over = std::make_unique<EXP_OVER>(IO_RECV);
	player = std::make_shared<Player>(-1, "", -1); // 기본 생성
	g_server.playerManager.AddPlayer(m_uniqueNo, player);
	do_recv();
}

SESSION::~SESSION() {
	g_server.playerManager.RemovePlayer(m_uniqueNo);
	closesocket(socket);
}

void SESSION::do_recv() {
	DWORD flags = 0;
	recv_over->wsabuf[0].buf = recv_over->buffer + remained;
	recv_over->wsabuf[0].len = BUF_SIZE - remained;
	WSARecv(socket, recv_over->wsabuf, 1, nullptr, &flags, &recv_over->over, nullptr);
}

void SESSION::do_send(void* packet) {
	EXP_OVER* over = new EXP_OVER(IO_SEND);
	int len = reinterpret_cast<unsigned char*>(packet)[0];
	memcpy(over->buffer, packet, len);
	over->wsabuf[0].len = len;
	WSASend(socket, over->wsabuf, 1, nullptr, 0, &over->over, nullptr);
}

void SESSION::process_packet(char* p) {
	char type = p[1];
	switch (type) {
	case C2S_P_LOGIN: {

		sc_packet_room_info rp;
		rp.size = sizeof(rp);
		rp.type = S2C_P_UPDATEROOM;
		for (int i = 0; i < g_server.rooms.size(); ++i)
			rp.room_info[i] = g_server.rooms[i].GetPlayerCount();

		for (auto& player : g_server.users)
			player.second->do_send(&rp);

		sc_packet_enter ep;
		ep.size = sizeof(ep);
		ep.type = S2C_P_ENTER;
		g_server.users[m_uniqueNo]->do_send(&ep);//당사자한테만 보내주기
		break;
	}
	case C2S_P_ENTER_ROOM: {
		cs_packet_enter_room* pkt = reinterpret_cast<cs_packet_enter_room*>(p);
		int room_Num = static_cast<int>(pkt->room_number);
		lock_guard<mutex> lock(g_server.rooms[room_Num].RoomMutex);

		Room& room = g_server.rooms[room_Num];
		if (room.InGameStart) {
			break;
			//이미 게임이 시작된 방에 들어오려고 하면 안됨
		}


		if (!g_server.rooms[room_Num].IsAddPlayer()) break;

		player->local_id = g_server.rooms[room_Num].GetPlayerCount();
		player->room_num = room_Num;
		g_server.rooms[room_Num].AddPlayer(m_uniqueNo);

		for (auto& id : g_server.rooms[room_Num].id) {					//기존유저한테 신규유저 들어왔다고 알림
			sc_packet_select_room sp;
			sp.size = sizeof(sp);
			sp.type = S2C_P_SELECT_ROOM;
			sp.Local_id = player->local_id;
			sp.room_number = static_cast<char>(room_Num);
			sp.is_self = (id == m_uniqueNo);
			g_server.users[id]->do_send(&sp);
		}

		for (int existing_id : g_server.rooms[room_Num].id) {			//신규 유저에게 기존유저들의 존재 알림
			if (existing_id == m_uniqueNo) continue;
			sc_packet_select_room sp_existing;
			sp_existing.size = sizeof(sp_existing);
			sp_existing.type = S2C_P_SELECT_ROOM;
			sp_existing.Local_id = g_server.users[existing_id]->player->local_id;
			sp_existing.room_number = static_cast<char>(room_Num);
			sp_existing.is_self = false;
			g_server.users[m_uniqueNo]->do_send(&sp_existing);
		}

		for (const auto& [existing_id, char_type] : g_server.rooms[room_Num].selected_characters) {		//신규유저가 들어왔는데 이미 기존유저가 캐릭터 선택했을수도 있으니 캐릭터 타입 알려주기
			sc_packet_pickcharacter cp;
			cp.size = sizeof(cp);
			cp.type = S2C_P_PICKCHARACTER;
			cp.C_type = char_type;
			cp.Local_id = g_server.users[existing_id]->player->local_id;
			g_server.users[m_uniqueNo]->do_send(&cp);
		}

		sc_packet_room_info rp;
		rp.size = sizeof(rp);
		rp.type = S2C_P_UPDATEROOM;
		for (int i = 0; i < g_server.rooms.size(); ++i)
			rp.room_info[i] = g_server.rooms[i].GetPlayerCount();

		for (auto& player : g_server.users)
			player.second->do_send(&rp);

		std::cout << "[로컬아이디 " << player->local_id << "을 가진 클라이언트 " << m_uniqueNo
			<< "]이 " << (int)room_Num << "번 방에 입장했습니다." << std::endl;
		break;
	}
	case C2S_P_PICKCHARACTER: {
		cs_packet_pickcharacter* pkt = reinterpret_cast<cs_packet_pickcharacter*>(p);
		Character Character_type = static_cast<Character>(pkt->C_type);
		int room_num = static_cast<int>(pkt->room_number);

		g_server.rooms[room_num].selected_characters[m_uniqueNo] = Character_type;
		//player->type = static_cast<Character> (Character_type); // 플레이어의 직업 설정
		player->Updatestatus(Character_type); // 플레이어 상태 업데이트
		sc_packet_pickcharacter cp;
		cp.size = sizeof(cp);
		cp.type = S2C_P_PICKCHARACTER;
		cp.Local_id = player->local_id;
		cp.Max_HP = player->GetHP();
		cp.Max_MP = player->GetMP();
		cp.C_type = static_cast<short>(Character_type);

		for (auto& id : g_server.rooms[room_num].id)
			g_server.users[id]->do_send(&cp);
		break;
	}
	case C2S_P_GETREADY: {
		cs_packet_getready* pkt = reinterpret_cast<cs_packet_getready*>(p);
		int room_num = player->room_num;
		player->isReady = pkt->isReady;

		if (player->isReady)
			g_server.rooms[room_num].SetReady_User(player->local_id, true);
		else
			g_server.rooms[room_num].SetReady_User(player->local_id, false);

		sc_packet_set_ready rp;
		rp.size = sizeof(rp);
		rp.type = S2C_P_SETREADY;
		rp.Local_id = player->local_id;
		rp.room_number = static_cast<char>(room_num);
		rp.is_ready = player->isReady;

		for (int id : g_server.rooms[room_num].id)
			if (id != m_uniqueNo)
				g_server.users[id]->do_send(&rp);

		if (g_server.rooms[room_num].IsAllReady()) {
			g_server.rooms[room_num].InGameStart = true;
			sc_packet_Ingame_start sp;
			sp.size = sizeof(sp);
			sp.type = S2C_P_ALLREADY;
			sp.room_number = static_cast<char>(room_num);

			for (int id : g_server.rooms[room_num].id)
				g_server.users[id]->do_send(&sp);
		}
		break;
	}

	case C2S_P_READYINGAME: {
		auto* pkt = reinterpret_cast<cs_packet_readytoIngame*>(p);
		int room_num = player->room_num; // 이미 player에 room_num이 설정되어 있음
		int local_id = player->local_id; // 로컬 ID
		short stage = pkt->Map; // 맵 정보

		Room& room = g_server.rooms[room_num];
		room.setStage(stage);
		room.setReady(local_id, true);  // ✅ 이 로컬 ID를 true로 표시



		if (room.isAllGameStartReady()) {
			room.bStageActive = true; // 게임 시작 준비 완료

			room.monsters.clear(); // 몬스터 초기화
			room.SpawnMonsters(); // 몬스터 스폰

			room.StartGame();  // 몬스터 스레드 시작

			room.InitailizeReadyingame();		//다시	player_readytoPlaygame 다 False로 초기화

			std::cout << "[서버] room " << room_num << " → 스테이지 " << pkt->Map << " 시작" << std::endl;
		}

		std::cout << "[Ingame Ready] room: " << room_num << ", local_id: " << local_id << "\n";

		break;
	}
	case C2S_P_ROOM_UPDATE: {
		sc_packet_room_info pkt;
		pkt.size = sizeof(pkt);
		pkt.type = S2C_P_UPDATEROOM;
		for (int i = 0; i < g_server.rooms.size(); ++i)
			pkt.room_info[i] = g_server.rooms[i].GetPlayerCount();

		for (auto& player : g_server.users)
			player.second->do_send(&pkt);
		break;
	}
	case C2S_P_MOVE: {
		cs_packet_move* pkt = reinterpret_cast<cs_packet_move*>(p);

		g_server.playerManager.SetPosition(m_uniqueNo, pkt->pos);
		player->setBoanPosition(pkt->BOGAN_POS); // 플레이어의 보안 위치 설정	
		//pkt->BOGAN_POS;

		sc_packet_move mp;
		mp.size = sizeof(mp);
		mp.type = S2C_P_MOVE;
		mp.Local_id = player->local_id;
		mp.pos = pkt->pos;
		mp.time = pkt->time;
		mp.state = pkt->state;
		mp.hp = player->GetHP();
		mp.mp = player->GetMP();
		auto duration = 3; // 간단 예시 (ping 시간 계산은 생략)
		mp.pingTime = static_cast<UINT>(duration);

		for (int id : g_server.rooms[player->room_num].id)
			g_server.users[id]->do_send(&mp);
		break;
	}

	case C2S_P_PLAYERATTACK: {
		cs_packet_player_attack* pkt = reinterpret_cast<cs_packet_player_attack*>(p);
		int monster_id = pkt->target_monster_id;
		int AttackType = pkt->attack_type;

		Room& room = g_server.rooms[player->room_num];
		if (monster_id == -1) {
			player->PlaySkill(AttackType); // 플레이어가 공격 애니메이션 재생
			sc_packet_change_mp mp;
			mp.size = sizeof(mp);
			mp.type = S2C_P_CHANGEMP;
			mp.local_id = player->local_id;
			mp.mp = player->GetMP();
			for (int id : room.id) {
				g_server.users[id]->do_send(&mp);
			}
			break;
		}

		auto it = room.monsters.find(monster_id);
		if (it == room.monsters.end()) break;
		auto& monster = it->second;
		bool isDead = monster->TakeDamage(player->GetDamage(AttackType)); // 나중에 10은 플레이어 직업 공격력으로 체크 //gGetDamage수정해야됨
		if (monster->GetHP() > 0) {
			cout << "[몬스터 공격] 몬스터 ID: " << monster_id
				<< ", 공격력: " << player->GetATK()
				<< ", 남은 HP: " << monster->GetHP() << std::endl;

			// 모두에게 히트 패킷 전송
			sc_packet_monster_hit hit;
			hit.size = sizeof(hit);
			hit.type = S2C_P_MONSTER_HIT;
			hit.monster_id = monster_id;
			hit.hp = monster->GetHP(); // 새로 만들면 좋음
			for (int pid : room.id)
				g_server.users[pid]->do_send(&hit);
		}

		if (isDead) {
			sc_packet_monster_die die{};
			die.size = sizeof(die);
			die.type = S2C_P_MONSTER_DIE;
			die.monster_id = monster_id;
			die.gold = monster->GetGold();

			for (int pid : room.id)
				g_server.users[pid]->do_send(&die);

			//  보스몬스터일 경우 다음 스테이지 전환
			if (monster->isBossMonster())
			{
				sc_packet_NextStage sp;
				sp.size = sizeof(sp);
				sp.type = S2C_P_NEXTSTAGE;

				for (int pid : room.id)
					g_server.users[pid]->do_send(&sp);

				if (monster->GetType() == MonsterType::Gorhorrid)
					room.ResetGame();
				room.StopGame();

				sc_packet_change_hp hp;
				hp.size = sizeof(hp);
				hp.type = S2C_P_CHANGEHP;
				hp.local_id = player->local_id;
				hp.hp = player->GetMaxHP();
				for (int pid : room.id)
					g_server.users[pid]->do_send(&hp);

				sc_packet_change_mp mp;
				mp.size = sizeof(mp);
				mp.type = S2C_P_CHANGEMP;
				mp.local_id = player->local_id;
				mp.mp = 100;
				for (int pid : room.id)
					g_server.users[pid]->do_send(&mp);





				std::cout << "[보스 사망 → 다음 스테이지로 이동]\n";
			}
		}

		break;
	}

	case C2S_P_MONSTER_ATTACK: {

		auto* pkt = reinterpret_cast<cs_packet_monster_attack*>(p);

		Room& room = g_server.rooms[player->room_num];

		auto monster = room.monsters[pkt->attacker_id];
		if (pkt->target_player_id >= room.id.size()) {
			return;
		}
		auto targetUniqId = room.id[pkt->target_player_id];
		auto target = g_server.playerManager.GetPlayer(targetUniqId);

		//  몬스터 공격 타입 적용
		int attackType = pkt->attack_type;


		//target->GetDamage(pkt->attack_power); // HP 감소 적용


		if (monster && target) {
			target->SetLastHitTime();  //  피격 시각 기록
			bool dead = target->TakeDamage(monster->GetATK()); // 예시: 10 데미지

			// 클라에 피격 정보 전송
			sc_packet_player_hit hpkt;
			hpkt.size = sizeof(hpkt);
			hpkt.type = S2C_P_PLAYER_HIT;
			hpkt.local_id = pkt->target_player_id;
			hpkt.hp = target->GetHP();


			for (int pid : room.id)
				g_server.users[pid]->do_send(&hpkt);

			if (dead) {

				sc_packet_player_die dpkt;
				dpkt.size = sizeof(dpkt);
				dpkt.type = S2C_P_PlAYER_DIE;
				dpkt.Local_id = pkt->target_player_id;
				for (int pid : room.id)
					g_server.users[pid]->do_send(&dpkt);
				// 죽었을 경우 처리 추가 가능
			}
		}
		break;
	}

	case C2S_P_USE_ITEM: {
		auto* pkt = reinterpret_cast<cs_packet_item_use*>(p);
		int room_num = player->room_num;
		ItemType type = static_cast<ItemType>(pkt->item_type);
		Room& room = g_server.rooms[player->room_num];
		switch (type)
		{
		case ItemType::HP_POTION:
		{
			player->PlusHP(480); // 예시로 50만큼 HP 회복
			sc_packet_change_hp ap;
			ap.size = sizeof(ap);
			ap.type = S2C_P_CHANGEHP;
			ap.local_id = player->local_id;
			ap.hp = player->GetHP();


			for (int id : room.id)
				g_server.users[id]->do_send(&ap);


			break;
		}
		case ItemType::MP_POTION:
		{
			player->PlusMP(50); // 예시로 5만큼 MP 회복
			sc_packet_change_mp mp;
			mp.size = sizeof(mp);
			mp.type = S2C_P_CHANGEMP;
			mp.local_id = player->local_id;
			mp.mp = player->GetMP();
			//  본인에게만 전송
			g_server.users[m_uniqueNo]->do_send(&mp);
			break;
		}
		case ItemType::ATK_BUFF:
		{
			float buff_amount = 100.f;
			float duration = 50.f;

			player->AddATKBuff_Potion(buff_amount, duration); //  정확한 함수 사용
			//player->UpdateBuffStatesIfChanged(false); // 버프 상태 업데이트

			break;
		}
		case ItemType::DEF_BUFF:
		{
			float buff_amount = 50.f;
			float duration = 60.f;

			player->AddDEFBuff(buff_amount, duration);
			//player->UpdateBuffStatesIfChanged(false); // 버프 상태 업데이트
			break;
		}
		default:
			break;
		}

		break;
	}

	case C2S_P_USE_SKILL: {
		auto* pkt = reinterpret_cast<cs_packet_skill_use*>(p);
		int skillNum = static_cast<int>(pkt->skillNumber);		// 1: 체력 회복 2: 공격력 증가 + 방어력 감소 3: 스킬게이지 최대치 

		Room& room = g_server.rooms[player->room_num];

		if (skillNum == 0 )
			player->PlusMP(-30.0f); // 예: 30 소모. 필요시 skillNum별 조절 가능
		else if(skillNum == 1)
			player->PlusMP(-40.0f); // 예: 30 소모. 필요시 skillNum별 조절 가능
		else if (skillNum == 2)
			player->PlusMP(-60.0f); // 예: 30 소모. 필요시 skillNum별 조절 가능
		for (int pid : room.id) {

			auto target = g_server.playerManager.GetPlayer(pid);
			if (!target) continue; // 대상 플레이어가 없으면 건너뜀
			switch (skillNum) {
			case 0: { // 체력 회복
			
				
				target->PlusHP(200); // 임시값. 나중에 조정

				sc_packet_change_hp pkt_hp;
				pkt_hp.size = sizeof(pkt_hp);
				pkt_hp.type = S2C_P_CHANGEHP;
				pkt_hp.local_id = target->local_id;
				pkt_hp.hp = target->GetHP();

				for (int id : room.id)
					g_server.users[id]->do_send(&pkt_hp);
				break;
			}
			case 1: // 공격력 증가 + 방어력 감소
			{
				
				player->AddATKBuff_Skill(150.0f, 30.0f); //  정확한 함수 사용
				target->AddDEFDEBuff(30.f, 10.0f); // -10 def, 10초
				target->UpdateBuffStatesIfChanged(true);   //  전체에게 전송
				// 이건 클라에 보여줄 필요 없으면 패킷 생략 가능
				break;
			}
			case 2: { // MP 최대치
				target->SetMP(100.0f); // max_skill_cost 기준

				sc_packet_change_mp pkt_mp;
				pkt_mp.size = sizeof(pkt_mp);
				pkt_mp.type = S2C_P_CHANGEMP;
				pkt_mp.local_id = target->local_id;
				pkt_mp.mp = target->GetMP();

				for (int id : room.id)
					g_server.users[id]->do_send(&pkt_mp);
				break;
			}
			default:
				break;
			}
		}
		break;
	}
	case C2S_P_MASTERKEY:
	{
		Room& room = g_server.rooms[player->room_num];
		// 스레드 멈춤
		sc_packet_NextStage sp;
		sp.size = sizeof(sp);
		sp.type = S2C_P_NEXTSTAGE;
		for (int pid : room.id) {

			auto target = g_server.playerManager.GetPlayer(pid);
			target->SetMP(100.0f); // 모든 플레이어 HP 회복
		}
		for (int pid : room.id)
			g_server.users[pid]->do_send(&sp);
		auto monster = room.monsters[0];
		if (monster->GetType() == MonsterType::Gorhorrid)
		{
			room.ResetGame();


		}

		room.StopGame();		//스레드 멈추는거



		sc_packet_change_hp ap;
		ap.size = sizeof(ap);
		ap.type = S2C_P_CHANGEHP;
		ap.local_id = player->local_id;
		ap.hp = player->GetMaxHP();
		for (int pid : room.id)
			g_server.users[pid]->do_send(&sp);
		break; // 마스터 키 패킷 처리
	}
	}
}

void SESSION::BroadCasting_position(const int& size) {
	for (int i = 0; i < size; ++i) {
		int num = g_server.rooms[player->room_num].id[i];
		auto& pos = g_server.users[num]->player->GetPosition();
		std::cout << "=== [객체" << num << " position] === x: "
			<< pos._41 << " y: " << pos._42 << " z: " << pos._43 << std::endl;
	}
}

Network::Network() : rooms(10) {}

void Network::Init() {
	WSAData wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	listen_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(listen_socket, (sockaddr*)&addr, sizeof(addr));
	listen(listen_socket, SOMAXCONN);

	iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(listen_socket), iocp, 0, 0);

	std::cout << "[서버 초기화 완료]" << std::endl;
}
