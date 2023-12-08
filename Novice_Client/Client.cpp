#include "Sprite.h"
#include "TextureManager.h"
#include "DirectXCommon.h"
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
} Vec2;

typedef struct {
	Vec2 center;
	float radius;
} Circle;

Circle a, b;
Circle bulletA, bulletB;
bool isShot = false;

Vec2 center = {100, 100};
char keys[256] = {0};
char preKeys[256] = {0};
int colorA = RED;
int colorB = BLUE;

int lifeA = 3;
int lifeB = 3;

enum class Scene {

	S_Title,
	S_Game
};
Scene scene = Scene::S_Title;

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	WSADATA wdData;
	static HANDLE hThread;
	static DWORD dwID;

	// ライブラリの初期化
	Novice::Initialize(kWindowTitle, 1024, 768);

	hwMain = GetDesktopWindow();

	a.center.x = 200.0f;
	a.center.y = 400.0f;
	a.radius = 50.0f;

	b.center.x = 800.0f;
	b.center.y = 400.0f;
	b.radius = 50.0f;

	uint32_t title = 1;
	title = TextureManager::Load("title.png");

	Sprite* sTitle = nullptr;
	sTitle = Sprite::Create(title, {100.0f, 100.0f});

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

		switch (scene) {
		case Scene::S_Title:
			if (keys[DIK_SPACE] != 0) {
				scene = Scene::S_Game;
			}

			break;
		case Scene::S_Game:
			if (keys[DIK_UP] != 0 || keys[DIK_W] != 0) {
				a.center.y -= 5.0f;
			} else if (keys[DIK_DOWN] != 0 || keys[DIK_S] != 0) {
				a.center.y += 5.0f;
			} else if (keys[DIK_RIGHT] != 0 || keys[DIK_D] != 0) {
				a.center.x += 5.0f;
			} else if (keys[DIK_LEFT] != 0 || keys[DIK_A] != 0) {
				a.center.x -= 5.0f;
			}

			if (keys[DIK_SPACE] != 0 && preKeys[DIK_SPACE] == 0) {
				isShot = true;
			}

			if (isShot) {
				if (bulletA.center.x >= 1000.0f) {
					isShot = false;
				}
			}
			break;
		default:
			break;
		}

		///
		/// ↓更新処理ここから
		///
		// 弾の座標を同期

		if (!isShot) {
			bulletA.center.x = a.center.x + 100.0f;
			bulletA.center.y = a.center.y;
			bulletA.radius = 20.0f;

		} else {
			const float speed = 10.0f;
			bulletA.center.x += speed;
		}
		bulletB.center.x = b.center.x - 100.0f;
		bulletB.center.y = b.center.y;
		bulletB.radius = 20.0f;

		float distanceA = sqrtf(
		  (float)pow((double)a.center.x - (double)bulletB.center.x, 2) +
		  (float)pow((double)a.center.y - (double)bulletB.center.y, 2));

		float distanceB = sqrtf(
		  (float)pow((double)b.center.x - (double)bulletA.center.x, 2) +
		  (float)pow((double)b.center.y - (double)bulletA.center.y, 2));

		if (distanceB <= b.radius + bulletA.radius) {
			colorB = RED;
			if (lifeB > 0) {

				isShot = false;
				lifeB--;
			}
		}
		else {

			colorB = BLUE;
		}
			
		///
		/// ↑更新処理ここまで
		///

		///
		/// ↓描画処理ここから
		///
		// コマンドリストの取得
		ID3D12GraphicsCommandList* commandList = DirectXCommon::GetInstance()->GetCommandList();

		switch (scene) {
		case Scene::S_Title:
			// 前景スプライト描画前処理
			Sprite::PreDraw(commandList);

			/// <summary>
			/// ここに前景スプライトの描画処理を追加できる
			/// </summary>
			sTitle->Draw();
			//
			// スプライト描画後処理
			Sprite::PostDraw();
			break;

		case Scene::S_Game:
			break;
		}
		Novice::DrawEllipse(
		  (int)a.center.x, (int)a.center.y, (int)a.radius, (int)a.radius, 0.0f, colorA,
		  kFillModeSolid);
		Novice::DrawEllipse(
		  (int)bulletA.center.x, (int)bulletA.center.y, (int)bulletA.radius, (int)bulletA.radius,
		  0.0f, WHITE, kFillModeSolid);

		Novice::DrawEllipse(
		  (int)b.center.x, (int)b.center.y, (int)b.radius, (int)b.radius, 0.0f, colorB,
		  kFillModeSolid);
		Novice::DrawEllipse(
		  (int)bulletB.center.x, (int)bulletB.center.y, (int)bulletB.radius, (int)bulletB.radius,
		  0.0f, WHITE, kFillModeSolid);

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
	delete sTitle;

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
		send(sConnect, (const char*)&bulletA, sizeof(Circle), 0);
		send(sConnect, (const char*)&colorB, sizeof(int), 0);
		send(sConnect, (const char*)&scene, sizeof(Scene), 0);
		send(sConnect, (const char*)&lifeB, sizeof(int), 0);

		// データ受信
		int nRcv = recv(sConnect, (char*)&b, sizeof(Circle), 0);
		int nRcvB = recv(sConnect, (char*)&bulletB, sizeof(Circle), 0);
		int nRcvCol1 = recv(sConnect, (char*)&colorA, sizeof(int), 0);
		int nRcvScene = recv(sConnect, (char*)&scene, sizeof(Scene), 0);
		int nRcvLifeA = recv(sConnect, (char*)&lifeA, sizeof(Scene), 0);
		
		if (nRcv == SOCKET_ERROR)
			break;
		if (nRcvB == SOCKET_ERROR)
			break;
		if (nRcvCol1 == SOCKET_ERROR)
			break;
		if (nRcvScene == SOCKET_ERROR)
			break;
		if (nRcvLifeA == SOCKET_ERROR)
			break;
	}

	shutdown(sConnect, 2);
	closesocket(sConnect);
	return 0;
}