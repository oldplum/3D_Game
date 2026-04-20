#include "Game.h"
#include "PowerUpEffect.h"
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
        networkMode(NetworkMode::NONE),
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
            powerUpSettings(),
    ball({400, 300}, {2, 2}, ballRadius),
      extraBall({-1000, -1000}, {0, 0}, 10),
    paddle(300, 550, paddleStartWidth, paddleHeight),
      paddleExpandTimer(0),
      ballSlowTimer(0),
      pierceTimer(0),
            multiballActive(false),
                        ballSlowActive(false),
                droppedPowerUpThisLevel(false),
                remotePaddleX(300.0f),
                remotePaddleY(80.0f),
                remotePaddleWidth(paddleStartWidth),
                interpolationActive(false),
                interpolationStartTime(0.0),
                interpolationDuration(0.05),
                interpolationBallFrom({0.0f, 0.0f}),
                interpolationBallTo({0.0f, 0.0f}),
                interpolationExtraBallFrom({0.0f, 0.0f}),
                interpolationExtraBallTo({0.0f, 0.0f}),
                interpolationPaddleFrom({0.0f, 0.0f, 0.0f, 0.0f}),
                interpolationPaddleTo({0.0f, 0.0f, 0.0f, 0.0f}) {}

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

        if (config.contains("powerups")) {
            const auto& powerupsConfig = config["powerups"];

            if (powerupsConfig.contains("paddle_extend")) {
                const auto& paddleExtend = powerupsConfig["paddle_extend"];
                powerUpSettings.paddleExpandExtraWidth = paddleExtend.value("extra_width", powerUpSettings.paddleExpandExtraWidth);
                powerUpSettings.paddleExpandDurationFrames = paddleExtend.value("duration", powerUpSettings.paddleExpandDurationFrames / 60) * 60;
                powerUpSettings.paddleExpandDropRate = paddleExtend.value("drop_rate", powerUpSettings.paddleExpandDropRate);
            }

            if (powerupsConfig.contains("multi_ball")) {
                const auto& multiBall = powerupsConfig["multi_ball"];
                powerUpSettings.multiBallExtraBalls = multiBall.value("extra_balls", powerUpSettings.multiBallExtraBalls);
                powerUpSettings.multiBallDurationFrames = multiBall.value("duration", powerUpSettings.multiBallDurationFrames / 60) * 60;
                powerUpSettings.multiBallDropRate = multiBall.value("drop_rate", powerUpSettings.multiBallDropRate);
            }

            if (powerupsConfig.contains("slow_ball")) {
                const auto& slowBall = powerupsConfig["slow_ball"];
                powerUpSettings.ballSlowSpeedFactor = slowBall.value("speed_factor", powerUpSettings.ballSlowSpeedFactor);
                powerUpSettings.ballSlowDurationFrames = slowBall.value("duration", powerUpSettings.ballSlowDurationFrames / 60) * 60;
                powerUpSettings.ballSlowDropRate = slowBall.value("drop_rate", powerUpSettings.ballSlowDropRate);
            }
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
        case GameState::NETWORK_WAITING:
            UpdateNetworkClient();
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
        DrawText("Press H to Host | Press C to Connect localhost", screenWidth / 2 - 250, 400, 20, DARKGRAY);
        DrawText("Controls: <- -> to move paddle | P to pause", screenWidth / 2 - 250, 450, 20, GRAY);
    } else if (gameState == GameState::LEADERBOARD) {
        DrawText("TOP 10 SCORES", screenWidth / 2 - 150, 50, 40, DARKBLUE);
        for (size_t i = 0; i < leaderboard.size() && i < 10; i++) {
            DrawText(TextFormat("#%d: %d pts (Level %d)", i + 1, leaderboard[i].score, leaderboard[i].level),
                100, 120 + i * 40, 24, DARKGRAY);
        }
        DrawText("Press L to return", screenWidth / 2 - 150, screenHeight - 50, 20, GRAY);
    } else if (gameState == GameState::NETWORK_WAITING) {
        DrawText("NETWORK WAITING", screenWidth / 2 - 190, 180, 48, DARKBLUE);
        DrawText("Connecting...", screenWidth / 2 - 120, 260, 28, DARKGRAY);
        DrawText("Press ESC to return", screenWidth / 2 - 140, 320, 20, GRAY);
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
        if (networkMode != NetworkMode::NONE) {
            DrawRectangle(remotePaddleX, remotePaddleY, remotePaddleWidth, paddleHeight, SKYBLUE);
            DrawRectangleLines(remotePaddleX, remotePaddleY, remotePaddleWidth, paddleHeight, DARKBLUE);
        }
        for (auto& brick : bricks) brick.Draw();
        DrawParticles();
        for (auto& powerUp : powerups) powerUp.Draw();

        DrawText(TextFormat("Score: %d", score), 12, 10, 20, DARKGRAY);
        DrawText(TextFormat("Level: %d", level), 12, 40, 20, DARKGRAY);
        DrawText(TextFormat("Lives: %d", lives), 12, 70, 20, lives <= 1 ? RED : DARKGRAY);
        if (combo > 0) {
            DrawText(TextFormat("Combo: %d x%.1f", combo, 1.0f + (combo / 5.0f)), 600, 10, 18, ORANGE);
        }
        DrawText(TextFormat("Speed: %.1f x", ballSpeedIncrease), 12, 100, 16, DARKGREEN);
        if (networkMode != NetworkMode::NONE) {
            DrawText(networkMode == NetworkMode::HOST ? "NET: HOST" : "NET: CLIENT", 650, 40, 18, PURPLE);
        }

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
    bool shouldDrop = !droppedPowerUpThisLevel;
    if (!shouldDrop) {
        shouldDrop = (rand() % 100 < powerUpDropChance);
    }

    if (shouldDrop) {
        float weights[3] = {
            std::max(0.0f, powerUpSettings.paddleExpandDropRate),
            std::max(0.0f, powerUpSettings.multiBallDropRate),
            std::max(0.0f, powerUpSettings.ballSlowDropRate)
        };
        float totalWeight = weights[0] + weights[1] + weights[2];

        if (totalWeight <= 0.0f) {
            return;
        }

        float randomValue = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * totalWeight;
        PowerUpType selectedType = PADDLE_EXPAND;

        if (randomValue < weights[0]) {
            selectedType = PADDLE_EXPAND;
        } else if (randomValue < weights[0] + weights[1]) {
            selectedType = MULTI_BALL;
        } else {
            selectedType = BALL_SLOW;
        }

        Vector2 pos = {brickPos.x + brickWidth * 0.5f, brickPos.y + brickHeight * 0.5f};
        powerups.emplace_back(pos, selectedType);
        droppedPowerUpThisLevel = true;
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
    ballSlowActive = false;
    droppedPowerUpThisLevel = false;
    levelReadyCountdown = 180;

    LevelData currentLevel = InitializeLevel(level);
    int randomXRange = std::max(1, screenWidth - 200);
    int randomX = 100 + rand() % randomXRange;
    int randomY = 120 + rand() % 180;
    ball = Ball({static_cast<float>(randomX), static_cast<float>(randomY)}, {0, 0}, ballRadius);
    paddle = Paddle((screenWidth - currentLevel.paddleWidth) * 0.5f, screenHeight - 50.0f, currentLevel.paddleWidth, paddleHeight);
    RebuildBricks(currentLevel);
    powerups.clear();
    particles.clear();
    remotePaddleWidth = currentLevel.paddleWidth;
    remotePaddleX = (screenWidth - remotePaddleWidth) * 0.5f;
    remotePaddleY = 80.0f;
    gameState = GameState::LEVEL_READY;
}

void Game::UpdateMenu() {
    if (IsKeyPressed(KEY_SPACE)) {
        StartNewRun();
    }

    if (IsKeyPressed(KEY_H)) {
        StartNetworkHost();
    }

    if (IsKeyPressed(KEY_C)) {
        StartNetworkClient("127.0.0.1");
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
    if (networkMode == NetworkMode::CLIENT) {
        UpdateNetworkClient();
        return;
    }

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

    UpdateParticles();

    if (paddleExpandTimer > 0) {
        paddleExpandTimer--;
        if (paddleExpandTimer == 0) {
            paddle.ResetWidth();
        }
    }

    if (ballSlowTimer > 0) {
        ballSlowTimer--;
        if (ballSlowTimer == 0 && ballSlowActive && powerUpSettings.ballSlowSpeedFactor > 0.0f) {
            Vector2 speed = ball.GetSpeed();
            speed.x /= powerUpSettings.ballSlowSpeedFactor;
            speed.y /= powerUpSettings.ballSlowSpeedFactor;
            ball.SetSpeed(speed);

            if (multiballActive) {
                Vector2 extraSpeed = extraBall.GetSpeed();
                extraSpeed.x /= powerUpSettings.ballSlowSpeedFactor;
                extraSpeed.y /= powerUpSettings.ballSlowSpeedFactor;
                extraBall.SetSpeed(extraSpeed);
            }

            ballSlowActive = false;
        }
    }
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
        const int maxPlayableLevel = 3;

        if (level < maxPlayableLevel) {
            level++;
            combo = 0;
            levelReadyCountdown = 180;

            paddleExpandTimer = 0;
            ballSlowTimer = 0;
            pierceTimer = 0;
            multiballActive = false;
            ballSlowActive = false;
            droppedPowerUpThisLevel = false;

            paddle.ResetWidth();
            powerups.clear();
            particles.clear();

            LevelData nextLevel = InitializeLevel(level);
            int randomXRange = std::max(1, screenWidth - 200);
            int randomX = 100 + rand() % randomXRange;
            int randomY = 120 + rand() % 180;
            ball = Ball({static_cast<float>(randomX), static_cast<float>(randomY)}, {0, 0}, ballRadius);
            paddle = Paddle((screenWidth - nextLevel.paddleWidth) * 0.5f, screenHeight - 50.0f, nextLevel.paddleWidth, paddleHeight);
            RebuildBricks(nextLevel);

            gameState = GameState::LEVEL_READY;
        } else {
            gameState = GameState::VICTORY;
            leaderboard.push_back({score, level});
            std::sort(leaderboard.rbegin(), leaderboard.rend(),
                [](const HighScore& a, const HighScore& b) { return a.score < b.score; });
            if (leaderboard.size() > 10) leaderboard.pop_back();
            SaveLeaderboard();
        }
    }

    if (networkMode == NetworkMode::HOST) {
        UpdateNetworkHost();
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

NetworkSnapshot Game::CaptureNetworkSnapshot() const {
    NetworkSnapshot snapshot;
    snapshot.gameState = static_cast<int>(gameState);
    snapshot.lives = lives;
    snapshot.score = score;
    snapshot.level = level;
    snapshot.combo = combo;
    snapshot.frameCounter = frameCounter;
    snapshot.ballSpeedIncrease = ballSpeedIncrease;
    snapshot.levelReadyCountdown = levelReadyCountdown;
    snapshot.multiballActive = multiballActive;
    snapshot.ballSlowActive = ballSlowActive;
    snapshot.droppedPowerUpThisLevel = droppedPowerUpThisLevel;

    Vector2 ballPos = ball.GetPosition();
    Vector2 ballSpeed = ball.GetSpeed();
    snapshot.ballX = ballPos.x;
    snapshot.ballY = ballPos.y;
    snapshot.ballSpeedX = ballSpeed.x;
    snapshot.ballSpeedY = ballSpeed.y;

    Vector2 extraBallPos = extraBall.GetPosition();
    Vector2 extraBallSpeed = extraBall.GetSpeed();
    snapshot.extraBallX = extraBallPos.x;
    snapshot.extraBallY = extraBallPos.y;
    snapshot.extraBallSpeedX = extraBallSpeed.x;
    snapshot.extraBallSpeedY = extraBallSpeed.y;

    Rectangle paddleRect = paddle.GetRect();
    snapshot.paddleX = paddleRect.x;
    snapshot.paddleY = paddleRect.y;
    snapshot.paddleWidth = paddleRect.width;

    snapshot.remotePaddleX = remotePaddleX;
    snapshot.remotePaddleY = remotePaddleY;
    snapshot.remotePaddleWidth = remotePaddleWidth;

    snapshot.brickActive.reserve(bricks.size());
    for (const auto& brick : bricks) {
        snapshot.brickActive.push_back(brick.IsActive() ? 1 : 0);
    }

    snapshot.powerups.reserve(powerups.size());
    for (const auto& powerUp : powerups) {
        Vector2 pos = powerUp.GetPosition();
        snapshot.powerups.push_back({static_cast<int>(powerUp.GetType()), pos.x, pos.y, powerUp.IsActive()});
    }

    return snapshot;
}

void Game::ApplyNetworkSnapshot(const NetworkSnapshot& snapshot) {
    gameState = static_cast<GameState>(snapshot.gameState);
    lives = snapshot.lives;
    score = snapshot.score;
    level = snapshot.level;
    combo = snapshot.combo;
    frameCounter = snapshot.frameCounter;
    ballSpeedIncrease = snapshot.ballSpeedIncrease;
    levelReadyCountdown = snapshot.levelReadyCountdown;
    multiballActive = snapshot.multiballActive;
    ballSlowActive = snapshot.ballSlowActive;
    droppedPowerUpThisLevel = snapshot.droppedPowerUpThisLevel;

    ball.SetPosition({snapshot.ballX, snapshot.ballY});
    ball.SetSpeed({snapshot.ballSpeedX, snapshot.ballSpeedY});
    extraBall.SetPosition({snapshot.extraBallX, snapshot.extraBallY});
    extraBall.SetSpeed({snapshot.extraBallSpeedX, snapshot.extraBallSpeedY});
    paddle = Paddle(snapshot.paddleX, snapshot.paddleY, snapshot.paddleWidth, paddleHeight);

    if (networkMode == NetworkMode::CLIENT) {
        remotePaddleY = snapshot.remotePaddleY;
        remotePaddleWidth = snapshot.remotePaddleWidth;
        remotePaddleX = std::clamp(remotePaddleX, 0.0f, screenWidth - remotePaddleWidth);
    } else {
        remotePaddleX = snapshot.remotePaddleX;
        remotePaddleY = snapshot.remotePaddleY;
        remotePaddleWidth = snapshot.remotePaddleWidth;
    }

    LevelData levelData = InitializeLevel(level);
    RebuildBricks(levelData);
    for (size_t i = 0; i < bricks.size() && i < snapshot.brickActive.size(); i++) {
        bricks[i].SetActive(snapshot.brickActive[i] != 0);
    }

    powerups.clear();
    for (const auto& savedPowerUp : snapshot.powerups) {
        PowerUp powerUp({savedPowerUp.x, savedPowerUp.y}, static_cast<PowerUpType>(savedPowerUp.type));
        powerUp.SetActive(savedPowerUp.active);
        powerups.push_back(powerUp);
    }
}

void Game::StartNetworkHost() {
    StopNetwork();
    networkSession = std::make_unique<NetworkSession>();
    if (networkSession && networkSession->StartHost(12345)) {
        networkMode = NetworkMode::HOST;
        interpolationActive = false;
        remotePaddleWidth = paddleStartWidth;
        remotePaddleX = (screenWidth - remotePaddleWidth) * 0.5f;
        remotePaddleY = 80.0f;
        StartNewRun();
    } else {
        networkSession.reset();
        networkMode = NetworkMode::NONE;
    }
}

bool Game::StartNetworkClient(const std::string& host) {
    StopNetwork();
    networkSession = std::make_unique<NetworkSession>();
    if (networkSession && networkSession->Connect(host, 12345)) {
        networkMode = NetworkMode::CLIENT;
        interpolationActive = false;
        remotePaddleWidth = paddleStartWidth;
        remotePaddleX = (screenWidth - remotePaddleWidth) * 0.5f;
        remotePaddleY = 80.0f;
        gameState = GameState::NETWORK_WAITING;
        return true;
    }

    networkSession.reset();
    networkMode = NetworkMode::NONE;
    return false;
}

void Game::StopNetwork() {
    if (networkSession) {
        networkSession->Shutdown();
        networkSession.reset();
    }

    networkMode = NetworkMode::NONE;
    interpolationActive = false;
    remotePaddleWidth = paddleStartWidth;
    remotePaddleX = (screenWidth - remotePaddleWidth) * 0.5f;
    remotePaddleY = 80.0f;
}

void Game::UpdateNetworkHost() {
    if (!networkSession || !networkSession->IsRunning()) {
        return;
    }

    NetworkEventBatch batch;
    networkSession->Poll(batch);

    if (batch.disconnected) {
        StopNetwork();
        gameState = GameState::MENU;
        return;
    }

    if (batch.hasRemoteInput) {
        remotePaddleX = std::clamp(batch.remotePaddleX, 0.0f, screenWidth - remotePaddleWidth);
    }

    remotePaddleWidth = paddle.GetRect().width;
    remotePaddleY = 80.0f;

    if (networkSession->SendSnapshot(CaptureNetworkSnapshot())) {
        // 主机每帧广播一次最新状态。
    }
}

void Game::UpdateNetworkClient() {
    if (IsKeyPressed(KEY_ESCAPE)) {
        StopNetwork();
        gameState = GameState::MENU;
        return;
    }

    if (!networkSession || !networkSession->IsRunning()) {
        return;
    }

    if (IsKeyDown(KEY_LEFT)) {
        remotePaddleX -= paddleMoveSpeed;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        remotePaddleX += paddleMoveSpeed;
    }
    remotePaddleX = std::clamp(remotePaddleX, 0.0f, screenWidth - remotePaddleWidth);
    networkSession->SendRemoteInput(remotePaddleX);

    NetworkEventBatch batch;
    networkSession->Poll(batch);

    if (batch.disconnected) {
        StopNetwork();
        gameState = GameState::MENU;
        return;
    }

    if (batch.hasSnapshot) {
        Vector2 previousBallPos = ball.GetPosition();
        Vector2 previousExtraBallPos = extraBall.GetPosition();
        Rectangle previousPaddleRect = paddle.GetRect();

        ApplyNetworkSnapshot(batch.snapshot);

        interpolationBallFrom = previousBallPos;
        interpolationBallTo = ball.GetPosition();
        interpolationExtraBallFrom = previousExtraBallPos;
        interpolationExtraBallTo = extraBall.GetPosition();
        interpolationPaddleFrom = previousPaddleRect;
        interpolationPaddleTo = paddle.GetRect();

        ball.SetPosition(interpolationBallFrom);
        extraBall.SetPosition(interpolationExtraBallFrom);
        paddle = Paddle(interpolationPaddleFrom.x, interpolationPaddleFrom.y, interpolationPaddleFrom.width, interpolationPaddleFrom.height);

        interpolationStartTime = GetTime();
        interpolationActive = true;

        if (gameState == GameState::NETWORK_WAITING) {
            gameState = GameState::PLAYING;
        }
        remotePaddleX = std::clamp(remotePaddleX, 0.0f, screenWidth - remotePaddleWidth);
    }

    if (interpolationActive) {
        const double elapsed = GetTime() - interpolationStartTime;
        float t = static_cast<float>(elapsed / interpolationDuration);
        t = std::clamp(t, 0.0f, 1.0f);

        Vector2 ballInterpolated = {
            interpolationBallFrom.x + (interpolationBallTo.x - interpolationBallFrom.x) * t,
            interpolationBallFrom.y + (interpolationBallTo.y - interpolationBallFrom.y) * t
        };
        ball.SetPosition(ballInterpolated);

        Vector2 extraBallInterpolated = {
            interpolationExtraBallFrom.x + (interpolationExtraBallTo.x - interpolationExtraBallFrom.x) * t,
            interpolationExtraBallFrom.y + (interpolationExtraBallTo.y - interpolationExtraBallFrom.y) * t
        };
        extraBall.SetPosition(extraBallInterpolated);

        Rectangle paddleInterpolated = {
            interpolationPaddleFrom.x + (interpolationPaddleTo.x - interpolationPaddleFrom.x) * t,
            interpolationPaddleFrom.y + (interpolationPaddleTo.y - interpolationPaddleFrom.y) * t,
            interpolationPaddleFrom.width + (interpolationPaddleTo.width - interpolationPaddleFrom.width) * t,
            interpolationPaddleFrom.height + (interpolationPaddleTo.height - interpolationPaddleFrom.height) * t
        };
        paddle = Paddle(paddleInterpolated.x, paddleInterpolated.y, paddleInterpolated.width, paddleInterpolated.height);

        if (t >= 1.0f) {
            interpolationActive = false;
        }
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
                    SpawnBrickParticles(brick.GetRect(), GetBrickColor(brick.GetType()));
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
                    SpawnBrickParticles(brick.GetRect(), GetBrickColor(brick.GetType()));
                    score += brick.GetPoints();
                    combo++;
                    TryDropPowerUp({brick.GetRect().x, brick.GetRect().y});
                }
            }
        }
    }

    if (!hitBrick) combo = 0;
}

void Game::SpawnBrickParticles(const Rectangle& brickRect, Color brickColor) {
    const int particleCount = 10;
    for (int i = 0; i < particleCount; i++) {
        Particle particle;

        float randomX = static_cast<float>(rand() % static_cast<int>(std::max(1.0f, brickRect.width)));
        float randomY = static_cast<float>(rand() % static_cast<int>(std::max(1.0f, brickRect.height)));
        particle.position = {brickRect.x + randomX, brickRect.y + randomY};

        float vx = (static_cast<float>(rand() % 100) - 50.0f) / 14.0f;
        float vy = (static_cast<float>(rand() % 100) - 50.0f) / 16.0f;
        particle.velocity = {vx, vy};

        particle.color = brickColor;
        particle.life = 30.0f + static_cast<float>(rand() % 12);
        particle.maxLife = particle.life;
        particle.size = 2.0f + static_cast<float>(rand() % 3);
        particle.active = true;
        particles.push_back(particle);
    }
}

void Game::UpdateParticles() {
    for (auto& particle : particles) {
        if (!particle.active) {
            continue;
        }

        particle.position.x += particle.velocity.x;
        particle.position.y += particle.velocity.y;
        particle.velocity.y += 0.08f;
        particle.life -= 1.0f;
        if (particle.life <= 0.0f) {
            particle.active = false;
        }
    }

    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& particle) { return !particle.active; }), particles.end());
}

void Game::DrawParticles() {
    for (const auto& particle : particles) {
        float alphaRatio = 0.0f;
        if (particle.maxLife > 0.0f) {
            alphaRatio = particle.life / particle.maxLife;
        }
        alphaRatio = std::max(0.0f, std::min(1.0f, alphaRatio));

        Color fadedColor = particle.color;
        fadedColor.a = static_cast<unsigned char>(255.0f * alphaRatio);
        DrawCircleV(particle.position, particle.size, fadedColor);
    }
}

Color Game::GetBrickColor(int brickType) const {
    if (brickType == 1) {
        return GREEN;
    }
    if (brickType == 2) {
        return BLUE;
    }
    return GOLD;
}

bool Game::CheckBottomCollision(const Ball& targetBall) const {
    return targetBall.GetPosition().y + targetBall.GetRadius() >= screenHeight;
}

void Game::ApplyPaddleExpandEffect(float extraWidth, int durationFrames, int scoreBonus) {
    if (paddleExpandTimer == 0) {
        paddle.SetWidth(paddle.GetWidth() + extraWidth);
    }
    paddleExpandTimer = durationFrames;
    score += scoreBonus;
}

void Game::ApplyBallSlowEffect(float speedFactor, int durationFrames, int scoreBonus) {
    if (!ballSlowActive && speedFactor > 0.0f) {
        Vector2 speed = ball.GetSpeed();
        speed.x *= speedFactor;
        speed.y *= speedFactor;
        ball.SetSpeed(speed);

        if (multiballActive) {
            Vector2 extraSpeed = extraBall.GetSpeed();
            extraSpeed.x *= speedFactor;
            extraSpeed.y *= speedFactor;
            extraBall.SetSpeed(extraSpeed);
        }

        ballSlowActive = true;
    }
    ballSlowTimer = durationFrames;
    score += scoreBonus;
}

void Game::ApplyBallPierceEffect(int durationFrames, int scoreBonus) {
    pierceTimer = durationFrames;
    score += scoreBonus;
}

void Game::ApplyMultiBallEffect(int scoreBonus) {
    if (!multiballActive && powerUpSettings.multiBallExtraBalls > 0) {
        multiballActive = true;
        extraBall = ball;
        extraBall.SetSpeed({-ball.GetSpeed().x, ball.GetSpeed().y});
    }
    score += scoreBonus;
}

void Game::ApplySlowFieldEffect(float factor, int scoreBonus) {
    ballSpeedIncrease *= factor;
    score += scoreBonus;
}

void Game::HandlePowerUpCatch(PowerUp& powerUp) {
    std::unique_ptr<PowerUpEffect> effect = CreatePowerUpEffect(powerUp.GetType());
    if (effect) {
        effect->Apply(*this);
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
