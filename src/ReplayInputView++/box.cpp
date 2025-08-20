/*
	credit AssistSoku 
*/

#include <cmath>
#include <map>
#include "box.hpp"

namespace riv { namespace box {

static const float BOXES_ALPHA = 0.25;
static const Color Color_Orange = 0xFFf07000;
static RectangleShape rectangle;
void setCamera() {
	rectangle.setCamera(&SokuLib::camera);
}

using LagWatcher = std::map<const GameObjectBase*, bool>;
#define CODEC_COL(o) ((o).collisionType + 100 * (o).collisionLimit)
#define DECODE_COLT(i) ((i)%100)
#define DECODE_COLL(i) ((i)/100)
#define TRUE_CLEAR(m) (LagWatcher().swap(m))
//static LagWatcher lag_watcher, lag_buffer;
static LagWatcher lag_saver, lag_buffer;
static bool dirty = false;
bool getDirty() { return dirty; }
void setDirty(bool d) {
	dirty = d;
	if (dirty) {
		lag_buffer.swap(lag_saver);
		TRUE_CLEAR(lag_buffer);
#ifdef _DEBUG
		printf("lag_saver %d -------------\n", lag_saver.size());
#endif // _DEBUG

	}
}
void cleanWatcher() {
	//lag_watcher.clear();
	TRUE_CLEAR(lag_saver); TRUE_CLEAR(lag_buffer);
}
unsigned char update_collision_shim[] = {//credit enebe shady/memory.cpp
	0x60,		// 0;pushad
	0x8B, 0xC8,	// 1;mov eax, ecx
	0xE8, 0x90, 0x90, 0x90, 0x90,   // 3;call updator
	0x61,       // 8;popad
	0x90, 0x90, 0x90, 0x90,	0x90, 0x90, 0x90, // 9; org opr
	0xE9, 0x90, 0x90, 0x90, 0x90,	//16;jmp back
};
void __fastcall lag_watcher_updator(const GameObjectBase* object) {
	if (!object) return;
	auto spec = determine(*object, BulletSpecial::SHARED_BOX | BulletSpecial::SUBBOX);
	if (spec.SubBox) {
		object = object->parentA;
		spec = determine(*object, BulletSpecial::SHARED_BOX);
	}
	if (!spec.Effect
		&& check_lag<false>(*object, spec) && object->collisionLimit && !object->collisionType)
		//lag_buffer.erase(object);
		lag_buffer[object] = true;
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

inline static BulletSpecial determine(const GameObjectBase& obj, unsigned char interest)
{
	auto fdata = obj.boxData.frameData;
	//auto fdata = obj.gameData.frameData;
	//auto sdata = obj.boxData.sequenceData;
	auto sdata = obj.gameData.sequenceData;

	//BulletSpecial special{ .value = interest };
	BulletSpecial special(interest);

	special.DynamicBox &= obj.customHitBox != nullptr;

	if (sdata) {
		special.Gap &= sdata->actionLock > 0;
	} else {
		special.Gap = false;
	}
	if (fdata) {
		special.Grab &= fdata->attackFlags.grab;
		special.SharedBox &= fdata->frameFlags.atkAsHit;
		special.Reflector &= fdata->frameFlags.reflectionProjectile;
		special.Entity &= fdata->frameFlags.chOnHit;
		//special.Effect &= !fdata->attackFlags.value && !(special.Reflector || special.Entity);
	} else {
		//special.value &= 0b11111111u;
		special.Grab = special.SharedBox = special.Reflector = special.Entity = false;
	}
	special.SubBox &= obj.parentA != nullptr;

	fdata = obj.gameData.frameData;
	special.Effect &= fdata && fdata->hurtBoxes.size() + fdata->attackBoxes.size() == 0 && obj.customHitBox == nullptr;
	
	return special;
}

inline static void get_collision(const GameObjectBase& object, int& colT, int& colL) {
	bool is_sub = determine(object, BulletSpecial::SUBBOX).SubBox;
	colT = is_sub ? object.parentA->collisionType : object.collisionType;
	colL = is_sub ? object.parentA->collisionLimit : object.collisionLimit;
	if (colT) {
		auto& data = is_sub ? object.parentA->boxData : object.boxData;
		//const auto& data = object.boxData;//no lag for subbox actually
		colT = data.prevCollisionType;
		colL = data.prevCollisionLimit;
	}
	
}
inline static bool check_hitbox_active(const GameObjectBase& object) {
	int colL = 0, colT = 0;
	get_collision(object, colT, colL);
	return colL && !colT;
}
inline static bool check_bullet_hurtbox_active(const GameObjectBase& object, BulletSpecial spec) {
	if (spec.Reflector || spec.Entity) {
		return object.boxData.frameData == object.gameData.frameData;
	}
	int colL = 0, colT = 0;
	get_collision(object, colT, colL);
	return colL;
}
template<bool checkHitBox = true>
inline static bool check_lag(const GameObjectBase& object, BulletSpecial spec) {//lag: bullet boxInfo not updated
	//return object.boxData.frameData != object.gameData.frameData;
	if (spec.SubBox)
		//return check_lag<checkHitBox>(*object.parentA, determine(*object.parentA, BulletSpecial::SHARED_BOX));
		return false;
	bool hitbox = object.gameData.frameData && object.gameData.frameData->attackBoxes.size() || object.customHitBox != nullptr;
	bool hurtbox =object.gameData.frameData && object.gameData.frameData->hurtBoxes.size();
	if (spec.SharedBox)
		hurtbox |= hitbox;

	if (checkHitBox
		&& hitbox && !object.collisionType //guaran by get_collision strategy
		//&& object.collisionLimit
	|| hurtbox 
	) return false;//FUN_0047d0d0

	return true;
}
inline static bool check_bullet_hitbox_active(const GameObjectBase& object, BulletSpecial spec) {
	using CT = GameObjectBase::CollisionType;
	auto* obj = spec.SubBox ? object.parentA : &object;
	spec = spec.SubBox ? determine(*obj, BulletSpecial::SHARED_BOX) : spec;
	if (!check_lag(*obj, spec)) { 
		lag_saver.erase(obj);
		return check_hitbox_active(*obj);
	}
	
	auto it = lag_saver.find(obj);
	return it!=lag_saver.end() && (*it).second;

	//if (spec.SubBox) {//lag from parentA
	//	auto it = lag_watcher.find(object.parentA);
	//	if (it != lag_watcher.end())
	//	{
	//		auto v = (*it).second;
	//		return !DECODE_COLT(v) && DECODE_COLL(v);
	//	}
	//}
	//if (dirty) {
	//	lag_buffer[&object] = CODEC_COL(object);
	//}
	//auto it = lag_watcher.find(&object);
	//if (it != lag_watcher.end())
	//	return !DECODE_COLT((*it).second) && DECODE_COLL((*it).second);
	//return check_hitbox_active(object);
	
}

template <int d = 0>
static void drawBox(const Box& box, const RotationBox* rotation, Color borderColor, Color fillColor)
{
	if (!rotation) {
		float d0 = d / SokuLib::camera.scale;
		FloatRect rect{
			box.left-d0 + 0.5, box.top-d0,
			box.right+d0 + 0.5, box.bottom + d0
		};//0.5 align
		rectangle.setRect(rect);
	}
	else {
		SokuLib::DrawUtils::Rect<SokuLib::Vector2f> rect;
		SokuLib::Vector2f da = rotation->pt1.to<float>(), db = rotation->pt2.to<float>();
		float length = sqrtf(da.x * da.x + da.y * da.y);
		da *= d/length;
		length = sqrtf(db.x * db.x + db.y * db.y);
		db *= d/length;
		da += db; db -= da - db;//da = d1+d2; db=-d1+d2


		rect.x1.x = SokuLib::camera.scale * (SokuLib::camera.translate.x + box.left);
		rect.x1.y = SokuLib::camera.scale * (SokuLib::camera.translate.y + box.top);
		rect.x1.x += -da.x; rect.x1.y += -da.y;

		rect.y1.x = SokuLib::camera.scale * (SokuLib::camera.translate.x + box.left + rotation->pt1.x);
		rect.y1.y = SokuLib::camera.scale * (SokuLib::camera.translate.y + box.top + rotation->pt1.y);
		rect.y1.x += -db.x; rect.y1.y += -db.y;

		rect.x2.x = SokuLib::camera.scale * (SokuLib::camera.translate.x + box.left + rotation->pt1.x + rotation->pt2.x);
		rect.x2.y = SokuLib::camera.scale * (SokuLib::camera.translate.y + box.top + rotation->pt1.y + rotation->pt2.y);
		rect.x2.x += da.x; rect.x2.y += da.y;

		rect.y2.x = SokuLib::camera.scale * (SokuLib::camera.translate.x + box.left + rotation->pt2.x);
		rect.y2.y = SokuLib::camera.scale * (SokuLib::camera.translate.y + box.top + rotation->pt2.y);
		rect.y2.x += db.x; rect.y2.y += db.y;

		rectangle.rawSetRect(rect);
	}

	rectangle.setFillColor(fillColor); //if (d) rectangle.fillColors[d - 1] = rectangle.fillColors[d + 1] = Color::Transparent;
	rectangle.setBorderColor(borderColor); //if (d) rectangle.borderColors[d-1] = rectangle.borderColors[d+1] = Color::Transparent;
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

static bool drawHitBoxes(const GameObjectBase& object)
{
	if (object.boxData.hitBoxCount > 5)
		return false;
	bool active = check_hitbox_active(object);
	auto spec = determine(object, BulletSpecial::GRAB);
	Color outline = spec.Grab ? Color_Orange: Color::Red;
	Color fill = active ? outline : outline * bool(object.collisionType) * min(1.0, log2f(1 + object.gameData.opponent->hitStop / 10.0));
	for (int i = 0; i < object.boxData.hitBoxCount; i++)
		drawBox(object.boxData.hitBoxes[i], object.boxData.hitBoxesRotation[i], outline, fill * BOXES_ALPHA);
	return object.boxData.hitBoxCount;
}

static bool drawBulletBoxes(const GameObject& object)
{
	using BS = BulletSpecial;
	auto spec = determine(object, BS::EFFECT | BS::ENTITY | BS::REFLECTOR | BS::GAP | BS::SHARED_BOX | BS::SUBBOX | BS::GRAB);
	if (spec.Effect) {
		lag_saver.erase(&object);
		return false;
	}

	bool hitbox_active = check_bullet_hitbox_active(object, spec);
	bool hurtbox_active = check_bullet_hurtbox_active(object, spec);
	
	Color outline;// = Color::Green;
	Color fill;// = active ? outline : Color::Transparent;
	if (spec.Entity) {//entity
		outline = Color::Cyan;
	} else if (spec.Reflector) {//reflector
		outline = Color::Blue;
	} else if (spec.Gap) {//gap
		outline = Color::Magenta;
	} else {
		outline = Color::Green;
	}
	fill = hurtbox_active ? outline : Color::Transparent;

	bool drawed = false;
	if (object.boxData.hurtBoxCount <= 5) {
		for (int i = 0; i < object.boxData.hurtBoxCount; i++) {
			if (spec.SharedBox)
				drawBox<1>(object.boxData.hurtBoxes[i], object.boxData.hurtBoxesRotation[i], outline, fill * BOXES_ALPHA * (hitbox_active ? 0.5 : 1));
			else
				drawBox(object.boxData.hurtBoxes[i], object.boxData.hurtBoxesRotation[i], outline, fill * BOXES_ALPHA);
			drawed = true;
		}
	}
	outline = spec.Grab ? Color_Orange : Color::Red;
	fill = hitbox_active ? outline : (object.collisionType ? outline * min(1.0, log2f(1 + object.gameData.opponent->hitStop / 10.0)) : Color::Transparent);
	if (object.boxData.hitBoxCount <= 5) {
		for (int i = 0; i < object.boxData.hitBoxCount; i++) {
			drawBox(object.boxData.hitBoxes[i], object.boxData.hitBoxesRotation[i], outline, fill * BOXES_ALPHA);
			drawed = true;
		}
	}

	drawPositionBox(object);

	return drawed;
}



void drawPlayerBoxes(const Player& player, bool hurtbreak, unsigned char delayedTimers)
{
	drawCollisionBox(player, player.grabInvulTimer || delayedTimers & 4);
	if (!hurtbreak) {
		drawHurtBoxes(player, player.meleeInvulTimer || delayedTimers & 1, player.projectileInvulTimer || delayedTimers & 2);
	}
	drawHitBoxes(player);
	drawPositionBox(player);

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
		withBox |= drawBulletBoxes(obj);
#ifdef _DEBUG
		if (!withBox) continue;
		//drawNumber(obj.frameState.actionId, obj.position.x, -obj.position.y - 20, 3); drawNumber(obj.frameState.sequenceId, obj.position.x+20, -obj.position.y - 20, 2);
		drawNumber(obj.collisionType, obj.position.x, -obj.position.y-10);
		drawNumber((unsigned)obj.collisionLimit, obj.position.x, -obj.position.y);
		drawNumber(obj.boxData.prevCollisionType, obj.position.x+10, -obj.position.y-10);
		drawNumber((unsigned)obj.boxData.prevCollisionLimit, obj.position.x+10, -obj.position.y);

		if (dirty && obj.frameState.actionId >= 800) {
			printf("OBJ %#8x| act%d | seq%d | frm%d | ---------\n"
				"\tobj.box.fdata %#8x, obj.fdata %#8x\n"
				"\tobj.prevType %2d | obj.Type %2d\n"
				"\tobj.prevLmt  %2u | obj.Lmt  %2u\n\n",
				&obj, obj.frameState.actionId, obj.frameState.sequenceId, obj.frameState.poseId,
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
void drawFloor() {
	Color outline = Color::White;
	Color fill = outline;
	Box bound = {-5, 0, 1280, 0};
	drawBox(bound, nullptr, outline, Color::Transparent);
	
	constexpr int size = sizeof(groundHeight) / sizeof(*groundHeight);
	bound.top = -groundHeight[0]; bound.bottom = 200;
	for (int i = 1; i < size; ++i)
	{
		if (*(int*)&groundHeight[i - 1] ^ *(int*)&groundHeight[i]) {
			bound.right = i; fill = outline * min(BOXES_ALPHA, abs(bound.top) / 50);
			drawBox(bound, nullptr, outline, fill);
			bound.left = i; bound.top = -groundHeight[i];
		}
	}
	bound.right = size; fill = outline * min(BOXES_ALPHA, abs(bound.top)/ 50);
	drawBox(bound, nullptr, outline, fill);

	
}


}}