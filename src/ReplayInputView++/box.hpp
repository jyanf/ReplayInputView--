//#include <SokuLib.hpp>
#include <Boxes.hpp>
using Box = SokuLib::Box;
using RotationBox = SokuLib::RotationBox;

#include <GameObject.hpp>
#include <Player.hpp>
using SokuLib::v2::GameObjectBase;
using SokuLib::v2::GameObject;
using SokuLib::v2::Player;

#include <TextureManager.hpp>
#include <DrawUtils.hpp>
using SokuLib::DrawUtils::RectangleShape;
using SokuLib::DrawUtils::FloatRect;
using Color = SokuLib::DrawUtils::DxSokuColor;

#include <Renderer.hpp>
using SokuLib::Renderer;

#include <GameData.hpp>
using SokuLib::v2::groundHeight;
#define MAX_UNTECHBAR_SPAN 100

#include <Design.hpp>
using SokuLib::CNumber;

namespace riv { 
int SetRenderMode(int mode);

namespace box {
	extern int Texture_armorBar, Texture_armorLifebar;
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

static void drawPositionBox(const GameObjectBase& object);

static void drawCollisionBox(const GameObjectBase& object, bool grabInvul, bool hurtbreak = false);
static void drawArmor(const Player& player, bool blockable = true);
static bool drawHurtBoxes(const GameObjectBase& object, bool meleeInvul, bool projnvul);
static bool drawHitBoxes(const GameObjectBase& object);

static bool drawBulletBoxes(const GameObject& object);


}
}