#pragma once

#include <windows.h>

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef signed char int8;
typedef short int16;
typedef int int32;
typedef long long int64;
typedef int32 bool32;

typedef struct BMP_FILE_RESULT
{
    void* data;
    uint32 dataSize;
} BMP_FILE_RESULT;

#pragma pack(push, 1)
typedef struct BMP_HEADER
{
    uint16 FileType;
    uint32 FileSize;
    uint16 Reserved1;
    uint16 Reserved2;
    uint32 BitmapOffset;
    uint32 HeaderSize;
    int32 Width;
    int32 Height;
    uint16 Planes;
    uint16 BitsPerPixel;
} BMP_HEADER;
#pragma pack(pop)

inline static uint32 SafeTruncateUint32(uint64 value)
{
    if(!(value < 0xffffffff))
        DebugBreak();
    return (uint32)value;
}

// IMPLEMENTATION

static void FreeBMP(void* memory)
{
    VirtualFree(memory, 0, MEM_RELEASE);
}

static BMP_FILE_RESULT ReadBMP(const char* filename)
{
    BMP_FILE_RESULT result = {0};
    
    void* fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(fileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSize;
        if(GetFileSizeEx(fileHandle, &fileSize))
        {
            uint32 fileSize32 = SafeTruncateUint32(fileSize.QuadPart);
            result.data = VirtualAlloc(0, (size_t)fileSize.QuadPart, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if(result.data)
            {
                DWORD bytesRead;
                if(ReadFile(fileHandle, result.data, fileSize32, &bytesRead, 0) && fileSize32 == bytesRead)
                {
                    result.dataSize = fileSize32;
                }
                else
                {
                    FreeBMP(result.data);
                    result.data = 0;
                }
            }
            else
            {
                // TODO: Log Error
            }
        }
        else
        {
            // TODO: Log Error
        }
        
        CloseHandle(fileHandle);
    }
    else
    {
        // TODO: Log Error
    }
    
    return result;
}

uint8* LoadBMP(const char* filename, int32* texWidth, int32* texHeight)
{
    BMP_FILE_RESULT readResult = ReadBMP(filename);
    uint8* result = 0;
    
    if(readResult.dataSize > 0)
    {
        BMP_HEADER* header = (BMP_HEADER*) readResult.data;
        *texWidth = header->Width;
        *texHeight = header->Height;
        
        result = (uint8*) readResult.data + header->BitmapOffset;
    }
    return result;
}