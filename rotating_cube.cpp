#include "raylib.h"
#include "rlgl.h"

int main() {
    // 初始化窗口
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Rotating Cube");

    // 配置 3D 摄像机：从斜上方俯视原点
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 6.0f, 6.0f, 6.0f };   // 摄像机位置
    camera.target   = (Vector3){ 0.0f, 1.0f, 0.0f };   // 注视目标
    camera.up       = (Vector3){ 0.0f, 1.0f, 0.0f };   // 上方向
    camera.fovy     = 45.0f;                             // 视场角
    camera.projection = CAMERA_PERSPECTIVE;              // 透视投影

    // 立方体参数
    const Vector3 cubeSize = { 2.0f, 2.0f, 2.0f };

    SetTargetFPS(60);

    // 主循环：按 ESC 或关闭窗口退出
    while (!WindowShouldClose()) {
        // 用运行时间驱动旋转角度，每秒 45°
        float rotationAngle = (float)GetTime() * 45.0f;

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

            // 利用矩阵栈实现绕 Y 轴自转
            rlPushMatrix();
                rlTranslatef(0.0f, 1.0f, 0.0f);               // 平移到绘制中心
                rlRotatef(rotationAngle, 0.0f, 1.0f, 0.0f);   // 绕 Y 轴旋转

                // 在局部坐标原点绘制立方体（实心 + 线框）
                DrawCube((Vector3){ 0 }, cubeSize.x, cubeSize.y, cubeSize.z, SKYBLUE);
                DrawCubeWires((Vector3){ 0 }, cubeSize.x, cubeSize.y, cubeSize.z, DARKBLUE);
            rlPopMatrix();

            // 地面参考网格
            DrawGrid(20, 1.0f);

        EndMode3D();

        // 2D 文字提示
        DrawText("Raylib 3D Demo: Rotating Cube", 10, 10, 20, DARKGRAY);
        DrawText("Press ESC to exit", 10, 40, 18, GRAY);

        // 显示小组编号
        DrawText("Group 28", screenWidth/2 - 60, screenHeight - 50, 32, MAROON);
        DrawText("Stanley Lee & Liang Zhixia", screenWidth/2 - 100, screenHeight - 20, 18, MAROON);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
