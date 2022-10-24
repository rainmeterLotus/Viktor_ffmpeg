//
// Created by rainmeterLotus on 2021/8/3.
//

#include "ff_egl_renderer_yuv420p.h"


GLboolean EGL_Renderer_yuv420p::fun_use(int width, int height){
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glUseProgram(program);

    if (0 == plane_textures[0]){
        glGenTextures(3,plane_textures);
    }

    for (int i = 0; i < 3; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D,plane_textures[i]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glUniform1i(us2_sampler[i],i);
    }

    glUniformMatrix3fv(um3_color_conversion,1,GL_FALSE,g_bt709);
    vert_pos_reset();
    text_pos_reset();
    text_pos_reloadVertex();
    vert_pos_reloadVertex();
    return GL_TRUE;
}

GLboolean EGL_Renderer_yuv420p::fun_uploadTexture(VideoOverlay *overlay){
    VIKTOR_LOGI("EGL_Renderer_yuv420p fun_uploadTexture ");
    if (!overlay){
        return GL_FALSE;
    }
    VIKTOR_LOGI("EGL_Renderer_yuv420p fun_uploadTexture 11");
    int width = overlay->w;
    int height = overlay->h;
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    uint8_t *destData[3];
    destData[0] = static_cast<uint8_t *>(malloc(width * height));
    destData[1] = static_cast<uint8_t *>(malloc(width/2 * height/2));
    destData[2] = static_cast<uint8_t *>(malloc(width/2 * height/2));

    uint8_t *y = destData[0];
    uint8_t *u = destData[1];
    uint8_t *v = destData[2];

    if (overlay->pitches[0] == width){
        memcpy(y,overlay->pixels[0],overlay->pitches[0] * height);
    } else {
        for (int i = 0; i < height; ++i) {
            memcpy(y + width*i,overlay->pixels[0] + overlay->pitches[0]*i,width);
        }
    }

    if (overlay->pitches[1] == width/2){
        memcpy(u,overlay->pixels[1],overlay->pitches[1] * height/2);
    } else {
        for (int i = 0; i < height/2; ++i) {
            memcpy(u + width/2*i,overlay->pixels[1] + overlay->pitches[1]*i,width/2);
        }
    }

    if (overlay->pitches[2] == width/2){
        memcpy(v,overlay->pixels[2],overlay->pitches[2] * height/2);
    } else {
        for (int i = 0; i < height/2; ++i) {
            memcpy(v + width/2*i,overlay->pixels[2] + overlay->pitches[2]*i,width/2);
        }
    }



    int planes[3] = {0,1,2};
    const GLsizei widths[3] = {overlay->w, overlay->w/2,overlay->w/2};
    const GLsizei heights[3] = {overlay->h,overlay->h/2,overlay->h/2};

//    const GLsizei widths[3]    = { overlay->pitches[0], overlay->pitches[1], overlay->pitches[2] };
//    const GLsizei heights[3]   = { overlay->h,          overlay->h / 2,      overlay->h / 2 };
//    const GLubyte *pixels[3]   = { overlay->pixels[0],  overlay->pixels[1],  overlay->pixels[2] };
    for (int i = 0; i < 3; ++i) {
        int plane = planes[i];
        glBindTexture(GL_TEXTURE_2D,plane_textures[i]);
        glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE,
                     widths[plane],heights[plane],
                     0,GL_LUMINANCE,GL_UNSIGNED_BYTE,destData[plane]);
    }

    free(destData[0]);
    free(destData[1]);
    free(destData[2]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//    disable_vertex_attrib_array();
    return GL_TRUE;
}

void EGL_Renderer_yuv420p::func_destroy(){

}