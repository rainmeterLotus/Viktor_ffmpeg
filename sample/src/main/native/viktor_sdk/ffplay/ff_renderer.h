//
// Created by rainmeterLotus on 2021/8/3.
//

#ifndef VIKTOR_FFMPEG_FF_RENDERER_H
#define VIKTOR_FFMPEG_FF_RENDERER_H

#include "ff_egl_renderer.h"
#include "ff_frame_queue.h"
#include "../timeline//util/ViktorLog.h"
#include "../../all/viktor_matrix.h"

#define GET_STR(x) #x
static const char *VERTEX_SHADER = GET_STR(
        precision highp float;
        varying   highp vec2 vv2_Texcoord;
        attribute highp vec4 av4_Position;
        attribute highp vec2 av2_Texcoord;
        uniform         mat4 um4_ModelViewProjection;
        uniform         mat4 textMatrix;

        void main()
        {
//            gl_Position  = um4_ModelViewProjection * av4_Position;
            gl_Position = av4_Position;
//            vec4 tp = vec4(av2_Texcoord.x,av2_Texcoord.y,1,1);
//            tp = textMatrix * tp;
            vv2_Texcoord = av2_Texcoord;
//            vv2_Texcoord = vec2(av2_Texcoord.x,1.0-av2_Texcoord.y);
//            vv2_Texcoord = vec2((um4_ModelViewProjection*av2_Texcoord).x,(um4_ModelViewProjection*av2_Texcoord).y);
        }

);

static const char *FRAGMENT_SHADER_YUV420P = GET_STR(
        precision highp float;
        varying   highp vec2 vv2_Texcoord;
        uniform         mat3 um3_ColorConversion;
        uniform   lowp  sampler2D us2_SamplerX;
        uniform   lowp  sampler2D us2_SamplerY;
        uniform   lowp  sampler2D us2_SamplerZ;

        void main()
        {
            mediump vec3 yuv;
            lowp    vec3 rgb;

            yuv.x = (texture2D(us2_SamplerX, vv2_Texcoord).r - (16.0 / 255.0));
            yuv.y = (texture2D(us2_SamplerY, vv2_Texcoord).r - 0.5);
            yuv.z = (texture2D(us2_SamplerZ, vv2_Texcoord).r - 0.5);
            rgb = um3_ColorConversion * yuv;
            gl_FragColor = vec4(rgb, 1);
        }

);

static const char *FRAGMENT_SHADER_RGB = GET_STR(
        precision highp float;
        varying   highp vec2 vv2_Texcoord;
        uniform   lowp  sampler2D us2_SamplerX;
        void main() {
            gl_FragColor = texture2D(us2_SamplerX, vv2_Texcoord.xy);
//            gl_FragColor = vec4(texture2D(us2_SamplerX, vv2_Texcoord).rgb, 1);
        }

);

GLuint GLES2_loadShader(GLenum shader_type, const char *shader_source);

GLboolean GLES2_Renderer_isValid(EGL_Renderer *renderer);
GLboolean GLES2_Renderer_use(EGL_Renderer *renderer,int width,int height);
void GLES2_Renderer_matrix(EGL_Renderer *renderer,int width,int height);
GLboolean GLES2_Renderer_renderOverlay(EGL_Renderer *renderer, VideoOverlay *overlay);
GLboolean GLES2_Renderer_isFormat(EGL_Renderer *renderer,int format);
GLboolean GLES2_Renderer_setupGLES();

EGL_Renderer *GLES2_Renderer_create(VideoOverlay *overlay);
void GLES2_Renderer_create_base(EGL_Renderer *renderer,const char *fragmentShader);
EGL_Renderer *GLES2_Renderer_create_yuv420p();
EGL_Renderer *GLES2_Renderer_create_rgb();

void GLES2_Renderer_TexCoords_reset(EGL_Renderer *renderer);
void GLES2_Renderer_Vertices_reset(EGL_Renderer *renderer);
void GLES2_Renderer_TexCoords_reloadVertex(EGL_Renderer *renderer);
void GLES2_Renderer_Vertices_reloadVertex(EGL_Renderer *renderer);
void GLES2_Renderer_TexCoords_cropRight(EGL_Renderer *renderer, GLfloat cropRight);
void GLES2_Renderer_Vertices_apply(EGL_Renderer *renderer);

void GLES2_Renderer_reset(EGL_Renderer *renderer);
void GLES2_Renderer_free(EGL_Renderer *renderer);
void GLES2_Renderer_freeP(EGL_Renderer **renderer);



#endif //VIKTOR_FFMPEG_FF_RENDERER_H
