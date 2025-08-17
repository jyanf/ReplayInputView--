/*
	credit AssistSoku 
*/

#include <cmath>

#include "box.hpp"

namespace riv { namespace box {

static const float BOXES_ALPHA = 0.25;
static RectangleShape rectangle;
void setCamera() {
	rectangle.setCamera(&SokuLib::camera);
}

static CNumber& num = *reinterpret_cast<CNumber*>(0x00882940);
const auto& CDrawNumber = SokuLib::union_cast<void(__thiscall CNumber::*)(int, float, float, int, bool)>(0x00414940);
inline static void drawNumber(int number, float x, float y, int length = 1) {
	x = (SokuLib::camera.translate.x + x) * SokuLib::camera.scale;
	y = (SokuLib::camera.translate.y + y) * SokuLib::camera.scale;

	(num.*CDrawNumber)(
		number, x, y, length,
		false//is take neg sign
	);
}



inline static bool check_active(const GameObjectBase& object) {
	int colL = 0, colT = 0;
	if (object.parentA) {
		colL = object.parentA->boxData.prevCollisionLimit;
		colT = object.parentA->boxData.prevCollisionType;
	}
	else {
		colL = object.boxData.prevCollisionLimit;
		colT = object.boxData.prevCollisionType;
	}

	if (object.boxData.frameData != object.gameData.frameData) {
		return false;
	}
	
	return colL && !colT;
}

SpecialBullet determine(const GameObject& obj)
{
	auto fdata1 = obj.gameData.frameData;
	auto fdata2 = obj.boxData.frameData;
	auto sdata1 = obj.gameData.sequenceData;
	auto sdata2 = obj.boxData.sequenceData;

	SpecialBullet special{0};
	special.value |= (fdata1 && fdata1->frameFlags.atkAsHit)>> 1//SharedBox
					| (obj.customHitBox != nullptr)			>> 2//DynamicBox
					| (sdata1 && sdata1->actionLock > 0)	>> 3//Gap
					| (fdata1 && fdata1->frameFlags.reflectionProjectile)	>> 4//Reflector
					| (fdata1 && fdata1->frameFlags.chOnHit)>> 5//Entity
					| (obj.parentA != nullptr)				>> 6//SubBox
		;
	return special;
}

static void drawBox(const Box& box, const RotationBox* rotation, Color borderColor, Color fillColor)
{
	if (!rotation) {
		FloatRect rect{
			box.left, box.top,
			box.right, box.bottom
		};
		rectangle.setRect(rect);
	}
	else {
		SokuLib::DrawUtils::Rect<SokuLib::Vector2f> rect;

		rect.x1.x = SokuLib::camera.scale * (SokuLib::camera.translate.x + box.left);
		rect.x1.y = SokuLib::camera.scale * (SokuLib::camera.translate.y + box.top);

		rect.y1.x = SokuLib::camera.scale * (SokuLib::camera.translate.x + box.left + rotation->pt1.x);
		rect.y1.y = SokuLib::camera.scale * (SokuLib::camera.translate.y + box.top + rotation->pt1.y);

		rect.x2.x = SokuLib::camera.scale * (SokuLib::camera.translate.x + box.left + rotation->pt1.x + rotation->pt2.x);
		rect.x2.y = SokuLib::camera.scale * (SokuLib::camera.translate.y + box.top + rotation->pt1.y + rotation->pt2.y);

		rect.y2.x = SokuLib::camera.scale * (SokuLib::camera.translate.x + box.left + rotation->pt2.x);
		rect.y2.y = SokuLib::camera.scale * (SokuLib::camera.translate.y + box.top + rotation->pt2.y);
		rectangle.rawSetRect(rect);
	}

	rectangle.setFillColor(fillColor);
	rectangle.setBorderColor(borderColor);
	rectangle.draw();
}

static void drawCollisionBox(const GameObjectBase& object, bool grabInvul)
{
	if (!object.gameData.frameData || !object.gameData.frameData->collisionBox) return;
	auto& box = *(object.gameData.frameData->collisionBox);
	//auto& box = object.boxData.collisionBoxPtr ? *object.boxData.collisionBoxPtr : object.boxData.collisionBoxBuffer;//??? testing

	FloatRect rect{
		std::ceil(object.position.x) + object.direction * box.left, box.top - std::ceil(object.position.y),
		std::ceil(object.position.x) + object.direction * box.right, box.bottom - std::ceil(object.position.y)
	};
	rectangle.setRect(rect);

	rectangle.setFillColor(object.gameData.frameData->frameFlags.grabInvincible || grabInvul ? Color::Transparent : Color::Yellow * BOXES_ALPHA);
	rectangle.setBorderColor(Color::Yellow);
	rectangle.draw();
}

static void drawPositionBox(const GameObjectBase& object)
{
	SokuLib::Vector2u size{ 5, 5 };
	SokuLib::Vector2i pos{ object.position.x - size.x / 2, -object.position.y - size.y / 2 };
	rectangle.setPosition(pos);
	rectangle.setSize(size);

	rectangle.setFillColor(Color::White);
	rectangle.setBorderColor(Color::White + Color::Black);
	rectangle.draw();
}

static bool drawHurtBoxes(const GameObjectBase& object, bool meleeInvul, bool projnvul)
{
	if (object.boxData.hurtBoxCount > 5)
		return false;
	auto flags = object.gameData.frameData ? &object.gameData.frameData->frameFlags : nullptr;
	Color outline = flags && flags->chOnHit ? Color::Cyan : Color::Green;
	Color fill = (flags && flags->meleeInvincible || meleeInvul) ? Color::Transparent : outline;
	for (int i = 0; i < object.boxData.hurtBoxCount; i++)
		drawBox(
			object.boxData.hurtBoxes[i],
			object.boxData.hurtBoxesRotation[i],
			outline,
			fill * BOXES_ALPHA
		);
	return object.boxData.hurtBoxCount;
}
static bool drawBulletBoxes(const GameObject& object)
{
	if (object.boxData.hurtBoxCount > 5)
		return false;
	bool active = check_active(object);
	
	Color outline = Color::Green;
	Color fill = active ? outline : Color::Transparent;
	if (object.gameData.frameData)
	{
		const auto& flags = object.gameData.frameData->frameFlags;
		if (flags.chOnHit) {//entity
			outline = Color::Cyan;
			fill = outline;
		} else if (flags.reflectionProjectile) {//reflector
			outline = Color::Blue;
			fill = outline;
		} else if (object.gameData.sequenceData && object.gameData.sequenceData->actionLock > 0) {//gap
			outline = Color::Magenta;
			fill = active ? outline : Color::Transparent;
		}
	}

	for (int i = 0; i < object.boxData.hurtBoxCount; i++)
		drawBox(
			object.boxData.hurtBoxes[i],
			object.boxData.hurtBoxesRotation[i],
			outline, fill * BOXES_ALPHA
		);
	return object.boxData.hurtBoxCount;
}

static bool drawHitBoxes(const GameObjectBase& object)
{
	if (object.boxData.hitBoxCount > 5)
		return false;
	bool active = check_active(object);
	for (int i = 0; i < object.boxData.hitBoxCount; i++)
		drawBox(
			object.boxData.hitBoxes[i],
			object.boxData.hitBoxesRotation[i],
			Color::Red,
			active ? Color::Red * BOXES_ALPHA : Color::Transparent);
	return object.boxData.hitBoxCount;
}

void drawPlayerBoxes(const Player& player, bool playerBoxes)
{
	if (playerBoxes) {//config
		drawCollisionBox(player, player.grabInvulTimer);
		drawHurtBoxes(player, player.meleeInvulTimer, player.projectileInvulTimer);
		drawHitBoxes(player);
		drawPositionBox(player);
	}

	if (!player.objectList) return;
	const auto& list = player.objectList->getList();//manager.objects.list.vector();

	for (const auto elem : list) {
		if (!elem) continue;
		const auto& obj = *elem;
		auto fdata = obj.gameData.frameData;
		auto sdata = obj.gameData.sequenceData;

		bool withBox = false;

		//TODO: config
		if (!obj.lifetime) return;
		if (obj.collisionLimit //possible case: 
		|| fdata && (fdata->attackFlags.value || fdata->frameFlags.value & 0x00080040)// normal / entity / reflector
		|| sdata && (sdata->actionLock || sdata->moveLock)//gap / umbrella
		) {
			withBox |= drawBulletBoxes(obj);
		}
		if (obj.collisionLimit 
		|| fdata && fdata->attackFlags.value
		) {//filter fx obj
			withBox |= drawHitBoxes(obj);
			drawPositionBox(obj);
		}
		if (!withBox) continue;
#ifdef _DEBUG
		//drawNumber(obj.frameState.actionId, obj.position.x, -obj.position.y - 20, 3); drawNumber(obj.frameState.sequenceId, obj.position.x+20, -obj.position.y - 20, 2);
		drawNumber(obj.collisionType, obj.position.x, -obj.position.y-10);
		drawNumber(obj.collisionLimit, obj.position.x, -obj.position.y);
		drawNumber(obj.boxData.prevCollisionType, obj.position.x+10, -obj.position.y-10);
		drawNumber(obj.boxData.prevCollisionLimit, obj.position.x+10, -obj.position.y);
		if (obj.frameState.actionId >= 800) {
			printf("OBJ %#8x| act%d | seq%d | frm%d | ---------\n"
				"\tobj.box.fdata %#8x, obj.fdata %#8x\n"
				"\tobj.prevType %2d | obj.Type %2d\n"
				"\tobj.prevLmt  %2d | obj.Lmt  %2d\n\n",
				&obj, obj.frameState.actionId, obj.frameState.sequenceId, obj.frameState.poseFrame,
				obj.boxData.frameData, obj.gameData.frameData,
				obj.boxData.prevCollisionType, obj.collisionType,
				obj.boxData.prevCollisionLimit, obj.collisionLimit
			);

		}
#endif // _DEBUG
		
	}
}

void drawUntechBar(Player& player) {
	int untech = player.untech;
	if (untech > MAX_UNTECHBAR_SPAN) {
		untech = MAX_UNTECHBAR_SPAN;
	}

	float x = player.position.x;
	float y = player.position.y;

	if (player.isGrounded() && player.speed.y < 0
	|| !(player.gameData.frameData && player.gameData.frameData->frameFlags.airborne)
	|| player.damageLimited
	|| !(50 <= player.frameState.actionId && player.frameState.actionId < 150)
	) {
		return;
	}
	float w_max = 300.0f;
	float w = float(untech) / MAX_UNTECHBAR_SPAN * w_max;
	float h = 5.0f;

	Color color = Color::Yellow;

	if (untech > 50) {
		color.r = (100 - untech) / 50.0f * 255;
	}
	else {
		unsigned char value = (unsigned char)((untech / 50.0f) * 255.0f);
		color.g = untech / 50.0f * 255;
	}
	FloatRect rect{
		x - w / 2.0f,
		-y,
		0,
		0
	}; rect.x2 = rect.x1 + w; rect.y2 = rect.y1 + h;

	rectangle.setBorderColor(Color::Transparent);
	rectangle.setRect(rect); rectangle.setFillColor(Color::Black);
	rectangle.draw();
	rect.y2 -= 1.0f;
	rectangle.setRect(rect); rectangle.setFillColor(color);
	rectangle.draw();
}



}}