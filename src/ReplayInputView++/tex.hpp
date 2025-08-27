#pragma once
#include "../main.hpp"

#include <DrawUtils.hpp>
//#include <Design.hpp>
#include <TextureManager.hpp>
#include <assert.h>

namespace riv { namespace tex {
//using SokuLib::CTile;
using SokuLib::Vector2f;
using SokuLib::DrawUtils::Vertex;
using SokuLib::DrawUtils::FloatRect;
using Color = SokuLib::DrawUtils::DxSokuColor;

	int create_texture_byid(int offset);

	template <int sx, int sy, int gx, int gy, int ox = 0, int oy = 0>
	struct TileDesc {
		//int w, h;
		int ref = 0;
		const int w = sx, h = sy;
		const int dx = gx, dy = gy;
		const int col = sx / gx, row = sy / gy;
		static_assert(sx >= gx && sy >= gy && gx > 0 && gy > 0 || sx<0 && sy<0, "asset para error");
		int texId = NULL;
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
		inline void create(int id) {
			++ref;
			if (texId) return;
			//D3DXGetImageInfoFromResource(SokuLib::pd3dDev, hDllModule, );
			texId = create_texture_byid(id);
		};
		inline void cancel() {
			if (--ref>0 || !texId) return;
			SokuLib::textureMgr.remove(texId);
			texId = NULL;
		}
		inline void set() { SokuLib::textureMgr.setTexture(texId, 0); }
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






}}

