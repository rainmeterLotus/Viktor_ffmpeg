//
// Created by rainmeterLotus on 2021/8/3.
//

#include "ff_renderer.h"
#include "ff_egl_renderer_yuv420p.h"
#include "ff_egl_renderer_rgb.h"

GLuint GLES2_loadShader(GLenum shader_type, const char *shader_source) {
    GLuint shader = glCreateShader(shader_type);
    if (shader == 0) {
        return 0;
    }

    glShaderSource(shader, 1, &shader_source, nullptr);
    glCompileShader(shader);
    GLint compile_status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
    if (compile_status == 0) {
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLboolean GLES2_Renderer_setupGLES() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_DEPTH_TEST);
    return GL_TRUE;
}

void GLES2_Renderer_TexCoords_cropRight(EGL_Renderer *renderer, GLfloat cropRight) {
    renderer->texcoords[0] = 0.0f;
    renderer->texcoords[1] = 1.0f;
    renderer->texcoords[2] = 1.0f;
    renderer->texcoords[3] = 1.0f - cropRight;
    renderer->texcoords[4] = 0.0f;
    renderer->texcoords[5] = 0.0f;
    renderer->texcoords[6] = 1.0f - cropRight;
    renderer->texcoords[7] = 0.0f;
}

/**
 // 顶点坐标
val VERTEX_COORDINATE = floatArrayOf(
    -1f, -1f, 1f, -1f, -1f, 1f, 1f, 1f
)

// 纹理坐标
val TEXTURE_COORDINATE = floatArrayOf(
    0f, 0f, 1f, 0f, 0f, 1f, 1f, 1f
)

// 纹理坐标，反向
val TEXTURE_COORDINATE_REVERSE = floatArrayOf(
    0f, 1f, 1f, 1f, 0f, 0f, 1f, 0f
)
 * @param renderer
 */
void GLES2_Renderer_TexCoords_reset(EGL_Renderer *renderer) {
    VIKTOR_LOGI("GLES2_Renderer_TexCoords_reset");
    renderer->texcoords[0] = 0.0f;
    renderer->texcoords[1] = 1.0f;

    renderer->texcoords[2] = 1.0f;
    renderer->texcoords[3] = 1.0f;

    renderer->texcoords[4] = 0.0f;
    renderer->texcoords[5] = 0.0f;

    renderer->texcoords[6] = 1.0f;
    renderer->texcoords[7] = 0.0f;

//    renderer->texcoords[0] = 0.0f;
//    renderer->texcoords[1] = 0.0f;
//
//    renderer->texcoords[2] = 1.0f;
//    renderer->texcoords[3] = 0.0f;
//
//    renderer->texcoords[4] = 0.0f;
//    renderer->texcoords[5] = 1.0f;
//
//    renderer->texcoords[6] = 1.0f;
//    renderer->texcoords[7] = 1.0f;
}

void GLES2_Renderer_Vertices_reset(EGL_Renderer *renderer) {
    VIKTOR_LOGI("GLES2_Renderer_Vertices_reset");
    renderer->vertices[0] = -1.0f;
    renderer->vertices[1] = -1.0f;

    renderer->vertices[2] = 1.0f;
    renderer->vertices[3] = -1.0f;

    renderer->vertices[4] = -1.0f;
    renderer->vertices[5] = 1.0f;

    renderer->vertices[6] = 1.0f;
    renderer->vertices[7] = 1.0f;

//    renderer->vertices[0] = -1.0f;
//    renderer->vertices[1] = 1.0f;
//    renderer->vertices[2] = -1.0f;
//    renderer->vertices[3] = -1.0f;
//    renderer->vertices[4] = 1.0f;
//    renderer->vertices[5] = 1.0f;
//    renderer->vertices[6] = 1.0f;
//    renderer->vertices[7] = -1.0f;
}

void GLES2_Renderer_Vertices_apply(EGL_Renderer *renderer) {
    VIKTOR_LOGI("GLES2_Renderer_Vertices_apply");
    if (!renderer) return;
    switch (renderer->gravity) {
        case GLES2_GRAVITY_RESIZE_ASPECT:
            break;
        case GLES2_GRAVITY_RESIZE_ASPECT_FILL:
            break;
        case GLES2_GRAVITY_RESIZE:
            GLES2_Renderer_Vertices_reset(renderer);
            return;
        default:
            GLES2_Renderer_Vertices_reset(renderer);
            return;
    }
    VIKTOR_LOGI("GLES2_Renderer_Vertices_apply 00");
    if (renderer->layer_width <= 0 ||
        renderer->layer_height <= 0 ||
        renderer->frame_width <= 0 ||
        renderer->frame_height <= 0) {
        VIKTOR_LOGI("[GLES2] invalid width/height for gravity aspect\n");
        GLES2_Renderer_Vertices_reset(renderer);
        return;
    }

    float width = renderer->frame_width;
    float height = renderer->frame_height;
    VIKTOR_LOGI("GLES2_Renderer_Vertices_apply 11");
    if (renderer->frame_sar_num > 0 && renderer->frame_sar_den > 0) {
        width = width * renderer->frame_sar_num / renderer->frame_sar_den;
    }

    const float dW = (float) renderer->layer_width / width;
    const float dH = (float) renderer->layer_height / height;
    float dd = 1.0f;
    float nW = 1.0f;
    float nH = 1.0f;
    VIKTOR_LOGI("GLES2_Renderer_Vertices_apply 22");
    switch (renderer->gravity) {
        case GLES2_GRAVITY_RESIZE_ASPECT_FILL:
            dd = FFMAX(dW, dH);
            break;
        case GLES2_GRAVITY_RESIZE_ASPECT:
            dd = FFMIN(dW, dH);
            break;
    }

    nW = (width * dd / (float) renderer->layer_width);
    nH = (height * dd / (float) renderer->layer_height);

    renderer->vertices[0] = -nW;
    renderer->vertices[1] = -nH;
    renderer->vertices[2] = nW;
    renderer->vertices[3] = -nH;
    renderer->vertices[4] = -nW;
    renderer->vertices[5] = nH;
    renderer->vertices[6] = nW;
    renderer->vertices[7] = nH;
}


void GLES2_Renderer_TexCoords_reloadVertex(EGL_Renderer *renderer) {
    VIKTOR_LOGI("GLES2_Renderer_TexCoords_reloadVertex");
    glVertexAttribPointer(renderer->av2_texcoord, 2, GL_FLOAT, GL_FALSE, 0, renderer->texcoords);
    glEnableVertexAttribArray(renderer->av2_texcoord);
}

void GLES2_Renderer_Vertices_reloadVertex(EGL_Renderer *renderer) {
    VIKTOR_LOGI("GLES2_Renderer_Vertices_reloadVertex");
    glVertexAttribPointer(renderer->av4_position, 2, GL_FLOAT, GL_FALSE, 0, renderer->vertices);
    glEnableVertexAttribArray(renderer->av4_position);
}

EGL_Renderer *GLES2_Renderer_create(VideoOverlay *overlay) {
    if (!overlay) {
        return nullptr;
    }

    EGL_Renderer *renderer = nullptr;
    switch (overlay->frameFormat) {
        case AV_PIX_FMT_YUV420P:
        case AV_PIX_FMT_YUVJ420P:
            renderer = GLES2_Renderer_create_yuv420p();
            break;
        case AV_PIX_FMT_RGBA:
            renderer = GLES2_Renderer_create_rgb();
            break;
        default:
            return nullptr;
    }
    return renderer;
}


void GLES2_Renderer_create_base(EGL_Renderer *renderer, const char *fragmentShader) {
    GLint link_status = GL_FALSE;
    if (!renderer) {
        goto fail;
    }

    renderer->vertex_shader = GLES2_loadShader(GL_VERTEX_SHADER, VERTEX_SHADER);
    if (!renderer->vertex_shader) {
        goto fail;
    }
    renderer->fragment_shader = GLES2_loadShader(GL_FRAGMENT_SHADER, fragmentShader);
    if (!renderer->fragment_shader) {
        goto fail;
    }

    renderer->program = glCreateProgram();
    if (!renderer->program) {
        goto fail;
    }

    glAttachShader(renderer->program, renderer->vertex_shader);
    glAttachShader(renderer->program, renderer->fragment_shader);
    glLinkProgram(renderer->program);


    glGetProgramiv(renderer->program, GL_LINK_STATUS, &link_status);
    if (!link_status) {
        goto fail;
    }

    renderer->av4_position = glGetAttribLocation(renderer->program, "av4_Position");
    renderer->av2_texcoord = glGetAttribLocation(renderer->program, "av2_Texcoord");
    renderer->um4_mvp = glGetUniformLocation(renderer->program, "um4_ModelViewProjection");
    renderer->um4_Matrix = glGetUniformLocation(renderer->program, "textMatrix");

    return;

    fail:
    GLES2_Renderer_free(renderer);
}

EGL_Renderer *GLES2_Renderer_create_yuv420p() {
    EGL_Renderer *renderer = new EGL_Renderer_yuv420p();
    GLES2_Renderer_create_base(renderer, FRAGMENT_SHADER_YUV420P);

    renderer->us2_sampler[0] = glGetUniformLocation(renderer->program, "us2_SamplerX");
    renderer->us2_sampler[1] = glGetUniformLocation(renderer->program, "us2_SamplerY");
    renderer->us2_sampler[2] = glGetUniformLocation(renderer->program, "us2_SamplerZ");
    renderer->um3_color_conversion = glGetUniformLocation(renderer->program, "um3_ColorConversion");

    return renderer;
}

EGL_Renderer *GLES2_Renderer_create_rgb() {
    EGL_Renderer *renderer = new EGL_Renderer_RGB();
    GLES2_Renderer_create_base(renderer, FRAGMENT_SHADER_RGB);
    renderer->us2_sampler[0] = glGetUniformLocation(renderer->program, "us2_SamplerX");
    return renderer;
}

GLboolean GLES2_Renderer_use(EGL_Renderer *renderer, int width, int height) {
    if (!renderer) {
        return GL_FALSE;
    }

    if (!renderer->fun_use(width,height)) {
        return GL_FALSE;
    }
//    GLES2_Renderer_TexCoords_reset(renderer);
//    GLES2_Renderer_TexCoords_reloadVertex(renderer);
//
//    GLES2_Renderer_Vertices_reset(renderer);
//    GLES2_Renderer_Vertices_reloadVertex(renderer);
    return GL_TRUE;
}

void GLES2_Renderer_matrix(EGL_Renderer *renderer,int width,int height){
    VIKTOR_LOGI("GLES2_Renderer_matrix orientation:%d",renderer->orientation);
    GLES_Matrix orthoMatrix;
    GLES2_loadOrtho(&orthoMatrix, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
//    GLES2_rotate(&orthoMatrix,renderer->orientation);

    GLES_Matrix modelMatrix = matrix_rotate(renderer->orientation, 0.0, 0.0, -1.0);
    GLES_Matrix viewMatrix = matrix_identity();

//    setLookAtM(&viewMatrix,0,
//               0.0f,0.0f,0.1f, // 相机位置
//               0.0f,0.0f,0.0f, // 目标位置
//               0.0f,1.0f,0.0f); // 相机正上方向量)


    GLES_Matrix matrix1 = matrix_multiply(&orthoMatrix, &viewMatrix);
    GLES_Matrix mvpMatrix = matrix_multiply(&matrix1, &modelMatrix);

    glUniformMatrix4fv(renderer->um4_mvp, 1, GL_FALSE, mvpMatrix.m);
}


GLboolean GLES2_Renderer_renderOverlay(EGL_Renderer *renderer, VideoOverlay *overlay) {
    if (!renderer) {
        return GL_FALSE;
    }
//    GLES2_Renderer_matrix(renderer,overlay->w,overlay->h);
//    glClear(GL_COLOR_BUFFER_BIT);
//    GLES2_Renderer_TexCoords_reset(renderer);
//    GLES2_Renderer_TexCoords_reloadVertex(renderer);
//
//    GLES2_Renderer_Vertices_reset(renderer);
//    GLES2_Renderer_Vertices_reloadVertex(renderer);
    if (!renderer->fun_uploadTexture(overlay)) {
        return GL_FALSE;
    }

    VIKTOR_LOGI("GLES2_Renderer_renderOverlay glDrawArrays");
//    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    return GL_TRUE;
}


GLboolean GLES2_Renderer_isValid(EGL_Renderer *renderer) {
    return renderer && renderer->program ? GL_TRUE : GL_FALSE;
}

GLboolean GLES2_Renderer_isFormat(EGL_Renderer *renderer, int format) {
    if (!GLES2_Renderer_isValid(renderer)) {
        return GL_FALSE;
    }

    return renderer->format == format ? GL_TRUE : GL_FALSE;
}

void GLES2_Renderer_reset(EGL_Renderer *renderer) {
    if (!renderer) return;

    if (renderer->vertex_shader) {
        glDeleteShader(renderer->vertex_shader);
    }
    if (renderer->fragment_shader) {
        glDeleteShader(renderer->fragment_shader);
    }
    if (renderer->program) {
        glDeleteProgram(renderer->program);
    }

    renderer->vertex_shader = 0;
    renderer->fragment_shader = 0;
    renderer->program = 0;
    for (int i = 0; i < GLES2_MAX_PLANE; ++i) {
        if (renderer->plane_textures[i]) {
            glDeleteTextures(1, &renderer->plane_textures[i]);
            renderer->plane_textures[i] = 0;
        }
    }
}

void GLES2_Renderer_freeP(EGL_Renderer **renderer) {
    if (!renderer || !*renderer) {
        return;
    }

    GLES2_Renderer_free(*renderer);
    *renderer = nullptr;
}

void GLES2_Renderer_free(EGL_Renderer *renderer) {
    if (!renderer) return;
    renderer->func_destroy();
    free(renderer);
}