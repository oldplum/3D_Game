#ifndef POWERUP_H
#define POWERUP_H

#include "raylib.h"

// 道具类型
enum PowerUpType {
    PADDLE_EXPAND,      // 加宽板 (绿色)
    BALL_SLOW,          // 减速球 (黄色)
    BALL_PIERCE,        // 穿透弹 (红色)
    MULTI_BALL,         // 多球 (紫色)
    SLOW_FIELD          // 慢速区 (青色)
};

class PowerUp {
private:
    Vector2 position;
    Vector2 velocity;
    float radius;
    PowerUpType type;
    bool active;
    int duration;  // 道具持续时间（帧数）
    static const int FALL_SPEED = 3;
    
public:
    PowerUp(Vector2 pos, PowerUpType t);
    
    void Update();
    void Draw();
    
    bool IsActive() const { return active; }
    void SetActive(bool a) { active = a; }
    
    Vector2 GetPosition() const { return position; }
    float GetRadius() const { return radius; }
    PowerUpType GetType() const { return type; }
    
    // 检查是否超出屏幕
    bool IsOutOfBounds() const { return position.y > 600; }
};

#endif
