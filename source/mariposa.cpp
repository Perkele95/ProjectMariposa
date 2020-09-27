#include <windows.h>
#include <Xinput.h>
#include <dsound.h>
#include <math.h>

#define internal static
#define local_persist static
#define global_variable static

#define PI32 3.14159265359

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef signed char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

typedef int32 bool32;

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

// TODO: UNglobal these:
global_variable bool32 GlobalRunning;
global_variable win32OffscreenBuffer GlobalBackbuffer;
global_variable IDirectSoundBuffer* GlobalSecondaryBuffer;

// ------------------------------------------------------------
// This is support for XInputGetState and XInputSetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_GET_STATE(x_Input_Get_State);
typedef X_INPUT_SET_STATE(x_Input_Set_State);
// Stub functions are safe guard against access violations
X_INPUT_GET_STATE(XInputGetStateStub){ return ERROR_DEVICE_NOT_CONNECTED; }
X_INPUT_SET_STATE(XInputSetStateStub){ return ERROR_DEVICE_NOT_CONNECTED; }

global_variable x_Input_Get_State* _XInputGetState = XInputGetStateStub;
global_variable x_Input_Set_State* _XInputSetState = XInputSetStateStub;
#define XInputGetState _XInputGetState
#define XInputSetState _XInputSetState

internal void Win32LoadXInput(void)
{
    HMODULE xInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!xInputLibrary)
    {
        // TODO: Log info library version
        HMODULE xInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
        
    if(xInputLibrary)
    {
        XInputGetState = (x_Input_Get_State*)GetProcAddress(xInputLibrary, "XInputGetState");
        XInputSetState = (x_Input_Set_State*)GetProcAddress(xInputLibrary, "XInputGetState");
    }
    else
    {
        // TODO: Log Error
    }
}
// ------------------------------------------------------------

// ------------------------------------------------------------
// This is support for DirectSoundCreate
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void Win32InitDirectSound(HWND window, int32 samplesPerSecond, int32 bufferSize)
{
    HMODULE dSoundlibrary = LoadLibraryA("dsound.dll");
    if(dSoundlibrary)
    {
        direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress(dSoundlibrary, "DirectSoundCreate");
        
        WAVEFORMATEX waveFormat = {};
        waveFormat.wFormatTag = WAVE_FORMAT_PCM;
        waveFormat.nChannels = 2;
        waveFormat.nSamplesPerSec = samplesPerSecond;
        waveFormat.wBitsPerSample = 16;
        waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
        waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
        waveFormat.cbSize = 0;
            
        IDirectSound* directSound;
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &directSound, 0)))
        {
            if(SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC bufferDesc = {};
                bufferDesc.dwSize = sizeof(bufferDesc);
                bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
                
                IDirectSoundBuffer* primaryBuffer;
                if(SUCCEEDED(directSound->CreateSoundBuffer(&bufferDesc, &primaryBuffer, 0)))
                {
                    if(SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
                    {
                        OutputDebugStringA("Primary DirectSound buffer created successfully!\n");
                    }
                    else
                    {
                        // TODO: Log error
                    }
                }
            }
            else
            {
                // TODO: Log error
            }
        }
        else
        {
            // TODO: Log error
        }
        
        DSBUFFERDESC bufferDesc = {};
        bufferDesc.dwSize = sizeof(bufferDesc);
        bufferDesc.dwFlags = 0;
        bufferDesc.dwBufferBytes = bufferSize;
        bufferDesc.lpwfxFormat = &waveFormat;
        
        if(SUCCEEDED(directSound->CreateSoundBuffer(&bufferDesc, &GlobalSecondaryBuffer, 0)))
        {
            OutputDebugStringA("Secondary DirectSound buffer created successfully!\n");
        }
    }
}
// ------------------------------------------------------------

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
    buffer->Memory = VirtualAlloc(0, bitMapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    
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
            bool32 wasDown = (lParam & (1 << 30) != 0);
            bool32 isDown = (lParam & (1 << 31) == 0);
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
                    // do
                }
            }
            bool32 altKeyDown = (lParam & (1 << 29));
            if(VKCode == VK_F4 && altKeyDown)
            {
                GlobalRunning = false;
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

struct Win32SoundOutput{
    int SamplesPerSecond;
    int BytesPerSample;
    int SuqareWaveFrequency;
    uint32 RunningSampleIndex;
    int WavePeriod;
    int HalfWavePeriod;
    int SecondaryBufferSize;
    int16 ToneVolume;
    int LatencySampleCount;
};

void Win32FillSoundBuffer(Win32SoundOutput* soundOutput, DWORD byteToLock, DWORD bytesToWrite)
{
    void* region1;
    DWORD region1size;
    void* region2;
    DWORD region2size;
    
    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(byteToLock, bytesToWrite, &region1, &region1size, &region2, &region2size, DSBLOCK_FROMWRITECURSOR)))
    {
        // TODO: ASSERT: assert that regionsizes are valid
        DWORD region1SampleCount = region1size / soundOutput->BytesPerSample;
        int16* sampleOut = (int16*)region1;
        for(DWORD sampleIndex = 0; sampleIndex < region1SampleCount; sampleIndex++)
        {
            float t = 2.0f * PI32 * (float)soundOutput->RunningSampleIndex / (float)soundOutput->WavePeriod;
            float sineValue = sinf(t);
            int16 sampleValue = (int16)(sineValue * soundOutput->ToneVolume);
            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;
            soundOutput->RunningSampleIndex++;
        }
        
        DWORD region2SampleCount = region2size / soundOutput->BytesPerSample;
        sampleOut = (int16*)region2;
        for(DWORD sampleIndex = 0; sampleIndex < region2SampleCount; sampleIndex++)
        {
            float t = 2.0f * PI32 * (float)soundOutput->RunningSampleIndex / (float)soundOutput->WavePeriod;
            float sineValue = sinf(t);
            int16 sampleValue = (int16)(sineValue * soundOutput->ToneVolume);
            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;
            soundOutput->RunningSampleIndex++;
        }
        
        GlobalSecondaryBuffer->Unlock(region1, region1size, region2, region2size);
    }
}

INT WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR commandLine, INT showCode)
{
    
    int64 perfCountFrequency;
    {
        LARGE_INTEGER perfCountFrequencyResult; 
        QueryPerformanceFrequency(&perfCountFrequencyResult);
        perfCountFrequency = perfCountFrequencyResult.QuadPart;
    }
    
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
        
        if(window)
        {
            HDC deviceContext = GetDC(window);
            
            // Note: Graphics test
            int xOffset = 0, yOffset = 0;
            
            // Note: Soundtest
            
            Win32SoundOutput soundOutput = {};
            
            soundOutput.SamplesPerSecond = 48000;
            soundOutput.SuqareWaveFrequency = 256;
            soundOutput.ToneVolume = 3000;
            soundOutput.WavePeriod = soundOutput.SamplesPerSecond / soundOutput.SuqareWaveFrequency;
            soundOutput.BytesPerSample = 2 * sizeof(int16);
            soundOutput.SecondaryBufferSize = soundOutput.SamplesPerSecond * soundOutput.BytesPerSample;
            soundOutput.LatencySampleCount = soundOutput.SamplesPerSecond / 15;
            
            Win32InitDirectSound(window, soundOutput.SamplesPerSecond, soundOutput.SecondaryBufferSize);
            Win32FillSoundBuffer(&soundOutput, 0, soundOutput.LatencySampleCount * soundOutput.BytesPerSample);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            
            GlobalRunning = true;
            
            LARGE_INTEGER lastCounter;
            QueryPerformanceCounter(&lastCounter);
            uint64 lastCycleCount = __rdtsc();
            
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
                        bool32 dpadUp = gamePad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                        bool32 dpadDown = gamePad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                        bool32 dpadLeft = gamePad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                        bool32 dpadRight = gamePad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                        bool32 padStart = gamePad->wButtons & XINPUT_GAMEPAD_START;
                        bool32 padBack = gamePad->wButtons & XINPUT_GAMEPAD_BACK;
                        bool32 leftThumb = gamePad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
                        bool32 rightThumb = gamePad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
                        bool32 leftShoulder = gamePad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                        bool32 rightShoulder = gamePad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                        bool32 aButton = gamePad->wButtons & XINPUT_GAMEPAD_A;
                        bool32 bButton = gamePad->wButtons & XINPUT_GAMEPAD_B;
                        bool32 xButton = gamePad->wButtons & XINPUT_GAMEPAD_X;
                        bool32 yButton = gamePad->wButtons & XINPUT_GAMEPAD_Y;
                        
                        int16 stickX = gamePad->sThumbLX;
                        int16 stickY = gamePad->sThumbLY;
                    }
                    else
                    {
                        // Controller not plugged in
                    }
                }
                
                RenderGradient(&GlobalBackbuffer, xOffset, yOffset);
                
                xOffset += 1;
                yOffset += 1;
                
                // DirectSound output test
                DWORD playCursor;
                DWORD writeCursor;
                if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
                {
                    DWORD byteToLock = ((soundOutput.RunningSampleIndex * soundOutput.BytesPerSample) % soundOutput.SecondaryBufferSize);
                    DWORD targetCursor = (playCursor + (soundOutput.LatencySampleCount * soundOutput.BytesPerSample)) % soundOutput.SecondaryBufferSize;
                    DWORD bytesToWrite;
                    if(byteToLock > targetCursor)
                    {
                        bytesToWrite = soundOutput.SecondaryBufferSize - byteToLock;
                        bytesToWrite += targetCursor;
                    }
                    else
                    {
                        bytesToWrite = targetCursor - byteToLock;
                    }
                    
                    Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite);
                }
                
                Win32WindowDimensions dimensions = Win32GetWindowDimensions(window);
                Win32CopyBufferToWindow(deviceContext, &GlobalBackbuffer, dimensions.width, dimensions.height);
                
                uint64 endCycleCount = __rdtsc();
                
                LARGE_INTEGER endCounter;
                QueryPerformanceCounter(&endCounter);
                
                uint64 cyclesElapsed = endCycleCount - lastCycleCount;
                int64 counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
                int32 msDeltaTime = (int32)((1000 * counterElapsed) / perfCountFrequency);
                int32 fps = (int32)(perfCountFrequency / counterElapsed);
                uint32 MCPF = (uint32)cyclesElapsed / 1000000;
                
                char buffer[256];
                wsprintfA(buffer, "ms/frame: %d, FPS: %d, MCPF: %d\n", msDeltaTime, fps, MCPF);
                OutputDebugStringA(buffer);
                
                lastCounter = endCounter;
                lastCycleCount = endCycleCount;
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
