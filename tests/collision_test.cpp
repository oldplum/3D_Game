#include "raylib.h"
#include "Ball.h"
#include "Brick.h"
#include <cassert>
#include <iostream>

int main() {
    auto checkBrickCollision = [](const Ball& ball, const Brick& brick) {
        return CheckCollisionCircleRec(ball.GetPosition(), ball.GetRadius(), brick.GetRect());
    };

    // Case 1: normal brick collision (clear overlap) should be true.
    Ball normalBall({120.0f, 120.0f}, {2.0f, 4.0f}, 10.0f);
    Brick normalBrick(100.0f, 110.0f, 50.0f, 20.0f, 1);
    bool normalHit = checkBrickCollision(normalBall, normalBrick);
    assert(normalHit && "Expected normal overlap collision with brick");

    // Case 2: edge collision (ball exactly touches the brick's left edge) should be true.
    Brick edgeBrick(200.0f, 220.0f, 50.0f, 20.0f, 1);
    float edgeY = edgeBrick.GetRect().y + edgeBrick.GetRect().height * 0.5f;
    Ball edgeBall({edgeBrick.GetRect().x - 10.0f, edgeY}, {3.0f, 0.0f}, 10.0f);
    bool edgeHit = checkBrickCollision(edgeBall, edgeBrick);
    assert(edgeHit && "Expected edge-touch collision with brick");

    // Optional regression guard: separated objects should not collide.
    Ball farBall({30.0f, 30.0f}, {0.0f, 0.0f}, 10.0f);
    bool farHit = checkBrickCollision(farBall, edgeBrick);
    assert(!farHit && "Expected no collision when ball is far from brick");

    std::cout << "collision_test passed: normal/edge/no-hit cases" << std::endl;
    return 0;
}
