#define internal static
#define local_persist static
#define global_variable static

#define PI32 3.14159265359f

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef signed char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

typedef int32 bool32;

#include "mariposa.cpp"

#include <Xinput.h>
#include <dsound.h>
#include <malloc.h>
#include <stdio.h>

#include "win32_mariposa.h"

// TODO: UNglobal these:
global_variable bool32 GlobalRunning;
global_variable win32OffscreenBuffer GlobalBackbuffer;
global_variable IDirectSoundBuffer* GlobalSecondaryBuffer;
global_variable int64 GlobalPerfCountFrequency;

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

internal debug_read_file_result DEBUG_PlatformReadEntireFile(char* fileName)
{
    debug_read_file_result result = {};
    
    void* fileHandle = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
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
                    DEBUG_PlatformFreeFileMemory(result.data);
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

internal void DEBUG_PlatformFreeFileMemory(void* memory)
{
    VirtualFree(memory, 0, MEM_RELEASE);
}

internal bool32 DEBUG_PlatformWriteEntireFile(char* fileName, debug_read_file_result* readData)
{
    bool32 result = false;
    void* fileHandle = CreateFileA(fileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        if(WriteFile(fileHandle, readData->data, readData->dataSize, &bytesWritten, 0))
        {
            result = (readData->dataSize == bytesWritten);
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

internal void Win32LoadXInput(void)
{
    HMODULE xInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!xInputLibrary)
    {
        // TODO: Log info library version
        xInputLibrary = LoadLibraryA("xinput1_3.dll");
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
            
        IDirectSound* directSound = {};
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
            MP_ASSERT(!"Keyboard input came in through a non-dispatch message!");
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

void Win32ClearSoundbuffer(Win32SoundOutput* soundOutput)
{
    void* region1;
    DWORD region1size;
    void* region2;
    DWORD region2size;
    
    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(0, soundOutput->SecondaryBufferSize, &region1, &region1size, &region2, &region2size, DSBLOCK_FROMWRITECURSOR)))
    {
        uint8* destSample = (uint8*)region1;
        for(DWORD byteIndex = 0; byteIndex < region1size; byteIndex++)
        {
            *destSample++ = 0;
        }
        
        destSample = (uint8*)region2;
        for(DWORD byteIndex = 0; byteIndex < region2size; byteIndex++)
        {
            *destSample++ = 0;
        }
        
        GlobalSecondaryBuffer->Unlock(region1, region1size, region2, region2size);
    }
}

void Win32FillSoundBuffer(Win32SoundOutput* soundOutput, DWORD byteToLock, DWORD bytesToWrite, MP_SOUNDOUTPUTBUFFER* sourceBuffer)
{
    void* region1;
    DWORD region1size;
    void* region2;
    DWORD region2size;
    
    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(byteToLock, bytesToWrite, &region1, &region1size, &region2, &region2size, DSBLOCK_FROMWRITECURSOR)))
    {
        // TODO: ASSERT: assert that regionsizes are valid
        DWORD region1SampleCount = region1size / soundOutput->BytesPerSample;
        int16* destSample = (int16*)region1;
        int16* sourceSample = sourceBuffer->Samples;
        
        for(DWORD sampleIndex = 0; sampleIndex < region1SampleCount; sampleIndex++)
        {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;
            soundOutput->RunningSampleIndex++;
        }
        
        DWORD region2SampleCount = region2size / soundOutput->BytesPerSample;
        destSample = (int16*)region2;
        for(DWORD sampleIndex = 0; sampleIndex < region2SampleCount; sampleIndex++)
        {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;
            soundOutput->RunningSampleIndex++;
        }
        
        GlobalSecondaryBuffer->Unlock(region1, region1size, region2, region2size);
    }
}

internal void Win32ProcessKeyboardEvent(MP_BUTTON_STATE* state, bool32 isDown)
{
    MP_ASSERT(state->EndedDown != isDown);
    state->EndedDown = isDown;
    state->HalfTransitionCount++;
}

internal float Win32ProcessXInputStickPosition(short value, short deadZoneThreshold)
{
    float result = 0;
    
    if(value < -deadZoneThreshold)
        result = (float)(value + deadZoneThreshold) / (32768.0f - deadZoneThreshold);
    else if(value > deadZoneThreshold)
        result = (float)(value - deadZoneThreshold) / (32767.0f - deadZoneThreshold);
       
    return result;
}

internal void Win32ProcessPendingMessages(MP_CONTROLLER_INPUT* keyboardController)
{
    MSG message;
    
    while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
    {
        switch(message.message)
        {
            case WM_QUIT:
            {
                GlobalRunning = false;
            } break;
            
            case WM_LBUTTONDOWN:
            {
                // Left mouse button pressed
            } break;
            case WM_RBUTTONDOWN:
            {
                // Right mouse button pressed
            } break;
            
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                uint32 VKCode = (uint32)message.wParam;
                bool32 wasDown = ((message.lParam & (1 << 30)) != 0);
                bool32 isDown = ((message.lParam & (1 << 31)) == 0);
                if(wasDown != isDown)
                {
                    if(VKCode == 'W')
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->Up, isDown);
                    }
                    else if(VKCode == 'A')
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->Left, isDown);
                    }
                    else if(VKCode == 'S')
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->Down, isDown);
                    }
                    else if(VKCode == 'D')
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->Right, isDown);
                    }
                    else if(VKCode == 'Q')
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->Q, isDown);
                    }
                    else if(VKCode == 'E')
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->E, isDown);
                    }
                    else if(VKCode == 'F')
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->F, isDown);
                    }
                    else if(VKCode == 'G')
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->G, isDown);
                    }
                    else if(VKCode == 'C')
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->C, isDown);
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
                        Win32ProcessKeyboardEvent(&keyboardController->Jump, isDown);
                    }
                    else if(VKCode == VK_ESCAPE)
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->Escape, isDown);
                    }
                    else if(VKCode == VK_CONTROL)
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->Crouch, isDown);
                    }
                    else if(VKCode == VK_SHIFT)
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->Sprint, isDown);
                    }
                    else
                    {
                        // do
                    }
                }
                bool32 altKeyDown = (message.lParam & (1 << 29));
                if(VKCode == VK_F4 && altKeyDown)
                {
                    GlobalRunning = false;
                }
            } break;
            
            default:
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            } break;
        }
    }
}

internal void Win32ProcessXInputDigitalButton(DWORD XInputButtonState, DWORD buttonBit, MP_BUTTON_STATE* oldState, MP_BUTTON_STATE* newState)
{
    newState->EndedDown = (XInputButtonState & buttonBit) == buttonBit;
    // TODO: Check if ? 1 : 0 is needed
    newState->HalfTransitionCount = (oldState->EndedDown != newState->EndedDown) ? 1 : 0;
}

inline internal LARGE_INTEGER Win32GetClockValue(void)
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

inline internal float Win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    float result = (float)(end.QuadPart - start.QuadPart) / (float)GlobalPerfCountFrequency;
    return result;
}

INT __stdcall WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR commandLine, INT showCode)
{
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    
    uint32 desiredSchedulerMS = 1;
    bool32 sleepIsGranular = (timeBeginPeriod(desiredSchedulerMS) == TIMERR_NOERROR);
    
    Win32LoadXInput();
    
    WNDCLASSA windowClass = {};
    
    Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

    windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    windowClass.hInstance = instance;
    //WindowClass.hIcon;
    windowClass.lpszClassName = "MariposaWindowClass";
    
    // TODO: refresh rate needs to be queried instead of set to a fixed value
    uint32 refreshRate = 300;
    float expectedDeltaTime = 1.0f / (float)refreshRate;
    
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
            
            Win32SoundOutput soundOutput = {};
            
            soundOutput.SamplesPerSecond = 48000;
            soundOutput.BytesPerSample = 2 * sizeof(int16);
            soundOutput.SecondaryBufferSize = soundOutput.SamplesPerSecond * soundOutput.BytesPerSample;
            soundOutput.LatencySampleCount = soundOutput.SamplesPerSecond / 15;        
                
            Win32InitDirectSound(window, soundOutput.SamplesPerSecond, soundOutput.SecondaryBufferSize);
            Win32ClearSoundbuffer(&soundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            
            GlobalRunning = true;
            
            int16* samples = (int16*)VirtualAlloc(0, soundOutput.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            
            #if MP_INTERNAL
                LPVOID baseAddress = (LPVOID)TeraBytes(2);
            #else
                LPVOID baseAddress = 0;
            #endif
            
            MP_MEMORY gameMemory = {};
            gameMemory.PermanentStorageSize = MegaBytes(64);
            gameMemory.TransientStorageSize = GigaBytes(1);
            
            uint64 totalSize = gameMemory.PermanentStorageSize + gameMemory.TransientStorageSize;
            gameMemory.PermanentStorage = VirtualAlloc(baseAddress, (size_t)totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            
            gameMemory.TransientStorage = ((uint8*)gameMemory.PermanentStorage + gameMemory.PermanentStorageSize);
            
            if(gameMemory.PermanentStorage && gameMemory.TransientStorage && samples)
            {            
                MP_INPUT input[2] = {};
                MP_INPUT* newInput = &input[0];
                MP_INPUT* oldInput = &input[1];
                    
                LARGE_INTEGER lastCounter = Win32GetClockValue();
                
                uint64 lastCycleCount = __rdtsc();
                
                while(GlobalRunning)
                {                    
                    MP_CONTROLLER_INPUT* oldKeyboardController = GetController(oldInput, 0);
                    MP_CONTROLLER_INPUT* newKeyboardController = GetController(newInput, 0);
                    *newKeyboardController = {};
                    newKeyboardController->IsConntected = true;
                    
                    for(int index = 0; index < ArrayCount(newKeyboardController->Buttons); index++)
                    {
                        newKeyboardController->Buttons[index].EndedDown = oldKeyboardController->Buttons[index].EndedDown;
                    }
                    
                    Win32ProcessPendingMessages(newKeyboardController);
                    
                    DWORD maxControllerCount = XUSER_MAX_COUNT;
                    if(maxControllerCount > ArrayCount(newInput->Controllers) - 1)
                    {
                        maxControllerCount = ArrayCount(newInput->Controllers) - 1;
                    }
                    
                    for(DWORD controllerIndex = 0; controllerIndex < maxControllerCount; controllerIndex++)
                    {
                        DWORD ourControllerIndex = controllerIndex + 1;
                        MP_CONTROLLER_INPUT* oldController = GetController(oldInput, ourControllerIndex);
                        MP_CONTROLLER_INPUT* newController = GetController(newInput, ourControllerIndex);
                        
                        XINPUT_STATE controllerState;
                        if(XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
                        {
                            newController->IsConntected = true;
                            XINPUT_GAMEPAD* gamePad = &controllerState.Gamepad;
                            
                            // TODO: Min/Max macro
                            newController->StickAverageX = Win32ProcessXInputStickPosition(gamePad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                            newController->StickAverageY = Win32ProcessXInputStickPosition(gamePad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                            
                            if(newController->StickAverageX != 0.0f || newController->StickAverageY != 0.0f)
                                newController->IsAnalog = true;
                            
                            if(gamePad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                            {
                                newController->StickAverageY = 1.0f;
                                newController->IsAnalog = false;
                            }
                            if(gamePad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                            {
                                newController->StickAverageY = -1.0f;
                                newController->IsAnalog = false;
                            }
                            if(gamePad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                            {
                                newController->StickAverageX = -1.0f;
                                newController->IsAnalog = false;
                            }
                            if(gamePad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                            {
                                newController->StickAverageX = 1.0f;
                                newController->IsAnalog = false;
                            }
                            
                            Win32ProcessXInputDigitalButton(gamePad->wButtons, XINPUT_GAMEPAD_A,
                                                            &oldController->Jump, &newController->Jump);
                            Win32ProcessXInputDigitalButton(gamePad->wButtons, XINPUT_GAMEPAD_B,
                                                            &oldController->C, &newController->C);
                            Win32ProcessXInputDigitalButton(gamePad->wButtons, XINPUT_GAMEPAD_X,
                                                            &oldController->F, &newController->F);
                            Win32ProcessXInputDigitalButton(gamePad->wButtons, XINPUT_GAMEPAD_Y,
                                                            &oldController->G, &newController->G);
                            
                            Win32ProcessXInputDigitalButton(gamePad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER,
                                                            &oldController->Q, &newController->Q);
                            Win32ProcessXInputDigitalButton(gamePad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                                            &oldController->E, &newController->E);
                            
                            Win32ProcessXInputDigitalButton(gamePad->wButtons, XINPUT_GAMEPAD_START,
                                                            &oldController->Escape, &newController->Escape);
                            //Win32ProcessXInputDigitalButton(gamePad->wButtons, XINPUT_GAMEPAD_BACK,
                            //                                &oldController->Escape, &newController->Escape);
                            
                            //bool32 leftThumb = gamePad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
                            //bool32 rightThumb = gamePad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
                        }
                        else
                        {
                            newController->IsConntected = true;
                        }
                    }
                    
                    // DirectSound output test
                    DWORD byteToLock = 0;
                    DWORD targetCursor = 0;
                    DWORD bytesToWrite = 0;
                    DWORD playCursor = 0;
                    DWORD writeCursor = 0;
                    bool32 isSoundValid = false;
                    
                    if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
                    {
                        byteToLock = ((soundOutput.RunningSampleIndex * soundOutput.BytesPerSample) % soundOutput.SecondaryBufferSize);
                        targetCursor = (playCursor + soundOutput.LatencySampleCount * soundOutput.BytesPerSample) % soundOutput.SecondaryBufferSize;
                        bytesToWrite;
                        if(byteToLock > targetCursor)
                        {
                            bytesToWrite = soundOutput.SecondaryBufferSize - byteToLock;
                            bytesToWrite += targetCursor;
                        }
                        else
                        {
                            bytesToWrite = targetCursor - byteToLock;
                        }
                        
                        isSoundValid = true;
                    }
                    
                    MP_SOUNDOUTPUTBUFFER soundBuffer = {};
                    soundBuffer.SamplesPerSecond = soundOutput.SamplesPerSecond;
                    soundBuffer.SampleCount = bytesToWrite / soundOutput.BytesPerSample;
                    soundBuffer.Samples = samples;
                    
                    MP_OFFSCREENBUFFER buffer = {};
                    buffer.Memory = GlobalBackbuffer.Memory;
                    buffer.Width = GlobalBackbuffer.Width;
                    buffer.Height = GlobalBackbuffer.Height;
                    buffer.Pitch = GlobalBackbuffer.Pitch;
                    GameUpdateAndRender(&gameMemory, newInput, &soundBuffer, &buffer);
                    
                    if(isSoundValid)
                    {                    
                        Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite, &soundBuffer);
                    }
                    
                    LARGE_INTEGER workCounter = Win32GetClockValue();
                    float workSecondsElapsed = Win32GetSecondsElapsed(lastCounter, workCounter);
                    
                    float secondsElapsedForFrame = workSecondsElapsed;
                    if(secondsElapsedForFrame < expectedDeltaTime)
                    {
                        while(secondsElapsedForFrame < expectedDeltaTime)
                        {
                            if(sleepIsGranular)
                            {
                                DWORD sleepMS = (DWORD)(1000.0f * (expectedDeltaTime - secondsElapsedForFrame));
                                Sleep(sleepMS);
                            }
                            secondsElapsedForFrame = Win32GetSecondsElapsed(lastCounter, Win32GetClockValue());
                        }
                    }
                    else
                    {
                        // Log: Missed frame!
                    }
                    
                    Win32WindowDimensions dimensions = Win32GetWindowDimensions(window);
                    Win32CopyBufferToWindow(deviceContext, &GlobalBackbuffer, dimensions.width, dimensions.height);
                    
                    // TODO: Swap macro
                    MP_INPUT* temp = newInput;
                    newInput = oldInput;
                    oldInput = temp;
                    
                    LARGE_INTEGER endCounter = Win32GetClockValue();
                    
                    #if 1
                    double msDeltaTime = 1000.0f * Win32GetSecondsElapsed(lastCounter, endCounter);
                    #endif
                    lastCounter = endCounter;
                    
                    uint64 endCycleCount = __rdtsc();
                    uint64 cyclesElapsed = endCycleCount - lastCycleCount;
                    lastCycleCount = endCycleCount;
                    
                    #if 1
                    double fps = 1000.0f / msDeltaTime;
                    double MCPF = (double)cyclesElapsed / 1000000.0f;
                    
                    char stringBuffer[256];
                    _snprintf_s(stringBuffer, sizeof(stringBuffer), "ms/frame: %.02f, FPS: %.02f, MCPF: %.02f\n", msDeltaTime, fps, MCPF);
                    OutputDebugStringA(stringBuffer);
                    #endif
                }
            }
            else
            {
                // Log error: Game or samples failed to init
            }
        }
        else
        {
            // Log error: Window creation failed
        }
    }
    else
    {
        // Log error: Registering window class failed
    }

    return 0;
}
