/*
 * glome - tesselated octahedron demo using a nanovg 3D canvas.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

#include "nanovg.h"
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "linmath.h"

#include "lv_color.h"
#include "lv_opengl.h"
#include "lv_vg.h"

typedef struct glome_app glome_app;
struct glome_app
{
    GLFWwindow* window;
    lv_context* ctx_nanovg;
    lv_context* ctx_buffer2d;
    lv_context* ctx_buffer3d;
    lv_context* ctx_xform;
    vec3 rotation;
    vec2f mouse;
    vec2f origin;
    float zoom;
    vec2f last_mouse;
    float last_zoom;
};

static const float min_zoom = 2.0f, max_zoom = 2048.0f;

static int opt_help = 0;
static int opt_animated = 1;
static float opt_zoom = 56.0f;
static float opt_rot = 0.1f;
static float opt_stroke = 2.0f;
static int opt_depth = 2;
static int opt_depth_step = 0;
static int opt_width = 1280;
static int opt_height = 720;
static int opt_frames = 1;
static int opt_inverse = 1;
static const char* opt_imagepath;

static int frame;

static lv_color blue;
static lv_color orange;
static lv_color green;
static lv_color red;
static lv_color purple;
static lv_color brown;
static lv_color pink;
static lv_color grey;
static lv_color olive;
static lv_color turquoise;
static lv_color yellow;
static lv_color white;
static lv_color charcoal;

static void lv_vg_uinit(glome_app* app)
{
    app->ctx_nanovg = (lv_context*)calloc(1, sizeof(lv_context));
    app->ctx_buffer2d = (lv_context*)calloc(1, sizeof(lv_context));
    app->ctx_buffer3d = (lv_context*)calloc(1, sizeof(lv_context));
    app->ctx_xform = (lv_context*)calloc(1, sizeof(lv_context));
    lv_vg_init(app->ctx_nanovg, &lv_nanovg_vg_ops, NULL);
    lv_vg_init(app->ctx_buffer2d, &lv_buffer_vg_ops, NULL);
    lv_vg_init(app->ctx_buffer3d, &lv_buffer_vg_ops, NULL);
    lv_vg_init(app->ctx_xform, &lv_xform_vg_ops, app->ctx_nanovg);
}

static void lv_vg_udestroy(glome_app* app)
{
    lv_vg_destroy(app->ctx_nanovg);
    lv_vg_destroy(app->ctx_buffer2d);
    lv_vg_destroy(app->ctx_buffer3d);
    lv_vg_destroy(app->ctx_xform);
    free(app->ctx_nanovg);
    free(app->ctx_buffer2d);
    free(app->ctx_buffer3d);
    free(app->ctx_xform);
}

static void lv_init_colors()
{
    blue = lv_rgb(0x1f,0x77,0xb4);
    orange = lv_rgb(0xff,0x7f,0x0e);
    green = lv_rgb(0x2c,0xa0,0x2c);
    red = lv_rgb(0xd6,0x27,0x28);
    purple = lv_rgb(0x94,0x67,0xbd);
    brown = lv_rgb(0x8c,0x56,0x4b);
    pink = lv_rgb(0xe3,0x77,0xc2);
    grey = lv_rgb(0x7f,0x7f,0x7f);
    olive = lv_rgb(0xbc,0xbd,0x22);
    turquoise = lv_rgb(0x17,0xbe,0xcf);
    yellow = lv_rgb(0xff,0xc0,0x00);
    white = lv_rgb(0xd0,0xd0,0xd0);
    charcoal = lv_rgb(0x20,0x20,0x20);
}

static float deg_rad(float a) { return a * M_PI / 180.0f; }

static void model_matrix_transform(mat4x4 m, vec3 scale, vec3 trans, vec3 rot)
{
    mat4x4_identity(m);
    mat4x4_scale_aniso(m, m, scale[0], scale[1], scale[2]);
    mat4x4_translate_in_place(m, trans[0], trans[1], trans[2]);
    mat4x4_rotate_X(m, m, deg_rad(rot[0]));
    mat4x4_rotate_Y(m, m, deg_rad(rot[1]));
    mat4x4_rotate_Z(m, m, deg_rad(rot[2]));
}

typedef enum {
    subd_octa_2d,
    subd_octa_3d,
} subd_type;

static void subdivide_triangle(lv_context *ctx, int depth, int max_depth,
    const vec3f vert[3], const vec2f uv[3], float r, subd_type type)
{
    if (depth > 0)
    {
        vec3f vert012 = {
            (vert[0].x + vert[1].x + vert[2].x) / 3.f,
            (vert[0].y + vert[1].y + vert[2].y) / 3.f,
            (vert[0].z + vert[1].z + vert[2].z) / 3.f
        };
        vec3f vert01 = {
            vert[0].x * 0.5f + vert[1].x * 0.5f,
            vert[0].y * 0.5f + vert[1].y * 0.5f,
            vert[0].z * 0.5f + vert[1].z * 0.5f
        };
        vec3f vert12 = {
            vert[1].x * 0.5f + vert[2].x * 0.5f,
            vert[1].y * 0.5f + vert[2].y * 0.5f,
            vert[1].z * 0.5f + vert[2].z * 0.5f
        };
        vec3f vert20 = {
            vert[2].x * 0.5f + vert[0].x * 0.5f,
            vert[2].y * 0.5f + vert[0].y * 0.5f,
            vert[2].z * 0.5f + vert[0].z * 0.5f
        };

        vec2f uv012 = {
            (uv[0].x + uv[1].x + uv[2].x) / 3.f,
            (uv[0].y + uv[1].y + uv[2].y) / 3.f,
        };
        vec2f uv01 = {
            uv[0].x * 0.5f + uv[1].x * 0.5f,
            uv[0].y * 0.5f + uv[1].y * 0.5f
        };
        vec2f uv12 = {
            uv[1].x * 0.5f + uv[2].x * 0.5f,
            uv[1].y * 0.5f + uv[2].y * 0.5f
        };
        vec2f uv20 = {
            uv[2].x * 0.5f + uv[0].x * 0.5f,
            uv[2].y * 0.5f + uv[0].y * 0.5f
        };

        const vec3f verta[3] = { vert[0], vert01, vert20 };
        const vec2f uva[3] = { uv[0], uv01, uv20 };
        subdivide_triangle(ctx, depth-1, max_depth, verta, uva, r, type);

        const vec3f vertb[3] = { vert01, vert[1], vert12 };
        const vec2f uvb[3] = { uv01, uv[1], uv12 };
        subdivide_triangle(ctx, depth-1, max_depth, vertb, uvb, r, type);

        const vec3f vertc[3] = { vert20, vert12, vert[2] };
        const vec2f uvc[3] = { uv20, uv12, uv[2] };
        subdivide_triangle(ctx, depth-1, max_depth, vertc, uvc, r, type);

        const vec3f vertd[3] = { vert01, vert12, vert20 };
        const vec2f uvd[3] = { uv01, uv12, uv20 };
        subdivide_triangle(ctx, depth-1, max_depth, vertd, uvd, r, type);
    }

    if (depth == 0 || type == subd_octa_2d)
    {
        float w, c;

        if (type == subd_octa_2d) {
            w = expf((4-max_depth + depth) * logf(1.7f)) * 0.5f * opt_stroke;
            c = (opt_inverse ? 1.f - 4.f * 0.1f : 0.f) +
                (3 - max_depth + depth) * 0.1f;
        } else {
            w = opt_stroke;
            c = opt_inverse ? 1.f : 0.f;
        }

        lv_vg_stroke_width(ctx, w);
        lv_vg_line_cap(ctx, lv_round);
        lv_vg_line_join(ctx, lv_round);
        lv_vg_fill_color(ctx, lv_rgbaf(uv[0].x, uv[1].y, uv[2].x, 0.25f));
        lv_vg_stroke_color(ctx, lv_rgbaf(c, c, c, 1.f));

        lv_vg_begin_path(ctx);
        switch(type) {
        case subd_octa_2d:
            for (int j = 0; j < 3; j++) {
                vec2f v = {
                    (uv[j].x * 2.0f - 1.0f) * r,
                    (uv[j].y * 2.0f - 1.0f) * r
                };
                if (j == 0) {
                    lv_vg_2d_move_to(ctx, v);
                } else {
                    lv_vg_2d_line_to(ctx, v);
                }
            }
            break;
        case subd_octa_3d:
            for (int j = 0; j < 3; j++) {
                vec3f v = vec3f_normalize(vert[j]);
                v.x *= r;
                v.y *= r;
                v.z *= r;
                if (j == 0) {
                    lv_vg_3d_move_to(ctx, v);
                } else {
                    lv_vg_3d_line_to(ctx, v);
                }
            }
            break;
        }
        lv_vg_close_path(ctx);
        lv_vg_fill(ctx);
        lv_vg_stroke(ctx);
    }
}

static void lv_render(glome_app* app, float w, float h, float r)
{
    static float s = 0.05f;
    static float m = 0.f;
    static float n = 0.f;

    mat4x4 m_model2d, m_model3d, m_proj, m_final2d, m_final3d;
    vec3 m_scale = { 1.0f, 1.0f, 1.0f };
    vec3 m_trans = { 0.0f, 0.0f, app->zoom };
    vec2f origin = { app->origin.x, app->origin.y };
    lv_context* ctx;

    ctx = app->ctx_buffer3d;

    float sw = expf(4.f * logf(1.5f)) * 0.5f * opt_stroke;

    lv_buffer_vg_clear(ctx);
    lv_vg_stroke_width(ctx, sw);

    if (opt_inverse) {
        lv_vg_fill_color(ctx, lv_rgbaf(1.f, 1.f, 1.f, 0.1f));
        lv_vg_stroke_color(ctx, lv_rgbaf(1.f, 1.f, 1.f, 1.f));
    } else {
        lv_vg_fill_color(ctx, lv_rgbaf(0.f, 0.f, 0.f, 0.1f));
        lv_vg_stroke_color(ctx, lv_rgbaf(0.f, 0.f, 0.f, 1.f));
    }

    lv_vg_begin_path(ctx);
    lv_vg_3d_move_to(ctx, lv_point_3d(-10.f, 10.f, 10.f));
    lv_vg_3d_line_to(ctx, lv_point_3d(-10.f,-10.f, 10.f));
    lv_vg_3d_line_to(ctx, lv_point_3d( 10.f,-10.f, 10.f));
    lv_vg_3d_line_to(ctx, lv_point_3d( 10.f,-10.f,-10.f));
    lv_vg_3d_line_to(ctx, lv_point_3d(-10.f,-10.f,-10.f));
    lv_vg_3d_line_to(ctx, lv_point_3d(-10.f,-10.f, 10.f));
    lv_vg_3d_line_to(ctx, lv_point_3d(-10.f, 10.f, 10.f));
    lv_vg_3d_line_to(ctx, lv_point_3d(-10.f, 10.f,-10.f));
    lv_vg_3d_line_to(ctx, lv_point_3d(-10.f,-10.f,-10.f));
    lv_vg_3d_line_to(ctx, lv_point_3d( 10.f,-10.f,-10.f));
    lv_vg_3d_line_to(ctx, lv_point_3d( 10.f, 10.f,-10.f));
    lv_vg_3d_line_to(ctx, lv_point_3d(-10.f, 10.f,-10.f));
    lv_vg_3d_line_to(ctx, lv_point_3d(-10.f, 10.f, 10.f));
    lv_vg_3d_line_to(ctx, lv_point_3d( 10.f, 10.f, 10.f));
    lv_vg_3d_line_to(ctx, lv_point_3d( 10.f, 10.f,-10.f));
    lv_vg_3d_line_to(ctx, lv_point_3d( 10.f,-10.f,-10.f));
    lv_vg_3d_line_to(ctx, lv_point_3d( 10.f,-10.f, 10.f));
    lv_vg_3d_line_to(ctx, lv_point_3d( 10.f, 10.f, 10.f));
    lv_vg_3d_line_to(ctx, lv_point_3d(-10.f, 10.f, 10.f));
    lv_vg_close_path(ctx);
    lv_vg_stroke(ctx);

    const struct { const vec3f vert[3]; } vertices[] = {
        { (vec3f) {  0.f, 0.f,-1.f, }, (vec3f) { -1.f, 0.f, 0.f, }, (vec3f) {  0.f, 1.f, 0.f, } },
        { (vec3f) {  0.f, 0.f,-1.f, }, (vec3f) {  0.f, 1.f, 0.f, }, (vec3f) {  1.f, 0.f, 0.f, } },
        { (vec3f) {  0.f, 0.f,-1.f, }, (vec3f) {  1.f, 0.f, 0.f, }, (vec3f) {  0.f,-1.f, 0.f, } },
        { (vec3f) {  0.f, 0.f,-1.f, }, (vec3f) {  0.f,-1.f, 0.f, }, (vec3f) { -1.f, 0.f, 0.f, } },
        { (vec3f) {  0.f, 0.f, 1.f, }, (vec3f) {  0.f, 1.f, 0.f, }, (vec3f) { -1.f, 0.f, 0.f, } },
        { (vec3f) {  0.f, 0.f, 1.f, }, (vec3f) {  1.f, 0.f, 0.f, }, (vec3f) {  0.f, 1.f, 0.f, } },
        { (vec3f) {  0.f, 0.f, 1.f, }, (vec3f) {  0.f,-1.f, 0.f, }, (vec3f) {  1.f, 0.f, 0.f, } },
        { (vec3f) {  0.f, 0.f, 1.f, }, (vec3f) { -1.f, 0.f, 0.f, }, (vec3f) {  0.f,-1.f, 0.f, } },
    };

    const struct { const vec2f uv[3]; } uvs[] = {
        { (vec2f) { 0.5f, 0.5f }, (vec2f) { 0.0f, 0.5f }, (vec2f) { 0.5f, 0.0f } },
        { (vec2f) { 0.5f, 0.5f }, (vec2f) { 0.5f, 0.0f }, (vec2f) { 1.0f, 0.5f } },
        { (vec2f) { 0.5f, 0.5f }, (vec2f) { 1.0f, 0.5f }, (vec2f) { 0.5f, 1.0f } },
        { (vec2f) { 0.5f, 0.5f }, (vec2f) { 0.5f, 1.0f }, (vec2f) { 0.0f, 0.5f } },
        { (vec2f) { 0.0f, 0.0f }, (vec2f) { 0.5f, 0.0f }, (vec2f) { 0.0f, 0.5f } },
        { (vec2f) { 1.0f, 0.0f }, (vec2f) { 1.0f, 0.5f }, (vec2f) { 0.5f, 0.0f } },
        { (vec2f) { 1.0f, 1.0f }, (vec2f) { 0.5f, 1.0f }, (vec2f) { 1.0f, 0.5f } },
        { (vec2f) { 0.0f, 1.0f }, (vec2f) { 0.0f, 0.5f }, (vec2f) { 0.5f, 1.0f } },
    };

    for (int i = 0; i < 8; i++) {
        subdivide_triangle(ctx, opt_depth, opt_depth,
            vertices[i].vert, uvs[i].uv, 7.5f, subd_octa_3d);
    }

    ctx = app->ctx_buffer2d;

    lv_buffer_vg_clear(ctx);

    for (int i = 0; i < 8; i++) {
        subdivide_triangle(ctx, opt_depth, opt_depth,
            vertices[i].vert, uvs[i].uv, 7.5f, subd_octa_2d);
    }

    mat4x4_identity(m_model2d);
    mat4x4_scale_aniso(m_model2d, m_model2d, m_scale[0], m_scale[1], m_scale[2]);
    mat4x4_translate_in_place(m_model2d, m_trans[0], m_trans[1], m_trans[2]);
    mat4x4_perspective(m_proj, deg_rad(30.f), r, 1.f, 1e6f);
    mat4x4_mul(m_final2d, m_proj, m_model2d);
    mat4x4_translate_in_place(m_final2d, origin.x*s - 16.f, origin.y*s, 0.f);
    mat4x4_rotate_Y(m_final2d, m_final2d, deg_rad(n));

    mat4x4_identity(m_model3d);
    mat4x4_scale_aniso(m_model3d, m_model3d, m_scale[0], m_scale[1], m_scale[2]);
    mat4x4_translate_in_place(m_model3d, m_trans[0], m_trans[1], m_trans[2]);
    mat4x4_perspective(m_proj, deg_rad(30.f), r, 1.f, 1e6f);
    mat4x4_mul(m_final3d, m_proj, m_model3d);
    mat4x4_translate_in_place(m_final3d, origin.x*s + 8.f, origin.y*s, 0.f);
    mat4x4_rotate_Y(m_final3d, m_final3d, deg_rad(m));

    ctx = app->ctx_xform;

    lv_vg_begin_frame(ctx, w, h, r);
    lv_vg_reset(ctx);
    lv_vg_push(ctx);
    lv_xform_proj_matrix(app->ctx_xform, m_final3d, 1);
    lv_buffer_vg_playback(app->ctx_buffer3d, ctx);
    lv_vg_pop(ctx);
    lv_vg_push(ctx);
    lv_xform_proj_matrix(app->ctx_xform, m_final2d, 1);
    lv_buffer_vg_playback(app->ctx_buffer2d, ctx);
    lv_vg_pop(ctx);
    lv_vg_end_frame(ctx);

    if (opt_animated) {
        m += opt_rot;
    }
    if (opt_depth_step) {
        if ((frame % opt_depth_step) == 0) opt_depth++;
    }
}

static void key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    NVG_NOTUSED(scancode);
    NVG_NOTUSED(mods);

    glome_app *ctx = (glome_app*)glfwGetWindowUserPointer(window);

    if( action != GLFW_PRESS ) return;

    float shiftz = (mods & GLFW_MOD_SHIFT ? -1.f : 1.f);

    switch (key) {
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
    case GLFW_KEY_X: opt_animated = !opt_animated; break;
    case GLFW_KEY_Z: ctx->rotation[2] += 5.f * shiftz; break;
    case GLFW_KEY_C: ctx->zoom += 5.f * shiftz; break;
    case GLFW_KEY_W: ctx->rotation[0] += 5.f; break;
    case GLFW_KEY_S: ctx->rotation[0] -= 5.f; break;
    case GLFW_KEY_A: ctx->rotation[1] += 5.f; break;
    case GLFW_KEY_D: ctx->rotation[1] -= 5.f; break;
    case GLFW_KEY_J: if (opt_depth < 8) opt_depth++; break;
    case GLFW_KEY_K: if (opt_depth > 0) opt_depth--; break;
    default: return;
    }
}

static void scroll(GLFWwindow* window, double xoffset, double yoffset)
{
    glome_app *ctx = (glome_app*)glfwGetWindowUserPointer(window);

    lv_trace("scroll\n");

    float quantum = ctx->zoom / 16.f;
    float ratio = 1.f + (float)quantum / (float)ctx->zoom;
    if (yoffset < 0. && ctx->zoom < max_zoom) {
        ctx->origin.x *= ratio;
        ctx->origin.y *= ratio;
        ctx->zoom += quantum;
    } else if (yoffset > 0. && ctx->zoom > min_zoom) {
        ctx->origin.x /= ratio;
        ctx->origin.y /= ratio;
        ctx->zoom -= quantum;
    }
}

static int mouse_left_drag;
static int mouse_right_drag;

static void mouse_button(GLFWwindow* window, int button, int action, int mods)
{
    glome_app *ctx = (glome_app*)glfwGetWindowUserPointer(window);

    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
        mouse_left_drag = (action == GLFW_PRESS);
        ctx->last_mouse = ctx->mouse;
        ctx->last_zoom = ctx->zoom;
        break;
    case GLFW_MOUSE_BUTTON_RIGHT:
        mouse_right_drag = (action == GLFW_PRESS);
        ctx->last_mouse = ctx->mouse;
        ctx->last_zoom = ctx->zoom;
        break;
    }
}

static void cursor_position(GLFWwindow* window, double xpos, double ypos)
{
    glome_app *ctx = (glome_app*)glfwGetWindowUserPointer(window);

    ctx->mouse = (vec2f) { xpos, ypos };

    if (mouse_left_drag) {
        ctx->origin.x += ctx->mouse.x - ctx->last_mouse.x;
        ctx->origin.y += ctx->mouse.y - ctx->last_mouse.y;
        ctx->last_mouse = ctx->mouse;
    }
    if (mouse_right_drag) {
        float delta0 = ctx->mouse.x - ctx->last_mouse.x;
        float delta1 = ctx->mouse.y - ctx->last_mouse.y;
        float zoom = ctx->last_zoom * powf(65.0f/64.0f,(float)-delta1);
        if (zoom != ctx->zoom && zoom > min_zoom && zoom < max_zoom) {
            ctx->zoom = zoom;
            ctx->origin.x = (ctx->origin.x * (zoom / ctx->zoom));
            ctx->origin.y = (ctx->origin.y * (zoom / ctx->zoom));
        }
    }
}


static void image_set_alpha(uchar* image, int w, int h, int stride, uchar a)
{
    int x, y;
    for (y = 0; y < h; y++) {
        uchar* row = &image[y*stride];
        for (x = 0; x < w; x++)
            row[x*4+3] = a;
    }
}

static void image_flip_horiz(uchar* image, int w, int h, int stride)
{
    int i = 0, j = h-1, k;
    while (i < j) {
        uchar* ri = &image[i * stride];
        uchar* rj = &image[j * stride];
        for (k = 0; k < w*4; k++) {
            uchar t = ri[k];
            ri[k] = rj[k];
            rj[k] = t;
        }
        i++;
        j--;
    }
}

static void lv_save_screenshot(int w, int h, const char* filename)
{
    uchar* image = (uchar*)malloc(w*h*4);
    if (image == NULL) return;
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, image);
    image_set_alpha(image, w, h, w*4, 255);
    image_flip_horiz(image, w, h, w*4);
    stbi_write_png(filename, w, h, 4, image, w*4);
    free(image);
}

static void lv_main_loop(GLFWwindow* window, glome_app *ctx)
{
    while (!glfwWindowShouldClose(window))
    {
        int winWidth, winHeight;
        int fbWidth, fbHeight;
        float pxRatio;

        glfwGetWindowSize(window, &winWidth, &winHeight);
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        pxRatio = (float)fbWidth / (float)fbHeight;

        glViewport(0, 0, fbWidth, fbHeight);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

        lv_render(ctx, winWidth, winHeight, pxRatio);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

static void lv_batch_loop(GLFWwindow* window, glome_app *ctx)
{
    for (frame = 1; frame <= opt_frames; frame++)
    {
        int winWidth, winHeight;
        int fbWidth, fbHeight;
        float pxRatio;

        static char filename[1024];
        snprintf(filename, sizeof(filename), opt_imagepath, frame);

        glfwGetWindowSize(window, &winWidth, &winHeight);
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        pxRatio = (float)fbWidth / (float)fbHeight;

        glViewport(0, 0, fbWidth, fbHeight);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

        lv_render(ctx, winWidth, winHeight, pxRatio);

        lv_save_screenshot(fbWidth, fbHeight, filename);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

/*
 * option processing
 */

static void print_help(int argc, char **argv)
{
    lv_info(
        "usage: %s [options]\n"
        "\n"
        "Options:\n"
        "  -l, (info|debug|trace)             debug level\n"
        "  -na, --no-animation                disable animation\n"
        "  -ni, --no-inverse                  disable inverse\n"
        "  -z, --zoom <float>                 buffer zoom\n"
        "  -w, --width <int>                  buffer width\n"
        "  -h, --height <int>                 buffer height\n"
        "  -sw, --stroke-width <float>        stroke width\n"
        "  -sd, --subd-depth <int>            subdivision depth\n"
        "  -ss, --subd-step <int>             subdivision step\n"
        "  -c, --frame-count <int>            frame count\n"
        "  -r, --rotation-step <float>        rotation step\n"
        "  -bt, --batch-tmpl <pngfile>        write image\n"
        "  -h, --help                         command line help\n",
        argv[0]
    );
}

static int match_opt(const char *arg, const char *opt, const char *longopt)
{
    return strcmp(arg, opt) == 0 || strcmp(arg, longopt) == 0;
}

static void parse_options(int argc, char **argv)
{
    int i = 1;
    while (i < argc) {
        if (match_opt(argv[i], "-h", "--help")) {
            opt_help++;
            i++;
        } else if (match_opt(argv[i], "-l", "--level")) {
            char* level = argv[++i];
            if (strcmp(level, "none") == 0) {
                lv_ll = lv_ll_none;
            } else if (strcmp(level, "info") == 0) {
                lv_ll = lv_ll_info;
            } else if (strcmp(level, "debug") == 0) {
                lv_ll = lv_ll_debug;
            } else if (strcmp(level, "trace") == 0) {
                lv_ll = lv_ll_trace;
            }
            i++;
        } else if (match_opt(argv[i], "-na", "--no-animation")) {
            opt_animated = 0;
            i++;
        } else if (match_opt(argv[i], "-ni", "--no-inverse")) {
            opt_inverse = 0;
            i++;
        } else if (match_opt(argv[i], "-z", "--zoom")) {
            opt_zoom = (float)atof(argv[++i]);
            i++;
        } else if (match_opt(argv[i], "-w", "--width")) {
            opt_width = atoi(argv[++i]);
            i++;
        } else if (match_opt(argv[i], "-h", "--height")) {
            opt_height = atoi(argv[++i]);
            i++;
        } else if (match_opt(argv[i], "-sw", "--stroke-width")) {
            opt_stroke = (float)atof(argv[++i]);
            i++;
        } else if (match_opt(argv[i], "-sd", "--subd-depth")) {
            opt_depth = atoi(argv[++i]);
            i++;
        } else if (match_opt(argv[i], "-ss", "--subd-step")) {
            opt_depth_step = atoi(argv[++i]);
            i++;
        } else if (match_opt(argv[i], "-c", "--frame-count")) {
            opt_frames = atoi(argv[++i]);
            i++;
        } else if (match_opt(argv[i], "-r", "--rotation-step")) {
            opt_rot = atof(argv[++i]);
            i++;
        } else if (match_opt(argv[i], "-bt", "--batch-tmpl")) {
            opt_imagepath = argv[++i];
            i++;
        } else {
            lv_error("error: unknown option: %s\n", argv[i]);
            opt_help++;
            break;
        }
    }

    if (opt_help) {
        print_help(argc, argv);
        exit(1);
    }
}

static void errorcb(int error, const char* desc)
{
    lv_error("GLFW error %d: %s\n", error, desc);
}

static void glome(int argc, char **argv)
{
    GLFWwindow* window;
    glome_app ctx;

    memset(&ctx, 0, sizeof(ctx));
    ctx.zoom = opt_zoom;

    if (!glfwInit()) {
        lv_panic("glfwInit failed\n");
    }

    glfwSetErrorCallback(errorcb);

    glfwWindowHint(GLFW_SCALE_TO_MONITOR , GL_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    if (opt_imagepath) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }

    ctx.window = window = glfwCreateWindow(opt_width, opt_height,
        "glome", NULL, NULL);
    if (!window) {
        lv_panic("glfwCreateWindow failed\n");
    }

    glfwSetWindowUserPointer(window, &ctx);
    glfwSetKeyCallback(window, key);
    glfwSetScrollCallback(window, scroll);
    glfwSetMouseButtonCallback(window, mouse_button);
    glfwSetCursorPosCallback(window, cursor_position);
    glfwMakeContextCurrent(window);
    gladLoadGL();
    glfwSwapInterval(0);
    glfwSetTime(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    if (opt_inverse) {
        glClearColor(0.f, 0.f, 0.f, 1.f);
    } else {
        glClearColor(1.f, 1.f, 1.f, 1.f);
    }

    lv_init_colors();
    lv_vg_uinit(&ctx);

    if (opt_imagepath) {
        lv_batch_loop(window, &ctx);
    } else {
        lv_main_loop(window, &ctx);
    }

    lv_vg_udestroy(&ctx);
    glfwTerminate();
}

/*
 * main program
 */

int main(int argc, char **argv)
{
    lv_ll = lv_ll_info;
    parse_options(argc, argv);
    glome(argc, argv);
    return 0;
}
