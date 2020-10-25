
#pragma once

#include <math.h>

typedef struct 
{
    float x, y;
} Vec2;

typedef struct 
{
    float x, y, z;
} Vec3;

typedef struct 
{
    float x, y, z, w;
} Vec4;

inline Vec3 Vec3Add(Vec3 a, Vec3 b)
{
    Vec3 result;
    
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    
    return result;
}

inline Vec3 NormaliseVec3(Vec3 vec)
{
    float length = sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
    Vec3 result;
    
    result.x = vec.x / length;
    result.y = vec.y / length;
    result.z = vec.z / length;
    
    return result;
}

inline float Vec3Dot(Vec3 a, Vec3 b)
{
    float result = a.x * b.x + a.y * b.y + a.z * b.z;
    
    return result;
}

inline Vec3 Vec3Cross(Vec3 a, Vec3 b)
{
    Vec3 result;
    
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    
    return result;
}

// ---------------------
// 4x4 Matrix
// ---------------------

typedef struct
{
    float data[4][4];
} Mat4x4;

inline Mat4x4 Mat4x4x4Mult(Mat4x4 a, Mat4x4 b)
{
    Mat4x4 result = {0};
    
    for(uint16 k = 0; k < 4; k++)
    {
        for(uint16 i = 0; i < 4; i++)
        {
            result.data[k][i] = a.data[k][0] * b.data[0][i] + a.data[k][1] * b.data[1][i] + a.data[k][2] * b.data[2][i] + a.data[k][3] * b.data[3][i];
        }
    }
    
    return result;
}

inline Mat4x4 Mat4x4RotateX(float rad)
{
    float c = cosf(rad);
    float s = sinf(rad);
    
    Mat4x4 result = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, c, -s, 0.0f,
        0.0f, s, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    return result;
}

inline Mat4x4 Mat4x4RotateY(float rad)
{
    float c = cosf(rad);
    float s = sinf(rad);
    
    Mat4x4 result = {
        c, 0.0f, s, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        -s, 0.0f, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    return result;
}

inline Mat4x4 Mat4x4RotateZ(float rad)
{
    float c = cosf(rad);
    float s = sinf(rad);
    
    Mat4x4 result = {
        c, -s, 0.0f, 0.0f,
        s, c, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    return result;
}

inline Mat4x4 Mat4x4LookAt(Vec3 eye, Vec3 center, Vec3 up)
{
    Vec3 X, Y, Z;
    
    Z = Vec3Add(eye, center);
    Z = NormaliseVec3(Z);
    Y = up;
    X = Vec3Cross(Y, Z);
    Y = Vec3Cross(Z, X);
    
    X = NormaliseVec3(X);
    Y = NormaliseVec3(Y);
    
    Mat4x4 result;
    
    result.data[0][0] = X.x;
    result.data[1][0] = X.y;
    result.data[2][0] = X.z;
    result.data[3][0] = -Vec3Dot(X, eye);
    
    result.data[0][1] = Y.x;
    result.data[1][1] = Y.y;
    result.data[2][1] = Y.z;
    result.data[3][1] = -Vec3Dot(Y, eye);
    
    result.data[0][2] = Z.x;
    result.data[1][2] = Z.y;
    result.data[2][2] = Z.z;
    result.data[3][2] = -Vec3Dot(Z, eye);
    
    result.data[0][3] = 0.0f;
    result.data[1][3] = 0.0f;
    result.data[2][3] = 0.0f;
    result.data[3][3] = 1.0f;
    
    return result;
}

// fov is in radians, aspectRatio = width / height, zNear and zFar are clipping plane z-positions
inline Mat4x4 Mat4x4Perspective(float fov, float aspectRatio, float zNear, float zFar)
{
    Mat4x4 result = {0};
    float tanHalfAngle = tanf(fov / 2.0f);
    
    result.data[0][0] = 1.0f / (aspectRatio * tanHalfAngle);
    result.data[1][1] = -1.0f / (tanHalfAngle);
    result.data[2][2] = -(zFar + zNear) / (zFar - zNear);
    result.data[2][3] = -1.0f;
    result.data[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);
    
    return result;
}

inline Mat4x4 Mat4x4Identity()
{
    Mat4x4 result = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    return result;
}