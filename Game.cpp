#include "Game.h"
#include <nlohmann/json.hpp>
#include "raylib.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>

using json = nlohmann::json;

Game::Game()
        : gameState(GameState::MENU),
            stateBeforeLeaderboard(GameState::MENU),
    screenWidth(800),
    screenHeight(600),
    windowTitle("Breakout"),
    baseBallSpeed(2.0f),
    ballRadius(10.0f),
    paddleStartWidth(150.0f),
    paddleHeight(20.0f),
    paddleMoveSpeed(9.0f),
    initialLives(3),
    powerUpDropChance(45),
    brickWidth(80.0f),
    brickHeight(30.0f),
    brickColsPerRow(4),
    brickSpacingX(90.0f),
    rowSpacing(50.0f),
    brickStartX(60.0f),
    brickStartY(80.0f),
      lives(3),
      score(0),
      level(1),
      combo(0),
      frameCounter(0),
      ballSpeedIncrease(1.0f),
      levelReadyCountdown(0),
    ball({400, 300}, {2, 2}, ballRadius),
      extraBall({-1000, -1000}, {0, 0}, 10),
    paddle(300, 550, paddleStartWidth, paddleHeight),
      paddleExpandTimer(0),
      ballSlowTimer(0),
      pierceTimer(0),
      multiballActive(false) {}

void Game::LoadConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return;
    }

    try {
        json config = json::parse(file);

        if (config.contains("window")) {
            const auto& window = config["window"];
            screenWidth = window.value("width", screenWidth);
            screenHeight = window.value("height", screenHeight);
            windowTitle = window.value("title", windowTitle);
        }

        if (config.contains("ball")) {
            const auto& ballConfig = config["ball"];
            ballRadius = ballConfig.value("radius", ballRadius);
            baseBallSpeed = ballConfig.value("baseSpeed", baseBallSpeed);
        }

        if (config.contains("paddle")) {
            const auto& paddleConfig = config["paddle"];
            paddleStartWidth = paddleConfig.value("width", paddleStartWidth);
            paddleHeight = paddleConfig.value("height", paddleHeight);
            paddleMoveSpeed = paddleConfig.value("speed", paddleMoveSpeed);
        }

        if (config.contains("bricks")) {
            const auto& bricksConfig = config["bricks"];
            brickWidth = bricksConfig.value("width", brickWidth);
            brickHeight = bricksConfig.value("height", brickHeight);
            brickColsPerRow = bricksConfig.value("cols", brickColsPerRow);
            brickSpacingX = bricksConfig.value("spacingX", brickSpacingX);
            rowSpacing = bricksConfig.value("spacingY", rowSpacing);
            brickStartX = bricksConfig.value("startX", brickStartX);
            brickStartY = bricksConfig.value("startY", brickStartY);
        }

        if (config.contains("game")) {
            const auto& gameConfig = config["game"];
            initialLives = gameConfig.value("initialLives", initialLives);
            powerUpDropChance = gameConfig.value("powerUpDropChance", powerUpDropChance);
        }
    } catch (const std::exception&) {
        // 配置解析失败时回退默认值，保持游戏可运行。
    }
}

void Game::Init() {
    LoadConfig("config.json");
    InitWindow(screenWidth, screenHeight, windowTitle.c_str());
    SetTargetFPS(60);
    srand(static_cast<unsigned>(time(NULL)));
    leaderboard = LoadLeaderboard();
}

void Game::Update() {
    frameCounter++;

    if (IsKeyPressed(KEY_L)) {
        if (gameState == GameState::LEADERBOARD) {
            gameState = stateBeforeLeaderboard;
        } else {
            stateBeforeLeaderboard = gameState;
            gameState = GameState::LEADERBOARD;
        }
        return;
    }

    switch (gameState) {
        case GameState::MENU:
            UpdateMenu();
            break;
        case GameState::PLAYING:
            UpdatePlaying();
            break;
        case GameState::PAUSED:
            UpdatePaused();
            break;
        case GameState::GAMEOVER:
            UpdateGameOver();
            break;
        case GameState::VICTORY:
            UpdateVictory();
            break;
        case GameState::LEADERBOARD:
            UpdateLeaderboard();
            break;
        case GameState::LEVEL_READY:
            UpdateLevelReady();
            break;
    }
}

void Game::Draw() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    DrawRectangle(0, 0, 5, screenHeight, GRAY);
    DrawRectangle(screenWidth - 5, 0, 5, screenHeight, GRAY);
    DrawRectangle(0, 0, screenWidth, 5, GRAY);
    DrawRectangle(0, screenHeight - 5, screenWidth, 5, GRAY);

    if (gameState == GameState::MENU) {
        DrawText("BREAKOUT 2D", screenWidth / 2 - 150, 80, 60, DARKBLUE);
        DrawText("Press SPACE to Start", screenWidth / 2 - 180, 250, 32, DARKGRAY);
        DrawText("Press L to View Leaderboard", screenWidth / 2 - 200, 320, 24, DARKGRAY);
        DrawText("Controls: <- -> to move paddle | P to pause", screenWidth / 2 - 250, 450, 20, GRAY);
    } else if (gameState == GameState::LEADERBOARD) {
        DrawText("TOP 10 SCORES", screenWidth / 2 - 150, 50, 40, DARKBLUE);
        for (size_t i = 0; i < leaderboard.size() && i < 10; i++) {
            DrawText(TextFormat("#%d: %d pts (Level %d)", i + 1, leaderboard[i].score, leaderboard[i].level),
                100, 120 + i * 40, 24, DARKGRAY);
        }
        DrawText("Press L to return", screenWidth / 2 - 150, screenHeight - 50, 20, GRAY);
    } else if (gameState == GameState::LEVEL_READY) {
        ball.Draw();
        paddle.Draw();
        for (auto& brick : bricks) brick.Draw();

        DrawText(TextFormat("Score: %d", score), 12, 10, 20, DARKGRAY);
        DrawText(TextFormat("Level: %d", level), 12, 40, 20, DARKGRAY);
        DrawText(TextFormat("Lives: %d", lives), 12, 70, 20, DARKGRAY);

        int secondsLeft = (levelReadyCountdown + 59) / 60;
        if (secondsLeft > 0) {
            DrawText("READY?", screenWidth / 2 - 100, screenHeight / 2 - 80, 50, RED);
            DrawText(TextFormat("%d", secondsLeft), screenWidth / 2 - 30, screenHeight / 2 + 50, 80, ORANGE);
        } else {
            DrawText("GO!", screenWidth / 2 - 50, screenHeight / 2 - 40, 60, GREEN);
        }
    } else if (gameState == GameState::PLAYING) {
        ball.Draw();
        if (multiballActive) extraBall.Draw();
        paddle.Draw();
        for (auto& brick : bricks) brick.Draw();
        for (auto& powerUp : powerups) powerUp.Draw();

        DrawText(TextFormat("Score: %d", score), 12, 10, 20, DARKGRAY);
        DrawText(TextFormat("Level: %d", level), 12, 40, 20, DARKGRAY);
        DrawText(TextFormat("Lives: %d", lives), 12, 70, 20, lives <= 1 ? RED : DARKGRAY);
        if (combo > 0) {
            DrawText(TextFormat("Combo: %d x%.1f", combo, 1.0f + (combo / 5.0f)), 600, 10, 18, ORANGE);
        }
        DrawText(TextFormat("Speed: %.1f x", ballSpeedIncrease), 12, 100, 16, DARKGREEN);

        if (paddleExpandTimer > 0) DrawText("PADDLE+", 350, 520, 16, GREEN);
        if (ballSlowTimer > 0) DrawText("SLOW", 350, 520, 16, YELLOW);
        if (pierceTimer > 0) DrawText("PIERCE", 350, 520, 16, RED);
        if (multiballActive) DrawText("2 BALLS", 350, 520, 16, MAGENTA);
    } else if (gameState == GameState::GAMEOVER) {
        DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 200});
        DrawText("GAME OVER", screenWidth / 2 - 150, screenHeight / 2 - 60, 48, RED);
        DrawText(TextFormat("Final Score: %d | Level: %d", score, level), screenWidth / 2 - 200, screenHeight / 2, 28, WHITE);
        DrawText("Press R to return Menu", screenWidth / 2 - 170, screenHeight / 2 + 100, 24, WHITE);
    } else if (gameState == GameState::VICTORY) {
        DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 200});
        DrawText("VICTORY", screenWidth / 2 - 120, screenHeight / 2 - 60, 56, GREEN);
        DrawText(TextFormat("Final Score: %d | Level: %d", score, level), screenWidth / 2 - 200, screenHeight / 2, 28, WHITE);
        DrawText("Press R to return Menu", screenWidth / 2 - 170, screenHeight / 2 + 100, 24, WHITE);
    } else if (gameState == GameState::PAUSED) {
        DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 150});
        DrawText("PAUSED", screenWidth / 2 - 100, screenHeight / 2 - 30, 50, WHITE);
        DrawText("Press P to Resume", screenWidth / 2 - 150, screenHeight / 2 + 50, 24, WHITE);
    }

    EndDrawing();
}

void Game::Shutdown() {
    SaveLeaderboard();
    CloseWindow();
}

Game::LevelData Game::InitializeLevel(int targetLevel) const {
    LevelData data;
    data.level = targetLevel;

    if (targetLevel == 1) data.ballSpeedMultiplier = 1.0f;
    else if (targetLevel == 2) data.ballSpeedMultiplier = 1.5f;
    else if (targetLevel == 3) data.ballSpeedMultiplier = 2.0f;
    else data.ballSpeedMultiplier = 2.7f + (targetLevel - 4) * 0.5f;

    if (targetLevel == 1) data.paddleWidth = paddleStartWidth;
    else if (targetLevel == 2) data.paddleWidth = paddleStartWidth * 0.8f;
    else if (targetLevel == 3) data.paddleWidth = paddleStartWidth * 0.6f;
    else data.paddleWidth = paddleStartWidth * 0.4f;

    data.brickPattern.clear();
    if (targetLevel == 1) {
        for (int i = 0; i < 4; i++) data.brickPattern.push_back(1);
    } else if (targetLevel == 2) {
        for (int i = 0; i < 4; i++) data.brickPattern.push_back(1);
        for (int i = 0; i < 2; i++) data.brickPattern.push_back(2);
    } else if (targetLevel == 3) {
        for (int i = 0; i < 4; i++) data.brickPattern.push_back(1);
        for (int i = 0; i < 3; i++) data.brickPattern.push_back(2);
        data.brickPattern.push_back(3);
    } else {
        for (int i = 0; i < 4; i++) data.brickPattern.push_back(1);
        for (int i = 0; i < 5; i++) data.brickPattern.push_back(2);
        for (int i = 0; i < 3; i++) data.brickPattern.push_back(3);
    }

    return data;
}

void Game::RebuildBricks(const LevelData& levelData) {
    bricks.clear();
    for (size_t i = 0; i < levelData.brickPattern.size(); i++) {
        int type = levelData.brickPattern[i];
        int row = static_cast<int>(i) / brickColsPerRow;
        int col = static_cast<int>(i) % brickColsPerRow;
        bricks.emplace_back(brickStartX + col * brickSpacingX, brickStartY + row * rowSpacing, brickWidth, brickHeight, type);
    }
}

void Game::TryDropPowerUp(Vector2 brickPos) {
    if (rand() % 100 < powerUpDropChance) {
        PowerUpType types[] = {PADDLE_EXPAND, BALL_SLOW, BALL_PIERCE, MULTI_BALL, SLOW_FIELD};
        Vector2 pos = {brickPos.x + brickWidth * 0.5f, brickPos.y + brickHeight * 0.5f};
        powerups.emplace_back(pos, types[rand() % 5]);
    }
}

std::vector<Game::HighScore> Game::LoadLeaderboard() const {
    std::vector<HighScore> scores;
    std::ifstream file("leaderboard.txt");
    if (file.is_open()) {
        int loadedScore, loadedLevel;
        while (file >> loadedScore >> loadedLevel) {
            scores.push_back({loadedScore, loadedLevel});
        }
        file.close();
    }
    return scores;
}

void Game::SaveLeaderboard() const {
    std::ofstream file("leaderboard.txt");
    for (const auto& entry : leaderboard) {
        file << entry.score << " " << entry.level << "\n";
    }
    file.close();
}

void Game::StartNewRun() {
    lives = initialLives;
    score = 0;
    level = 1;
    combo = 0;
    ballSpeedIncrease = 1.0f;
    frameCounter = 0;
    paddleExpandTimer = 0;
    ballSlowTimer = 0;
    pierceTimer = 0;
    multiballActive = false;
    levelReadyCountdown = 180;

    LevelData currentLevel = InitializeLevel(level);
    int randomXRange = std::max(1, screenWidth - 200);
    int randomX = 100 + rand() % randomXRange;
    int randomY = 120 + rand() % 180;
    ball = Ball({static_cast<float>(randomX), static_cast<float>(randomY)}, {0, 0}, ballRadius);
    paddle = Paddle((screenWidth - currentLevel.paddleWidth) * 0.5f, screenHeight - 50.0f, currentLevel.paddleWidth, paddleHeight);
    RebuildBricks(currentLevel);
    powerups.clear();
    gameState = GameState::LEVEL_READY;
}

void Game::UpdateMenu() {
    if (IsKeyPressed(KEY_SPACE)) {
        StartNewRun();
    }
}

void Game::UpdateLeaderboard() {
}

void Game::UpdateLevelReady() {
    levelReadyCountdown--;

    if (levelReadyCountdown <= 0) {
        LevelData currentLevel = InitializeLevel(level);
        float levelBallSpeed = baseBallSpeed * currentLevel.ballSpeedMultiplier * ballSpeedIncrease;
        ball.SetSpeed({levelBallSpeed, levelBallSpeed});
        gameState = GameState::PLAYING;
        frameCounter = 0;
    }
}

void Game::UpdatePlaying() {
    if (frameCounter > 0 && frameCounter % 600 == 0) {
        ballSpeedIncrease += 0.1f;
    }

    ball.Move();
    ball.BounceEdge(screenWidth, screenHeight);

    if (multiballActive) {
        extraBall.Move();
        extraBall.BounceEdge(screenWidth, screenHeight);
    }

    for (auto& powerUp : powerups) {
        powerUp.Update();
    }
    powerups.erase(std::remove_if(powerups.begin(), powerups.end(),
        [](const PowerUp& p) { return !p.IsActive(); }), powerups.end());

    if (paddleExpandTimer > 0) paddleExpandTimer--;
    if (ballSlowTimer > 0) ballSlowTimer--;
    if (pierceTimer > 0) pierceTimer--;

    if (IsKeyDown(KEY_LEFT)) paddle.MoveLeft(paddleMoveSpeed);
    if (IsKeyDown(KEY_RIGHT)) paddle.MoveRight(paddleMoveSpeed);
    if (IsKeyPressed(KEY_P)) gameState = GameState::PAUSED;

    CheckPaddleCollision(ball);
    if (multiballActive) CheckPaddleCollision(extraBall);

    CheckBrickCollision(ball);
    if (multiballActive) CheckBrickCollision(extraBall);

    for (auto& powerUp : powerups) {
        if (CheckCollisionCircleRec(powerUp.GetPosition(), powerUp.GetRadius(), paddle.GetRect())) {
            HandlePowerUpCatch(powerUp);
            powerUp.SetActive(false);
        }
    }

    if (CheckBottomCollision(ball)) {
        if (multiballActive) {
            multiballActive = false;
        } else {
            lives--;
            if (lives <= 0) {
                gameState = GameState::GAMEOVER;
                leaderboard.push_back({score, level});
                std::sort(leaderboard.rbegin(), leaderboard.rend(),
                    [](const HighScore& a, const HighScore& b) { return a.score < b.score; });
                if (leaderboard.size() > 10) leaderboard.pop_back();
                SaveLeaderboard();
            } else {
                int randomXRange = std::max(1, screenWidth - 200);
                int randomX = 100 + rand() % randomXRange;
                int randomY = 120 + rand() % 180;
                ball = Ball({static_cast<float>(randomX), static_cast<float>(randomY)}, {0, 0}, ballRadius);
                combo = 0;
                levelReadyCountdown = 180;
                gameState = GameState::LEVEL_READY;
            }
        }
    }

    if (CheckBottomCollision(extraBall)) {
        multiballActive = false;
    }

    if (AreAllBricksClear()) {
        gameState = GameState::VICTORY;
        leaderboard.push_back({score, level});
        std::sort(leaderboard.rbegin(), leaderboard.rend(),
            [](const HighScore& a, const HighScore& b) { return a.score < b.score; });
        if (leaderboard.size() > 10) leaderboard.pop_back();
        SaveLeaderboard();
    }
}

void Game::UpdatePaused() {
    if (IsKeyPressed(KEY_P)) gameState = GameState::PLAYING;
}

void Game::UpdateGameOver() {
    if (IsKeyPressed(KEY_R)) {
        gameState = GameState::MENU;
    }
}

void Game::UpdateVictory() {
    if (IsKeyPressed(KEY_R)) {
        gameState = GameState::MENU;
    }
}

void Game::CheckPaddleCollision(Ball& targetBall) {
    Vector2 ballPos = targetBall.GetPosition();
    Vector2 ballSpeed = targetBall.GetSpeed();
    float ballRadius = targetBall.GetRadius();
    Rectangle paddleRect = paddle.GetRect();

    if (ballSpeed.y > 0 && CheckCollisionCircleRec(ballPos, ballRadius, paddleRect)) {
        targetBall.ReverseY();
        ballPos.y = paddleRect.y - ballRadius - 1.0f;
        targetBall.SetPosition(ballPos);
    }
}

void Game::CheckBrickCollision(Ball& targetBall) {
    bool hitBrick = false;

    for (auto& brick : bricks) {
        if (!brick.IsActive()) continue;

        if (CheckCollisionCircleRec(targetBall.GetPosition(), targetBall.GetRadius(), brick.GetRect())) {
            if (pierceTimer == 0) {
                if (brick.Hit()) {
                    int basePoints = brick.GetPoints();
                    int multiplier = 1 + (combo / 5);
                    score += basePoints * multiplier;
                    combo++;
                    TryDropPowerUp({brick.GetRect().x, brick.GetRect().y});
                }
                targetBall.ReverseY();
                hitBrick = true;
                break;
            } else {
                if (brick.Hit()) {
                    score += brick.GetPoints();
                    combo++;
                    TryDropPowerUp({brick.GetRect().x, brick.GetRect().y});
                }
            }
        }
    }

    if (!hitBrick) combo = 0;
}

bool Game::CheckBottomCollision(const Ball& targetBall) const {
    return targetBall.GetPosition().y + targetBall.GetRadius() >= screenHeight;
}

void Game::HandlePowerUpCatch(PowerUp& powerUp) {
    switch (powerUp.GetType()) {
        case PADDLE_EXPAND:
            paddle.SetWidth(paddle.GetWidth() + 50);
            paddleExpandTimer = 300;
            break;
        case BALL_SLOW:
            ballSpeedIncrease = std::max(0.5f, ballSpeedIncrease - 0.3f);
            ballSlowTimer = 300;
            score += 50;
            break;
        case BALL_PIERCE:
            pierceTimer = 180;
            score += 75;
            break;
        case MULTI_BALL:
            if (!multiballActive) {
                multiballActive = true;
                extraBall = ball;
                extraBall.SetSpeed({-ball.GetSpeed().x, ball.GetSpeed().y});
            }
            score += 100;
            break;
        case SLOW_FIELD:
            ballSpeedIncrease *= 0.7f;
            score += 60;
            break;
    }
}

bool Game::AreAllBricksClear() const {
    for (const auto& brick : bricks) {
        if (brick.IsActive()) {
            return false;
        }
    }
    return true;
}
