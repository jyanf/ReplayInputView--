#include "riv.hpp"

ManagerInit ogBattleMgrConstruct[TARGET_MODES_COUNT];
VManagerDestruct ogBattleMgrDestruct[TARGET_MODES_COUNT];
VManagerOnProcess ogBattleMgrOnProcess[TARGET_MODES_COUNT];
VManagerOnRender ogBattleMgrOnRender[TARGET_MODES_COUNT];
int ogBattleMgrSize[TARGET_MODES_COUNT];

void(__fastcall* ogUpdateMovement)(GameDataManager*);


namespace riv {
static bool invulMelee[PLAYERS_NUMBER], invulBullet[PLAYERS_NUMBER], invulGrab[PLAYERS_NUMBER], unblockable[PLAYERS_NUMBER];
static int slowdown_method = 1;
static HotKeys toggle_keys;

static bool check_key(unsigned int key) {
	// return CheckKeyOneshot(key, mod1, mod2, mod3);
	//int* keytable = (int*)0x8998D8;//only for F Keys
	if (!key) return false;
	const unsigned char(&keytable)[256] = *(unsigned char(*)[256])0x8a01b8;//DIK array
	return keytable[key] & 0x80;
}
inline static Player* get_player(const GameDataManager* This, int index) {
	if (index < 0 || index >= PLAYERS_NUMBER) return nullptr;
	//auto& players = *reinterpret_cast<std::array<SokuLib::v2::Player*, PLAYERS_NUMBER>*>((DWORD)This + 0x28);
	return This && This->enabledPlayers[index] ? This->players[index] : nullptr;
}
inline static Player* get_player(const BattleManager* This, int index) {
	if (index < 0 || index >= PLAYERS_NUMBER) return nullptr;
	auto& players = *reinterpret_cast<std::array<SokuLib::v2::Player*, PLAYERS_NUMBER>*>((DWORD)This + 0xC);
	auto manager = GameDataManager::instance;
	return manager && manager->enabledPlayers[index] ? players[index] : nullptr;
}

	RivControl::RivControl() {
		auto path = configPath.c_str();
		for (int i = 0; i < PLAYERS_NUMBER; ++i) {
			//if (!manager->enabledPlayers[i]) continue;
			panels[i] = new pnl::Panel(i%2 ? -1 : 1);

			WCHAR buf[24], buf2[32];
			wsprintfW(buf, L"p%d.Enabled", i + 1);
			bool eni = GetPrivateProfileIntW(L"InputPanel", buf, 0, path) != 0;
			bool enr = GetPrivateProfileIntW(L"RecordPanel", buf, 0, path) != 0;
			panels[i]->enableState = (eni^enr ? 1 : 0) + enr * 2;//0, 1, 3, 2

			float x, y;
			wsprintfW(buf, L"p%d.Position", i + 1);
			GetPrivateProfileStringW(L"InputPanel", buf, L"", buf2, 24, path);//
			if (swscanf(buf2, L"%f,%f", &x, &y) == 2)
				panels[i]->setPosI(SokuLib::Vector2f{ x, y });
			GetPrivateProfileStringW(L"RecordPanel", buf, L"", buf2, 24, path);//
			if (swscanf(buf2, L"%f,%f", &x, &y) == 2)
				panels[i]->setPosR(SokuLib::Vector2f{ x, y });
				
		}
		forwardCount = 1;
		forwardStep = 1;
		forwardIndex = 0;

		hitboxes = GetPrivateProfileIntW(L"HitboxDisplay", L"Enabled", 0, path) != 0;
		untech = GetPrivateProfileIntW(L"JuggleMeter", L"Enabled", 0, path) != 0;

		paused = false;

		
	}

	int RivControl::update(BattleManager* This, int ind) {
		int ret = (This->*ogBattleMgrOnProcess[ind])();
		riv::box::setDirty(true);
		if (ret > 0 && ret < 4)
			return ret;

		for (int i = 0; i < panels.size(); ++i) {
			auto player = get_player(This, i);
			if (!player || !panels[i]) continue;
			if (*(int*)0x8985d8<=1) panels[i]->input.reset();
			panels[i]->update(*player);
		}
		return ret;
	}

	void __fastcall SaveTimers(GameDataManager* This) {
		for (int i = 0; i < PLAYERS_NUMBER; ++i) {
			auto player = get_player(This, i);
			if (!player) continue;
			invulMelee[i] = player->meleeInvulTimer;
			invulBullet[i] = player->projectileInvulTimer;
			invulGrab[i] = player->grabInvulTimer;
			unblockable[i] = player->unknown4AA;//blockDisabled
		}
		return ogUpdateMovement(This);
	}
//static void draw_debug_info(void* This) {
//	static D3DCOLOR gray[] = { 0x4f909090, 0x4f909090, 0x4f909090, 0x4f909090 };
//	CustomQuad quad = {};
//	quad.set_rect(115, 80, 260, 90);
//	quad.set_colors(gray);
//	CBattleManager_RenderQuad(quad.v);
//
//	static char buffer[1024];
//
//	int scene_addr = ACCESS_INT(ADDR_BATTLE_MANAGER, 0);
//	int p1 = ACCESS_INT(scene_addr, 0x0C);
//	int p2 = ACCESS_INT(scene_addr, 0x10);
//
//	int p1_seq = ACCESS_SHORT(p1, CF_CURRENT_SEQ);
//	int p1_sub = ACCESS_SHORT(p1, CF_CURRENT_SUBSEQ);
//	int p1_frm = ACCESS_SHORT(p1, CF_CURRENT_FRAME);
//	int p2_seq = ACCESS_SHORT(p2, CF_CURRENT_SEQ);
//	int p2_sub = ACCESS_SHORT(p2, CF_CURRENT_SUBSEQ);
//	int p2_frm = ACCESS_SHORT(p2, CF_CURRENT_FRAME);
//
//	sprintf_s(buffer, sizeof(buffer),
//		"1f %03d %03d %03d 1l %04d\n"
//		"2f %03d %03d %03d 2l %04d\n"
//		"1stop %03d untech %04d 1hst %01d 1mhc %02d\n"
//		"2stop %03d untech %04d 2hst %01d 2mhc %02d\n"
//		"1pos %0.2f %0.2f 1vel %0.2f %0.2f 1g %0.2f\n"
//		"2pos %0.2f %0.2f 2vel %0.2f %0.2f 2g %0.2f\n",
//		ACCESS_SHORT(p1, CF_CURRENT_SEQ), ACCESS_SHORT(p1, CF_CURRENT_SUBSEQ), ACCESS_SHORT(p1, CF_CURRENT_FRAME), ACCESS_INT(p1, CF_ELAPSED_IN_SUBSEQ),
//		ACCESS_SHORT(p2, CF_CURRENT_SEQ), ACCESS_SHORT(p2, CF_CURRENT_SUBSEQ), ACCESS_SHORT(p2, CF_CURRENT_FRAME), ACCESS_INT(p2, CF_ELAPSED_IN_SUBSEQ),
//		ACCESS_SHORT(p1, CF_HIT_STOP), ACCESS_SHORT(p1, CF_UNTECH), ACCESS_INT(p1, CF_HIT_STATE), ACCESS_SHORT(p1, 0x7D0), ACCESS_SHORT(p2, CF_HIT_STOP),
//		ACCESS_SHORT(p2, CF_UNTECH), ACCESS_INT(p2, CF_HIT_STATE), ACCESS_SHORT(p2, 0x7D0), ACCESS_FLOAT(p1, CF_X_POS), ACCESS_FLOAT(p1, CF_Y_POS),
//		ACCESS_FLOAT(p1, CF_X_SPEED), ACCESS_FLOAT(p1, CF_Y_SPEED), ACCESS_FLOAT(p1, CF_GRAVITY), ACCESS_FLOAT(p2, CF_X_POS), ACCESS_FLOAT(p2, CF_Y_POS),
//		ACCESS_FLOAT(p2, CF_X_SPEED), ACCESS_FLOAT(p2, CF_Y_SPEED), ACCESS_FLOAT(p2, CF_GRAVITY));
//
//	text::SetText(buffer);
//	text::OnRender(This);
//}


}


using riv::slowdown_method, riv::RivControl, riv::check_key, riv::toggle_keys, riv::Mode, riv::SubMode;
using riv::box::drawPlayerBoxes, riv::box::drawUntechBar, riv::box::drawFloor;

template<int i>
BattleManager* __fastcall CBattleManager_OnConstruct(BattleManager* This) {
	RivControl& riv = *(RivControl*)((char*)This + ogBattleMgrSize[i]);

#ifdef SUIT_4_PLAYERS
	//This->characterManager3 = This->characterManager4 = nullptr; //gamedata manager
#endif // SUIT_4_PLAYERS
	
	ogBattleMgrConstruct[i](This);

	static char tmp[1024];

	auto& submode = SokuLib::subMode;
	auto& mode = SokuLib::mainMode;
	if (submode == SubMode::BATTLE_SUBMODE_REPLAY
		|| mode != Mode::BATTLE_MODE_VSCLIENT && mode != Mode::BATTLE_MODE_VSSERVER
	) {
		riv.enabled = true;
	}
	else
		riv.enabled = false;

	if (riv.enabled) {
		//riv::box::Texture_armorBar = riv::tex::create_texture_byid(8);
		riv::box::ArmorBar.create(8);
		riv::box::Texture_armorLifebar = riv::tex::create_texture_byid(12);
		//riv.texID = create_texture(4);

		new(&riv)RivControl();//placement new

		//riv.show_debug = GetPrivateProfileIntW(L"Debug", L"Enabled", 0, path) != 0;

		auto path = configPath.c_str();
		slowdown_method = GetPrivateProfileIntW(L"Framerate", L"AdjustmentMethod", 1, path);
		if (slowdown_method != 0 && slowdown_method != 1) {
			slowdown_method = 1;
		}
		toggle_keys.display_boxes = GetPrivateProfileIntW(L"Keys", L"display_boxes", 0, path);
		toggle_keys.display_info = GetPrivateProfileIntW(L"Keys", L"display_info", 0, path);
		toggle_keys.display_inputs = GetPrivateProfileIntW(L"Keys", L"display_inputs", 0, path);
		toggle_keys.decelerate = GetPrivateProfileIntW(L"Keys", L"decelerate", 0, path);
		toggle_keys.accelerate = GetPrivateProfileIntW(L"Keys", L"accelerate", 0, path);
		toggle_keys.stop = GetPrivateProfileIntW(L"Keys", L"stop", 0, path);
		toggle_keys.framestep = GetPrivateProfileIntW(L"Keys", L"framestep", 0, path);

		riv::box::setDirty(false);
	}
	return This;
}
TC_INSTANTIATE(0); TC_INSTANTIATE(1); TC_INSTANTIATE(2); TC_INSTANTIATE(3);

template<int ind>
int __fastcall CBattleManager_OnProcess(BattleManager* This) {
	static int fps_steps[] = { 16, 20, 24, 28, 33, 66, 100, 200 };

	static int fps_index = 0;
	static int n_steps = sizeof(fps_steps) / sizeof(*fps_steps);

	RivControl& riv = *(RivControl*)((DWORD)This + ogBattleMgrSize[ind]);
	int ret = 0;

	int* delay = (int*)0x8A0FF8;

	if (riv.enabled) {
		
		static bool old_display_boxes = false;
		static bool old_display_info = false;
		static bool old_display_inputs = false;
		static bool old_decelerate = false;
		static bool old_accelerate = false;
		static bool old_framestop = false;
		static bool old_stop = false;
		if (check_key(toggle_keys.display_boxes) || (old_display_boxes = false)) {
			if (!old_display_boxes) {
				riv.hitboxes = !riv.hitboxes;
				riv.untech = !riv.untech;
			}
			old_display_boxes = true;
		}
		else if (check_key(toggle_keys.display_info) || (old_display_info = false)) {
			if (!old_display_info)
				riv.show_debug = !riv.show_debug;
			old_display_info = true;
		}
		else if (check_key(toggle_keys.display_inputs) || (old_display_inputs = false)) {
			if (!old_display_inputs) {
				int i=-1;
				for (auto p : riv.panels)
				{
					if (!p) continue;
					p->switchState(i);
					i = p->enableState;
				}
			}
			old_display_inputs = true;
		}
		else if (SokuLib::mainMode != Mode::BATTLE_MODE_VSWATCH 
			&& check_key(toggle_keys.decelerate) || (old_decelerate = false)) {
			if (old_decelerate) {}
			else if (slowdown_method) {//
				if (riv.forwardStep > 1) {
					riv.forwardCount = 1;
					riv.forwardStep -= 1;
				}
				else {
					riv.forwardCount += 1;
					riv.forwardStep = 1;
				}
				riv.forwardIndex = 0;
			}
			else {//useless with gr locked rate
				//*delay += 4;
				if (fps_index < (n_steps - 1)) {
					*delay = fps_steps[++fps_index];
				}
			}
			old_decelerate = true;
		}
		else if (SokuLib::mainMode != Mode::BATTLE_MODE_VSWATCH 
			&& check_key(toggle_keys.accelerate) || (old_accelerate = false)) {
			if (old_accelerate) {}
			else if (slowdown_method) {
				if (riv.forwardCount > 1) {
					riv.forwardCount -= 1;
					riv.forwardStep = 1;
				}
				else {
					riv.forwardCount = 1;
					riv.forwardStep += 1;
				}
				riv.forwardIndex = 0;
			}
			else {
				//*delay -= 4;
				if (0 < fps_index) {
					*delay = fps_steps[--fps_index];
				}
			}
			old_accelerate = true;
		}
		else if (SokuLib::mainMode != Mode::BATTLE_MODE_VSWATCH
			&& check_key(toggle_keys.stop) || (old_stop = false)) {
			if (old_stop) {}
			else if (!riv.paused) {
				riv.forwardCount = -1;
				riv.forwardStep = 0;
				riv.forwardIndex = 0;
				riv.paused = true;
			}
			else {
				riv.forwardCount = 1;
				riv.forwardStep = 1;
				riv.forwardIndex = 0;
				riv.paused = false;
			}
			old_stop = true;
		}
		else if (check_key(toggle_keys.framestep) || (old_framestop = false)) {
			if (old_framestop) {}
			else if (riv.paused) {
				//process_frame(This, riv);
				//int ret = (This->*ogBattleMgrOnProcess[ind])();
				ret = riv.update(This, ind);
			}
			old_framestop = true;//fix
		}

		riv.forwardIndex += riv.forwardStep;
		if (riv.forwardIndex >= riv.forwardCount) {
			for (int i = riv.forwardIndex / riv.forwardCount; i > 0; --i) {
				ret = riv.update(This, ind);
			}
			riv.forwardIndex = 0;
		}
		
	}
	else {
		ret = (This->*ogBattleMgrOnProcess[ind])();
		//riv::box::setDirty(true);
	}

	return ret;
}
TP_INSTANTIATE(0); TP_INSTANTIATE(1); TP_INSTANTIATE(2);

template<int i>
void __fastcall CBattleManager_OnRender(BattleManager* This) {
	RivControl& riv = *(RivControl*)((DWORD)This + ogBattleMgrSize[i]);

	(This->*ogBattleMgrOnRender[i])();

	if (riv.enabled) {
		// fix story blend
		auto old = riv::SetRenderMode(1);
		

		riv::box::setCamera();
		if (riv.hitboxes) drawFloor();
		for (int i = 0; i<PLAYERS_NUMBER; ++i) {
			auto player = riv::get_player(This, i);
			if (!player) continue;
			if (riv.hitboxes && This->matchState > 0) {
				drawPlayerBoxes(*player, 
					This->matchState <= 1 || This->matchState >= 6 
						|| This->matchState==2 && This->frameCount==0, 
					riv::invulMelee[i]<<0 | riv::invulBullet[i]<<1 | riv::invulGrab[i]<<2 | riv::unblockable[i]<<3);
			}
			if (riv.untech) {
				drawUntechBar(*player);
			}
			if (riv.panels[i]) riv.panels[i]->render();
		}
		riv::box::setDirty(false);

		if (riv.show_debug) {
			//riv::draw_debug_info(This);
		}

		riv::SetRenderMode(old);
	}
}
TR_INSTANTIATE(0); TR_INSTANTIATE(1); TR_INSTANTIATE(2);

template<int i>
BattleManager* __fastcall CBattleManager_OnDestruct(BattleManager* This, int _, char dyn) {
	RivControl& riv = *(RivControl*)((char*)This + ogBattleMgrSize[i]);

	if (riv.enabled) {
		//SokuLib::textureMgr.remove(riv.texID);
		//SokuLib::textureMgr.remove(riv::box::Texture_armorBar);
		riv::box::ArmorBar.cancel();
		SokuLib::textureMgr.remove(riv::box::Texture_armorLifebar);
		//text::OnDestruct(This, _, dyn);
		/*auto path = configPath.c_str();
		for (int i = 0; i < PLAYERS_NUMBER; i++)
		{
			WCHAR buf[24];
			if (!riv::get_player(This, i) || !riv.panels[i]) continue;
			wsprintfW(buf, L"p%d.Enabled", i+1);
			auto state = riv.panels[i]->enableState;
			WritePrivateProfileStringW(L"InputPanel", buf, 1<=state && state<=2  ? L"1" : L"0", path);
			WritePrivateProfileStringW(L"RecordPanel", buf, 2<=state && state<=3 ? L"1" : L"0", path);
		}*/
	}
	riv.~RivControl();
	riv::box::cleanWatcher();
	return (This->*ogBattleMgrDestruct[i])(dyn);
}
TD_INSTANTIATE(0); TD_INSTANTIATE(1); TD_INSTANTIATE(2);


