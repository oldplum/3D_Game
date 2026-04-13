#include "PowerUpEffect.h"
#include "Game.h"

namespace {

class ExtendPaddleEffect : public PowerUpEffect {
public:
    void Apply(Game& game) override {
        game.ApplyPaddleExpandEffect(game.GetPaddleExpandExtraWidth(), game.GetPaddleExpandDurationFrames(), 0);
    }
};

class SlowBallEffect : public PowerUpEffect {
public:
    void Apply(Game& game) override {
        game.ApplyBallSlowEffect(game.GetBallSlowSpeedFactor(), game.GetBallSlowDurationFrames(), 50);
    }
};

class PierceBallEffect : public PowerUpEffect {
public:
    void Apply(Game& game) override {
        game.ApplyBallPierceEffect(180, 75);
    }
};

class MultiBallEffect : public PowerUpEffect {
public:
    void Apply(Game& game) override {
        if (game.GetMultiBallExtraBalls() > 0) {
            game.ApplyMultiBallEffect(100);
        }
    }
};

class SlowFieldEffect : public PowerUpEffect {
public:
    void Apply(Game& game) override {
        game.ApplySlowFieldEffect(0.7f, 60);
    }
};

} // namespace

std::unique_ptr<PowerUpEffect> CreatePowerUpEffect(PowerUpType type) {
    switch (type) {
        case PADDLE_EXPAND:
            return std::make_unique<ExtendPaddleEffect>();
        case BALL_SLOW:
            return std::make_unique<SlowBallEffect>();
        case BALL_PIERCE:
            return std::make_unique<PierceBallEffect>();
        case MULTI_BALL:
            return std::make_unique<MultiBallEffect>();
        case SLOW_FIELD:
            return std::make_unique<SlowFieldEffect>();
        default:
            return nullptr;
    }
}
