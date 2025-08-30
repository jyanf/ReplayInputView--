#pragma once
//#include <SokuLib.hpp>
#include <GameObject.hpp>
#include <Player.hpp>
#define MAX_UNTECHBAR_SPAN 100
#include <Boxes.hpp>
#include <TextureManager.hpp>
#include <DrawUtils.hpp>
#include <Renderer.hpp>
#include <GameData.hpp>
#include <Design.hpp>

#include "tex.hpp"
#include "info.hpp"

namespace riv { 
using SokuLib::Renderer;
	int SetRenderMode(int mode);

namespace box {
using SokuLib::v2::GameObjectBase;
using SokuLib::v2::GameObject;
using SokuLib::v2::Player;

using Box = SokuLib::Box;
using RotationBox = SokuLib::RotationBox;
using SokuLib::DrawUtils::RectangleShape;
using SokuLib::DrawUtils::FloatRect;
using Color = SokuLib::DrawUtils::DxSokuColor;
using SokuLib::v2::groundHeight;
using SokuLib::CNumber;

	extern const Color Color_Orange, Color_Gray , Color_Purple;
	extern int Texture_armorLifebar;
	extern tex::TileDesc<768, 128, 256, 32> ArmorBar;

	union BulletSpecial {
		struct {
			/* 01 */ bool Melee			: 1;
			/* 02 */ bool SharedBox		: 1;
			/* 04 */ bool DynamicBox	: 1;
			/* 08 */ bool Gap			: 1;
			/* 10 */ bool Reflector		: 1;
			/* 20 */ bool Entity		: 1;
			/* 40 */ bool SubBox		: 1;
			/* 80 */ bool Effect		: 1;
		};
		unsigned char value;
		enum : unsigned char
		{
			NONE=		0,
			MELEE=		0x1,
			SHARED_BOX=	0x2,
			DYNAMIC_BOX=0x4,
			GAP=		0x8,
			REFLECTOR=	0x10,
			ENTITY=		0x20,
			SUBBOX=		0x40,
			EFFECT=		0x80,
			ALL=		0xFF
		};
		inline operator unsigned char() {
			return value;
		}
		BulletSpecial(int v) {
			value = (unsigned char)v;
		}
	};

	void setCamera();

	bool getDirty(); void setDirty(bool d);
	void cleanWatcher();
	extern unsigned char update_collision_shim[];
	void __fastcall lag_watcher_updator(const GameObjectBase* object);

	void drawPlayerBoxes(const Player& player, bool hurtbreak, unsigned char delayTimers);
	void drawUntechBar(Player& player);
	void drawFloor();


inline static void get_collision(const GameObjectBase& object, int& colT, int& colL);
inline static bool check_hitbox_active(const GameObjectBase& object);

inline static BulletSpecial determine(const GameObjectBase& obj, unsigned char interest = BulletSpecial::ALL);
template<bool> inline static bool check_lag(const GameObjectBase& object, BulletSpecial spec);
	
inline static bool check_bullet_hurtbox_active(const GameObjectBase& object, BulletSpecial spec);
inline static bool check_bullet_hitbox_active(const GameObjectBase& object, BulletSpecial spec);

template <int d>
static void drawBox(const Box& box, const RotationBox* rotation, Color borderColor, Color fillColor);

template <int s = 5> void drawPositionBox(const GameObjectBase& object, Color fill = Color::White, Color border = Color::White + Color::Black);

static void drawCollisionBox(const GameObjectBase& object, bool grabInvul, bool hurtbreak = false);
static void drawArmor(const Player& player, bool blockable = true);
static bool drawHurtBoxes(const Player& object, bool meleeInvul, bool projnvul);
static bool drawHitBoxes(const GameObjectBase& object);

static bool drawBulletBoxes(const GameObject& object);


}
}