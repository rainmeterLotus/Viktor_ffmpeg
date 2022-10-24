//
// Created by rainmeterLotus on 2022/2/22.
//

#include "viktor_filter_color.h"

void Viktor_Filter_Color::onInit(int width,int height){
    GLuint vertex_shader = load_shader(GL_VERTEX_SHADER,color_VERTEX_SHADER);
    if (!vertex_shader){
        return;
    }
    GLuint fragment_shader = load_shader(GL_FRAGMENT_SHADER,color_FRAGMENT_SHADER_RGB);
    if (!fragment_shader){
        return;
    }

    m_program = glCreateProgram();
    if (!m_program){
        return;
    }
    glAttachShader(m_program, vertex_shader);
    glAttachShader(m_program, fragment_shader);
    glLinkProgram(m_program);

    GLint link_status = GL_FALSE;
    glGetProgramiv(m_program, GL_LINK_STATUS, &link_status);
    if (!link_status) {
        return;
    }

    m_av4_position = glGetAttribLocation(m_program, "av4_Position");
    m_av2_texcoord = glGetAttribLocation(m_program, "av2_Texcoord");
    m_us2_sampler = glGetUniformLocation(m_program,"us2_SamplerX");


    text_pos_reset();
    vert_pos_reset();
}


void Viktor_Filter_Color::onRender(GLuint texture_id,int width,int height,int orientation,int serial){
    if (!m_program){
        return;
    }
    VIKTOR_LOGD("onRender--\n");
//    glGenFramebuffers(1,m_frame_buffer);
//    glGenTextures(1,m_texture);
//    glBindTexture(GL_TEXTURE_2D,m_texture[0]);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE, nullptr);
//
//    glBindFramebuffer(GL_FRAMEBUFFER,m_frame_buffer[0]);
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, m_texture[0], 0);
    glViewport(0,0,width,height);
    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    onDraw(texture_id);
//    glBindTexture(GL_TEXTURE_2D,0);
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, 0, 0);
}

void Viktor_Filter_Color::onDraw(GLuint texture_id){

    glUseProgram(m_program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,texture_id);
    glUniform1i(m_us2_sampler,0);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    text_pos_reloadVertex();
    vert_pos_reloadVertex();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    disable_vertex_attrib_array();
    glBindTexture(GL_TEXTURE_2D,0);
}