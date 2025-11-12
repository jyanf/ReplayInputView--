#include "gui.hpp"

#include <boost/lexical_cast.hpp>
#include <regex>
#include <unordered_map>

#include <Windows.h>
#include <GameObject.hpp>
#include <DrawUtils.hpp>

namespace gui {
	std::vector<std::wstring> Font::LoadAllFontsInFolder(const std::wstring& folderPath, bool recursive) {
		std::vector<std::wstring> loadedFonts;
		WIN32_FIND_DATAW fd;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		std::wstring searchPath = folderPath + L"\\*";

		hFind = FindFirstFileW(searchPath.c_str(), &fd);
		if (hFind == INVALID_HANDLE_VALUE)
			return loadedFonts;

		do {
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if (recursive && wcscmp(fd.cFileName, L".") != 0 && wcscmp(fd.cFileName, L"..") != 0) {
					auto sub = LoadAllFontsInFolder(folderPath + L"\\" + fd.cFileName, true);
					loadedFonts.insert(loadedFonts.end(), sub.begin(), sub.end());
				}
			}
			else {
				std::wstring filename = fd.cFileName;
				std::wstring ext = filename.substr(filename.find_last_of(L".") + 1);
				for (auto& c : ext) c = towlower(c);

				if (ext == L"ttf" || ext == L"otf" || ext == L"ttc") {
					std::wstring fullPath = folderPath + L"\\" + filename;
					if (AddFontResourceExW(fullPath.c_str(), FR_PRIVATE, 0))
						loadedFonts.push_back(fullPath);
				}
			}
		} while (FindNextFileW(hFind, &fd) != 0);

		FindClose(hFind);
		return loadedFonts;
	}


	UINT xml::XmlHelper::gameCP = CP_ACP, xml::XmlHelper::fileCP = CP_UTF8;

	SokuLib::Sprite Container::helper;

	std::map<int, Font> RivDesign::_fonts;
	CDesign* RivDesign::_current = nullptr;
	Console* RivDesign::_console = nullptr;
	_hover* RivDesign::_hoverbuffer = nullptr;
	ValueSpec_PlayerClip::Clipper ValueSpec_PlayerClip::clipper, ValueSpec_PlayerClip::buffer;
	//HWND Console::hwndTT = NULL;
#define SIMPLE_IS_PLAYER(o) ((o).frameState.actionId< 800)
#define SIMPLE_IS_OBJECT(o) ((o).frameState.actionId>=800)
	_hider::manager& _hider::Rules() {
		static manager Rules{
			//{"always", [](void*) { return true; }},
			//{"never", [](void*) { return false; }},
			/*{"looping-sequence", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.sequenceData && obj.gameData.sequenceData->isLoop;
			}},*/

			{"flag-stance-standing", [](void* ptr) {
				auto& obj= *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData && obj.gameData.frameData->frameFlags.stand && !obj.gameData.frameData->frameFlags.crouch;
			}},
			{"flag-stance-crouching", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData && obj.gameData.frameData->frameFlags.crouch && !obj.gameData.frameData->frameFlags.stand;
			}},
			{"flag-stance-both", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData && obj.gameData.frameData->frameFlags.crouch && obj.gameData.frameData->frameFlags.stand;
			}},
			{"flag-stance-down", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData && obj.gameData.frameData->frameFlags.down;
			}},
			{"flag-stance-guarding", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData && obj.gameData.frameData->frameFlags.guarding;
			}},
			{"flag-stance-none", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				if (!obj.gameData.frameData) return false;
				auto flags = obj.gameData.frameData->frameFlags;
				return SIMPLE_IS_PLAYER(obj)
					&& !flags.crouch 
					&& !flags.stand
					&& !flags.down
					&& !flags.guarding
					&& !flags.airborne;
			}},
			{"flag-stance-airborne", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData && obj.gameData.frameData->frameFlags.airborne;
			}},
			{"flag-invul-melee", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData && obj.gameData.frameData->frameFlags.meleeInvincible;
			}},
			{"flag-invul-bullet", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData && obj.gameData.frameData->frameFlags.projectileInvincible;
			}},
			{"flag-invul-grab", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData && obj.gameData.frameData->frameFlags.grabInvincible;
			}},
			{"flag-grazing", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData && obj.gameData.frameData->frameFlags.graze;
			}},
			{"flag-armor-normal", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData && obj.gameData.frameData->frameFlags.extendedArmor;
			}},
			{"flag-armor-super", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData && obj.gameData.frameData->frameFlags.superArmor;
			}},
			{"flag-guardpoint", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData && obj.gameData.frameData->frameFlags.guardPoint;
			}},
			{"flag-guard-available", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData && obj.gameData.frameData->frameFlags.guardAvailable && !obj.gameData.frameData->frameFlags.guarding;
			}},
			{"flag-counter-hit", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return SIMPLE_IS_PLAYER(obj) && obj.gameData.frameData && obj.gameData.frameData->frameFlags.chOnHit;
			}},
			{"flag-cancelable", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData && obj.gameData.frameData->frameFlags.cancellable;
			}},
			{"flag-uncancelable", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return SIMPLE_IS_PLAYER(obj) && obj.gameData.frameData && !obj.gameData.frameData->frameFlags.cancellable;
			}},
			{"flag-move-cancelable", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData && obj.gameData.frameData->frameFlags.highJumpCancellable;
			}},
			{"flag-parry-high", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return SIMPLE_IS_PLAYER(obj) && obj.gameData.frameData && obj.gameData.frameData->frameFlags.invMidBlow;
			}},
			{"flag-parry-low", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return SIMPLE_IS_PLAYER(obj) && obj.gameData.frameData && obj.gameData.frameData->frameFlags.invLowBlow;
			}},
			{"flag-parry-air", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return SIMPLE_IS_PLAYER(obj) && obj.gameData.frameData && obj.gameData.frameData->frameFlags.invAirborne;
			}},
			{"flag-parry-obj", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return SIMPLE_IS_PLAYER(obj) && obj.gameData.frameData && obj.gameData.frameData->frameFlags.invShoot;
			}},
			{"flag-reflector", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return SIMPLE_IS_OBJECT(obj) && obj.gameData.frameData && obj.gameData.frameData->frameFlags.reflectionProjectile;
			}},
			{"flag-entity", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return SIMPLE_IS_OBJECT(obj) && obj.gameData.frameData && obj.gameData.frameData->frameFlags.chOnHit;
			}},
#define IS_BULLET 0x400000
#define IGNORE_BULLET_INVUL 0x40000
			{"attack-melee", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData && !(obj.gameData.frameData->attackFlags.grazable);
			}},
			{"attack-bullet", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData 
					&& !(obj.gameData.frameData->attackFlags.unk40000)
					&& (obj.gameData.frameData->attackFlags.grazable );
			}},
			{"attack-grab", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData && obj.gameData.frameData->attackFlags.grab;
			}},
			{"attack-highRB", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData 
					&& !obj.gameData.frameData->attackFlags.lowHit
					&& obj.gameData.frameData->attackFlags.midHit;
			}},
			{"attack-lowRB", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.gameData.frameData->attackFlags.lowHit
					&& !obj.gameData.frameData->attackFlags.midHit;
			}},
			{"attack-noRB", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& !obj.gameData.frameData->attackFlags.midHit
					&& !obj.gameData.frameData->attackFlags.lowHit;
			}},
			{"attack-bothRB", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.gameData.frameData->attackFlags.midHit
					&& obj.gameData.frameData->attackFlags.lowHit;
			}},
			{"attack-stagger", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.gameData.frameData->attackFlags.unk1;
			}},
			{"attack-knockback", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& !obj.gameData.frameData->attackFlags.unk1;
			}},
			{"attack-ungrazable", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.gameData.frameData->attackFlags.unk800000;
			}},
			{"attack-drain", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.gameData.frameData->attackFlags.unk1000000;
			}},		
			{"attack-counter-hit", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.gameData.frameData->attackFlags.crashHit;
			}},
			{"attack-unreflectable", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.gameData.frameData->attackFlags.unk20000;
			}},
			{"attack-friendly-fire", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.gameData.frameData->attackFlags.hitsAll;
			}},
			{"attack-unblockable", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.gameData.frameData->attackFlags.unblockable;
			}},
			{"attack-air-unblockable", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.boxData.hitBoxCount > 0
					&& !obj.gameData.frameData->attackFlags.unblockable
					&& !obj.gameData.frameData->attackFlags.airBlockable;
			}},
			{"attack-hitWB", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.gameData.frameData->attackFlags.unk40;
			}},
			{"attack-crushWB", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.gameData.frameData->attackFlags.guardCrush;
			}},
			{"attack-ignore-armor", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.gameData.frameData->attackFlags.unk20;
			}},
			{"attack-skill", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.gameData.frameData->attackFlags.unk800;
			}},
			{"attack-spell", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.gameData.frameData->attackFlags.unk1000;
			}},
			{"attack-parry-air", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.gameData.frameData->attackFlags.unk2000;
			}},
			{"attack-parry-high", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.gameData.frameData->attackFlags.unk4000;
			}},
			{"attack-parry-low", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.gameData.frameData->attackFlags.knockBack;
			}},
			{"attack-parry-obj", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::GameObjectBase*>(ptr);
				return obj.gameData.frameData
					&& obj.gameData.frameData->attackFlags.unk10000;
			}},

			{"center", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::AnimationObject*>(ptr);
				return abs(obj.center.x) > FLT_EPSILON || abs(obj.center.y) > FLT_EPSILON;
			}},
			{"blend-options", [](void* ptr) {
				auto& obj = *reinterpret_cast<SokuLib::v2::AnimationObject*>(ptr);
				using FD = SokuLib::v2::FrameData;
				return obj.frameData && obj.frameData->renderGroup==FD::WITHBLEND && obj.frameData->blendOptionsPtr;
			}},
			{"result-uncancelable", [](void* ptr){
				using data = SokuLib::v2::GameObjectBase;
				auto& obj = *reinterpret_cast<data*>(ptr);
				return SIMPLE_IS_PLAYER(obj) && obj.frameState.actionId >= 300
					&& (obj.collisionType == data::COLLISION_TYPE_NONE 
						|| obj.collisionType == data::COLLISION_TYPE_INVUL);
			}},
			{"lock-movement", [](void* ptr){
				using data = SokuLib::v2::GameObjectBase;
				auto& obj = *reinterpret_cast<data*>(ptr);
				return SIMPLE_IS_PLAYER(obj) && obj.speed.y > 0 && obj.position.y <= 100.0f;
			}},
			{{"lock-border-escape"}, [](void* ptr){
				using data = SokuLib::v2::Player;
				auto& obj = *reinterpret_cast<data*>(ptr);
				return SIMPLE_IS_PLAYER(obj) && obj.isBELocked;
			}},
			{"is-in-untech", [](void* ctx) {
				using data = SokuLib::v2::GameObjectBase;
				auto& obj = *reinterpret_cast<data*>(ctx);
				return 50 <= obj.frameState.actionId && obj.frameState.actionId < 150;
			}},

		};
		return Rules;
	}
	
	namespace xml {
		UINT XmlHelper::get_doc_codepage(const std::string& s) {
			static const std::unordered_map<std::string, int> encoding_to_codepage = {
				// === Unicode ===
				{"utf-8", 65001}, {"utf8", 65001}, {"utf_8", 65001},
				{"utf-7", 65000}, {"utf7", 65000}, {"utf_7", 65000},
				{"utf-16", 1200}, {"utf16", 1200}, {"utf_16", 1200}, {"utf16le", 1200}, {"utf-16le", 1200},
				{"utf-16be", 1201}, {"utf16be", 1201},
				{"utf-32", 12000}, {"utf32", 12000}, {"utf-32be", 12001}, {"utf32be", 12001},

				// === Windows ===
				{"windows-1250", 1250}, {"win1250", 1250}, {"cp1250", 1250},
				{"windows-1251", 1251}, {"win1251", 1251}, {"cp1251", 1251},
				{"windows-1252", 1252}, {"win1252", 1252}, {"cp1252", 1252},
				{"windows-1253", 1253}, {"win1253", 1253}, {"cp1253", 1253},
				{"windows-1254", 1254}, {"win1254", 1254}, {"cp1254", 1254},
				{"windows-1255", 1255}, {"win1255", 1255}, {"cp1255", 1255},
				{"windows-1256", 1256}, {"win1256", 1256}, {"cp1256", 1256},
				{"windows-1257", 1257}, {"win1257", 1257}, {"cp1257", 1257},
				{"windows-1258", 1258}, {"win1258", 1258}, {"cp1258", 1258},

				// === East Asian Encodings ===
				{"shift_jis", 932}, {"shift-jis", 932}, {"sjis", 932}, {"ms932", 932},
				{"gb2312", 936}, {"gbk", 936}, {"gb_2312", 936},
				{"big5", 950}, {"big-5", 950}, {"big_5", 950},
				{"euc-jp", 51932}, {"euc_jp", 51932},
				{"euc-kr", 51949}, {"euc_kr", 51949},
				{"euc-cn", 51936}, {"euc_cn", 51936},
				{"iso-2022-jp", 50220}, {"iso_2022_jp", 50220},
				{"iso-2022-kr", 50225}, {"iso_2022_kr", 50225},
				{"hz-gb-2312", 52936}, {"hz_gb_2312", 52936},
				{"gb18030", 54936},

				// === ISO Latin ===
				{"iso-8859-1", 28591}, {"latin1", 28591}, {"iso8859-1", 28591},
				{"iso-8859-2", 28592}, {"latin2", 28592},
				{"iso-8859-5", 28595}, {"cyrillic", 28595},
				{"iso-8859-7", 28597}, {"greek", 28597},
				{"iso-8859-8", 28598}, {"hebrew", 28598},
				{"iso-8859-9", 28599}, {"turkish", 28599},
				{"iso-8859-15", 28605}, {"latin9", 28605},

				// === DOS / OEM ===
				{"ibm437", 437}, {"cp437", 437},
				{"ibm850", 850}, {"cp850", 850},
				{"ibm852", 852}, {"cp852", 852},
				{"ibm866", 866}, {"cp866", 866},
				{"dos-720", 720},
				{"dos-862", 862},
				{"ibm864", 864},
				{"koi8-r", 20866}, {"koi8r", 20866},
				{"koi8-u", 21866}, {"koi8u", 21866},

				// === Mac ===
				{"macintosh", 10000}, {"mac", 10000}, {"x-mac-japanese", 10001},
				{"x-mac-chinesetrad", 10002}, {"x-mac-chinesesimp", 10008},
				{"x-mac-korean", 10003}, {"x-mac-cyrillic", 10007},
				{"x-mac-greek", 10006}, {"x-mac-hebrew", 10005},

				// === ASCII ===
				{"us-ascii", 20127}, {"ascii", 20127}, {"ansi_x3.4-1968", 20127},
			};
			//auto declOpt = doc.get_optional<string>("<xmlattr>.encoding");
			std::smatch m;
			if (std::regex_search(s, m, std::regex(R"(encoding\s*=\s*["']([^"']+)["'])"))) {
				auto name = m[1].str(); std::transform(name.begin(), name.end(), name.begin(), tolower);
				auto it = encoding_to_codepage.find(name);
				if (it != encoding_to_codepage.end()) {
					std::cout << "encoding=" << name <<"; cp=" << it->second <<std::endl;
					return it->second;
				}
			}
			return CP_UTF8;
		}
		std::istringstream XmlHelper::read_file(const std::string& name) {
			IFileReader* reader = nullptr;
			printf("Reading file: %s\n", name.c_str());
			if (!_init_reader(&reader, 0, name.c_str())) {
				delete reader;
				throw std::runtime_error("failed to init reader");
			}
			auto length = reader->GetLength();
			std::string buf(length, '\0');
			if (!reader->Read(buf.data(), length)) {
				delete reader;
				throw std::runtime_error("failed to read file");
			}
			delete reader;
			decrypt(buf);
			fileCP = get_doc_codepage(buf);
			return std::istringstream(buf); // 将字符串内容绑定进流
		}

		void XmlHelper::decrypt(std::string& buf) {//credit shady-packer
			// Decrypt
			uint8_t a = 0x8b, b = 0x71;
			//auto len = buf.length();
			for (auto& c : buf) {
				c ^= a;
				a += b;
				b -= 0x6b;
			}
		}

		template<typename T>
		std::vector<T> XmlHelper::get_array(const ptree& node, const std::string& key, char sep) {
			std::vector<T> ret;
			auto attrsOpt = node.get_child_optional("<xmlattr>");
			if (!attrsOpt)
				return ret;
			auto valOpt = attrsOpt->get_optional<string>(key);
			if (!valOpt)
				return ret;

			std::stringstream ss(*valOpt);
			std::string item;
			while (std::getline(ss, item, sep)) {
				//strip
				size_t start = item.find_first_not_of(" \t");
				size_t end = item.find_last_not_of(" \t");
				if (start != std::string::npos && end >= start) {
					item = item.substr(start, end - start + 1);
					try {
						ret.push_back(boost::lexical_cast<T>(item));
					}
					catch (...) {/*忽略单个转换失败*/ }
				}
			}
			return ret;
		}
	}

	void Font::load(const xml::ptree& node) {
		//static std::set<string> Loaded;
		setFontName(node.get_optional<string>("<xmlattr>.name").value_or("").c_str());
		height = node.get_optional<int>("<xmlattr>.height").value_or(height);
		weight = node.get_optional<bool>("<xmlattr>.bold").value_or(false) ? 600 : 400;
		useOffset = node.get_optional<bool>("<xmlattr>.wrap").value_or((bool)useOffset);
		shadow = node.get_optional<bool>("<xmlattr>.stroke").value_or((bool)shadow);
		charSpaceX = node.get_optional<int>("<xmlattr>.xspacing").value_or(node.get_optional<int>("<xmlattr>.spacing").value_or(charSpaceX));
		charSpaceY = node.get_optional<int>("<xmlattr>.yspacing").value_or(charSpaceY);
		setColor(
			xml::XmlHelper::get_hex(node, "color_top").value_or(0xFFFFFF),
			xml::XmlHelper::get_hex(node, "color_bottom").value_or(0xFFFFFF)
		);
		prepare();
	}
	void Value::load(const xml::ptree& node) {
		auto typeStr = node.get_optional<string>("<xmlattr>.type").value_or("int");
		if (typeStr == "bool") type = BOOL;
		else if(typeStr == "char") type = CHAR;
		else if (typeStr == "byte" || typeStr == "uchar") type = BYTE;
		else if (typeStr == "short") type = SHORT;
		else if (typeStr == "ushort") type = USHORT;
		else if (typeStr == "int") type = INT;
		else if (typeStr == "uint") type = UINT;
		else if (typeStr == "ptr") type = PTR;
		else if (typeStr == "float") type = FLOAT;
		else if (typeStr == "double") type = DOUBLE;
		else type = INT;
		for (auto& [k,v] : xml::XmlHelper::make_range(node.equal_range("offset"))) {
			auto ofs = xml::XmlHelper::get_hex(v, "hex");
			if (ofs) offsets.push_back(*ofs);
		}
	}
	inline static bool safe_deref(DWORD &ptr) {
		ptr = *reinterpret_cast<DWORD*>(ptr);
		return ptr;
	}
	void Value::set(void* ptr) {
		val = 0;
		auto ref0= reinterpret_cast<DWORD>(ptr); //for safety
		auto addr = reinterpret_cast<DWORD>(&ref0);
		for (auto it = offsets.begin(); it != offsets.end(); ++it) {
			if (!safe_deref(addr)) return;
			addr += *it;
		}
		switch (type) {
		case BOOL: val = *(bool*)addr; return;
		case CHAR: val = *(char*)addr; return;
		case BYTE: val = *(unsigned char*)addr; return;
		case SHORT: val = *(short*)addr; return;
		case USHORT: val = *(unsigned short*)addr; return;
		case INT: val = *(int*)addr; return;
		case UINT: val = *(unsigned int*)addr; return;
		case FLOAT: val = *(float*)addr; return;
		case DOUBLE: val = *(double*)addr; return;
		case PTR: val = *(DWORD*)addr; return;
		}
	}

	bool _hover::debug(const SokuLib::Vector2i& cursor) {
		//push string to console
		if (!check(cursor.x, cursor.y) || description.empty()) return false;
		return RivDesign::pushConsole(description, { x2,y2 })
			&& RivDesign::setBuffer(this)
			;
	}

	void Container::load(noderef node) {
#ifdef _DEBUG
		auto id = 200;
#else
		auto id = 0;
#endif // _DEBUG
		if (auto bk = node.get_child_optional("back")) {
			id = bk->get_optional<int>("<xmlattr>.ref_id").value_or(id);
		};
		
		id && (back.ref = RivDesign::getSprite(id));
		
		for (auto& [key, child] : node) {
			//auto className = v.get_optional<string>("<xmlattr>.class");
			if (key == "pad")
				addChildren(new Pad(child));
		}

	}
	bool Container::debug(const SokuLib::Vector2i& cursor) {
		if (!visible) return false;
		//bool hit = false;
		for (auto* child : children | std::views::reverse) {
			if (child && child->debug(cursor))
				return RivDesign::setBuffer(this);
		}
		return _hover::debug(cursor);
	}
	bool Container::onClick(const SokuLib::Vector2i& cursor, const Callback& cb) {
		if (!visible) return false;
		//bool hit = false;
		for (auto* child : children | std::views::reverse) {
			if (child && child->onClick(cursor, cb)) {
				return true;
			}
		}
		return _hover::onClick(cursor, cb);
	}
	void Icon::load(noderef node) {
		auto id = node.get_optional<int>("<xmlattr>.ref_id");
		if (id) {
			auto ref = RivDesign::getSprite(*id);
			//Base::sprite.setTexture2(ref->);
			ref_icons = ref;
		}
		auto xyi = xml::XmlHelper::get_array<int>(node, "tile", ',');
		if (xyi.size() >= 3) {
			cols = xyi[0]; rows = xyi[1]; index = xyi[2];
		}
		w = node.get_optional<int>("<xmlattr>.width").value_or(ref_icons && cols ? ref_icons->sprite.size.x / cols : 0);
		h = node.get_optional<int>("<xmlattr>.height").value_or(ref_icons && rows ? ref_icons->sprite.size.y / rows : 0);
	}

	void Number::bind(const Layout& l, int idv, int idn) {
		//FLT_EPSILON
		ref_val = l.getValue(idv);
		auto temp = RivDesign::getObject(idn);
		if (temp && *(DWORD*)temp == DWORD(SokuLib::ADDR_VTBL_CDESIGN_NUMBER)) {
			ref_num = reinterpret_cast<decltype(ref_num)>(temp);
		}
	}
	void Number::load(noderef node, const Layout* l) {
		auto idv = node.get_optional<int>("<xmlattr>.value_id");
		auto idn = node.get_optional<int>("<xmlattr>.number_id");
		if (!idv || !idn) return;
		if (l)
			bind(*l, *idv, *idn);
		//auto hide = node.get_optional<float>("<xmlattr>.hide_if");
		//if (hide) hide_if = *hide;
		maxi = node.get_optional<float>("<xmlattr>.max").value_or(maxi);
		mini = node.get_optional<float>("<xmlattr>.min").value_or(mini);
	}
	void Grider::load(noderef node, const Layout* l) {
		maxInRow = node.get_optional<int>("<xmlattr>.rowmax").value_or(maxInRow);
		gridSizeX = node.get_optional<int>("<xmlattr>.xgrid").value_or(gridSizeX);
		gridSizeY = node.get_optional<int>("<xmlattr>.ygrid").value_or(gridSizeY);
		/*for (auto& [key, child] : xml::XmlHelper::make_range(node.equal_range("field"))) {
			addChildren(new Canva(child, l));
		}*/
	}
	void Color::load(noderef node, const Layout* l) {
		w = node.get_optional<int>("<xmlattr>.width").value_or(w);
		h = node.get_optional<int>("<xmlattr>.height").value_or(h);
		auto idv = node.get_optional<int>("<xmlattr>.value_id");
		if (!idv || !l) return;
		ref_val = l->getValue(*idv);
		//back.ref = RivDesign::getSprite(201);
		if (description.empty()) {
			description = "%s";
		}
	}
	bool Color::debug(const SokuLib::Vector2i& cursor) {
		if (!visible || !check(cursor.x, cursor.y) || description.empty()) return false;
		//no child
		//if 0x00FFFFFF then show 0xFFFFFF
		unsigned char a = (color >> 24) & 0xFF;
		unsigned int rgb = color & 0x00FFFFFF;
		string temp; temp.resize((description.size() + 12));
		if (a) {
			//temp = std::move(std::format("{:02X}#{:06X}", a, rgb));
			sprintf(temp.data(), description.c_str(), std::format("{:02X}#{:06X}", a, rgb).c_str());
		} else {
			//temp = std::move(std::format("#{:06X}", rgb));
			sprintf(temp.data(), description.c_str(), std::format("#{:06X}", rgb).c_str());

		}
		return RivDesign::pushConsole(temp, { x2,y2 }) && RivDesign::setBuffer(this);
		
	}
	void Text::load(noderef node) {
		auto ft = node.get_optional<int>("<xmlattr>.font_id");
		if (!ft) return;
		font = &RivDesign::getFont(*ft);
		boxw = node.get_optional<int>("<xmlattr>.width").value_or(boxw);
		boxh = node.get_optional<int>("<xmlattr>.height").value_or(boxh);
		auto str = node.get_value_optional<string>();
		if (str && !str->empty()) setText(*str);

	}
	void Canva::load(noderef node, const Layout* l) {
		for (auto& [key, child] : node) {
			Element* elem = nullptr;
			if (key == "textbox") {
				elem = addChildren(new Text(child, l));
			}
			else if (key == "number") {
				elem = addChildren(new Number(child, l));
			}
			else if (key == "icon") {
				elem = addChildren(new Icon(child, l));
			}
			else if (key == "sprite") {
				elem = addChildren(new Sprite(child, l));
			}
			else if (key == "color") {
				elem = addChildren(new Color(child, l));
			}
			else if (key == "tail") {
				elem = addChildren(new Tail(child, l));
			}

			else if (key == "field") {
				elem = addChildren(new Canva(child, l, false));
			}
			else if (key == "grid") {
				elem = addChildren(new Grider(child, l));
			}
			else if (key == "link") {
				elem = addChildren(new Link(child, l));
			}
		}
		auto cond = xml::XmlHelper::get_array<int>(node, "hide_if", '=');
		int idv = 0;
		if (cond.size() >= 2) {
			idv = cond[0];
			hideif = cond[1];
		} else {
			idv = node.get_optional<float>("<xmlattr>.value_id").value_or(0);
			if (cond.size() == 1) hideif = cond[0];
		}
		if (idv && l) ref_ptr = l->getValue(idv);
		cond = xml::XmlHelper::get_array<int>(node, "show_if", '=');
		if (cond.size() >= 2) {
			idv = cond[0];
			showif = cond[1];
		}
		else {
			idv = node.get_optional<float>("<xmlattr>.value_id").value_or(0);
			if (cond.size() == 1) showif = cond[0];
		}
		if (idv && l) ref_ptr2 = l->getValue(idv);
	}
	void Layout::load(noderef node) {
		for (auto& [k, v] : xml::XmlHelper::make_range(node.equal_range("title")))
			titles.push_back(TitleBar(v));//titles.emplace_back(v);//wtf
		auto valueMgr = node.get_child_optional("values");
		if (valueMgr) {
			for (const auto& [k, v] : xml::XmlHelper::make_range(valueMgr->equal_range("value"))) {
				auto id = v.get_optional<int>("<xmlattr>.id");
				if (!id) continue;
				auto cl = xml::XmlHelper::get_class(v);
				Value* nv= nullptr;
				if (cl == "shader-color")//
					nv = new ValueSpec_ShaderColor(v);
				else if (cl == "skill-level")
					nv = new ValueSpec_SkillLevel(v);
				else if (cl == "cancel-level")
					nv = new ValueSpec_CancelLevel(v);
				else if (cl == "delta-hp")
					nv = new ValueSpec_DeltaHP(v);
				else if (cl == "real-damage")
					nv = new ValueSpec_RealDamage(v);
				else if (cl == "player-stun")
					nv = new ValueSpec_PlayerStun(v);
				else if (v.get_optional<bool>("<xmlattr>.use_clip").value_or(false)) {
					nv = new ValueSpec_PlayerClip(v);
				}
				//else if (cl == "hitstop")
					//nv = new Value(v);
				else
					nv = new Value(v);
				if (nv) values.emplace(*id, nv);
			}
		}

		auto names = xml::XmlHelper::get_array<string>(node, "inherit");
		parents.reserve(names.size());
		for (auto& n : names)
			parents.emplace_back(std::move(n), nullptr);
		names.clear();
		//for (auto& [key, v] : xml::XmlHelper::make_range(node.equal_range("field"))) {
		for (auto& [key, v] : node) {
			if (key == "field") {
				addChildren(new Canva(v, this));
			}
			else if (key == "grid") {
				addChildren(new Grider(v, this));
			}
			else if (key == "link") {
				addChildren(new Link(v, this));
			}
			
		}
	}

	
	inline void Layout::inheriting(std::map<string, Layout>& layouts, Layout& current) {
		for (auto& [n, l] : current.parents) {
			printf("\tInheriting parent sublayout %s(%d)\n", n.c_str(), n.size());
			auto it = layouts.find(n);
			if (it != layouts.end()) {
				printf("\tInherits successful by %p\n", &it->second);
				l = &it->second;
			}
		}
	}
	
	void RivDesign::load(const string& name, const string& name2) {
		CDesign::loadResource(name.c_str());
#ifdef _DEBUG
		for (auto& [i,obj] : CDesign::objectMap) {
			if (i < 100 || *(DWORD*)obj != SokuLib::ADDR_VTBL_CDESIGN_SPRITE) continue;
			obj->active = true;
			obj->y2 += 400;
			obj->setColor(0xA0FFFFFF);
		}
#endif
		_current = this;
		auto doc0 = xml::XmlHelper(name2);
		const auto& doc = doc0.get().get_child("layout");
		//version = doc.get_optional<string>("layout.<xmlattr>.riv_version", version).value();
		auto fontsMgr = doc.get_child_optional("fonts");
		if (fontsMgr) {
			for (const auto& [key, node] : xml::XmlHelper::make_range(fontsMgr->equal_range("font"))) {
				auto id = node.get_optional<int>("<xmlattr>.id");
				if (!id) continue;
				_fonts.emplace(*id, node);
				//fonts[*id] = std::move(Font(node));
			}
		}
		auto window = doc.get_child_optional("window");
		if (window) {
			windowSize.x = window->get_optional<int>("<xmlattr>.width").value_or(windowSize.x);
			windowSize.y = window->get_optional<int>("<xmlattr>.height").value_or(windowSize.y);
			basePoint.x = window->get_optional<int>("<xmlattr>.xbase").value_or(basePoint.x);
			basePoint.y = window->get_optional<int>("<xmlattr>.ybase").value_or(basePoint.y);
			auto ver = window->get_child_optional("title");
			if (ver && xml::XmlHelper::check_class(*ver, "layout-version")) {
				version.emplace(*ver);
			}
		}
		for (const auto& [key, node] : xml::XmlHelper::make_range(doc.equal_range("sublayout"))) {
			auto name = node.get_optional<string>("<xmlattr>.class");
			if (!name) continue;
			sublayouts.emplace(*name, node);
			//sublayouts[*name] = std::move(Layout(node));
		}
		//inherit all
		for (auto& [name, layout] : sublayouts) {
			printf("%s(%d) Inheriting, %p\n", name.c_str(), name.size(), &layout);
			Layout::inheriting(sublayouts, layout);
		}
		//console

		_console = &console;
	
	}


	void Number::update(_box& parea, void* ctx) {
		Element::update(parea, ctx);
		if (visible && ref_val) {
			auto val = ref_val->getf();
			//visible = !(hide_if && val-*hide_if < FLT_EPSILON);
			if (val > maxi) val = maxi;
			if (val < mini) val = mini;
			rounding(val);
		}
	}
	void Number::render() const {
		Element::render();
		if (!visible) return;
		if (!ref_num) return;
		using IVal = struct CNumberValue { void* vtable, * value; };
		static auto CNSub = IVal{ (void*)0x85b2ac, nullptr };
		CNSub.value = (void*) &rounded;
		auto& ival = *reinterpret_cast<IVal**>(&ref_num->number.value);
		if (ival) SokuLib::DeleteFct(ival);
		ival = &CNSub;
		int old = ref_num->number.size;
		if (old!=0) {
			int digits = 1 + log10(max(abs(rounded), 1));
			old = max(old, digits);
			std::swap(ref_num->number.size, old);
		}
		ref_num->active = true;
		ref_num->renderPos(basept.x, basept.y);
		ref_num->active = false;
		std::swap(ref_num->number.size, old);
		ival = nullptr;
	}
	
	void Icon::render() const {
		Element::render();
		//this->ref_icons
		if (!visible) return;
		if (!ref_icons) return;
		/*_box uv{}; getBorder(uv);
		const SokuLib::DxVertex vertices[] = {
			{x1, y1,	0.0f, 1.0f,	0xFFFFFFFF,	uv.x1,	uv.y1},
			{x2, y1,	0.0f, 1.0f, 0xFFFFFFFF,	uv.x2,	uv.y1},
			{x1, y2,	0.0f, 1.0f,	0xFFFFFFFF,	uv.x1,	uv.y2},
			{x2, y2,	0.0f, 1.0f, 0xFFFFFFFF,	uv.x2,	uv.y2},
		};
		SokuLib::textureMgr.setTexture(ref_icons->sprite.dxHandle, 0);
		SokuLib::pd3dDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (const void*)vertices, sizeof(SokuLib::DxVertex));*/
		helper.setColor(0xFFFFFFFF);
		//auto sz = ref_icons->sprite.size;
		//int ox = ref_icons->sprite.
		//int dx = cols > 0 ? sz.x / cols : 0, dy = rows > 0 ? sz.y / rows : 0;
		//helper.setTexture2(ref_icons->sprite.dxHandle, index%cols * dx, index/cols * dy, dx, dy);
		_box uv{}; getBorder(uv);
		helper.dxHandle = ref_icons->sprite.dxHandle;
		setBorder(helper, uv);
		helper.renderScreen(x1, y1, x2, y2);
	}
	void Grider::update(_box& parea, void* ctx) {
		visible = false;
		//Canva::update(parea, ctx);
		_box& b = this->area();
		b = parea.basept + Vector2f{ (float)offsetX, (float)offsetY };
		int count = 0;
		for (auto* child : children) {
			if (!child) continue;
			b.basept = { b.x1 + float(gridSizeX)*(count%maxInRow), b.y1 + float(gridSizeY)*(count/maxInRow)};
			//fori.x2 = fori.x1 + gridSizeX; fori.y2 = fori.y1 + gridSizeY;
			//child->area() = fori;
			child->update(b, ctx);
			if (child->visible) {
				++count;
				//b.hold(fori);
				visible |= true;
			}
		}
		
		//parea.basept.y = b.y2;
		if (visible) {
			stack(parea);
			parea.hold(b);
		}
	}
	
	void Sprite::cropping() {
		return;
	}
	namespace { constexpr float DEG2RAD = 3.1415927 / 180; }
	static void apply_rotation(SokuLib::DxVertex(&vertices)[4], const SokuLib::v2::BlendOptions* blend, const SokuLib::Vector2f& basept = SokuLib::Vector2f{0, 0}) {
		if (!blend) return;

		float rz = blend->rotateZ * DEG2RAD;
		// XY旋转 → 缩放
		float sy = std::cos(blend->rotateX * DEG2RAD);
		float sx = std::cos(blend->rotateY * DEG2RAD);
		// Z旋转矩阵
		float cz = std::cos(rz);
		float sz = std::sin(rz);

		for (int i = 0; i < 4; ++i) {
			float dx = vertices[i].x - basept.x;
			float dy = vertices[i].y - basept.y;
			// 绕Z旋转
			float nx = dx * cz - dy * sz, ny = dx * sz + dy * cz;
			dx = nx; dy = ny;
			// XY缩放
			dx *= sx; dy *= sy;

			vertices[i].x = basept.x + dx;
			vertices[i].y = basept.y + dy;
		}
	}
	void Sprite::update(_box& parea, void* ctx) {
		Element::update(parea, ctx);
		if (!visible) return;
		x2 += frameW; y2 += frameH;
		parea.hold(this->area());
		if (!ctx) return;
		using data = const SokuLib::v2::GameObjectBase;
		auto& obj = *reinterpret_cast<data*>(ctx);
		dxHandle = NULL;
		if (!obj.frameData) return;
		SokuLib::Sprite& sp = *this;
		sp.dxHandle = obj.sprite.dxHandle;
		sp.size = obj.frameData->texSize.to<float>();
		SokuLib::Vector2f basept, s, r = obj.frameData->offset.to<float>();
		float scale = 1.0f;
		switch (obj.frameData->renderGroup) { using FD = SokuLib::v2::FrameData;
		case FD::SPRITE:
			sp.size *= 2;
			scale *= 0.5f;
		ForcedCharacter:
			s = sp.size * scale;
			r *= scale;
			vertices[0].x = vertices[2].x =  - r.x;
			vertices[0].y = vertices[1].y =  - r.y;
			vertices[1].x = vertices[3].x =  - r.x + s.x;
			vertices[2].y = vertices[3].y =  - r.y + s.y;
			apply_rotation(vertices, obj.frameData->blendOptionsPtr);
			basept = { (x1 + x2) / 2, y2 - 10};
			break;
		case FD::WITHBLEND:
			if (obj.frameData->blendOptionsPtr) {
				auto dscale = obj.frameData->blendOptionsPtr->scale;
				sp.size.x *= dscale.x; sp.size.y *= dscale.y;
				sp.rotation = obj.frameData->blendOptionsPtr->rotateZ * DEG2RAD;
				r.x *= dscale.x; r.y *= dscale.y;
				if (obj.frameState.actionId < 800) { 
					float fixed = max(dscale.x, dscale.y);
					scale = min(0.5f, 1.5f/fixed);
					goto ForcedCharacter;
				}
			}
		default:
			if (sp.size.x > frameW || sp.size.y > frameH)
				scale = (sp.size.x / sp.size.y < frameW / frameH) ? frameH / sp.size.y : frameW / sp.size.x;
			s = sp.size * scale;
			r *= scale;
			vertices[0].x = vertices[2].x = -r.x;
			vertices[0].y = vertices[1].y = -r.y;
			vertices[1].x = vertices[3].x = -r.x + s.x;
			vertices[2].y = vertices[3].y = -r.y + s.y;
			apply_rotation(vertices, obj.frameData->blendOptionsPtr);
			basept = { (x1 + x2) / 2, (y1 + y2) / 2 };
			//move base to fit in frame
			float edges[4] = {
				HUGE_VALF,//frameW / 2.f - r.x,
				HUGE_VALF,//frameW / 2.f + r.x - s.x,
				-HUGE_VALF,//frameH / 2.f - r.y,
				-HUGE_VALF,//frameH / 2.f + r.y - s.y,
			};
			for (int i = 0; i < 4; ++i) {
				edges[0] = min(edges[0], vertices[i].x);
				edges[1] = min(edges[1], vertices[i].y);
				edges[2] = max(edges[2], vertices[i].x);
				edges[3] = max(edges[3], vertices[i].y);
			}
			for (int i = 0; i < 4; ++i) {
				if (i % 2) {//y
					edges[i] = (i / 2 ? y2 : y1) - (edges[i]+basept.y);
				}
				else {//x
					edges[i] = (i / 2 ? x2 : x1) - (edges[i]+basept.x);
				}
			}
			if (edges[0]>=0 || edges[2]<=0)
				basept.x += (edges[0] + edges[2]) / 2;
			if (edges[1]>=0 || edges[3]<=0)
				basept.y += (edges[1] + edges[3]) / 2;
			break;
		}
		for (int i = 0; i < 4; ++i) {
			vertices[i].x += basept.x;
			vertices[i].y += basept.y;
			vertices[i].u = obj.sprite.vertices[i].u;
			vertices[i].v = obj.sprite.vertices[i].v;
		}
		
		if (crop) cropping();
		//if (obj.frameData->renderGroup == 2 && obj.frameData->blendOptionsPtr)
			//vertices[0].color = vertices[1].color = vertices[2].color = vertices[3].color = target->frameData->blendOptionsPtr->color;
		sp.pos = basept;
	}
	void Sprite::render() const {
		Element::render();
		if (!visible || !dxHandle) return;
		static SokuLib::DrawUtils::RectangleShape temp;
		//tex border
		//temp.setRect({vertices[0].x-1, vertices[0].y-1, vertices[3].x+1, vertices[3].y+1});
		 //temp.setBorderColor
		//SokuLib::DrawUtils::Rect<SokuLib::Vector2f> rect;
		Vector2f d {0.5,0.5};
		float ax = atan2( vertices[1].y - vertices[0].y, vertices[1].x - vertices[0].x );
		
		d.rotate(ax, {0,0});
		constexpr float piby2 = 90 * DEG2RAD;
		Vector2f d2 = d.rotate(piby2, {0,0});
		
		temp.rawSetRect({
			Vector2f{vertices[0].x, vertices[0].y}-d, 
			Vector2f{vertices[1].x, vertices[1].y}-d2,
			Vector2f{vertices[3].x, vertices[3].y}+d,
			Vector2f{vertices[2].x, vertices[2].y}+d2,
			});
		auto scp = riv::tex::RendererGuard();
		scp.setTexture(NULL);
		temp.setFillColor(0); temp.setBorderColor(0xA0FFFFFF); temp.draw();
		scp.setTexture(dxHandle); SokuLib::pd3dDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(*vertices));
		//SokuLib::SpriteEx::rotate
		//base pt
		temp.setPosition(pos.to<int>() - SokuLib::Vector2i{ 2,2 }); temp.setSize({ 4,4 });
		temp.setFillColor(0xA0000000); temp.setBorderColor(0xFFFFFFFF); temp.draw();
	}

	void Layout::update(_box& parea, void* ctx) {
		for (auto& [n,l] : parents) {
			if (!l) continue;
			//progress
			l->update(parea, ctx);
		}
		for (auto [i,v] : values) {
			if (!v) continue;
			v->update(ctx);
		}
		_box& b = this->area();
		b = parea.basept;
		//_box stacker = b;
		for (auto& tt : titles) {
			tt.update(b, ctx);
		}
		Container::update(parea, ctx);
		stack(parea);
		//parea.basept = b.basept;
		parea.hold(b);
	}
	void Layout::render() const {
		for (auto& [n,l] : parents) {
			if (!l) continue;
			//progress
			l->render();
		}
		Container::render();
		for (auto& tt : titles) {
			tt.render();
		}
	}


	bool RivDesign::updates(const string& cl, void* ctx, bool& successful) {
		//if (!ctx) return false;
		auto it = sublayouts.find(cl);
		if (it == sublayouts.end() && !cl.empty()) {
			return false;
		}

		_box base(basePoint);
		Sprite *copied = nullptr;
		getById(&copied, 3);
		//if (hint)
			//hint->active = it == sublayouts.end();
		if (copied && hint_timer > 0) {
			copied->active = --hint_timer;
			copied->setColor(SokuLib::DrawUtils::DxSokuColor::White * (min(hint_timer, 40) / 40.f));
		}
		if (version) {
			version->update(base, ctx);
			successful &= true;
		}
		if (it != sublayouts.end()) {
			auto& layout = it->second;
			layout.area() = base;
			if (updating.try_lock()){
				layout.update(base, ctx);
				if (cl != currentLayout) _hoverbuffer = nullptr;
				currentLayout = cl;
				updating.unlock();
				successful &= true;
			} else { 
				puts("updating try lock failed");
				successful &= false;
			}
		} else {
			//currentLayout = "";
		}
		return true;
	}
	void RivDesign::render() const {
		auto& cl = currentLayout;
		auto it = sublayouts.find(cl);
		if (it == sublayouts.end() && !cl.empty()) {
			return;
		}
		auto& design = *const_cast<RivDesign*>(this);
		Sprite* hint = nullptr, * cursor = nullptr;
		//getById(&hint, 1); 
		design.getById(&cursor, 2);

		design.CDesign::render4();
		if (version) {
			version->render();
		}
		if (it != sublayouts.end()) {
			auto& layout = it->second;
			layout.render();
		}

		if (cursor) {
			cursor->active = console.clientpt.x > 0 && console.clientpt.y > 0;
			cursor->renderPos(console.clientpt.x - cursor->sprite.size.x / 2, console.clientpt.y - cursor->sprite.size.y / 2);
			cursor->active = false;
		}
	}

}

