#pragma once
#include <d3d9.h>
#include <DrawUtils.hpp>
//#include <TextureManager.hpp>
#define FVF_SWRVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

struct SWRVERTEX {
	float x, y, z;
	float rhw; // RHW = reciprocal of the homogeneous (clip space) w coordinate of a vertex (the 4th dimension for computing the scaling and translating)
	D3DCOLOR color;
	float u, v;
	void set(float _x, float _y, float _z) {
		x = _x;
		y = _y;
		z = _z;
	}
	void set_xy(float _x, float _y) {
		x = _x;
		y = _y;
	}
};
using SokuLib::Vector2f;
using Color = SokuLib::DrawUtils::DxSokuColor;
using SokuLib::DrawUtils::FloatRect;
#define MATH_SIGN(v) ((v)>0 ? 1 : (v)<0 ? -1 : 0)
template<int NUM_FRAGS, int leanby10 = 10>
inline void Draw2DCircle(LPDIRECT3DDEVICE9 dev, Vector2f pos, float radius, float width, Vector2f fanangle, FloatRect uvBorder = {0,0,1,1}, float multi = 1.0f, Color color = Color::White)
{
	//if (lineangle.x > lineangle.y) std::swap(lineangle.x, lineangle.y);
	//if (lineangle.x < 0) lineangle.x = 0;
	//if (lineangle.y > 360) lineangle.y = 360;
	constexpr double dAngle = 360.0 / NUM_FRAGS;
	int a1 = fanangle.x/dAngle + 0.5 * MATH_SIGN(fanangle.x), a2 = fanangle.y/dAngle + 0.5 * MATH_SIGN(fanangle.y);
	int dir = (a2 - a1 >= 0) ? 1 : -1;
	if ((a2 - a1)*dir < 1) 
		return;

	DWORD old;
	dev->GetFVF(&old);
#ifdef _DEBUG
	//printf("fvf %#x\n", old);//0x144
#endif
	dev->SetFVF(FVF_SWRVERTEX);
	//dev->SetVertexShader (D3DFVF_MIRUSVERTEX);
	int step = NUM_FRAGS / multi;
	auto CurvePts = new SWRVERTEX[step+1][2]{ 0 };
	for (int i = a1; (a2 - i)*dir > 0; i+= dir * step)//curves
	{
		float theta, x, y;
		theta = D3DXToRadian((dir * i) * dAngle + 90.0f - dir * 1.5f);
		x = (float)(pos.x + (radius + width / 2) * cos(theta));
		y = (float)(pos.y - leanby10 / 10.0f * (radius + width / 2) * sin(theta));
		CurvePts[0][0] = { x, y, 0.0f, 1.0f, color, uvBorder.x1, uvBorder.y1 };
		x = (float)(pos.x + (radius - width / 2) * cos(theta));
		y = (float)(pos.y - leanby10 / 10.0f * (radius - width / 2) * sin(theta));
		CurvePts[0][1] = { x, y, 0.0f, 1.0f, color, uvBorder.x1, uvBorder.y2 };
		int limit = min(step, (a2 - i) * dir);
		for (int j = 1; j <= limit; ++j)
		{
			theta = D3DXToRadian((dir * j + i) * dAngle + 90.0f - dir * 1.5f);
			float u = j * 1.0f/step * (uvBorder.x2 - uvBorder.x1) + uvBorder.x1;
			x = (float)(pos.x + (radius + width / 2) * cos(theta));
			y = (float)(pos.y - leanby10 / 10.0f * (radius + width / 2) * sin(theta));
			CurvePts[j][0] = { x, y, 0.0f, 1.0f, color, u, uvBorder.y1 };
			x = (float)(pos.x + (radius - width / 2) * cos(theta));
			y = (float)(pos.y - leanby10 / 10.0f * (radius - width / 2) * sin(theta));
			CurvePts[j][1] = { x, y, 0.0f, 1.0f, color, u, uvBorder.y2 };
		}

		dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2*(limit), CurvePts, sizeof(CurvePts[0][0]));
	}
	dev->SetFVF(old);
	delete [] CurvePts;
}