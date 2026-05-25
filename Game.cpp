#include "Game.h"
#include "PowerUpEffect.h"
#include <nlohmann/json.hpp>
#include "raylib.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <thread>

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
                asyncLoading(false),
                asyncLoadCompleted(false),
                asyncColorApplied(false),
                asyncLoadedBrickTint({255, 120, 80, 120}),
                interpolationActive(false),
                interpolationStartTime(0.0),
                interpolationDuration(0.05),
                lastSnapshotSendTime(0.0),
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
    
    // Load levels from JSON
    LoadLevelsFromJSON("levels.json");
    
    // Initialize particle pool
    particles.clear();
    particles.resize(MAX_PARTICLES);
    for (auto& p : particles) {
        p.active = false;
    }
    particlePoolIndex = 0;
    
    // Initialize performance logging
    InitPerformanceLogging();
}

void Game::Update() {
    frameCounter++;
    PollAsyncLoadTask();

    if (networkMode == NetworkMode::HOST) {
        UpdateNetworkHost();
    } else if (networkMode == NetworkMode::CLIENT) {
        UpdateNetworkClient();
    }

    if (IsKeyPressed(KEY_TAB)) {
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
            break;
    }
    
    // Log performance metrics every 60 frames
    LogPerformanceMetrics();
}

void Game::Draw() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    bool showLoading = false;
    bool showLoadedTint = false;
    Color loadedTint = asyncLoadedBrickTint;
    {
        std::lock_guard<std::mutex> lock(asyncLoadMutex);
        showLoading = asyncLoading;
        showLoadedTint = asyncColorApplied;
        loadedTint = asyncLoadedBrickTint;
    }

    DrawRectangle(0, 0, 5, screenHeight, GRAY);
    DrawRectangle(screenWidth - 5, 0, 5, screenHeight, GRAY);
    DrawRectangle(0, 0, screenWidth, 5, GRAY);
    DrawRectangle(0, screenHeight - 5, screenWidth, 5, GRAY);

    if (gameState == GameState::MENU) {
        DrawText("BREAKOUT 2D", screenWidth / 2 - 150, 80, 60, DARKBLUE);
        DrawText("Press SPACE to Start", screenWidth / 2 - 180, 250, 32, DARKGRAY);
        DrawText("Press TAB to View Leaderboard", screenWidth / 2 - 225, 320, 24, DARKGRAY);
        DrawText("Press H to Host | Press C to Connect localhost", screenWidth / 2 - 250, 400, 20, DARKGRAY);
        DrawText("Controls: <- -> move | P pause | L async load", screenWidth / 2 - 255, 450, 20, GRAY);
        
        // Show load game option if save exists
        if (CheckForSavedGame()) {
            DrawText("Press G to Load Saved Game", screenWidth / 2 - 200, 520, 20, GREEN);
        }
    } else if (gameState == GameState::LEADERBOARD) {
        DrawText("TOP 10 SCORES", screenWidth / 2 - 150, 50, 40, DARKBLUE);
        for (size_t i = 0; i < leaderboard.size() && i < 10; i++) {
            DrawText(TextFormat("#%d: %d pts (Level %d)", i + 1, leaderboard[i].score, leaderboard[i].level),
                100, 120 + i * 40, 24, DARKGRAY);
        }
        DrawText("Press TAB to return", screenWidth / 2 - 170, screenHeight - 50, 20, GRAY);
    } else if (gameState == GameState::NETWORK_WAITING) {
        DrawText("NETWORK WAITING", screenWidth / 2 - 190, 180, 48, DARKBLUE);
        DrawText("Connecting...", screenWidth / 2 - 120, 260, 28, DARKGRAY);
        DrawText("Press ESC to return", screenWidth / 2 - 140, 320, 20, GRAY);
    } else if (gameState == GameState::LEVEL_READY) {
        ball.Draw();
        paddle.Draw();
        for (auto& brick : bricks) brick.Draw();
        if (showLoadedTint) {
            for (const auto& brick : bricks) {
                if (brick.IsActive()) {
                    DrawRectangleRec(brick.GetRect(), loadedTint);
                }
            }
        }

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
        if (showLoadedTint) {
            for (const auto& brick : bricks) {
                if (brick.IsActive()) {
                    DrawRectangleRec(brick.GetRect(), loadedTint);
                }
            }
        }
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

    if (showLoading && (gameState == GameState::PLAYING || gameState == GameState::LEVEL_READY)) {
        static const char* loadingFrames[] = {"Loading", "Loading.", "Loading..", "Loading..."};
        int frameIndex = (frameCounter / 15) % 4;
        DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 100});
        DrawText(loadingFrames[frameIndex], screenWidth / 2 - 90, screenHeight / 2 - 20, 42, WHITE);
    }

    if (gameState == GameState::PLAYING || gameState == GameState::LEVEL_READY) {
        int fps = GetFPS();
        int activeParticles = 0;
        for (const auto& p : particles) {
            if (p.active) activeParticles++;
        }

        const int metricsX = 10;
        const int metricsY = 128;
        DrawRectangle(metricsX - 4, metricsY - 4, 180, 52, Color{255, 255, 255, 200});
        DrawText(TextFormat("FPS: %d", fps), metricsX, metricsY, 20, BLACK);
        DrawText(TextFormat("Particles: %d/%d", activeParticles, MAX_PARTICLES), metricsX, metricsY + 22, 18, DARKGRAY);
    }

    EndDrawing();
}

void Game::Shutdown() {
    if (asyncLoadFuture.valid()) {
        asyncLoadFuture.wait();
        asyncLoadFuture.get();
    }
    SaveLeaderboard();
    CloseWindow();
}

bool Game::LoadLevelsFromJSON(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    try {
        json config = json::parse(file);
        
        if (!config.contains("levels") || !config["levels"].is_array()) {
            return false;
        }

        allLevels.clear();
        for (const auto& levelJson : config["levels"]) {
            LevelData level;
            level.level = levelJson.value("level", 1);
            level.ballSpeedMultiplier = levelJson.value("ballSpeedMultiplier", 1.0f);
            level.paddleWidth = levelJson.value("paddleWidth", paddleStartWidth);
            
            if (levelJson.contains("brickPattern") && levelJson["brickPattern"].is_array()) {
                level.brickPattern.clear();
                for (int brickType : levelJson["brickPattern"]) {
                    level.brickPattern.push_back(brickType);
                }
            }
            
            allLevels.push_back(level);
        }
        
        return !allLevels.empty();
    } catch (const std::exception& e) {
        return false;
    }
}

bool Game::CheckForSavedGame() {
    std::ifstream file("savegame.json");
    return file.good();
}

void Game::PromptLoadGame() {
    if (!CheckForSavedGame()) {
        return;
    }
    
    // 在菜单状态显示读档选项 - 稍后会在 Draw() 中处理
}

Game::LevelData Game::InitializeLevel(int targetLevel) const {
    // 先尝试从加载的关卡中找
    for (const auto& level : allLevels) {
        if (level.level == targetLevel) {
            return level;
        }
    }
    
    // 如果没有加载关卡，使用默认硬编码
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
    for (auto& p : particles) {
        p.active = false;
    }
    particlePoolIndex = 0;
    {
        std::lock_guard<std::mutex> lock(asyncLoadMutex);
        asyncLoading = false;
        asyncLoadCompleted = false;
        asyncColorApplied = false;
    }
    remotePaddleWidth = currentLevel.paddleWidth;
    remotePaddleX = (screenWidth - remotePaddleWidth) * 0.5f;
    remotePaddleY = screenHeight - 50.0f;
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
    
    // Load saved game with G key
    if (IsKeyPressed(KEY_G)) {
        if (LoadGameState("savegame.json")) {
            // 恢复游戏状态，重新构建砖块
            LevelData currentLevel = InitializeLevel(level);
            RebuildBricks(currentLevel);
            gameState = GameState::LEVEL_READY;
            levelReadyCountdown = 180;
        }
    }
}

void Game::UpdateLeaderboard() {
}

void Game::StartAsyncLoadTask() {
    {
        std::lock_guard<std::mutex> lock(asyncLoadMutex);
        if (asyncLoading) {
            return;
        }
        asyncLoading = true;
        asyncLoadCompleted = false;
    }

    if (asyncLoadFuture.valid()) {
        asyncLoadFuture.wait();
        asyncLoadFuture.get();
    }

    asyncLoadFuture = std::async(std::launch::async, [this]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        std::lock_guard<std::mutex> lock(asyncLoadMutex);
        asyncLoadedBrickTint = {255, 120, 80, 120};
        asyncLoading = false;
        asyncLoadCompleted = true;
    });
}

void Game::PollAsyncLoadTask() {
    if (asyncLoadFuture.valid() &&
        asyncLoadFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        asyncLoadFuture.get();
    }

    std::lock_guard<std::mutex> lock(asyncLoadMutex);
    if (asyncLoadCompleted) {
        asyncColorApplied = true;
        asyncLoadCompleted = false;
    }
}

bool Game::IsAsyncLoading() const {
    std::lock_guard<std::mutex> lock(asyncLoadMutex);
    return asyncLoading;
}

void Game::UpdateLevelReady() {
    if (networkMode != NetworkMode::CLIENT) {
        levelReadyCountdown--;
    }

    if (levelReadyCountdown <= 0) {
        if (networkMode != NetworkMode::CLIENT) {
            LevelData currentLevel = InitializeLevel(level);
            float levelBallSpeed = baseBallSpeed * currentLevel.ballSpeedMultiplier * ballSpeedIncrease;
            ball.SetSpeed({levelBallSpeed, levelBallSpeed});
            gameState = GameState::PLAYING;
            frameCounter = 0;
        }
    }
}

void Game::UpdatePlaying() {
    if (networkMode == NetworkMode::CLIENT) {
        return;
    }

    if (frameCounter > 0 && frameCounter % 600 == 0) {
        ballSpeedIncrease += 0.1f;
    }

    if (IsKeyPressed(KEY_F5)) {
        SaveGameState("savegame.json");
    }

    if (IsKeyPressed(KEY_F9)) {
        LoadGameState("savegame.json");
    }

    if (IsKeyPressed(KEY_L)) {
        StartAsyncLoadTask();
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

    if (networkMode == NetworkMode::HOST) {
        Rectangle remotePaddleRect = {remotePaddleX, remotePaddleY, remotePaddleWidth, paddleHeight};
        CheckPaddleCollisionWithRect(ball, remotePaddleRect, false);
        if (multiballActive) {
            CheckPaddleCollisionWithRect(extraBall, remotePaddleRect, false);
        }
    }

    CheckBrickCollision(ball);
    if (multiballActive) CheckBrickCollision(extraBall);

    for (auto& powerUp : powerups) {
        bool localCaught = CheckCollisionCircleRec(powerUp.GetPosition(), powerUp.GetRadius(), paddle.GetRect());
        bool remoteCaught = false;
        if (networkMode == NetworkMode::HOST) {
            Rectangle remotePaddleRect = {remotePaddleX, remotePaddleY, remotePaddleWidth, paddleHeight};
            remoteCaught = CheckCollisionCircleRec(powerUp.GetPosition(), powerUp.GetRadius(), remotePaddleRect);
        }

        if (localCaught || remoteCaught) {
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
                // Auto-save at game over
                SaveGameState("savegame.json");
            } else {
                int randomXRange = std::max(1, screenWidth - 200);
                int randomX = 100 + rand() % randomXRange;
                int randomY = 120 + rand() % 180;
                ball = Ball({static_cast<float>(randomX), static_cast<float>(randomY)}, {0, 0}, ballRadius);
                combo = 0;
                levelReadyCountdown = 180;
                gameState = GameState::LEVEL_READY;
                // Auto-save after losing a life
                SaveGameState("savegame.json");
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
            for (auto& p : particles) {
                p.active = false;
            }
            particlePoolIndex = 0;

            LevelData nextLevel = InitializeLevel(level);
            int randomXRange = std::max(1, screenWidth - 200);
            int randomX = 100 + rand() % randomXRange;
            int randomY = 120 + rand() % 180;
            ball = Ball({static_cast<float>(randomX), static_cast<float>(randomY)}, {0, 0}, ballRadius);
            paddle = Paddle((screenWidth - nextLevel.paddleWidth) * 0.5f, screenHeight - 50.0f, nextLevel.paddleWidth, paddleHeight);
            RebuildBricks(nextLevel);

            gameState = GameState::LEVEL_READY;
            // Auto-save after level complete
            SaveGameState("savegame.json");
        } else {
            gameState = GameState::VICTORY;
            leaderboard.push_back({score, level});
            std::sort(leaderboard.rbegin(), leaderboard.rend(),
                [](const HighScore& a, const HighScore& b) { return a.score < b.score; });
            if (leaderboard.size() > 10) leaderboard.pop_back();
            SaveLeaderboard();
            // Auto-save at victory
            SaveGameState("savegame.json");
        }
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
        lastSnapshotSendTime = 0.0;
        remotePaddleWidth = paddleStartWidth;
        remotePaddleX = (screenWidth - remotePaddleWidth) * 0.5f;
        remotePaddleY = screenHeight - 50.0f;
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
        lastSnapshotSendTime = 0.0;
        remotePaddleWidth = paddleStartWidth;
        remotePaddleX = (screenWidth - remotePaddleWidth) * 0.5f;
        remotePaddleY = screenHeight - 50.0f;
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
    lastSnapshotSendTime = 0.0;
    remotePaddleWidth = paddleStartWidth;
    remotePaddleX = (screenWidth - remotePaddleWidth) * 0.5f;
    remotePaddleY = screenHeight - 50.0f;
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
    remotePaddleY = screenHeight - 50.0f;

    const double now = GetTime();
    if (lastSnapshotSendTime == 0.0 || now - lastSnapshotSendTime >= (1.0 / 30.0)) {
        lastSnapshotSendTime = now;
        if (networkSession->SendSnapshot(CaptureNetworkSnapshot())) {
            // 主机按 30Hz 广播最新状态，避免网络队列拥塞。
        }
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

// 处理球与本方挡板的碰撞：统一走带矩形参数的版本，便于主机和远程挡板复用。
void Game::CheckPaddleCollision(Ball& targetBall) {
    CheckPaddleCollisionWithRect(targetBall, paddle.GetRect(), false);
}

// 根据挡板位置和球的运动方向判断反弹方向，避免球穿透挡板。
void Game::CheckPaddleCollisionWithRect(Ball& targetBall, const Rectangle& paddleRect, bool topPaddle) {
    Vector2 ballPos = targetBall.GetPosition();
    Vector2 ballSpeed = targetBall.GetSpeed();
    float ballRadius = targetBall.GetRadius();

    if (topPaddle) {
        if (ballSpeed.y < 0 && CheckCollisionCircleRec(ballPos, ballRadius, paddleRect)) {
            targetBall.ReverseY();
            ballPos.y = paddleRect.y + paddleRect.height + ballRadius + 1.0f;
            targetBall.SetPosition(ballPos);
        }
    } else {
        if (ballSpeed.y > 0 && CheckCollisionCircleRec(ballPos, ballRadius, paddleRect)) {
            targetBall.ReverseY();
            ballPos.y = paddleRect.y - ballRadius - 1.0f;
            targetBall.SetPosition(ballPos);
        }
    }
}

// 逐个检查砖块碰撞：
// 1) 普通模式下只命中第一个有效砖块并反弹；
// 2) 穿透模式下可以连续击穿多个砖块，但仍只计算一次球的方向反转。
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

    // 没有击中任何砖块时，连击断开并清零。
    if (!hitBrick) combo = 0;
}

void Game::SpawnBrickParticles(const Rectangle& brickRect, Color brickColor) {
    const int particleCount = 10;
    for (int i = 0; i < particleCount; i++) {
        // 粒子池采用环形复用，防止每次击碎砖块都重新分配内存。
        if (particlePoolIndex >= MAX_PARTICLES) {
            particlePoolIndex = 0;
        }
        
        Particle& particle = particles[particlePoolIndex];
        
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
        
        particlePoolIndex++;
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

// 判断球是否越过底边；具体扣命、重开或结算由外层状态机处理。
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

Game::GameStateData Game::CaptureGameState() const {
    GameStateData data;
    data.gameState = static_cast<int>(gameState);
    data.lives = lives;
    data.score = score;
    data.level = level;
    data.combo = combo;
    data.frameCounter = frameCounter;
    data.ballSpeedIncrease = ballSpeedIncrease;
    data.levelReadyCountdown = levelReadyCountdown;
    data.multiballActive = multiballActive;
    data.ballSlowActive = ballSlowActive;
    data.droppedPowerUpThisLevel = droppedPowerUpThisLevel;

    Vector2 ballPos = ball.GetPosition();
    Vector2 ballSpeed = ball.GetSpeed();
    data.ballX = ballPos.x;
    data.ballY = ballPos.y;
    data.ballSpeedX = ballSpeed.x;
    data.ballSpeedY = ballSpeed.y;

    Vector2 extraBallPos = extraBall.GetPosition();
    Vector2 extraBallSpeed = extraBall.GetSpeed();
    data.extraBallX = extraBallPos.x;
    data.extraBallY = extraBallPos.y;
    data.extraBallSpeedX = extraBallSpeed.x;
    data.extraBallSpeedY = extraBallSpeed.y;

    Rectangle paddleRect = paddle.GetRect();
    data.paddleX = paddleRect.x;
    data.paddleY = paddleRect.y;
    data.paddleWidth = paddleRect.width;

    data.brickActive.reserve(bricks.size());
    for (const auto& brick : bricks) {
        data.brickActive.push_back(brick.IsActive() ? 1 : 0);
    }

    data.powerups.reserve(powerups.size());
    for (const auto& powerUp : powerups) {
        Vector2 pos = powerUp.GetPosition();
        data.powerups.push_back({static_cast<int>(powerUp.GetType()), pos.x, pos.y, powerUp.IsActive()});
    }

    return data;
}

void Game::ApplyGameState(const GameStateData& data) {
    gameState = static_cast<GameState>(data.gameState);
    lives = data.lives;
    score = data.score;
    level = data.level;
    combo = data.combo;
    frameCounter = data.frameCounter;
    ballSpeedIncrease = data.ballSpeedIncrease;
    levelReadyCountdown = data.levelReadyCountdown;
    multiballActive = data.multiballActive;
    ballSlowActive = data.ballSlowActive;
    droppedPowerUpThisLevel = data.droppedPowerUpThisLevel;

    ball.SetPosition({data.ballX, data.ballY});
    ball.SetSpeed({data.ballSpeedX, data.ballSpeedY});
    extraBall.SetPosition({data.extraBallX, data.extraBallY});
    extraBall.SetSpeed({data.extraBallSpeedX, data.extraBallSpeedY});
    paddle = Paddle(data.paddleX, data.paddleY, data.paddleWidth, paddleHeight);

    LevelData levelData = InitializeLevel(level);
    RebuildBricks(levelData);
    for (size_t i = 0; i < bricks.size() && i < data.brickActive.size(); i++) {
        bricks[i].SetActive(data.brickActive[i] != 0);
    }

    powerups.clear();
    for (const auto& savedPowerUp : data.powerups) {
        PowerUp powerUp({savedPowerUp.x, savedPowerUp.y}, static_cast<PowerUpType>(savedPowerUp.type));
        powerUp.SetActive(savedPowerUp.active);
        powerups.push_back(powerUp);
    }
}

void Game::SaveGameState(const std::string& path) const {
    GameStateData data = CaptureGameState();

    json saved;
    saved["gameState"] = data.gameState;
    saved["lives"] = data.lives;
    saved["score"] = data.score;
    saved["level"] = data.level;
    saved["combo"] = data.combo;
    saved["frameCounter"] = data.frameCounter;
    saved["ballSpeedIncrease"] = data.ballSpeedIncrease;
    saved["levelReadyCountdown"] = data.levelReadyCountdown;
    saved["multiballActive"] = data.multiballActive;
    saved["ballSlowActive"] = data.ballSlowActive;
    saved["droppedPowerUpThisLevel"] = data.droppedPowerUpThisLevel;
    saved["ball"] = {data.ballX, data.ballY, data.ballSpeedX, data.ballSpeedY};
    saved["extraBall"] = {data.extraBallX, data.extraBallY, data.extraBallSpeedX, data.extraBallSpeedY};
    saved["paddle"] = {data.paddleX, data.paddleY, data.paddleWidth};
    saved["brickActive"] = data.brickActive;

    json powerupArray = json::array();
    for (const auto& powerUp : data.powerups) {
        powerupArray.push_back({
            {"type", powerUp.type},
            {"x", powerUp.x},
            {"y", powerUp.y},
            {"active", powerUp.active}
        });
    }
    saved["powerups"] = powerupArray;

    std::ofstream file(path);
    if (file.is_open()) {
        file << saved.dump(2);
    }
}

bool Game::LoadGameState(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    try {
        json saved = json::parse(file);
        GameStateData data;

        data.gameState = saved.value("gameState", static_cast<int>(GameState::MENU));
        data.lives = saved.value("lives", lives);
        data.score = saved.value("score", score);
        data.level = saved.value("level", level);
        data.combo = saved.value("combo", combo);
        data.frameCounter = saved.value("frameCounter", frameCounter);
        data.ballSpeedIncrease = saved.value("ballSpeedIncrease", ballSpeedIncrease);
        data.levelReadyCountdown = saved.value("levelReadyCountdown", levelReadyCountdown);
        data.multiballActive = saved.value("multiballActive", multiballActive);
        data.ballSlowActive = saved.value("ballSlowActive", ballSlowActive);
        data.droppedPowerUpThisLevel = saved.value("droppedPowerUpThisLevel", droppedPowerUpThisLevel);

        auto ballArray = saved.value("ball", std::vector<float>{400.0f, 300.0f, 2.0f, 2.0f});
        if (ballArray.size() >= 4) {
            data.ballX = ballArray[0];
            data.ballY = ballArray[1];
            data.ballSpeedX = ballArray[2];
            data.ballSpeedY = ballArray[3];
        }

        auto extraBallArray = saved.value("extraBall", std::vector<float>{-1000.0f, -1000.0f, 0.0f, 0.0f});
        if (extraBallArray.size() >= 4) {
            data.extraBallX = extraBallArray[0];
            data.extraBallY = extraBallArray[1];
            data.extraBallSpeedX = extraBallArray[2];
            data.extraBallSpeedY = extraBallArray[3];
        }

        auto paddleArray = saved.value("paddle", std::vector<float>{300.0f, 550.0f, paddleStartWidth});
        if (paddleArray.size() >= 3) {
            data.paddleX = paddleArray[0];
            data.paddleY = paddleArray[1];
            data.paddleWidth = paddleArray[2];
        }

        data.brickActive = saved.value("brickActive", std::vector<int>{});

        if (saved.contains("powerups") && saved["powerups"].is_array()) {
            for (const auto& item : saved["powerups"]) {
                SerializedPowerUp powerUpData;
                powerUpData.type = item.value("type", static_cast<int>(PADDLE_EXPAND));
                powerUpData.x = item.value("x", 0.0f);
                powerUpData.y = item.value("y", 0.0f);
                powerUpData.active = item.value("active", true);
                data.powerups.push_back(powerUpData);
            }
        }

        ApplyGameState(data);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void Game::InitPerformanceLogging() {
    std::string logFile = "performance_log_" + std::to_string(time(NULL)) + ".csv";
    performanceLog.open(logFile, std::ios::out);
    if (performanceLog.is_open()) {
        performanceLog << "Frame,FPS,ActiveParticles,GameState\n";
        performanceLog.flush();
    }
    framesSinceLastLog = 0;
    totalFpsForLog = 0.0;
}

void Game::LogPerformanceMetrics() {
    framesSinceLastLog++;
    totalFpsForLog += GetFPS();
    
    // Log every 60 frames (about 1 second at 60 FPS)
    if (framesSinceLastLog >= 60 && performanceLog.is_open()) {
        int avgFps = static_cast<int>(totalFpsForLog / framesSinceLastLog);
        int activeParticles = 0;
        for (const auto& p : particles) {
            if (p.active) activeParticles++;
        }
        
        performanceLog << frameCounter << "," << avgFps << "," << activeParticles << "," << static_cast<int>(gameState) << "\n";
        performanceLog.flush();
        
        framesSinceLastLog = 0;
        totalFpsForLog = 0.0;
    }
}
