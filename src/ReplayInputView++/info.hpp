#pragma once
#include <DrawUtils.hpp>
#include <TextureManager.hpp>
#include <Player.hpp>
#include <dinput.h>
//#include <ShellScalingApi.h>
#include <windowsx.h>
#include <map>
#include "../main.hpp"


namespace info {
using SokuLib::v2::GameObjectBase;
using SokuLib::v2::GameObject;
using SokuLib::v2::Player;
	//void createViceWindow();
	//void destroyViceWindow();
	extern const DIMOUSESTATE& sokuDMouse;//mouse data
	
	class Interface {
		using Anchor = SokuLib::Vector2i;//screen coordinate
		template<typename T> using map2D = std::map<int, std::multimap<int, const T*>>;
		//map2D<Player> bufPlayer, poolPlayer;
		//map2D<GameObject> bufObject, poolObject;
		int index = -1;
		std::vector<const GameObjectBase*> hovers;
		std::mutex inserting;
	public:
		constexpr static int toler = 5;
		Anchor cursor = { -1, -1 };
		const GameObjectBase* focus = nullptr;
		Interface(int reserve) {
			hovers.reserve(reserve);
		}
		constexpr static int sokuW = 640, sokuH = 480;
		inline static bool checkInWnd(const Anchor& pos) {
			return !(pos.x < 0 || pos.y < 0 || pos.x > sokuW || pos.y > sokuH);
		}
		inline static bool checkHover(const Anchor& c, const Anchor& p) {
			return abs(c.x - p.x) <= toler && abs(c.y - p.y) <= toler;
		}
		
		inline bool insert(const GameObjectBase* o) {
			Anchor pos = {
				SokuLib::camera.scale * (SokuLib::camera.translate.x + o->position.x),
				SokuLib::camera.scale * (SokuLib::camera.translate.y - o->position.y)
			};
			if (!checkInWnd(pos) || !checkHover(pos, cursor))
				return false;
			std::lock_guard lock(inserting);
			//auto& buf = isPlayer ? bufPlayer : bufObject;
			hovers.emplace_back(o);
			return true;
		}
		//void updateHover();
		inline bool switchHover(int dir) {
			if (hovers.empty()) {
				index = -1; //focus = nullptr;
				return false;
			}
			const auto size = getCount();
			index = (index + size + dir) % size;
			return true;

		}
		inline size_t getCount() {
			std::lock_guard lock(inserting);
			return hovers.size();
		}
		inline int getIndex() { return index; }
		inline const GameObjectBase* getHover() {
			if (index >= 0) {
				std::lock_guard lock(inserting);
				return hovers[index];
			}
			return nullptr;
		}
		inline bool checkDMouse() {
			auto old = focus;
			if (sokuDMouse.rgbButtons[0]) {
				focus = getHover();
				return old != focus;
			}
			if (sokuDMouse.rgbButtons[1] && focus) {
				focus = nullptr;
				return old != focus;
			}
			if (abs(sokuDMouse.lZ) >= WHEEL_DELTA) {
				return switchHover(sokuDMouse.lZ > 0 ? 1 : -1);
			}
			return false;
		}
		/*inline void flush() {
			bufPlayer.swap(poolPlayer);
			map2D<Player>().swap(bufPlayer);
			bufObject.swap(poolObject);
			map2D<GameObject>().swap(bufObject);
		}*/
		inline void clear() {
			hovers.clear();
			index = -1;
		}

	};
	
	class Vice {
		//static HANDLE viceThread;
		static HHOOK mainHook, mouseHook;

		//static bool registered;
		static HWND viceWND;
		static LPDIRECT3D9 vicePD3;
		static LPDIRECT3DDEVICE9 vicePD3D;
		constexpr static UINT WM_VICE_WINDOW_DESTROY = WM_USER + 0x233;
		constexpr static UINT WM_VICE_WINDOW_UPDATE = WM_USER + 0x234;

		constexpr static int WIDTH = 200, HEIGHT = 500;
	public:
		static std::atomic_bool viceDisplay, dirty;
		static Interface inter;

		void updateWnd();
		static bool createWnd();
		static void destroyWnd();
		static bool CreateD3D(HWND&);
		inline static bool DestroyD3D();
		inline static void showWnd(HWND hwnd = viceWND) {
			if (!hwnd) return;
			ShowWindow(hwnd, SW_SHOWNOACTIVATE);
			//SetWindowPos(viceWND, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			viceDisplay = true;
		}
		inline static void hideWnd(HWND hwnd = viceWND) {
			if (!hwnd) return;
			ShowWindow(hwnd, SW_HIDE);
			viceDisplay = false;
		}
		inline static bool toggleWnd() {
			if (!viceWND) {
				createWnd();
				//Sleep(200);
				return true;
			}
			if (viceDisplay)
				hideWnd();
			else
				showWnd();
			return viceDisplay;
		}
		static DWORD WINAPI WindowMain(LPVOID params);
		static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
		static LRESULT CALLBACK MainWindowProcHook(int nCode, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK MainWindowMouseHook(int nCode, WPARAM wParam, LPARAM lParam);
		static bool InstallHooks(HINSTANCE hInstance, HWND hwnd);
		static void UnsintallHooks();
		Vice() {
			//createWnd();
			/*Sleep(100);*/
			//hideWnd();
			inter.cursor = { -1, -1 };
			inter.focus = nullptr;
		}
		~Vice() {
			destroyWnd();
		}
	};



}