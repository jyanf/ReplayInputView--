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

#include <DrawUtils.hpp>
using SokuLib::DrawUtils::Vertex;
#include <Design.hpp>
using SokuLib::CTile;

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

using SWRVERTEX = SokuLib::DxVertex;

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

void __fastcall SaveTimers(GameDataManager* This);
template<int> BattleManager* __fastcall CBattleManager_OnConstruct(BattleManager* This);
template<int> int __fastcall CBattleManager_OnProcess(BattleManager* This);
template<int> void __fastcall CBattleManager_OnRender(BattleManager* This);
template<int> BattleManager* __fastcall CBattleManager_OnDestruct(BattleManager* This, int _, char dyn);
#define TC_INSTANTIATE(n) template BattleManager* __fastcall CBattleManager_OnConstruct<(n)>(BattleManager* This)
#define TP_INSTANTIATE(n) template int __fastcall CBattleManager_OnProcess<(n)>(BattleManager* This)
#define TR_INSTANTIATE(n) template void __fastcall CBattleManager_OnRender<(n)>(BattleManager* This)
#define TD_INSTANTIATE(n) template BattleManager* __fastcall CBattleManager_OnDestruct<(n)>(BattleManager* This, int _, char dyn)

extern HMODULE hDllModule;
extern std::filesystem::path configPath;

#define TARGET_MODES_COUNT (4)
typedef BattleManager* (__fastcall *ManagerInit)(BattleManager*);
typedef BattleManager* (BattleManager::* VManagerDestruct)(char);
typedef int (BattleManager::* VManagerOnProcess)();
typedef void (BattleManager::* VManagerOnRender)();
extern ManagerInit ogBattleMgrConstruct[TARGET_MODES_COUNT];
extern VManagerDestruct ogBattleMgrDestruct[TARGET_MODES_COUNT];
extern VManagerOnProcess ogBattleMgrOnProcess[TARGET_MODES_COUNT];
extern VManagerOnRender ogBattleMgrOnRender[TARGET_MODES_COUNT];
extern int ogBattleMgrSize[TARGET_MODES_COUNT];

extern void (__fastcall *ogUpdateMovement)(GameDataManager*);

