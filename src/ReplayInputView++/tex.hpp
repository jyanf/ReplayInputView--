#pragma once
#include "../main.hpp"

#include <DrawUtils.hpp>
#include <Renderer.hpp>
//#include <Design.hpp>
#include <TextureManager.hpp>
#include <assert.h>
#include <unordered_map>

namespace riv {
using SokuLib::Renderer;
    int SetRenderMode(int mode);
	

namespace tex {
//using SokuLib::CTile;
using SokuLib::Vector2f;
using SokuLib::DrawUtils::Vertex;
using SokuLib::DrawUtils::FloatRect;
using Color = SokuLib::DrawUtils::DxSokuColor;

	int create_texture_byid(int offset);

	struct Tex {
		int ref = 0;
		int texId = NULL;
		inline void create(int id) {
			++ref;
			if (texId) return;
			//D3DXGetImageInfoFromResource(SokuLib::pd3dDev, hDllModule, );
			texId = create_texture_byid(id);
		};
		inline void cancel() {
			if (--ref > 0 || !texId) return;
			SokuLib::textureMgr.remove(texId);
			texId = NULL;
		}
		inline void set() const { SokuLib::textureMgr.setTexture(texId, 0); }
	};

	template <int sx, int sy, int gx, int gy, int ox = 0, int oy = 0>
	struct TileDesc : public Tex {
		const int w = sx, h = sy;
		const int dx = gx, dy = gy;
		const int col = sx / gx, row = sy / gy;
		static_assert(sx >= gx && sy >= gy && gx > 0 && gy > 0 || sx<0 && sy<0, "asset para error");
		//void create(int rcId);
		inline void render(int index, FloatRect vert, Color color = Color::White) {
			/*float u = ox + (index % col) * gx, v = oy + (index / row) * gy;
			u /= sx; v /= sy;
			float du = gx / sx, dv = gy / sy;*/
			auto uv = getBorder(index);
			const Vertex vertices[] = {
				{vert.x1, vert.y1,	0.0f, 1.0f,	color,	uv.x1,	uv.y1},
				{vert.x2, vert.y1,	0.0f, 1.0f, color,	uv.x2,	uv.y1},
				{vert.x2, vert.y2,	0.0f, 1.0f, color,	uv.x2,	uv.y2},
				{vert.x1, vert.y2,	0.0f, 1.0f,	color,	uv.x1,	uv.y2},
			};
			set();
			SokuLib::pd3dDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, (const void*)vertices, sizeof(Vertex));
		}
		inline void render(int index, Vector2f pos, float alpha) {
			render(index, { pos.x, pos.y, pos.x + gx, pos.y + gy }, Color::White * alpha);
		}
		
		inline FloatRect getBorder(int index) {
#ifdef _DEBUG
			assert(0<=index && index<row*col);
#endif
			float u = ox + (index % col) * gx, v = oy + (index / col) * gy;
			u /= sx; v /= sy;
			float du = gx, dv = gy;
			du /= sx; dv /= sy;
			return { u, v, u + du, v + dv };
		}
	};
	typedef TileDesc<-1, -1, -1, -1> TileBlank;


    class [[nodiscard]] RendererGuard {
        LPDIRECT3DDEVICE9 mpd = SokuLib::pd3dDev;
        std::unordered_map<D3DRENDERSTATETYPE, DWORD> orgStates;

		std::unordered_map<DWORD, LPDIRECT3DBASETEXTURE9> orgTextures;
        int originalMode = -1;
    public:
		explicit RendererGuard() = default;
        explicit RendererGuard(LPDIRECT3DDEVICE9 pDevice) : mpd(pDevice) {
            if (!pDevice) {
                throw std::invalid_argument("Device pointer cannot be null");
            }
        }
		RendererGuard(const RendererGuard&) = delete;
		RendererGuard& operator=(const RendererGuard&) = delete;
		void* operator new(size_t) = delete;
		void* operator new(size_t, void*) = delete;
		void* operator new[](size_t) = delete;
		void operator delete(void*) = delete;
		void operator delete[](void*) = delete;

        ~RendererGuard() {
            restore();
        }
		inline bool isSoku() const { return mpd && mpd == SokuLib::pd3dDev; }
        // 设置渲染状态，自动保存原始值
		RendererGuard& setRenderState(D3DRENDERSTATETYPE state, DWORD value) &;
		inline void resetRenderState() & {
			if (!mpd) return;
			for (auto& state : orgStates) {
				mpd->SetRenderState(state.first, state.second);
			}
			orgStates.clear();
		}
		inline void resetRenderState(D3DRENDERSTATETYPE state) & {
			if (!mpd) return;
			if (orgStates.find(state) == orgStates.end()) return;
			mpd->SetRenderState(state, orgStates[state]);
			orgStates.erase(state);
		}
		RendererGuard& setRenderMode(int mode) &;
		inline void resetRenderMode() & {
			if (!isSoku() || originalMode<0) return;
			riv::SetRenderMode(originalMode);
			originalMode = -1;
		}
		RendererGuard& setTexture(LPDIRECT3DTEXTURE9 handle, int stage = 0) &;
		inline RendererGuard& saveTexture() & {
			return setTexture(NULL, 0);
		}
		inline RendererGuard& setTexture(int texId) & {
			if (!isSoku()) return *this;
			if (!texId) return setTexture(NULL, 0);
			auto phandle = SokuLib::textureMgr.Get(texId);
			return phandle ? setTexture(*phandle, 0) : *this;
		}
		inline RendererGuard& setTexture(const Tex& tex) & {
			return setTexture(tex.texId);
		}
		inline void resetTextures() & {
			if (!isSoku()) return;
			for (auto& tex : orgTextures) {
				//SokuLib::textureMgr.setTexture((int)tex.second, tex.first);
				mpd->SetTexture(tex.first, tex.second);
			}
			orgTextures.clear();
		}
		inline RendererGuard& setInvert() & {
			return setRenderState(D3DRS_ALPHABLENDENABLE, TRUE)
				.setRenderState(D3DRS_SRCBLEND, D3DBLEND_INVDESTCOLOR)
				.setRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA)
				.setRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD)
				//.setRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE)
				//.setRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_SRCALPHA)
				//.setRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA)
			;
		}
		inline RendererGuard& setInvert2() & {
			return setRenderState(D3DRS_DESTBLEND, D3DBLEND_INVDESTCOLOR)
					//.setRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ONE)
				;
		}
        // 恢复所有修改过的状态
        inline void restore() & {
			resetRenderState();
			resetRenderMode();
			resetTextures();
        }
    };


}
}

