#if defined(_WIN32) && defined(_DEBUG)
#include <vld.h>
#endif
//#ifndef _DEBUG
//	#undef printf
//	#define _NO_CRT_STDIO_INLINE

//	extern "C" int printf(const char*, ...);
//#endif
#ifdef _DEBUG
//#define printf(...) printf(__VA_ARGS__)
#else
#include <cstdio>
#undef printf
inline void _wrap_printf(...) { }
#define printf(...)  _wrap_printf(__VA_ARGS__)
#define puts(...) _wrap_printf(__VA_ARGS__)
#endif

#pragma once
#include <Tamper.hpp>
#include <Shlwapi.h>
#include <filesystem>

extern HMODULE hDllModule;
extern std::filesystem::path basePath;
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
	inline void hook(void* target){
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
	inline void restore() {
		if (!addr) return;
		DWORD old;
		VirtualProtect((PVOID)addr, oprSize, PAGE_EXECUTE_READWRITE, &old);
		memcpy((void*)addr, &opra, oprSize);
		VirtualProtect((PVOID)addr, oprSize, old, &old);
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
		restore();
	}



};

#include "config.hpp"
//extern Config<bool, int, std::string, SokuLib::Vector2f> iniBuffer;
using namespace cfg::ex;

#include <array>
template<typename Adapter>
struct MultiField : cfg::_supported_types::ValueBase<bool> {
	std::array<Adapter, PLAYERS_NUMBER> fields;
	MultiField(std::initializer_list<typename Adapter::value_type> s) : Base(true) {
		//assert(s.size() == PLAYERS_NUMBER);
		std::copy(s.begin(), s.begin()+PLAYERS_NUMBER, fields.begin());
	}
	Adapter::value_type& operator[](int i) {
		return fields[i].value;
	}
	void read(const cstring& path, const cstring& section, const cstring& key) override {
		for (int i = 0; i < fields.size(); ++i) {
			char buf[24];
			sprintf(buf, key, i + 1);
			fields[i].read(path, section, buf);
		}
	}
	void write(const cstring& path, const cstring& section, const cstring& key) const override {
		for (int i = 0; i < fields.size(); ++i) {
			char buf[24];
			sprintf(buf, key, i + 1);
			fields[i].write(path, section, buf);
		}
	}
};

inline auto iniProxy = Config{
#ifdef INI_FILENAME
	INI_FILENAME, 
#else
	L"ReplayInputView++.ini",
#endif // INI_FILENAME
	addSection<"Assets">(
		addString<"File">(
	#ifdef DAT_FILENAME
		DAT_FILENAME
	#else
		"ReplayInputView++.dat"
	#endif // DAT_FILENAME
		)
	),
	addSection<"Debug">(addBool<"Enabled">(false)
		,addField<"Hotkey.p%d", MultiField<cfg::_supported_types::Integer>>({
			0x4F, 0x50,
			0x51, 0x4B
		})
		,addInteger<"Hotkey.reset">(0x52)
	),
	addSection<"InputPanel">(
		addField<"p%d.Enabled", MultiField<cfg::_supported_types::Integer>>({
			false, false,
			false, false
		}),
		addField<"p%d.Position", MultiField<cfg::_supported_types::Point>>({
			{60, 340}, {410, 340},
			{60, 360}, {410, 360},
		})
		//addBool<"p1.Enabled">(false), addBool<"p2.Enabled">(false)
		//,addPoint<"p1.Position">({60, 340})
		//,addPoint<"p2.Position">({410, 340})
#ifdef SUIT_4_PLAYERS
		//,addBool<"p3.Enabled">(false), addBool<"p4.Enabled">(false)
		//,addPoint<"p3.Position">({60, 360})
		//,addPoint<"p4.Position">({410, 360})
#endif // SUIT_4_PLAYERS
	),
	addSection<"InputRecord">(
		addField<"p%d.Enabled", MultiField<cfg::_supported_types::Integer>>({
			false, false,
			false, false
		})
		,addField<"p%d.Position", MultiField<cfg::_supported_types::Point>>({
			{0, 300}, {393, 300},
			{0, 280}, {393, 280},
		})
		//addBool<"p1.Enabled">(false), addBool<"p2.Enabled">(false)
		//,addPoint<"p1.Position">({0, 300})
		//,addPoint<"p2.Position">({393, 300})
#ifdef SUIT_4_PLAYERS
		//,addBool<"p3.Enabled">(false), addBool<"p4.Enabled">(false)
		//,addPoint<"p3.Position">({0, 280})
		//,addPoint<"p4.Position">({393, 280})
#endif // SUIT_4_PLAYERS
	),
	addSection<"BoxDisplay">(addBool<"Enabled">(false)
		,addBool<"Floor">(true)
		,addBool<"JuggleMeter">(true)
		,addBool<"ArmorMeter">(true)
		,addBool<"Hitbox.FadeByHitstop">(true)
		,addBool<"StencilTest.Enabled">(true)
			,addBool<"StencilTest.OuterMostOnly">(true)
		,addBool<"TagSoku.CheckPlayerHidden">(false)
		,addField<"p%d.Character", MultiField<cfg::_supported_types::Integer>>({
			true, true,
			true, true
		})
		,addField<"p%d.Bullets", MultiField<cfg::_supported_types::Integer>>({
			true, true,
			true, true
		})
	),

	addSection<"Keys">(
		addInteger<"display_boxes">(0x3E),
		addInteger<"display_info">(0x40),
		addInteger<"display_inputs">(0x41),
		addInteger<"decelerate">(0x43),
		addInteger<"accelerate">(0x44),
		addInteger<"stop">(0x57),
		addInteger<"framestep">(0x58)
	),
	addSection<"FrameRate">(addInteger<"AdjustmentMethod">(1)),
	addSection<"ColorProfile">(//Color(ARGB)
		addInteger<"CollisionBox">(0xFFffff00)//yellow
		,addInteger<"Hitbox.Melee">(0xFFff0000)//red
			,addInteger<"Hitbox.Bullet">(0xFFff0000)
			,addInteger<"Hitbox.Grab">(0xFFf07000)//orange
		,addInteger<"Hurtbox.Character">(0xFF00ff00)//green
			,addInteger<"Hurtbox.Counter">(0xFF00ffff)//cyan
			,addInteger<"Hurtbox.Guard">(0xFFffffff)//white
			,addInteger<"Hurtbox.Parry">(0xFFaa00ff)//purple
				,addInteger<"Hurtbox.Parry.Melee">(0xFFff0000)
				,addInteger<"Hurtbox.Parry.Bullet">(0xFF0000ff)
			,addInteger<"Hurtbox.InvulLine.Melee">(0xFFff0000)
			,addInteger<"Hurtbox.InvulLine.Bullet">(0xFF0000ff)
		,addInteger<"Hurtbox.Object">(0xFF00ff00)//green
			,addInteger<"Hurtbox.Entity">(0xFF00ffff)//cyan
			,addInteger<"Hurtbox.Reflector">(0xFF0000ff)//blue
			,addInteger<"Hurtbox.Gap">(0xFFff00ff)//Magenta
		,addInteger<"FloorBox">(0xFFcccccc)//pale
	),
};