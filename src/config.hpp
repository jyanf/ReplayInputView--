//template<typename T, typename... Ts>
//concept TypesCheck = (std::same_as<T, Ts> || ...);

#include <string>
#include <tuple>
#include <filesystem>
#include <Windows.h>
#include <Vector2.hpp>
#include <string_view>
namespace cfg {
    
    using string = std::string;
    typedef const string::value_type *cstring, cstringbuf[];
    template <size_t N> using cstringarr = const string::value_type[N];
    using string_view = std::basic_string_view<std::string::value_type>;

//--------------------------------
// 定义字面量
//--------------------------------
    namespace _string_literal {
        
        template<size_t N>
        struct _literal_helper {
            string::value_type str[N];
            constexpr _literal_helper(const string::value_type(&s)[N]) {
                for (size_t i = 0; i < N; ++i) str[i] = s[i];
            }
            
            constexpr bool operator==(const string_view& str_view) const {
                return string_view(str) == str_view;
            }
        };
        template<_literal_helper S> struct _literal {
            static constexpr auto str = S;
            static constexpr string_view str_view = S.str;
            constexpr operator cstring() {
                return str_view.data();
            }
        };

        template <_literal_helper Target, typename First, typename... Rest>
        consteval size_t find_index() {
            if constexpr (Target == First::str_view) {
                return 0;
            }
            else {
                return 1 + find_index<Target, Rest...>();
            }
        }
        // 从 tuple 中按 字符串字面量 查找值
        template <_literal_helper Key, typename... Fields>
        auto& get_field(std::tuple<Fields...>& t) {
            return std::get<find_index<Key, Fields...>()>(t);
        }


        template<typename... Fields> struct keys_are_unique;
        template<> struct keys_are_unique<> : std::true_type {};
        template<typename F, typename... Rest>
        struct keys_are_unique<F, Rest...>
            : std::bool_constant<((!(F::str_view == Rest::str_view)) && ...) && keys_are_unique<Rest...>::value> {
        };

    }

    template<size_t N> using Literal = _string_literal::_literal_helper<N>;

    template<Literal Key, typename Adapter>
    struct Field {
        static constexpr auto str = Key;
        static constexpr string_view str_view = Key.str;
        Adapter value;
        using value_type = Adapter::value_type;

        //Field(const Adapter& a) : value(a) {}
        //Field(Adapter&& a = Adapter()) : value(std::move(a)) {}
        Field() {}
        Field(const value_type& v) : value(v) {}
        Field(value_type&& v) : value(std::move(v)) {}

        operator value_type() const { return value; }
        // 裸类型赋值
        Field& operator=(const value_type& v) {
            this->value = v;
            return *this;
        }
        Field& operator=(value_type&& v) {
            this->value = std::move(v);
            return *this;
        }

        void read(const cstring& path, const cstring& section) {
            value.read(path, section, str_view.data());
        }
        void write(const cstring& path, const cstring& section) const {
            value.write(path, section, str_view.data());
        }
    };
    //template <Literal Key, typename Adapter> Field(Adapter) -> Field<Key, Adapter>;

    template<Literal Name, typename... Fields>
    struct Section {
        static constexpr auto str = Name;
        static constexpr string_view str_view = Name.str;
        //static constexpr _string_literal::_literal<Name> literal{};
        static_assert(_string_literal::keys_are_unique<Fields...>::value, "Duplicate Field keys in Section!");
        std::tuple<Fields...> fields;
        
        Section(Fields... fs) : fields(std::move(fs)...) {}

        template<typename Key>
        auto& operator[](Key key) {
            //return _string_literal::get_field < Key::str > (fields);
            return get<Key::str>();
        }
        template <Literal key>
        auto& get() {
            return _string_literal::get_field<key>(fields);
        }
        void read(const cstring& path) {
            std::apply([&](auto&... f) { (f.read(path, str_view.data()), ...); }, fields);
        }
        void write(const cstring& path) const {
            std::apply([&](auto&... f) { (f.write(path, str_view.data()), ...); }, fields);
        }
    };

    template<typename... Sections>
    class Config {
        static_assert(_string_literal::keys_are_unique<Sections...>::value, "Duplicate Section names in Config!");
        std::filesystem::path path;
        std::tuple<Sections...> sections;
    public:

        Config(const std::string& p, Sections... ss) : path(p), sections(std::move(ss)...) {}

        template<typename Key>
        auto& operator[](Key name) {
            //return _string_literal::get_field < Key::str, Sections...>(sections);
            return get<Key::str>();
        }
        template <Literal key>
        auto& get() {
            return _string_literal::get_field<key>(sections);
        }

        void read() {
            std::apply([&](auto&... s) { (s.read(path.string().c_str()), ...); }, sections);
        }
        void write() const {
            std::apply([&](auto&... s) { (s.write(path.string().c_str()), ...); }, sections);
        }
        inline void setPath(const std::filesystem::path& p) { path = p; }
        inline auto getPath() const { return path; }
    };
    //template <Literal Name, typename... Fs> Section(Fs...) -> Section<Name, Fs...>;
    namespace _support_types {
        template<typename Var>
        struct ValueBase {
            using Base = ValueBase<Var>;
            using value_type = Var;

            value_type value;
            ValueBase(value_type v) : value(std::move(v)) {}
            operator value_type() const { return value; }
            virtual void read(const cstring& path, const cstring& section, const cstring& key) = 0;
            virtual void write(const cstring& path, const cstring& section, const cstring& key) const = 0;

            value_type operator=(value_type v) { value = v; }
        };
        //--------------------------------
        // 自定义类型适配
        //--------------------------------
        struct Integer : ValueBase<int> {
            Integer(int v = 0) : Base(v) {}

            void read(const cstring& path, const cstring& section, const cstring& key) override {
                value = GetPrivateProfileIntA(section, key, value, path);
            }
            void write(const cstring& path, const cstring& section, const cstring& key) const override {
                WritePrivateProfileStringA(section, key, std::to_string(value).c_str(), path);
            }
        };

        struct String : ValueBase<string> {
            String(cstring v = "") : Base(v) {}

            void read(const cstring& path, const cstring& section, const cstring& key) override {
                char buf[1024];
                GetPrivateProfileStringA(section, key, value.c_str(), buf, sizeof(buf), path);
            }
            void write(const cstring& path, const cstring& section, const cstring& key) const override {
                WritePrivateProfileStringA(section, key, value.c_str(), path);
            }
        };

        struct Point : ValueBase<SokuLib::Vector2f> {
            Point(SokuLib::Vector2f v = { 0,0 }) : Base(v) {}

            void read(const cstring& path, const cstring& section, const cstring& key) override {
                char buf[128];
                sprintf_s(buf, "%f,%f", value.x, value.y);
                GetPrivateProfileStringA(section, key, buf, buf, sizeof(buf), path);
                sscanf_s(buf, "%f,%f", &value.x, &value.y);
            }
            void write(const cstring& path, const cstring& section, const cstring& key) const override {
                char buf[64];
                sprintf_s(buf, "%g, %g", value.x, value.y);
                WritePrivateProfileStringA(section, key, buf, path);
            }
        };
    }
    namespace ex {
        template <Literal Name, typename... Fs>
        auto addSection(Fs&&... fs) {
            return Section<Name, std::decay_t<Fs>...>(std::forward<Fs>(fs)...);
        }
        /*template<typename U, typename Adapter>
        concept AdapterAccepts = requires(Adapter v) {
            std::same_as<decltype(v.value), U>;
            { Adapter(U()) };
        };
        template<size_t N, typename U, AdapterAccepts<U> Adapter>
        auto addField(const char key[N], U&& v) {
            return Field < Literal{key}, Adapter > (std::forward<U>(v));
        }*/
        template<Literal key, typename Adapter>
        auto addField(Adapter&& v) {
            //using Adapter = _support_types::to_adapter<U>::type;
            return Field<key, Adapter>(std::forward<Adapter::value_type>(v));
        }
        template<Literal key> using addInteger = Field<key, _support_types::Integer>;
        template<Literal key> using addString = Field<key, _support_types::String>;
        template<Literal key> using addPoint = Field<key, _support_types::Point>;

        
        template<Literal S>
        constexpr _string_literal::_literal<S> operator""_l() {
            return {};
        }
    }
}

//constexpr auto test = cfg::Literal("nihao");
//constexpr bool csame = cfg::Literal{ "123" } == "123";
//auto tt = cfg::ex::addPoint<"t">::str.str;
//inline auto tester = cfg::Config{
//    "ini",
//    cfg::addSection<"s0">(
//        //cfg::addField<"nihao2">(123)
//        cfg::addField<"f0">(123)
//    ),
//};
//auto test3 = decltype("li"_l)::str_view;