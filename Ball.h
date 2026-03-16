#ifndef BALL_H
#define BALL_H

#include "raylib.h"

class Ball {
private:
    Vector2 position;
    Vector2 speed;
    float radius;
public:
    Ball(Vector2 pos, Vector2 sp, float r);
    void Move();
    void Draw();
    void BounceEdge(int screenWidth, int screenHeight);
    Vector2 GetPosition() const;
    Vector2 GetSpeed() const;
    float GetRadius() const;
    void SetPosition(Vector2 pos);
    void SetSpeed(Vector2 sp);
    void ReverseY();
};

#endif