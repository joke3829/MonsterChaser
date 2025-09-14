// Network.h
#pragma once


#include "stdafx.h"
#include "protocol.h"
#include "Room.h"
#include "Player.h"
#include "PlayerManager.h"
#include "Monster.h"

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 3500
#define BUF_SIZE 1024


enum IO_OP { IO_RECV, IO_SEND, IO_ACCEPT };

class EXP_OVER {
public:
    EXP_OVER(IO_OP op);
    WSAOVERLAPPED over;
    IO_OP io_op;
    WSABUF wsabuf[1];
     char buffer[BUF_SIZE];
    SOCKET accept_socket;
};




class SESSION {
public:
    SOCKET socket;
    std::unique_ptr<EXP_OVER> recv_over;
    unsigned char remained = 0;

    int m_uniqueNo;

    std::shared_ptr<Player> player;  //  게임 상태는 여기로

    SESSION(int Num, SOCKET s);
    ~SESSION();

    void do_recv();
    void do_send(void* buff);
    void process_packet(char* p);
    void BroadCasting_position(const int& size);
};

class Network {
public:
    Network();
    void Init();
    std::vector<Room> rooms;

    HANDLE iocp;
   // std::unordered_map<int, std::unique_ptr<SESSION>> users;
    //concurrent_unordered_map <int, std::unique_ptr<SESSION>> users;
    concurrent_unordered_map <int,shared_ptr <SESSION>> users;
    
    

    PlayerManager playerManager;  //  플레이어 상태 관리는 이걸로!


   SOCKET listen_socket;
};