//
// Created by rainmeterLotus on 2022/2/28.
//

#ifndef VIKTOR_FFMPEG_VIKTOR_ROTATE_FILTER_H
#define VIKTOR_FFMPEG_VIKTOR_ROTATE_FILTER_H


#include "viktor_base_filter.h"

static const char *ROTATE_VERTEX_SHADER = GET_STR(
        precision highp float;
        varying   highp vec2 vv2_Texcoord;
        attribute highp vec4 av4_Position;
        attribute highp vec2 av2_Texcoord;
        uniform         mat4 um4_projection;

        void main()
        {
//            gl_Position  = um4_projection * av4_Position;
            gl_Position  = av4_Position;
            vv2_Texcoord = av2_Texcoord.xy;
        }

);


static const char *ROTATE_FRAGMENT_SHADER_RGB = GET_STR(
        precision highp float;
        varying   highp vec2 vv2_Texcoord;
        uniform   lowp  sampler2D us2_SamplerX;
        void main() {
//            if(vv2_Texcoord.x < 0.5){
//                gl_FragColor = texture2D(us2_SamplerX, vv2_Texcoord.xy) * vec4(0.0,1.0,0.0,1.0);
//            } else {
//                gl_FragColor = texture2D(us2_SamplerX, vv2_Texcoord.xy);
//            }
            gl_FragColor = texture2D(us2_SamplerX, vv2_Texcoord.xy);

        }

);

class Viktor_Rotate_Filter: public Viktor_Base_Filter {
public:
    virtual void onInit(int width,int height);
    virtual void onRender(GLuint texture_id,int width,int height,int orientation,int serial);
    virtual void onDraw(GLuint texture_id);
private:
    GLuint m_um4_mvp;
    GLuint m_us2_sampler;
};


#endif //VIKTOR_FFMPEG_VIKTOR_ROTATE_FILTER_H
