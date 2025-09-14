
// MainServer.cpp
#include "stdafx.h"
#include "Room.h"
#include "Network.h"
#include <MSWSock.h>
#include "HeightMap.h"


mutex myMutex;
Network g_server;
std::atomic<int> g_client_id = 0;
std::atomic<int>  g_monster_id = 50000;

// =====================================================
std::unique_ptr<CHeightMapImage> g_pStage1Height{};
std::unique_ptr<CHeightMapImage> g_pStage1Collision{};
std::unique_ptr<CHeightMapImage> g_pStage2Height{};
std::unique_ptr<CHeightMapImage> g_pStage2Collision{};
std::unique_ptr<CHeightMapImage> g_pStage3Height{};
std::unique_ptr<CHeightMapImage> g_pStage3Collision{};
// =====================================================

void do_accept(Network& server) {
	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	EXP_OVER* accept_over = new EXP_OVER(IO_ACCEPT);
	accept_over->accept_socket = c_socket;

	AcceptEx(server.listen_socket, c_socket, accept_over->buffer, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		NULL, &accept_over->over);
}
void disconnect(int client_id) {
	auto it = g_server.users.find(client_id);
	if (it == g_server.users.end()) return;

	std::shared_ptr<SESSION> session = it->second;
	int room_num = session->player->room_num;
	std::lock_guard<std::mutex> lock(myMutex);
	

	
	// 방에서 제거
	if (room_num != -1)
	{
		std::lock_guard<std::mutex> lock(g_server.rooms[room_num].RoomMutex);
		auto& room_ids = g_server.rooms[room_num].id;
		room_ids.erase(std::remove(room_ids.begin(), room_ids.end(), client_id), room_ids.end());
		auto& room = g_server.rooms[room_num];
		room.SetReady_User(client_id, false); // 방에서 준비 상태 해제
		// 다른 클라이언트들에게 Leave 패킷 전송
		sc_packet_leave leave_pkt;
		leave_pkt.size = sizeof(leave_pkt);
		leave_pkt.type = S2C_P_LEAVE;
		leave_pkt.Local_id = session->player->local_id;

		for (int id : g_server.rooms[room_num].id) {
			g_server.users[id]->do_send(&leave_pkt);
		}
	}
	g_server.users.unsafe_erase(client_id);

	sc_packet_room_info rp;
	rp.size = sizeof(rp);
	rp.type = S2C_P_UPDATEROOM;
	for (int i = 0; i < g_server.rooms.size(); ++i)
		rp.room_info[i] = g_server.rooms[i].GetPlayerCount();

	for (auto& player : g_server.users)
		player.second->do_send(&rp);

	closesocket(session->socket);


	std::cout << "[클라이언트 " << client_id << "] 비정상 종료 처리됨." << std::endl;
}

void worker_thread(Network& server) {
	while (true) {
		DWORD bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over;

		BOOL success = GetQueuedCompletionStatus(server.iocp, &bytes, &key, &over, INFINITE);
		if (over == nullptr) continue; // 예외 상황 방어

		EXP_OVER* exp = reinterpret_cast<EXP_OVER*>(over);

		if (!success || (bytes == 0 && (exp->io_op == IO_RECV || exp->io_op == IO_SEND))) {
			disconnect(static_cast<int>(key));
			if (exp->io_op == IO_SEND) delete exp;
			continue;
		}

		switch (exp->io_op) {
		case IO_ACCEPT:
		{
			int client_id = g_client_id++;
			CreateIoCompletionPort(reinterpret_cast<HANDLE>(exp->accept_socket), server.iocp, client_id, 0);
			server.users[client_id] = std::make_shared<SESSION>(client_id, exp->accept_socket);

			sc_packet_enter ep;
			ep.size = sizeof(ep);
			ep.type = S2C_P_ENTER;

			for (auto& u : server.users)
				u.second->do_send(&ep);

			for (auto& u : server.users) {
				if (u.first != client_id) {
					sc_packet_enter ep2;
					ep2.size = sizeof(ep2);
					ep2.type = S2C_P_ENTER;
					server.users[client_id]->do_send(&ep2);
				}
			}

			std::cout << "[클라이언트 " << client_id << " ] 이 접속했습니다." << std::endl;
			do_accept(server);
			break;
		}
		case IO_RECV:
		{
			SESSION* user = server.users[key].get();
			char* ptr = exp->buffer;
			int processed = 0;
			bytes += user->remained;

			while (processed < bytes) {
				int size = static_cast<unsigned char>(ptr[0]);
				if (size > bytes - processed)
					break;
				user->process_packet(ptr);
				ptr += size;
				processed += size;
			}
			
			user->remained = bytes - processed;
			if (user->remained > 0) {
				memcpy(exp->buffer, ptr, user->remained);
			}

			user->do_recv();
			break;
		}
		case IO_SEND:
			delete exp;
			break;
		}
	}
}
int main() {

	g_server.Init();

	// 경로는 알아서 변경  
	g_pStage1Height = std::make_unique<CHeightMapImage>(L"src\\model\\Map\\ETP\\ETP_Terrain_Road.raw", 1024, 1024, XMFLOAT3(1.0f, 0.0156, 1.0f));
	g_pStage1Collision = std::make_unique<CHeightMapImage>(L"src\\model\\Map\\ETP\\ETP_CollisionMap.raw", 1024, 1024, XMFLOAT3(1.0f, 0.0156, 1.0f));

	g_pStage2Height = std::make_unique<CHeightMapImage>(L"src\\model\\Map\\Cave\\CaveHeightMap.raw", 512, 512, XMFLOAT3(500.0f / 512.0f, 0.0092f, 500.0f / 512.0f));
	g_pStage2Collision = std::make_unique<CHeightMapImage>(L"src\\model\\Map\\Cave\\CaveCollisionHMap.raw", 512, 512, XMFLOAT3(500.0f / 512.0f, 1.0f, 500.0f / 512.0f));

	g_pStage3Height = std::make_unique<CHeightMapImage>(L"src\\model\\Map\\WinterLand\\Terrain_Road.raw", 2049, 2049, XMFLOAT3(1.0f, 0.0312f, 1.0f));
	g_pStage3Collision = std::make_unique<CHeightMapImage>(L"src\\model\\Map\\WinterLand\\Terrain_Collision.raw", 2049, 2049, XMFLOAT3(1.0f, 0.0312f, 1.0f));

	do_accept(g_server);
	//netework에 있는 소켓으로 처리 
	std::vector<std::thread> workers;
	int thread_count = thread::hardware_concurrency() - 2;
	for (int i = 0; i < thread_count; ++i)
		workers.emplace_back(worker_thread, std::ref(g_server));

	for (auto& th : workers) th.join();
	return 0;
}


