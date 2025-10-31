#pragma once
#include <Design.hpp>
#include <Font.hpp>
#include <Sprite.hpp>
#include <IFileReader.hpp>
#include <FrameData.hpp>
#include <Player.hpp>

#include "tex.hpp"
#include <variant>
//#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <iostream>
#include <map>
#include <set>
#include <CommCtrl.h>

namespace gui {
	using string = std::string;
	using string_view = std::string_view;

	namespace xml {
		using std::istringstream;
		using boost::property_tree::ptree, boost::property_tree::read_xml;
		using boost::property_tree::xml_parser::no_comments, boost::property_tree::xml_parser::trim_whitespace;
		using SokuLib::IFileReader;
		static const auto _init_reader = reinterpret_cast<bool(__fastcall*)(IFileReader** ret, int _, LPCSTR filename)>(0x40d1e0);
		class XmlHelper {
			ptree doc;
			template <typename It>
			struct iterator_range {
				It b, e;
				It begin() const noexcept { return b; }
				It end()   const noexcept { return e; }
			};
		public:

			// 辅助函数：从 equal_range 返回的 pair 创建 range
			inline explicit XmlHelper(const std::string& name) {
				try {
					auto iss = read_file(name);
					read_xml(iss, doc, no_comments | trim_whitespace);
				} catch(std::exception detail) {
					const auto msg = std::format("ReplayInputView++: failed to parse layout file ({}).\nPlease check file contents and xml syntax!\nDetails:{}", name, detail.what());
					MessageBoxA(nullptr, msg.c_str(), "RIV Error", MB_ICONERROR | MB_OK);
					throw std::runtime_error(msg);
				}
				
			}
			inline ptree& get() { return doc; }

			static istringstream read_file(const string& name);
			static void decrypt(string& buf);
			template <typename It> inline static iterator_range<It> make_range(std::pair<It, It> p) noexcept { return { p.first, p.second }; }
			template<typename T> static std::vector<T> get_array(const ptree& node, const std::string& key, char sep = ',');
			
			static inline string get_class(const ptree& node) {
				auto n = node.get_optional<string>("<xmlattr>.class");
				if (n) return *n;
				return "";
			}
			static inline bool check_class(const ptree& node, const string& name) {
				return get_class(node) == name;
			}

			static std::optional<unsigned long> get_hex(const ptree& node, const string& key) {
				auto attr = node.get_child_optional("<xmlattr>");
				if (!attr) return {};
				auto val = attr->get_optional<string>(key);
				return val ? std::stoul(*val, nullptr, 0) : std::optional<unsigned long>{};
			}
		};


		

	}
	//using _box = riv::tex::FloatRect;
	struct _box : riv::tex::FloatRect {
		using Rect = riv::tex::FloatRect;
		using Vector2f = riv::tex::Vector2f;
		Vector2f basept = { 0,0 };
		inline _box() : Rect{0} {};
		//_box(const _box&) = default; _box(_box&&) = default;
		inline _box(float a, float b, float c, float d) : Rect(a,b,c,d) {
			basept.x = a; basept.y = b;
		}
		template<typename T> inline _box(const T& v) : _box(v.x, v.y, v.x, v.y) {}
		inline _box(Vector2f&& v) noexcept : basept(std::move(v)) {
			x1 = x2 = basept.x;
			y1 = y2 = basept.y;
		}
		inline _box(const Rect& r) : Rect(r) {
			basept.x = x1; basept.y = y1;
		}
		inline _box(Rect&& r) : Rect(std::move(r)) {
			basept.x = x1; basept.y = y1;
		}
		inline const _box& area() const { return *this; }
		inline _box& area() { return *this; }
		inline void move(const Vector2f& d) {
			x1 += d.x; x2 += d.x;
			y1 += d.y; y2 += d.y;
			basept += d;
		}
		inline void hold(const _box& b2) {
			x1 = min(x1, b2.x1); y1 = min(y1, b2.y1);
			x2 = max(x2, b2.x2); y2 = max(y2, b2.y2);
		}

		inline static _box hold(const _box& b1, const _box& b2) {
			return {
				min(b1.x1, b2.x1),
				min(b1.y1, b2.y1),
				max(b1.x2, b2.x2),
				max(b1.y2, b2.y2),
			};
		}
	};
	using riv::tex::TileDesc;
	using SokuLib::CDesign;
	class Layout;


	using FontDesc = SokuLib::FontDescription;
	class Font : public FontDesc {//credit shady-loader
		SokuLib::SWRFont* handle = nullptr;
	public:
		void load(const xml::ptree& node);
		inline Font() {
			weight = 400; height = 15;
			italic = shadow = useOffset = false;
			offsetX = offsetY = charSpaceX = charSpaceY = 0;
			bufferSize = 2048;
			r1 = g1 = b1 = b2 = r2 = g2 = 0xff;
			strcpy_s(faceName, SokuLib::defaultFontName);
		}
		inline Font(const xml::ptree& node) : Font() {
			load(node);
		}
		//inline Font(const Font&) = delete;
		//inline Font(Font&&) noexcept = default;
		inline ~Font() { if (handle) { handle->destruct(); delete handle; } handle = nullptr; }
		inline void setFontName(const char* name) { if(strlen(name)) strcpy(faceName, name); }
		inline void setColor(int c1, int c2) { r1 = c1 >> 16; g1 = c1 >> 8; b1 = c1; r2 = c2 >> 16; g2 = c2 >> 8; b2 = c2; }
		inline void prepare() { if (!handle) { handle = new SokuLib::SWRFont(); handle->create(); }  handle->setIndirect(*this); }

		inline int newText(const string& str, int& boxw, int& boxh) const {
			int ret = 0;
			if (handle) {
				if (boxw <= 0) boxw = str.length() * (height + (int)charSpaceX);
				if (boxh < height) boxh = height + (int)charSpaceY + 10;
				if (useOffset) boxh = 10000;
				SokuLib::textureMgr.createTextTexture(&ret, str.c_str(), *handle, boxw, boxh, &boxw, &boxh);
			}
			return ret;
		}
		static std::vector<std::wstring> LoadAllFontsInFolder(const std::wstring& folderPath, bool recursive = false);
	};

	class _hover : protected virtual _box {
	public:
		string description = "<description>";
		inline void load(const xml::ptree& node) {
			auto desc = node.get_optional<string>("<xmlattr>.description");
			if (desc) description = *desc;
		}
		inline _hover(const xml::ptree& node) {
			load(node);
		}
		inline bool check(int cx, int cy) const {
			return x1 <= cx && cx < x2 && y1 <= cy && cy < y2;
		}
		inline virtual void debug() const {
			//push string to console
		}
		//void update();
	};
	class _align {
		enum Align : unsigned char {
			Fixed = 0,
			Left, Center, Right,
			Top, Middle, Bottom,
		} align_h = Align::Left, align_v = Align::Top;
	public:
		_align() = default;
		_align(const string_view& ha, const string_view& va) {
			if (ha == "left" || ha == "l") align_h = Align::Left;
			else if (ha == "center" || ha == "c" || ha == "middle" || ha == "m") align_h = Align::Center;
			else if (ha == "right" || ha == "r") align_h = Align::Right;
			if (va == "top" || va == "t") align_v = Align::Top;
			else if (va == "center" || va == "c" || va == "middle" || va == "m") align_v = Align::Middle;
			else if (va == "bottom" || va == "b") align_v = Align::Bottom;
		}
		void apply(const _box& self, const _box& container) {
			/*switch (align_h) {
			case Align::Left:		x = x; break;
			case Align::Center:		x = x + (w - gw) / 2; break;
			case Align::Right:		x = x + (w - gw); break;
			}
			switch (align_v) {
			case Align::Top:		y = y; break;
			case Align::Middle:		y = y + (h - gh) / 2; break;
			case Align::Bottom:		y = y + (h - gh); break;
			}*/
		}
	};
	
	class _hider {
	public:
		using checker = std::function<bool(void*)>;

	protected:
		const checker* rule = nullptr;  // 指向规则函数
		using manager = std::map<std::string, checker>;
		//static manager Rules;

	public:
		static manager& Rules();
		_hider(const xml::ptree& node) {
			load(node);
		};
		inline static void Register(const string& name, const checker& fn) {
			Rules().emplace(name, fn);
		}
		inline static const checker* Get(const string& name) {
			auto it = Rules().find(name);
			if (it != Rules().end())
				return &it->second;
			return nullptr;
		}
		void load(const xml::ptree& node) {
			auto cl = xml::XmlHelper::get_class(node);
			rule = Get(cl);
		}
		inline bool verify(void* ctx) const {
			return !rule || (*rule)(ctx);
		}
	};

	class Container : 
		protected virtual _box,
		//public virtual _align,
		public _hider
	{
	protected:
		//const std::array<int, 9> tile_edges{ 0 };//id of 9-slice box
		//std::optional<int> back_ref;
		struct _back_ref {
			CDesign::Sprite* ref= nullptr;
			void getBorder(_box& t, int i) const {									// ___________
				if (ref)														//|_0_|_1_|_2_|
					t = riv::tex::Tex::getBorder(i, 3, 3, ref->sprite);			//|_3_|_4_|_5_|
			}																	//|_6_|_7_|_8_|
			inline static void draw(const _box& area, const _box& uv) {
				SokuLib::DxVertex v[4] = {
					{ area.x1, area.y1, 0.0f, 1.0f, 0xFFFFFFFF, uv.x1, uv.y1 },
					{ area.x1, area.y2, 0.0f, 1.0f, 0xFFFFFFFF, uv.x1, uv.y2 },
					{ area.x2, area.y1, 0.0f, 1.0f, 0xFFFFFFFF, uv.x2, uv.y1 },
					{ area.x2, area.y2, 0.0f, 1.0f, 0xFFFFFFFF, uv.x2, uv.y2 },
				};
				SokuLib::pd3dDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(SokuLib::DxVertex));
			}
			inline void render(const _box& area) const {
				if (!ref) return;
				auto dsize = ref->sprite.size / 3.0f;
				float dx[4] = {
					area.x1 - dsize.x,
					area.x1,
					area.x2,
					area.x2 + dsize.x,
				};
				float dy[4] = {
					area.y1 - dsize.y,
					area.y1,
					area.y2,
					area.y2 + dsize.y,
				};
				SokuLib::textureMgr.setTexture(ref->sprite.dxHandle, 0);
				// 3x3 grid
				for (int row = 0; row < 3; ++row) {
					for (int col = 0; col < 3; ++col) {
						int idx = row * 3 + col;
						_box uv; getBorder(uv, idx);
						_box dst;
						dst.x1 = dx[col]; dst.x2 = dx[col + 1];
						dst.y1 = dy[row]; dst.y2 = dy[row + 1];
						draw(dst, uv);//can be inline and use TRIANGLELIST
					}
				}
				
			}
		} back;
		std::vector<Container*> children;
		template<typename T> inline T* addChildren(T* p) {
			static_assert(std::is_base_of_v<Container, T>, "Container holds non-based obj.");
			children.emplace_back(static_cast<Container*>(p));
			return p;
		}
	public:
		using noderef = const xml::ptree&;
		bool visible = true;
		//Container() = default;
		void load(noderef node);
		Container(noderef node) : _hider(node) {
			load(node);
			this->area() = { 0, 0, 0, 0 };
		}
		Container(const Container&) = delete; Container& operator=(const Container&) = delete;
		Container(Container&&) noexcept = default; Container& operator=(Container&&) noexcept = default;
		inline virtual ~Container() {
			for (auto ptr : children)
				delete ptr;
		}
		inline void verify(void* ctx) { visible = _hider::verify(ctx); }
		//Container() { visible = true; }
		inline virtual void update(_box& parentArea, void* ctx) {
			verify(ctx);
			for (auto* child : children) {
				if (child) child->update(*this, ctx);
			}
		}
		inline virtual void render() const {
			if(!visible) return;
			back.render(this->area());
			for (auto* child : children) {
				if (child) child->render();
			}
		}//render _box for debug
		using _box::area;
		
		//virtual void hold();
	};
	struct Pad : public Container {
		int w = 0, h = 0, ofsx = 0, ofsy = 0;
		inline void load(noderef node) {
			w = node.get_optional<int>("<xmlattr>.width").value_or(w);
			h = node.get_optional<int>("<xmlattr>.height").value_or(h);
			ofsx = node.get_optional<int>("<xmlattr>.xoffset").value_or(ofsx);
			ofsy = node.get_optional<int>("<xmlattr>.yoffset").value_or(ofsy);
		}
		inline Pad(noderef node) : Container(node) {
			load(node);
			visible = false;
		}
		inline virtual void update(_box& parentArea, void* ctx) override {
			x1 = parentArea.x1 + ofsx; y1 = parentArea.y1 + ofsy;
			x2 = x1 + w; y2 = y1 + h;
			parentArea.hold(this->area());
		}
		inline virtual void render() const override { return; }

	};

	class Value {
	protected:
		/*union {
			int vali;
			float valf;
		};*/
		std::variant<unsigned, int, float, double> val{ 0 };
		//float val = 0.0f;
		enum Type : int {
			CHAR, SHORT, INT,
			BYTE, USHORT, UINT,
			FLOAT, DOUBLE,
			PTR, 
		} type = INT;
		std::vector<unsigned int> offsets;
	public:
		void load(const xml::ptree& node);
		inline Value() = default;
		inline Value(const xml::ptree& node) {
			offsets.reserve(1);
			load(node);
		}
		virtual ~Value() = default;
		void set(void* ptr);
		inline virtual float getf() const {
			return std::visit([](auto&& v) -> float {
				using T = std::decay_t<decltype(v)>;
				if constexpr (std::is_arithmetic_v<T>)
					return static_cast<float>(v);
				else
					return 0.0f;
				}, val);
		}
		inline virtual int geti() const {
			return std::visit([](auto&& v) -> int {
				using T = std::decay_t<decltype(v)>;
				if constexpr (std::is_integral_v<T>)
					return static_cast<int>(v);
				else
					return 0;
				}, val);
		}
		inline virtual void update(void* ptr) { return set(ptr); }
		inline virtual void updateForward(void*) { return; }
	};
	
	class Element : public Container {
	protected:
		int offsetX = 0, offsetY = 0;
		//friend class Canva;
	public:
		inline void load(noderef node) {
			offsetX = node.get_optional<int>("<xmlattr>.xoffset").value_or(offsetX);
			offsetY = node.get_optional<int>("<xmlattr>.yoffset").value_or(offsetY);
		}
		Element(noderef node) : Container(node) {
			load(node);
		}
		Element(const Element&) = delete; Element(Element&&) noexcept = default;
		inline virtual void update(_box& parea, void* ctx) override {
			auto& b = this->area();
			b = parea.basept + Vector2f{ (float)offsetX, (float)offsetY };
			Container::update(parea, ctx);
			//x1 = x2 = parea.basept.x + offsetX;
			//y1 = y2 = parea.basept.y + offsetY;
		}
		//virtual void render() const override;
	};
	class Icon : public Element, public _hover {
	protected:
		const CDesign::Sprite* ref_icons = nullptr;
		//bool active = false;//
		int cols = 1, rows = 1, index = 0;
		int w, h;
	public:
		void load(noderef node);
		Icon(noderef node) : Element(node), _hover(node) {
			load(node);
		}
		inline void getBorder(_box& uv) const {
			if (ref_icons && index>=0) {
				uv = riv::tex::Tex::getBorder(index, cols, rows, ref_icons->sprite);
			}
		}
		//Icon(int id) : tileId(id) {}
		//template <int sx, int sy, int gx, int gy, int ox = 0, int oy = 0> void render(riv::tex::Tex);
		inline virtual void update(_box& parea, void* ctx) override {
			Element::update(parea, ctx);
			if (!visible) return;
			_box& b = this->area();
			b.x2 = b.x1 + w; b.y2 = b.y1 + h;
			parea.hold(b);
		}
		virtual void render() const override;//

	};
	class Text : public Element, public SokuLib::Sprite {//textbox
	protected:
		const Font* font = nullptr;
		int boxw = -1, boxh = -1;
	public:
		void load(noderef node);
		Text() = default;
		Text(noderef node) : Element(node) {
			load(node);
		}
		Text(const Text&) = delete; Text(Text&&) noexcept = default;
		inline void setFont(const Font& f) { font = &f; }
		inline void setText(const string& str) {
			if (!font) return;
			if (dxHandle) SokuLib::textureMgr.remove(dxHandle);
			auto handle = font->newText(str, boxw, boxh);
			SokuLib::Sprite::setTexture2(handle, 0, 0, boxw, boxh);
			//x2 = x1 + boxw; y2 = y1 + boxh;
		}
		//inline void setBox(int w, int h) { boxw = w; boxh = h; }
		//inline void setBoxAuto() { boxw = -1; boxh = -1; }
		inline virtual void update(_box& parea, void* ctx) override {
			Element::update(parea, ctx);
			if (!visible) return;
			_box t = this->area();
			if (boxw>=0) t.x2 = t.basept.x + boxw;
			if (boxh>=0) t.y2 = t.basept.y + boxh;
			this->area().hold(t);
			//parea = hold(parea, *this);
		}
		inline virtual void render() const override {
			if (!visible || !dxHandle) return;
			Element::render();
			//_align::apply(rx, this->, x2 - x1, y2 - y1, boxw, boxh);
			SokuLib::Sprite& sp= *const_cast<Text*>(this);//damn
			sp.render(basept.x, basept.y);
		}
	};
	class TitleBar : public Text {
	public:
		//void load(noderef node);
		inline TitleBar(noderef node) : Text(node) {
			//load(node);
		}
		TitleBar(TitleBar&&) noexcept = default;
		//virtual void render() override;
		//inline virtual void update(_box& parea, void* ctx) override {
		//	Text::update(*this, ctx);
		//}
		//virtual _box area() override;
	};
	class Number : public Element {
	protected:
		//int ref_val = 0, ref_num = 0;
		const Value* ref_val = nullptr;
		CDesign::Number* ref_num = nullptr;
		float rounded;
		//std::optional<float> hide_if;
		//std::vector<int> offsets;
	public:
		void load(noderef node, const Layout& l);
		Number(noderef node, const Layout& l) : Element(node) {
			load(node, l);
		}
		inline void rounding(float v) {
			if (!ref_num) return;
			auto precision = ref_num->number.floatSize;
			if (precision < 0 || precision > 5) return;
			int b = pow(10, precision);
			rounded = std::round(v*b) / b;
		}
		void bind(const Layout& l, int idv, int idn);
		//using FieldBar::;
		virtual void update(_box& parea, void* ctx) override;
		virtual void render() const override;
	};
	class Color : public Element, public _hover {
		int w = 18, h = 18;
		const Value* ref_val = nullptr;
		unsigned int color = 0xFFFFFFFF;
	public:
		void load(noderef node, const Layout& l);
		inline Color(noderef node, const Layout& l) : Element(node), _hover(node) {
			load(node, l);
		}
		inline virtual void update(_box& parea, void* ctx) override {
			Element::update(parea, ctx);
			x2 = x1 + w; y2 = y1 + w;
			if (ref_val) {
				color = ref_val->geti();
			}
			parea.hold(this->area());
		}
		inline virtual void render() const {
			Element::render();
			SokuLib::DxVertex verts[4] = {
				{x1 + 0.5f, y1 + 0.5f,	0.f, 1.f,	color, 0,0},
				{x1 + 0.5f, y2 + 0.5f,	0.f, 1.f,	color, 0,1},
				{x2 + 0.5f, y1 + 0.5f,	0.f, 1.f,	color, 1,0},
				{x2 + 0.5f, y2 + 0.5f,	0.f, 1.f,	color, 1,1},
			};
			SokuLib::textureMgr.setTexture(NULL, 0);
			SokuLib::pd3dDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(*verts));
		}
		virtual void debug() const override;//output string
	};
	class Sprite : public Element, SokuLib::Sprite {
		//SokuLib::Vector2f scale;
		//SokuLib::v2::BlendOptions* blend = nullptr;//group2 only
		//SokuLib::Vector2<short> size, ofs;
		int frameW = 128, frameH = 128;
		bool crop = false;
	public:
		inline void load(noderef node) {
			frameW = node.get_optional<int>("<xmlattr>.width").value_or(frameW);
			frameH = node.get_optional<int>("<xmlattr>.height").value_or(frameH);
			crop = node.get_optional<bool>("<xmlattr>.crop").value_or(crop);
		}
		inline Sprite(noderef node) : Element(node) {
			load(node);
			SokuLib::DxVertex verts[] = {
				{0, 0,	0.0f, 1.0f,	0xFFFFFFFF,	0.0f,	0.0f},
				{0, 0,	0.0f, 1.0f, 0xFFFFFFFF,	1.0f,	0.0f},
				{0, 0,	0.0f, 1.0f, 0xFFFFFFFF,	0.0f,	1.0f},
				{0, 0,	0.0f, 1.0f,	0xFFFFFFFF,	1.0f,	1.0f},
			};
			memcpy(vertices, verts, sizeof(verts));
		}
		void cropping();
		virtual void update(_box& parea, void* ctx) override;
		virtual void render() const override;
	};
	class _stacker : protected virtual _box {
	protected:
		bool enabled = true;
	public:
		_stacker() = default;
		inline _stacker(bool en) : enabled(en) {}
		inline void load(const xml::ptree& node) {
			enabled = node.get_optional<bool>("<xmlattr>.stack").value_or(enabled);
		}
		inline _stacker(const xml::ptree& node, bool def = true) : _stacker(def) {
			load(node);
		}
		inline void stack(_box& bp) {
			if (!enabled) return;
			stack(bp, this->area());
		}
		inline static void stack(_box& bp, _box& bs) {//simple version
			bp.basept.y = bs.y2;
			//bp.hold(bs);
		}

	};
	class Canva : public Element, public _stacker {
		//std::variant<Text, Icon, std::nullopt_t> name;
		//std::vector<Element*> elements;
	public:
		void load(noderef node, const Layout& l);
		inline Canva(noderef node, const Layout& l, bool stk = true) : Element(node), _stacker(node, stk) {
			load(node, l);
		}
		void verify2();//hide_if?
		inline virtual void update(_box& parea, void* ctx) override {
			_box& b = this->area();
			//b = parea.basept;
			Element::update(parea, ctx);
			//parea.basept = { b.x1, b.y2 };
			stack(parea);
			parea.hold(b);
		}
	};
	class Grider : public Canva {
	protected:
		//int ref = -1;
		int maxInRow = 1, gridSizeX = 0, gridSizeY = 0;
		//std::vector<Icon> icons;
	public:
		void load(noderef node, const Layout& l);
		inline Grider(noderef node, const Layout& l) : Canva(node, l) {
			load(node, l);
		}
		virtual void update(_box& parea, void* ptr);
		//virtual void render() const override;
	};

	class Layout : public Container {
		//std::map<string, Canva> canvases;
		union {
			std::vector<Layout*> object;
			std::vector<string> name;
		};
		std::map<int, Value*> values;
		std::vector<TitleBar> titles;
	public:
		void load(noderef node);
		inline Layout(noderef node) : Container(node) {
			new (&name) std::vector<string>();
			load(node);
		}
		Layout(Layout&&) noexcept = default;
		inline ~Layout() {
			for (auto [i,p] : values)
				delete p;
		}
		inline Value* getValue(int id) const { auto it = values.find(id); return it != values.end() ? it->second : nullptr; }
		//void stack();//align and hold
		virtual void update(_box& parea, void* ctx) override;
		virtual void render() const override;
		static inline void inheriting(std::map<string, Layout>& layouts, Layout& current);
	};

	//class Console : public Layout {
	//	const Font* font = nullptr;
	//	std::vector<Text> lines;
	//public:
	//	void load(noderef node);//font id
	//	inline Console(noderef node) : Layout(node) {
	//		load(node);
	//	}
	//	void push(const string& str);//lines transform `setFont(*font)`
	//	//virtual void update() override;
	//	//virtual void render() override;
	//};
	class Console {
		static HWND hwndTT;
	public:
		inline Console(const xml::ptree& node) {
			load(node);
			//create();
		}
		void load(const xml::ptree& node);
		inline static void create(HWND parentWnd) {
			if (hwndTT || !parentWnd) return;
			
		}
		inline static void cancel() {
			if (!hwndTT) return;
		}
		~Console() {
			cancel();
		}

	};
	class RivDesign : public CDesign {
		//string version = "Unknown riv version!";
		std::optional<TitleBar> version;
		SokuLib::Vector2i basePoint{ 0, 0 };
		std::map<string, Layout> sublayouts;
		std::optional<Console> console;
		static std::map<int, Font> _fonts;
		static CDesign* _current;
		static Console* _console;
	public:
		SokuLib::Vector2i windowSize{ 240, 980 };
		inline static const Font& getFont(int id) {
			auto it = _fonts.find(id);
			if (it != _fonts.end()) return it->second;
			return _fonts[-1];
		}
		inline static CDesign::Object* getObject(int id) {
			CDesign::Object* obj = nullptr;
			if (_current) _current->getById(&obj, id);
			return obj;
		}
		inline static CDesign::Sprite* getSprite(int id) {
			CDesign::Sprite* obj = nullptr;
			if (_current) _current->getById(&obj, id);
			return obj;
		}
		inline static void pushConsole(const string& str) {
			if (!_console) return;
			//_console->push(str);
		}
		RivDesign(const string& name = "rivpp/layout.xml", const string& name2 = "rivpp/layout_plus.txt") : CDesign() {
			auto temp = std::map<int, Font>{ {-1, Font()}, };
			_fonts.swap(temp);
			_current = nullptr;
			_console = nullptr;
			load(name, name2);
		}
		void load(const string&, const string& name2);
		void render(const string& cl, void* ctx);
		virtual void clear() override {
			CDesign::clear(); CDesign::objectMap.clear();
			_current = nullptr; _console = nullptr;
			sublayouts.clear();
			version.reset(); console.reset();
		}
	};
	
	/*------------------------------------------------------------------
	* Usage
	------------------------------------------------------------------*/


	class ValueSpec_ShaderColor : public Value {
		//unsigned int shaderColor = 0;
	public:
		inline ValueSpec_ShaderColor(const xml::ptree& node) : Value(node) {}
		inline virtual void update(void* ctx) override {
			using data = SokuLib::v2::GameObjectBase;
			if (!ctx) return;
			auto& obj = *reinterpret_cast<data*>(ctx);
			SokuLib::DrawUtils::DxSokuColor shaderColor;
			shaderColor.color = (unsigned int)obj.renderInfos.shaderColor;
			if (!obj.renderInfos.shaderType)
				shaderColor.color = 0;
			else if (obj.renderInfos.shaderType == 3)
				shaderColor.a = 0xFF;
			val = shaderColor.color;
		}
		inline virtual float getf() const override { return 0; }
		/*inline virtual int geti() const override {
			return shaderColor;
		}*/
	};
	class ValueSpec_SkillLevel : public Value {
	public:
		inline ValueSpec_SkillLevel(const xml::ptree& node) : Value() {}
		inline virtual void update(void* ctx) override {
			using data = SokuLib::v2::GameObjectBase;
			if (!ctx) return;
			auto& obj = *reinterpret_cast<data*>(ctx);
			constexpr int lim = _countof(SokuLib::v2::Player::effectiveSkillLevel);
			val = 0<=obj.skillIndex&&obj.skillIndex<lim && obj.gameData.owner ? obj.gameData.owner->effectiveSkillLevel[obj.skillIndex] : -1;
		}
	};
	class ValueSpec_Countdown {

	};
	class ValueSpec_Hitstop : public Value {
	public:
		bool check_count1();
	};
	class FrameFlags : public Grider {

	};
}