#include "Paddle.h"

Paddle::Paddle(float x, float y, float w, float h) {
    rect = { x, y, w, h };
    originalWidth = w;
}

void Paddle::Draw() {
    DrawRectangleRec(rect, BLUE);
}

void Paddle::MoveLeft(float speed) {
    rect.x -= speed;
    if (rect.x < 0) rect.x = 0;
}

void Paddle::MoveRight(float speed) {
    rect.x += speed;
    if (rect.x + rect.width > GetScreenWidth())
        rect.x = GetScreenWidth() - rect.width;
}

void Paddle::SetWidth(float w) {
    rect.width = w;
    // 保证挡板不超出右边界
    if (rect.x + rect.width > GetScreenWidth()) {
        rect.x = GetScreenWidth() - rect.width;
    }
}