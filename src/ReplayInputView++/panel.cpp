#include "panel.hpp"

#include <assert.h>


namespace riv::pnl {

	TileDesc<256, 64, 32, 32> Panel::texBtn;

	static const Color backColors[] = {
		0xA0808080, 0xA0808080,
		0xA0202020, 0xA0202020
	};
	void Panel::update(const Player& player)
	{
		if (player.inputData.inputType == 2) {
			hide = true; return;
		} else
			hide = false;

		const auto& cur = player.inputData.keyInput;
		const auto& buf = player.inputData.bufferedKeyInput;

		input.prev = input.cur;
		input.cur.update(cur);
		input.one.check_one(player.inputData);
		input.buf.check_buf(player.inputData);

		input.record.update(input.cur, input.prev, input.buf | input.one);


	}
	void Panel::renderBack(float x, float y, float cx, float cy) const {
		float d = direction * slant;
		const Vertex vertices[] = {
			{x - d,		y,		0.0f, 1.0f, backColors[0], 0.0f, 0.0f},
			{x + cx - d,	y,		0.0f, 1.0f, backColors[1], 1.0f, 0.0f},
			{x + cx + d,	y + cy,	0.0f, 1.0f, backColors[2], 1.0f, 1.0f},
			{x + d,		y + cy,	0.0f, 1.0f, backColors[3], 0.0f, 1.0f},
		};
		SokuLib::textureMgr.setTexture(NULL, 0);//
		SokuLib::pd3dDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, (const void*)vertices, sizeof(Vertex));
	}

	void Panel::renderInputs() const {
		tex::RendererGuard guard;//auto old = SetRenderMode(1);
		guard.setRenderMode(1);
		float x = posI.x, y = posI.y;
		guard.saveTexture(); renderBack(x, y, 24 * 6 + 24, 24 * 3 + 12);
		// Directions
		x += 9; y += 6;
		texBtn.render(0, { x + 24,	y }, input.cur.up ? 1.0 : 0.18); /* ¡ü */
		texBtn.render(1, { x + 24,	y + 48 }, input.cur.dn ? 1.0 : 0.18); /* ¡ý */
		texBtn.render(2, { x,		y + 24 }, input.cur.lf ? 1.0 : 0.18); /* ¡û */
		texBtn.render(3, { x + 48,	y + 24 }, input.cur.rt ? 1.0 : 0.18); /* ¡ú */
		texBtn.render(4, { x,		y }, input.cur.lu ? 1.0 : 0.18); /* ¨I */
		texBtn.render(5, { x + 48,	y }, input.cur.ru ? 1.0 : 0.18); /* ¨J */
		texBtn.render(6, { x + 48,	y + 48 }, input.cur.rd ? 1.0 : 0.18); /* ¨K */
		texBtn.render(7, { x,		y + 48 }, input.cur.ld ? 1.0 : 0.18); /* ¨L */
		// Buttons
		x += 75; y += 12;
		float d = direction * slant;
		texBtn.render(8, { x - d,		y }, input.cur.a ? 1.0 : 0.18);
		texBtn.render(9, { x - d + 27,	y }, input.cur.b ? 1.0 : 0.18);
		texBtn.render(10, { x - d + 54,	y }, input.cur.c ? 1.0 : 0.18);
		texBtn.render(11, { x + d,		y + 24 }, input.cur.d ? 1.0 : 0.18);
		texBtn.render(12, { x + d + 27,	y + 24 }, input.cur.ch ? 1.0 : 0.18);
		texBtn.render(13, { x + d + 54,	y + 24 }, input.cur.s ? 1.0 : 0.18);
		//guard.setRenderMode(2);
		guard.setInvert();
		for (int i = Inputs::Rec::A; i <= Inputs::Rec::S; ++i) {
			float x2 = x - d + (i - 9) / 3 * 2 * d + (i - 9) % 3 * 27, y2 = y + (i - 9) / 3 * 24;
			if (input.one[i])
				texBtn.render(14, { x2-2.5f, y2-2.5f, x2+3+32, y2+3+32}, Color::White);
			else if (input.buf[i])
				texBtn.render(15, { x2, y2 }, 1.0f);
		}
	}

	void Panel::renderRecord() const {
		tex::RendererGuard guard;
		float x = posR.x, y = posR.y;
		float sx = 24 * 10 + 6, sy = 24 + 6;
		guard.saveTexture(); renderBack(x, y, sx, sy);
		int dir = direction;//[RecordPanel].Mirror
		x += sx * (1+dir)/2 + 3 - dir * texBtn.dx/2; y += 3;

		for (int i = 0; i < input.record.len; ++i) {
			int j = (input.record.base + (dir > 0 ? input.record.len - 1 - i : i)) % _countof(input.record.id);
			int id = input.record.id[j];
			if (id) {
				float alpha = dir > 0 ? 1.0 - max(i - 6, 0) * 0.25 : 1.0 - max(input.record.len - i - 7, 0) * 0.25;
				if (id != Inputs::Rec::NONE) {
					alpha *= input.record.expire[j] ? 1.0f : 0.5f;
				}
				texBtn.render(id - 1, Vector2f{ x - i * dir * 24 - texBtn.dx / 2, y }, alpha);
			}
		}
	}

	void Panel::render() const {
		//renderBack();
		if (hide) return;
		switch (enableState) {
		case 1:
			renderInputs();
			break;
		case 2:
			renderInputs();
		case 3:
			renderRecord();
		case 0:
		default:
			break;
		}
	}

	void Inputs::Cmd::check_buf(const Player::InputInfo& input) {//check btn active
		val = 0;
		const KeyInputLight& keys = input.keyInput;
		//update<true>(keys);
		const KeyInputLight& buf = input.bufferedKeyInput;
		a |= keys.a == 2 || 0 < input.keyUpA && input.keyUpA <= 2 || buf.a;
		b |= keys.b == 2 || 0 < input.keyUpB && input.keyUpB <= 2 || buf.b;
		c |= keys.c == 2 || 0 < input.keyUpC && input.keyUpC <= 2 || buf.c;
		s |= 0 < keys.spellcard && keys.spellcard <= 2 || buf.spellcard;

		d |= keys.d && (keys.horizontalAxis || keys.verticalAxis);

	}
	void Inputs::Cmd::check_one(const Player::InputInfo& input) {//check btn active
		val = 0;
		const KeyInputLight& keys = input.keyInput;
		//update<true>(keys);
		const KeyInputLight& buf = input.bufferedKeyInput;
		
		a = keys.a == 1;
		b = keys.b == 1;
		c = keys.c == 1;
		d = keys.d == 1;
		ch = keys.changeCard == 4 && buf.changeCard && input.bufferTimer == 9;//make sure the first frame
		s = keys.spellcard == 1;
	}

	void Inputs::Cmd::update(const KeyInputLight& keys) {
		val = 0;
		if (keys.verticalAxis < 0)
			up = true;
		else if (keys.verticalAxis > 0)
			dn = true;
		if (keys.horizontalAxis < 0)
			lf = true;
		else if (keys.horizontalAxis > 0)
			rt = true;

		auto dir4 = val & Comb::DIR4;
		if ((dir4 ^ Comb::LU & Comb::DIR4) == 0) val ^= Comb::LU;
		if ((dir4 ^ Comb::RU & Comb::DIR4) == 0) val ^= Comb::RU;
		if ((dir4 ^ Comb::RD & Comb::DIR4) == 0) val ^= Comb::RD;
		if ((dir4 ^ Comb::LD & Comb::DIR4) == 0) val ^= Comb::LD;
#ifdef _DEBUG
		unsigned short dir8 = val & Comb::DIR8;
		assert((dir8 & (dir8 - 1)) == 0);//only one dir or nothing
#endif // _DEBUG

		if (keys.a > 0) a = true;
		if (keys.b > 0) b = true;
		if (keys.c > 0) c = true;
		if (keys.d > 0) d = true;
		if (keys.changeCard > 0) ch = true;
		if (keys.spellcard > 0) s = true;
	}
	void Inputs::Rec::update(Cmd cur, Cmd prev, Cmd act) {
		for (int i = UP; i <= S; ++i) {
			if (!cur[i] || prev[i]) continue;
			int index = (len + base) % _countof(id);
			id[index] = ID(i);
			expire[index] = 14;
			if (len >= _countof(id)) {
				base = (base + 1) % _countof(id);
			}
			else {
				++len;
			}
		}
		for (int i = len-1; i >= 0; --i) {
			int index = (base + i) % _countof(id);
			if (id[index] == NONE || expire[index]<1) continue;
			//--expire[i];
			bool unused = cur[id[index]];
			expire[index] -= !unused;
			if (unused) {
				cur.val ^= 1 << (id[index]-1);
			}
			//expire[i] -= !act[id[i]-1];
		}
	}

}
