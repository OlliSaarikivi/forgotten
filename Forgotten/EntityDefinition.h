#pragma once

struct MobileDef
{
    string true_name;
    vec2 position;
    b2Shape* shape;
    float knockback = 0;
};