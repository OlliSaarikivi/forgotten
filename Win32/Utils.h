#pragma once

#ifdef __GNUC__
#define NORETURN __attribute__((noreturn))
#else
#ifdef _MSC_VER
#define NORETURN __declspec(noreturn)
#else
#define NORETURN
#endif
#endif

#define FORMAT(ITEMS) \
    ((dynamic_cast<std::ostringstream &> (\
    std::ostringstream().seekp(0, std::ios_base::cur) << ITEMS) \
    ).str())

// The same as boost::hash_combine except hasher agnostic
inline void hash_combine(size_t& seed, size_t hash) {
    seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<class T> struct TypeWrapper {
	using type = T;
};

template<class T> struct TypeWrap {
	using type = TypeWrapper<T>;
};

template<class... T> void ignore(const T&... x) {}

template<class T>
struct FauxPointer {
	FauxPointer(const T& value) : value(value) {}
	T* operator->() {
		return &value;
	}
private:
	T value;
};

//namespace std
//{
//    template<>
//    struct hash<vec2>
//    {
//        size_t operator()(const vec2& v)
//        {
//            std::hash<float> hasher;
//            size_t seed = 0;
//            hash_combine(seed, hasher(v.x));
//            hash_combine(seed, hasher(v.y));
//            return seed;
//        }
//    };
//}
//
//inline b2Vec2 toB2(const vec2& v)
//{
//    return b2Vec2(v.x, v.y);
//}
//
//inline vec2 toGLM(const b2Vec2& v)
//{
//    return vec2(v.x, v.y);
//}
//
//inline float clamp(float x, float min, float max)
//{
//    return std::max(min, std::min(x, max));
//}
//
//template<typename E>
//auto toIntegral(E e) -> typename std::underlying_type<E>::type
//{
//    return static_cast<typename std::underlying_type<E>::type>(e);
//}
