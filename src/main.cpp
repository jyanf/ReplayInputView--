#include "main.hpp"
#include <SokuLib.hpp>
#include <iostream>

#pragma comment(lib, "Shlwapi.lib")
//#pragma comment(lib, "winmm.lib")
#include "ReplayInputView++/riv.hpp"

const int& battleCounter = *reinterpret_cast<int*>(0x8985d8);
const int& globalTimer = *reinterpret_cast<int*>(0x89FFC4);
const int& globalFPS = *reinterpret_cast<int*>(0x89FFD0);
const int& randomseed = *reinterpret_cast<int*>(0x89b660);

//credit enebe/shady-loader
static bool GetModulePath(HMODULE handle, std::filesystem::path& result) {
	std::wstring buffer;
	int len = MAX_PATH + 1;
	do {
		buffer.resize(len);
		len = GetModuleFileNameW(handle, buffer.data(), buffer.size());
	} while (len > buffer.size());

	if (len) result = std::filesystem::path(buffer.begin(), buffer.end()).parent_path();
	return len;
}
//static bool GetModuleName(HMODULE handle, std::wstring& result) {
//	int len = MAX_PATH + 1;
//	do {
//		result.resize(len);
//		len = GetModuleFileNameW(handle, result.data(), result.size());
//	} while (len > result.size());
//	result.resize(len);
//
//	return len;
//}

static auto orgSokuSetup = reinterpret_cast<bool(__fastcall*)(void*, int, void*)>(0x407970);
static inline std::filesystem::path toAbsPath(const std::filesystem::path& path, const std::filesystem::path& base = basePath) {
	return std::filesystem::weakly_canonical(path.is_absolute() ? path : base / path);
}
static bool __fastcall onSokuSetup(void* self, int unused, void* data) {
	auto& name = iniProxy["Assets"_l]["File"_l];
	std::filesystem::path path = name.value.value;
	path = toAbsPath(path);
	if (std::filesystem::is_regular_file(path))
		try {
			SokuLib::appendDatFile(path.string().c_str());
		} catch(...) {
			const auto msg = "ReplayInputView++: failed to load assets file (.dat).\nPlease check RIV files and .ini settings!";
			MessageBoxA(nullptr, msg, "RIV Error", MB_ICONERROR | MB_OK);
			throw std::runtime_error(msg);
		}
	else {
		const auto msg = "ReplayInputView++: assets file (.dat) not found.\nPlease check RIV file and .ini settings!";
		MessageBoxA(nullptr, msg, "RIV Error", MB_ICONERROR | MB_OK);
		throw std::runtime_error(msg);
	}
	bool ret = orgSokuSetup(self, unused, data);
	gui::xml::_textReader = reinterpret_cast<decltype(gui::xml::_textReader)>(*reinterpret_cast<DWORD*>(0x405853 + 1) + 0x405853 + 5);//use hooked txt reader
	info::Vice::delayedInit();//reader not ready?
	auto& cap_check = *reinterpret_cast<D3DCAPS9*>(0x8a0e74);
	printf("D3D9 Square texture only: %d\n", (cap_check.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY) != 0);
	return ret; 
}

static bool init = false;
HMODULE hDllModule;
std::filesystem::path basePath;

/******************************** DLL export ***********************************/
// We check if the game version is what we target (in our case, Soku 1.10a).
extern "C" __declspec(dllexport) bool Initialize(HMODULE hMyModule, HMODULE hParentModule)
{
	using std::cout, std::cerr, std::endl;
#ifdef _DEBUG
	FILE *_;
	AllocConsole();
	freopen_s(&_, "CONOUT$", "w", stdout);
	freopen_s(&_, "CONOUT$", "w", stderr);
#endif
	hDllModule = hMyModule;

	GetModulePath(hMyModule, basePath);
	cout <<"Absolute Folder Path[" << basePath << "]\n";
	iniProxy.setPath(toAbsPath(iniProxy.getPath()));

	iniProxy.load();//load .ini settings

	gui::Font::LoadAllFontsInFolder(basePath / L"fonts", true);

	cout << "Absolute Ini Path[" << iniProxy.getPath() << "]\n";

	using riv::box::update_collision_shim, riv::box::lag_watcher_updator, riv::SaveTimers;
	using riv::combo_thre_shim, riv::reconsider_counter_show;

	DWORD old;
	/******************************** Hooks ***********************************/
	VirtualProtect((PVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, PAGE_EXECUTE_WRITECOPY, &old); {
		// combo counter show on hit 1
			//0x4792fb 83f8	02			CMP        EAX, 0x2	
			// --> CMP EAX, 0x1
			//0x4792fe 8945	10			MOV        dword ptr[EBP + 0x10], EAX
			//0x479301 7c	07			JL         LAB_0047930a
			//0x479303 c745 18 b4000000 MOV        dword ptr[EBP + 0x18], 0xb4
			//0x47930a 8a81 ad040000	MOV        AL, byte ptr[this->comboDesign + 0x4ad]
		//auto& op = *reinterpret_cast<byte(*)[3]>(0x4792FB);
		//if (op[0] == 0x83 && op[1] == 0xf8 && op[2] == 2) op[2] = 1;
		combo_thre_shim.hook(reconsider_counter_show);

		// to avoid cursor offset, fix wrong fullscreen window pos, by setting pos to 0,0
			//0x415324 03 c3			ADD     EAX, EBX-->  PUSH 0x0
			//0x415326 6a 00			PUSH    0x0
			//0x415328 f7 d8			NEG     EAX		-->  PUSH 0x0
			//0x41532a 50				PUSH    EAX		--> NOP
			//0x41532b a1 84 0f 8a 00	MOV     EAX, [D3DPresent_paramters.hDeviceWindow]
			//0x415330 57				PUSH    EDI		--> NOP
		auto popr = reinterpret_cast<byte*>(0x415324); popr[0] = 0x6A; popr[1] = 0x00;
		popr = reinterpret_cast<byte*>(0x415328); popr[0] = 0x6A; popr[1] = 0x00;
		popr = reinterpret_cast<byte*>(0x41532a); popr[0] = 0x90;
		popr = reinterpret_cast<byte*>(0x415330); popr[0] = 0x90;

		//CBattleManager_UpdateMovement
			//JMP FUN_0046e010; 004796c6: E9 4549FFFF -> CALL
		ogUpdateMovement = SokuLib::TamperNearJmp(0x4796c6, SaveTimers);
		//CBattleManager_UpdateCollision
			//CMP dword ptr [EAX + 0x174],0x0; 0047d2c4: 83B8 74010000 00 -> JMP shim
		update_collision_shim.hook(lag_watcher_updator);

		//Common
		ogBattleMgrSize[0] = SokuLib::TamperDwordAdd(SokuLib::ADDR_BATTLE_MANAGER_SIZE, sizeof(riv::RivControl));
		ogBattleMgrConstruct[0] = SokuLib::TamperNearCall(SokuLib::CBattleManager_Creater, CBattleManager_OnConstruct<0>);
		//Arcade
		ogBattleMgrSize[1] = SokuLib::TamperDwordAdd(0x439d32, sizeof(riv::RivControl));
		ogBattleMgrConstruct[1] = SokuLib::TamperNearCall(0x439d50, CBattleManager_OnConstruct<1>);
		//Story
		ogBattleMgrSize[2] = SokuLib::TamperDwordAdd(0x43c5a3, sizeof(riv::RivControl));
		ogBattleMgrConstruct[2] = SokuLib::TamperNearCall(0x43c5c3, CBattleManager_OnConstruct<2>);//base init
		//Result
		//ogBattleMgrSize[3] = SokuLib::TamperDwordAdd(0x438803, sizeof(riv::RivControl));
		ogBattleMgrConstruct[3] = SokuLib::TamperNearCall(0x438823, CBattleManager_OnConstruct<3>);//base init

		//Hook Setup to load custom .dat
		orgSokuSetup = SokuLib::TamperNearCall(0x7fb871, onSokuSetup);
	} VirtualProtect((PVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, old, &old);

	/******************************** VHooks ***********************************/
	VirtualProtect((PVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, PAGE_WRITECOPY, &old); {
		ogBattleMgrDestruct[0] = SokuLib::TamperDword(&SokuLib::VTable_BattleManager.destructor, CBattleManager_OnDestruct<0>);
		ogBattleMgrOnProcess[0] = SokuLib::TamperDword(&SokuLib::VTable_BattleManager.onProcess, CBattleManager_OnProcess<0>);
		ogBattleMgrOnRender[0] = SokuLib::TamperDword(&SokuLib::VTable_BattleManager.onRender, CBattleManager_OnRender<0>);
	
		ogBattleMgrDestruct[1] = SokuLib::TamperDword((VManagerDestruct*)0x85899c, CBattleManager_OnDestruct<1>);
		ogBattleMgrOnProcess[1] = SokuLib::TamperDword((VManagerOnProcess*)0x8589a8, CBattleManager_OnProcess<1>);
		ogBattleMgrOnRender[1] = SokuLib::TamperDword((VManagerOnRender*)0x8589d4, CBattleManager_OnRender<1>);

		ogBattleMgrDestruct[2] = SokuLib::TamperDword((VManagerDestruct*)0x858934, CBattleManager_OnDestruct<2>);
		ogBattleMgrOnProcess[2] = SokuLib::TamperDword((VManagerOnProcess*)0x858940, CBattleManager_OnProcess<2>);
		ogBattleMgrOnRender[2] = SokuLib::TamperDword((VManagerOnRender*)0x85896c, CBattleManager_OnRender<2>);

		ogSceneBattleRender = SokuLib::TamperDword(SokuLib::union_cast<VBattleRender*>(&SokuLib::VTable_Battle.onRender), info::Vice::CBattle_Render);
	} VirtualProtect((PVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, old, &old);

	reset_swapchain_shim.hook(info::Vice::BeforeD3D9DevReset);//in case no WindowResizer
	reset_swapchain_shim2.hook(info::Vice::AfterD3D9DevReset);

	FlushInstructionCache(GetCurrentProcess(), NULL, 0);
	return true;
}

extern "C" __declspec(dllexport) int getPriority()
{
	return -10000;
}

extern "C" __declspec(dllexport) bool CheckVersion(const BYTE hash[16])
{
	return memcmp(hash, SokuLib::targetHash, sizeof(SokuLib::targetHash)) == 0;
}
extern "C" int APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		info::ScopedActCtx::InitMyActCtx(hModule);
		break;
	case DLL_PROCESS_DETACH:
		info::ScopedActCtx::ShutdownMyActCtx();
		break;
	default:
		break;
	}
	return TRUE;
}
