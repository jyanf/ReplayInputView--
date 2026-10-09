#pragma once
#include <unordered_map>
#include <mutex>
namespace riv {

class Clipper {
public:
	using Player = SokuLib::v2::Player;
	using Clip = std::array<char, sizeof(Player)>;
	using Store = std::unordered_map<Player*, Clip>;

	inline static Store clipper, buffer;
	inline static std::mutex fresh;
	inline static void Insert(Player* p) {
		if (!p) return;
		buffer.insert_or_assign(p, *reinterpret_cast<Clip*>(p));
	}
	inline static void Swap() {
		if (fresh.try_lock()) {
			clipper.swap(buffer);
			fresh.unlock();
		}
		else puts("clip swap failed");
	}
	inline static void Clear() {
		Store a, b;
		clipper.swap(a);
		buffer.swap(b);
	}
	inline static void* get_clipped(Player* p) {
		auto it = clipper.find(p);
		if (it != clipper.end()) {
			return (void*)&it->second;
		}
		return p;
	}
};

}