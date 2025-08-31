#include "info.hpp"

namespace info {
    const DIMOUSESTATE& sokuDMouse = *(LPDIMOUSESTATE)(0x8a019c + 0x140);

    HHOOK Vice::mainHook = NULL, Vice::mouseHook = NULL;
    //bool Vice::registered = false;
    HWND Vice::viceWND = NULL;
    LPDIRECT3D9 Vice::vicePD3 = NULL;
    LPDIRECT3DDEVICE9 Vice::vicePD3D = NULL;
    std::atomic_bool Vice::viceDisplay = false, Vice::dirty = false;

	Interface Vice::inter(10);//hovers reserve

struct ViceThreadParams {
    HANDLE hMainThread;
    HWND hParentWnd;
    HINSTANCE hInstance;
};
    bool Vice::createWnd() {
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
        auto viceParam = new ViceThreadParams{
			.hMainThread = duplicatedHandle,
            .hParentWnd = SokuLib::window,
            .hInstance = hDllModule,
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
            MessageBoxW(NULL, L"创建副线程失败！", L"错误", MB_ICONERROR);
            return false;
        }
		CloseHandle(hViceThread);  // 不需要保留线程句柄
		return true;
	}
    void Vice::updateWnd() {
        if (!viceWND) return;

        //UpdateWindow(viceWND);
        PostMessageW(viceWND, WM_VICE_WINDOW_UPDATE, 0, 0);
    }
    void Vice::destroyWnd() {
        if (viceWND) {
            //DestroyWindow(viceWND);
            //viceWND = NULL;
            PostMessageW(viceWND, WM_VICE_WINDOW_DESTROY, 0, 0);
        }
    }

    bool Vice::CreateD3D(HWND& hwnd) {
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
        }
		return true;
    }
    bool Vice::DestroyD3D()
    {
        bool t = true;
        if (vicePD3D) {
            vicePD3D->Release();
            vicePD3D = NULL;
        } else t = false;

        if (vicePD3) {
            vicePD3->Release();
            vicePD3 = NULL;
        } else t = false;
        return t;
    }



constexpr auto VICE_CLASSNAME = L"SokuDbgInfoPanel";
    DWORD WINAPI Vice::WindowMain(LPVOID pParams) {
        //SetThreadDpiAwarenessContex(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
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
            .hbrBackground = CreateSolidBrush(RGB(255, 0, 0)),
            .lpszClassName = VICE_CLASSNAME,
        };
        //WM_VICE_WINDOW_DESTROY = RegisterWindowMessageW(L"WM_VICE_WINDOW_DESTROY");
        RegisterClassExW(&wc);
        //if (!viceWND) {
        RECT gameRect;
        GetWindowRect(params.hParentWnd, &gameRect);
        viceWND = CreateWindowExW(
            WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
            VICE_CLASSNAME,
            L"Defaulting to Players: ",
            WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX & ~WS_MINIMIZEBOX & ~WS_VISIBLE | WS_POPUP,  // 禁止调整大小/最大化
            gameRect.right, gameRect.top,  // 位置跟随游戏窗口右侧
            WIDTH, HEIGHT,
            params.hParentWnd,
            NULL,
            params.hInstance,
            NULL
        );
        if (!viceWND) goto ExitLoop;
        if (!InstallHooks(params.hInstance, params.hParentWnd)) {
            UnsintallHooks();
            goto ExitLoop;
        }

        ShowWindow(viceWND, SW_SHOWNOACTIVATE);
        UpdateWindow(viceWND);
        viceDisplay = true;


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
    LRESULT CALLBACK Vice::WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
        case WM_CREATE:
            CreateD3D(hwnd);
			return 0;
            // 拦截焦点相关消息
        case WM_SETFOCUS: case WM_ACTIVATE: case WM_ACTIVATEAPP:
            return 0;

        // 拦截输入
        case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
        case WM_LBUTTONUP: case WM_RBUTTONUP: case WM_MBUTTONUP:
        case WM_NCRBUTTONDOWN: case WM_NCRBUTTONUP://case WM_NCLBUTTONDOWN:
        case WM_NCLBUTTONDBLCLK: case WM_LBUTTONDBLCLK: case WM_NCRBUTTONDBLCLK: case WM_RBUTTONDBLCLK:
        case WM_MOUSEWHEEL:
        case WM_KEYDOWN: case WM_KEYUP: case WM_CHAR:
            //ProcessClick(lp);
            return 0;
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        
        case WM_EXITSIZEMOVE:
            // 拖拽结束，焦点设置回主窗口
            if (IsWindow(SokuLib::window)) {
                SetForegroundWindow(SokuLib::window);
                // SetFocus(GetDlgItem(hMainWnd, IDC_MAIN_BUTTON));
            }
            break;
        case WM_VICE_WINDOW_UPDATE:
            InvalidateRect(hwnd, NULL, true);
            break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc;
            RECT rt;
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rt);
            DrawTextW(hdc, L"Test", -1, &rt, DT_LEFT | DT_VCENTER | DT_WORD_ELLIPSIS);
			rt.top += 100;
            WCHAR buf[48] = { 0 };
            wsprintfW(buf, L"Cursor: %d %d", inter.cursor.x, inter.cursor.y);
            DrawTextW(hdc, buf, -1, &rt, DT_LEFT | DT_VCENTER);
            rt.top += 100;
            wsprintfW(buf, L"Focus: 0x%p", inter.focus);
            DrawTextW(hdc, buf, -1, &rt, DT_LEFT | DT_VCENTER);
            rt.top += 100;
            auto phover = inter.getHover();
            wsprintfW(buf, L"Hover: 0x%p\n pos: %.2f", phover, phover ? phover->position.x : NAN);
            DrawTextW(hdc, buf, -1, &rt, DT_LEFT | DT_VCENTER);
            rt.top += 100;



            EndPaint(hwnd, &ps);
            dirty = false;
            return 0;
        }
        case WM_CLOSE:
            hideWnd(hwnd);
            return 0;
        case WM_DESTROY:
            DestroyD3D();
			UnsintallHooks();
			viceWND = NULL;
            PostQuitMessage(0);
            return 0;
        case WM_VICE_WINDOW_DESTROY:
            DestroyWindow(hwnd);
			return 0;
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    LRESULT CALLBACK Vice::MainWindowProcHook(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode >= 0 && SokuLib::window && viceWND) {
            CWPSTRUCT* pCwp = (CWPSTRUCT*)lParam;
            if (pCwp->hwnd == SokuLib::window) {
                switch (pCwp->message) {
                //case WM_MOVING:
                //case WM_WINDOWPOSCHANGED:
                case WM_SIZE: case WM_MOVE: {
                    WINDOWPOS* pWp = (WINDOWPOS*)pCwp->lParam;
                    RECT mainRect;
                    GetWindowRect(SokuLib::window, &mainRect);
                    // 计算子窗口新位置（主窗口位置 + 偏移）
                    int childX = mainRect.right;
                    int childY = mainRect.top;
                    // 移动子窗口（不激活、不改变大小）
                    SetWindowPos(viceWND, NULL, childX, childY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                    break;
                }
                case WM_KILLFOCUS:
                    inter.cursor = { -1, -1 };
                    break;

                }
            }
        }
        // 传递消息给下一个钩子/窗口
        return CallNextHookEx(mainHook, nCode, wParam, lParam);
    }
    LRESULT CALLBACK Vice::MainWindowMouseHook(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode >= 0 && SokuLib::window && viceWND) {
            auto pms = (LPMOUSEHOOKSTRUCT)lParam;
            if (pms->hwnd == SokuLib::window) {
                switch (wParam) {
                
                case WM_MOUSEMOVE: {
                    auto pt = pms->pt;
                    ScreenToClient(pms->hwnd, &pt);
                    float scaleX, scaleY;
                    RECT rt;
                    GetClientRect(pms->hwnd, &rt);
                    scaleX = (float)rt.right / inter.sokuW;
                    scaleY = (float)rt.bottom / inter.sokuH;
                    pt.x /= scaleX;
                    pt.y /= scaleY;
                    if (inter.cursor.x != pt.x || inter.cursor.y != pt.y)
                        dirty = true;
                    inter.cursor = { pt.x, pt.y };
                    break;
                }
                case WM_NCMOUSEMOVE: case WM_NCMOUSELEAVE: case WM_MOUSELEAVE: {
                    inter.cursor = { -1, -1 };
					dirty = true;
                    break;
                }
                }
            }
        }
        // 传递消息给下一个钩子/窗口
        return CallNextHookEx(mouseHook, nCode, wParam, lParam);
    }
    bool Vice::InstallHooks(HINSTANCE hInstance, HWND hwnd = SokuLib::window) {
        if (!hwnd || mainHook) return false;
        DWORD mainThreadId = GetWindowThreadProcessId(SokuLib::window, NULL);
        // 设置WH_CALLWNDPROC钩子，监视主窗口线程的消息
        mainHook = SetWindowsHookExW(
            WH_CALLWNDPROC,
            MainWindowProcHook,
            hInstance,
            mainThreadId
        );
        mouseHook = SetWindowsHookExW(
            WH_MOUSE,
            MainWindowMouseHook,
            hInstance,
            mainThreadId
        );
        return mainHook != NULL && mouseHook != NULL;
    }
    void Vice::UnsintallHooks() {
        if (mainHook) {
            UnhookWindowsHookEx(mainHook);
            mainHook = NULL;
        }
        if (mouseHook) {
            UnhookWindowsHookEx(mouseHook);
            mouseHook = NULL;
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

}