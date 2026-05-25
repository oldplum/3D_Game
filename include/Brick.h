#ifndef BRICK_H
#define BRICK_H

#include "raylib.h"

// 砖块类型：1=普通绿砖(10分), 2=硬蓝砖(25分), 3=金砖(50分)
class Brick {
private:
    Rectangle rect;
    bool active;
    int type;        // 1=普通(绿), 2=硬砖(蓝), 3=金砖(黄)
    int durability;  // 普通1, 硬砖2, 金砖3次破碎
public:
    Brick(float x, float y, float w, float h, int brickType = 1);
    void Draw();
    bool IsActive() const { return active; }  // 改为 const
    void SetActive(bool a) { active = a; }
    Rectangle GetRect() const { return rect; }
    
    int GetType() const { return type; }
    int GetPoints() const { return type == 1 ? 10 : (type == 2 ? 25 : 50); }
    
    // 受击一次，返回是否被摧毁
    bool Hit();
};

#endif