//
// Created by rainmeterLotus on 2022/2/28.
//

#include <vector>
#include <EGL/egl.h>
#include "viktor_rotate_filter.h"
#include "../all/viktor_matrix.h"

void Viktor_Rotate_Filter::onInit(int width,int height){
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GLuint vertex_shader = load_shader(GL_VERTEX_SHADER,ROTATE_VERTEX_SHADER);
    if (!vertex_shader){
        return;
    }
    GLuint fragment_shader = load_shader(GL_FRAGMENT_SHADER,ROTATE_FRAGMENT_SHADER_RGB);
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
    m_um4_mvp = glGetUniformLocation(m_program, "um4_projection");
    m_us2_sampler = glGetUniformLocation(m_program,"us2_SamplerX");
    text_pos_reset();
    vert_pos_reset();
    glUseProgram(m_program);
//    create_fbo(width,height);
}

void Viktor_Rotate_Filter::onRender(GLuint texture_id,int width,int height,int orientation,int serial) {
    if (!m_program) {
        return;
    }
//    glViewport(0, 0, width, height);
//    glClear(GL_COLOR_BUFFER_BIT);
    if (serial != m_serial){
        m_serial = serial;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,texture_id);
    glUniform1i(m_us2_sampler,0);

    text_pos_reloadVertex();
    vert_pos_reloadVertex();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    disable_vertex_attrib_array();

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Viktor_Rotate_Filter::onDraw(GLuint texture_id){

}