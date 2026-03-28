#include "Brick.h"

Brick::Brick(float x, float y, float w, float h, int brickType) 
    : type(brickType) {
    rect = { x, y, w, h };
    active = true;
    durability = (type == 1) ? 1 : (type == 2 ? 2 : 3);
}

void Brick::Draw() {
    if (active) {
        // 按类型绘制颜色：1=绿普通, 2=蓝硬, 3=金稀有
        Color brickColor = (type == 1) ? GREEN : (type == 2 ? BLUE : GOLD);
        DrawRectangleRec(rect, brickColor);
        
        // 硬砖和金砖显示剩余耐久度
        if (type >= 2) {
            int textX = (int)(rect.x + rect.width / 2 - 5);
            int textY = (int)(rect.y + rect.height / 2 - 8);
            DrawText(TextFormat("%d", durability), textX, textY, 14, BLACK);
        }
        DrawRectangleLines(rect.x, rect.y, rect.width, rect.height, DARKGRAY);
    }
}

bool Brick::Hit() {
    durability--;
    if (durability <= 0) {
        active = false;
        return true;  // 被完全摧毁
    }
    return false;  // 仍存活但受伤
}

