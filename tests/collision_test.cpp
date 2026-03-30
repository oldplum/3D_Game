#include "raylib.h"
#include <cassert>
#include <iostream>

int main() {
    // Simple positive case: circle overlaps paddle rectangle.
    Vector2 ballPos = {200.0f, 200.0f};
    float ballRadius = 10.0f;
    Rectangle paddleRect = {190.0f, 205.0f, 60.0f, 20.0f};

    bool hit = CheckCollisionCircleRec(ballPos, ballRadius, paddleRect);
    assert(hit && "Expected collision between ball and paddle rectangle");

    // Simple negative case: circle is far from rectangle.
    Vector2 ballPosNoHit = {50.0f, 50.0f};
    bool noHit = CheckCollisionCircleRec(ballPosNoHit, ballRadius, paddleRect);
    assert(!noHit && "Expected no collision when ball is far away");

    std::cout << "collision_test passed" << std::endl;
    return 0;
}
