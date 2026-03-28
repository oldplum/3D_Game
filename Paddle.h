#ifndef PADDLE_H
#define PADDLE_H

#include "raylib.h"

class Paddle {
private:
    Rectangle rect;
    float originalWidth;  // 原始宽度，用于重置
public:
    Paddle(float x = 350, float y = 550, float w = 100, float h = 20);
    void Draw();
    void MoveLeft(float speed);
    void MoveRight(float speed);
    Rectangle GetRect() const { return rect; }
    
    // 道具相关方法
    void SetWidth(float w);
    void ResetWidth() { rect.width = originalWidth; }
    float GetWidth() const { return rect.width; }
};

#endif