#pragma once

struct Vector3
{
    float X, Y, Z;
};

inline Vector3 operator*(float a, Vector3 b)
{
    Vector3 result;
    
    result.X = a * b.X;
    result.Y = a * b.Y;
    result.Z = a * b.Z;
    
    return result;
}

inline Vector3 operator*(Vector3 b, float a)
{
    return a * b;
}

inline Vector3& operator*=(Vector3 &a, float b)
{
    a = b * a;
    
    return a;
}

inline Vector3& operator+=(Vector3 &a, Vector3 b)
{
    a = a + b;
    
    return a;
}

inline Vector3& operator-=(Vector3 &a, Vector3 b)
{
    a = a - b;
    
    return a;
}

inline Vector3 operator+(Vector3 a)
{
    Vector3 result;
    
    result.X = -a.X;
    result.Y = -a.Y;
    result.Z = -a.Z;
    
    return result;
}

inline Vector3 operator+(Vector3 a, Vector3 b)
{
    Vector3 result;
    
    result.X = a.X + b.X;
    result.Y = a.Y + b.Y;
    result.Z = a.Z + b.Z;
    
    return result;
}

inline Vector3 operator-(Vector3 a, Vector3 b)
{
    Vector3 result;
    
    result.X = a.X - b.X;
    result.Y = a.Y - b.Y;
    result.Z = a.Z - b.Z;
    
    return result;
}