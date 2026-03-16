#include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include <vector>

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "打砖块2D - 第二周");

    // 创建游戏对象
    Ball ball({400, 300}, {2, 2}, 10);
    Paddle paddle(350, 550, 100, 20);

    // 创建砖块（示例：一排5个）
    std::vector<Brick> bricks;
    float brickWidth = 100;
    float brickHeight = 30;
    for (int i = 0; i < 8; i++) {
        bricks.emplace_back(50 + i * 120, 100, brickWidth, brickHeight);
    }

    int score = 0;
    bool gameOver = false;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (!gameOver) {
            // 更新
            ball.Move();
            ball.BounceEdge(screenWidth, screenHeight);

            // 板移动
            if (IsKeyDown(KEY_LEFT)) paddle.MoveLeft(5);
            if (IsKeyDown(KEY_RIGHT)) paddle.MoveRight(5);

            // 球碰挡板反弹：仅在球向下运动时处理，避免抖动。
            Vector2 ballPos = ball.GetPosition();
            Vector2 ballSpeed = ball.GetSpeed();
            float ballRadius = ball.GetRadius();
            Rectangle paddleRect = paddle.GetRect();
            if (ballSpeed.y > 0 && CheckCollisionCircleRec(ballPos, ballRadius, paddleRect)) {
                ball.ReverseY();
                ballPos.y = paddleRect.y - ballRadius - 1.0f;
                ball.SetPosition(ballPos);
            }

            // 球碰砖块：砖块消失 + 计分 + 反弹。
            for (auto& brick : bricks) {
                if (!brick.IsActive()) continue;
                if (CheckCollisionCircleRec(ball.GetPosition(), ball.GetRadius(), brick.GetRect())) {
                    brick.SetActive(false);
                    score += 10;
                    ball.ReverseY();
                    break;
                }
            }

            // 球触底判负。
            if (ball.GetPosition().y + ball.GetRadius() >= screenHeight) {
                gameOver = true;
            }
        }

        // 绘制
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // 绘制左右墙为灰色矩形
        DrawRectangle(0, 0, 5, screenHeight, GRAY);   // 左墙宽5像素
        DrawRectangle(screenWidth - 5, 0, 5, screenHeight, GRAY); // 右墙
        // 绘制天花板和地板
        DrawRectangle(0, 0, screenWidth, 5, GRAY);    // 顶墙高5像素
        DrawRectangle(0, screenHeight - 5, screenWidth, 5, GRAY); // 底墙

        ball.Draw();
        paddle.Draw();
        for (auto& brick : bricks) brick.Draw();
        DrawText(TextFormat("Score: %d", score), 12, 10, 24, DARKGRAY);

        if (gameOver) {
            DrawText("GAME OVER", screenWidth / 2 - 120, screenHeight / 2 - 10, 40, RED);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}