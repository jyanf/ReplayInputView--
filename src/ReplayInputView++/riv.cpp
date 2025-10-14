#include "riv.hpp"
#include "DrawCircle.hpp"
#include "InfoManager.hpp"
#include <functional>

ManagerInit ogBattleMgrConstruct[TARGET_MODES_COUNT]{};
VManagerDestruct ogBattleMgrDestruct[TARGET_MODES_COUNT]{};
VManagerOnProcess ogBattleMgrOnProcess[TARGET_MODES_COUNT]{};
VManagerOnRender ogBattleMgrOnRender[TARGET_MODES_COUNT]{};
int ogBattleMgrSize[TARGET_MODES_COUNT]{};

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
	return get_player(GameDataManager::instance, index);
	//if (index < 0 || index >= PLAYERS_NUMBER) return nullptr;
	//auto& players = *reinterpret_cast<std::array<Player*, PLAYERS_NUMBER>*>((DWORD)This + 0xC);
	//auto manager = GameDataManager::instance;
	//return manager && manager->enabledPlayers[index] ? players[index] : nullptr;
}
inline static void traversing_players(const BattleManager* This, 
	const std::function<void(int, Player*)>& forP,
	const std::function<void(int, GameObject*)>& forO
) {
	for (int i = 0; i < PLAYERS_NUMBER; ++i) {
		auto player = get_player(This, i);
		if (!player) continue;
		if (forP) forP(i, player);
		if (player->objectList) {
			auto& objects = player->objectList->getList();
			for (auto object : objects) {
				if (!object || object->lifetime<=0) continue;
				if (forO) forO(i, object);
			}
		}
	}
	return;
}
inline static bool check_hurtbreak(const BattleManager* This) {
	return This->matchState <= 1 
		|| This->matchState >= 6 
		|| This->matchState == 2 && This->frameCount == 0 
		|| This->matchState == 4 && SokuLib::mainMode == Mode::BATTLE_MODE_STORY;
}
	RivControl::RivControl() {
		for (int i = 0; i < PLAYERS_NUMBER; ++i) {
			//if (!manager->enabledPlayers[i]) continue;
			panels[i] = new pnl::Panel(i%2 ? -1 : 1);
			bool eni = iniProxy["InputPanel"_l]["p%d.Enabled"_l].value[i] != 0;
			bool enr = iniProxy["InputRecord"_l]["p%d.Enabled"_l].value[i] != 0;
			panels[i]->enableState = (eni^enr ? 1 : 0) + enr * 2;//0, 1, 3, 2

			//float x, y;
			//wsprintfW(buf, L"p%d.Position", i + 1);
			//GetPrivateProfileStringW(L"InputPanel", buf, L"", buf2, 24, path);//
			//if (swscanf(buf2, L"%f,%f", &x, &y) == 2)
				panels[i]->setPosI(iniProxy["InputPanel"_l]["p%d.Position"_l].value[i]);
			//GetPrivateProfileStringW(L"RecordPanel", buf, L"", buf2, 24, path);//
			//if (swscanf(buf2, L"%f,%f", &x, &y) == 2)
				panels[i]->setPosR(iniProxy["InputRecord"_l]["p%d.Position"_l].value[i]);
		}
		forwardCount = 1;
		forwardStep = 1;
		forwardIndex = 0;

		hitboxes = iniProxy["BoxDisplay"_l]["Enabled"_l] != 0;
		untech = hitboxes;

		paused = false;

		
	}
	using box::layers;
	int RivControl::update(BattleManager* This, int ind) {
		int ret = (This->*ogBattleMgrOnProcess[ind])();
		riv::box::flushWatcher();
		if (ret > 0 && ret < 4) {
			//vice.destroyWnd();
			vice.hideWnd(); vice.inter.focus = nullptr; 
			show_debug = false;//auto on? to do later
			return ret;
		}
		bool focus_valid = !vice.inter.focus;
		traversing_players(This,
			[this, &focus_valid](int i, Player* player) {
				if (panels[i]) {
					if (battleCounter <= 1)
						panels[i]->input.reset();
					panels[i]->update(*player);

				}

				if (vice.inter.focus == player)
					focus_valid = true;
			},
			[this, &focus_valid](int i, GameObject* object) {
				if (vice.inter.focus == object)
					focus_valid = true;
			}
		);
		vice.dirty = true;
		if (!focus_valid) {
			vice.inter.focus = nullptr;
			//vice.inter.index = -1;
		}
		return ret;
	}
	struct PanelData {
		riv::pnl::Panel* panel;
		static void __fastcall draw(PanelData* This) {
			This->panel->render();
		}
	};
	inline void pushPanel(riv::pnl::Panel* panel) {
		layers.push(riv::box::LayerManager::Top, PanelData{ panel });
	}
	void RivControl::render(BattleManager* This) {
		if (show_debug) {//insert all anchors
			if (vice.dirty) {//
				vice.inter.clear();

				traversing_players(This,
					[this](int i, Player* player) {
						vice.inter.insert(player);
					},
					[this](int i, GameObject* object) {
						vice.inter.insert(object);
					}
				);
				vice.inter.switchHover(1);
			}
			//vice.dirty = vice.inter.checkDMouse() || vice.dirty;
			if (vice.inter.checkDMouse() || vice.dirty) {
				//vice.updateWnd();
				vice.dirty = true;
			}			
		}
		// fix story blend
		auto guard = tex::RendererGuard();
		guard.setRenderMode(1);//auto old = SetRenderMode(1);
		box::setCamera();
		if (hitboxes 
		&& iniProxy["BoxDisplay"_l]["Floor"_l]
		&& This->matchState < 6
		&& (SokuLib::mainMode == Mode::BATTLE_MODE_STORY ? This->matchState != 4 && This->matchState != 0 : true)
		) {
			box::drawFloor();
		}

		const auto& pfocus = vice.inter.focus;
		auto phover = vice.inter.getHover();
		//float time = timeGetTime() / 1000.f;
		float time = ++counter / 60.f;
		traversing_players(This,//draw
			[this, This, pfocus, phover, time](int i, Player* player) {
				if (hitboxes && iniProxy["BoxDisplay"_l]["ArmorMeter"_l])
					layers.pushArmor(*player, !(player->unknown4AA || unblockable[i]) && player->boxData.frameData && player->boxData.frameData->frameFlags.guardAvailable);
				if (untech && iniProxy["BoxDisplay"_l]["JuggleMeter"_l]) {
					//box::drawUntechBar(*player);
					layers.pushUntech(*player);
				}
				if (panels[i]) {
					//panels[i]->render();
					pushPanel(panels[i]);
				}
				if (show_debug) {
					if (player == phover) {
						//guard.setInvert2();
						//info::drawObjectHover(player, time);
						return;
					}
					else {
						//box::drawPositionBox(*player);
						layers.pushPosition(*player);
					}
				}
				if (hitboxes && This->matchState > 0 && iniProxy["BoxDisplay"_l]["p%d.Character"_l].value[i]) {//boxes
					layers.pushPlayer(*player, check_hurtbreak(This), invulMelee[i] << 0 | invulBullet[i] << 1 | invulGrab[i] << 2);
				}
			},
			[this, This, pfocus, phover, time](int i, GameObject* object) {
				if (show_debug) {
					if (object == phover) {
						//info::drawObjectHover(object, time);
						return;
					}
					else {
						//box::drawPositionBox(*object, 5, Color::White * 0.5, box::Color_Gray * 0.5);//default every
						layers.pushPosition(*object, 5, Color::White * 0.5, box::Color_Gray * 0.5);
					}
				}
				if (hitboxes && This->matchState > 0 && iniProxy["BoxDisplay"_l]["p%d.Bullets"_l].value[i]) {
					layers.pushBullet(*object, check_hurtbreak(This));
				}

			}
		);
		layers.renderFore();
		//indicators
		if (show_debug) {
			if (phover) {
				info::drawObjectHover(phover, time);
				box::drawPositionBox(*phover, 7, box::Color_Gray, box::Color::White);
			}
			if (vice.inter.checkInWnd(vice.inter.cursor)) {
				guard
					//
					//.setRenderState(D3DRS_DESTBLEND, D3DBLEND_INVDESTCOLOR)
					//.setRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE)
					//.setRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE)
					//.setRenderState(D3DRS_BLENDOP, D3DBLENDOP_SUBTRACT)
					//.setRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_SUBTRACT)         // Alpha£º¼Ó·¨²Ù×÷
					.setInvert()
					.setTexture(0);
				float interp = sinf(time * 0.6f * 2*3.1416f) + 2.f; interp /= 3.01f;
				/*Draw2DCircle<32>(SokuLib::pd3dDev, vice.inter.cursor.to<float>(),
					vice.inter.toler + 20, 20, Vector2f{ 0, 360 },
					{ 0 }, 1, Color::White);*/
				Draw2DCircle<32>(SokuLib::pd3dDev, vice.inter.cursor.to<float>(),
					vice.inter.toler + 10, 20, Vector2f{0, 360},
					{ 0 }, 1, Color(int(0xFF * interp) * 0x01010101)* Color::Yellow);
				
				guard.resetRenderState();

				auto index = vice.inter.getIndex();
				if (index >= 0) {
					auto count = vice.inter.getCount();
					float rs = 360.0f / (count ? count : 1);
					Draw2DCircle<32>(SokuLib::pd3dDev, vice.inter.cursor.to<float>(),
						vice.inter.toler, 1.0f, Vector2f{ index * rs, (index + 1) * rs },
						{ 0 }, 1, Color::Black);
				}
			}
			if (pfocus) box::drawPositionBox(*pfocus, 7, box::Color_Gray, box::Color::Black);
			
		}
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
using riv::box::layers;

template<int i>
BattleManager* __fastcall CBattleManager_OnConstruct(BattleManager* This) {
	RivControl& riv = *(RivControl*)((char*)This + ogBattleMgrSize[i]);
#ifdef SUIT_4_PLAYERS
	//This->characterManager3 = This->characterManager4 = nullptr; //gamedata manager
#endif // SUIT_4_PLAYERS
	
	info::unfvck(SokuLib::pd3dDev);

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
		iniProxy.load();
		//riv::box::Texture_armorBar = riv::tex::create_texture_byid(8);
		riv::box::ArmorBar.create("rivpp/ArmorBar.png");
		riv::box::Texture_armorLifebar = riv::tex::create_texture("rivpp/ArmorLifebar.png");
		//riv.texID = create_texture(4);

		new(&riv)RivControl();//placement new

		riv.show_debug = iniProxy["Debug"_l]["Enabled"_l] != 0;
		slowdown_method = iniProxy["FrameRate"_l]["AdjustmentMethod"_l];
		if (slowdown_method != 0 && slowdown_method != 1) {
			slowdown_method = 1;
		}
		toggle_keys.display_boxes	=	iniProxy["Keys"_l]["display_boxes"_l];
		toggle_keys.display_info	=	iniProxy["Keys"_l]["display_info"_l];
		toggle_keys.display_inputs	=	iniProxy["Keys"_l]["display_inputs"_l];
		toggle_keys.decelerate		=	iniProxy["Keys"_l]["decelerate"_l];
		toggle_keys.accelerate		=	iniProxy["Keys"_l]["accelerate"_l];
		toggle_keys.stop			=	iniProxy["Keys"_l]["stop"_l];
		toggle_keys.framestep		=	iniProxy["Keys"_l]["framestep"_l];

		riv::box::startWatcher();
	}
	else {
		riv::box::closeWatcher();
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
				riv.untech = riv.hitboxes = !riv.hitboxes;
			}
			old_display_boxes = true;
		}
		else if (SokuLib::mainMode != Mode::BATTLE_MODE_VSWATCH
			&& check_key(toggle_keys.display_info) || (old_display_info = false)) {
			if (!old_display_info) {
				riv.show_debug = riv.vice.toggleWnd();
				if (riv.show_debug) {
					for (auto p : riv.panels)
						if (p) p->switchState(0);
				}
				//riv.show_debug = !riv.show_debug;
			}
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
			&& !riv.paused
			&& check_key(toggle_keys.decelerate) || (old_decelerate = false)
		) {
			if (old_decelerate) {}
			else if (slowdown_method) {//
				if (riv.forwardStep > 1) {
					riv.forwardCount = 1;
					riv.forwardStep -= 1;
				}
				else if (riv.forwardCount > 0) {//fix pause dece div0 crash
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
			&& !riv.paused
			&& check_key(toggle_keys.accelerate) || (old_accelerate = false)
		) {
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
				riv.paused = false;
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
		if (riv.forwardCount>=0 && riv.forwardIndex >= riv.forwardCount) {
			for (int i = riv.forwardIndex / riv.forwardCount; i > 0; --i) {
				ret = riv.update(This, ind);
			}
			riv.forwardIndex = 0;
		}
		else {
			auto info = SokuLib::v2::InfoManager::instance;
			if (info) {
				static const auto calcf = reinterpret_cast<float(__cdecl*)(int)>(0x4096d0);
				auto counterOfs = reinterpret_cast<int*>(DWORD(&info->state1[0]) + 0x14);
				auto offset = reinterpret_cast<float*>(DWORD(&info->state1[0]) + 0x8);
				*offset = 0.f;
				if (*counterOfs > 0) {
					*offset = (1 - calcf((*counterOfs)-- * 9)) * 300;
				}
				counterOfs = reinterpret_cast<int*>(DWORD(&info->state1[1]) + 0x14);
				offset = reinterpret_cast<float*>(DWORD(&info->state1[1]) + 0x8);
				*offset = 0.f;
				if (*counterOfs > 0) {
					*offset = (1 - calcf((*counterOfs)-- * 9)) * 300;
				}
				struct OfsAndCounter {
					float offset;  int counter; char unknown008[0x48];
					inline void calc(int dir = 1) {
						if (counter > 0) {
							offset = (1 - calcf(--counter * 9)) * 300 * dir;
						}
					}
				};
				auto& mods1 = *reinterpret_cast<SokuLib::Deque<OfsAndCounter>*>(&info->state1[0].unknown20);
				for (OfsAndCounter& o : mods1) { o.calc(info->state1[0].player && info->state1[0].player->teamId == 1 ? 1 : -1); }
				auto& mods2 = *reinterpret_cast<SokuLib::Deque<OfsAndCounter>*>(&info->state1[1].unknown20);
				for (OfsAndCounter& o : mods2) { o.calc(info->state1[1].player && info->state1[1].player->teamId == 1 ? 1 : -1); }
			}
		}
		riv.show_debug = riv.vice.viceDisplay;
	}
	else {
		ret = (This->*ogBattleMgrOnProcess[ind])();
		//riv::box::flushWatcher();

	}

	return ret;
}
TP_INSTANTIATE(0); TP_INSTANTIATE(1); TP_INSTANTIATE(2);

template<int i>
void __fastcall CBattleManager_OnRender(BattleManager* This) {
	RivControl& riv = *(RivControl*)((DWORD)This + ogBattleMgrSize[i]);
	layers.clear();
	//layers.renderBack();
	(This->*ogBattleMgrOnRender[i])();
	if (riv.enabled) {
		riv.render(This);
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
		riv.~RivControl();//i put destructor out of enabled check, my bad
	}
	riv::box::cleanWatcher();
	return (This->*ogBattleMgrDestruct[i])(dyn);
}
TD_INSTANTIATE(0); TD_INSTANTIATE(1); TD_INSTANTIATE(2);


