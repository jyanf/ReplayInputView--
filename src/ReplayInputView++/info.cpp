#include "info.hpp"

#include <optional>

VBattleRender ogSceneBattleRender{};

namespace info {
    std::array<unsigned char, fvckSize> fvckIchirin{ 0x90 };

    const DIMOUSESTATE& sokuDMouse = *(LPDIMOUSESTATE)(0x8a019c + 0x140);
    const LPDIRECT3D9& pd3d = *(LPDIRECT3D9*)(0x8A0E2C);
    CRITICAL_SECTION& d3dMutex = *reinterpret_cast<CRITICAL_SECTION*>(0x8A0E14);
	const LPDIRECT3DSWAPCHAIN9& pd3dSwapChain = *(LPDIRECT3DSWAPCHAIN9*)(0x8A0E34);
	const bool& d3dWindowed = *(bool*)(0x8A0F88);


	
    WNDPROC Vice::ogMainWndProc = NULL;
    HHOOK Vice::mouseHook = NULL;
    //bool Vice::registered = false;
    HWND Vice::viceWND = NULL;
    //LPDIRECT3D9 Vice::vicePD3 = NULL;
    //LPDIRECT3DDEVICE9 Vice::vicePD3D = NULL;
	LPDIRECT3DSWAPCHAIN9 Vice::viceSwapChain = NULL;
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
        HANDLE duplicatedHandle = NULL;
        // ����α���Ϊ�ɿ��̵߳���Ч���
        if (!DuplicateHandle(
            GetCurrentProcess(),  // Դ����
            GetCurrentThread(),          // Դ�����α�����
            GetCurrentProcess(),  // Ŀ����̣�ͬһ���̣�
            &duplicatedHandle,  // Ŀ�����������
            0,                    // ����Ȩ�ޣ����ȴ��ã���������Ȩ�ޣ�
            FALSE,                // ���ɼ̳�
            DUPLICATE_SAME_ACCESS  // ��ͬ����Ȩ��
        )) {
            MessageBoxW(NULL, L"����������ʧ�ܣ�", L"����", MB_ICONERROR);
            return false;
        }
        Design::Object* rect = nullptr;
        layout->getById(&rect, 1);
        auto viceParam = new ViceThreadParams{
			.hMainThread = duplicatedHandle,
            .hParentWnd = SokuLib::window,
            .hInstance = hDllModule,
            .width = rect ? int(rect->x2) : WIDTH,
            .height = rect ? int(rect->y2) : HEIGHT,
        };
        HANDLE hViceThread = CreateThread(
            NULL,               // Ĭ�ϰ�ȫ����
            0,                  // Ĭ��ջ��С
            WindowMain,     // ���̺߳���
            viceParam,         // ���ݸ����̵߳Ĳ���
            0,                  // ��������
            NULL                // ����ȡ�߳�ID
        );
        if (hViceThread == NULL) {
            CloseHandle(duplicatedHandle);  // �ͷŸ��Ƶľ��
            MessageBoxW(NULL, L"������`CreateThread`ʧ�ܣ�", L"����", MB_ICONERROR);
            return false;
        }
		CloseHandle(hViceThread); // ����Ҫ�����߳̾��
		return true;
	}
    bool Vice::CreateD3D(HWND& hwnd) {
        if (!d3dWindowed) {
            //MessageBoxW(NULL, L"��ǰΪȫ��ģʽ���޷�����Direct3D�豸��", L"����", MB_ICONERROR);
			//hideWnd(hwnd);
            return false;
		}
        if (hwnd && !viceSwapChain) {
            D3DPRESENT_PARAMETERS d3dpp{ 0 };
            d3dpp.Windowed = TRUE;
            d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;

            /*D3DDISPLAYMODE mode{0};
            if (FAILED(pd3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode))) {
                MessageBoxW(NULL, L"`GetAdapterDisplayMode`ʧ�ܣ�", L"����", MB_ICONERROR);
                return false;
            }*/
            d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;//mode.Format;
            d3dpp.hDeviceWindow = hwnd;
            /*WINDOWINFO wi{ 0 };
            GetWindowInfo(hwnd, &wi);
            d3dpp.BackBufferWidth = wi.rcClient.right - wi.rcClient.left;
            d3dpp.BackBufferHeight = wi.rcClient.bottom - wi.rcClient.top;*/
            d3dpp.EnableAutoDepthStencil = 1;
            d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
            d3dpp.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
            d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;// no vsync?

            if (FAILED(SokuLib::pd3dDev->CreateAdditionalSwapChain(&d3dpp, &viceSwapChain)) || !viceSwapChain) {
                MessageBoxW(NULL, L"`CreateAdditionalSwapChain`ʧ�ܣ�", L"����", MB_ICONERROR);
                return false;
            }

            
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
        HANDLE waitHandles[] = { params.hMainThread };  // �ȴ��Ķ������߳̾��
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
        viceWND = CreateWindowExW(
            WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
            VICE_CLASSNAME,
            L"Defaulting to Players: ",
            WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX & ~WS_MINIMIZEBOX & ~WS_VISIBLE | WS_POPUP,  // ��ֹ������С/���
            gameRect.right, gameRect.top,  // λ�ø�����Ϸ�����Ҳ�
            params.width, params.height,
            params.hParentWnd,
            NULL,
            params.hInstance,
            NULL
        );
        if (!viceWND) goto ExitLoop;
        if (!InstallHooks(params.hInstance, params.hParentWnd)) {
            UninstallHooks(params.hParentWnd);
            goto ExitLoop;
        }

        ShowWindow(viceWND, SW_SHOWNOACTIVATE);
        UpdateWindow(viceWND);
        viceDisplay = true;

        showCursor(true, 1);

        while (true) {
            // �ȴ���Ҫô���߳̽�����������źţ���Ҫô�д�����Ϣ����������WAIT_OBJECT_0 + 1��
            DWORD waitResult = MsgWaitForMultipleObjects(
                waitCount,
                waitHandles,
                FALSE,        // ���ȴ����ж�����һ���źż�����
                INFINITE,     // ���޵ȴ���ֱ�����߳̽���������Ϣ��
                QS_ALLINPUT   // �ȴ��������͵�������Ϣ
            );

            switch (waitResult) {
                // ���1�����߳̽������ȴ�����0���źţ�
            case WAIT_OBJECT_0:
                DestroyWindow(viceWND);
                goto ExitLoop;

                // ���2���д�����Ϣ����
            case WAIT_OBJECT_0 + 1:
                // ������Ϣ���У����������������߳̽���ʱ�޷���Ӧ��
                while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                break;

                // ���3���ȴ�ʧ�ܣ�������Ч��
            default:
                goto ExitLoop;
            }
        }

    ExitLoop:
        viceWND = NULL;                        // ��Ǵ���������
        UnregisterClassW(VICE_CLASSNAME, params.hInstance);  // ע��������
        CloseHandle(params.hMainThread);                        // �ͷ����߳̾��
        return 0;
    }
    LRESULT CALLBACK Vice::WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
        case WM_CREATE:
            CreateD3D(hwnd);
			return 0;
            // ���ؽ��������Ϣ
        case WM_SETFOCUS: case WM_ACTIVATE: case WM_ACTIVATEAPP:
            return 0;

        // ��������
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
            // ��ק�������������û�������
            if (IsWindow(SokuLib::window)) {
                SetForegroundWindow(SokuLib::window);
                // SetFocus(GetDlgItem(hMainWnd, IDC_MAIN_BUTTON));
            }
            break;
        case WM_VICE_WINDOW_UPDATE:
            InvalidateRect(hwnd, NULL, true);
			UpdateWindow(hwnd);
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
            /*
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



            */

            EndPaint(hwnd, &ps);
            //dirty = false;
            return 0;
        }
        case WM_CLOSE:
            hideWnd(hwnd);
            return 0;
        case WM_DESTROY:
            DestroyD3D();
			UninstallHooks();
			viceWND = NULL;
            PostQuitMessage(0);
            return 0;
        case WM_VICE_WINDOW_DESTROY:
            DestroyWindow(hwnd);
			return 0;
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    LRESULT CALLBACK Vice::MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        //case WM_MOVING:
        //case WM_WINDOWPOSCHANGED:
        case WM_SIZE: case WM_MOVE: {
            RECT mainRect;
            GetWindowRect(SokuLib::window, &mainRect);
            // �����Ӵ�����λ�ã�������λ�� + ƫ�ƣ�
            int childX = mainRect.right;
            int childY = mainRect.top;
            // �ƶ��Ӵ��ڣ���������ı��С��
            SetWindowPos(viceWND, NULL, childX, childY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);


            break;
        }
        case WM_KILLFOCUS:
            inter.cursor = { -1, -1 };
            break;
        case WM_MAIN_TOGGLECURSOR:
            if (wParam)
                printf("Show Cursor %d by %d\n", ShowCursor(TRUE), lParam);
            else
                printf("Hide Cursor %d by %d\n", ShowCursor(FALSE), lParam);
            return 0;
        }
            
        CallWindowProcW(ogMainWndProc, hwnd, msg, wParam, lParam);
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
        // ������Ϣ����һ������/����
        return CallNextHookEx(mouseHook, nCode, wParam, lParam);
    }
    bool Vice::InstallHooks(HINSTANCE hInstance, HWND hwnd = SokuLib::window) {
        if (!hwnd || ogMainWndProc) return false;
        DWORD mainThreadId = GetWindowThreadProcessId(SokuLib::window, NULL);
        ogMainWndProc = (WNDPROC)SetWindowLongW(hwnd, GWL_WNDPROC, (LONG)MainWindowProc);
        /*mainHook = SetWindowsHookExW(
            WH_CALLWNDPROC,
            MainWindowProcHook,
            hInstance,
            mainThreadId
        );*/
        mouseHook = SetWindowsHookExW(
            WH_MOUSE,
            MainWindowMouseHook,
            hInstance,
            mainThreadId
        );
        return ogMainWndProc != NULL && mouseHook != NULL;
    }
    void Vice::UninstallHooks(HWND hwnd) {
        if (ogMainWndProc) {
            SetWindowLongW(hwnd, GWL_WNDPROC, (LONG)ogMainWndProc);
            ogMainWndProc = NULL;
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

    void drawObjectHover(GameObjectBase* object, float time = timeGetTime()/1000.f) {
        using riv::tex::RendererGuard, riv::tex::Color;
        using SokuLib::v2::BlendOptions, SokuLib::v2::FrameData;
		if (!object || !object->frameData) return;
        
        RendererGuard guard;
        guard.setRenderMode(0);//save mode
        auto temp = object->renderInfos; auto& color = *reinterpret_cast<Color*>(&object->renderInfos.color);
        color.a = 0xFF;
        float interp = tanhf(sinf(time * 2 * 3.1416f)) + 1.f; interp /= 2.f;
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
    if(!viceDisplay|| !dirty || !layout.has_value() || !TryEnterCriticalSection(&info::d3dMutex))
		return ret;
    if (!viceSwapChain)
		return LeaveCriticalSection(&info::d3dMutex), ret;
    LPDIRECT3DSURFACE9 pBackBuffer = NULL, pOrgBuffer = NULL;
    if (FAILED(viceSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer))
    || FAILED(SokuLib::pd3dDev->GetRenderTarget(0, &pOrgBuffer))
    ) {
        return LeaveCriticalSection(&info::d3dMutex), ret;
	}
	SokuLib::pd3dDev->SetRenderTarget(0, pBackBuffer); pBackBuffer->Release();
    if (SUCCEEDED(SokuLib::pd3dDev->BeginScene())) {
    //rendering
        RendererGuard guard; guard.setRenderMode(1);
        SokuLib::pd3dDev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(194, 144, 198), 1.0f, 0);
        auto target = inter.focus ? inter.focus : inter.getHover();
        Vertex vertices[4] = {
                {0, 0,	0.0f, 1.0f,	riv::tex::Color::White,	0.0f,	0.0f},
                {0, 0,	0.0f, 1.0f, riv::tex::Color::White,	0.0f,	1.0f},
                {0, 0,	0.0f, 1.0f, riv::tex::Color::White,	1.0f,	1.0f},
                {0, 0,	0.0f, 1.0f,	riv::tex::Color::White,	1.0f,	0.0f},
        };
        if (target && target->frameData) {
            Design::Object *texpos = nullptr, *texframe = nullptr;
		    layout->getById(&texpos, 10); layout->getById(&texframe, 11);
            if (texpos && texframe) {
                float x1 = texpos->x2, y1 = texpos->y2, x2 = texframe->x2, y2 = texframe->y2;
                float w = abs(x2 - x1), h = abs(y2 - y1);
                auto r = target->frameData->texSize;
                float dscale = target->frameData->renderGroup==0 ? 2.0f : 1.0f;
                float scale = 1.0f;//scale = r.x/r.y > w/h ? w/r.x : h/r.y;
                w = r.x*scale; h = r.y*scale;
                r = (target->frameData->offset / dscale * scale).to<short>();
                vertices[0].x = vertices[2].x = (x1 + x2) / 2 - r.x;
                vertices[1].x = vertices[3].x = (x1 + x2) / 2 + w - r.x;
                vertices[0].y = vertices[1].y = (y1 + y2) / 2 - r.y;
                vertices[2].y = vertices[3].y = (y1 + y2) / 2 + h - r.y;
                for (int i = 0; i < 4; ++i) {
				    vertices[i].u = target->sprite.vertices[i].u;
                    vertices[i].v = target->sprite.vertices[i].v;
                }
                if (target->frameData->renderGroup==2 && target->frameData->blendOptionsPtr)
                    vertices[0].color = vertices[1].color = vertices[2].color = vertices[3].color = target->frameData->blendOptionsPtr->color;
                                                            
                guard.setTexture(0);
                SokuLib::pd3dDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(*vertices));
                guard.setTexture(target->sprite.dxHandle);  
                SokuLib::pd3dDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(*vertices));
            }
        }

        layout->render4();
        
    //SokuLib::renderer.end();
		fvck(SokuLib::pd3dDev); SokuLib::pd3dDev->EndScene(); fvck(SokuLib::pd3dDev);
    }
    



    SokuLib::pd3dDev->SetRenderTarget(0, pOrgBuffer); pOrgBuffer->Release();
    //if (*(bool*)0x896b76) {//renderer is on, d3d_ok
    //if (SokuLib::pd3dDev->TestCooperativeLevel() == D3D_OK) {
        viceSwapChain->Present(NULL, NULL, NULL, NULL,
            //D3DPRESENT_FORCEIMMEDIATE
            D3DPRESENT_INTERVAL_IMMEDIATE
            //D3DPRESENT_DONOTWAIT
        );
    //}

	LeaveCriticalSection(&info::d3dMutex);

    auto target = inter.focus ? inter.focus : inter.getHover();
    if (target) {
        auto formatted = std::format("{}:{:#08x} | act{:03} | seq{:03}", "Object", (DWORD)target, target->frameState.actionId, target->frameState.sequenceId);
        //format = ;//tuple<> pfocus
        auto buf = std::wstring(MultiByteToWideChar(CP_UTF8, 0, formatted.c_str(), (int)formatted.size(), nullptr, 0), 0);
        MultiByteToWideChar(CP_UTF8, 0, formatted.c_str(), (int)formatted.size(), buf.data(), buf.size());
        wndTitle(buf);
    }

    updateWnd();
    dirty = false;
	return ret;
}

TrampTamper<5> reset_swapchian_shim(0x4151dd);
void info::Vice::ResetD3D9Dev() {
	//EnterCriticalSection(&d3dMutex);
    DestroyD3D();
    CreateD3D(viceWND);
    //LeaveCriticalSection(&d3dMutex);
}
