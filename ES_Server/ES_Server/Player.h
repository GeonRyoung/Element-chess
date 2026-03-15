#pragma once

class Player {
public:
    Player(int32_t profileId, string nickname, int32_t gold, int32_t level);

    bool CheckGold(int32_t amount) { return _gold >= amount; }
    void DeductGold(int32_t amount) { _gold -= amount; }

    int32_t GetProfileId() { return _profileId; }
    string GetNickname() { return _nickname; }
    int32_t GetGold() { return _gold; }
    int32_t GetLevel() { return _level; }

private:
    int32_t _profileId;
    string  _nickname;
    int32_t _gold;
    int32_t _level;
};

