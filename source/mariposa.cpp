#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef signed char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

struct win32OffscreenBuffer
{
    BITMAPINFO Info;
    void* Memory;
    int Width, Height;
    int Pitch;
    int BytesPerPixel;
};

// TODO: UNglobal these:
global_variable bool Running;
global_variable win32OffscreenBuffer GlobalBackbuffer;

struct Win32WindowDimensions
{
    int width, height;
};

Win32WindowDimensions Win32GetWindowDimensions(HWND window)
{
    Win32WindowDimensions result;
    
    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.width = clientRect.right - clientRect.left;
    result.height = clientRect.bottom - clientRect.top;
    
    return result;
}

internal void RenderGradient(win32OffscreenBuffer buffer, int xOffset, int yOffset)
{    
    uint8* row = (uint8*)buffer.Memory;
    for(int y = 0; y < buffer.Height; y++)
    {
        uint32* pixel = (uint32*)row;
        for(int x= 0; x < buffer.Width; x++)
        {
            // pixel in memory: BB GG RR CC
            uint8 blue = x + xOffset;
            uint8 green = y + yOffset;
            
            *pixel++ = blue | (green << 8);
        }
        
        row += buffer.Pitch;
    }
}

internal void Win32ResizeDIBSection(win32OffscreenBuffer* buffer, int width, int height)    // Device independent bitmap
{
    if(buffer->Memory)
    {
        VirtualFree(buffer->Memory, 0, MEM_RELEASE);
    }
    
    buffer->Width = width;
    buffer->Height = height;
    buffer->BytesPerPixel = 4;
    
    buffer->Info.bmiHeader.biSize = sizeof(buffer->Info.bmiHeader);
    buffer->Info.bmiHeader.biWidth = buffer->Width;
    buffer->Info.bmiHeader.biHeight = -buffer->Height;
    buffer->Info.bmiHeader.biPlanes = 1;
    buffer->Info.bmiHeader.biBitCount = 32;
    buffer->Info.bmiHeader.biCompression = BI_RGB;
    
    int bitMapMemorySize = (buffer->Width * buffer->Height) * buffer->BytesPerPixel;
    buffer->Memory = VirtualAlloc(0, bitMapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    
    buffer->Pitch = width * buffer->BytesPerPixel;
}

internal void Win32CopyBufferToWindow(HDC deviceContext, int windowWidth, int windowHeight, win32OffscreenBuffer buffer, int x, int y, int width, int height)
{
    // NOTE: int width and int height are unused
    StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight,
                                 0, 0, buffer.Width, buffer.Height,
                                 buffer.Memory,
                                 &buffer.Info,
                                 DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_SIZE:
        {
        } break;

        case WM_CLOSE:
        {
            Running = false;
        } break;

        case WM_DESTROY:
        {
            Running = false;
        } break;
        
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            
            Win32WindowDimensions dimensions = Win32GetWindowDimensions(window);
            
            Win32CopyBufferToWindow(deviceContext, dimensions.width, dimensions.height, GlobalBackbuffer, x, y, width, height);
            EndPaint(window, &paint);
        } break;

        default:
        {
            //OutputDebugStringA("default\n");
            result = DefWindowProc(window, message, wParam, lParam);
        } break;
    }

    return result;
}

INT WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR commandLine, INT showCode)
{
    WNDCLASSA windowClass = {};
    
    Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    windowClass.hInstance = instance;
    //WindowClass.hIcon;
    windowClass.lpszClassName = "MariposaWindowClass";
    
    if(RegisterClassA(&windowClass))
    {
        HWND window = CreateWindowExA(0, windowClass.lpszClassName, "Mariposa",
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                0, 0, instance, 0);
        if(window)
        {
            Running = true;
            int xOffset = 0, yOffset = 0;
            while(Running)
            {
                MSG message;
                while(PeekMessage(&message, window, 0, 0, PM_REMOVE))
                {
                    if(message.message == WM_QUIT)
                        Running = false;
                    
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }
                
                RenderGradient(GlobalBackbuffer, xOffset, yOffset);
                HDC deviceContext = GetDC(window);
                
                Win32WindowDimensions dimensions = Win32GetWindowDimensions(window);
                
                Win32CopyBufferToWindow(deviceContext, dimensions.width, dimensions.height, GlobalBackbuffer, 0, 0, dimensions.width, dimensions.height);
                ReleaseDC(window, deviceContext);
                
                xOffset += 2;
                yOffset++;
            }
        }
        else
        {
            // ASSERT: Window creation failed
        }
    }
    else
    {
        // ASSERT: Registering window class failed
    }

    return 0;
}
