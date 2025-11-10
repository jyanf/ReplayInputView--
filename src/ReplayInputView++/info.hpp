#pragma once
#include <DrawUtils.hpp>
#include <TextureManager.hpp>
#include <Player.hpp>
#include <Scenes.hpp>
#include <Renderer.hpp>
#include <Design.hpp>
#include <dinput.h>
//#include <ShellScalingApi.h>
#include <windowsx.h>
#include <WinUser.h>
#include <CommCtrl.h>
#include <map>
#include <array>
#include <variant>

#include "../main.hpp"
#include "tex.hpp"
#include "gui.hpp"

#pragma comment(lib, "comctl32.lib")

#define ANCHOR_BORDER_CLAMP

namespace info {
using SokuLib::v2::GameObjectBase;
using SokuLib::v2::GameObject;
using SokuLib::v2::Player;
//using Design = SokuLib::CDesign;
using Design = gui::RivDesign;
	//void createViceWindow();
	//void destroyViceWindow();
	extern const DIMOUSESTATE& sokuDMouse;//mouse data
	extern const LPDIRECT3D9& pd3d;
	extern CRITICAL_SECTION& d3dMutex;
	extern const LPDIRECT3DSWAPCHAIN9& pd3dSwapChain;

	constexpr int fvckSize = 10;
	extern std::array<unsigned char, fvckSize> fvckIchirin;//why do you hook `EndScene()`?! I spent a whole day to find out and fix your shit
	inline void unfvck(LPDIRECT3DDEVICE9& dev) {
		if (!dev || fvckIchirin[0] != 0x90) return;
		auto orgf = reinterpret_cast<void*>((*(DWORD(**))dev)[42]);
		fvckIchirin[0] = TRUE;
		DWORD old; VirtualProtect(orgf, fvckSize, PAGE_EXECUTE_READ, &old);
		memcpy(&fvckIchirin, orgf, fvckSize);
		VirtualProtect(orgf, fvckSize, old, &old);
	}
	inline void fvck(LPDIRECT3DDEVICE9& dev) {
		if (!dev || fvckIchirin[0] == 0x90) return;
		auto& orgf = *reinterpret_cast<std::array<unsigned char, fvckSize>*>((*(DWORD(**))dev)[42]);
		DWORD old; VirtualProtect(&orgf, fvckSize, PAGE_EXECUTE_READWRITE, &old);
		orgf.swap(fvckIchirin);
		VirtualProtect(&orgf, fvckSize, old, &old);
	}
	
	class Interface {
		using Anchor = SokuLib::Vector2i;//screen coordinate
		template<typename T> using map2D = std::map<int, std::multimap<int, const T*>>;
		//map2D<Player> bufPlayer, poolPlayer;
		//map2D<GameObject> bufObject, poolObject;
		int index = -1;
		//using Phover = const GameObjectBase*;
		using PhoverBase = std::variant<std::nullptr_t, const GameObject*, const Player*, const GameObjectBase*>;
		struct Phover : public PhoverBase {
			using PhoverBase::PhoverBase;
			using PhoverBase::operator=;
			inline bool has_value() const { return !std::holds_alternative<std::nullptr_t>(*this); }
			inline bool is_player() const { return std::holds_alternative<const Player*>(*this); }
			inline bool is_object() const { return std::holds_alternative<const GameObject*>(*this); }
			
			inline const GameObjectBase* get_base() const {
				if (std::holds_alternative<std::nullptr_t>(*this))
					return nullptr;
				else if (std::holds_alternative<const GameObjectBase*>(*this))
					return std::get<const GameObjectBase*>(*this);
				else if (std::holds_alternative<const GameObject*>(*this))
					return std::get<const GameObject*>(*this);
				else if (std::holds_alternative<const Player*>(*this))
					return std::get<const Player*>(*this);
				return nullptr;
			}
			inline const Player* get_player() const {
				if (std::holds_alternative<const Player*>(*this))
					return std::get<const Player*>(*this);
				return nullptr;
			}
			inline const GameObject* get_object() const {
				if (std::holds_alternative<const GameObject*>(*this))
					return std::get<const GameObject*>(*this);
				return nullptr;
			}
			inline operator bool() const { return has_value(); }
			inline operator const GameObjectBase* () const { return get_base(); }
			inline operator GameObjectBase* () const { return const_cast<GameObjectBase*>(get_base()); }
			inline const GameObjectBase* operator->() const noexcept { return get_base(); }
			inline const GameObjectBase& operator*() const noexcept { return *get_base(); }
			explicit inline operator DWORD () const noexcept { return DWORD(get_base()); }
			inline std::string tostring() const {
				return std::format("{:#08x}", DWORD(*this));
			}
		};
		std::vector<Phover> hovers;
		int zaccu = 0, ztimer = 0; 
		static constexpr int zthre = WHEEL_DELTA, zcool = 16;
		DWORD oldi = 0;
		std::mutex inserting;
	public:
		constexpr static Anchor toler = { 6, 6 };
		Anchor cursor = toler * -2, cursor2 = toler * -2;
		Phover focus = nullptr;
		//std::variant<const GameObjectBase*, const GameObject*, const Player*> focus;
		Interface(int reserve) {
			hovers.reserve(reserve);
		}
		inline void init() {
			hovers.clear();
			index = -1;
			cursor = cursor2 = toler * -2;
			zaccu = ztimer = 0; oldi = 0;
			focus = nullptr;
		}
		constexpr static int sokuW = 640, sokuH = 480;
		inline static bool checkInWnd(const Anchor& pos) {
			return !(pos.x < 0 || pos.y < 0 || pos.x > sokuW || pos.y > sokuH);
		}
		inline static bool checkHover(const Anchor& c, const Anchor& p) {
			return abs(c.x - p.x) <= toler.x && abs(c.y - p.y) <= toler.y;
		}
		
		inline bool insert(const Phover& o) {
			Anchor pos = o->isGui ? o->position.to<int>() : Anchor {
#ifdef ANCHOR_BORDER_CLAMP
				.x = int(SokuLib::camera.scale * (SokuLib::camera.translate.x + std::clamp(o->position.x, 0.f, 1280.f))),
				.y = int(SokuLib::camera.scale * (SokuLib::camera.translate.y - std::clamp(o->position.y, SokuLib::camera.bottomEdge, 840.f)))
#else
				.x = int(SokuLib::camera.scale * (SokuLib::camera.translate.x + o->position.x)),
				.y = int(SokuLib::camera.scale * (SokuLib::camera.translate.y - o->position.y))
#endif
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
			index = index<0 ? 0 : (index + size + dir) % size;
			if(dir) printf("Hover switched %d of %d.\n", index+1, size);
			return true;

		}
		inline size_t getCount() {
			std::lock_guard lock(inserting);
			return hovers.size();
		}
		inline int getIndex() { return index; }
		inline Phover getHover() {
			if (index >= 0) {
				std::lock_guard lock(inserting);
				if (index >= hovers.size()) index = 0;
				return hovers[index];
			}
			return nullptr;
		}
		inline static bool cursor_refresh(Anchor& cur, POINT raw, HWND hwnd = SokuLib::window, int ww = sokuW, int wh = sokuH) {
			if (!hwnd || !wh || !ww) return false;
			float scaleX, scaleY;
			RECT rt;
			GetClientRect(hwnd, &rt);
			scaleX = (float)rt.right / ww;
			scaleY = (float)rt.bottom / wh;
			raw.x /= scaleX;
			raw.y /= scaleY;
			bool moved = cur.x != raw.x || cur.y != raw.y;
			cur.x = raw.x; cur.y = raw.y;
			return moved;
		}
		bool checkDMouse();
		inline void clear() {
			hovers.clear();
			//index = -1;
		}
		enum IndicatorState {
			Idle = 0, Selected, 
		};
		void drawIndicator(const riv::tex::Vector2f& pos, int timer, IndicatorState state);
	};
	
	class Vice {
		static WNDPROC ogMainWndProc;
		//static HHOOK mainMouseHook, mouseHook;
		static HWND viceWND, tipWND;
		static TOOLINFOW tipINFO;
		//static LPDIRECT3D9 vicePD3;
		//static LPDIRECT3DDEVICE9 vicePD3D;
		static LPDIRECT3DSWAPCHAIN9 viceSwapChain;
		static std::optional<Design> layout;
		inline static bool followMainWnd = false;
		//static Design& layout;
		
		constexpr static UINT WM_VICE_WINDOW_DESTROY = WM_USER + 0x233;
		constexpr static UINT WM_VICE_WINDOW_UPDATE = WM_USER + 0x234;
		constexpr static UINT WM_MAIN_TOGGLECURSOR = WM_USER + 0x235;

		constexpr static int WIDTH = 240, HEIGHT = 980;
	public:
		static std::atomic_bool viceDisplay, dirty;
		static Interface inter; friend class Interface;

		static bool CreateD3D(HWND);
		inline static bool DestroyD3D();
		static bool createWnd();
		inline static DWORD GetTextCodePage() {//credit th123intl
			auto handle = GetModuleHandle("th123intl.dll");
			if (!handle) return 932;
			FARPROC proc = GetProcAddress(handle, "GetTextCodePage");
			if (!proc) return 932;
			return proc();//no FreeMosule cuz it's not LoadLibrary...
		}
		inline static void delayedInit() {
			//static bool inited = false;
			//static std::mutex fvckskipintro;
			static std::once_flag skipintro_initialized;
			std::call_once(skipintro_initialized, []() {
				if (!layout) {
					gui::xml::XmlHelper::gameCP = GetTextCodePage();
					layout.emplace("rivpp/layout.dat", "rivpp/layout_plus.cv0");
				}
			});
		}

		inline static void showCursor(bool show, int id) {
			if (!SokuLib::window) return;
			if (show)
				PostMessageW(SokuLib::window, WM_MAIN_TOGGLECURSOR, 1, id);
			else
				PostMessageW(SokuLib::window, WM_MAIN_TOGGLECURSOR, 0, id);
		}
		inline static void updateWnd() {
			if (!viceWND) return;
			//UpdateWindow(viceWND);
			PostMessageW(viceWND, WM_VICE_WINDOW_UPDATE, 0, 0);
		}
		inline static void destroyWnd() {
			if (!viceWND) return;
			//DestroyWindow(viceWND);
			//viceWND = NULL;
			PostMessageW(viceWND, WM_VICE_WINDOW_DESTROY, 0, 0);
			if (viceDisplay) {
				showCursor(false, 4);
			}
			followMainWnd = false;
		}
		inline static void showWnd(HWND hwnd = viceWND) {
			if (!hwnd) return;
			ShowWindow(hwnd, SW_SHOWNOACTIVATE);
			//SetWindowPos(viceWND, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			dirty = true;
			if (!viceDisplay) showCursor(true, 3);
			viceDisplay = true;
		}
		inline static void hideWnd(HWND hwnd = viceWND) {
			if (!hwnd) return;
			ShowWindow(hwnd, SW_HIDE);
			if (viceDisplay) showCursor(false, 2);
			viceDisplay = false;
		}
		inline static bool toggleWnd() {
			if (!viceWND) {
				return createWnd();
				//Sleep(200);
			}
			if (viceDisplay)
				hideWnd();
			else
				showWnd();
			return viceDisplay;
		}
		static DWORD WINAPI WindowMain(LPVOID params);
		static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
		static LRESULT CALLBACK MainWindowProc(HWND, UINT, WPARAM wParam, LPARAM lParam);
		//static LRESULT CALLBACK WindowMouseHook(int nCode, WPARAM wParam, LPARAM lParam);
		//static LRESULT CALLBACK MainWindowMouseHook(int nCode, WPARAM wParam, LPARAM lParam);

		static void resetViceSP(HWND hwnd, bool resize = false);
		inline static void wndTitle(const std::wstring& title) {
			if (viceWND)
				SetWindowTextW(viceWND, title.c_str());
		}
		static bool InstallHooks(HINSTANCE, HWND);
		static void UninstallHooks(HWND);
		Vice() {
			//createWnd();
			/*Sleep(100);*/
			//hideWnd();
			inter.init();
			delayedInit();
			//if (!layout.has_value())
				//layout.emplace("rivpp/layout.xml");
			//layout->loadResource();
		}
		~Vice() {
			destroyWnd();
			/*if (layout.has_value()) {//loading problem, test only
				layout->clear();
				layout.reset();
			}*/
		}
		static bool __fastcall CBattle_Render(SokuLib::Battle* This);
		static void ResetD3D9Dev();
	};

	void drawObjectHover(GameObjectBase* object, float time);
	
	class ScopedActCtx {
		static HANDLE g_hActCtx;
		ULONG_PTR m_cookie;
		bool m_active;
	public:
		ScopedActCtx() : m_cookie(0), m_active(false) {
			if (g_hActCtx != INVALID_HANDLE_VALUE) {
				if (ActivateActCtx(g_hActCtx, &m_cookie)) m_active = true;
			}
			INITCOMMONCONTROLSEX iccex{ .dwSize = sizeof(iccex), .dwICC = ICC_WIN95_CLASSES };
			InitCommonControlsEx(&iccex);
		}
		~ScopedActCtx() {
			if (m_active) DeactivateActCtx(0, m_cookie);
		}
		// non-copyable
		ScopedActCtx(const ScopedActCtx&) = delete;
		ScopedActCtx& operator=(const ScopedActCtx&) = delete;
		inline static bool InitMyActCtx(HMODULE hdll = hDllModule) {
			if (g_hActCtx != INVALID_HANDLE_VALUE) return true;
#if 1
			auto res= FindResource(hdll, MAKEINTRESOURCE(2), RT_MANIFEST);
			if (!res) {
				printf("FindResource failed: %lu\n", GetLastError());
				return false;
			}
			ACTCTXW act = {
				.cbSize = sizeof(act),
				.dwFlags = ACTCTX_FLAG_HMODULE_VALID | ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID,
				.lpAssemblyDirectory = basePath.c_str(),
				.lpResourceName = MAKEINTRESOURCEW(2),
				.hModule = hdll,
			};
#else
			auto path = (basePath / L"v6.manifest");
			DWORD attr = GetFileAttributesW(path.c_str());
			if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) {
				std::wcerr << L"Manifest file not found:" <<path.c_str() << L"\n";
				return false;
			}
			ACTCTXW act = {
				.cbSize = sizeof(act),
				.dwFlags = 0,
				.lpSource = path.c_str(),
			};
#endif
			g_hActCtx = CreateActCtxW(&act);
			if (g_hActCtx == INVALID_HANDLE_VALUE) {
				printf("CreateActCtx failed: %lu\n", GetLastError());
				return false;
			}
			return true;
		}
		inline static void ShutdownMyActCtx() {
			if (g_hActCtx != INVALID_HANDLE_VALUE) {
				ReleaseActCtx(g_hActCtx);
				g_hActCtx = INVALID_HANDLE_VALUE;
			}
		}
	};
}

typedef bool (SokuLib::Battle::* VBattleRender)();
extern VBattleRender ogSceneBattleRender;
typedef bool (*ResetD3D9Dev)();
extern TrampTamper<5> reset_swapchian_shim;

