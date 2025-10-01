#include "tex.hpp"
#include <UnionCast.hpp>
namespace riv{
static const auto _setRenderMode = SokuLib::union_cast<void (Renderer::*)(int)>(0x404b80);
static const auto _renderMode = SokuLib::union_cast<int Renderer::*>(0xC);
	int SetRenderMode(int mode) {
		DWORD old = SokuLib::renderer.*_renderMode;
		if (mode == old) return old;
		(SokuLib::renderer.*_setRenderMode)(mode);
		//SokuLib::pd3dDev->GetRenderState(, &old);
		//SokuLib::pd3dDev->SetRenderState(, mode);
		return old;
	}
    /*
    static LPD3DXBUFFER pPSByteCode = NULL;
    // ֻ����������ɫ����ʹ�ù̶����ܶ������
    bool SetupPixelShaderOnly(LPDIRECT3DDEVICE9 pDevice) {
        LPD3DXBUFFER pPSErrors = NULL;
        const char* psCode =
            R"(
            texture2D SceneTexture;
            sampler2D SceneSampler = sampler_state { Texture = <SceneTexture>; };
            float4 InverseTransparentPS(float2 TexCoord : TEXCOORD0, float4 Color : COLOR0) : COLOR0 {
                float4 sceneColor = tex2D(SceneSampler, TexCoord);
                float alpha = Color.a;
                float4 result;
                result.rgb = (Color.rgb - sceneColor.rgb) * alpha + sceneColor.rgb * (1 - alpha);
                result.a = sceneColor.a;
                return result;
            }
            float4 SimpleInversePS(float4 color : COLOR0) : COLOR0 {
                return float4(1.0 - color.r, 1.0 - color.g, 1.0 - color.b, color.a);
            }
            )";

        // ����������ɫ��
        HRESULT hr = D3DXCompileShader(
            psCode,
            strlen(psCode),
            NULL,               // �궨��
            NULL,               // ����
            "InverseTransparentPS",
            "ps_2_0",           // ������ɫ��ģ��
            0,                  // ��־
            &pPSByteCode,
            &pPSErrors,
            NULL                // ������
        );

        if (FAILED(hr)) {
            if (pPSErrors) {
                OutputDebugStringA((char*)pPSErrors->GetBufferPointer());
                pPSErrors->Release();
            }
            return false;
        }

        // ����������ɫ��
        hr = pDevice->CreatePixelShader((DWORD*)pPSByteCode->GetBufferPointer(), &m_pInversePS);
        pPSByteCode->Release();

        return SUCCEEDED(hr);
    }

    // ��Ⱦ������ֻʹ��������ɫ����
    void RenderWithPixelShaderOnly(LPDIRECT3DDEVICE9 pDevice,
        LPDIRECT3DTEXTURE9 pSceneTexture,
        LPDIRECT3DVERTEXBUFFER9 pVB)
    {
        // ���浱ǰ״̬
        RenderStateGuard guard(pDevice);

        // ����������ɫ��
        pDevice->SetPixelShader(m_pInversePS);

        // ���ó�������
        pDevice->SetTexture(0, pSceneTexture);
        pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

        // ��������׶�״̬ - ʹ�ù̶����ܹ���
        pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

        // ���ö�������
        pDevice->SetStreamSource(0, pVB, 0, sizeof(Vertex));
        pDevice->SetFVF(VertexFVF);

        // ������Ⱦ״̬
        pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE); // ��ɫ�������ϣ�����Ӳ�����
        pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
        pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

        // ���ö�����ɫ��ΪNULL��ʹ�ù̶����ܹ���
        pDevice->SetVertexShader(NULL);

        // ����ͼԪ
        pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);

        // �ָ�Ĭ��������ɫ��
        pDevice->SetPixelShader(NULL);
    }*/

namespace tex{

	int create_texture(int offset) {
		int id;
		auto pphandle = SokuLib::textureMgr.allocate(&id);
        if (!pphandle)
            return 0;
        *pphandle = NULL;
		if (SUCCEEDED(D3DXCreateTextureFromResource(SokuLib::pd3dDev, hDllModule, MAKEINTRESOURCE(offset), pphandle)))
			return id;
		
		SokuLib::textureMgr.deallocate(id);
		return 0;
	}
    int create_texture(LPCSTR name, int* width, int* height) {
        int id=0;
        SokuLib::textureMgr.loadTexture(&id, name, width, height);
		return id;
    }

	RendererGuard& RendererGuard::setRenderState(D3DRENDERSTATETYPE state, DWORD value) & {
		if (!mpd) return *this;
		if (orgStates.find(state) == orgStates.end()) {
			DWORD old;
			if (FAILED(mpd->GetRenderState(state, &old))) {
				throw std::runtime_error("Failed to get render state");
			}
			if (old == value) return *this;
			orgStates[state] = old;
		}
		// ������ֵ
		if (FAILED(mpd->SetRenderState(state, value))) {
			throw std::runtime_error("Failed to set render state");
		}
		return *this;
	}
	RendererGuard& RendererGuard::setRenderMode(int mode) & {
		if (!isSoku()) return *this;
		auto old = riv::SetRenderMode(mode);
		if (originalMode == -1)
			originalMode = old;
		return *this;
	}
	RendererGuard& RendererGuard::setTexture(LPDIRECT3DTEXTURE9 handle, int stage) & {
		if (orgTextures.find(stage) == orgTextures.end()) {
			LPDIRECT3DBASETEXTURE9 old;
            if (FAILED(mpd->GetTexture(stage, &old))) { throw std::runtime_error("Failed to get/set texture"); }
			if (old == handle) return *this;
			orgTextures[stage] = old;
		}
        mpd->SetTexture(stage, handle);
		return *this;
	}


    }}
