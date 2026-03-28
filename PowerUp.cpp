#include "PowerUp.h"

PowerUp::PowerUp(Vector2 pos, PowerUpType t) 
    : position(pos), type(t), active(true), duration(5 * 60) {  // 持续 5 秒（60fps）
    velocity = {0, FALL_SPEED};
    radius = 10;
}

void PowerUp::Update() {
    if (!active) return;
    
    position.y += velocity.y;
    
    // 超出屏幕则消失
    if (position.y > 600) {
        active = false;
    }
}

void PowerUp::Draw() {
    if (!active) return;
    
    // 按类型绘制不同颜色
    Color color;
    const char* label = "";
    
    switch(type) {
        case PADDLE_EXPAND:
            color = GREEN;
            label = "W";  // Wide
            break;
        case BALL_SLOW:
            color = YELLOW;
            label = "S";  // Slow
            break;
        case BALL_PIERCE:
            color = RED;
            label = "P";  // Pierce
            break;
        case MULTI_BALL:
            color = MAGENTA;
            label = "M";  // Multi
            break;
        case SLOW_FIELD:
            color = Color{0, 255, 255, 255};  // 青色
            label = "F";  // Field
            break;
    }
    
    DrawCircle((int)position.x, (int)position.y, radius, color);
    DrawCircleLines((int)position.x, (int)position.y, radius, BLACK);
    DrawText(label, (int)position.x - 4, (int)position.y - 6, 12, BLACK);
}
