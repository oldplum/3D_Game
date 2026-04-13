#ifndef POWERUPEFFECT_H
#define POWERUPEFFECT_H

#include "PowerUp.h"
#include <memory>

class Game;

class PowerUpEffect {
public:
    virtual ~PowerUpEffect() = default;
    virtual void Apply(Game& game) = 0;
};

std::unique_ptr<PowerUpEffect> CreatePowerUpEffect(PowerUpType type);

#endif
