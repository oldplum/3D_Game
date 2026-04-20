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

    void ApplyPaddleExpandEffect(float extraWidth, int durationFrames, int scoreBonus);
    void ApplyBallSlowEffect(float speedFactor, int durationFrames, int scoreBonus);
    void ApplyBallPierceEffect(int durationFrames, int scoreBonus);
    void ApplyMultiBallEffect(int scoreBonus);
    void ApplySlowFieldEffect(float factor, int scoreBonus);

    void SaveGameState(const std::string& path) const;
    bool LoadGameState(const std::string& path);

    float GetPaddleExpandExtraWidth() const { return powerUpSettings.paddleExpandExtraWidth; }
    int GetPaddleExpandDurationFrames() const { return powerUpSettings.paddleExpandDurationFrames; }
    float GetBallSlowSpeedFactor() const { return powerUpSettings.ballSlowSpeedFactor; }
    int GetBallSlowDurationFrames() const { return powerUpSettings.ballSlowDurationFrames; }
    int GetMultiBallExtraBalls() const { return powerUpSettings.multiBallExtraBalls; }

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

    struct SerializedPowerUp {
        int type;
        float x;
        float y;
        bool active;
    };

    struct GameStateData {
        int gameState;
        int lives;
        int score;
        int level;
        int combo;
        int frameCounter;
        float ballSpeedIncrease;
        int levelReadyCountdown;
        bool multiballActive;
        bool ballSlowActive;
        bool droppedPowerUpThisLevel;
        float ballX;
        float ballY;
        float ballSpeedX;
        float ballSpeedY;
        float extraBallX;
        float extraBallY;
        float extraBallSpeedX;
        float extraBallSpeedY;
        float paddleX;
        float paddleY;
        float paddleWidth;
        std::vector<int> brickActive;
        std::vector<SerializedPowerUp> powerups;
    };

    struct PowerUpSettings {
        float paddleExpandExtraWidth = 40.0f;
        int paddleExpandDurationFrames = 300;
        float paddleExpandDropRate = 0.30f;

        int multiBallExtraBalls = 1;
        int multiBallDurationFrames = 0;
        float multiBallDropRate = 0.20f;

        float ballSlowSpeedFactor = 0.70f;
        int ballSlowDurationFrames = 300;
        float ballSlowDropRate = 0.25f;
    };

    struct Particle {
        Vector2 position;
        Vector2 velocity;
        Color color;
        float life;
        float maxLife;
        float size;
        bool active;
    };

    void LoadConfig(const std::string& path);
    GameStateData CaptureGameState() const;
    void ApplyGameState(const GameStateData& data);

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
    PowerUpSettings powerUpSettings;

    Ball ball;
    Ball extraBall;
    Paddle paddle;
    std::vector<Brick> bricks;
    std::vector<PowerUp> powerups;
    std::vector<Particle> particles;
    std::vector<HighScore> leaderboard;

    int paddleExpandTimer;
    int ballSlowTimer;
    int pierceTimer;
    bool multiballActive;
    bool ballSlowActive;
    bool droppedPowerUpThisLevel;

    LevelData InitializeLevel(int targetLevel) const;
    void RebuildBricks(const LevelData& levelData);
    void TryDropPowerUp(Vector2 brickPos);
    void SpawnBrickParticles(const Rectangle& brickRect, Color brickColor);
    void UpdateParticles();
    void DrawParticles();
    Color GetBrickColor(int brickType) const;

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
