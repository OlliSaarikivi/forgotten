#pragma once

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

inline float clamp(float x, float min, float max)
{
    return std::max(min, std::min(x, max));
}