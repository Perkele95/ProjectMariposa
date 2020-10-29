#pragma once

#include <math.h>

// ---------------------
// Vectors
// ---------------------

struct vec2
{
    float X, Y;
};

struct vec3
{
    float X, Y, Z;
};

inline vec3 operator*(float a, vec3 b)
{
    vec3 result;
    
    result.X = a * b.X;
    result.Y = a * b.Y;
    result.Z = a * b.Z;
    
    return result;
}

inline vec3 operator*(vec3 b, float a)
{
    return a * b;
}

inline vec3& operator*=(vec3 &a, float b)
{
    a = b * a;
    
    return a;
}

inline vec3 operator+(vec3 a)
{
    vec3 result;
    
    result.X = -a.X;
    result.Y = -a.Y;
    result.Z = -a.Z;
    
    return result;
}

inline vec3 operator+(vec3 a, vec3 b)
{
    vec3 result;
    
    result.X = a.X + b.X;
    result.Y = a.Y + b.Y;
    result.Z = a.Z + b.Z;
    
    return result;
}

inline vec3& operator+=(vec3 &a, vec3 b)
{
    a = a + b;
    
    return a;
}

inline vec3 operator-(vec3 a, vec3 b)
{
    vec3 result;
    
    result.X = a.X - b.X;
    result.Y = a.Y - b.Y;
    result.Z = a.Z - b.Z;
    
    return result;
}

inline vec3& operator-=(vec3 &a, vec3 b)
{
    a = a - b;
    
    return a;
}

vec3 Normalisevec3(vec3 vec)
{
    float length = sqrtf(vec.X * vec.X + vec.Y * vec.Y + vec.Z * vec.Z);
    vec3 result;
    
    result.X = vec.X / length;
    result.Y = vec.Y / length;
    result.Z = vec.Z / length;
    
    return result;
}

float vec3Dot(vec3 a, vec3 b)
{
    float result = a.X * b.X + a.Y * b.Y + a.Z * b.Z;
    
    return result;
}

vec3 vec3Cross(vec3 a, vec3 b)
{
    vec3 result;
    
    result.X = a.Y * b.Z - a.Z * b.Y;
    result.Y = a.Z * b.X - a.X * b.Z;
    result.Z = a.X * b.Y - a.Y * b.X;
    
    return result;
}

// ---------------------
// 4x4 Matrix
// ---------------------

struct mat4x4
{
    float data[4][4];
};

inline mat4x4 Mat4x4Identity()
{
    mat4x4 result = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    return result;
}

mat4x4 operator*(mat4x4 a, mat4x4 b)
{
    mat4x4 result = {};
    
    for(uint16 k = 0; k < 4; k++)
    {
        for(uint16 i = 0; i < 4; i++)
        {
            result.data[k][i] = a.data[k][0] * b.data[0][i] + a.data[k][1] * b.data[1][i] + a.data[k][2] * b.data[2][i] + a.data[k][3] * b.data[3][i];
        }
    }
    
    return result;
}

mat4x4 Mat4RotateX(float rad)
{
    float c = cosf(rad);
    float s = sinf(rad);
    
    mat4x4 result = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, c, -s, 0.0f,
        0.0f, s, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    return result;
}

mat4x4 Mat4RotateY(float rad)
{
    float c = cosf(rad);
    float s = sinf(rad);
    
    mat4x4 result = {
        c, 0.0f, s, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        -s, 0.0f, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    return result;
}

mat4x4 Mat4RotateZ(float rad)
{
    float c = cosf(rad);
    float s = sinf(rad);
    
    mat4x4 result = {
        c, -s, 0.0f, 0.0f,
        s, c, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    return result;
}

mat4x4 LookAt(vec3 eye, vec3 center, vec3 up)
{
    vec3 X, Y, Z;
    
    Z = eye - center;
    Z = Normalisevec3(Z);
    Y = up;
    X = vec3Cross(Y, Z);
    Y = vec3Cross(Z, X);
    
    X = Normalisevec3(X);
    Y = Normalisevec3(Y);
    
    mat4x4 result;
    
    result.data[0][0] = X.X;
    result.data[1][0] = X.Y;
    result.data[2][0] = X.Z;
    result.data[3][0] = -vec3Dot(X, eye);
    
    result.data[0][1] = Y.X;
    result.data[1][1] = Y.Y;
    result.data[2][1] = Y.Z;
    result.data[3][1] = -vec3Dot(Y, eye);
    
    result.data[0][2] = Z.X;
    result.data[1][2] = Z.Y;
    result.data[2][2] = Z.Z;
    result.data[3][2] = -vec3Dot(Z, eye);
    
    result.data[0][3] = 0.0f;
    result.data[1][3] = 0.0f;
    result.data[2][3] = 0.0f;
    result.data[3][3] = 1.0f;
    
    return result;
}

// fov is in radians, aspectRatio = width / height, zNear and zFar are clipping plane z-positions
mat4x4 Perspective(float fov, float aspectRatio, float zNear, float zFar)
{
    mat4x4 result = {};
    float tanHalfAngle = tanf(fov / 2.0f);
    
    result.data[0][0] = 1.0f / (aspectRatio * tanHalfAngle);
    result.data[1][1] = -1.0f / (tanHalfAngle);
    result.data[2][2] = -(zFar + zNear) / (zFar - zNear);
    result.data[2][3] = -1.0f;
    result.data[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);
    
    return result;
}