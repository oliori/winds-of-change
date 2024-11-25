#pragma once

#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <cstdint>
#include <iostream>
#include <optional>
#include <vector>
#include <cassert>

#define woc_internal static
#define woc_global static
#define woc_local static

namespace woc
{
    using i8 = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;
    using u8 = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;
    using f32 = float;
    using f64 = double;
    
    struct Radian {
        f32 val;
    };
    struct Degree {
        f32 val;
    };

}
