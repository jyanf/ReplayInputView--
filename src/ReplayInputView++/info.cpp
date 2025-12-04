#include "info.hpp"
#include "DrawCircle.hpp"
#include <optional>
#include <string>

#include <GameData.hpp>

VBattleRender ogSceneBattleRender{};

namespace info {
    std::array<unsigned char, fvckSize> fvckIchirin{ 0x90 };

    const DIMOUSESTATE& sokuDMouse = *(LPDIMOUSESTATE)(0x8a019c + 0x140);
    const LPDIRECT3D9& pd3d = *(LPDIRECT3D9*)(0x8A0E2C);
    CRITICAL_SECTION& d3dMutex = *reinterpret_cast<CRITICAL_SECTION*>(0x8A0E14);
	const LPDIRECT3DSWAPCHAIN9& pd3dSwapChain = *(LPDIRECT3DSWAPCHAIN9*)(0x8A0E34);
    using riv::tex::d3dpp;

	
    WNDPROC Vice::ogMainWndProc = NULL;
    //HHOOK Vice::mainMouseHook = NULL, Vice::mouseHook = NULL;
    //bool Vice::registered = false;
    HWND Vice::viceWND = NULL, Vice::tipWND = NULL;
    TOOLINFOW Vice::tipINFO {
        //.cbSize = sizeof TOOLINFOW,
    };
    //LPDIRECT3D9 Vice::vicePD3 = NULL;
    //LPDIRECT3DDEVICE9 Vice::vicePD3D = NULL;
	LPDIRECT3DSWAPCHAIN9 Vice::viceSwapChain = NULL;
    LPDIRECT3DSURFACE9 Vice::viceDepthStencil = NULL;
	std::optional<Design> Vice::layout;//delay Design construct to avoid soku runtime dependency, which is not considered by archaic swrstoys loader
    //Design& Vice::layout = *_layout;
    std::atomic_bool Vice::viceDisplay = false, Vice::dirty = false;

	Interface Vice::inter(10);//hovers reserve

struct ViceThreadParams {
    HANDLE hMainThread;
    HWND hParentWnd;
    HINSTANCE hInstance;
	int width, height;
};
    bool Vice::createWnd() {
        if (!d3dpp.Windowed) {
            return false;
        }
        HANDLE duplicatedHandle = NULL;
        // 复制伪句柄为可跨线程的有效句柄
        if (!DuplicateHandle(
            GetCurrentProcess(),  // 源进程
            GetCurrentThread(),          // 源句柄（伪句柄）
            GetCurrentProcess(),  // 目标进程（同一进程）
            &duplicatedHandle,  // 目标句柄（输出）
            0,                    // 访问权限（仅等待用，无需特殊权限）
            FALSE,                // 不可继承
            DUPLICATE_SAME_ACCESS  // 相同访问权限
        )) {
            MessageBoxW(NULL, L"创建副窗口失败！", L"错误", MB_ICONERROR);
            return false;
        }
        //Design::Object* rect = nullptr;
        //layout->getById(&rect, 1);
        auto viceParam = new ViceThreadParams{
			.hMainThread = duplicatedHandle,
            .hParentWnd = SokuLib::window,
            .hInstance = hDllModule,
            .width = layout ? layout->windowSize.x : WIDTH,//rect ? int(rect->x2) : WIDTH,
            .height = layout ? layout->windowSize.y : HEIGHT,//rect ? int(rect->y2) : HEIGHT,
        };
        HANDLE hViceThread = CreateThread(
            NULL,               // 默认安全属性
            0,                  // 默认栈大小
            WindowMain,     // 副线程函数
            viceParam,         // 传递给副线程的参数
            0,                  // 立即运行
            NULL                // 不获取线程ID
        );
        if (hViceThread == NULL) {
            CloseHandle(duplicatedHandle);  // 释放复制的句柄
            MessageBoxW(NULL, L"副窗口`CreateThread`失败！", L"错误", MB_ICONERROR);
            return false;
        }
		CloseHandle(hViceThread); // 不需要保留线程句柄
		followMainWnd = true;
		return true;
	}
    bool Vice::CreateD3D(HWND hwnd) {
        if (!d3dpp.Windowed) {
            //MessageBoxW(NULL, L"当前为全屏模式，无法创建Direct3D设备！", L"错误", MB_ICONERROR);
			//hideWnd(hwnd);
            return false;
		}
        if (hwnd && layout && !viceSwapChain) {
            D3DPRESENT_PARAMETERS d3dpp{ 0 };
            d3dpp.Windowed = TRUE;
            d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;

            /*D3DDISPLAYMODE mode{0};
            if (FAILED(pd3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode))) {
                MessageBoxW(NULL, L"`GetAdapterDisplayMode`失败！", L"错误", MB_ICONERROR);
                return false;
            }*/
            d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;//mode.Format;
            d3dpp.hDeviceWindow = hwnd;

            d3dpp.BackBufferWidth = layout->windowSize.x;
            d3dpp.BackBufferHeight = layout->windowSize.y;
            d3dpp.EnableAutoDepthStencil = 1;
            //auto SokuBackBufferFormat = *(D3DFORMAT*)0x8a0fac;
            d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
            d3dpp.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
            d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;// no vsync?

            if (FAILED(SokuLib::pd3dDev->CreateAdditionalSwapChain(&d3dpp, &viceSwapChain)) || !viceSwapChain) {
                MessageBoxW(NULL, L"`CreateAdditionalSwapChain`失败！", L"错误", MB_ICONERROR);
                return false;
            }
            if (viceDepthStencil) {
                viceDepthStencil->Release();
                viceDepthStencil = NULL;
			}
            SokuLib::pd3dDev->CreateDepthStencilSurface(
                d3dpp.BackBufferWidth,
                d3dpp.BackBufferHeight,
                d3dpp.AutoDepthStencilFormat,
                D3DMULTISAMPLE_NONE,
                0,
                TRUE,
                &viceDepthStencil,
                nullptr
            );
            
            /*
            if (!vicePD3) {
                vicePD3 = Direct3DCreate9(D3D_SDK_VERSION);
                if (!vicePD3) return false;
            }
            if (!vicePD3D) {
                D3DPRESENT_PARAMETERS d3dpp = {
                    .SwapEffect = D3DSWAPEFFECT_DISCARD,
                    .hDeviceWindow = viceWND,
                    .Windowed = TRUE,
                };
                if (!SUCCEEDED(vicePD3->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                    D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &vicePD3D)))
                {
                    //DestroyWindow(hwnd);
                    vicePD3->Release(); vicePD3 = NULL;
                    return false;
                }
            }*/
			return true;
        }
		return true;
    }
    bool Vice::DestroyD3D()
    {
        bool t = true;
        if (viceSwapChain) {
            viceSwapChain->Release();
			viceSwapChain = NULL;
        } else t = false;
        if (viceDepthStencil) {
            viceDepthStencil->Release();
            viceDepthStencil = NULL;
        }
        /*
        if (vicePD3D) {
            vicePD3D->Release();
            vicePD3D = NULL;
        } else t = false;

        if (vicePD3) {
            vicePD3->Release();
            vicePD3 = NULL;
        } else t = false;
        */
        return t;
    }
    static auto adjust_window_size(SIZE org, LONG style) {
        RECT rc = { 0, 0, org.cx, org.cy };
        AdjustWindowRectEx(&rc, style, FALSE, WS_EX_TOOLWINDOW);
		return SIZE{ rc.right - rc.left, rc.bottom - rc.top };
    }
    HANDLE ScopedActCtx::g_hActCtx = INVALID_HANDLE_VALUE;
constexpr auto VICE_CLASSNAME = L"SokuDbgInfoPanel";
    DWORD WINAPI Vice::WindowMain(LPVOID pParams) {
        //SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        using DpiFunc = HRESULT(WINAPI* )(HANDLE);
        HMODULE hUser32 = LoadLibraryW(L"user32.dll");
        if (hUser32) {
            auto pFunc = (DpiFunc)GetProcAddress(hUser32, "SetThreadDpiAwarenessContext");
            if (pFunc) {
                pFunc(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            }
            FreeLibrary(hUser32);
        }

        auto params = *reinterpret_cast<ViceThreadParams*>(pParams);
        delete pParams;
        MSG msg = { 0 };
        HANDLE waitHandles[] = { params.hMainThread };  // 等待的对象：主线程句柄
        DWORD waitCount = _countof(waitHandles);
        WNDCLASSEXW wc = {
            .cbSize = sizeof(WNDCLASSEX),
            .lpfnWndProc = WindowProc,
            .hInstance = params.hInstance,
            //.hbrBackground = CreateSolidBrush(RGB(255, 0, 0)),
            .lpszClassName = VICE_CLASSNAME,
        };
        //WM_VICE_WINDOW_DESTROY = RegisterWindowMessageW(L"WM_VICE_WINDOW_DESTROY");
        RegisterClassExW(&wc);
        //if (!viceWND) {
        RECT gameRect;
        GetWindowRect(params.hParentWnd, &gameRect);
		SIZE viceSize = { params.width, params.height };
        auto style = WS_OVERLAPPEDWINDOW 
            //& ~WS_THICKFRAME 
            & ~WS_MAXIMIZEBOX 
			& ~WS_MINIMIZEBOX;
		viceSize = adjust_window_size(viceSize, style);
		printf("Creating Vice Window of (%d,%d) adjusted size (%d, %d)\n", params.width, params.height, viceSize.cx, viceSize.cy);
        viceWND = CreateWindowExW(
            WS_EX_TOOLWINDOW,
            VICE_CLASSNAME,
            L"Defaulting to Players: ",
            style,
            gameRect.right, gameRect.top,  // 位置跟随游戏窗口右侧
            viceSize.cx, viceSize.cy,
            params.hParentWnd,
            NULL,
            params.hInstance,
            &params
        );
        if (!viceWND) goto ExitLoop;
        if (!InstallHooks(params.hInstance, params.hParentWnd)) {
            UninstallHooks(params.hParentWnd);
            goto ExitLoop;
        }

        showWnd(viceWND);
        //ShowWindow(viceWND, SW_SHOWNOACTIVATE);
        //UpdateWindow(viceWND);
        //viceDisplay = true;
        //showCursor(true, 1);

        while (true) {
            // 等待：要么主线程结束（句柄有信号），要么有窗口消息到来（返回WAIT_OBJECT_0 + 1）
            DWORD waitResult = MsgWaitForMultipleObjects(
                waitCount,
                waitHandles,
                FALSE,        // 不等待所有对象，任一有信号即返回
                INFINITE,     // 无限等待（直到主线程结束或有消息）
                QS_ALLINPUT   // 等待所有类型的输入消息
            );

            switch (waitResult) {
                // 情况1：主线程结束（等待对象0有信号）
            case WAIT_OBJECT_0:
                DestroyWindow(viceWND);
                goto ExitLoop;

                // 情况2：有窗口消息到来
            case WAIT_OBJECT_0 + 1:
                // 处理消息队列（非阻塞，避免主线程结束时无法响应）
                while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                break;

                // 情况3：等待失败（如句柄无效）
            default:
                goto ExitLoop;
            }
        }

    ExitLoop:
        viceWND = NULL;                        // 标记窗口已销毁
        UnregisterClassW(VICE_CLASSNAME, params.hInstance);  // 注销窗口类
        CloseHandle(params.hMainThread);                        // 释放主线程句柄
        return 0;
    }
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
    namespace {
        template<typename Ch>
        struct _char_traits { static constexpr UINT val = 0; };
        template<> struct _char_traits<char> { static constexpr UINT val = CF_TEXT; };
        template<> struct _char_traits<char16_t> { static constexpr UINT val = CF_UNICODETEXT; };

        template<typename In> struct _enum_format { static constexpr UINT val = _char_traits<void>::val; };
        template<typename Ch> struct _enum_format<std::basic_string<Ch>> {
            //using T = std::basic_string<Ch>;
            static constexpr UINT val = _char_traits<Ch>::val;
            template<typename T> static auto get_size(const T& t) noexcept { return (t.length() + 1) * sizeof(Ch); }
            template<typename T> static const void* get_data(const T& t) noexcept { return t.data(); }
        };
        template<typename Ch> struct _enum_format<std::basic_string_view<Ch>> : _enum_format<std::basic_string<Ch>> {
            /*using T = std::basic_string_view<Ch>;
            static constexpr UINT val = _char_traits<Ch>::val;
            template<typename T> static auto get_size(const T& t) noexcept { return (t.length() + 1) * sizeof(Ch); }
            template<typename T> static const void* get_data(const T& t) noexcept { return t.c_str(); }*/
        };
        template<typename Ch> struct _enum_format<Ch*> {
            static constexpr UINT val = _char_traits<Ch>::val;
            static auto get_size(const Ch* t) { return (std::char_traits<Ch>::length(t) + 1) * sizeof(Ch); }
            static const void* get_data(const Ch* t) { return t; }
        };
        template<typename Ch, size_t N> struct _enum_format<Ch[N]> {
            static constexpr UINT val = _char_traits<Ch>::val;
            static constexpr auto get_size(const Ch(&)[N]) { return N * sizeof(Ch); }
            static const void* get_data(const Ch(&t)[N]) { return t; }
        };

#if WCHAR_MAX == 0xFFFF
        template<> struct _char_traits<wchar_t> { static constexpr UINT val = CF_UNICODETEXT; };
        //template<> struct _char_traits<char8_t> { static constexpr UINT val = CF_UNICODETEXT; };
        template<> struct _enum_format<char8_t*> {
            using T = const char8_t*;
            static constexpr UINT val = CF_UNICODETEXT;
            std::wstring buf;
            /*_enum_format(const std::u8string& t) {
                buf.resize(t.length());
                std::mbstowcs(buf.data(), reinterpret_cast<const char*>(t.c_str()), t.length());*/
            inline void convert(T s) {
                if (!buf.empty()) return;
                int len = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)s, -1, nullptr, 0);
                buf.resize(len, L'\0');
                MultiByteToWideChar(CP_UTF8, 0, (LPCCH)s, -1, buf.data(), len);
            }
            auto get_size(T t) {
                convert(t);
                return (buf.length() + 1) * sizeof(wchar_t);
            }
            const void* get_data(T t) {
                convert(t);
                return buf.c_str();
            }
        };
        template<size_t N> struct _enum_format<char8_t[N]> : _enum_format<char8_t*> {};
        template<> struct _enum_format<std::u8string> : _enum_format<char8_t*> {
			//using T = std::u8string;
            template<typename T> auto get_size(const T& t) {
				convert(t.data());
                return (buf.length() + 1) * sizeof(wchar_t);
            }
            template<typename T> const void* get_data(const T& t) {
                convert(t.data());
				return buf.c_str();
            }
        };
        template<> struct _enum_format<std::u8string_view> : _enum_format<std::u8string> {};

        template<> struct _enum_format<char32_t*> {
            using T = const char32_t*;
            static constexpr UINT val = CF_UNICODETEXT;
            std::wstring buf;
            inline void convert(T s) {
                if (!buf.empty()) return;
                auto sv = std::u32string_view(s);
                for (char32_t c : sv) {
                    if (c <= 0xFFFF) {
                        buf.push_back(static_cast<wchar_t>(c));
                    }
                    else {
                        // surrogate pair
                        c -= 0x10000;
                        wchar_t high = static_cast<wchar_t>(0xD800 + ((c >> 10) & 0x3FF));
                        wchar_t low = static_cast<wchar_t>(0xDC00 + (c & 0x3FF));
                        buf.push_back(high);
                        buf.push_back(low);
                    }
                }
                buf.push_back(L'\0');
            }
            auto get_size(T t) {
                convert(t);
                return (buf.length() + 1) * sizeof(wchar_t);
            }
            const void* get_data(T t) {
                convert(t);
                return buf.c_str();
            }
        };
        template<size_t N> struct _enum_format<char32_t[N]> : _enum_format<char32_t*> {};
        template<> struct _enum_format<std::u32string> : _enum_format<char32_t*> {
            template<typename T> auto get_size(const T& t) {
                convert(t.data());
                return (buf.length() + 1) * sizeof(wchar_t);
            }
            template<typename T> const void* get_data(const T& t) {
                convert(t.data());
                return buf.c_str();
            }
        };
        template<> struct _enum_format<std::u32string_view> : _enum_format<std::u32string> {};
        


#elif (WCHAR_MAX == 0x7FFFFFFF || WCHAR_MAX == 0xFFFFFFFF)
        
#endif
        template<typename T>
		struct remove_cvcref {
            using type = std::remove_cvref_t<T>;
        };
		template<typename T>
        struct  remove_cvcref<const T*> {
            using type = remove_cvcref<T*>::type;
        };
        template<typename T> using remove_cvcref_t = typename remove_cvcref<T>::type;
        template<typename In> static void WriteToClipboard(HWND hWnd, const In& text) {
            //using T = std::remove_cvref_t<In>;
			using T = remove_cvcref_t<In>;
            //using C = _enum_format<T>;
			auto c = _enum_format<T>{};
            constexpr auto fmt = c.val;//C::val;
            static_assert(fmt, "Unsupported text type for clipboard");
            printf("Copied to clipboard! ACP: %u\n", GetACP());
            if (!OpenClipboard(hWnd)) return;
            
            EmptyClipboard();
            size_t size = c.get_size(text);//C::get_size(text);
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
            if (hMem)
            {
                void* dst = GlobalLock(hMem);
                if (dst)
                {
                    memcpy(dst, c.get_data(text), size);
                    GlobalUnlock(hMem);
                    SetClipboardData(fmt, hMem);
                }
                else
                {
                    GlobalFree(hMem);
                }
            }
            CloseClipboard();
        }
    }
    namespace { constexpr int bufsz = 256; }
    inline static bool BeginMouseLeaveTracking(HWND hwnd) {
        TRACKMOUSEEVENT tme = { sizeof(tme) };
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        return TrackMouseEvent(&tme);
    }
    LRESULT CALLBACK Vice::WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
		//return DefWindowProcW(hwnd, msg, wp, lp);
        thread_local static bool tracked = false;
        switch (msg) {
        case WM_CREATE: {
			resetViceSP(hwnd, true);
            CreateD3D(hwnd);
            //tipWND creation
            ScopedActCtx::InitMyActCtx();
            ScopedActCtx actCtx;
            auto& params = *reinterpret_cast<ViceThreadParams*>(LPCREATESTRUCT(lp)->lpCreateParams);
            tipWND = CreateWindowExW(
                WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW 
                | WS_EX_TRANSPARENT,//credit https://zhuanlan.zhihu.com/p/1925854171443208696
                TOOLTIPS_CLASSW,
                L"SokuDbgTooltip",
                WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_BALLOON,
                CW_USEDEFAULT, CW_USEDEFAULT,
                CW_USEDEFAULT, CW_USEDEFAULT,
                hwnd,
                NULL,
                params.hInstance,
                NULL
            );
            if (!tipWND) {
                printf("CreateWindowExW(TOOLTIPS) failed: %lu\n", GetLastError());
                return 0;
            }
            //static thread_local WCHAR tipBuf[256];
            //wcscpy_s(tipBuf, L"This is a floating tip\n这是一个浮动说明");
            tipINFO.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_TRANSPARENT | TTF_CENTERTIP;
            tipINFO.hwnd = hwnd;                 // 被监控窗口（viceWND）
            tipINFO.uId = (UINT_PTR)hwnd;        // 用 HWND 做 id
            tipINFO.hinst = NULL;                // NULL 表示 lpszText 是内存字符串
            //ti.lpszText = tipBuf;

            constexpr size_t sz[] = { TTTOOLINFOW_V2_SIZE, TTTOOLINFOW_V3_SIZE, TTTOOLINFOW_V1_SIZE, sizeof(tipINFO)};
            for (auto s : sz) {
                tipINFO.cbSize = s;
                SetLastError(0);
                LRESULT addRes = SendMessageW(tipWND, TTM_ADDTOOLW, 0, (LPARAM)&tipINFO);
                printf("Try cbsize %zu: TTM_ADDTOOLW=%ld, GetLastError=%lu, toolcount=%ld\n", 
                    s, addRes, GetLastError(), SendMessageW(tipWND, TTM_GETTOOLCOUNT, 0, 0));
                if (addRes) {
                    SendMessageW(tipWND, TTM_SETMAXTIPWIDTH, 0, params.width);
                    SendMessageW(tipWND, TTM_SETDELAYTIME, TTDT_AUTOMATIC, 50);
                    break;
                }
			}
            //LRESULT addRes = SendMessageW(tipWND, TTM_ADDTOOLW, 0, (LPARAM)&tipINFO);
            //DWORD err = GetLastError();
            //LRESULT count = SendMessageW(tipWND, TTM_GETTOOLCOUNT, 0, 0);
            //printf("Try IDISHWND: TTM_ADDTOOLW=%ld, GetLastError=%lu, toolcount=%ld\n", addRes, err, count);
            // 激活并置顶，尝试可见性
            //SendMessageW(tipWND, TTM_ACTIVATE, TRUE, 0);
            //ShowWindow(tipWND, SW_SHOWNOACTIVATE);
            //SetWindowPos(tipWND, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
            break;
        }
        
        // 拦截焦点相关消息
        case WM_SETFOCUS:
        case WM_ACTIVATE: case WM_ACTIVATEAPP:
            return 0;
        // 拦截输入
        //case WM_LBUTTONUP: 
        case WM_RBUTTONUP: case WM_MBUTTONUP:
        case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
        //case WM_NCRBUTTONUP:
        case WM_NCRBUTTONDOWN: 
        //case WM_NCLBUTTONDOWN:
        //case WM_NCLBUTTONDBLCLK:
        case WM_LBUTTONDBLCLK: case WM_NCRBUTTONDBLCLK: case WM_RBUTTONDBLCLK:
        case WM_MOUSEWHEEL:
        case WM_KEYDOWN: case WM_KEYUP: case WM_CHAR:
            //ProcessClick(lp);
            return 0;
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_SETCURSOR://wtf? fix it
            if (LOWORD(lp) == HTCLIENT) {
                SetCursor(NULL);
                return TRUE;
            }
            return DefWindowProc(hwnd, msg, wp, lp);
        
        case WM_SIZE:
            dirty = true;
            break;

        case WM_MOUSEMOVE:
            if (!tracked) {
                tracked = BeginMouseLeaveTracking(hwnd);
            }
            auto pt2 = POINT{ MAKEPOINTS(lp).x, MAKEPOINTS(lp).y };

            static thread_local WCHAR tipBuf2[bufsz];
            if (layout && Interface::cursor_refresh(inter.cursor2, pt2, hwnd, layout->windowSize.x, layout->windowSize.y) && tipWND) {
                printf("MOUSEMOVE from viceWindow; <%3d, %3d>\n", pt2.x, pt2.y);
                if (int result=0; result = layout->debug(inter.cursor2, pt2, tipBuf2, bufsz)) {
                    if (result == 1) {
                        tipINFO.lpszText = tipBuf2;
                        //SendMessageW(tipWND, TTM_SETTOOLINFO, 0, (LPARAM)&tipINFO);
                        SendMessageW(tipWND, TTM_UPDATETIPTEXTW, 0, (LPARAM)&tipINFO);
                    }
                    //swprintf_s(tipBuf2, _countof(tipBuf2), L"%d, \n%d", pt2.x, pt2.y);
                    ClientToScreen(hwnd, &pt2);
                    /*RECT rcScreen;
                    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcScreen, 0);
                    SIZE tipSize{};
                    SendMessageW(tipWND, TTM_GETBUBBLESIZE, 0, (LPARAM)&tipINFO);
                    if (pt2.x + tipSize.cx > rcScreen.right) pt2.x -= tipSize.cx + 20;
                    if (pt2.y + tipSize.cy > rcScreen.bottom) pt2.y -= tipSize.cy + 20;*/
					//pt2.x += 8; pt2.y += 8;
					pt2.y += 12;
                    SendMessageW(tipWND, TTM_TRACKPOSITION, 0, MAKELPARAM(pt2.x, pt2.y));
                    SendMessageW(tipWND, TTM_TRACKACTIVATE, TRUE, (LPARAM)&tipINFO);
					//SendMessageW(tipWND, TTM_ACTIVATE, TRUE, 0);//确保启用?
                }
                else {
                    SendMessageW(tipWND, TTM_TRACKACTIVATE, FALSE, (LPARAM)&tipINFO);
                }
            }
			dirty = true;//for a cursor...
            break;
        case WM_MOUSELEAVE:
            printf("MOUSELEAVE from viceWindow.\n");
            SendMessageW(tipWND, TTM_TRACKACTIVATE, FALSE, (LPARAM)&tipINFO);
            tracked = false;
        case WM_KILLFOCUS:
            inter.cursor2 = Interface::toler * -2;
            layout && layout->debug(inter.cursor2, pt2, tipBuf2, bufsz);
            dirty = true;//for a cursor...
            break;
        case WM_EXITSIZEMOVE:
            // 拖拽结束，焦点设置回主窗口
            if (IsWindow(SokuLib::window)) {
                SetForegroundWindow(SokuLib::window);
                // SetFocus(GetDlgItem(hMainWnd, IDC_MAIN_BUTTON));
            }
			followMainWnd = false;
            break;
        case WM_NCRBUTTONUP:
            //WriteToClipboard(hwnd, "s:只因你太美");//多字节，但是剪贴板的解释在GBK和UTF-8间变化?
            if (inter.focus) {
                WriteToClipboard(hwnd, inter.focus.tostring());
                layout && (layout->hint_timer = 160);
                dirty = true;
            }
            //WriteToClipboard(hwnd, (char8_t*)"u8:\xE5\x8F\xAA\xE5\x9B\xA0\xE4\xBD\xA0\xE5\xA4\xAA\xE7\xBE\x8E\xF0\x9F\x98\x83");
            //WriteToClipboard(hwnd, u"u16:\u53EA\u56E0\u4F60\u592A\u7F8E\xD83D\xDE03");
            //WriteToClipboard(hwnd, U"u32:\U000053EA\U000056E0\U00004F60\U0000592A\U00007F8E\U0001F603");
            return 0;
        case WM_NCLBUTTONDBLCLK:
            followMainWnd = true;
            resetViceSP(hwnd, true);
            return 0;
        case WM_LBUTTONUP:
            //inter.focus = SokuLib::v2::GameDataManager::instance->activePlayers[0];
            if (layout && layout->clickBuffer(inter.cursor2, 
                [](void* ptr) {
					if (!ptr) return;
                    if (gui::get_player_id((GameObjectBase*)ptr))
                        inter.focus = (Player*)ptr;
                    else 
						inter.focus = (GameObject*)ptr;
                }
            ))
			    dirty = true;
            return 0;

        case WM_VICE_WINDOW_UPDATE:
            InvalidateRect(hwnd, NULL, true);
			UpdateWindow(hwnd);
            //if (layout && layout->debug(inter.cursor2, pt2, tipBuf2, 256)) {
            if (int result = 0;  layout && (result = layout->debug_mini(inter.cursor2, pt2, tipBuf2, bufsz))) {
				//tipINFO.lpszText = tipBuf2;
                if (result == 1) {
                    SendMessageW(tipWND, TTM_UPDATETIPTEXTW, 0, (LPARAM)&tipINFO);
                }
            }
            else
                SendMessageW(tipWND, TTM_TRACKACTIVATE, FALSE, (LPARAM)&tipINFO);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc;
            RECT rt;
            hdc = BeginPaint(hwnd, &ps);
            /*if (!TryEnterCriticalSection(&d3dMutex)) {
				EndPaint(hwnd, &ps);
                return 0;
            }

			LeaveCriticalSection(&d3dMutex);
            */
            EndPaint(hwnd, &ps);
            //dirty = false;
            return 0;
        }
        case WM_CLOSE:
            hideWnd(hwnd);
            return 0;
        case WM_DESTROY:
            EnterCriticalSection(&d3dMutex);
            DestroyD3D();
            LeaveCriticalSection(&d3dMutex);
            UninstallHooks(GetParent(hwnd));
            viceWND = NULL;
            if (tipWND && IsWindow(tipWND)) {
                SendMessageW(tipWND, TTM_TRACKACTIVATE, FALSE, (LPARAM)&tipINFO);
                SendMessageW(tipWND, TTM_DELTOOL, 0, (LPARAM)&tipINFO);
                DestroyWindow(tipWND);
                tipWND = nullptr;
                tipINFO = {sizeof TOOLINFOW};
            }
            PostQuitMessage(0);
            return 0;
        case WM_VICE_WINDOW_DESTROY:
            DestroyWindow(hwnd);
			return 0;
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
    static void setViceSP(HWND hwnd, POINT xy, SIZE wh) {
		if (!hwnd) return;
        bool noMove = (xy.x < 0 || xy.y < 0);
        bool noSize = (wh.cx <= 0 || wh.cy <= 0);
        // 获取窗口当前矩形
        RECT rcCur{};
        GetWindowRect(hwnd, &rcCur);
        if (noMove) {
            xy.x = rcCur.left;
            xy.y = rcCur.top;
        }
        if (noSize) {
            wh.cx = rcCur.right - rcCur.left;
            wh.cy = rcCur.bottom - rcCur.top;
        }
        RECT desired = { xy.x, xy.y, xy.x + wh.cx, xy.y + wh.cy };
        HMONITOR hMon = MonitorFromRect(&desired, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi{ .cbSize = sizeof(mi) };
        if (!GetMonitorInfoW(hMon, &mi)) {
            // 若失败，退回到系统主监视器工作区
            mi.rcWork.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
            mi.rcWork.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
            mi.rcWork.right = mi.rcWork.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
            mi.rcWork.bottom = mi.rcWork.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
        }
        auto fixx = GetSystemMetrics(SM_CXFRAME), fixy = GetSystemMetrics(SM_CYFRAME);
        wh.cx = std::clamp(wh.cx, 1L, mi.rcWork.right - mi.rcWork.left + fixx);
        wh.cy = std::clamp(wh.cy, 1L, mi.rcWork.bottom - mi.rcWork.top + fixy);

        // 限制位置：保持窗口完全在工作区内
        LONG limx = max(mi.rcWork.left, mi.rcWork.right - wh.cx) ; 
        LONG limy = max(mi.rcWork.top, mi.rcWork.bottom - wh.cy);
        xy.x = std::clamp(xy.x, mi.rcWork.left, limx + fixx);
        xy.y = std::clamp(xy.y, mi.rcWork.top, limy);
        UINT flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED;
        if (noMove) flags |= SWP_NOMOVE;
        if (noSize) flags |= SWP_NOSIZE;
        SetWindowPos(hwnd, NULL, xy.x, xy.y, wh.cx, wh.cy, flags);
    }
        static HWND adjusted_size_chached = NULL;
        static SIZE vice_adjusted_size{ 0 };
    void Vice::resetViceSP(HWND hwnd, bool resize) {
        if (adjusted_size_chached != hwnd) {
            //WINDOWINFO wi{ .cbSize = sizeof(WINDOWINFO) };
            //GetWindowInfo(hwnd, &wi);
			RECT rcWindow;
			GetWindowRect(hwnd, &rcWindow);
            adjusted_size_chached = hwnd;
            vice_adjusted_size = { rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top };
        }
        RECT mainRect;
        GetWindowRect(SokuLib::window, &mainRect);
        int childX = mainRect.right;
        int childY = mainRect.top;
        setViceSP(hwnd, { childX, childY }, resize ? vice_adjusted_size : SIZE{-1,-1});
	}
    LRESULT CALLBACK Vice::MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        thread_local static bool tracked = false;
        switch (msg) {
        //case WM_MOVING:
        //case WM_WINDOWPOSCHANGED:
        case WM_SIZE: case WM_MOVE: {
            LONG style = GetWindowLongW(hwnd, GWL_STYLE);
            if (followMainWnd |= !(style & WS_CAPTION) || !(style & WS_THICKFRAME)) {
                resetViceSP(viceWND, false);
                //SetWindowPos(viceWND, NULL, childX, childY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
			dirty = true;

            break;
        }
        case WM_MOUSEMOVE:
            if (!tracked) {
                tracked = BeginMouseLeaveTracking(hwnd);
            }
            auto pt2 = POINT{ MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y };
            printf("MOUSEMOVE from MainWindow; <%3d, %3d>\n", pt2.x, pt2.y);
            if (Interface::cursor_refresh(inter.cursor, pt2, hwnd))
                dirty = true;
            break;
        case WM_MOUSELEAVE:
            printf("MOUSELEAVE from MainWindow.\n");
            tracked = false;
        case WM_KILLFOCUS:
            inter.cursor = Interface::toler * -2;
            break;
        case WM_MAIN_TOGGLECURSOR:
            if (wParam)
                printf("Show Cursor %d by %d\n", ShowCursor(TRUE), lParam);
            else
                printf("Hide Cursor %d by %d\n", ShowCursor(FALSE), lParam);
            return 0;
        case WM_DPICHANGED:
            //ResetD3D9Dev();
            //dirty = true;
			break;
        }
            
        return CallWindowProcW(ogMainWndProc, hwnd, msg, wParam, lParam);
    }
    /*LRESULT CALLBACK Vice::WindowMouseHook(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode >= 0) {
            auto pms = (LPMOUSEHOOKSTRUCT)lParam;
            if (SokuLib::window && pms->hwnd == SokuLib::window) {
                switch (wParam) {
                case WM_MOUSEMOVE: {
                    auto pt = pms->pt;
                    ScreenToClient(pms->hwnd, &pt);
                    if (Interface::cursor_refresh(inter.cursor, pt, pms->hwnd))
                        dirty = true;
                    break;
                }
                case WM_NCMOUSEMOVE: case WM_NCMOUSELEAVE: {
                    inter.cursor = { -1, -1 };
                    dirty = true;
                    break;
                }
                }
                return CallNextHookEx(mainMouseHook, nCode, wParam, lParam);
            } else if (viceWND && pms->hwnd == viceWND) {
                switch (wParam) {
                case WM_MOUSEMOVE: {
                    auto pt = pms->pt;
                    ScreenToClient(pms->hwnd, &pt);
                    layout && Interface::cursor_refresh(inter.cursor2, pt, pms->hwnd, layout->windowSize.x, layout->windowSize.y);
                    break;
                }
                case WM_NCMOUSEMOVE: case WM_NCMOUSELEAVE: case WM_MOUSELEAVE: {
                    inter.cursor2 = { -1, -1 };
                    break;
                }
                }
                return CallNextHookEx(mouseHook, nCode, wParam, lParam);
            }
        }
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }*/
    bool Vice::InstallHooks(HINSTANCE hInstance, HWND hwndParent) {
        if (hwndParent) {
            if (!ogMainWndProc)
                ogMainWndProc = (WNDPROC)SetWindowLongW(hwndParent, GWL_WNDPROC, (LONG)MainWindowProc);
            /*mainHook = SetWindowsHookExW(
                WH_CALLWNDPROC,
                MainWindowProcHook,
                hInstance,
                mainThreadId
            );*/
            /*if (!mainMouseHook) {
                DWORD mainThreadId = GetWindowThreadProcessId(SokuLib::window, NULL);
                mainMouseHook = SetWindowsHookExW(WH_MOUSE, WindowMouseHook, hInstance, mainThreadId);
            }*/
        }
        /*if (viceWND && !mouseHook) {
            DWORD viceThreadId = GetWindowThreadProcessId(viceWND, NULL);
            mouseHook = SetWindowsHookExW(WH_MOUSE, WindowMouseHook, hInstance, viceThreadId);
        }*/
        return ogMainWndProc;//&& mainMouseHook&& mouseHook;
    }
    void Vice::UninstallHooks(HWND hwndParent) {
        if (hwndParent) {
            if (ogMainWndProc) {
                SetWindowLongW(hwndParent, GWL_WNDPROC, (LONG)ogMainWndProc);
                ogMainWndProc = NULL;
            }
            /*if (mainMouseHook) {
                UnhookWindowsHookEx(mainMouseHook);
                mainMouseHook = NULL;
            }*/
        }
        /*if (viceWND) {
            if (mouseHook) {
                UnhookWindowsHookEx(mouseHook);
                mouseHook = NULL;
            }
        }*/
	}

    bool Interface::checkDMouse() {
        if ((ztimer -= ztimer > 0 ? 1 : 0) == 0) {
			if (zaccu) printf("Scroll accu reset.\n");
            zaccu = 0;
        }
        auto& curi = *reinterpret_cast<const DWORD*>(sokuDMouse.rgbButtons);
        auto old = oldi; oldi = curi;
        if (curi && !old) {//some button pressed
            //POINT pt; GetCursorPos(&pt);
            //ScreenToClient(SokuLib::window, &pt);
            //cursor_refresh(cursor, pt);
            if (!checkInWnd(cursor)) return false;
            auto oldf = focus;
            if (sokuDMouse.rgbButtons[0]) {
                printf("LB Clicked at <%3d, %3d>\n", cursor.x, cursor.y);
                //focus = getHover();
                auto n = getHover();
				//if (n) focus = n;//implicitly converted to GameObjectBase*
				//if (n) focus.swap(n);
                if (n) { focus = std::move(n); }
                return oldf != focus;
            }
            if (sokuDMouse.rgbButtons[1]) {
                printf("RB Clicked at <%3d, %3d>\n", cursor.x, cursor.y);
                focus = nullptr;
				//index = -1;
                switchHover(0);
                return oldf != focus;
            }
        }
        else if (sokuDMouse.lZ) {
            if (!checkInWnd(cursor)) return false;
            int d = abs(sokuDMouse.lZ * 4 / (ztimer + 4));//decay if big and fast enough
            ztimer = zcool;
            d = max(d, zthre/10); d = min(d, zthre);//~thre
            d *= sokuDMouse.lZ >= 0 ? 1:-1;
            zaccu = d + ((sokuDMouse.lZ * zaccu >= 0) ? zaccu : 0);
            printf("Scroll %3d(%d) of %3d\n", zaccu, d, zthre);
            return abs(zaccu) >= zthre 
                && (zaccu = 0, switchHover(sokuDMouse.lZ < 0 ? 1 : -1));
        }
        return false;
    }

    void Interface::drawIndicator(const Vector2f& pos, int timer, IndicatorState state) {
        static IndicatorState lastState = Idle;
        constexpr float easing = 0.2f;
        constexpr float baseRadius = 30.f;   // 环的基准半径
        constexpr float baseWidth = 30.f;   // 环宽度
        constexpr float minWidth = 8.f;    // 收缩时变窄宽度
        // 检测状态切换
        //static int changePoint = 0;
        if (state != lastState) {
            //changePoint = timer;
            //lastState = state;
        }
        //constexpr int length = 120;
        static float progress = 0.f;
        static Vector2f angle = {0.f,360.f};
        progress += ((state == Selected ? 1.f : 0.f)-progress) * easing;
        // --- 扇环角度 ---
        int count = getCount(), index = getIndex(); index = max(0, index);
        float angleSpan = (count > 0) ? (360.f / count) : 360.f;
        float baseAngle =  + angleSpan / 2;
        Vector2f targetAngle = { index * angleSpan, index * angleSpan + angleSpan};
        angle -= (angle - targetAngle) * easing;
        float fanWidth = baseWidth - (baseWidth - minWidth) * progress;
        float fanRadius = progress * baseRadius + fanWidth / 2;
        auto scp = riv::tex::RendererGuard();
        scp.setInvert().setTexture(0);
        Draw2DCircle<128>(
            SokuLib::pd3dDev,
            {
                std::clamp(pos.x, baseRadius, sokuW-baseRadius),
                std::clamp(pos.y, baseRadius, sokuH - baseRadius),
            },
            fanRadius,
            fanWidth,
            angle,
            { 0 },
            1,
            Color(0xFFdfb3ff)
        );
        float ringRaius = (fanRadius + fanWidth / 2) * progress + (toler.x);
        Draw2DCircle<32>(
            SokuLib::pd3dDev,
            pos,
            ringRaius,
            toler.x,
            {0,360},
            { 0 },
            1,
            Color(0xFFe4ffaa)
        );
        // 滚动 z 控制当前环的扩展
        float scrollFactor = float((abs(zaccu)+zthre) % zthre) / zthre;
        static float scrollRadius = 0.f;
        scrollRadius -= (scrollRadius - baseRadius * 1.5f * scrollFactor) * 0.5f;

        
        
        if (state == Selected && count>1 && scrollFactor > 0.1f) {
            Draw2DCircle<32>(
                SokuLib::pd3dDev,
                pos,
                scrollRadius * progress,
                3.f,
                { 0.f, 360.f },
                { 0 },
                1,
                Color(0x02020202 * int(127.f * ztimer / zcool))
            );
        }


        
    }

    //inline void Interface::updateHover() {
    //    if (!checkInWnd(cursor)) return;
    //    if (!inserting.try_lock())
    //        return;//fix race
    //    hovers.clear(); index = -1;
    //    for (auto it = poolPlayer.lower_bound(cursor.x - toler),
    //        it2 = poolPlayer.upper_bound(cursor.x + toler);
    //        it != it2 && it != poolPlayer.end(); ++it)
    //    {
    //        for (auto it3 = it->second.lower_bound(cursor.y - toler),
    //            it4 = it->second.upper_bound(cursor.y + toler);
    //            it3 != it4 && it3 != it->second.end(); ++it3)
    //        {
    //            hovers.emplace_back(it3->second);
    //        }
    //    }
    //    for (auto it = poolObject.lower_bound(cursor.x - toler),
    //        it2 = poolObject.upper_bound(cursor.x + toler);
    //        it != it2 && it != poolObject.end(); ++it)
    //    {
    //        for (auto it3 = it->second.lower_bound(cursor.y - toler),
    //            it4 = it->second.upper_bound(cursor.y + toler);
    //            it3 != it4 && it3 != it->second.end(); ++it3)
    //        {
    //            hovers.emplace_back(it3->second);
    //        }
    //    }
    //    //index = hovers.empty() ? -1 : 0;
    //    inserting.unlock();
    //}

    void drawObjectHover(Interface::PhoverPublic& phover, float time = timeGetTime()/1000.f) {
        using riv::tex::RendererGuard, riv::tex::Color;
        using SokuLib::v2::BlendOptions, SokuLib::v2::FrameData;
        auto* object = const_cast<GameObjectBase*>(phover.get_base());
		if (!object || !object->frameData) return;
        
        RendererGuard guard;
        guard.setRenderMode(0);//save mode
        auto temp = object->renderInfos; auto& color = *reinterpret_cast<Color*>(&object->renderInfos.color);
        color.a = 0xFF;
        float interp = tanhf(sinf(time * 2 * 3.1416f));
        if (phover.is_object()) {
            auto* object = phover.get_object();
            //float interp2 = (-interp + 1.f) / 2.f;
            if (object->tail) {
                auto& mode = object->tail->paramD;
                auto old = mode;
                switch (mode) {
                case BlendOptions::NORMAL:
                    break;
                case BlendOptions::ADDITIVE:
                    mode = BlendOptions::MULTIPLY; break;
                case BlendOptions::MULTIPLY:
                    mode = BlendOptions::ADDITIVE; break;
                }
                object->tail->render();
                mode = old;
            }
        }
        interp = (interp+1.f) / 2.f;

        switch (object->frameData->renderGroup) {
        case FrameData::SPRITE:
        case FrameData::TEXTURE:
        BLEND_NORMAL:
            object->renderInfos.shaderType = 3;
            object->renderInfos.shaderColor = 0x01010101 * int(interp * 0x7F);
            object->render();//sets render mode by group and blendmode
            break;
        default:
        case FrameData::WITHBLEND:
            auto blend = object->frameData->blendOptionsPtr;
			if (!blend) return;
            object->frameData->blendOptionsPtr = new BlendOptions{ *blend };

            switch (object->frameData->blendOptionsPtr->mode) {
            case BlendOptions::MULTIPLY:
                object->frameData->blendOptionsPtr->mode = BlendOptions::ADDITIVE;
                color = color * interp;
                //object->frameData->blendOptionsPtr->color = 0xFFFF00FF;
                break;
            case BlendOptions::NORMAL:
                ////goto BLEND_NORMAL;
                //color += Color::Green;
                object->renderInfos.shaderType = 2;
                object->renderInfos.shaderColor = Color::White * interp;
                break;
            case BlendOptions::ADDITIVE:
                object->frameData->blendOptionsPtr->mode = BlendOptions::MULTIPLY;
                color = color * interp;
                //object->frameData->blendOptionsPtr->color = 0xFF00FF00;
                break;
            }
            object->render();//sets render mode by group and blendmode
            delete object->frameData->blendOptionsPtr; object->frameData->blendOptionsPtr = blend;
        }
        object->renderInfos = temp;

    }


}

bool __fastcall info::Vice::CBattle_Render(SokuLib::Battle* This)
{using riv::tex::RendererGuard, riv::tex::Vertex;
    auto ret = (This->*ogSceneBattleRender)();
    bool d = dirty;
    if(!viceDisplay 
    || !viceSwapChain 
    || !layout || !d && !layout->hint_timer 
    ) return ret;


    //if (layout) {
    //auto target = inter.focus ? inter.focus : inter.getHover();
    auto target = inter.getCount() > 0 ? inter.getHover() : inter.focus;
    //int count = inter.getCount(), index = inter.getIndex();
    void* ctx = nullptr;
	//gui::string_view name;
    gui::string name;
	std::array<std::string, 5>::const_iterator it, end;
    static const std::array<gui::string, 5> Ofallbacks = { "%s.%d", "Object", "Basic", "Mini", "None"};
	static const std::array<gui::string, 5> Pfallbacks = { "%s", "Player", "Basic", "Mini", "None"};
    if (target.is_object()) {
		it = Ofallbacks.begin(); end = Ofallbacks.end();
        auto* obj = target.get_object();
        ctx = (void*)obj;
        int actId = target->frameState.actionId;
        if (actId >= 1000) {//common obj
            gui::base_char buf[128];
            std::snprintf(buf, sizeof(buf), (*it).c_str(), "common", actId);
            name = buf;
            //name = temp;
        } else if (actId >= 800 && obj->gameData.owner) {//specific character bullet
            const auto* chr = SokuLib::getCharName(obj->gameData.owner->characterIndex);
            if (!chr || strlen(chr) <= 0) goto FALLBACK;
            gui::base_char buf[128];
            std::snprintf(buf, sizeof(buf), (*it).c_str(), chr, actId);
            name = buf;
        } else {
FALLBACK:
            name = *++it;
        }
    }
    else if (target.is_player()) {
		it = Pfallbacks.begin(); end = Pfallbacks.end();
        ctx = (void*)target.get_player();
        name = SokuLib::getCharName(((Player*)ctx)->characterIndex);
        if (!name.empty()) {
            gui::base_char buf[128];
            std::snprintf(buf, sizeof(buf), (*it).c_str(), name.c_str());
            name = buf;
        } else {
            name = *++it;
        }
    } else if (ctx = (void*)target.get_base()) {
		it = Ofallbacks.begin()+2; end = Ofallbacks.end();
		name = *it;
    } else {
        it = Ofallbacks.begin()+4; end = Ofallbacks.end();
		name = *it;
    }
    bool successful = true;
    //while (it!=end && !layout->render(*it, ctx)) { it++; }
    while (!layout->updates(name, ctx, successful)) {
        if (++it != end) {
            name = *it;
        }
        else break;
    }//}

    //rendering
    if (!successful || !TryEnterCriticalSection(&info::d3dMutex))
		return ret;
    LPDIRECT3DSURFACE9 pBackBuffer = NULL, pOrgBuffer = NULL;
    if (FAILED(viceSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer))
    || FAILED(SokuLib::pd3dDev->GetRenderTarget(0, &pOrgBuffer))
    ) {
        return LeaveCriticalSection(&info::d3dMutex), ret;
	}
    D3DVIEWPORT9 old_vp, new_vp{
        .X=0, .Y=0, 
        .Width = (DWORD)layout->windowSize.x, .Height = (DWORD)layout->windowSize.y, 
        .MinZ = 0.0f, .MaxZ = 1.0f
    }, def_vp{0};
    RECT old_scr, new_scr{
            .left = 0,
            .top = 0,
            .right = (LONG)layout->windowSize.x,
            .bottom = (LONG)layout->windowSize.y
    };
    LPDIRECT3DSURFACE9 old_depth = nullptr;
    SokuLib::pd3dDev->GetDepthStencilSurface(&old_depth);
    SokuLib::pd3dDev->GetViewport(&old_vp);
    SokuLib::pd3dDev->GetScissorRect(&old_scr);
    SokuLib::pd3dDev->SetRenderTarget(0, pBackBuffer); pBackBuffer->Release();
    //D3DSURFACE_DESC dsd;
    //static int callonce0 = (old_depth->GetDesc(&dsd), printf("Modified DepthStencil = %dx%d\n", dsd.Width, dsd.Height));//480p
	SokuLib::pd3dDev->SetDepthStencilSurface(viceDepthStencil);
    //SokuLib::pd3dDev->GetViewport(&def_vp);
	SokuLib::pd3dDev->SetViewport(&new_vp);
    //static int callonce = printf("Viewport: %d %d %d %d | Window: (%d %d)\n", def_vp.X, def_vp.Y, def_vp.Width, def_vp.Height, layout->windowSize.x, layout->windowSize.y);
    SokuLib::pd3dDev->SetScissorRect(&new_scr);
    if (SUCCEEDED(SokuLib::pd3dDev->BeginScene())) {
        RendererGuard guard; guard.setRenderMode(1).setTexture(0);
        guard.setRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        SokuLib::pd3dDev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(194, 144, 198), 1.0f, 0);
        /*DWORD scissorEnable = 0; SokuLib::pd3dDev->GetRenderState(D3DRS_SCISSORTESTENABLE, &scissorEnable);
        static auto callonce = scissorEnable ? printf("Scissor test was ENABLED\n") : printf("Scissor test was disabled\n");
        static auto callonce2 = printf("ScissorRect {left=%d top=%d right=%d bottom=%d}\n", old_scr.left, old_scr.top, old_scr.right, old_scr.bottom);*/
        
        /*DWORD ClipPlaneEnable = 0; SokuLib::pd3dDev->GetRenderState(D3DRS_CLIPPLANEENABLE, &ClipPlaneEnable);
        static auto callonceA = ClipPlaneEnable ? printf("ClipPlane was ENABLED\n") : printf("ClipPlane was disabled\n");
		float plane[4];
        SokuLib::pd3dDev->GetClipPlane(0, plane);
        static auto callonceB = printf("ClipPlane {A=%d B=%d C=%d D=%d}\n", plane[0], plane[1], plane[2], plane[3]);*/
        
        //guard.setRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        //guard.setRenderState(D3DRS_CLIPPLANEENABLE, FALSE);
        layout->render();
        //SokuLib::pd3dDev->GetScissorRect(&new_scr);
        //static auto callonce3 = printf("Modified ScissorRect {left=%d top=%d right=%d bottom=%d}\n", new_scr.left, new_scr.top, new_scr.right, new_scr.bottom);
        //SokuLib::pd3dDev->GetViewport(&def_vp);
        //static auto callonce4 = printf("Modified Viewport: %d %d %d %d | Window: (%d %d)\n", def_vp.X, def_vp.Y, def_vp.Width, def_vp.Height, layout->windowSize.x, layout->windowSize.y);
        
		fvck(SokuLib::pd3dDev); SokuLib::pd3dDev->EndScene(); fvck(SokuLib::pd3dDev);//SokuLib::renderer.end();
    }

    //if (*(bool*)0x896b76) {//renderer is on, d3d_ok
    //if (SokuLib::pd3dDev->TestCooperativeLevel() == D3D_OK) {
        auto hr = viceSwapChain->Present(NULL, NULL, viceWND, NULL,
            //D3DPRESENT_FORCEIMMEDIATE
            D3DPRESENT_INTERVAL_IMMEDIATE
            | D3DPRESENT_DONOTWAIT
        );
    //}
    SokuLib::pd3dDev->SetRenderTarget(0, pOrgBuffer); pOrgBuffer->Release();
	SokuLib::pd3dDev->SetDepthStencilSurface(old_depth); old_depth && old_depth->Release();
	SokuLib::pd3dDev->SetViewport(&old_vp);
    SokuLib::pd3dDev->SetScissorRect(&old_scr);
	LeaveCriticalSection(&info::d3dMutex);

    //auto target = inter.focus ? inter.focus : inter.getHover();
    if (!target.has_value()) {
		wndTitle(L"[SokuDebugPanel] Idling...");
    }
    else if (target.is_player()) {
        auto player = target.get_player(); if (player) {
        std::string name = SokuLib::getCharName(player->characterIndex); name = name.length() > 0 ? (name[0] = std::toupper(name[0]), name) : "Player";
        int pid = gui::get_player_id(player);
        auto formatted = pid ? std::format("(P{:1d})", pid) : "";
        formatted = std::format("{:8s}{}: 0x{:08x}",
            //index+1, count,
            name,
            formatted, 
            (DWORD)player
        );
        auto buf = std::wstring(MultiByteToWideChar(CP_UTF8, 0, formatted.c_str(), (int)formatted.size(), nullptr, 0), 0);
        MultiByteToWideChar(CP_UTF8, 0, formatted.c_str(), (int)formatted.size(), buf.data(), buf.size());
        wndTitle(buf);
    }}
    else if (target.is_object()) {
		auto object = target.get_object(); if (object) {
        auto player = object->gameData.owner;
		int teamid = player ? player->teamId+1 : 0;
        auto formatted = std::format("Object{}: 0x{:08x}", 
                //index + 1, count,
                teamid > 0 ? std::format("(P{:1d})", teamid) : std::string(""), 
                (DWORD)object
        );
        //format = ;//tuple<> pfocus
        auto buf = std::wstring(MultiByteToWideChar(CP_UTF8, 0, formatted.c_str(), (int)formatted.size(), nullptr, 0), 0);
        MultiByteToWideChar(CP_UTF8, 0, formatted.c_str(), (int)formatted.size(), buf.data(), buf.size());
        wndTitle(buf);
    }}
    else {

    }

    updateWnd();
    dirty = false;
	return ret;
}

TrampTamper<5> reset_swapchain_shim(0x415186);
void info::Vice::BeforeD3D9DevReset() {
    puts("Hooked before reset: swapchain release.");
    DestroyD3D();
}

TrampTamper<5> reset_swapchain_shim2(0x4151dd);
void info::Vice::AfterD3D9DevReset() {
	//EnterCriticalSection(&d3dMutex);
    puts("Hooked after reset: create additional swapchain.");
    CreateD3D(viceWND);
    //LeaveCriticalSection(&d3dMutex);
}
