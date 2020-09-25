#include <windows.h>
#include <Xinput.h>

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
};

struct Win32WindowDimensions
{
    int width, height;
};

// ------------------------------------------------------------
// This is support for XInputGetState and XInputSetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_GET_STATE(x_Input_Get_State);
typedef X_INPUT_SET_STATE(x_Input_Set_State);
// Stub functions are safe guard against access violations
X_INPUT_GET_STATE(XInputGetStateStub){ return 0; }
X_INPUT_SET_STATE(XInputSetStateStub){ return 0; }

global_variable x_Input_Get_State* _XInputGetState = XInputGetStateStub;
global_variable x_Input_Set_State* _XInputSetState = XInputSetStateStub;
#define XInputGetState _XInputGetState
#define XInputSetState _XInputSetState

internal void Win32LoadXInput(void)
{
    HMODULE xInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(xInputLibrary)
    {
        XInputGetState = (x_Input_Get_State*)GetProcAddress(xInputLibrary, "XInputGetState");
        XInputSetState = (x_Input_Set_State*)GetProcAddress(xInputLibrary, "XInputGetState");
    }
}
// ------------------------------------------------------------

// TODO: UNglobal these:
global_variable bool GlobalRunning;
global_variable win32OffscreenBuffer GlobalBackbuffer;

internal Win32WindowDimensions Win32GetWindowDimensions(HWND window)
{
    Win32WindowDimensions result;
    
    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.width = clientRect.right - clientRect.left;
    result.height = clientRect.bottom - clientRect.top;
    
    return result;
}

internal void RenderGradient(win32OffscreenBuffer* buffer, int xOffset, int yOffset)
{    
    uint8* row = (uint8*)buffer->Memory;
    for(int y = 0; y < buffer->Height; y++)
    {
        uint32* pixel = (uint32*)row;
        for(int x= 0; x < buffer->Width; x++)
        {
            // pixel in memory: BB GG RR CC
            uint8 blue = x + xOffset;
            uint8 green = y + yOffset;
            
            *pixel++ = blue | (green << 8);
        }
        
        row += buffer->Pitch;
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
    int bytesPerPixel = 4;
    
    buffer->Info.bmiHeader.biSize = sizeof(buffer->Info.bmiHeader);
    buffer->Info.bmiHeader.biWidth = buffer->Width;
    buffer->Info.bmiHeader.biHeight = -buffer->Height;
    buffer->Info.bmiHeader.biPlanes = 1;
    buffer->Info.bmiHeader.biBitCount = 32;
    buffer->Info.bmiHeader.biCompression = BI_RGB;
    
    int bitMapMemorySize = (buffer->Width * buffer->Height) * bytesPerPixel;
    buffer->Memory = VirtualAlloc(0, bitMapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    
    buffer->Pitch = width * bytesPerPixel;
}

internal void Win32CopyBufferToWindow(HDC deviceContext, win32OffscreenBuffer* buffer, int windowWidth, int windowHeight)
{
    // NOTE: int width and int height are unused
    StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight,
                                 0, 0, buffer->Width, buffer->Height,
                                 buffer->Memory,
                                 &buffer->Info,
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
            GlobalRunning = false;
        } break;

        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;
        
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 VKCode = wParam;
            bool wasDown = (lParam & (1 << 30) != 0);
            bool isDown = (lParam & (1 << 31) == 0);
            if(wasDown != isDown)
            {
                if(VKCode == 'W')
                {
                    // do
                }
                else if(VKCode == 'A')
                {
                    // do
                }
                else if(VKCode == 'S')
                {
                    // do
                }
                else if(VKCode == 'D')
                {
                    // do
                }
                else if(VKCode == 'Q')
                {
                    // do
                }
                else if(VKCode == 'E')
                {
                    // do
                }
                else if(VKCode == VK_UP)
                {
                    // do
                }
                else if(VKCode == VK_DOWN)
                {
                    // do
                }
                else if(VKCode == VK_LEFT)
                {
                    // do
                }
                else if(VKCode == VK_RIGHT)
                {
                    // do
                }
                else if(VKCode == VK_SPACE)
                {
                    // do
                }
                else if(VKCode == VK_ESCAPE)
                {
                    // do
                }
                else if(VKCode == VK_CONTROL)
                {
                    // do
                }
                else if(VKCode == VK_SHIFT)
                {
                    // do
                }
                else
                {
                    // do default
                }
            }
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);
            
            Win32WindowDimensions dimensions = Win32GetWindowDimensions(window);
            
            Win32CopyBufferToWindow(deviceContext, &GlobalBackbuffer, dimensions.width, dimensions.height);
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
    Win32LoadXInput();
    
    WNDCLASSA windowClass = {};
    
    Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

    windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
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
                                
        HDC deviceContext = GetDC(window);
                
        if(window)
        {
            GlobalRunning = true;
            int xOffset = 0, yOffset = 0;
            while(GlobalRunning)
            {
                MSG message;
                while(PeekMessage(&message, window, 0, 0, PM_REMOVE))
                {
                    if(message.message == WM_QUIT)
                        GlobalRunning = false;
                    
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }
                
                for(int controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++)
                {
                    XINPUT_STATE controllerState;
                    if(XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
                    {
                        // Controller plugged in
                        XINPUT_GAMEPAD* gamePad = &controllerState.Gamepad;
                        bool dpadUp = gamePad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                        bool dpadDown = gamePad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                        bool dpadLeft = gamePad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                        bool dpadRight = gamePad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                        bool padStart = gamePad->wButtons & XINPUT_GAMEPAD_START;
                        bool padBack = gamePad->wButtons & XINPUT_GAMEPAD_BACK;
                        bool leftThumb = gamePad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
                        bool rightThumb = gamePad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
                        bool leftShoulder = gamePad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                        bool rightShoulder = gamePad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                        bool aButton = gamePad->wButtons & XINPUT_GAMEPAD_A;
                        bool bButton = gamePad->wButtons & XINPUT_GAMEPAD_B;
                        bool xButton = gamePad->wButtons & XINPUT_GAMEPAD_X;
                        bool yButton = gamePad->wButtons & XINPUT_GAMEPAD_Y;
                        
                        int16 stickX = gamePad->sThumbLX;
                        int16 stickY = gamePad->sThumbLY;
                    }
                    else
                    {
                        // Controller not plugged in
                    }
                }
                
                RenderGradient(&GlobalBackbuffer, xOffset, yOffset);
                
                Win32WindowDimensions dimensions = Win32GetWindowDimensions(window);
                Win32CopyBufferToWindow(deviceContext, &GlobalBackbuffer, dimensions.width, dimensions.height);
                
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
