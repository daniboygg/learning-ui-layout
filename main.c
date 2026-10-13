#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"
#include "raymath.h"

// init main raylib functions
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_MIN_WIDTH 320
#define WINDOW_MIN_HEIGHT 240

#define FONT_SIZE 24

Font ui_font;

void init(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "UI Layout");
    SetWindowMinSize(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT);
    SetTargetFPS(60);

    ui_font = LoadFontEx("assets/JetBrainsMono-Bold.ttf", FONT_SIZE, NULL, 0);
}

void fini(void) {
    UnloadFont(ui_font);
    CloseWindow();
}

bool quit_pressed(void) {
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

// fini main raylib functions


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

Color get_color(void) {
    int count = sizeof(colors) / sizeof(colors[0]);
    Color c = colors[current_color];
    current_color = (current_color + 1) % count;
    return c;
}

int current_name = 1;

int get_name(void) {
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

NodeAllocator node_allocator_init(void) {
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
    UIElement value,
    int name
) {
    // https://en.wikipedia.org/wiki/Left-child_right-sibling_binary_tree
    assert(a->len + 1 <= a->cap);
    UINode *n = &a->data[a->len++];
    *n = (UINode){.value = value, .name = name};

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

void node_add_default(NodeAllocator *a, UINode *root, Sizing sizing, Color color, int name) {
    node_add(
        a,
        root,
        (UIElement){
            .sizing = sizing,
            .bg_color = color,
        },
        name
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

// init main app data

typedef struct Input {
    Size screen_size;
    Vector2 mouse;
    bool left_click;
    bool add_box;
    bool remove_box;
} Input;

typedef struct Box {
    Color bg_color;
    Sizing sizing;
} Box;

#define APP_SELECTION_NO (-1)

typedef struct App {
    int selection; // APP_SELECTION_NO empty
    Box boxes[64];
    int boxes_len;
} App;

App app_init() {
    App app = {0};
    return app;
}

int app_add_box(App *app, Box box) {
    assert(app->boxes_len < 64);
    app->boxes[app->boxes_len++] = box;
    return app->boxes_len - 1;
}

void app_pop_box(App *app) {
    assert(app->boxes_len -1 >= 0);
    app->boxes_len -= 1;
}

Box *app_get_box(App *app, int name) {
    return &app->boxes[name];
}

void app_select(App *app, int name) {
    app->selection = name;
}

Box *app_get_selected(App *app) {
    if (app->selection == APP_SELECTION_NO) { return NULL; }
    return &app->boxes[app->selection];
}

bool app_is_selected(App *app, int name) {
    if (app->selection == APP_SELECTION_NO) { return false; }
    return app->selection == name;
}

void app_deselect(App *app) {
    app->selection = APP_SELECTION_NO;
}

// fini main app data


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

void calculate_positions_and_draw(UINode *node, App *app, Input input, bool *found, float left_offest) {
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
            if (input.left_click && CheckCollisionPointRec(
                    input.mouse,
                    (Rectangle){
                        .x = x,
                        .y = y,
                        .width = width,
                        .height = height,
                    })
            ) {
                app_select(app, c->name);
                *found = true;
            }

            if (app_is_selected(app, c->name)) {
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


void draw_uielement(UINode *parent, App *app, Input input) {
    // 1. fit sizing
    calculate_fit_sizing(parent);

    // 2. grow sizing
    calculate_grow_sizing(parent);

    // 3. calculate positions and draw
    bool found = false;
    calculate_positions_and_draw(
        parent,
        app,
        input,
        &found,
        parent->value.position.x + parent->value.padding.left
    );

    if (input.left_click && !found) {
        app_deselect(app);
    }
}


void draw_frame(UINode *root, App *app, Input input) {
    BeginDrawing();
    ClearBackground(DARKGRAY);

    draw_uielement(root, app, input);

    // debug size of screen
    draw_text(
        TextFormat("%d x %d", (int) input.screen_size.width, (int) input.screen_size.height),
        (Vector2){.x = 20, .y = 0}
    );

    EndDrawing();
}

typedef enum GROW_DIR {
    GROW_DIR_WIDTH,
    GROW_DIR_HEIGHT,
} GROW_DIRECTION;

void change_size(App *app, GROW_DIRECTION direction, float amount) {
    Box *box = app_get_selected(app);
    if (box == NULL) { return; }

    switch (direction) {
        case GROW_DIR_WIDTH:
            box->sizing.x.value = fmaxf(box->sizing.x.value + amount, 0);
            break;
        case GROW_DIR_HEIGHT:
            box->sizing.y.value = fmaxf(box->sizing.y.value + amount, 0);
            break;
    }
}

int main(void) {
    float amount = 10;
    NodeAllocator a = node_allocator_init();
    App app = {0};

    int container_name = app_add_box(&app, (Box){.bg_color = COLOR_BG});
    int b1 = app_add_box(&app, (Box){.bg_color = get_color(), .sizing = sizing_fixed(300, 300)});
    int b2 = app_add_box(&app, (Box){.bg_color = get_color(), .sizing = sizing_grow()});
    int b3 = app_add_box(&app, (Box){.bg_color = get_color(), .sizing = sizing_fixed(350, 200)});

    init();

    while (!quit_pressed()) {
        Input input = {0};

        // resize selected node
        input.mouse = GetMousePosition();
        input.left_click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        input.screen_size = (Size){.width = (float) GetScreenWidth(), .height = (float) GetScreenHeight()};

        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
            change_size(&app, GROW_DIR_WIDTH, amount);
        }
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
            change_size(&app, GROW_DIR_WIDTH, -amount);
        }
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
            change_size(&app, GROW_DIR_HEIGHT, -amount);
        }
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
            change_size(&app, GROW_DIR_HEIGHT, amount);
        }

        input.add_box = IsKeyPressed(KEY_F);
        input.remove_box = IsKeyPressed(KEY_G);

        // update elements
        if (input.add_box) {
            app_add_box(&app, (Box){.bg_color = get_color(), .sizing = sizing_fixed(50, 50),});
        }
        if (input.remove_box && app.boxes_len > 4) {
            app_pop_box(&app);
        }

        // define layout
        UINode *root = node_add(
            &a,
            NULL,
            (UIElement){
                .sizing = sizing_fixed(input.screen_size.width, input.screen_size.height),
                .padding = padding_all(20),
                .child_gap = 20,
                .bg_color = app_get_box(&app, container_name)->bg_color,
            },
            container_name
        );
        node_add(
            &a,
            root,
            (UIElement){
                .sizing = app_get_box(&app, b1)->sizing,
                .bg_color = app_get_box(&app, b1)->bg_color
            },
            b1
        );
        node_add(
            &a,
            root,
            (UIElement){
                .sizing = app_get_box(&app, b2)->sizing,
                .bg_color = app_get_box(&app, b2)->bg_color
            },
            b2
        );
        node_add(
            &a,
            root,
            (UIElement){
                .sizing = app_get_box(&app, b3)->sizing,
                .bg_color = app_get_box(&app, b3)->bg_color
            },
            b3
        );
        if (app.boxes_len > 4) {
            for (int i = 4; i < app.boxes_len; i++) {
                node_add_default(&a, root, app_get_box(&app, i)->sizing, app_get_box(&app, i)->bg_color, i);
            }
        }

        draw_frame(root, &app, input);

        // reset arena on each frame
        a.len = 0;
    }

    fini();
    return 0;
}
