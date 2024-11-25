#include "windsofchange.h"

namespace woc
{
    woc_internal constexpr Level level_1()
    {
        auto result = Level {};
        result.enemies[1][3] = EnemyState {
            .health = 10.f
        };
        
        return result;
    }
}
