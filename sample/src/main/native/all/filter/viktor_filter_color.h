//
// Created by rainmeterLotus on 2022/2/22.
//

#ifndef VIKTOR_FFMPEG_VIKTOR_FILTER_COLOR_H
#define VIKTOR_FFMPEG_VIKTOR_FILTER_COLOR_H

#include "viktor_base_filter.h"


static const char *color_VERTEX_SHADER = GET_STR(
        precision highp float;
        varying   highp vec2 vv2_Texcoord;
        attribute highp vec4 av4_Position;
        attribute highp vec2 av2_Texcoord;

        void main()
        {
            gl_Position  = av4_Position;
            vv2_Texcoord = av2_Texcoord.xy;
        }

);

static const char *color_FRAGMENT_SHADER_RGB = GET_STR(
        precision highp float;
        varying   highp vec2 vv2_Texcoord;
        uniform   lowp  sampler2D us2_SamplerX;
        void main() {
            if(vv2_Texcoord.x > 0.5){
                gl_FragColor = texture2D(us2_SamplerX, vv2_Texcoord.xy) * vec4(1.0,0.0,0.0,1.0);
            } else {
                gl_FragColor = texture2D(us2_SamplerX, vv2_Texcoord.xy);
            }

//            gl_FragColor = vec4(texture2D(us2_SamplerX, vv2_Texcoord).rgb, 1);

        }

);

class Viktor_Filter_Color: public Viktor_Base_Filter {
public:
    virtual void onInit(int width,int height);
    virtual void onRender(GLuint texture_id,int width,int height,int orientation,int serial);
    virtual void onDraw(GLuint texture_id);
private:
    GLuint in_texture_id;
    GLuint m_us2_sampler;

    GLuint m_frame_buffer[1];
};


#endif //VIKTOR_FFMPEG_VIKTOR_FILTER_COLOR_H
