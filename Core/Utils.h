#pragma once

#define CACHE_LINE_SIZE 64

#define ALIGN(x) __declspec(align(x))
#define CACHE_ALIGN ALIGN(CACHE_LINE_SIZE)

#ifdef __GNUC__
#define NORETURN __attribute__((noreturn))
#else
#ifdef _MSC_VER
#define NORETURN __declspec(noreturn)
#else
#define NORETURN
#endif
#endif

std::wstring formatLastWin32Error(std::wstring file, int line);

std::wstring formatError(std::wstring file, int line, std::wstring message);

void exitWithMessage(std::wstring message);

inline void breakWithMessage(std::wstring message) {
	OutputDebugString(message.c_str());
	DebugBreak();
}

inline void checkWin32(bool check, std::wstring file, int line) {
	if (check) breakWithMessage(formatLastWin32Error(file, line));
}

#define FORGOTTEN_ERROR(MESSAGE) do { breakWithMessage(formatError(TEXT(__FILE__), __LINE__, MESSAGE)); } while(false)

#define CHECK_WIN32(x) do { checkWin32(x, TEXT(__FILE__), __LINE__); } while(false)

// The same as boost::hash_combine except hasher agnostic
inline void hash_combine(size_t& seed, size_t hash) {
	seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<class Task> struct TypeWrapper {
	using type = Task;
};

template<class Task> struct TypeWrap {
	using type = TypeWrapper<Task>;
};

template<template<typename> class TTemplate>
struct IsFromTemplate1 {
	template<class Task> struct Check : mpl::bool_<false> {};
	template<class TArg> struct Check<TTemplate<TArg>> : mpl::bool_<true> {};
};

template<class Task> struct GetOnlyArgument;
template<template<typename> class TTemplate, class TArg> struct GetOnlyArgument<TTemplate<TArg>> {
	using type = TArg;
};

template<class... Task> void ignore(const Task&... x) {}

template<class Task>
struct FauxPointer {
	FauxPointer(const Task& value) : value(value) {}
	Task* operator->() {
		return &value;
	}
private:
	Task value;
};

template<class TCollection, class TFunc> void forEach(TCollection& collection, TFunc func) {
	using std::begin; using std::end;
	forEach(begin(collection), end(collection), func);
}

template<class TIter, class TSentinel, class TFunc> void forEach(TIter rangeBegin, TSentinel rangeEnd, TFunc func) {
	auto iter = rangeBegin;
	for (; iter != rangeEnd; ++iter)
		func(*iter);
}

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
