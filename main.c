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
    Color bg_color;
} UIElement;


// init tree structure
typedef struct Node {
    int name;
    UIElement *value;
    struct Node *parent;
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
    a.data = malloc(sizeof(Node) * a.cap);
    assert(a.data != NULL);
    return a;
}

Node *node_alloc(NodeAllocator *a) {
    assert(a->len + 1 < a->cap);
    return &a->data[a->len++];
}

Node *node_add(NodeAllocator *a, Node *parent, UIElement *v, int name) {
    // https://en.wikipedia.org/wiki/Left-child_right-sibling_binary_tree
    Node *n = node_alloc(a);
    *n = (Node){.value = v, .name = name};

    if (parent == NULL) {
        return n;
    }

    if (parent->first_children == NULL) {
        parent->first_children = n;
        n->parent = parent;
    } else {
        Node *c = parent->first_children;
        while (c->next_sibling != NULL) {
            c = c->next_sibling;
        }
        c->next_sibling = n;
        n->parent = parent;
    }
    return n;
}

void node_print_postorder(Node *node) {
    if (node == NULL) {
        return;
    }
    if (node->first_children == NULL) {
        printf("%d\n", node->name);
        return;
    }

    Node *child = node->first_children;
    while (child->next_sibling != NULL) {
        node_print_postorder(child);
        child = child->next_sibling;
    }
    node_print_postorder(child);

    printf("%d\n", node->name);
}

// fini tree structure

// fini ui framework

UIElement children[UI_CHILDREN_LIMIT];
Color colors[] = {PURPLE, YELLOW, ORANGE, PINK, RED, GREEN, BEIGE, BROWN};
int children_size = 0;

UIElement *selected = NULL;


void layout_add_with_params(NodeAllocator *a, Node *root, Vector2 pos, Size size, Color bg_color, int name) {
    int count = sizeof(colors) / sizeof(colors[0]);

    if (children_size >= count) {
        return;
    }

    children[children_size] = (UIElement){
        .position = pos,
        .size = size,
        .bg_color = bg_color,
    };
    children_size++;
    node_add(a, root, &children[children_size - 1], name);
}

void layout_add(NodeAllocator *a, Node *root) {
    layout_add_with_params(
        a,
        root,
        Vector2_ZERO,
        (Size){
            .width = 50,
            .width_sizing = SIZING_TYPE_FIXED,
            .height = 50,
            .height_sizing = SIZING_TYPE_FIXED,
        },
        colors[children_size],
        children_size
    );
}


Node *layout_init(NodeAllocator *a) {
    UIElement root_data = (UIElement){
        .size = {
            .width = WINDOW_WIDTH,
            .width_sizing = SIZING_TYPE_FIXED,
            .height = WINDOW_HEIGHT,
            .height_sizing = SIZING_TYPE_FIXED,
        },
        .padding = padding_all(20),
        .child_gap = 20,
        .bg_color = BLUE
    };
    children[children_size++] = root_data;
    Node *root = node_add(a, NULL, &children[children_size - 1], 1);

    layout_add_with_params(
        a,
        root,
        Vector2_ZERO,
        (Size){
            .width = 300,
            .width_sizing = SIZING_TYPE_FIXED,
            .height = 300,
            .height_sizing = SIZING_TYPE_FIXED,
        },
        colors[children_size],
        2
    );
    layout_add_with_params(
        a,
        root,
        Vector2_ZERO,
        (Size){
            .width_sizing = SIZING_TYPE_GROW,
            .height_sizing = SIZING_TYPE_GROW,
        },
        colors[children_size],
        3
    );
    layout_add_with_params(
        a,
        root,
        Vector2_ZERO,
        (Size){
            .width = 350,
            .width_sizing = SIZING_TYPE_FIXED,
            .height = 200,
            .height_sizing = SIZING_TYPE_FIXED,
        },
        colors[children_size],
        4
    );

    return root;
}

// TODO
// void layout_remove(Node *_) {
//     if (children_size <= 2) {
//         return;
//     }
//     children_size--;
// }


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

void calculate_fit_sizing(Node *node) {
    // first depth post-order
    assert(node != NULL);

    UIElement *value = node->value;
    if (value->size.width_sizing == SIZING_TYPE_FIT) {
        value->size.width = 0;
    }
    if (value->size.height_sizing == SIZING_TYPE_FIT) {
        value->size.height = 0;
    }

    // leaf node
    if (node->first_children == NULL && node->parent != NULL) {
        UIElement *parent = node->parent->value;
        if (parent->size.width_sizing == SIZING_TYPE_FIT) {
            parent->size.width += value->size.width;
        }
        if (parent->size.height_sizing == SIZING_TYPE_FIT) {
            parent->size.height = fmaxf(parent->size.height, value->size.height);
        }
        return;
    }

    // iterate over all children
    Node *child = node->first_children;
    size_t n_children = 0;
    while (child != NULL) {
        calculate_fit_sizing(child);
        child = child->next_sibling;
        n_children += 1;
    }

    if (value->size.width_sizing == SIZING_TYPE_FIT) {
        float all_child_gaps = (float) (n_children - 1) * value->child_gap;
        value->size.width += all_child_gaps;
        value->size.width += value->padding.left + value->padding.right;
    }
    if (value->size.height_sizing == SIZING_TYPE_FIT) {
        value->size.height += value->padding.top + value->padding.bottom;
    }
}

void calculate_grow_sizing(Node *node) {
    assert(node != NULL);

    UIElement *value = node->value;
    float remaining_width = value->size.width - value->padding.left - value->padding.right;
    float remaining_height = node->value->size.height - node->value->padding.top - node->value->padding.bottom;

    // iterate over all children
    Node *child = node->first_children;
    size_t n_children = 0;
    while (child != NULL) {
        remaining_width -= child->value->size.width;
        child = child->next_sibling;
        n_children += 1;
    }
    if (n_children > 0) {
        float all_child_gaps = (float) (n_children - 1) * value->child_gap;
        remaining_width -= all_child_gaps;

        child = node->first_children;
        while (child != NULL) {
            if (child->value->size.width_sizing == SIZING_TYPE_GROW) {
                child->value->size.width += remaining_width;
            }
            if (child->value->size.height_sizing == SIZING_TYPE_GROW) {
                child->value->size.height += remaining_height - child->value->size.height;
            }
            child = child->next_sibling;
        }
    }

    child = node->first_children;
    while (child != NULL) {
        calculate_grow_sizing(child);
        child = child->next_sibling;
    }
}

void calculate_positions_and_draw(Node *node, Vector2 click, bool *found, float left_offest) {
    assert(node != NULL);
    draw_rectangle(
        *node->value,
        node->value->position.x,
        node->value->position.y,
        node->value->size.width,
        node->value->size.height
    );

    if (node->first_children == NULL) {
        // lead node do nothing
        return;
    }

    // iterate over all children
    Node *c = node->first_children;
    while (c != NULL) {
        float x, y, height, width;
        x = left_offest + c->value->position.x;
        width = c->value->size.width;
        y = node->value->position.y + node->value->padding.top + c->value->position.y;
        height = c->value->size.height;

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
                selected = c->value;
                *found = true;
            }

            if (c->value == selected) {
                c->value->border = border_all(3);
                c->value->border_color = BLACK;
            } else {
                c->value->border = border_all(0);
            }
        }

        draw_rectangle(*c->value, x, y, width, height);
        left_offest += c->value->size.width + node->value->child_gap;

        c = c->next_sibling;
    }
}


void draw_uielement(Node *parent, Vector2 click) {
    // 1. fit sizing
    calculate_fit_sizing(parent);

    // 2. grow sizing
    calculate_grow_sizing(parent);

    // 3. calculate positions and draw
    bool found = false;
    calculate_positions_and_draw(
        parent,
        click,
        &found,
        parent->value->position.x + parent->value->padding.left
    );

    if (!Vector2Equals(click, Vector2_ZERO) && !found) {
        selected = NULL;
    }
}


void draw_frame(Node *root, Vector2 click) {
    BeginDrawing();
    ClearBackground(DARKGRAY);

    draw_uielement(root, click);

    // debug size of screen
    int32_t width = GetScreenWidth();
    int32_t height = GetScreenHeight();
    draw_text(
        TextFormat("%d x %d", width, height),
        (Vector2){.x = 0, .y = 0}
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

int main(void) {
    init();

    float amount = 10;
    Vector2 click = Vector2_ZERO;

    NodeAllocator a = node_allocator_init();
    Node *root = layout_init(&a);

    printf("==================\n");
    node_print_postorder(root);
    printf("==================\n");

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
            layout_add(&a, root);
        }
        // TODO
        // if (IsKeyPressed(KEY_G)) {
        //     layout_remove(root);
        // }

        click = Vector2_ZERO;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            click = GetMousePosition();
        }


        if (IsWindowResized()) {
            root->value->size.width = (float) GetScreenWidth();
            root->value->size.height = (float) GetScreenHeight();
        }
        draw_frame(root, click);
    }

    fini();
    return 0;
}
