#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

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

typedef enum SIZING_TYPE {
    SIZING_TYPE_FIT,
    SIZING_TYPE_FIXED,
    SIZING_TYPE_GROW,
} SIZING_TYPE;

typedef struct {
    float width, height;
    SIZING_TYPE width_sizing, height_sizing;
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


void layout_add_with_params(UIElement *_, Vector2 pos, Size size, Color bg_color) {
    size_t count = sizeof(colors) / sizeof(colors[0]);

    if (children_size >= count) {
        return;
    }

    children[children_size] = (UIElement){
        .position = pos,
        .size = size,
        .children = NULL,
        .bg_color = bg_color,
    };
    children_size++;
}

void layout_add(UIElement *root) {
    layout_add_with_params(
        root,
        Vector2_ZERO,
        (Size){.width = 50, .height = 50},
        colors[children_size]
    );
}


UIElement layout_init() {
    UIElement root = {
        .size = {
            .width = WINDOW_WIDTH,
            .width_sizing = SIZING_TYPE_FIXED,
            .height = WINDOW_HEIGHT,
            .height_sizing = SIZING_TYPE_FIXED,
        },
        .padding = padding_all(20),
        .child_gap = 20,
        .children = children,
        .bg_color = BLUE
    };

    layout_add_with_params(
        &root,
        Vector2_ZERO,
        (Size){.width = 300, .height = 300},
        colors[children_size]
    );
    layout_add_with_params(
        &root,
        Vector2_ZERO,
        (Size){
            .width = 350,
            .width_sizing = SIZING_TYPE_GROW,
            .height = 200,
            .height_sizing = SIZING_TYPE_GROW,
        },
        colors[children_size]
    );
    layout_add_with_params(
        &root,
        Vector2_ZERO,
        (Size){.width = 300, .height = 300},
        colors[children_size]
    );


    return root;
}

void layout_remove(UIElement *_) {
    if (children_size <= 2) {
        return;
    }
    children_size--;
}


void draw_rectangle(UIElement e, float x, float y, float width, float height) {
    DrawRectangle((int) x, (int) y, (int) width, (int) height, e.border_color);
    DrawRectangle(
        (int) (x + e.border.left),
        (int) (y + e.border.top),
        (int) (width - e.border.left - e.border.right),
        (int) (height - e.border.top - e.border.bottom),
        e.bg_color
    );
}


void draw_uielement(UIElement parent, Vector2 click) {
    // 1. fit sizing
    float left_offest = parent.position.x + parent.padding.left + parent.border.left;

    for (size_t i = 0; i < children_size; i++) {
        UIElement c = parent.children[i];
        switch (parent.size.width_sizing) {
            case SIZING_TYPE_FIT:
                parent.size.width += c.size.width;
                break;
            case SIZING_TYPE_FIXED:
            case SIZING_TYPE_GROW:
                break;
        }
        switch (parent.size.height_sizing) {
            case SIZING_TYPE_FIT:
                parent.size.height = fmaxf(parent.size.height, c.size.height);
                break;
            case SIZING_TYPE_FIXED:
            case SIZING_TYPE_GROW:
                break;
        }
    }

    float all_child_gaps = ((float) children_size - 1) * parent.child_gap;
    switch (parent.size.width_sizing) {
        case SIZING_TYPE_FIT:
            parent.size.width += parent.padding.left + parent.padding.right + all_child_gaps;
            break;
        case SIZING_TYPE_FIXED:
            break;
        case SIZING_TYPE_GROW:
            assert(false);
            break;
    }
    switch (parent.size.width_sizing) {
        case SIZING_TYPE_FIT:
            parent.size.height += parent.padding.top + parent.padding.bottom;
            break;
        case SIZING_TYPE_FIXED:
            break;
        case SIZING_TYPE_GROW:
            assert(false);
            break;
    }

    // 2. grow sizing
    float remaining_width = parent.size.width - parent.padding.left - parent.padding.right;
    for (size_t i = 0; i < children_size; i++) {
        remaining_width -= parent.children[i].size.width;
    }
    remaining_width -= all_child_gaps;

    float remaining_height = parent.size.height - parent.padding.top - parent.padding.bottom;

    for (size_t i = 0; i < children_size; i++) {
        switch (parent.children[i].size.width_sizing) {
            case SIZING_TYPE_FIT:
            case SIZING_TYPE_FIXED:
                break;
            case SIZING_TYPE_GROW:
                parent.children[i].size.width += remaining_width;
                break;
        }
        switch (parent.children[i].size.height_sizing) {
            case SIZING_TYPE_FIT:
            case SIZING_TYPE_FIXED:
                break;
            case SIZING_TYPE_GROW:
                parent.children[i].size.height += remaining_height - parent.children[i].size.height;
                break;
        }
    }

    // 3. calculate positions and draw
    draw_rectangle(parent, parent.position.x, parent.position.y, parent.size.width, parent.size.height);

    bool found = false;
    for (size_t i = 0; i < children_size; i++) {
        UIElement c = parent.children[i];

        float x, y, height, width;
        x = left_offest + c.position.x;
        width = c.size.width;
        y = parent.position.y + parent.padding.top + c.position.y;
        height = c.size.height;

        {
            if (!Vector2Equals(click, Vector2_ZERO) && CheckCollisionPointRec(
                    click,
                    (Rectangle){
                        .x = x,
                        .y = y,
                        .width = width,
                        .height = height,
                    })
            ) {
                selected = &parent.children[i];
                found = true;
            }

            if (&parent.children[i] == selected) {
                c.border = border_all(3);
                c.border_color = RED;
            } else {
                c.border = border_all(0);
            }
        }

        draw_rectangle(c, x, y, width, height);
        left_offest += c.size.width + parent.child_gap;
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
    }
}


// init tree structure
typedef struct Node {
    char value;
    struct Node *first_children;
    struct Node *next_sibling;
} Node;

typedef struct Allocator {
    size_t len;
    size_t cap;
    Node *data;
} NodeAllocator;

NodeAllocator node_allocator_init() {
    NodeAllocator a = {0};
    a.len = 0;
    a.cap = 1024 * 1024;
    a.data = malloc(sizeof(uint8_t) * a.cap);
    assert(a.data != NULL);
    return a;
}

Node *node_alloc(NodeAllocator *a) {
    assert(a->len + 1 < a->cap);
    return &a->data[a->len++];
}

Node *node_add(NodeAllocator *a, Node *parent, char v) {
    // https://en.wikipedia.org/wiki/Left-child_right-sibling_binary_tree
    Node *n = node_alloc(a);
    *n = (Node){.value = v};

    if (parent == NULL) {
        return n;
    }

    if (parent->first_children == NULL) {
        parent->first_children = n;
    } else {
        Node *c = parent->first_children;
        while (c->next_sibling != NULL) {
            c = c->next_sibling;
        }
        c->next_sibling = n;
    }
    return n;
}

void node_print_postorder(Node *node) {
    if (node == NULL) {
        return;
    }
    if (node->first_children == NULL) {
        printf("%c\n", node->value);
        return;
    }

    Node *child = node->first_children;
    while (child->next_sibling != NULL) {
        node_print_postorder(child);
        child = child->next_sibling;
    }
    node_print_postorder(child);

    printf("%c\n", node->value);
}
// fini tree structure

int main(void) {
    init();

    NodeAllocator a = node_allocator_init();
    Node *r = node_add(&a, NULL, '1');

    Node *n2 = node_add(&a, r, '2');
    node_add(&a, n2, '5');
    Node *n3 = node_add(&a, r, '3');
    Node *n4 = node_add(&a, r, '4');
    node_add(&a, n4, '6');
    node_add(&a, n4, '7');
    node_add(&a, n4, '8');
    node_add(&a, n4, '9');



    printf("================================\n");

    node_print_postorder(r);

    printf("================================\n");

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


        if (IsWindowResized()) {
            root.size.width = (float) GetScreenWidth();
            root.size.height = (float) GetScreenHeight();
        }
        draw_frame(root, click);
    }

    fini();
    return 0;
}
