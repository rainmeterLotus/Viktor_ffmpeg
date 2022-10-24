//
// Created by rainmeterLotus on 2021/8/11.
//

#include <vector>
#include "ff_egl_renderer_rgb.h"
#include "ff_video_texture.h"

GLboolean EGL_Renderer_RGB::fun_use(int width, int height) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glUseProgram(program);

    if (0 == plane_textures[0]) {
        glGenTextures(1, plane_textures);
    }

    for (int i = 0; i < 1; ++i) {
//        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, plane_textures[i]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

//        glUniform1i(us2_sampler[i],i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    vert_pos_reset();
    text_pos_reset();
    create_fbo(width, height);
    return GL_TRUE;
}

GLboolean EGL_Renderer_RGB::fun_uploadTexture(VideoOverlay *overlay) {
    switch (overlay->frameFormat) {
        case AV_PIX_FMT_RGBA:
            return fun_uploadTextureRGB(overlay);
        default:
            return GL_FALSE;
    }
}

GLboolean EGL_Renderer_RGB::fun_uploadTextureRGB(VideoOverlay *overlay) {
    if (!overlay) {
        return GL_FALSE;
    }

    int width = overlay->w;
    int height = overlay->h;

    glViewport(0,0,width,height);
//    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    int linesize = overlay->pitches[0];

    int num = linesize / width;
    VIKTOR_LOGI("fun_uploadTextureRGB888 width:%d,height:%d,linesize:%d", width, height, linesize);
    VIKTOR_LOGI("fun_uploadTextureRGB888 num:%d", num);

    uint8_t *destData[0];
    destData[0] = static_cast<uint8_t *>(malloc(width * height * num));
    uint8_t *y = destData[0];
   if (width * num  == linesize){
        memcpy(y,overlay->pixels[0],overlay->pitches[0] * height);
    } else {
        for (int i = 0; i < height; ++i) {
            memcpy(y + width * num * i,overlay->pixels[0] + overlay->pitches[0]*i,width * num);
        }
    }

    //数据上下颠倒，下面做颠倒处理
//    for (int i = 0; i < height; ++i) {
//        memcpy(y + width * num * i,overlay->pixels[0] + overlay->pitches[0]*(height - 1 -i),width * num);
//    }


    int planes[1] = {0};
    const GLsizei widths[1] = {width};
    const GLsizei heights[1] = {height};
    const GLubyte *pixels[3] = {overlay->pixels[0]};
    if (has_filter) {
        glBindFramebuffer(GL_FRAMEBUFFER,m_frame_buffer[0]);
    }
    GLES_Matrix orthoMatrix;
    GLES2_loadOrtho(&orthoMatrix, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    GLES_Matrix modelMatrix = matrix_rotate(overlay->orientation, 0.0, 0.0, -1.0);
    GLES_Matrix viewMatrix = matrix_identity();

    GLES_Matrix matrix1 = matrix_multiply(&orthoMatrix, &viewMatrix);
    GLES_Matrix mvpMatrix = matrix_multiply(&matrix1, &modelMatrix);

    glUniformMatrix4fv(um4_mvp, 1, GL_FALSE, mvpMatrix.m);


    glActiveTexture(GL_TEXTURE0 + 0);

    for (int i = 0; i < 1; ++i) {
        int plane = planes[i];
        glBindTexture(GL_TEXTURE_2D, plane_textures[i]);
        glUniform1i(us2_sampler[i], i);
        text_pos_reloadVertex();
        vert_pos_reloadVertex();
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,
                     widths[plane],heights[plane],
                     0,GL_RGBA,GL_UNSIGNED_BYTE,destData[plane]);

    }

    free(destData[0]);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    disable_vertex_attrib_array();

    glBindTexture(GL_TEXTURE_2D, 0);
    if (has_filter) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    return GL_TRUE;
}

GLboolean EGL_Renderer_RGB::sws_scale_frame(AVFrame *avFrame) {
    if (!avFrame) {
        return GL_FALSE;
    }
    int width = avFrame->width;
    int height = avFrame->height;
    int sampleSize = calculateInSampleSize(width, height, 720, 1280);
    int destWidth = width / sampleSize;
    int destHeight = height / sampleSize;
    VIKTOR_LOGI("sws_scale_frame sampleSize:%d", sampleSize);
    VIKTOR_LOGI("sws_scale_frame width:%d,height:%d,avFrame->linesize[0]:%d", width, height, avFrame->linesize[0]);
    VIKTOR_LOGI("sws_scale_frame destWidth:%d,destHeight:%d", destWidth, destHeight);
    if (!m_SwsContext) {

        m_RGBAFrame = av_frame_alloc();
        int frame_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, destWidth, destHeight, 1);
        AVBufferRef *frame_buffer_ref = av_buffer_alloc(frame_bytes);
        if (!frame_buffer_ref) {
            return GL_FALSE;
        }

        av_image_fill_arrays(m_RGBAFrame->data, m_RGBAFrame->linesize,
                             frame_buffer_ref->data, AV_PIX_FMT_RGBA, destWidth, destHeight, 1);

        m_SwsContext = sws_getContext(width, height, static_cast<AVPixelFormat>(avFrame->format),
                                      destWidth, destHeight, AV_PIX_FMT_RGBA, SWS_FAST_BILINEAR,
                                      NULL, NULL, NULL);
        delete frame_buffer_ref;
    }

    sws_scale(m_SwsContext, (const uint8_t **) avFrame->data, avFrame->linesize, 0, height, m_RGBAFrame->data, m_RGBAFrame->linesize);

    m_RGBAFrame->width = destWidth;
    m_RGBAFrame->height = destHeight;
    VIKTOR_LOGI("sws_scale_frame");
    return GL_TRUE;
}

int EGL_Renderer_RGB::calculateInSampleSize(int srcWidth, int srcHeight, int reqWidth, int reqHeight) {
    int inSampleSize = 1;

    if (srcHeight > reqHeight || srcWidth > reqWidth) {

        while ((srcWidth / inSampleSize) > reqHeight || (srcHeight / inSampleSize) > reqWidth) {
            inSampleSize *= 2;
        }
    }

    return inSampleSize;
}

void EGL_Renderer_RGB::func_destroy() {
    if (m_SwsContext) {
        sws_freeContext(m_SwsContext);
        m_SwsContext = nullptr;
    }

    if (m_RGBAFrame) {
        av_frame_free(&m_RGBAFrame);
        m_RGBAFrame = nullptr;
    }
}