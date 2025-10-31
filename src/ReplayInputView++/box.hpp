#pragma once
#include <GameObject.hpp>
#include <Player.hpp>
#define MAX_UNTECHBAR_SPAN 100
#include <Boxes.hpp>
#include <TextureManager.hpp>
#include <DrawUtils.hpp>
#include <GameData.hpp>

#include <map>
#include <vector>
#include <functional>
#include "tex.hpp"
#include "info.hpp"

namespace riv { 
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

	extern const Color Color_Orange, Color_Gray, Color_Pale , Color_Purple;
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

	void flushWatcher(); void cleanWatcher();
	void closeWatcher(); void startWatcher();
	extern TrampTamper<7> update_collision_shim;
	void __fastcall lag_watcher_updator(const GameObjectBase* object);

	



template<bool check_sub = true> inline static void get_collision(const GameObjectBase& object, int& colT, int& colL);
inline static bool check_hitbox_active(const GameObjectBase& object);

inline static BulletSpecial determine(const GameObjectBase& obj, unsigned char interest = BulletSpecial::ALL);
template<bool> inline static bool check_lag(const GameObjectBase& object, BulletSpecial spec);
	
inline static bool check_bullet_hurtbox_active(const GameObjectBase& object, BulletSpecial spec);
inline static bool check_bullet_hitbox_active(const GameObjectBase& object, BulletSpecial spec);

template <int d>
static void drawBox(const Box& box, const RotationBox* rotation, Color borderColor, Color fillColor);

void drawPositionBox(const GameObjectBase& object, int s=5, Color fill = Color::White, Color border = Color::White + Color::Black);

void drawCollisionBox(const GameObjectBase& object, bool grabInvul, bool hurtbreak = false);
bool drawHurtBoxes(const Player& object, bool meleeInvul, bool projnvul);
bool drawHitBoxes(const GameObjectBase& object);

bool drawBulletBoxes(const GameObject& object, bool hurtbreak = false);
//void drawPlayerBoxes(const Player& player, bool hurtbreak, unsigned char delayTimers);
void drawUntechBar(Player& player);
void drawArmor(const Player& player, bool blockable = true);
void drawFloor();

extern class LayerManager {
public:
	enum Layer : signed char {
		Back = -1,//TODO: hook before game UI render
		Sprite = 0,
		Armor,
		Collision,
		Hurtbox,
		Hitbox,
		Bullets,
		Untech,
		Anchors,
		Top = 0x7F
	};
private:
	class SimpleCallback {
		using Data = void*;
		using Func = void(__fastcall*)(Data);
		Func func = nullptr;
		Data data = nullptr;
	public:
		SimpleCallback(Func fun, void* d, size_t s) : func(fun) {
			data = new char[s];
			memcpy(data, d, s);
		}
		template <typename D> SimpleCallback(D d) : SimpleCallback((Func)D::draw, (Data)&d, sizeof(D)) {}
		~SimpleCallback() {
			delete[] data;
		}
		SimpleCallback(const SimpleCallback&) = delete;
		SimpleCallback& operator=(const SimpleCallback&) = delete;
		SimpleCallback(SimpleCallback&& other) noexcept : func(other.func), data(other.data) {
			other.data = nullptr;
		}
		SimpleCallback& operator=(SimpleCallback&& other) noexcept {
			if (this != &other) {
				delete[] data;
				func = other.func;
				data = other.data;
				other.data = nullptr;
			}
			return *this;
		}
		inline void operator()() {
			return func(data);
		}
	};
	//using Callback = std::function<void()>;
	using Callback = SimpleCallback;
	std::map<Layer, std::vector<Callback>> tasks;
public:
	LayerManager() {
		//tasks[Layer(0)].reserve(4);
	}
	void renderBack();//neg layers
	inline void renderFore() {//0+ layers
		for (auto& [layer, vec] : tasks) {
			if (layer >= 0) {
				for (auto& cmds : vec)
					cmds();
				//vec.clear();
			}
		}
	}
	inline void renderAll() {
		for (auto& [layer, vec] : tasks) {
			for (auto& cmds : vec)
				cmds();
			//vec.clear();
		}
	}
	inline void clear() {
		for (auto& [layer, vec] : tasks)
			vec.clear();
	}
	template <typename D>
	inline void push(Layer layer, D d) {
		tasks[layer].emplace_back(d);
	}
	struct CollisionData {
		const GameObjectBase& object;
		bool grabInvul, hurtbreak;
		static void __fastcall draw(CollisionData* This) {
			drawCollisionBox(This->object, This->grabInvul, This->hurtbreak);
		}
	};
	inline void pushCollision(const GameObjectBase& object, bool grabInvul, bool hurtbreak) {
		push(Collision, CollisionData{ object, grabInvul, hurtbreak });
	}
	struct HurtboxData {
		const Player& object;
		bool meleeInvul, projnvul;
		static void __fastcall draw(HurtboxData* This) {
			drawHurtBoxes(This->object, This->meleeInvul, This->projnvul);
		}
	};
	inline void pushHurtbox(const Player& object, bool grabInvul, bool hurtbreak) {
		push(Hurtbox, HurtboxData{ object, grabInvul, hurtbreak });
	}
	struct HitboxData {
		const GameObjectBase& object;
		static void __fastcall draw(HitboxData* This) {
			drawHitBoxes(This->object);
		}
	};
	inline void pushHitbox(const GameObjectBase& object) {
		push(Hitbox, HitboxData{ object });
	}
	struct ArmorData {
		const Player& player;
		bool blockable;
		static void __fastcall draw(ArmorData* This) {
			drawArmor(This->player, This->blockable);
		}
	};
	inline void pushArmor(const Player& player, bool blockable) {
		push(Armor, ArmorData{ player, blockable });
	}
	struct UntechData {
		Player& player;
		static void __fastcall draw(UntechData* This) {
			drawUntechBar(This->player);
		}
	};
	inline void pushUntech(Player& player) {
		push(Untech, UntechData{ player });
	}
	struct BulletData {
		const GameObject& object;
		bool hurtbreak;
		static void __fastcall draw(BulletData* This) {
			drawBulletBoxes(This->object, This->hurtbreak);
		}
	};
	void pushBullet(const GameObject& object, bool hurtbreak);
	struct PositionData {
		const GameObjectBase& object;
		int size;
		Color fill, border;
		static void __fastcall draw(PositionData* This) {
			drawPositionBox(This->object, This->size, This->fill, This->border);
		}
	};
	inline void pushPosition(const GameObjectBase& object, Color fill=Color::White, Color border=Color_Gray, int s = 6) {
		push(Anchors, PositionData{ object, s, fill, border });
	}
	void pushPlayer(const Player& player, bool hurtbreak, unsigned char delayedTimers);

} layers;

}}