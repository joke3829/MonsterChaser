// Client.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "Client.h"
#include "GameFramework.h"
#include "C_Socket.h"
#include "ObjectManager.h"


#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.
CGameFramework gGameFramework;
std::thread recvThread;

std::unordered_map<int, Player> Players;               // 모든 플레이어들		

std::unordered_map<int, std::unique_ptr<Monster>> Monsters;           // 몬스터들


std::array<short, 10>	 userPerRoom{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };			// 방 UI대신 쓸거 
TitleState g_state = Title;
InGameState g_InGameState;

// 07.25 ======================================
std::array<bool, 3>	g_PlayerBuffState{};
std::array<float, 3> g_maxHPs;
std::array<float, 3> g_maxMPs;
std::array<float, 3> g_SkillCoolTime{};
std::array<float, 3> g_SkillCurCTime{};
std::array<float, 3> g_SkillCost{};
std::array<bool, 3> g_PlayerDie{};
// ============================================

C_Socket Client;

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// 여기는 방 UI들어오면 삭제할 부분



void SetCursorPosition(int x, int y) {
	COORD coord = { static_cast<SHORT>(x), static_cast<SHORT>(y) };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}


// 여기는 방 UI들어오면 삭제할 부분
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONIN$", "r", stdin);
	std::string ip;
	std::cout << "서버 IP 주소를 입력하세요: ";
	std::cin >> ip;


	//// TODO: 여기에 코드를 입력합니다.

	if (!Client.Init(ip.c_str(), PORT_NUM))            //Change IP Address (DY - 220.120.240.160)
	//if (!Client.Init("127.0.0.1", PORT_NUM))
	{
		MessageBoxA(nullptr, "서버 연결 실패. 클라이언트를 종료합니다.", "연결 실패", MB_ICONERROR);
		return 0;  // 창 생성 없이 종료
	}

	//  콘솔 종료
	//FreeConsole();
	//std::thread recvThread(&C_Socket::do_recv, &Client);
	//recvThread.join();
	//	std::thread drawThread(RoomListThread);



		//준비 완료되기 전까지 대기
	/*   while (!Client.get_ready_to_start()) {
		   std::this_thread::sleep_for(std::chrono::milliseconds(100));
	   }*/








	   // 전역 문자열을 초기화합니다.
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_CLIENT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 애플리케이션 초기화를 수행합니다:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENT));


	//MSG msg;

	// 기본 메시지 루프입니다:
	/*while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}*/
	recvThread = std::thread(&C_Socket::do_recv, &Client);
	for (MSG msg;;) {
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				return 0;
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		gGameFramework.Render();
	}
	if (recvThread.joinable()) {
		recvThread.join();
	}
	//drawThread.join();
	//return (int) msg.wParam;
}



//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENT));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

	DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_BORDER;
	RECT rt = { 0, 0, DEFINED_UAV_BUFFER_WIDTH, DEFINED_UAV_BUFFER_HEIGHT };
	// rt는 추후에 화면 크기 조정을 추가하면 바뀔 수 있다.

	AdjustWindowRect(&rt, dwStyle, FALSE);

	HWND hWnd = CreateWindow(szWindowClass, szTitle, dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, rt.right - rt.left, rt.bottom - rt.top, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}
	if (!gGameFramework.OnInit(hWnd, hInstance))
		exit(0);


	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
	case WM_MOUSEWHEEL:
	case WM_KEYDOWN:
	case WM_KEYUP:
		gGameFramework.WMMessageProcessing(hWnd, message, wParam, lParam);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

