//
// Created by rainmeterLotus on 2022/2/28.
//

#ifndef VIKTOR_FFMPEG_VIKTOR_MATRIX_H
#define VIKTOR_FFMPEG_VIKTOR_MATRIX_H
#include <GLES2/gl2.h>
typedef struct GLES_Matrix{
    GLfloat m[16];
} GLES_Matrix;


void GLES2_loadOrtho(GLES_Matrix *matrix, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near, GLfloat far);
void GLES2_rotate(GLES_Matrix *matrix,int angle);

GLES_Matrix matrix_rotate(int angle,float x,float y,float z);
GLES_Matrix matrix_multiply(GLES_Matrix *a,GLES_Matrix *b);
GLES_Matrix matrix_identity();
GLES_Matrix matrix_perspective(float fovY, float aspectRatio, float nearZ, float farZ);
void setLookAtM(GLES_Matrix *matrix, int rmOffset,
                float eyeX, float eyeY, float eyeZ,
                float centerX, float centerY, float centerZ, float upX, float upY,
                float upZ);

float matrix_length(float x, float y, float z);
void matrix_translateM(GLES_Matrix *matrix, int mOffset,float x, float y, float z);

#endif //VIKTOR_FFMPEG_VIKTOR_MATRIX_H
