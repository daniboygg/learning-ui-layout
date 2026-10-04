#include <stddef.h>
#include <stdint.h>

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


void draw_frame() {
    int32_t width = GetScreenWidth();
    int32_t height = GetScreenHeight();

    BeginDrawing();
    ClearBackground(DARKGRAY);

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
