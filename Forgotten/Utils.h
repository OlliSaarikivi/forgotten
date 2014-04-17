#pragma once

//template<typename FwdIt, typename T, typename P>
//FwdIt binarySearch(FwdIt begin, FwdIt end, T const &val, P pred) {
//	FwdIt result = end;
//	while (begin != end) {
//		FwdIt middle(begin);
//		std::advance(middle, std::distance(begin, end) >> 1);
//		FwdIt middle_plus_one(middle);
//		++middle_plus_one;
//		auto &middle_value = *middle;
//		bool const gt_middle = pred(middle_value, val);
//		bool const lt_middle = pred(val, middle_value);
//		begin = gt_middle ? middle_plus_one : begin;
//		end = gt_middle ? end : (lt_middle ? middle : begin);
//		result = !gt_middle && !lt_middle ? middle : result;
//	}
//	return result;
//}

// The same as boost::hash_combine except hasher agnostic
inline void hash_combine(size_t& seed, size_t hash)
{
    seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std
{
    template<>
    struct hash<vec2>
    {
        size_t operator()(const vec2& v)
        {
            std::hash<float> hasher;
            size_t seed = 0;
            hash_combine(seed, hasher(v.x));
            hash_combine(seed, hasher(v.y));
            return seed;
        }
    };
}


inline b2Vec2 toB2(const vec2& v)
{
    return b2Vec2(v.x, v.y);
}

inline vec2 toGLM(const b2Vec2& v)
{
    return vec2(v.x, v.y);
}