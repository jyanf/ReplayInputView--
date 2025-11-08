/*
	credit AssistSoku 
*/

#include <cmath>
#include <unordered_map>
#include "box.hpp"
#include "DrawCircle.hpp"

namespace riv { 
	
namespace box {
	//int Texture_armorBar;
	tex::TileDesc<768, 128, 256, 32> ArmorBar;
	int Texture_armorLifebar;
	LayerManager layers;

static const float BOXES_ALPHA = 0.25;
const Color Color_Orange = 0xFFf07000, Color_Gray = 0xFF808080, Color_Pale = 0xFFcccccc, Color_Purple = 0xFFaa00ff;
static auto& colorProfile = iniProxy["ColorProfile"_l];
static RectangleShape rectangle;
	void setCamera() {
		rectangle.setCamera(&SokuLib::camera);
	}

using LagWatcher = std::unordered_map<const GameObjectBase*, bool>;
#define CODEC_COL(o) ((o).collisionType + 100 * (o).collisionLimit)
#define DECODE_COLT(i) ((i)%100)
#define DECODE_COLL(i) ((i)/100)
#define TRUE_CLEAR(m) (LagWatcher().swap(m))
static LagWatcher lag_saver, lag_buffer;
static bool enabled = false, unflushed = false;
void flushWatcher() {
#ifdef _DEBUG
	if (lag_saver.size() + lag_buffer.size())
		printf("lag_saver %d buffer %d\n", lag_saver.size(), lag_buffer.size());
#endif // _DEBUG
	if (!unflushed) return;
	lag_buffer.swap(lag_saver);
	TRUE_CLEAR(lag_buffer);
	unflushed = false;
}
void cleanWatcher() {
	//lag_watcher.clear();
	TRUE_CLEAR(lag_saver); TRUE_CLEAR(lag_buffer);
	enabled = unflushed = false;
}
void closeWatcher() { enabled = false; }
void startWatcher() { cleanWatcher(); enabled = true; }
TrampTamper<7> update_collision_shim(0x47d2c4);
void __fastcall lag_watcher_updator(const GameObjectBase* object) {
	if (!enabled || !object) return;
	auto spec = determine(*object, BulletSpecial::SHARED_BOX | BulletSpecial::SUBBOX | BulletSpecial::EFFECT);
	if (spec.SubBox) {
		object = object->parentA;
		spec = determine(*object, BulletSpecial::SHARED_BOX);
	}
	if (!spec.Effect
		&& check_lag<false>(*object, spec) && object->collisionLimit && !object->collisionType)
		//lag_buffer.erase(object);
		lag_buffer[object] = true;
	unflushed = true;
}


static CNumber& num = *reinterpret_cast<CNumber*>(0x00882940);
inline static void drawNumber(int number, float x, float y, int length = 1) {
	x = (SokuLib::camera.translate.x + x) * SokuLib::camera.scale;
	y = (SokuLib::camera.translate.y + y) * SokuLib::camera.scale;
	num.render(
		number, x, y, length,
		false//is take neg sign
	);
}

inline static BulletSpecial determine(const GameObjectBase& obj, unsigned char interest)
{
	auto fdata = obj.boxData.frameData;
	//auto fdata2 = obj.gameData.frameData;
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
		special.SharedBox &= fdata->frameFlags.atkAsHit;
		special.Melee &= !fdata->attackFlags.grazable;//wrong flag name but nah
		special.Reflector &= fdata->frameFlags.reflectionProjectile;
		special.Entity &= fdata->frameFlags.chOnHit;
		//special.Effect &= !fdata->attackFlags.value && !(special.Reflector || special.Entity);
	} else {
		//special.value &= 0b11111111u;
		special.SharedBox = special.Melee = special.Reflector = special.Entity = false;
	}
	/*if (fdata2) {
		special.SharedBox &= fdata2->frameFlags.atkAsHit;
	} else {
		special.SharedBox = false;
	}*/
	special.SubBox &= obj.parentA != nullptr;

	fdata = obj.gameData.frameData;
	special.Effect &= fdata && fdata->hurtBoxes.size() + fdata->attackBoxes.size() == 0 && obj.customHitBox == nullptr;
	
	return special;
}

template<bool check_sub> inline static void get_collision(const GameObjectBase& object, int& colT, int& colL) {
	bool is_sub = check_sub && determine(object, BulletSpecial::SUBBOX).SubBox;
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
	if (object.boxData.frameData != object.gameData.frameData) return false;//only meaningful when object lag with hurtbox disappeared
	if (spec.Reflector) {
		return true;
	} else if (spec.Entity) {
		return (bool)object.unknown1AC;
	}
	int colL = 0, colT = 0;
	get_collision<false>(object, colT, colL);
	return colL > 0;
}
template<bool checkHitBox = true>
inline static bool check_lag(const GameObjectBase& object, BulletSpecial spec) {//lag: bullet boxInfo not updated
	//return object.boxData.frameData != object.gameData.frameData;
	if (spec.SubBox)
		//return check_lag<checkHitBox>(*object.parentA, determine(*object.parentA, BulletSpecial::SHARED_BOX));
		return false;
	bool hitbox = object.gameData.frameData && object.gameData.frameData->attackBoxes.size() || object.customHitBox != nullptr;
	bool hurtbox =object.gameData.frameData && object.gameData.frameData->hurtBoxes.size();
	if (/*spec.SharedBox*/ object.gameData.frameData && object.gameData.frameData->frameFlags.atkAsHit)
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
static SokuLib::DrawUtils::Rect<SokuLib::Vector2f> precised_vertex(const FloatRect& b, const SokuLib::Vector2f& d) {
	float scale = SokuLib::camera.scale, tx = SokuLib::camera.translate.x, ty = SokuLib::camera.translate.y;
	float	left=	(b.x1 + tx) * scale - d.x,
			top=	(b.y1 + ty) * scale - d.y,
			right=	(b.x2 + tx) * scale + d.x,
			bottom=	(b.y2 + ty) * scale + d.y;
	SokuLib::DrawUtils::Rect<SokuLib::Vector2f> rect{
		{left, top},		{right, top},
		{right, bottom},	{left, bottom},
	};
	return rect;
}
template <int d = 0>
static void drawBox(const Box& box, const RotationBox* rotation, Color borderColor, Color fillColor)
{
	if (rotation) {
		SokuLib::DrawUtils::Rect<SokuLib::Vector2f> rect;
		SokuLib::Vector2f da = rotation->pt1.to<float>(), db = rotation->pt2.to<float>();
		float length = sqrtf(da.x * da.x + da.y * da.y);
		da *= d/length;
		length = sqrtf(db.x * db.x + db.y * db.y);
		db *= d/length;
		da += db; db -= da - db;//da = d1+d2; db=-d1+d2

		float scale = SokuLib::camera.scale, tx = SokuLib::camera.translate.x, ty = SokuLib::camera.translate.y;
		Vector2f base{ tx + box.left, ty + box.top };

		rect.x1 = base * scale; rect.x1 -= da;
		//rect.x1.x = scale * (tx + box.left);
		//rect.x1.y = scale * (ty + box.top);
		//rect.x1.x += -da.x; rect.x1.y += -da.y;
		rect.y1 = (base + rotation->pt1) * scale; rect.y1 -= db;
		//rect.y1.x = scale * (tx + box.left + rotation->pt1.x);
		//rect.y1.y = scale * (ty + box.top + rotation->pt1.y);
		//rect.y1.x += -db.x; rect.y1.y += -db.y;
		rect.x2 = (base + rotation->pt1 + rotation->pt2) * scale; rect.x2 += da;
		//rect.x2.x = scale * (tx + box.left + rotation->pt1.x + rotation->pt2.x);
		//rect.x2.y = scale * (ty + box.top + rotation->pt1.y + rotation->pt2.y);
		//rect.x2.x += da.x; rect.x2.y += da.y;
		rect.y2 = (base + rotation->pt2) * scale; rect.y2 += db;
		//rect.y2.x = scale * (tx + box.left + rotation->pt2.x);
		//rect.y2.y = scale * (ty + box.top + rotation->pt2.y);
		//rect.y2.x += db.x; rect.y2.y += db.y;

		rectangle.rawSetRect(rect);
	} else if constexpr (d) {
		/*float scale = SokuLib::camera.scale, tx = SokuLib::camera.translate.x, ty = SokuLib::camera.translate.y;
		float	left=	(box.left	+ tx) * scale - d,
				top=	(box.top	+ ty) * scale - d,
				right=	(box.right	+ tx) * scale + d,
				bottom=	(box.bottom	+ ty) * scale + d;
		SokuLib::DrawUtils::Rect<SokuLib::Vector2f> rect{
			{left, top},		{right, top},
			{right, bottom},	{left, bottom},
		};*/
		rectangle.rawSetRect(
			precised_vertex( FloatRect(box.left , box.top, box.right, box.bottom), { d,d })
		);
	} else {
		FloatRect rect{
			box.left,		box.top,
			box.right,		box.bottom,
		};//0.5 align?
		rectangle.setRect(rect);//精度尼玛掉完了
	}

	rectangle.setFillColor(fillColor); //if (d) rectangle.fillColors[d - 1] = rectangle.fillColors[d + 1] = Color::Transparent;
	rectangle.setBorderColor(borderColor); //if (d) rectangle.borderColors[d-1] = rectangle.borderColors[d+1] = Color::Transparent;
	rectangle.draw();


	/*if (d) {
		int old;
		old = SetRenderMode(2);
		rectangle.setFillColor(Color::Transparent);
		rectangle.draw();
		SetRenderMode(old);
	}*/
}

void drawPositionBox(const GameObjectBase& object, int s, Color fill, Color border)
{
	rectangle.setFillColor(fill);
	rectangle.setBorderColor(border);
	SokuLib::Vector2f size{ s, s };
	SokuLib::Vector2f pos = object.position;
	//float scale = object.isGui ? 1.f : SokuLib::camera.scale;
	
	if (object.isGui) {
		pos -= size / 2.f;
		rectangle.setCamera(nullptr);
		rectangle.setPosition(pos.to<int>());
		rectangle.setSize(size.to<unsigned>());
		constexpr float rot = 45.0 / 180 * 3.14159;
		rectangle.setRotation(rot);
		rectangle.draw();
		setCamera();
		rectangle.setRotation(0);
	}
	else {
		//size /= scale;
		//pos -= size / 2.f;
		//pos.x = roundf(pos.x); //pos.y = roundf(pos.y);
#ifdef ANCHOR_BORDER_CLAMP
		pos.x = std::clamp(pos.x, 0.f, 1280.f); pos.y = std::clamp(pos.y, SokuLib::camera.bottomEdge, 840.f);
#endif
		pos.x = ceilf(pos.x); pos.y = ceilf(pos.y);
		pos.y *= -1;
		//rectangle.setPosition(pos.to<int>());
		//rectangle.setSize(size.to<unsigned>());
		rectangle.rawSetRect(
			precised_vertex( { pos.x, pos.y, pos.x, pos.y }, size/2 )
		);
		rectangle.draw();
	}
	
}
//template void drawPositionBox<3>(const GameObjectBase& object, Color fill, Color border);
//template void drawPositionBox(const GameObjectBase& object, Color fill, Color border);
//template void drawPositionBox<7>(const GameObjectBase& object, Color fill, Color border);

void drawCollisionBox(const GameObjectBase& object, bool grabInvul, bool hurtbreak)
{
	
	if (!object.gameData.frameData || !object.gameData.frameData->collisionBox) return;
	grabInvul |= object.gameData.frameData->frameFlags.grabInvincible || object.gameData.frameData->frameFlags.guarding;
	FloatRect rect;
	
	if (hurtbreak)//unbuffered state
	{
		const auto& box2 = *(object.gameData.frameData->collisionBox);
		rect = FloatRect(std::ceil(object.position.x) + object.direction * box2.left, box2.top - std::ceil(object.position.y),
			std::ceil(object.position.x) + object.direction * box2.right, box2.bottom - std::ceil(object.position.y));
		//rectangle.setFillColor(Color::Transparent);
		
	}
	else {//correct but require matchState
		const auto& box = object.boxData.collisionBoxBuffer;
		rect = FloatRect( box.left, box.top, box.right, box.bottom );
	}
	rectangle.setRect(rect);
	Color color(colorProfile["CollisionBox"_l]);
	rectangle.setFillColor(grabInvul ? Color::Transparent : color * BOXES_ALPHA);
	rectangle.setBorderColor(color);
	rectangle.draw();
}


bool drawHurtBoxes(const Player& player, bool meleeInvul, bool projnvul)
{
	if (player.boxData.hurtBoxCount > 5)
		return false;
	auto flags = player.gameData.frameData ? &player.gameData.frameData->frameFlags : nullptr;
	Color outline(colorProfile["Hurtbox.Character"_l]), addline = Color::Transparent;
	Color fill = outline, fill2 = Color::Transparent;

	meleeInvul |= flags && flags->meleeInvincible;
	projnvul |= flags && flags->projectileInvincible;
	if (flags) {//invul>atemi>guard>counter
		bool atemiM = flags->invAirborne || flags->invLowBlow || flags->invMidBlow;
		bool atemiB = flags->invShoot;
		if (flags->guardPoint) //guard point
			fill = outline = colorProfile["Hurtbox.Guard"_l];
		else if(flags->chOnHit)
			fill = outline = colorProfile["Hurtbox.Counter"_l];
		if (atemiM || atemiB) {
			if (atemiM && atemiB)
				fill = outline = colorProfile["Hurtbox.Parry"_l];
			else if (atemiM) {//atemiwaza melee
				fill = colorProfile["Hurtbox.Parry.Melee"_l]; //addline = Color::Red;
			} else if (atemiB) {//atemiwaza bullet
				fill = colorProfile["Hurtbox.Parry.Bullet"_l]; //addline = Color::Blue;
			}
		}
		
	}
	if (meleeInvul || projnvul) {
		std::swap(fill, fill2);
		if (meleeInvul && projnvul) {
			
		} else if (meleeInvul) {
			addline = colorProfile["Hurtbox.InvulLine.Melee"_l];
		} else if (projnvul) {
			addline = colorProfile["Hurtbox.InvulLine.Bullet"_l];
		}
	}
	for (int i = 0; i < player.boxData.hurtBoxCount; i++) {
		drawBox(player.boxData.hurtBoxes[i], player.boxData.hurtBoxesRotation[i], outline, fill * BOXES_ALPHA);
		if(addline)
			drawBox<-1>(player.boxData.hurtBoxes[i], player.boxData.hurtBoxesRotation[i], addline, fill2 * BOXES_ALPHA);
	}
	return player.boxData.hurtBoxCount;
}

bool drawHitBoxes(const GameObjectBase& object) {
	if (object.boxData.hitBoxCount > 5)
		return false;
	auto spec = determine(object, BulletSpecial::MELEE);
	Color outline;
	if (object.boxData.frameData && object.boxData.frameData->attackFlags.grab)
		outline = colorProfile["Hitbox.Grab"_l];
	else
		outline = spec.Melee ? colorProfile["Hitbox.Melee"_l] : colorProfile["Hitbox.Bullet"_l];
	
	Color fill = outline;
	if (!check_hitbox_active(object)) {
		if (object.collisionType!=0 && iniProxy["BoxDisplay"_l]["Hitbox.FadeByHitstop"_l])
			fill *= min(1.0, log2f(1 + object.gameData.opponent->hitStop / 10.0f));
		else
			fill = Color::Transparent;
	}
	auto scp = tex::RendererGuard();
	scp.setRenderState(D3DRS_COLORWRITEENABLE, FALSE)
		.setRenderState(D3DRS_STENCILENABLE, TRUE)
		.setRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS)
		.setRenderState(D3DRS_STENCILREF, 1)
		.setRenderState(D3DRS_STENCILMASK, 0xFF)
		.setRenderState(D3DRS_STENCILWRITEMASK, 0xFF)
		.setRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_REPLACE)
		.setRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE)
		.setRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_REPLACE);
	SokuLib::pd3dDev->Clear(0, nullptr, D3DCLEAR_STENCIL, 0, 0, 0);
	//Box obb{};
	fill *= BOXES_ALPHA;
	for (int i = 0; i < object.boxData.hitBoxCount; i++) {
		//obb.left = min(obb.left, object.boxData.hitBoxes[i].left);
		drawBox(object.boxData.hitBoxes[i], object.boxData.hitBoxesRotation[i], 0, fill);
	}
	scp.setRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA)
		.setRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL)
		.setRenderState(D3DRS_STENCILREF, 1)
		.setRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
	const auto& d3dpp = *reinterpret_cast<D3DPRESENT_PARAMETERS*>(0x8a0f68);
	tex::Vertex v[] = {
		{0,0,											0, 1.f, fill, 0,0},
		{0,d3dpp.BackBufferHeight,						0, 1.f, fill, 0,1},
		{d3dpp.BackBufferWidth, 0,						0, 1.f, fill, 1,0},
		{d3dpp.BackBufferWidth,d3dpp.BackBufferHeight,	0, 1.f, fill, 1,1},
	};
	SokuLib::pd3dDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(*v));
	scp.resetRenderState();
	for (int i = 0; i < object.boxData.hitBoxCount; i++) {
		drawBox(object.boxData.hitBoxes[i], object.boxData.hitBoxesRotation[i], outline, 0);
	}
	return object.boxData.hitBoxCount;
}

bool drawBulletBoxes(const GameObject& object, bool hurtbreak)
{
	using BS = BulletSpecial;
	auto spec = determine(object, BS::ENTITY | BS::REFLECTOR | BS::GAP | BS::SHARED_BOX | BS::SUBBOX | BS::MELEE);
	/*if (spec.Effect) {
		lag_saver.erase(&object);
		return false;
	}*/

	bool hitbox_active = check_bullet_hitbox_active(object, spec);
	bool hurtbox_active = check_bullet_hurtbox_active(object, spec);
	
	Color outline;// = Color::Green;
	Color fill;// = active ? outline : Color::Transparent;
	if (spec.Entity) {//entity
		outline = colorProfile["Hurtbox.Entity"_l];
	} else if (spec.Reflector) {//reflector
		outline = colorProfile["Hurtbox.Reflector"_l];
	} else if (spec.Gap) {//gap
		outline = colorProfile["Hurtbox.Gap"_l];
	} else {
		outline = colorProfile["Hurtbox.Object"_l];//0xFF10cd00; //
	}
	fill = hurtbox_active ? outline : Color::Transparent;
	bool drawed = false;
	if (!hurtbreak && object.boxData.hurtBoxCount <= 5) {
		for (int i = 0; i < object.boxData.hurtBoxCount; i++) {
			if (spec.SharedBox)
				drawBox<1>(object.boxData.hurtBoxes[i], object.boxData.hurtBoxesRotation[i], 
					(!hurtbox_active && hitbox_active ? (outline.r *= 0.9, outline.g *= 0.9, outline.b *= 0.9, outline) : outline ), 
					fill * BOXES_ALPHA * (hitbox_active ? 0.6 : 1)
				);
			else
				drawBox(object.boxData.hurtBoxes[i], object.boxData.hurtBoxesRotation[i], outline, fill * BOXES_ALPHA);
			drawed = true;
		}
	}
	if (object.boxData.frameData && object.boxData.frameData->attackFlags.grab)
		outline = colorProfile["Hitbox.Grab"_l];
	else 
		outline = spec.Melee ? colorProfile["Hitbox.Melee"_l] : colorProfile["Hitbox.Bullet"_l];
	fill = outline;
	if (!hitbox_active) {
		if (object.collisionType!=0 && spec.Melee && iniProxy["BoxDisplay"_l]["Hitbox.FadeByHitstop"_l])
			fill *= min(1.0, log2f(1 + object.gameData.opponent->hitStop / 10.0f));
		else
			fill = Color::Transparent;
	}
	if (!hurtbreak && object.boxData.hitBoxCount <= 5) {
		for (int i = 0; i < object.boxData.hitBoxCount; i++) {
			drawBox(object.boxData.hitBoxes[i], object.boxData.hitBoxesRotation[i], outline, fill * BOXES_ALPHA);
			drawed = true;
		}
	}

	//drawPositionBox(object);

	return drawed;
}


/*void drawPlayerBoxes(const Player& player, bool hurtbreak, unsigned char delayedTimers)
{
	drawCollisionBox(player, player.grabInvulTimer || delayedTimers & 4, hurtbreak);
	if (!hurtbreak) {
		drawHurtBoxes(player, player.meleeInvulTimer || delayedTimers & 1, player.projectileInvulTimer || delayedTimers & 2);
		drawHitBoxes(player);
	}
	drawPositionBox(player);

	drawArmor(player, !(player.unknown4AA || delayedTimers & 8) && player.boxData.frameData && player.boxData.frameData->frameFlags.guardAvailable);

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
		withBox |= drawBulletBoxes(obj, hurtbreak);
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
}*/

void drawFloor() {
	Color outline(colorProfile["FloorBox"_l]);
	Color fill = outline;
	Box bound = {-5, 0, 1280, 0};
	//drawBox(bound, nullptr, outline, Color::Transparent);
	
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
void drawArmor(const Player& player, bool blockable) {
	constexpr int threshold = 100;
	if (!player.boxData.frameData) return;
	auto powerMultiplier = player.unknown538;
	if (player.boxData.frameData->frameFlags.superArmor)
		powerMultiplier *= 0.0f;
	else if (player.boxData.frameData->frameFlags.extendedArmor)
		powerMultiplier *= 0.5f;
	bool superArmored = powerMultiplier == 0.0f;
	bool normalArmored = 0.0f < powerMultiplier && powerMultiplier < 1.0f;
	if (!superArmored && !normalArmored) return;
	const auto& armorTimer = player.unknown4BC;
	auto pos = SokuLib::Vector2f{
				(player.position.x + SokuLib::camera.translate.x) * SokuLib::camera.scale,
				(-player.position.y - 100 + SokuLib::camera.translate.y) * SokuLib::camera.scale };
	auto radius = 100 * SokuLib::camera.scale;
	tex::RendererGuard guard; guard.setRenderMode(2);
	if (superArmored 
		//&& SokuLib::activeWeather != SokuLib::WEATHER_TYPHOON 
		//&& !(700 <= player.frameState.actionId && player.frameState.actionId < 800)//in story
	) {
		/*SokuLib::textureMgr.setTexture(Texture_armorBar, 0);*/
		guard.setTexture(ArmorBar);
		//const auto& dmg = player.superArmorDamageTaken;
		Draw2DCircle<threshold, 4>(SokuLib::pd3dDev, pos, radius + player.frameState.currentFrame % 2 * 6,
			25.0f, SokuLib::Vector2f{  20.0f , 340.0f } * -player.direction,
			ArmorBar.getBorder(player.frameState.currentFrame / 3 % 3 + (blockable ? 9 : 6)), 9 / 8.0f, 0xC0FFFFFF);
	} else if (normalArmored) {
		guard.setTexture(Texture_armorLifebar);
		//float u1 = (player.frameState.currentFrame/3 % 3) / 3.0, u2 = (player.frameState.currentFrame/3 % 3 + 1) / 3.0, v1 = 0 / 4.0, v2 = 1 / 4.0;
		float u1 = 0.01, u2 = 1, v1 = 0, v2 = 1;
		float step = threshold * powerMultiplier;

		for (int i = threshold, j = 0; i > 0; i-= step, ++j)
		{
			guard.setRenderMode(2);
			Draw2DCircle<threshold>(SokuLib::pd3dDev, pos, radius + j*15, 20.0f, 
				SokuLib::Vector2f{-1, 359}* -player.direction, 
				{ u1,v1,u2,v2 }, 1, Color_Pale*0.3);
			guard.setRenderMode(1);
			//if (!(50 <= player.frameState.actionId && player.frameState.actionId < 150) && armorTimer < threshold)
				Draw2DCircle<threshold>(SokuLib::pd3dDev, pos, radius+j*15, 20.0f, 
					SokuLib::Vector2f{ -1, -1+max(min((i-armorTimer)/min(step, i), 1), 0) * 360.0f }*-player.direction,
					{ u1,v1,u2,v2 });

		}

	}
}


void LayerManager::pushBullet(const GameObject& object, bool hurtbreak) {
	auto spec = determine(object, BulletSpecial::EFFECT);
	if (spec.Effect) {
		lag_saver.erase(&object);
		return;
	}
	push(Bullets, BulletData{ object,  hurtbreak });
	pushPosition(object);
}

void LayerManager::pushPlayer(const Player& player, bool hurtbreak, unsigned char delayedTimers) {
	pushCollision(player, player.grabInvulTimer || delayedTimers & 4, hurtbreak);
	if (!hurtbreak) {
		pushHurtbox(player, player.meleeInvulTimer || delayedTimers & 1, player.projectileInvulTimer || delayedTimers & 2);
		pushHitbox(player);
	}
	pushPosition(player);
}


}}