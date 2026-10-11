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
        BLACK
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
    SIZING_TYPE type;
    float value;
} SizingAxis;

SizingAxis sizingaxis_fixed(float size) {
    return (SizingAxis){.type = SIZING_TYPE_FIXED, .value = size};
}

SizingAxis sizingaxis_fit(void) {
    return (SizingAxis){.type = SIZING_TYPE_FIT};
}

SizingAxis sizingaxis_grow(void) {
    return (SizingAxis){.type = SIZING_TYPE_GROW};
}

typedef struct {
    SizingAxis x, y;
} Sizing;

Sizing sizing_fixed(float x, float y) {
    return (Sizing){.x = sizingaxis_fixed(x), .y = sizingaxis_fixed(y)};
}

Sizing sizing_fit(void) {
    return (Sizing){.x = sizingaxis_fit(), .y = sizingaxis_fit()};
}

Sizing sizing_grow(void) {
    return (Sizing){.x = sizingaxis_grow(), .y = sizingaxis_grow()};
}

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

#define COLOR_BG ((Color){  78, 121, 167, 255 })
// clang-format off
Color colors[] = {
    { 255, 157, 167, 255 },  // pink
    { 237, 201,  72, 255 },  // yellow
    { 118, 183, 178, 255 },  // teal
    { 242, 142,  43, 255 },  // orange
    { 225,  87,  89, 255 },  // red
    {  89, 161,  79, 255 },  // green
    { 176, 122, 161, 255 },  // purple
    { 156, 117,  95, 255 },  // brown
    { 186, 176, 172, 255 },  // gray
};
// clang-format on
int current_color = 0;

Color get_color() {
    int count = sizeof(colors) / sizeof(colors[0]);
    Color c = colors[current_color];
    current_color = (current_color + 1) % count;
    return c;
}

int current_name = 1;

int get_name() {
    return current_name++;
}


typedef struct UIElement {
    Vector2 position;
    Sizing sizing;
    Size size;
    Padding padding;
    Border border;
    Color border_color;
    float child_gap;
    Color bg_color;
} UIElement;

typedef struct UINode {
    int name;
    UIElement value;
    struct UINode *parent;
    struct UINode *first_children;
    struct UINode *next_sibling;
} UINode;

typedef struct NodeAllocator {
    size_t len;
    size_t cap;
    UINode *data;
} NodeAllocator;

NodeAllocator node_allocator_init() {
    NodeAllocator a = {0};
    a.len = 0;
    a.cap = 1024 * 1024;
    a.data = malloc(sizeof(UINode) * a.cap);
    assert(a.data != NULL);
    return a;
}

UINode *node_add(
    NodeAllocator *a,
    UINode *parent,
    UIElement value
) {
    // https://en.wikipedia.org/wiki/Left-child_right-sibling_binary_tree
    assert(a->len + 1 <= a->cap);
    UINode *n = &a->data[a->len++];
    *n = (UINode){.value = value, .name = get_name()};

    if (parent == NULL) {
        return n;
    }

    if (parent->first_children == NULL) {
        parent->first_children = n;
        n->parent = parent;
    } else {
        UINode *c = parent->first_children;
        while (c->next_sibling != NULL) {
            c = c->next_sibling;
        }
        c->next_sibling = n;
        n->parent = parent;
    }
    return n;
}

void node_add_default(NodeAllocator *a, UINode *root) {
    node_add(
        a,
        root,
        (UIElement){
            .sizing = sizing_fixed(50, 50),
            .bg_color = get_color(),
        }
    );
}

void node_print_postorder(UINode *node) {
    if (node == NULL) {
        return;
    }
    if (node->first_children == NULL) {
        printf("%d\n", node->name);
        return;
    }

    UINode *child = node->first_children;
    while (child->next_sibling != NULL) {
        node_print_postorder(child);
        child = child->next_sibling;
    }
    node_print_postorder(child);

    printf("%d\n", node->name);
}

// fini ui framework


UINode *selected = NULL;


UINode *layout_init(NodeAllocator *a) {
    UINode *root = node_add(
        a,
        NULL,
        (UIElement){
            .sizing = sizing_fixed(WINDOW_WIDTH, WINDOW_HEIGHT),
            .padding = padding_all(20),
            .child_gap = 20,
            .bg_color = COLOR_BG,
        }
    );

    node_add(
        a,
        root,
        (UIElement){
            .sizing = sizing_fixed(300, 300),
            .bg_color = get_color(),
        }
    );
    node_add(
        a,
        root,
        (UIElement){
            .sizing = sizing_grow(),
            .bg_color = get_color(),
        }
    );
    node_add(
        a,
        root,
        (UIElement){
            .sizing = sizing_fixed(350, 200),
            .bg_color = get_color(),
        }
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


void draw_rectangle(UINode node, float x, float y, float width, float height) {
    DrawRectangle((int) x, (int) y, (int) width, (int) height, node.value.border_color);
    DrawRectangle(
        (int) (x + node.value.border.left),
        (int) (y + node.value.border.top),
        (int) (width - node.value.border.left - node.value.border.right),
        (int) (height - node.value.border.top - node.value.border.bottom),
        node.value.bg_color
    );
    draw_text(TextFormat("%d", node.name), (Vector2){.x = x, .y = y});
}

void calculate_fit_sizing(UINode *node) {
    // first depth post-order
    assert(node != NULL);

    UIElement *value = &node->value;
    if (value->sizing.x.type == SIZING_TYPE_FIXED) {
        value->size.width = value->sizing.x.value;
    }
    if (value->sizing.y.type == SIZING_TYPE_FIXED) {
        value->size.height = value->sizing.y.value;
    }
    if (value->sizing.x.type == SIZING_TYPE_FIT) {
        value->size.width = 0;
    }
    if (value->sizing.y.type == SIZING_TYPE_FIT) {
        value->size.height = 0;
    }

    // leaf node
    if (node->first_children == NULL && node->parent != NULL) {
        UIElement *parent = &node->parent->value;
        if (parent->sizing.x.type == SIZING_TYPE_FIT) {
            parent->size.width += value->size.width;
        }
        if (parent->sizing.y.type == SIZING_TYPE_FIT) {
            parent->size.height = fmaxf(parent->size.height, value->size.height);
        }
        return;
    }

    // iterate over all children
    UINode *child = node->first_children;
    size_t n_children = 0;
    while (child != NULL) {
        calculate_fit_sizing(child);
        child = child->next_sibling;
        n_children += 1;
    }

    if (value->sizing.x.type == SIZING_TYPE_FIT) {
        float all_child_gaps = (float) (n_children - 1) * value->child_gap;
        value->size.width += all_child_gaps;
        value->size.width += value->padding.left + value->padding.right;
    }
    if (value->sizing.y.type == SIZING_TYPE_FIT) {
        value->size.height += value->padding.top + value->padding.bottom;
    }
}

void calculate_grow_sizing(UINode *node) {
    assert(node != NULL);

    UIElement *value = &node->value;
    float remaining_width = value->size.width - value->padding.left - value->padding.right;
    float remaining_height = node->value.size.height - node->value.padding.top - node->value.padding.bottom;

    // iterate over all children
    UINode *child = node->first_children;
    size_t n_children = 0;
    while (child != NULL) {
        remaining_width -= child->value.size.width;
        child = child->next_sibling;
        n_children += 1;
    }
    if (n_children > 0) {
        float all_child_gaps = (float) (n_children - 1) * value->child_gap;
        remaining_width -= all_child_gaps;

        child = node->first_children;
        while (child != NULL) {
            if (child->value.sizing.x.type == SIZING_TYPE_GROW) {
                child->value.size.width += remaining_width;
            }
            if (child->value.sizing.y.type == SIZING_TYPE_GROW) {
                child->value.size.height += remaining_height - child->value.size.height;
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

void calculate_positions_and_draw(UINode *node, Vector2 click, bool *found, float left_offest) {
    assert(node != NULL);
    draw_rectangle(
        *node,
        node->value.position.x,
        node->value.position.y,
        node->value.size.width,
        node->value.size.height
    );

    if (node->first_children == NULL) {
        // lead node do nothing
        return;
    }

    // iterate over all children
    UINode *c = node->first_children;
    while (c != NULL) {
        float x, y, height, width;
        x = left_offest + c->value.position.x;
        width = c->value.size.width;
        y = node->value.position.y + node->value.padding.top + c->value.position.y;
        height = c->value.size.height;

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
                selected = c;
                *found = true;
            }

            if (c == selected) {
                c->value.border = border_all(3);
                c->value.border_color = BLACK;
            } else {
                c->value.border = border_all(0);
            }
        }

        draw_rectangle(*c, x, y, width, height);
        left_offest += c->value.size.width + node->value.child_gap;

        c = c->next_sibling;
    }
}


void draw_uielement(UINode *parent, Vector2 click) {
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
        parent->value.position.x + parent->value.padding.left
    );

    if (!Vector2Equals(click, Vector2_ZERO) && !found) {
        selected = NULL;
    }
}


void draw_frame(UINode *root, Vector2 click) {
    BeginDrawing();
    ClearBackground(DARKGRAY);

    draw_uielement(root, click);

    // debug size of screen
    int32_t width = GetScreenWidth();
    int32_t height = GetScreenHeight();
    draw_text(
        TextFormat("%d x %d", width, height),
        (Vector2){.x = 20, .y = 0}
    );

    EndDrawing();
}

typedef enum GROW_DIR {
    GROW_DIR_WIDTH,
    GROW_DIR_HEIGHT,
} GROW_DIRECTION;

void change_size(UINode *node, GROW_DIRECTION direction, float amount) {
    if (node == NULL) { return; }
    switch (direction) {
        case GROW_DIR_WIDTH:
            node->value.sizing.x.value = fmaxf(node->value.sizing.x.value + amount, 0);
            break;
        case GROW_DIR_HEIGHT:
            node->value.sizing.y.value = fmaxf(node->value.sizing.y.value + amount, 0);
            break;
    }
}

int main(void) {
    init();

    float amount = 10;
    Vector2 click = Vector2_ZERO;

    NodeAllocator a = node_allocator_init();
    UINode *root = layout_init(&a);

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
            node_add_default(&a, root);
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
            root->value.sizing = sizing_fixed(
                (float) GetScreenWidth(),
                (float) GetScreenHeight()
            );
        }
        draw_frame(root, click);
    }

    fini();
    return 0;
}
