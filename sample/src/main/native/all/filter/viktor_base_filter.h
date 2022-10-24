//
// Created by rainmeterLotus on 2022/2/23.
//

#ifndef VIKTOR_FFMPEG_VIKTOR_BASE_FILTER_H
#define VIKTOR_FFMPEG_VIKTOR_BASE_FILTER_H

#include <GLES2/gl2.h>
#include "../../viktor_sdk/timeline/util/ViktorLog.h"

#define GET_STR(x) #x

class Viktor_Base_Filter {
public:
    GLuint m_program;
    GLuint m_av4_position;
    GLuint m_av2_texcoord;
    GLuint m_texture[1];

    GLfloat texcoords[8];

    GLfloat vertices[8];

    int m_orientation;

    /**
    * 用于区分是否同一视频源
    */
    int m_serial = -1;
    GLuint m_frame_buffer[1];
    GLuint m_out_texture[1];


    virtual void onInit(int width, int height) = 0;

    virtual void onRender(GLuint texture_id, int width, int height, int orientation, int serial) = 0;

    virtual void onDraw(GLuint texture_id) = 0;

    GLuint load_shader(GLenum shader_type, const char *shader_source) {
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

    void gen_textures() {
        glGenTextures(1, m_texture);
        for (int i = 0; i < 1; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, m_texture[i]);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
    }

    GLuint get_texture_id() {
        return m_out_texture[0];
    }

    void text_pos_reset() {
//        texcoords[0] = 0.0f;
//        texcoords[1] = 1.0f;
//        texcoords[2] = 1.0f;
//        texcoords[3] = 1.0f;
//        texcoords[4] = 0.0f;
//        texcoords[5] = 0.0f;
//        texcoords[6] = 1.0f;
//        texcoords[7] = 0.0f;

        texcoords[0] = 0.0f;
        texcoords[1] = 0.0f;

        texcoords[2] = 1.0f;
        texcoords[3] = 0.0f;

        texcoords[4] = 0.0f;
        texcoords[5] = 1.0f;

        texcoords[6] = 1.0f;
        texcoords[7] = 1.0f;
    }

    void vert_pos_reset() {
        vertices[0] = -1.0f;
        vertices[1] = -1.0f;
        vertices[2] = 1.0f;
        vertices[3] = -1.0f;
        vertices[4] = -1.0f;
        vertices[5] = 1.0f;
        vertices[6] = 1.0f;
        vertices[7] = 1.0f;
    }

    void text_pos_reloadVertex() {
        glVertexAttribPointer(m_av2_texcoord, 2, GL_FLOAT, GL_FALSE, 0, texcoords);
        glEnableVertexAttribArray(m_av2_texcoord);
    }

    void vert_pos_reloadVertex() {
        glVertexAttribPointer(m_av4_position, 2, GL_FLOAT, GL_FALSE, 0, vertices);
        glEnableVertexAttribArray(m_av4_position);
    }

    void disable_vertex_attrib_array() {
        glDisableVertexAttribArray(m_av2_texcoord);
        glDisableVertexAttribArray(m_av4_position);
    }


    void create_fbo(int width, int height) {

        glGenTextures(1, m_out_texture);
        glBindTexture(GL_TEXTURE_2D, m_out_texture[0]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, GL_NONE);

        glGenFramebuffers(1, m_frame_buffer);

        glBindTexture(GL_TEXTURE_2D, m_out_texture[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buffer[0]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_out_texture[0], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            VIKTOR_LOGD("glCheckFramebufferStatus status != GL_FRAMEBUFFER_COMPLETE");
            return;
        }
        glBindTexture(GL_TEXTURE_2D, GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void reset_fbo(int width, int height) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteTextures(1, m_out_texture);
        glDeleteFramebuffers(1, m_frame_buffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);

        create_fbo(width, height);
    }
};


#endif //VIKTOR_FFMPEG_VIKTOR_BASE_FILTER_H
