#pragma once
#include <SokuLib.hpp>
#include <Shlwapi.h>
#include <filesystem>

extern HMODULE hDllModule;
extern std::filesystem::path configPath, dataPath;
extern const int &battleCounter, &globalTimer, &gameFPS;
//iniBuffer;

template <int oprSize>
class TrampTamper {//credit enebe shady/memory.cpp
	static_assert(oprSize >= 5);
	static const int size = oprSize + 14;
	std::array<byte, size> shim;
	byte &jmpa = shim[size - 5], &calla = shim[3], &opra = shim[9];
	DWORD addr = NULL;

	TrampTamper(const TrampTamper&) = delete;
	TrampTamper& operator=(const TrampTamper&) = delete;
public:
	void hook(void* target){
		if (!addr) return;
		
		DWORD old;
		VirtualProtect((PVOID)addr, oprSize, PAGE_EXECUTE_READWRITE, &old);
		memcpy((void*)&opra, (const void*)addr, oprSize);
		memset((void*)addr, 0x90, oprSize);
		SokuLib::TamperNearJmp(addr, shim.data());
		VirtualProtect((PVOID)addr, oprSize, old, &old);

		SokuLib::TamperNearCall((DWORD)&calla, target);
		SokuLib::TamperNearJmp((DWORD)&jmpa, addr + oprSize);
	}
	TrampTamper(DWORD source) : addr(source) {
		shim.fill(0x90);
		shim[0] = 0x60;// 0;pushad
		shim[1] = 0x8B; shim[2] = 0xC8;// 1;mov eax, ecx
		calla = 0xE8;// 3;call hook
		shim[8] = 0x61;// 8;popad
		jmpa = 0xE9;//size-5;jmp org
	}
	~TrampTamper() {
		if (!addr) return;
		DWORD old;
		VirtualProtect((PVOID)addr, oprSize, PAGE_EXECUTE_READWRITE, &old);
		memcpy((void*)addr, &opra, oprSize);
		VirtualProtect((PVOID)addr, oprSize, old, &old);
	}



};
