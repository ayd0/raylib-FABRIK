#include <math.h>
#include <raylib.h>
#include <stdlib.h>

#define SCR_W 1920
#define SCR_H 1080
#define NUM_ARMS 5

typedef struct
{
    Vector2 pos;
    float r;
    Color color;

    Vector2 para_pos;
    Vector2 para_normal;
    float para_r;
    float para_c;
    Color para_color;
} Arm;

void Arm_ArmsInit(Arm** arms, int arms_size);
void Arm_UpdateParaPos(Arm* arm);
void Arm_ResolveDistConstraint(Arm* arm, Arm* last);
void Arm_ArmsResolveDistConstraints(Arm** arms, int arms_size, bool invert);
void Arm_ArmsResolveFABRIK(Arm** arms, int arms_size, Vector2* anchor, Vector2* target);
void Arm_DrawArms(Arm** arms, int arms_size);

int main()
{
    SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    InitWindow(SCR_W, SCR_H, "FABRIK test");

    Vector2 scr_center = { SCR_W/2.0f, SCR_H/2.0f };

    Camera2D camera = { 0 };
    camera.target = scr_center;
    camera.offset = scr_center;
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    int arms_len = NUM_ARMS;
    int arms_size = arms_len;
    Arm** arms = (Arm**)malloc(sizeof(Arm*) * arms_size);
    for (int i = 0; i < arms_size; ++i)
    {
        arms[i] = (Arm*)malloc(sizeof(Arm));
        Arm* arm = arms[i];

        arm->pos = scr_center;
        arm->r = 90.0f;
        arm->color = SKYBLUE;

        arm->para_pos = (Vector2){ 0.0f, 0.0f };
        arm->para_normal = (Vector2){ 0.0f, 0.0f };
        arm->para_r = 10.0f;
        arm->para_c = 0.0f;
        arm->para_color = MAROON;
    }

    Vector2 anchor_pos = scr_center;
    Vector2 target_pos = (Vector2){ 0.0f, 0.0f };
    bool target_active = false;

    Arm_ArmsInit(arms, arms_size);

    while (!WindowShouldClose())
    {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            target_active = true;
            target_pos = GetMousePosition();
        }
        if (target_active)
        {
            Arm_ArmsResolveFABRIK(arms, arms_size, &anchor_pos, &target_pos);
        }
        
        ClearBackground(BLACK);
        BeginDrawing();
        BeginMode2D(camera);

        Arm_DrawArms(arms, arms_size);
        DrawCircleV(anchor_pos, 5.0f, RAYWHITE);
        if (target_active)
        {
            DrawCircleV(target_pos, 5.0f, RAYWHITE);
        }

        EndMode2D();
        EndDrawing();
    }

    CloseWindow();
    
    for (int i = 0; i < arms_size; ++i)
    {
        free(arms[i]);
    }
    free(arms);

    return 0;
}

void Arm_UpdateParaPos(Arm* arm)
{
    arm->para_pos.x = arm->pos.x + arm->r * sinf(arm->para_c);
    arm->para_pos.y = arm->pos.y + arm->r * cosf(arm->para_c);

    Vector2 diff = { arm->para_pos.x - arm->pos.x, arm->para_pos.y - arm->pos.y };
    float P = sqrtf(diff.x * diff.x + diff.y * diff.y);
    arm->para_normal = (Vector2){ diff.x / P, diff.y / P };
}

void Arm_ResolveDistConstraint(Arm* arm, Arm* last)
{
    Vector2 diff = { last->pos.x - arm->pos.x, last->pos.y - arm->pos.y };
    float P = sqrtf(diff.x * diff.x + diff.y * diff.y);
    if (P > arm->r)
    {
        Vector2 normal = { diff.x / P, diff.y / P };
        float dist = P - arm->r;
        arm->pos.x += dist * normal.x;
        arm->pos.y += dist * normal.y;
    }
}

void Arm_ArmsResolveDistConstraints(Arm** arms, int arms_size, bool invert)
{
    if (arms_size > 1)
    {
        if (invert)
        {
            for (int i = arms_size - 1; i >= 1; --i)
            {
                Arm_ResolveDistConstraint(arms[i - 1], arms[i]);
            }
        }
        else
        {
            for (int i = 1; i < arms_size; ++i)
            {
                Arm_ResolveDistConstraint(arms[i], arms[i - 1]);
            }
        }
    }
}

void Arm_ArmsResolveFABRIK(Arm** arms, int arms_size, Vector2* anchor, Vector2* target)
{
    arms[0]->pos = *anchor;
    Arm_ArmsResolveDistConstraints(arms, arms_size, false);
    arms[arms_size - 1]->pos = *target;
    Arm_ArmsResolveDistConstraints(arms, arms_size, true);

    Vector2 dist = { target->x - arms[0]->pos.x, target->y - arms[0]->pos.y };
    float P = sqrtf(dist.x * dist.x + dist.y * dist.y);
    if (P > 10.0f)
    {
        arms[0]->pos = *anchor;
        Arm_ArmsResolveDistConstraints(arms, arms_size, false);
    }
}

void Arm_ArmsInit(Arm** arms, int arms_size)
{
    Arm* last = NULL;
    for (int i = 0; i < arms_size; ++i)
    {
        Arm* arm = arms[i];
        if (last != NULL)
        {
            arm->pos = last->para_pos;
        }

        Arm_UpdateParaPos(arm);

        last = arm;
    }
}

void Arm_DrawArms(Arm** arms, int arms_size)
{
    if (arms_size > 0)
    {
        DrawCircleV(arms[0]->pos, arms[0]->para_r, arms[0]->para_color);
    }
    for (int i = arms_size - 1; i >= 0; --i)
    {
        Arm* arm = arms[i];
        if (i > 0)
        {
            Arm* last = arms[i - 1];
            Vector2 diff = { arm->pos.x - last->pos.x, arm->pos.y - last->pos.y};
            float P = sqrtf(diff.x * diff.x + diff.y * diff.y);
            Vector2 normal = { diff.x / P, diff.y / P };
            Rectangle rec = { last->pos.x + normal.x * P/2.0f, last->pos.y + normal.y * P/2.0f, 8.0f, P };
            DrawRectanglePro(rec, (Vector2){ rec.width/2.0f, rec.height/2.0f }, -atan2f(diff.x, diff.y) * RAD2DEG, GOLD);
        }
        // DrawCircleLinesV(arm->pos, arm->r, arm->color);
        DrawCircleV(arms[i]->pos, arms[i]->para_r, arms[i]->para_color);
    }
}
