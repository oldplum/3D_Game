#ifndef GAME_H
#define GAME_H

#include "Ball.h"
#include "Brick.h"
#include "Paddle.h"
#include "PowerUp.h"
#include <vector>

class Game {
public:
    Game();

    void Init();
    void Update();
    void Draw();
    void Shutdown();

private:
    enum GameState { MENU, PLAYING, GAME_OVER, PAUSED, LEADERBOARD, LEVEL_READY };

    struct LevelData {
        int level;
        float ballSpeedMultiplier;
        float paddleWidth;
        std::vector<int> brickPattern;
    };

    struct HighScore {
        int score;
        int level;
    };

    static const int SCREEN_WIDTH = 800;
    static const int SCREEN_HEIGHT = 600;

    GameState gameState;
    int lives;
    int score;
    int level;
    int combo;
    int frameCounter;
    float ballSpeedIncrease;
    int levelReadyCountdown;

    Ball ball;
    Ball extraBall;
    Paddle paddle;
    std::vector<Brick> bricks;
    std::vector<PowerUp> powerups;
    std::vector<HighScore> leaderboard;

    int paddleExpandTimer;
    int ballSlowTimer;
    int pierceTimer;
    bool multiballActive;

    LevelData InitializeLevel(int targetLevel) const;
    void RebuildBricks(const LevelData& levelData);
    void TryDropPowerUp(Vector2 brickPos);

    std::vector<HighScore> LoadLeaderboard() const;
    void SaveLeaderboard() const;

    void StartNewRun();
    void UpdateMenu();
    void UpdateLeaderboard();
    void UpdateLevelReady();
    void UpdatePlaying();
    void UpdatePaused();

    void CheckPaddleCollision(Ball& targetBall);
    void CheckBrickCollision(Ball& targetBall);
    bool CheckBottomCollision(const Ball& targetBall) const;
    void HandlePowerUpCatch(PowerUp& powerUp);
    bool AreAllBricksClear() const;
};

#endif
