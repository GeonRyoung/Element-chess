#include "pch.h"
#include "Player.h"

Player::Player(int32_t profileId, string nickname, int32_t gold, int32_t level)
    : _profileId(profileId), _nickname(nickname), _gold(gold), _level(level) {}