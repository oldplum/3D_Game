#ifndef GAME_H
#define GAME_H

#include "Ball.h"
#include "Brick.h"
#include "Paddle.h"
#include "PowerUp.h"
#include <string>
#include <vector>

class Game {
public:
    Game();

    void Init();
    void Update();
    void Draw();
    void Shutdown();

private:
    enum class GameState {
        MENU,
        PLAYING,
        PAUSED,
        GAMEOVER,
        VICTORY,
        LEADERBOARD,
        LEVEL_READY
    };

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

    void LoadConfig(const std::string& path);

    GameState gameState;
    GameState stateBeforeLeaderboard;
    int screenWidth;
    int screenHeight;
    std::string windowTitle;

    float baseBallSpeed;
    float ballRadius;
    float paddleStartWidth;
    float paddleHeight;
    float paddleMoveSpeed;
    int initialLives;
    int powerUpDropChance;

    float brickWidth;
    float brickHeight;
    int brickColsPerRow;
    float brickSpacingX;
    float rowSpacing;
    float brickStartX;
    float brickStartY;

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
    void UpdateGameOver();
    void UpdateVictory();

    void CheckPaddleCollision(Ball& targetBall);
    void CheckBrickCollision(Ball& targetBall);
    bool CheckBottomCollision(const Ball& targetBall) const;
    void HandlePowerUpCatch(PowerUp& powerUp);
    bool AreAllBricksClear() const;
};

#endif
