//
// Created by rainmeterLotus on 2021/8/3.
//

#ifndef VIKTOR_FFMPEG_FF_EGL_RENDERER_H
#define VIKTOR_FFMPEG_FF_EGL_RENDERER_H
#define GLES2_MAX_PLANE 3

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "ff_frame_queue.h"
#include "../timeline//util/ViktorLog.h"
#include "../../all/viktor_matrix.h"

static const int has_filter = 1;
static const GLfloat g_bt709[] = {
        1.164,  1.164,  1.164,
        0.0,   -0.213,  2.112,
        1.793, -0.533,  0.0,
};

static const GLfloat g_bt601[] = {
        1.164,  1.164, 1.164,
        0.0,   -0.392, 2.017,
        1.596, -0.813, 0.0,
};

class EGL_Renderer {
public:

    virtual GLboolean fun_use(int width, int height) = 0;
    virtual GLboolean fun_uploadTexture(VideoOverlay *overlay) = 0;
    virtual void func_destroy() = 0;

    GLuint program;
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint plane_textures[GLES2_MAX_PLANE];
    int format;

    GLuint av4_position;
    GLuint av2_texcoord;
    GLuint um4_mvp;
    GLuint um4_Matrix;

    GLuint us2_sampler[GLES2_MAX_PLANE];
    GLuint um3_color_conversion;
    GLsizei layer_width;
    GLsizei layer_height;
    GLsizei buffer_width;
    GLsizei visible_width;

    GLfloat texcoords[8];

    GLfloat vertices[8];
    int     vertices_changed;
    int     gravity;
    int     frame_width;
    int     frame_height;
    int     frame_sar_num;
    int     frame_sar_den;
    GLsizei last_buffer_width;

    int orientation;

    GLuint m_frame_buffer[1];
    GLuint m_render_buffer[1];
    GLuint m_out_texture[1];

    void set_texture_id(GLuint texture_id){
        plane_textures[0] = texture_id;
    }

    void create_fbo(int width, int height){
        glGenTextures(1,m_out_texture);
        glBindTexture(GL_TEXTURE_2D,m_out_texture[0]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//        glBindTexture(GL_TEXTURE_2D,GL_NONE);

        glGenFramebuffers(1,m_frame_buffer);
//        glGenRenderbuffers(1,m_render_buffer);

//        glBindTexture(GL_TEXTURE_2D,m_out_texture[0]);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE, nullptr);

        glBindFramebuffer(GL_FRAMEBUFFER,m_frame_buffer[0]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, m_out_texture[0], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!= GL_FRAMEBUFFER_COMPLETE) {
            VIKTOR_LOGE("glCheckFramebufferStatus status != GL_FRAMEBUFFER_COMPLETE");
            return;
        }
        glBindTexture(GL_TEXTURE_2D,GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
//
//
//        glBindRenderbuffer(GL_RENDERBUFFER,m_render_buffer[0]);
//        //glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH24_STENCIL8_OES,width,height);
//        glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,m_render_buffer[0]);
//        glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_STENCIL_ATTACHMENT,GL_RENDERBUFFER,m_render_buffer[0]);
//
//        glBindRenderbuffer(GL_RENDERBUFFER,0);
    }

    void reset_fbo(int width, int height){
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteTextures(1,m_out_texture);
        glDeleteFramebuffers(1,m_frame_buffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, 0, 0);

        create_fbo(width,height);
    }

    void text_pos_reset(){
        texcoords[0] = 0.0f;
        texcoords[1] = 1.0f;

        texcoords[2] = 1.0f;
        texcoords[3] = 1.0f;

        texcoords[4] = 0.0f;
        texcoords[5] = 0.0f;

        texcoords[6] = 1.0f;
        texcoords[7] = 0.0f;

//        texcoords[0] = 0.0f;
//        texcoords[1] = 0.0f;
//
//        texcoords[2] = 1.0f;
//        texcoords[3] = 0.0f;
//
//        texcoords[4] = 0.0f;
//        texcoords[5] = 1.0f;
//
//        texcoords[6] = 1.0f;
//        texcoords[7] = 1.0f;
    }

    void text_pos_90_reset(){
        texcoords[0] = 1.0f;
        texcoords[1] = 1.0f;

        texcoords[2] = 1.0f;
        texcoords[3] = 0.0f;

        texcoords[4] = 0.0f;
        texcoords[5] = 1.0f;

        texcoords[6] = 0.0f;
        texcoords[7] = 0.0f;
    }

    void vert_pos_reset(){
        vertices[0] = -1.0f;
        vertices[1] = -1.0f;

        vertices[2] =  1.0f;
        vertices[3] = -1.0f;

        vertices[4] = -1.0f;
        vertices[5] =  1.0f;

        vertices[6] =  1.0f;
        vertices[7] =  1.0f;
    }
    void vert_pos_90_reset(){
        vertices[0] = -1.0f;
        vertices[1] = 1.0f;

        vertices[2] =  -1.0f;
        vertices[3] = -1.0f;

        vertices[4] = 1.0f;
        vertices[5] =  1.0f;

        vertices[6] =  1.0f;
        vertices[7] =  -1.0f;
    }

    void text_pos_reloadVertex(){
        glVertexAttribPointer(av2_texcoord,2,GL_FLOAT,GL_FALSE,0,texcoords);
        glEnableVertexAttribArray(av2_texcoord);
    }

    void vert_pos_reloadVertex(){
        glVertexAttribPointer(av4_position,2,GL_FLOAT,GL_FALSE,0,vertices);
        glEnableVertexAttribArray(av4_position);
    }

    void disable_vertex_attrib_array(){
        glDisableVertexAttribArray(av2_texcoord);
        glDisableVertexAttribArray(av4_position);
    }
private:
};


#endif //VIKTOR_FFMPEG_FF_EGL_RENDERER_H
