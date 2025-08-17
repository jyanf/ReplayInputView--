//#include <SokuLib.hpp>
#include <Boxes.hpp>
using Box = SokuLib::Box;
using RotationBox = SokuLib::RotationBox;

#include <GameObject.hpp>
#include <Player.hpp>
using GameObjectBase = SokuLib::v2::GameObjectBase;
using GameObject = SokuLib::v2::GameObject;
using Player = SokuLib::v2::Player;

#include <DrawUtils.hpp>
using SokuLib::DrawUtils::RectangleShape;
using SokuLib::DrawUtils::FloatRect;
using Color = SokuLib::DrawUtils::DxSokuColor;

#define MAX_UNTECHBAR_SPAN 100

#include <Design.hpp>
using SokuLib::CNumber;

namespace riv { namespace box {

	union SpecialBullet {
		struct {
			/* 01 */ bool SharedBox		: 1;
			/* 02 */ bool DynamicBox	: 1;
			/* 04 */ bool Gap			: 1;
			/* 08 */ bool Reflector		: 1;
			/* 10 */ bool Entity		: 1;
			/* 20 */ bool SubBox		: 1;
			/* 40 */ 
			/* 80 */ 
		};
		unsigned char value;
		
	};
	SpecialBullet determine(const GameObject& obj);

static void drawBox(const Box& box, const RotationBox* rotation, Color borderColor, Color fillColor);


static void drawCollisionBox(const GameObjectBase& object, bool grabInvul);

static void drawPositionBox(const GameObjectBase& object);

static bool drawHurtBoxes(const GameObjectBase& object, bool meleeInvul, bool projnvul);
static bool drawBulletBoxes(const GameObject& object);

static bool drawHitBoxes(const GameObjectBase& object);


void drawPlayerBoxes(const Player& player, bool playerBoxes = true);

void drawUntechBar(Player& player);
void setCamera();
}}