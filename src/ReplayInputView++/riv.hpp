#include <TextureManager.hpp>
#include <BattleManager.hpp>
#include <BattleMode.hpp>
#include <VTables.hpp>
#include <GameData.hpp>
#include <DrawUtils.hpp>
using BattleManager = SokuLib::BattleManager;
using GameDataManager = SokuLib::v2::GameDataManager;

#include "../main.hpp"
#include "box.hpp"
#include "panel.hpp"
#include "info.hpp"

namespace riv {
using Mode = SokuLib::BattleMode;
using SubMode = SokuLib::BattleSubMode;
using SokuLib::v2::Player, SokuLib::v2::GameObjectBase, SokuLib::v2::GameObject;

static bool check_key(unsigned int key);
inline static Player* get_player(const GameDataManager* This, int index);
inline static Player* get_player(const BattleManager* This, int index);

	void __fastcall SaveTimers(GameDataManager* This);
	struct HotKeys {
		UINT display_boxes;
		UINT display_info;
		UINT display_inputs;
		UINT decelerate;
		UINT accelerate;
		UINT stop;
		UINT framestep;
	};

//using SWRVERTEX = SokuLib::DxVertex;
	struct RivControlOld {
		bool enabled = true;
		int texID = NULL;
		int forwardCount;
		int forwardStep;
		int forwardIndex;
		pnl::SWRCMDINFO cmdp1;
		pnl::SWRCMDINFO cmdp2;
		bool hitboxes = false;
		bool untech = false;
		bool show_debug = false;//unused
		bool paused = false;
	};

	class RivControl : public RivControlOld {
	public:
		std::array<pnl::Panel*, PLAYERS_NUMBER> panels = {nullptr};
		info::Vice vice;
		//std::array<SWRCMDINFO, PLAYERS_NUMBER> cmds;
		RivControl();
		RivControl(RivControl&& n) noexcept {
			memmove(this, &n, sizeof(n));
		}
		RivControl& operator=(RivControl&& n) noexcept {
			memmove(this, &n, sizeof(n));
			return *this;
		}
		~RivControl() {
			for (auto ptr : panels)
				delete ptr;
		}
		int update(BattleManager* This, int ind);
		void render(BattleManager* This);
	};

}

template<int> BattleManager* __fastcall CBattleManager_OnConstruct(BattleManager* This);
template<int> int __fastcall CBattleManager_OnProcess(BattleManager* This);
template<int> void __fastcall CBattleManager_OnRender(BattleManager* This);
template<int> BattleManager* __fastcall CBattleManager_OnDestruct(BattleManager* This, int _, char dyn);
#define TC_INSTANTIATE(n) template BattleManager* __fastcall CBattleManager_OnConstruct<(n)>(BattleManager* This)
#define TP_INSTANTIATE(n) template int __fastcall CBattleManager_OnProcess<(n)>(BattleManager* This)
#define TR_INSTANTIATE(n) template void __fastcall CBattleManager_OnRender<(n)>(BattleManager* This)
#define TD_INSTANTIATE(n) template BattleManager* __fastcall CBattleManager_OnDestruct<(n)>(BattleManager* This, int _, char dyn)

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

