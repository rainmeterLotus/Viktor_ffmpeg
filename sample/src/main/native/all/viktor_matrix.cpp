//
// Created by rainmeterLotus on 2022/2/28.
//

#include "viktor_matrix.h"
extern "C"{
#include <libavutil/imgutils.h>
}

void GLES2_loadOrtho(GLES_Matrix *matrix, GLfloat left, GLfloat right, GLfloat bottom,
                     GLfloat top, GLfloat near, GLfloat far) {
    GLfloat r_l = right - left;
    GLfloat t_b = top - bottom;
    GLfloat f_n = far - near;
    GLfloat tx = -(right + left) / (right - left);
    GLfloat ty = -(top + bottom) / (top - bottom);
    GLfloat tz = -(far + near) / (far - near);

    matrix->m[0] = 2.0f / r_l;
    matrix->m[1] = 0.0f;
    matrix->m[2] = 0.0f;
    matrix->m[3] = 0.0f;

    matrix->m[4] = 0.0f;
    matrix->m[5] = 2.0f / t_b;
    matrix->m[6] = 0.0f;
    matrix->m[7] = 0.0f;

    matrix->m[8] = 0.0f;
    matrix->m[9] = 0.0f;
    matrix->m[10] = -2.0f / f_n;
    matrix->m[11] = 0.0f;

    matrix->m[12] = tx;
    matrix->m[13] = ty;
    matrix->m[14] = tz;
    matrix->m[15] = 1.0f;
}

GLES_Matrix matrix_identity() {
    GLES_Matrix temp_matrix;
    temp_matrix.m[0] = 1.0;
    temp_matrix.m[1] = 0.0;
    temp_matrix.m[2] = 0.0;
    temp_matrix.m[3] = 0.0;

    temp_matrix.m[4] = 0.0;
    temp_matrix.m[5] = 1.0;
    temp_matrix.m[6] = 0.0;
    temp_matrix.m[7] = 0.0;

    temp_matrix.m[8] = 0.0;
    temp_matrix.m[9] = 0.0;
    temp_matrix.m[10] = 1.0;
    temp_matrix.m[11] = 0.0;

    temp_matrix.m[12] = 0.0;
    temp_matrix.m[13] = 0.0;
    temp_matrix.m[14] = 0.0;
    temp_matrix.m[15] = 1.0;
    return temp_matrix;
}

GLES_Matrix matrix_perspective(float fovY, float aspectRatio, float zNear, float zFar) {
    GLES_Matrix temp_matrix;
    float f = tanf(static_cast<float>(fovY * (M_PI / 360.0f)));
    float rangeReciprocal = 1.0f / (zNear - zFar);

    temp_matrix.m[0] = f / aspectRatio;
    temp_matrix.m[1] = 0.0f;
    temp_matrix.m[2] = 0.0f;
    temp_matrix.m[3] = 0.0f;

    temp_matrix.m[4] = 0.0f;
    temp_matrix.m[5] = f;
    temp_matrix.m[6] = 0.0f;
    temp_matrix.m[7] = 0.0f;

    temp_matrix.m[8] = 0.0f;
    temp_matrix.m[9] = 0.0f;
    temp_matrix.m[10] = (zFar + zNear) * rangeReciprocal;
    temp_matrix.m[11] = -1.0f;

    temp_matrix.m[12] = 0.0f;
    temp_matrix.m[13] = 0.0f;
    temp_matrix.m[14] = 2.0f * zFar * zNear * rangeReciprocal;
    temp_matrix.m[15] = 0.0f;
    return temp_matrix;
}


GLES_Matrix matrix_multiply(GLES_Matrix *a, GLES_Matrix *b) {
    GLES_Matrix temp_matrix;
    temp_matrix.m[0] = a->m[0] * b->m[0] + a->m[4] * b->m[1] + a->m[8] * b->m[2] + a->m[12] * b->m[3];
    temp_matrix.m[1] = a->m[1] * b->m[0] + a->m[5] * b->m[1] + a->m[9] * b->m[2] + a->m[13] * b->m[3];
    temp_matrix.m[2] = a->m[2] * b->m[0] + a->m[6] * b->m[1] + a->m[10] * b->m[2] + a->m[14] * b->m[3];
    temp_matrix.m[3] = a->m[3] * b->m[0] + a->m[7] * b->m[1] + a->m[11] * b->m[2] + a->m[15] * b->m[3];

    temp_matrix.m[4] = a->m[0] * b->m[4] + a->m[4] * b->m[5] + a->m[8] * b->m[6] + a->m[12] * b->m[7];
    temp_matrix.m[5] = a->m[1] * b->m[4] + a->m[5] * b->m[5] + a->m[9] * b->m[6] + a->m[13] * b->m[7];
    temp_matrix.m[6] = a->m[2] * b->m[4] + a->m[6] * b->m[5] + a->m[10] * b->m[6] + a->m[14] * b->m[7];
    temp_matrix.m[7] = a->m[3] * b->m[4] + a->m[7] * b->m[5] + a->m[11] * b->m[6] + a->m[15] * b->m[7];

    temp_matrix.m[8] = a->m[0] * b->m[8] + a->m[4] * b->m[9] + a->m[8] * b->m[10] + a->m[12] * b->m[11];
    temp_matrix.m[9] = a->m[1] * b->m[8] + a->m[5] * b->m[9] + a->m[9] * b->m[10] + a->m[13] * b->m[11];
    temp_matrix.m[10] = a->m[2] * b->m[8] + a->m[6] * b->m[9] + a->m[10] * b->m[10] + a->m[14] * b->m[11];
    temp_matrix.m[11] = a->m[3] * b->m[8] + a->m[7] * b->m[9] + a->m[11] * b->m[10] + a->m[15] * b->m[11];

    temp_matrix.m[12] = a->m[0] * b->m[12] + a->m[4] * b->m[13] + a->m[8] * b->m[14] + a->m[12] * b->m[15];
    temp_matrix.m[13] = a->m[1] * b->m[12] + a->m[5] * b->m[13] + a->m[9] * b->m[14] + a->m[13] * b->m[15];
    temp_matrix.m[14] = a->m[2] * b->m[12] + a->m[6] * b->m[13] + a->m[10] * b->m[14] + a->m[14] * b->m[15];
    temp_matrix.m[15] = a->m[3] * b->m[12] + a->m[7] * b->m[13] + a->m[11] * b->m[14] + a->m[15] * b->m[15];
    return temp_matrix;
}

void setLookAtM(GLES_Matrix *matrix, int rmOffset,
                float eyeX, float eyeY, float eyeZ,
                float centerX, float centerY, float centerZ, float upX, float upY,
                float upZ) {
    float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;

    // Normalize f
    float rlf = 1.0f / matrix_length(fx, fy, fz);
    fx *= rlf;
    fy *= rlf;
    fz *= rlf;

    // compute s = f x up (x means "cross product")
    float sx = fy * upZ - fz * upY;
    float sy = fz * upX - fx * upZ;
    float sz = fx * upY - fy * upX;

    // and normalize s
    float rls = 1.0f / matrix_length(sx, sy, sz);
    sx *= rls;
    sy *= rls;
    sz *= rls;

    // compute u = s x f
    float ux = sy * fz - sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy - sy * fx;

    matrix->m[rmOffset + 0] = sx;
    matrix->m[rmOffset + 1] = ux;
    matrix->m[rmOffset + 2] = -fx;
    matrix->m[rmOffset + 3] = 0.0f;

    matrix->m[rmOffset + 4] = sy;
    matrix->m[rmOffset + 5] = uy;
    matrix->m[rmOffset + 6] = -fy;
    matrix->m[rmOffset + 7] = 0.0f;

    matrix->m[rmOffset + 8] = sz;
    matrix->m[rmOffset + 9] = uz;
    matrix->m[rmOffset + 10] = -fz;
    matrix->m[rmOffset + 11] = 0.0f;

    matrix->m[rmOffset + 12] = 0.0f;
    matrix->m[rmOffset + 13] = 0.0f;
    matrix->m[rmOffset + 14] = 0.0f;
    matrix->m[rmOffset + 15] = 1.0f;
    matrix_translateM(matrix,rmOffset,-eyeX,-eyeY,-eyeZ);
}

void matrix_translateM(GLES_Matrix *matrix, int mOffset,float x, float y, float z){
    for (int i=0 ; i<4 ; i++) {
        int mi = mOffset + i;
        matrix->m[12 + mi] += matrix->m[mi] * x + matrix->m[4 + mi] * y + matrix->m[8 + mi] * z;
    }
}

float matrix_length(float x, float y, float z) {
    return (float) sqrt(x * x + y * y + z * z);
}

GLES_Matrix matrix_rotate(int angle, float x, float y, float z) {
    double radian = angle * (M_PI / 180.0);
    float c = cos(radian);
    float s = sin(radian);

    GLES_Matrix temp_matrix;

    temp_matrix.m[3] = 0.0f;
    temp_matrix.m[7] = 0.0f;
    temp_matrix.m[11] = 0.0f;
    temp_matrix.m[12] = 0.0f;
    temp_matrix.m[13] = 0.0f;
    temp_matrix.m[14] = 0.0f;
    temp_matrix.m[15] = 1.0f;

    float nc = 1.0f - c;
    float xy = x * y;
    float yz = y * z;
    float zx = z * x;
    float xs = x * s;
    float ys = y * s;
    float zs = z * s;

    temp_matrix.m[0] = x * x * nc + c;
    temp_matrix.m[4] = xy * nc - zs;
    temp_matrix.m[8] = zx * nc + ys;
    temp_matrix.m[1] = xy * nc + zs;
    temp_matrix.m[5] = y * y * nc + c;
    temp_matrix.m[9] = yz * nc - xs;
    temp_matrix.m[2] = zx * nc - ys;
    temp_matrix.m[6] = yz * nc + xs;
    temp_matrix.m[10] = z * z * nc + c;

    return temp_matrix;
}



void GLES2_rotate(GLES_Matrix *matrix, int angle) {
    double radian = angle * (M_PI / 180.0);
    matrix->m[0] = cos(radian);
    matrix->m[1] = -sin(radian);
    matrix->m[4] = sin(radian);
    matrix->m[5] = cos(radian);

}
