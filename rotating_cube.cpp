#include "raylib.h"
#include "rlgl.h"

int main() {
    // 初始化窗口
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Rotating Cube"); //设置窗口宽高和标题

    // 配置 3D 摄像机：从斜上方俯视原点
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 6.0f, 6.0f, 6.0f };   // 摄像机位置
    camera.target   = (Vector3){ 0.0f, 1.0f, 0.0f };   // 注视目标
    camera.up       = (Vector3){ 0.0f, 1.0f, 0.0f };   // 上方向
    camera.fovy     = 45.0f;                             // 视场角
    camera.projection = CAMERA_PERSPECTIVE;              // 透视投影

    // 立方体参数
    const Vector3 cubeSize = { 2.0f, 2.0f, 2.0f };

    // 按优先级尝试可显示中文的字体（WSL 下优先复用 Windows 字体）
    const char *fontCandidates[] = {
        "/mnt/c/Windows/Fonts/NotoSansSC-VF.ttf",
        "/mnt/c/Windows/Fonts/msyh.ttc",
        "/mnt/c/Windows/Fonts/simhei.ttf",
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc"
    };

    // 目标文本：后续按这段文本提取码点，确保中文字形被加载
    const char *memberText = u8"Stanley Lee & 梁致夏";

    Font chineseFont = { 0 };
    bool customFontLoaded = false;
    for (int i = 0; i < 4; i++) {
        if (FileExists(fontCandidates[i])) {
            int codepointCount = 0;
            // 从 UTF-8 文本提取 Unicode 码点，避免中文变成 ???
            int *codepoints = LoadCodepoints(memberText, &codepointCount);
            chineseFont = LoadFontEx(fontCandidates[i], 36, codepoints, codepointCount);
            UnloadCodepoints(codepoints);
            if (chineseFont.texture.id > 0) {
                customFontLoaded = true;
                break;
            }
        }
    }
    // 所有候选字体都失败时回退默认字体（通常不含中文，仅保证程序可运行）
    if (!customFontLoaded) 
        chineseFont = GetFontDefault();

    SetTargetFPS(60);  //帧率

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

        const float memberFontSize = 26.0f;
        // 先测量文本尺寸，再按窗口宽度水平居中绘制
        Vector2 memberSize = MeasureTextEx(chineseFont, memberText, memberFontSize, 1.0f);
        DrawTextEx(chineseFont, memberText, (Vector2){ (screenWidth - memberSize.x) * 0.5f, (float)screenHeight - 22.0f }, memberFontSize, 1.0f, MAROON);

        EndDrawing();
    }

    // 仅释放自定义加载的字体，默认字体无需手动释放
    if (customFontLoaded) 
        UnloadFont(chineseFont);
    CloseWindow();
    return 0;
}
