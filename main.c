#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "raylib.h"
#include "raymath.h"

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
#define Vector2_NULL ((Vector2){.x = -1.0f, .y =-1.0f})

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

typedef struct {
    float top, right, bottom, left;
} Border;

Border border_all(float value) {
    Border b = {.top = value, .right = value, .bottom = value, .left = value};
    return b;
}


typedef struct UIElement {
    Vector2 position;
    Size size;
    Padding padding;
    Border border;
    Color border_color;
    float child_gap;
    struct UIElement *children;
    Color bg_color;
} UIElement;


// fini ui framework

UIElement children[UI_CHILDREN_LIMIT];
size_t children_size = 0;
UIElement *selected = NULL;
Color colors[] = {PURPLE, YELLOW, ORANGE, PINK, RED, GREEN, BEIGE, BROWN};


UIElement layout_init() {
    children[children_size++] = (UIElement){
        .position = Vector2_ZERO,
        .size = {.width = 300, .height = 300},
        .children = NULL,
        .bg_color = colors[children_size - 1]
    };
    children[children_size++] = (UIElement){
        .position = Vector2_ZERO,
        .size = {.width = 350, .height = 200},
        .children = NULL,
        .bg_color = colors[children_size - 1]
    };
    UIElement root = {
        .position = {.x = 50, .y = 50},
        .padding = padding_all(20),
        .child_gap = 20,
        .children = children,
        .bg_color = BLUE
    };

    return root;
}

void layout_add(UIElement *root) {
    size_t count = sizeof(colors) / sizeof(colors[0]);

    if (children_size >= count) {
        return;
    }

    children[children_size++] = (UIElement){
        .position = Vector2_ZERO,
        .size = {.width = 50, .height = 50},
        .children = NULL,
        .bg_color = colors[children_size - 1]
    };
}

void layout_remove(UIElement *root) {
    if (children_size <= 2) {
        return;
    }
    children_size--;
}



void draw_rectangle(UIElement e, float posX, float posY, float width, float height) {
    DrawRectangle(posX, posY, width, height, e.border_color);
    DrawRectangle(
        posX + e.border.left,
        posY + e.border.top,
        width - e.border.left - e.border.right,
        height - e.border.top - e.border.bottom,
        e.bg_color
    );
}


void draw_uielement(UIElement e, Vector2 click) {
    // 1. layout (calculate sizing)
    float left_offest = e.position.x + e.padding.left + e.border.left;

    for (int i = 0; i < children_size; i++) {
        UIElement c = e.children[i];
        e.size.width += c.size.width;
        e.size.height = fmaxf(e.size.height, c.size.height);
    }

    float all_child_gaps = ((float) children_size - 1) * e.child_gap;
    e.size.width += e.padding.left + e.padding.right + all_child_gaps;
    e.size.height += e.padding.top + e.padding.bottom;

    // 2. draw (calculate positions)
    draw_rectangle(e, e.position.x, e.position.y, e.size.width, e.size.height);

    bool found = false;
    for (int i = 0; i < children_size; i++) {
        UIElement c = e.children[i];

        if (!Vector2Equals(click, Vector2_ZERO) && CheckCollisionPointRec(
                click,
                (Rectangle){
                    .x = left_offest + c.position.x,
                    .y = e.position.y + e.padding.top + c.position.y,
                    .width = c.size.width,
                    .height = c.size.height,
                })
        ) {
            selected = &e.children[i];
            found = true;
        }

        if (&e.children[i] == selected) {
            c.border = border_all(3);
            c.border_color = RED;
        } else {
            c.border = border_all(0);
        }

        draw_rectangle(
            c,
            left_offest + c.position.x,
            e.position.y + e.padding.top + c.position.y,
            c.size.width,
            c.size.height
        );
        left_offest += c.size.width + e.child_gap;
    }

    if (!Vector2Equals(click, Vector2_ZERO) && !found) {
        selected = NULL;
    }
}


void draw_frame(UIElement root, Vector2 click) {
    BeginDrawing();
    ClearBackground(DARKGRAY);

    draw_uielement(root, click);

    // debug size of screen
    int32_t width = GetScreenWidth();
    int32_t height = GetScreenHeight();
    draw_text(
        TextFormat("%d x %d", width, height),
        (Vector2){.x = 20, .y = 20}
    );

    EndDrawing();
}

typedef enum GROW_DIR {
    GROW_DIR_WIDTH,
    GROW_DIR_HEIGHT,
    GROW_DIR_COUNT,
} GROW_DIRECTION;

void change_size(UIElement *e, GROW_DIRECTION direction, float amount) {
    if (e == NULL) { return; }
    switch (direction) {
        case GROW_DIR_WIDTH:
            e->size.width = fmaxf(e->size.width + amount, 0);
            break;
        case GROW_DIR_HEIGHT:
            e->size.height = fmaxf(e->size.height + amount, 0);
            break;
        case GROW_DIR_COUNT:
            assert(false);
            break;
    }
}

int main(void) {
    init();

    float amount = 10;
    Vector2 click = Vector2_ZERO;

    UIElement root = layout_init();

    while (!quit_pressed()) {
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
            change_size(selected, GROW_DIR_WIDTH, amount);
        }
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
            change_size(selected, GROW_DIR_WIDTH, -amount);
        }
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
            change_size(selected, GROW_DIR_HEIGHT, -amount);
        }
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
            change_size(selected, GROW_DIR_HEIGHT, amount);
        }

        if (IsKeyPressed(KEY_F)) {
            layout_add(&root);
        }
        if (IsKeyPressed(KEY_G)) {
            layout_remove(&root);
        }

        click = Vector2_ZERO;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            click = GetMousePosition();
        }

        draw_frame(root, click);
    }

    fini();
    return 0;
}
