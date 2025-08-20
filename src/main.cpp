//
// Created by PinkySmile on 31/10/2020
//

#include <SokuLib.hpp>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#include "ReplayInputView++/riv.hpp"

static bool init = false;

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


/******************************** Hooks ***********************************/


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


	DWORD old;
	VirtualProtect((PVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, PAGE_EXECUTE_WRITECOPY, &old);
	ogBattleMgrSize = SokuLib::TamperDwordAdd(SokuLib::ADDR_BATTLE_MANAGER_SIZE, sizeof(riv::RivControl));
	ogBattleMgrInit = SokuLib::TamperNearCall(SokuLib::CBattleManager_Creater, CBattleManager_OnCreate);

	// combo counter show on hit 1
	auto& op = *reinterpret_cast<byte(*)[3]>(0x4792FB);
	if (op[0] == 0x83 && op[1] == 0xf8 && op[2] == 2) op[2] = 1;

	//CBattleManager_UpdateMovement
		//JMP FUN_0046e010; 004796c6: E9 4549FFFF
		//-> CALL
	ogUpdateMovement = SokuLib::TamperNearJmp(0x4796c6, SaveTimers);

	//CBattleManager_UpdateCollision
		//CMP dword ptr [EAX + 0x174],0x0; 0047d2c4: 83B8 74010000 00
		//->JMP shim
	using riv::box::update_collision_shim, riv::box::lag_watcher_updator;
	memcpy(update_collision_shim + 9, (void*)0x47d2c4, 7);//more check?
	memset((void*)0x47d2c4, 0x90, 7);
	SokuLib::TamperNearJmp(0x47d2c4, update_collision_shim);
	SokuLib::TamperNearCall((DWORD)update_collision_shim + 3, lag_watcher_updator);
	SokuLib::TamperNearJmp((DWORD)update_collision_shim + 16, 0x47d2c4 + 7);

	VirtualProtect((PVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, old, &old);

	VirtualProtect((PVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, PAGE_WRITECOPY, &old);
	ogBattleMgrDestruct = SokuLib::TamperDword(&SokuLib::VTable_BattleManager.destructor, CBattleManager_OnDestruct);
	ogBattleMgrOnRender = SokuLib::TamperDword(&SokuLib::VTable_BattleManager.onRender, CBattleManager_OnRender);
	ogBattleMgrOnProcess = SokuLib::TamperDword(&SokuLib::VTable_BattleManager.onProcess, CBattleManager_OnProcess);
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
