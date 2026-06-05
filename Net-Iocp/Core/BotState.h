#pragma once
#include <cstdint>

enum class BotState : uint8_t
{
    IDLE = 0,
    MOVE,
    ATTACK,
    DEAD
};
