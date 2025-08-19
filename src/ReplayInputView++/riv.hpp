#include <TextureManager.hpp>
#include <BattleManager.hpp>
using BattleManager = SokuLib::BattleManager;

#include <BattleMode.hpp>
using Mode = SokuLib::BattleMode;
using SubMode = SokuLib::BattleSubMode;

#include <VTables.hpp>
#include <GameData.hpp>
using SokuLib::v2::groundHeight;
using GameDataManager = SokuLib::v2::GameDataManager;

#include <filesystem>

#include "box.hpp"

namespace riv {


struct Keys {
	UINT display_boxes;
	UINT display_info;
	UINT display_inputs;
	UINT decelerate;
	UINT accelerate;
	UINT stop;
	UINT framestep;
};

struct SWRVERTEX {
	float x, y, z;
	float rhw; // RHW = reciprocal of the homogeneous (clip space) w coordinate of a vertex (the 4th dimension for computing the scaling and translating)
	D3DCOLOR color;
	float u, v;

	void set(float _x, float _y, float _z) {
		x = _x;
		y = _y;
		z = _z;
	}

	void set_xy(float _x, float _y) {
		x = _x;
		y = _y;
	}
};

struct SWRCMDINFO {
	bool enabled;
	int prev; // number representing the previously pressed buttons (masks are applied)
	int now; // number representing the current pressed buttons (masks are applied)

	struct {
		bool enabled;
		int id[10];
		int base; // once len reaches 10 (first cycle), is incremented modulo 10
		int len; // starts at 0, caps at 10
	} record;
};

struct RivControl {
	bool enabled;
	int texID;
	int forwardCount;
	int forwardStep;
	int forwardIndex;
	SWRCMDINFO cmdp1;
	SWRCMDINFO cmdp2;
	bool hitboxes;
	bool untech;
	bool show_debug;
	bool paused;
};

}


BattleManager* __fastcall CBattleManager_OnCreate(BattleManager* This);
int __fastcall CBattleManager_OnProcess(BattleManager* This);
void __fastcall CBattleManager_OnRender(BattleManager* This);
BattleManager* __fastcall CBattleManager_OnDestruct(BattleManager* This, int _, char dyn);

extern HMODULE hDllModule;
extern std::filesystem::path configPath;
extern int ogBattleMgrSize;
extern BattleManager* (__fastcall *ogBattleMgrInit)(BattleManager*);
extern BattleManager* (BattleManager::* ogBattleMgrDestruct)(char);
extern int (BattleManager::* ogBattleMgrOnProcess)();
extern void (BattleManager::* ogBattleMgrOnRender)();

