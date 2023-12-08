#include <Novice.h>
#include <fstream>
#include <math.h>
#include <mmsystem.h>
#include <process.h>
#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "winmm.lib")

DWORD WINAPI Threadfunc(void* px);
HWND hwMain;
const char kWindowTitle[] = "KAMATA ENGINEクライアント";

typedef struct {
	float x;
	float y;
} Vector2;

typedef struct {
	Vector2 center;
	float radius;
} Circle;

Circle a, b;
Vector2 center = {100, 100};
char keys[256] = {0};
char preKeys[256] = {0};
int color = RED;

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	WSADATA wdData;
	static HANDLE hThread;
	static DWORD dwID;

	// ライブラリの初期化
	Novice::Initialize(kWindowTitle, 1024, 768);

	hwMain = GetDesktopWindow();

	a.center.x = 200;
	a.center.y = 400;
	a.radius = 50;

	b.center.x = 800;
	b.center.y = 400;
	b.radius = 50;

	// winsock初期化
	WSAStartup(MAKEWORD(2, 0), &wdData);

	// データを送受信処理をスレッド（WinMainの流れに関係なく動作する処理の流れ）として生成。
	// データ送受信をスレッドにしないと何かデータを受信するまでRECV関数で止まってしまう。
	hThread = (HANDLE)CreateThread(NULL, 0, &Threadfunc, (LPVOID)&a, 0, &dwID);

	// ウィンドウの×ボタンが押されるまでループ
	while (Novice::ProcessMessage() == 0) {
		// フレームの開始
		Novice::BeginFrame();

		// キー入力を受け取る
		memcpy(preKeys, keys, 256);
		Novice::GetHitKeyStateAll(keys);

		if (keys[DIK_UP] != 0) {
			a.center.y -= 5;
		}
		if (keys[DIK_DOWN] != 0) {
			a.center.y += 5;
		}
		if (keys[DIK_RIGHT] != 0) {
			a.center.x += 5;
		}
		if (keys[DIK_LEFT] != 0) {
			a.center.x -= 5;
		}

		///
		/// ↓更新処理ここから
		///

		float distance = sqrtf(
		  (float)pow((double)a.center.x - (double)b.center.x, 2) +
		  (float)pow((double)a.center.y - (double)b.center.y, 2));

		if (distance <= a.radius + b.radius) {
			color = BLUE;
		} else
			color = RED;
		///
		/// ↑更新処理ここまで
		///

		///
		/// ↓描画処理ここから
		///
		Novice::DrawEllipse(
		  (int)a.center.x, (int)a.center.y, (int)a.radius, (int)a.radius, 0.0f, WHITE,
		  kFillModeSolid);
		Novice::DrawEllipse(
		  (int)b.center.x, (int)b.center.y, (int)b.radius, (int)b.radius, 0.0f, color,
		  kFillModeSolid);
		///
		/// ↑描画処理ここまで
		///

		// フレームの終了
		Novice::EndFrame();

		// ESCキーが押されたらループを抜ける
		if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
			break;
		}
	}

	// ライブラリの終了
	Novice::Finalize();

	// winsock終了
	WSACleanup();

	return 0;
}

// 通信スレッド関数
DWORD WINAPI Threadfunc(void* px) {

	SOCKET sConnect;
	struct sockaddr_in saConnect;
	char addr[20];

	WORD wPort = 8000;

	px = 0; // warning回避
	// ファイル読み込み
	std::ifstream ifs("ip.txt");
	ifs.getline(addr, sizeof(addr));
	sConnect = socket(PF_INET, SOCK_STREAM, 0);

	ZeroMemory(&saConnect, sizeof(sockaddr_in));
	saConnect.sin_family = AF_INET;
	saConnect.sin_addr.s_addr = inet_addr(addr);
	saConnect.sin_port = htons(wPort);

	// サーバーに接続
	if (connect(sConnect, (sockaddr*)(&saConnect), sizeof(saConnect)) == SOCKET_ERROR) {
		closesocket(sConnect);
		WSACleanup();

		return 1;
	}
	while (1) {
		// データ送信
		send(sConnect, (const char*)&a, sizeof(Circle), 0);

		// データ受信
		int nRcv = recv(sConnect, (char*)&b, sizeof(Circle), 0);

		if (nRcv == SOCKET_ERROR)
			break;
	}

	shutdown(sConnect, 2);
	closesocket(sConnect);

	return 0;
}