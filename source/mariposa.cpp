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

// TODO: UNglobal these:
global_variable bool Running;

global_variable BITMAPINFO BitMapInfo;
global_variable void* BitMapMemory;
global_variable int BitmapWidth, BitmapHeight;
global_variable int WindowWidth, WindowHeight;
global_variable int bytesPerPixel = 4;

internal void RenderGradient(int xOffset, int yOffset)
{
    int width = BitmapWidth;
    int height = BitmapHeight;
    
    int pitch = width * bytesPerPixel;
    uint8* row = (uint8*)BitMapMemory;
    for(int y = 0; y < BitmapHeight; y++)
    {
        uint32* pixel = (uint32*)row;
        for(int x= 0; x < BitmapWidth; x++)
        {
            // pixel in memory: BB GG RR CC
            uint8 blue = x + xOffset;
            uint8 green = y + yOffset;
            
            *pixel++ = blue | (green << 8);
        }
        
        row += pitch;
    }
}

internal void Win32ResizeDIBSection(int width, int height)    // Device independent bitmap
{
    if(BitMapMemory)
    {
        VirtualFree(&BitMapMemory, 0, MEM_RELEASE);
    }
    
    BitmapWidth = width;
    BitmapHeight = height;
    
    BitMapInfo.bmiHeader.biSize = sizeof(BitMapInfo.bmiHeader);
    BitMapInfo.bmiHeader.biWidth = BitmapWidth;
    BitMapInfo.bmiHeader.biHeight = -BitmapHeight;
    BitMapInfo.bmiHeader.biPlanes = 1;
    BitMapInfo.bmiHeader.biBitCount = 32;
    BitMapInfo.bmiHeader.biCompression = BI_RGB;
    
    int bitMapMemorySize = (BitmapWidth * BitmapHeight) * bytesPerPixel;
    BitMapMemory = VirtualAlloc(0, bitMapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32UpdateWindow(HDC deviceContext, RECT* windowRect, int x, int y, int width, int height)
{
    int WindowWidth = windowRect->right - windowRect->left;
    int WindowHeight = windowRect->bottom - windowRect->top;
    
    StretchDIBits(deviceContext, 0, 0, BitmapWidth, BitmapHeight,
                                 0, 0, WindowWidth, WindowHeight,
                                 BitMapMemory,
                                 &BitMapInfo,
                                 DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_SIZE:
        {
            RECT windowRect;
            GetClientRect(window, &windowRect);
            int height = windowRect.bottom - windowRect.top;
            int width = windowRect.right - windowRect.left;
            Win32ResizeDIBSection(width, height);
            OutputDebugStringA("WM_SIZE\n");
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
            
            RECT windowRect;
            GetClientRect(window, &windowRect);
            
            Win32UpdateWindow(deviceContext, &windowRect, x, y, width, height);
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

    //windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
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
                
                RenderGradient(xOffset, yOffset);
                HDC deviceContext = GetDC(window);
                
                RECT clientRect;
                GetClientRect(window, &clientRect);
                int WindowWidth = clientRect.right - clientRect.left;
                int WindowHeight = clientRect.bottom - clientRect.top;
                Win32UpdateWindow(deviceContext, &clientRect, 0, 0, WindowWidth, WindowHeight);
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
