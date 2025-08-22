#include "riv.hpp"

HMODULE hDllModule;
std::filesystem::path configPath;
ManagerInit ogBattleMgrConstruct[TARGET_MODES_COUNT];
VManagerDestruct ogBattleMgrDestruct[TARGET_MODES_COUNT];
VManagerOnProcess ogBattleMgrOnProcess[TARGET_MODES_COUNT];
VManagerOnRender ogBattleMgrOnRender[TARGET_MODES_COUNT];
int ogBattleMgrSize[TARGET_MODES_COUNT];

static bool invulMelee[PLAYERS_NUMBER], invulBullet[PLAYERS_NUMBER], invulGrab[PLAYERS_NUMBER], unblockable[PLAYERS_NUMBER];
void(__fastcall* ogUpdateMovement)(GameDataManager*);

static int s_slowdown_method = 1;


/*
 * ReplayInputView functions
 * Displays the inputted buttons and create a history from it.
 */

namespace riv {

template<int d = 2>
static void __fastcall RenderMyBack(float x, float y, int cx, int cy) {
	const SWRVERTEX vertices[] = {
		{x - d, y, 0.0f, 1.0f, 0xa0808080, 0.0f, 0.0f},
		{x + cx - d, y, 0.0f, 1.0f, 0xa0808080, 1.0f, 0.0f},
		{x + cx + d, y + cy, 0.0f, 1.0f, 0xa0202020, 1.0f, 1.0f},
		{x + d, y + cy, 0.0f, 1.0f, 0xa0202020, 0.0f, 1.0f},
	};
	SokuLib::textureMgr.setTexture(NULL, 0);
	SokuLib::pd3dDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, (const void*)vertices, sizeof(SWRVERTEX));
}

static void __fastcall RenderQuad(SWRVERTEX quad[4]) {
	SokuLib::textureMgr.setTexture(NULL, 0);
	SokuLib::pd3dDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, (const void*)quad, sizeof(SWRVERTEX));
}

static void RenderTile(float x, float y, int u, int v, int a) {
	int dif = (a << 24) | 0xFFFFFF;
	float fu = u / 256.0f;
	float fv = v / 64.0f;

	// ¥Ç¥Ð¥Ã¥°¥Ó¥ë¥É¤À¤È¥°¥À¥°¥À¤Ë¤Ê¤ë¤Î¤ÏºÎ¤Ê¤Î£¿°³ËÀ¤Ì¤Î£¿
	const SWRVERTEX vertices[] = {
		{x, y, 0.0f, 1.0f, dif, fu, fv},
		{x + 32.0f, y, 0.0f, 1.0f, dif, fu + 0.125f, fv},
		{x + 32.0f, y + 32.0f, 0.0f, 1.0f, dif, fu + 0.125f, fv + 0.5f},
		{x, y + 32.0f, 0.0f, 1.0f, dif, fu, fv + 0.5f},
	};
	SokuLib::pd3dDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, (const void*)vertices, sizeof(SWRVERTEX));
}

template <int d = 2>
static void RenderInputPanel(const RivControl& riv, SWRCMDINFO& cmd, float x, float y) {

	if (cmd.enabled) {
		// Background
		RenderMyBack<d>(x, y, 24 * 6 + 24, 24 * 3 + 12);

		SokuLib::textureMgr.setTexture(riv.texID, 0);
#ifdef _DEBUG
		DWORD old;
		SokuLib::pd3dDev->GetFVF(&old);
		printf("%#x\n", old);
#endif // _DEBUG


		// Directions
		RenderTile(x + 9 + 24,	y + 6,		0,	0,	(cmd.now % 16 == 1 ? 255 : 48)); /* ¡ü */
		RenderTile(x + 9 + 24,	y + 6 + 48, 32,	0,	(cmd.now % 16 == 2 ? 255 : 48)); /* ¡ý */
		RenderTile(x + 9,		y + 6 + 24, 64,	0,	(cmd.now % 16 == 4 ? 255 : 48)); /* ¡û */
		RenderTile(x + 9 + 48,	y + 6 + 24, 96,	0,	(cmd.now % 16 == 8 ? 255 : 48)); /* ¡ú */
		RenderTile(x + 9,		y + 6,		128,0,	(cmd.now % 16 == 5 ? 255 : 48)); /* ¨I */
		RenderTile(x + 9 + 48,	y + 6,		160,0,	(cmd.now % 16 == 9 ? 255 : 48)); /* ¨K */
		RenderTile(x + 9 + 48,	y + 6 + 48, 192,0,	(cmd.now % 16 ==10 ? 255 : 48)); /* ¨J */
		RenderTile(x + 9,		y + 6 + 48, 224,0,	(cmd.now % 16 == 6 ? 255 : 48)); /* ¨L */
		// Buttons
		RenderTile(x + 9 + 75 - d,		y + 6 + 12, 0,	32, (cmd.now & 16 ? 255 : 48));
		RenderTile(x + 9 + 75 - d + 27, y + 6 + 12, 32, 32, (cmd.now & 32 ? 255 : 48));
		RenderTile(x + 9 + 75 - d + 54, y + 6 + 12, 64, 32, (cmd.now & 64 ? 255 : 48));
		RenderTile(x + 9 + 75 + d,		y + 6 + 36, 96, 32, (cmd.now & 128? 255 : 48));
		RenderTile(x + 9 + 75 + d + 27, y + 6 + 36, 128,32, (cmd.now & 256? 255 : 48));
		RenderTile(x + 9 + 75 + d + 54, y + 6 + 36, 160,32, (cmd.now & 512? 255 : 48));
	}
}

template <int d = 2>
static void RenderRecordPanel(const RivControl& riv, SWRCMDINFO& cmd, float x, float y) {

	if (cmd.record.enabled) {
		RenderMyBack<d>(x, y, 24 * 10 + 6, 24 + 6);

		SokuLib::textureMgr.setTexture( riv.texID, 0);
		for (int i = 0; i < cmd.record.len; ++i) { // Render all the buttons in the buffer
			int j = (i + cmd.record.base) % _countof(cmd.record.id);
			int id = cmd.record.id[j];
			RenderTile(x + 3 + i * 24, y + 3, (id % 8) * 32, (id / 8) * 32, 255); // x, y, width of image, height of image, alpha
		}
	}
}

static void DetermineRecord(SWRCMDINFO& cmd, int mask, int flag, int id) // this function is called for every button: direction first, buttons after
{
	if ((cmd.prev & mask) != flag && (cmd.now & mask) == flag) {
		int index = (cmd.record.base + cmd.record.len) % _countof(cmd.record.id);

		cmd.record.id[index] = id;
#ifdef _DEBUG
		for (int i = 0; i < _countof(cmd.record.id); ++i) // 0-7 = directions,  8-13 = buttons
			printf("%d ", cmd.record.id[i]);
		puts("");
#endif // _DEBUG

		if (cmd.record.len == _countof(cmd.record.id)) {
			cmd.record.base = (cmd.record.base + 1) % _countof(cmd.record.id);
		}
		else {
			cmd.record.len++;
		}
	}
}

static void RefreshCommandInfo(SWRCMDINFO& cmd, Player* Char) {
	auto& input = Char->inputData.keyInput;

	cmd.prev = cmd.now;
	cmd.now = 0;
	if (input.verticalAxis < 0)
		cmd.now |= 1;
	if (input.verticalAxis > 0)
		cmd.now |= 2;
	if (input.horizontalAxis < 0)
		cmd.now |= 4;
	if (input.horizontalAxis > 0)
		cmd.now |= 8;
	if (input.a > 0)
		cmd.now |= 16;
	if (input.b > 0)
		cmd.now |= 32;
	if (input.c > 0)
		cmd.now |= 64;
	if (input.d > 0)
		cmd.now |= 128;
	if (input.changeCard > 0)
		cmd.now |= 256;
	if (input.spellcard > 0)
		cmd.now |= 512;

	if (cmd.record.enabled) {
		DetermineRecord(cmd, 15, 5, 4);
		DetermineRecord(cmd, 15, 1, 0);
		DetermineRecord(cmd, 15, 9, 5);
		DetermineRecord(cmd, 15, 4, 2);
		DetermineRecord(cmd, 15, 8, 3);
		DetermineRecord(cmd, 15, 6, 7);
		DetermineRecord(cmd, 15, 2, 1);
		DetermineRecord(cmd, 15, 10, 6);

		DetermineRecord(cmd, 16, 16, 8);
		DetermineRecord(cmd, 32, 32, 9);
		DetermineRecord(cmd, 64, 64, 10);
		DetermineRecord(cmd, 128, 128, 11);
		DetermineRecord(cmd, 256, 256, 12);
		DetermineRecord(cmd, 512, 512, 13);
	}
}

/*
 * ReplayInputView+ functions
 * Displays the hitboxes of characters and attacks, and manipulates the refresh rate of the game.
 */


static bool check_key(unsigned int key, bool mod1, bool mod2, bool mod3) {
	// return CheckKeyOneshot(key, mod1, mod2, mod3);
	//int* keytable = (int*)0x8998D8;//only for F Keys
	const unsigned char (&keytable)[256] = *(unsigned char(*)[256])0x8a01b8;//DIK array
	return keytable[key] & 0x80;
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

static Keys toggle_keys;

}


using riv::RivControl, riv::check_key, riv::toggle_keys;
using riv::box::drawPlayerBoxes, riv::box::drawUntechBar, riv::box::drawFloor;

static int create_texture(int offset) {
	//SUCCEEDED(D3DXCreateTextureFromResource(SokuLib::pd3dDev, hDllModule, MAKEINTRESOURCE(8), pphandle)
	int id;
	auto pphandle = SokuLib::textureMgr.allocate(&id);
	if (pphandle)
		*pphandle = NULL;
	if (SUCCEEDED(D3DXCreateTextureFromResource(SokuLib::pd3dDev, hDllModule, MAKEINTRESOURCE(offset), pphandle))) {
		return id;
	}
	else {
		SokuLib::textureMgr.deallocate(id);
		return 0;
	}
}
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
		//CTextureManager_LoadTextureFromResource(g_textureMgr, &riv.texID, s_hDllModule, );
		riv.texID = create_texture(4);
		riv::box::Texture_armorBar = create_texture(8);
		riv::box::Texture_armorLifebar = create_texture(12);

		//text::LoadSettings(ini, "Debug");
		//text::OnCreate(This);

		if (riv.texID != 0) {
			riv.forwardCount = 1;
			riv.forwardStep = 1;
			riv.forwardIndex = 0;

			auto path = configPath.c_str();

			riv.cmdp1.enabled = GetPrivateProfileIntW(L"Input", L"p1.Enabled", 1, path) != 0;
			riv.cmdp1.prev = 0;
			riv.cmdp1.record.base = riv.cmdp1.record.len = 0;
			riv.cmdp1.record.enabled = GetPrivateProfileIntW(L"Record", L"p1.Enabled", 0, path) != 0;

			riv.cmdp2.enabled = GetPrivateProfileIntW(L"Input", L"p2.Enabled", 1, path) != 0;
			riv.cmdp2.prev = 0;
			riv.cmdp2.record.base = riv.cmdp2.record.len = 0;
			riv.cmdp2.record.enabled = GetPrivateProfileIntW(L"Record", L"p2.Enabled", 0, path) != 0;

			riv.hitboxes = GetPrivateProfileIntW(L"HitboxDisplay", L"Enabled", 0, path) != 0;
			riv.untech = GetPrivateProfileIntW(L"JuggleMeter", L"Enabled", 0, path) != 0;

			riv.paused = false;//fix failed pause problem?

			//riv.show_debug = GetPrivateProfileIntW(L"Debug", L"Enabled", 0, path) != 0;

			s_slowdown_method = GetPrivateProfileIntW(L"Framerate", L"AdjustmentMethod", 0, path);
			if (s_slowdown_method != 0 && s_slowdown_method != 1) {
				s_slowdown_method = 1;
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
		else {
			riv.enabled = false;
		}
	}
	return This;
}
TC_INSTANTIATE(0); TC_INSTANTIATE(1); TC_INSTANTIATE(2); TC_INSTANTIATE(3);

void __fastcall SaveTimers(GameDataManager* This) {
	auto& players = *reinterpret_cast<std::array<Player*, PLAYERS_NUMBER>*>((DWORD)This + 0x28);
	for (int i = 0; i < players.size(); ++i) {
		if (!players[i] || This && !This->enabledPlayers[i]) continue;
		invulMelee[i] = players[i]->meleeInvulTimer;
		invulBullet[i] = players[i]->projectileInvulTimer;
		invulGrab[i] = players[i]->grabInvulTimer;
		unblockable[i] = players[i]->unknown4AA;//blockDisabled
	}
	return ogUpdateMovement(This);
}
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
		if (check_key(toggle_keys.display_boxes, 0, 0, 0) || (old_display_boxes = false)) {
			if (!old_display_boxes) {
				riv.hitboxes = !riv.hitboxes;
				riv.untech = !riv.untech;
			}
			old_display_boxes = true;
		}
		else if (check_key(toggle_keys.display_info, 0, 0, 0) || (old_display_info = false)) {
			if (!old_display_info)
				riv.show_debug = !riv.show_debug;
			old_display_info = true;
		}
		else if (check_key(toggle_keys.display_inputs, 0, 0, 0) || (old_display_inputs = false)) {
			if (!old_display_inputs) {
				bool cmdEnabled = riv.cmdp1.enabled;
				bool recordEnabled = riv.cmdp1.record.enabled;
				riv.cmdp1.enabled = !recordEnabled;
				riv.cmdp1.record.enabled = cmdEnabled;
				riv.cmdp2.enabled = riv.cmdp1.enabled;
				riv.cmdp2.record.enabled = riv.cmdp1.record.enabled;
			}
			old_display_inputs = true;
		}
		else if (SokuLib::mainMode != Mode::BATTLE_MODE_VSWATCH 
			&& check_key(toggle_keys.decelerate, 0, 0, 0) || (old_decelerate = false)) {
			if (old_decelerate) {}
			else if (s_slowdown_method) {//
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
			&& check_key(toggle_keys.accelerate, 0, 0, 0) || (old_accelerate = false)) {
			if (old_accelerate) {}
			else if (s_slowdown_method) {
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
			&& check_key(toggle_keys.stop, 0, 0, 0) || (old_stop = false)) {
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
		else if (check_key(toggle_keys.framestep, 0, 0, 0) || (old_framestop = false)) {
			if (old_framestop) {}
			else if (riv.paused) {
				//process_frame(This, riv);
				int ret = (This->*ogBattleMgrOnProcess[ind])();
				if (ret <= 0 || ret >= 4) {
					RefreshCommandInfo(riv.cmdp1, (Player*)&This->leftCharacterManager);
					RefreshCommandInfo(riv.cmdp2, (Player*)&This->rightCharacterManager);
				}

				riv::box::setDirty(true);
			}
			old_framestop = true;//fix
		}

		riv.forwardIndex += riv.forwardStep;
		if (riv.forwardIndex >= riv.forwardCount) {
			for (int i = riv.forwardIndex / riv.forwardCount; i--;) {
				ret = (This->*ogBattleMgrOnProcess[ind])();
				riv::box::setDirty(true);
				if (ret > 0 && ret < 4)
					break;

				RefreshCommandInfo(riv.cmdp1, (Player*)&This->leftCharacterManager);
				RefreshCommandInfo(riv.cmdp2, (Player*)&This->rightCharacterManager);
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
		auto& players = *reinterpret_cast<std::array<Player*, PLAYERS_NUMBER>*>((DWORD)This + 0xC);

		if (players[0]->inputData.inputType != 2) {//wait for refactor
			RenderInputPanel<3>(riv, riv.cmdp1, 60, 340);
			RenderRecordPanel<3>(riv, riv.cmdp1, 0, 300);
		}
		if (players[1]->inputData.inputType != 2) {//wait for refactor
			RenderInputPanel<-3>(riv, riv.cmdp2, 400, 340);
			RenderRecordPanel<-3>(riv, riv.cmdp2, 390, 300);
		}
		//SokuLib::ADDR_RENDERER
		riv::box::setCamera();
		auto manager = GameDataManager::instance;
		for (int i = 0; i<players.size(); ++i) {
			if (!players[i] || manager && !manager->enabledPlayers[i]) continue;
			if (riv.hitboxes && This->matchState > 0) {
				drawPlayerBoxes(*players[i], This->matchState == 1 || This->matchState >= 6, invulMelee[i]<<0 | invulBullet[i]<<1 | invulGrab[i]<<2 | unblockable[i]<<3);
			}
			if (riv.untech) {
				drawUntechBar(*players[i]);
			}
		}
		if (riv.hitboxes) drawFloor();
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
		SokuLib::textureMgr.remove(riv.texID);
		SokuLib::textureMgr.remove(riv::box::Texture_armorBar);
		SokuLib::textureMgr.remove(riv::box::Texture_armorLifebar);
		//text::OnDestruct(This, _, dyn);
		auto path = configPath.c_str();
		WritePrivateProfileStringW(L"Input", L"p1.Enabled", riv.cmdp1.enabled ? L"1" : L"0", path);
		WritePrivateProfileStringW(L"Input", L"p2.Enabled", riv.cmdp2.enabled ? L"1" : L"0", path);
		WritePrivateProfileStringW(L"Record", L"p1.Enabled", riv.cmdp1.record.enabled ? L"1" : L"0", path);
		WritePrivateProfileStringW(L"Record", L"p2.Enabled", riv.cmdp2.record.enabled ? L"1" : L"0", path);
	}
	riv::box::cleanWatcher();
	return (This->*ogBattleMgrDestruct[i])(dyn);
}
TD_INSTANTIATE(0); TD_INSTANTIATE(1); TD_INSTANTIATE(2);


