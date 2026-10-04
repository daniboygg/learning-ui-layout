#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "raylib.h"

// Same initial size as the learning-chip8 window (64 * 15 + 640, 32 * 15 + 400).
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_MIN_WIDTH 320
#define WINDOW_MIN_HEIGHT 240

#define FONT_SIZE 24

Font ui_font;

void init() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "UI Layout");
    SetWindowMinSize(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT);
    SetTargetFPS(60);

    ui_font = LoadFontEx("assets/JetBrainsMono-Bold.ttf", FONT_SIZE, NULL, 0);
}

void fini() {
    UnloadFont(ui_font);
    CloseWindow();
}

bool quit_pressed() {
    return WindowShouldClose() || IsKeyPressed(KEY_CAPS_LOCK);
}

void draw_text(const char *text, Vector2 position) {
    DrawTextEx(
        ui_font,
        text,
        position,
        FONT_SIZE,
        0,
        WHITE
    );
}


// init ui framework

#define UI_CHILDREN_LIMIT 512

#define Vector2_ZERO ((Vector2){.x = 0.0f, .y = 0.0f})

typedef struct {
    float width, height;
} Size;

typedef struct {
    float top, right, bottom, left;
} Padding;

Padding padding_all(float value) {
    Padding p = {.top = value, .right = value, .bottom = value, .left = value};
    return p;
}


typedef struct UIElement {
    Vector2 position;
    Size size;
    Padding padding;
    float child_gap;
    struct UIElement *children;
    Color bg_color;
} UIElement;


// fini ui framework

size_t children_size = 0;

void draw_uielement(UIElement e) {
    // 1. layout (calculate sizing)
    float left_offest = e.position.x + e.padding.left;

    for (int i = 0; i < children_size; i++) {
        UIElement c = e.children[i];
        e.size.width += c.size.width;
        e.size.height = fmaxf(e.size.height, c.size.height);
    }

    float all_child_gaps = ((float)children_size - 1) * e.child_gap;
    e.size.width += e.padding.left + e.padding.right + all_child_gaps;
    e.size.height += e.padding.top + e.padding.bottom;

    // 2. draw (calculate positions)
    DrawRectangle(e.position.x, e.position.y, e.size.width, e.size.height, e.bg_color);

    for (int i = 0; i < children_size; i++) {
        UIElement c = e.children[i];
        DrawRectangle(
            left_offest + c.position.x,
            e.position.y + e.padding.top + c.position.y,
            c.size.width,
            c.size.height,
            c.bg_color
        );
        left_offest += c.size.width + e.child_gap;
    }
}


void draw_frame() {
    BeginDrawing();
    ClearBackground(DARKGRAY);

    UIElement children[UI_CHILDREN_LIMIT];
    children_size = 0;

    children[children_size++] = (UIElement){
        .position = Vector2_ZERO,
        .size = {.width = 300, .height = 300},
        .children = NULL,
        .bg_color = PURPLE
    };
    children[children_size++] = (UIElement){
        .position = Vector2_ZERO,
        .size = {.width = 350, .height = 200},
        .children = NULL,
        .bg_color = YELLOW
    };

    // drawing algorithm
    UIElement rect = {
        .position = {.x = 50, .y = 50},
        .padding = padding_all(20),
        .child_gap = 20,
        .children = children,
        .bg_color = BLUE
    };

    draw_uielement(rect);

    // debug size of screen
    int32_t width = GetScreenWidth();
    int32_t height = GetScreenHeight();
    draw_text(
        TextFormat("%d x %d", width, height),
        (Vector2){.x = 20, .y = 20}
    );

    EndDrawing();
}

int main(void) {
    init();

    while (!quit_pressed()) {
        draw_frame();
    }

    fini();
    return 0;
}
