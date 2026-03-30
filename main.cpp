#include "raylib.h"
#include "Game.h"

int main() {
    InitWindow(800, 600, "打砖块2D - 终极升级版");

    Game game;
    game.Init();

    while (!WindowShouldClose()) {
        game.Update();
        game.Draw();
    }

    game.Shutdown();
    CloseWindow();
    return 0;
}
