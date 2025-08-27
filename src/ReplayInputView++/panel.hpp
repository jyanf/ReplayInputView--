#include "../main.hpp"
#include "tex.hpp"
#include "box.hpp"
#include <Player.hpp>

namespace riv {	namespace pnl {
using tex::TileDesc;
using tex::Vertex;
using tex::Color;
using SokuLib::Vector2f;
using SokuLib::v2::Player;
using SokuLib::KeyInputLight;

	struct SWRCMDINFO {
		bool enabled;
		int prev; // number representing the previously pressed buttons (masks are applied)
		int now; // number representing the current pressed buttons (masks are applied)

		struct {
			bool enabled;
			int id[10];
			int base; // once len reaches 10 (first cycle), is incremented modulo 10
			int len; // starts at 0, caps at 10
		} record;

	};
	struct Inputs {
		union Cmd{
			enum Comb : unsigned short {
				LU = 0b00010101,
				RU = 0b00101001,
				RD = 0b01001010,
				LD = 0b10000110,
				DIR4 = 0b00001111,
				DIR8 = 0b11111111,
				ACTS = 0b11111100000000,
			};
			struct {
				bool up : 1; bool dn : 1; bool lf : 1; bool rt : 1;
				bool lu : 1; bool ru : 1; bool rd : 1; bool ld : 1;
				bool a : 1;
				bool b : 1;
				bool c : 1;
				bool d : 1;
				bool ch: 1;
				bool s : 1;
			};
			unsigned short val = 0;
			template<bool oneshot = false> void update(const KeyInputLight&);
			inline bool operator[](int id) const { return val >> id & 1; }
		} cur, prev, one;
		//keyup, bufTimer
		struct Rec{
			enum ID : unsigned char{
				NONE = 0,
				UP, DN, LF, RT, LU, RU, RD, LD,
				A, B, C, D, CH, S,
			} id[10] = {NONE};
			bool expire[10] = { 0 };
			int base = 0;
			int len = 0;
			void update(Cmd, Cmd);
		} record;
		inline void reset() {
			memset(&cur, 0, sizeof(Cmd));
			memset(&record, 0, sizeof(Rec));
		}
	};


	class Panel {
		bool hide = false;
		static TileDesc<256, 64, 32, 32> texBtn;
		//static int texBack;
	public:
		unsigned char enableState = 0;//0, 1, 2, 3
		int direction = 1;
		float slant = 3;
		Vector2f posI, posR;
		Inputs input;

		inline void setPosI(Vector2f p) { posI = p; }
		inline void setPosR(Vector2f p) { posR = p; }
		inline unsigned char switchState(int s = -1) {
			unsigned char old = enableState;
			enableState = s >= 0 ? s : (enableState + 1) % 4;
			return old; 
		}
		//Panel() = default;
		Panel(int dir) : direction(dir)
		{
			texBtn.create(4);
			posI.x = dir > 0 ? 60 : 400; posI.y = 340;
			posR.x = dir > 0 ? 0 : 390; posR.y = 300;
			//input.init();
		}
		~Panel()
		{
			texBtn.cancel();
		}
		void update(const Player&);
		void render() const;
		void renderBack(float x, float y, float cx, float cy) const;
		void renderInputs() const;
		void renderRecord() const;


	};


}}