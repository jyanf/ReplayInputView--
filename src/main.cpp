//
// Created by PinkySmile on 31/10/2020
//

#include <SokuLib.hpp>
//#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#include "main.hpp"
#include "ReplayInputView++/riv.hpp"

static bool init = false;
HMODULE hDllModule;
std::filesystem::path configPath;

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




/******************************** DLL export ***********************************/
// We check if the game version is what we target (in our case, Soku 1.10a).
extern "C" __declspec(dllexport) bool CheckVersion(const BYTE hash[16])
{
	return memcmp(hash, SokuLib::targetHash, sizeof(SokuLib::targetHash)) == 0;
}
extern "C" __declspec(dllexport) bool Initialize(HMODULE hMyModule, HMODULE hParentModule)
{
#ifdef _DEBUG
	FILE *_;
	AllocConsole();
	freopen_s(&_, "CONOUT$", "w", stdout);
	freopen_s(&_, "CONOUT$", "w", stderr);
#endif
	hDllModule = hMyModule;

	GetModulePath(hMyModule, configPath);
#ifdef INI_FILENAME
	configPath /= INI_FILENAME;
#else
	configPath /= L"ReplayInputView++.ini";
#endif // INI_FILENAME

	using riv::box::update_collision_shim, riv::box::lag_watcher_updator, riv::SaveTimers;

	DWORD old;
	/******************************** Hooks ***********************************/
	VirtualProtect((PVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, PAGE_EXECUTE_WRITECOPY, &old);
	// combo counter show on hit 1
	auto& op = *reinterpret_cast<byte(*)[3]>(0x4792FB);
	if (op[0] == 0x83 && op[1] == 0xf8 && op[2] == 2) op[2] = 1;

	//CBattleManager_UpdateMovement
		//JMP FUN_0046e010; 004796c6: E9 4549FFFF -> CALL
	ogUpdateMovement = SokuLib::TamperNearJmp(0x4796c6, SaveTimers);
	//CBattleManager_UpdateCollision
		//CMP dword ptr [EAX + 0x174],0x0; 0047d2c4: 83B8 74010000 00 -> JMP shim
	memcpy(update_collision_shim + 9, (void*)0x47d2c4, 7);//more check?
	memset((void*)0x47d2c4, 0x90, 7);
	SokuLib::TamperNearJmp(0x47d2c4, update_collision_shim);
	SokuLib::TamperNearCall((DWORD)update_collision_shim + 3, lag_watcher_updator);
	SokuLib::TamperNearJmp((DWORD)update_collision_shim + 16, 0x47d2c4 + 7);

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
	VirtualProtect((PVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, old, &old);
	/******************************** VHooks ***********************************/
	VirtualProtect((PVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, PAGE_WRITECOPY, &old);
	ogBattleMgrDestruct[0] = SokuLib::TamperDword(&SokuLib::VTable_BattleManager.destructor, CBattleManager_OnDestruct<0>);
	ogBattleMgrOnProcess[0] = SokuLib::TamperDword(&SokuLib::VTable_BattleManager.onProcess, CBattleManager_OnProcess<0>);
	ogBattleMgrOnRender[0] = SokuLib::TamperDword(&SokuLib::VTable_BattleManager.onRender, CBattleManager_OnRender<0>);
	
	ogBattleMgrDestruct[1] = SokuLib::TamperDword((VManagerDestruct*)0x85899c, CBattleManager_OnDestruct<1>);
	ogBattleMgrOnProcess[1] = SokuLib::TamperDword((VManagerOnProcess*)0x8589a8, CBattleManager_OnProcess<1>);
	ogBattleMgrOnRender[1] = SokuLib::TamperDword((VManagerOnRender*)0x8589d4, CBattleManager_OnRender<1>);

	ogBattleMgrDestruct[2] = SokuLib::TamperDword((VManagerDestruct*)0x858934, CBattleManager_OnDestruct<2>);
	ogBattleMgrOnProcess[2] = SokuLib::TamperDword((VManagerOnProcess*)0x858940, CBattleManager_OnProcess<2>);
	ogBattleMgrOnRender[2] = SokuLib::TamperDword((VManagerOnRender*)0x85896c, CBattleManager_OnRender<2>);
	VirtualProtect((PVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, old, &old);

	FlushInstructionCache(GetCurrentProcess(), NULL, 0);
	return true;
}

extern "C" int APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
{
	return TRUE;
}

extern "C" __declspec(dllexport) int getPriority()
{
	return -10000;
}
