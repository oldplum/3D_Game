#include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include "PowerUp.h"
#include <vector>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <fstream>
#include <algorithm>

// 游戏状态
enum GameState { MENU, PLAYING, GAME_OVER, PAUSED, LEADERBOARD, LEVEL_READY };

// 关卡数据结构
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

// 初始化关卡
LevelData InitializeLevel(int level) {
    LevelData data;
    data.level = level;
    
    // 更陡峭的速度增长
    if (level == 1) data.ballSpeedMultiplier = 1.0f;
    else if (level == 2) data.ballSpeedMultiplier = 1.5f;
    else if (level == 3) data.ballSpeedMultiplier = 2.0f;
    else data.ballSpeedMultiplier = 2.7f + (level - 4) * 0.5f;
    
    // 挡板逐关缩小
    if (level == 1) data.paddleWidth = 150;
    else if (level == 2) data.paddleWidth = 120;
    else if (level == 3) data.paddleWidth = 90;
    else data.paddleWidth = 60;
    
    // 砖块配置
    data.brickPattern.clear();
    if (level == 1) {
        for (int i = 0; i < 4; i++) data.brickPattern.push_back(1);
    } else if (level == 2) {
        for (int i = 0; i < 4; i++) data.brickPattern.push_back(1);
        for (int i = 0; i < 2; i++) data.brickPattern.push_back(2);
    } else if (level == 3) {
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

// 重建砖块
void RebuildBricks(std::vector<Brick>& bricks, const LevelData& levelData) {
    bricks.clear();
    float brickWidth = 80;
    float brickHeight = 30;
    int colsPerRow = 4;
    float brickSpacingX = 90;
    float startX = 60;
    float startY = 80;
    float rowSpacing = 50;
    
    for (size_t i = 0; i < levelData.brickPattern.size(); i++) {
        int type = levelData.brickPattern[i];
        int row = i / colsPerRow;
        int col = i % colsPerRow;
        bricks.emplace_back(startX + col * brickSpacingX, startY + row * rowSpacing, brickWidth, brickHeight, type);
    }
}

// 道具掉落逻辑
void TryDropPowerUp(std::vector<PowerUp>& powerups, Vector2 brickPos) {
    if (rand() % 100 < 45) {  // 45% 概率
        PowerUpType types[] = {PADDLE_EXPAND, BALL_SLOW, BALL_PIERCE, MULTI_BALL, SLOW_FIELD};
        Vector2 pos = {brickPos.x + 40, brickPos.y + 15};
        powerups.emplace_back(pos, types[rand() % 5]);
    }
}

// 加载排行榜
std::vector<HighScore> LoadLeaderboard() {
    std::vector<HighScore> scores;
    std::ifstream file("leaderboard.txt");
    if (file.is_open()) {
        int score, level;
        while (file >> score >> level) {
            scores.push_back({score, level});
        }
        file.close();
    }
    return scores;
}

// 保存排行榜
void SaveLeaderboard(const std::vector<HighScore>& scores) {
    std::ofstream file("leaderboard.txt");
    for (const auto& s : scores) {
        file << s.score << " " << s.level << "\n";
    }
    file.close();
}

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "打砖块2D - 终极升级版");
    SetTargetFPS(60);
    srand(static_cast<unsigned>(time(NULL)));  // 初始化随机数种子
    
    GameState gameState = MENU;
    int lives = 3;
    int score = 0;
    int level = 1;
    int combo = 0;
    int frameCounter = 0;  // 时间加速用
    float ballSpeedIncrease = 1.0f;  // 球速增加倍数
    int levelReadyCountdown = 0;  // 关卡准备倒计时（帧数）
    
    Ball ball({400, 300}, {2, 2}, 10);
    Paddle paddle(300, 550, 150, 20);
    std::vector<Brick> bricks;
    std::vector<PowerUp> powerups;
    std::vector<HighScore> leaderboard = LoadLeaderboard();
    
    // 菜单变量
    int selectedMenu = 0;
    
    // 道具效果计时器
    int paddleExpandTimer = 0;
    int ballSlowTimer = 0;
    int pierceTimer = 0;
    int multiballActive = false;
    Ball extraBall({-1000, -1000}, {0, 0}, 10);
    
    while (!WindowShouldClose()) {
        frameCounter++;
        
        // ============ 菜单界面 ============
        if (gameState == MENU) {
            if (IsKeyPressed(KEY_SPACE)) {
                lives = 3;
                score = 0;
                level = 1;
                combo = 0;
                ballSpeedIncrease = 1.0f;
                frameCounter = 0;
                paddleExpandTimer = 0;
                ballSlowTimer = 0;
                pierceTimer = 0;
                multiballActive = false;
                levelReadyCountdown = 180;  // 3秒倒计时（60fps × 3）
                
                LevelData currentLevel = InitializeLevel(level);
                // 球位置随机在屏幕上方：X在100-700，Y在150-300
                int randomX = 100 + rand() % 600;
                int randomY = 150 + rand() % 150;
                ball = Ball({(float)randomX, (float)randomY}, {0, 0}, 10);  // 球速为0，倒计时结束后获得速度
                paddle = Paddle(300, 550, currentLevel.paddleWidth, 20);
                RebuildBricks(bricks, currentLevel);
                powerups.clear();
                gameState = LEVEL_READY;  // 进入准备阶段
            }
            if (IsKeyPressed(KEY_L)) {
                gameState = LEADERBOARD;
            }
        }
        
        // ============ 排行榜界面 ============
        if (gameState == LEADERBOARD) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                gameState = MENU;
            }
        }
        
        // ============ 关卡准备阶段 ============
        if (gameState == LEVEL_READY) {
            levelReadyCountdown--;
            
            if (levelReadyCountdown <= 0) {
                // 倒计时结束，进入游戏
                LevelData currentLevel = InitializeLevel(level);
                ball.SetSpeed({2 * currentLevel.ballSpeedMultiplier * ballSpeedIncrease, 
                               2 * currentLevel.ballSpeedMultiplier * ballSpeedIncrease});
                gameState = PLAYING;
                frameCounter = 0;
            }
        }
        
        // ============ 游戏进行中 ============
        if (gameState == PLAYING) {
            LevelData currentLevel = InitializeLevel(level);
            
            // 时间加速：每 10 秒球速 +10%
            if (frameCounter > 0 && frameCounter % 600 == 0) {
                ballSpeedIncrease += 0.1f;
            }
            
            // 更新球
            ball.Move();
            ball.BounceEdge(screenWidth, screenHeight);
            
            // 更新额外的球（多球）
            if (multiballActive) {
                extraBall.Move();
                extraBall.BounceEdge(screenWidth, screenHeight);
            }
            
            // 更新道具
            for (auto& pu : powerups) {
                pu.Update();
            }
            powerups.erase(std::remove_if(powerups.begin(), powerups.end(), 
                [](const PowerUp& p) { return !p.IsActive(); }), powerups.end());
            
            // 更新计时器
            if (paddleExpandTimer > 0) paddleExpandTimer--;
            if (ballSlowTimer > 0) ballSlowTimer--;
            if (pierceTimer > 0) pierceTimer--;
            
            // 挡板移动
            if (IsKeyDown(KEY_LEFT)) paddle.MoveLeft(9);
            if (IsKeyDown(KEY_RIGHT)) paddle.MoveRight(9);
            
            // 暂停
            if (IsKeyPressed(KEY_P)) gameState = PAUSED;
            
            // ===== 碰撞检测 =====
            
            // 球碰挡板
            auto checkPaddleCollision = [&](Ball& b) {
                Vector2 ballPos = b.GetPosition();
                Vector2 ballSpeed = b.GetSpeed();
                float ballRadius = b.GetRadius();
                Rectangle paddleRect = paddle.GetRect();
                if (ballSpeed.y > 0 && CheckCollisionCircleRec(ballPos, ballRadius, paddleRect)) {
                    b.ReverseY();
                    ballPos.y = paddleRect.y - ballRadius - 1.0f;
                    b.SetPosition(ballPos);
                }
            };
            checkPaddleCollision(ball);
            if (multiballActive) checkPaddleCollision(extraBall);
            
            // 球碰砖块
            auto checkBrickCollision = [&](Ball& b) {
                bool hitBrick = false;
                for (auto& brick : bricks) {
                    if (!brick.IsActive()) continue;
                    if (CheckCollisionCircleRec(b.GetPosition(), b.GetRadius(), brick.GetRect())) {
                        if (pierceTimer == 0) {  // 非穿透模式
                            if (brick.Hit()) {
                                int basePoints = brick.GetPoints();
                                int multiplier = 1 + (combo / 5);
                                score += basePoints * multiplier;
                                combo++;
                                TryDropPowerUp(powerups, {brick.GetRect().x, brick.GetRect().y});
                            }
                            b.ReverseY();
                            hitBrick = true;
                            break;
                        } else {  // 穿透模式，直接摧毁
                            if (brick.Hit()) {
                                score += brick.GetPoints();
                                combo++;
                                TryDropPowerUp(powerups, {brick.GetRect().x, brick.GetRect().y});
                            }
                        }
                    }
                }
                if (!hitBrick) combo = 0;
            };
            checkBrickCollision(ball);
            if (multiballActive) checkBrickCollision(extraBall);
            
            // 挡板碰道具
            for (auto& pu : powerups) {
                if (CheckCollisionCircleRec(pu.GetPosition(), pu.GetRadius(), paddle.GetRect())) {
                    switch (pu.GetType()) {
                        case PADDLE_EXPAND:
                            paddle.SetWidth(paddle.GetWidth() + 50);
                            paddleExpandTimer = 300;  // 5 秒
                            break;
                        case BALL_SLOW:
                            ballSpeedIncrease = std::max(0.5f, ballSpeedIncrease - 0.3f);
                            ballSlowTimer = 300;
                            score += 50;
                            break;
                        case BALL_PIERCE:
                            pierceTimer = 180;  // 3 秒
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
                    pu.SetActive(false);
                }
            }
            
            // 球触底
            auto checkBottomCollision = [&](Ball& b) {
                if (b.GetPosition().y + b.GetRadius() >= screenHeight) {
                    return true;
                }
                return false;
            };
            
            if (checkBottomCollision(ball)) {
                if (multiballActive) {
                    multiballActive = false;
                } else {
                    lives--;
                    if (lives <= 0) {
                        gameState = GAME_OVER;
                        leaderboard.push_back({score, level});
                        std::sort(leaderboard.rbegin(), leaderboard.rend(), 
                            [](const HighScore& a, const HighScore& b) { return a.score < b.score; });
                        if (leaderboard.size() > 10) leaderboard.pop_back();
                        SaveLeaderboard(leaderboard);
                    } else {
                        // 失球后进入准备阶段（倒计时）
                        int randomX = 100 + rand() % 600;
                        int randomY = 150 + rand() % 150;
                        ball = Ball({(float)randomX, (float)randomY}, {0, 0}, 10);
                        combo = 0;
                        levelReadyCountdown = 180;  // 3秒倒计时
                        gameState = LEVEL_READY;  // 进入准备阶段
                    }
                }
            }
            
            if (checkBottomCollision(extraBall)) {
                multiballActive = false;
            }
            
            // 检查关卡完成
            bool allBricksClear = true;
            for (const auto& brick : bricks) {
                if (brick.IsActive()) {
                    allBricksClear = false;
                    break;
                }
            }
            if (allBricksClear) {
                level++;
                LevelData nextLevel = InitializeLevel(level);
                // 下一关球位置也随机
                int randomX = 100 + rand() % 600;
                int randomY = 150 + rand() % 150;
                ball = Ball({(float)randomX, (float)randomY}, {0, 0}, 10);
                paddle = Paddle(300, 550, nextLevel.paddleWidth, 20);
                RebuildBricks(bricks, nextLevel);
                powerups.clear();
                paddleExpandTimer = 0;
                ballSlowTimer = 0;
                pierceTimer = 0;
                multiballActive = false;
                paddle.ResetWidth();
                ballSpeedIncrease = 1.0f;
                combo = 0;
                levelReadyCountdown = 180;  // 3秒倒计时
                gameState = LEVEL_READY;  // 进入准备阶段
            }
        }
        
        // ============ 暂停 ============
        if (gameState == PAUSED) {
            if (IsKeyPressed(KEY_P)) gameState = PLAYING;
        }
        
        // ============ 绘制 ============
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // 绘制墙壁
        DrawRectangle(0, 0, 5, screenHeight, GRAY);
        DrawRectangle(screenWidth - 5, 0, 5, screenHeight, GRAY);
        DrawRectangle(0, 0, screenWidth, 5, GRAY);
        DrawRectangle(0, screenHeight - 5, screenWidth, 5, GRAY);
        
        if (gameState == MENU) {
            DrawText("BREAKOUT 2D", screenWidth / 2 - 150, 80, 60, DARKBLUE);
            DrawText("Press SPACE to Start", screenWidth / 2 - 180, 250, 32, DARKGRAY);
            DrawText("Press L to View Leaderboard", screenWidth / 2 - 200, 320, 24, DARKGRAY);
            DrawText("Controls: <- -> to move paddle | P to pause", screenWidth / 2 - 250, 450, 20, GRAY);
        } else if (gameState == LEADERBOARD) {
            DrawText("TOP 10 SCORES", screenWidth / 2 - 150, 50, 40, DARKBLUE);
            for (size_t i = 0; i < leaderboard.size() && i < 10; i++) {
                DrawText(TextFormat("#%d: %d pts (Level %d)", i + 1, leaderboard[i].score, leaderboard[i].level),
                    100, 120 + i * 40, 24, DARKGRAY);
            }
            DrawText("Press ESC to return", screenWidth / 2 - 150, 550, 20, GRAY);
        } else if (gameState == LEVEL_READY) {
            // 绘制游戏元素但球不移动
            ball.Draw();
            paddle.Draw();
            for (auto& brick : bricks) brick.Draw();
            
            // UI
            DrawText(TextFormat("Score: %d", score), 12, 10, 20, DARKGRAY);
            DrawText(TextFormat("Level: %d", level), 12, 40, 20, DARKGRAY);
            DrawText(TextFormat("Lives: %d", lives), 12, 70, 20, DARKGRAY);
            
            // 倒计时文本
            int secondsLeft = (levelReadyCountdown + 59) / 60;  // 取整向上
            if (secondsLeft > 0) {
                DrawText("READY?", screenWidth / 2 - 100, screenHeight / 2 - 80, 50, RED);
                DrawText(TextFormat("%d", secondsLeft), screenWidth / 2 - 30, screenHeight / 2 + 50, 80, ORANGE);
            } else {
                DrawText("GO!", screenWidth / 2 - 50, screenHeight / 2 - 40, 60, GREEN);
            }
        } else if (gameState == PLAYING) {
            ball.Draw();
            if (multiballActive) extraBall.Draw();
            paddle.Draw();
            for (auto& brick : bricks) brick.Draw();
            for (auto& pu : powerups) pu.Draw();
            
            // UI
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
        } else if (gameState == GAME_OVER) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 200});
            DrawText("GAME OVER", screenWidth / 2 - 150, screenHeight / 2 - 60, 48, RED);
            DrawText(TextFormat("Final Score: %d | Level: %d", score, level), screenWidth / 2 - 200, screenHeight / 2, 28, WHITE);
            DrawText("Press R to Restart or ESC to Menu", screenWidth / 2 - 220, screenHeight / 2 + 100, 24, WHITE);
            
            if (IsKeyPressed(KEY_R)) {
                gameState = MENU;
            }
        } else if (gameState == PAUSED) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 150});
            DrawText("PAUSED", screenWidth / 2 - 100, screenHeight / 2 - 30, 50, WHITE);
            DrawText("Press P to Resume", screenWidth / 2 - 150, screenHeight / 2 + 50, 24, WHITE);
        }
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}
