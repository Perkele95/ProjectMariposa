#include "mariposa.h"

#include <Xinput.h>
#include <dsound.h>
#include <malloc.h>
#include <stdio.h>
#include <windowsx.h>

#include "win32_mariposa.h"

#include "mp_vulkan.cpp"

// TODO: UNglobal these:
global_variable bool32 GlobalRunning;
global_variable bool32 GlobalPause;
global_variable win32OffscreenBuffer GlobalBackbuffer;
global_variable IDirectSoundBuffer* GlobalSecondaryBuffer;
global_variable int64 GlobalPerfCountFrequency;
global_variable WINDOWPLACEMENT GlobalWindowPos = { sizeof(GlobalWindowPos) };

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

internal void ConcatenateStrings(uint64 aCount, char* a, uint64 bCount, char* b, uint64 destCount, char* dest)
{
    for(int i = 0; i < aCount; i++)
    {
        *dest++ = *a++;
    }
    for(int i = 0; i < bCount; i++)
    {
        *dest++ = *b++;
    }
    
    *dest++ = 0;
}

internal void Win32GetExeFilename(Win32State* state)
{
    DWORD sizeOffilename = GetModuleFileNameA(0, state->ExeFilename, sizeof(state->ExeFilename));
    state->OnePastLastSlash = state->ExeFilename;
    for(char* scan = state->ExeFilename; *scan; scan++)
    {
        if(*scan == '\\')
            state->OnePastLastSlash = scan + 1;
    }
}

internal int StringLength(char* string)
{
    int charCount = 0;
    while(*string++)
    {
        charCount++;
    }
    return charCount;
}

internal void Win32BuildEXEFilepathName(Win32State* state, char* filename, char* dest, int destCount)
{
    ConcatenateStrings(state->OnePastLastSlash - state->ExeFilename, state->ExeFilename,
                        StringLength(filename), filename, 
                        destCount, dest);
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    VirtualFree(memory, 0, MEM_RELEASE);
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result result = {};
    
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
                    DEBUGPlatformFreeFileMemory(thread, result.data);
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

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    bool32 result = false;
    void* fileHandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
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

inline FILETIME Win32GetLastWriteTime(char* filename)
{
    FILETIME lastWriteTime = {};
    _WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    
    if(GetFileAttributesExA(filename, GetFileExInfoStandard, &fileInfo))
    {
        lastWriteTime = fileInfo.ftLastWriteTime;
    }
    
    return lastWriteTime;
}

internal Win32GameCode Win32LoadGameCode(char* sourceDLLName, char* tempDLLName, char* lockfilename)
{
    Win32GameCode result = {};
    
    WIN32_FILE_ATTRIBUTE_DATA data;
    if(!GetFileAttributesExA(lockfilename, GetFileExInfoStandard, &data))
    {
        result.DLLLastWriteTime = Win32GetLastWriteTime(sourceDLLName);
        
        CopyFileA(sourceDLLName, tempDLLName, FALSE);
        result.DLL = LoadLibraryA(tempDLLName);
        if(result.DLL)
        {
            result.UpdateAndRender = (game_update_and_render*)GetProcAddress(result.DLL, "GameUpdateAndRender");
            result.GetSoundSamples = (get_sound_samples*)GetProcAddress(result.DLL, "GetSoundSamples");
            
            result.IsValid = (result.UpdateAndRender && result.GetSoundSamples);
        }
    }
    
    if(!result.IsValid)
    {
        result.UpdateAndRender = 0;
        result.GetSoundSamples = 0;
    }
    
    return result;
}

internal void Win32UnloadGameCode(Win32GameCode* gameCode)
{
    if(gameCode->DLL)
    {
        FreeLibrary(gameCode->DLL);
        gameCode->DLL = 0;
    }
    
    gameCode->IsValid = false;
    gameCode->UpdateAndRender = 0;
    gameCode->GetSoundSamples = 0;
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
        bufferDesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
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
    buffer->bytesPerPixel = bytesPerPixel;
    
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
            // TODO: Remove if this feature is unecessary
            #if 0
            if(wParam)
                SetLayeredWindowAttributes(window, 0xFFFFFF, 0x9F, LWA_COLORKEY);
            else
                SetLayeredWindowAttributes(window, 0xFFFFFF, 0x9F, LWA_ALPHA);
            #endif
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
    if(state->EndedDown != isDown)
    {
        state->EndedDown = isDown;
        state->HalfTransitionCount++;
    }
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

internal void Win32GetInputFileLocation(Win32State* state, bool32 isInputStream, int slotIndex, char* dest, int destCount)
{
    char temp[64];
    wsprintfA(temp, "input_recording_%s_%d.mpr", (isInputStream ? "input" : "state"), slotIndex);
    Win32BuildEXEFilepathName(state, temp, dest, destCount);
}

internal Win32ReplayBuffer* Win32GetReplayBuffer(Win32State* state, uint32 index)
{
    MP_ASSERT(index < ArrayCount(state->ReplayBuffers))
    Win32ReplayBuffer* result = &state->ReplayBuffers[index];
    return result;
}

internal void Win32BeginInputRecording(Win32State* state, int inputRecordingIndex)
{
    Win32ReplayBuffer* replayBuffer = Win32GetReplayBuffer(state, inputRecordingIndex);
    if(replayBuffer->Data)
    {
        state->InputRecordingIndex = inputRecordingIndex;
        char filename[MAX_PATH];
        Win32GetInputFileLocation(state, true, inputRecordingIndex, filename, sizeof(filename));
        state->RecordingHandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
        
        //state->RecordingHandle = replayBuffer->FileHandle;
        #if 0
        LARGE_INTEGER filePos;
        filePos.QuadPart = state->TotalSize;
        SetFilePointerEx(state->RecordingHandle, filePos, 0, FILE_BEGIN);
        #endif
        memcpy(replayBuffer->Data, state->GameMemoryBlock, state->TotalSize);
    }
}

internal void Win32EndInputRecording(Win32State* state)
{
    CloseHandle(state->RecordingHandle);
    state->InputRecordingIndex = 0;
}

internal void Win32BeginInputPlayback(Win32State* state, int inputPlaybackIndex)
{
    Win32ReplayBuffer* replayBuffer = Win32GetReplayBuffer(state, inputPlaybackIndex);
    if(replayBuffer->Data)
    {
        state->InputPlaybackIndex = inputPlaybackIndex;
        char filename[MAX_PATH];
        Win32GetInputFileLocation(state, true, inputPlaybackIndex, filename, sizeof(filename));
        state->PlaybackHandle = CreateFileA(filename, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
        
        //state->PlaybackHandle = replayBuffer->FileHandle;
        #if 0
        LARGE_INTEGER filePos;
        filePos.QuadPart = state->TotalSize;
        SetFilePointerEx(state->PlaybackHandle, filePos, 0, FILE_BEGIN);
        #endif
        memcpy(state->GameMemoryBlock, replayBuffer->Data, state->TotalSize);
    }
}

internal void Win32EndInputPlayback(Win32State* state)
{
    CloseHandle(state->PlaybackHandle);
    state->InputPlaybackIndex = 0;
}

internal void Win32RecordInput(Win32State* state, MP_INPUT* newInput)
{
    DWORD bytesWritten = 0;
    WriteFile(state->RecordingHandle, newInput, sizeof(*newInput), &bytesWritten, 0);
}

internal void Win32PlaybackInput(Win32State* state, MP_INPUT* newInput)
{
    DWORD bytesRead = 0;
    if(ReadFile(state->PlaybackHandle, newInput, sizeof(*newInput), &bytesRead, 0))
    {
        if(bytesRead == 0)
        {
            int playbackIndex = state->InputPlaybackIndex;
            Win32EndInputPlayback(state);
            Win32BeginInputPlayback(state, playbackIndex);
            ReadFile(state->PlaybackHandle, newInput, sizeof(*newInput), &bytesRead, 0);
        }
    }
}

internal void Win32ToggleFullScreen(HWND window)
{
    // Thanks to Raymond Chan we can do this:
    // https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
    DWORD style = GetWindowLong(window, GWL_STYLE);
    if (style & WS_OVERLAPPEDWINDOW)
    {
        HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(window, &GlobalWindowPos) && GetMonitorInfoA(monitor, &mi))
        {
            SetWindowLong(window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(window, HWND_TOP,
                        mi.rcMonitor.left, mi.rcMonitor.top,
                        mi.rcMonitor.right - mi.rcMonitor.left,
                        mi.rcMonitor.bottom - mi.rcMonitor.top,
                        SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window, &GlobalWindowPos);
        SetWindowPos(window, NULL, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                    SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

internal void Win32ProcessPendingMessages(Win32State* state , MP_CONTROLLER_INPUT* keyboardController, MP_MOUSE_INPUT* mouseInput)
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
            
            case WM_MOUSEMOVE:
            {
                mouseInput->X = GET_X_LPARAM(message.lParam);
                mouseInput->Y = GET_Y_LPARAM(message.lParam);
            } break;
            case WM_LBUTTONDOWN:
            {
                mouseInput->LeftClick.Down = true;
            } break;
            case WM_LBUTTONUP:
            {
                mouseInput->LeftClick.Up = true;
            } break;
            case WM_RBUTTONDOWN:
            {
                mouseInput->RightClick.Down = true;
            } break;
            case WM_RBUTTONUP:
            {
                mouseInput->RightClick.Up = true;
            } break;
            case WM_MBUTTONDOWN:
            {
                mouseInput->Middle.Down = true;
            } break;
            case WM_MBUTTONUP:
            {
                mouseInput->Middle.Up = true;
            } break;
            case WM_MOUSEWHEEL:
            {
                mouseInput->Wheel = GET_WHEEL_DELTA_WPARAM(message.wParam);
            } break;
            
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                uint32 keyCode = (uint32)message.wParam;
                bool32 wasDown = ((message.lParam & (1 << 30)) != 0);
                bool32 isDown = ((message.lParam & (1 << 31)) == 0);
                if(wasDown != isDown)
                {
                    if(keyCode == 'W')
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->Up, isDown);
                    }
                    else if(keyCode == 'A')
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->Left, isDown);
                    }
                    else if(keyCode == 'S')
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->Down, isDown);
                    }
                    else if(keyCode == 'D')
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->Right, isDown);
                    }
                    else if(keyCode == 'Q')
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->Q, isDown);
                    }
                    else if(keyCode == 'E')
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->E, isDown);
                    }
                    else if(keyCode == 'F')
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->F, isDown);
                    }
                    else if(keyCode == 'G')
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->G, isDown);
                    }
                    else if(keyCode == 'C')
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->C, isDown);
                    }
                    else if(keyCode == 'P')
                    {
                        #if MP_INTERNAL
                        if(isDown)
                            GlobalPause = !GlobalPause;
                        #endif
                    }
                    else if(keyCode == 'L')
                    {
                        if(isDown)
                        {
                            if(state->InputPlaybackIndex == 0)
                            {
                                if(state->InputRecordingIndex == 0)
                                {
                                    Win32BeginInputRecording(state, 1);
                                }
                                else
                                {
                                    Win32EndInputRecording(state);
                                    Win32BeginInputPlayback(state, 1);
                                }
                            }
                            else
                            {
                                Win32EndInputPlayback(state);
                            }
                        }
                    }
                    else if(keyCode == 'K')
                    {
                        ShowCursor(mouseInput->ShowCursor);
                        mouseInput->ShowCursor = !mouseInput->ShowCursor;
                    }
                    else if(keyCode == VK_UP)
                    {
                        // do
                    }
                    else if(keyCode == VK_DOWN)
                    {
                        // do
                    }
                    else if(keyCode == VK_LEFT)
                    {
                        // do
                    }
                    else if(keyCode == VK_RIGHT)
                    {
                        // do
                    }
                    else if(keyCode == VK_SPACE)
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->Jump, isDown);
                    }
                    else if(keyCode == VK_ESCAPE)
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->Escape, isDown);
                    }
                    else if(keyCode == VK_CONTROL)
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->Crouch, isDown);
                    }
                    else if(keyCode == VK_SHIFT)
                    {
                        Win32ProcessKeyboardEvent(&keyboardController->Sprint, isDown);
                    }
                    else
                    {
                        // do
                    }
                }
                if(isDown)
                {
                    bool32 altKeyDown = (message.lParam & (1 << 29));
                    if(keyCode == VK_F4 && altKeyDown)
                    {
                        GlobalRunning = false;
                    }
                    if(keyCode == VK_RETURN && altKeyDown)
                    {
                        Win32ToggleFullScreen(message.hwnd);
                    }
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

internal void Win32DebugDrawVertical(win32OffscreenBuffer* backBuffer, int x, int top, int bottom, uint32 colour)
{
    if(top <= 0)
        top = 0;
       
    if(bottom > backBuffer->Height)
        bottom = backBuffer->Height;
    
    if(x >= 0 && x < backBuffer->Width)
    {
        uint8* pixel = (uint8*)backBuffer->Memory + x * backBuffer->bytesPerPixel + top * backBuffer->Pitch;
        for(int y = top; y < bottom; y++)
        {
            *(uint32*)pixel = colour;
            pixel += backBuffer->Pitch;
        }
    }
}

inline internal void Win32DrawSoundbufferMarker(win32OffscreenBuffer* backBuffer, Win32SoundOutput* soundBuffer,
                                                float C, int padX, int top, int bottom, DWORD value, uint32 colour)
{
    int x = padX + (int)(C * (float)value);
    Win32DebugDrawVertical(backBuffer, x, top, bottom, colour);
}

// TODO: Remove/replace this when we have a good ol renderer
#if 0
internal void Win32DebugSyncDisplay(win32OffscreenBuffer* backBuffer, int markerCount,
                        Win32DebugTimeMarker* markers, int currentMarkerIndex,  Win32SoundOutput* soundBuffer, float expectedDeltaTime)
{
    int padX = 16;
    int padY = 16;
    int lineHeight = 64;
    
    float C = (float)(backBuffer->Width - 2 * padX) / (float)soundBuffer->SecondaryBufferSize;
    for(int index = 0; index < markerCount; index++)
    {
        Win32DebugTimeMarker* thisMarker = &markers[index];
        MP_ASSERT(thisMarker->FlipPlayCursor < soundBuffer->SecondaryBufferSize);
        MP_ASSERT(thisMarker->FlipWriteCursor < soundBuffer->SecondaryBufferSize);
        MP_ASSERT(thisMarker->OutputPlayCursor < soundBuffer->SecondaryBufferSize);
        MP_ASSERT(thisMarker->OutputWriteCursor < soundBuffer->SecondaryBufferSize);
        MP_ASSERT(thisMarker->OutputLocation < soundBuffer->SecondaryBufferSize);
        MP_ASSERT(thisMarker->OutputByteCount < soundBuffer->SecondaryBufferSize);
        
        DWORD playColour = 0xFF00FFFF;
        DWORD writeColour = 0xFFFF0000;
        DWORD expectedFlipColour = 0xFFFFFF00;
        DWORD playWindowColour = 0xFFFF00FF;
        int top = padY;
        int bottom = padY + lineHeight;
        
        if(index == currentMarkerIndex)
        {
            int firstTop = top;
            top += lineHeight + padY;
            bottom += lineHeight + padY;
            Win32DrawSoundbufferMarker(backBuffer, soundBuffer, C, padX, top, bottom, thisMarker->OutputPlayCursor, playColour);
            Win32DrawSoundbufferMarker(backBuffer, soundBuffer, C, padX, top, bottom, thisMarker->OutputWriteCursor, writeColour);
            
            top += lineHeight + padY;
            bottom += lineHeight + padY;
            Win32DrawSoundbufferMarker(backBuffer, soundBuffer, C, padX, top, bottom, thisMarker->OutputLocation, playColour);
            Win32DrawSoundbufferMarker(backBuffer, soundBuffer, C, padX, top, bottom, (thisMarker->OutputLocation + thisMarker->OutputByteCount), writeColour);
            
            top += lineHeight + padY;
            bottom += lineHeight + padY;
            Win32DrawSoundbufferMarker(backBuffer, soundBuffer, C, padX, firstTop, bottom, thisMarker->ExpectedFlipPlayCursor, expectedFlipColour);
        }
        
        Win32DrawSoundbufferMarker(backBuffer, soundBuffer, C, padX, top, bottom, thisMarker->FlipPlayCursor, playColour);
        Win32DrawSoundbufferMarker(backBuffer, soundBuffer, C, padX, top, bottom, (thisMarker->FlipPlayCursor + 480 * soundBuffer->BytesPerSample), playWindowColour);
        Win32DrawSoundbufferMarker(backBuffer, soundBuffer, C, padX, top, bottom, thisMarker->FlipWriteCursor, writeColour);
    }
}
#endif

INT __stdcall WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR commandLine, INT showCode)
{
    Win32State win32State = {};
    
    Win32GetExeFilename(&win32State);
    
    char sourceDLLFullPath[MAX_PATH];
    Win32BuildEXEFilepathName(&win32State, "mariposa.dll", sourceDLLFullPath, sizeof(sourceDLLFullPath));
    
    char tempDLLFullPath[MAX_PATH];
    Win32BuildEXEFilepathName(&win32State, "mariposa_temp.dll", tempDLLFullPath, sizeof(tempDLLFullPath));
    
    char lockFullPath[MAX_PATH];
    Win32BuildEXEFilepathName(&win32State, "lock.tmp", lockFullPath, sizeof(lockFullPath));
    
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    
    uint32 desiredSchedulerMS = 1;
    bool32 sleepIsGranular = (timeBeginPeriod(desiredSchedulerMS) == TIMERR_NOERROR);
    
    Win32LoadXInput();
    
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
            Win32SoundOutput soundOutput = {};
            
            // TODO: refresh rate needs to be queried instead of set to a fixed value
            HDC RefreshDC = GetDC(window);
            int win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
            ReleaseDC(window, RefreshDC);
            int deviceRefreshRate = (win32RefreshRate > 1) ? win32RefreshRate : 60;
            float gameRefreshRate = (float)deviceRefreshRate / 2.0f;
            float expectedDeltaTime = 0.0f / gameRefreshRate;
            
            soundOutput.SamplesPerSecond = 48000;
            soundOutput.BytesPerSample = 2 * sizeof(int16);
            soundOutput.SecondaryBufferSize = soundOutput.SamplesPerSecond * soundOutput.BytesPerSample;
            // TODO: remove LatencySampleCount
            soundOutput.SafetyBytes = (DWORD)((float)soundOutput.SamplesPerSecond * (float)soundOutput.BytesPerSample / (3.0f * gameRefreshRate));
            
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
            gameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
            gameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
            gameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;
            
            win32State.TotalSize = gameMemory.PermanentStorageSize + gameMemory.TransientStorageSize;
            win32State.GameMemoryBlock = VirtualAlloc(baseAddress, win32State.TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            gameMemory.PermanentStorage = win32State.GameMemoryBlock;
            gameMemory.TransientStorage = ((uint8*)gameMemory.PermanentStorage + gameMemory.PermanentStorageSize);
            
            for(int replayIndex = 0; replayIndex < ArrayCount(win32State.ReplayBuffers); replayIndex++)
            {
                Win32ReplayBuffer* replayBuffer = &win32State.ReplayBuffers[replayIndex];
                
                Win32GetInputFileLocation(&win32State, false, replayIndex, replayBuffer->Filename, sizeof(replayBuffer->Filename));
                
                replayBuffer->FileHandle = CreateFileA(replayBuffer->Filename, GENERIC_WRITE | GENERIC_READ, 0, 0, CREATE_ALWAYS, 0, 0);
                DWORD maxSizeHigh = win32State.TotalSize >> 32;
                DWORD maxSizeLow = win32State.TotalSize & 0xFFFFFFFF;
                replayBuffer->MapView = CreateFileMappingA(replayBuffer->FileHandle, 0, PAGE_READWRITE,
                                                       maxSizeHigh, maxSizeLow, 0);
                replayBuffer->Data = MapViewOfFile(replayBuffer->MapView, FILE_MAP_ALL_ACCESS, 0, 0, win32State.TotalSize);
                
                if(replayBuffer->Data)
                {
                    
                }
                else
                {
                    // TODO: Log error
                }
            }
            
            if(gameMemory.PermanentStorage && gameMemory.TransientStorage && samples)
            {            
                MP_INPUT input[2] = {};
                MP_INPUT* newInput = &input[0];
                MP_INPUT* oldInput = &input[1];
                
                LARGE_INTEGER lastCounter = Win32GetClockValue();
                LARGE_INTEGER flipClock = Win32GetClockValue();
                float msDeltaTime = 0.0f;
                
                uint32 debugTimeMarkerIndex = 0;
                Win32DebugTimeMarker debugTimeMarkers[30] = {};
                
                bool32 soundIsValid = false;
                DWORD audioLatencyBytes = 0;
                float audioLatencySeconds = 0;
                
                Win32GameCode game = Win32LoadGameCode(sourceDLLFullPath, tempDLLFullPath, lockFullPath);
                
                VulkanData* vkData = VulkanInit(&gameMemory);
                
                uint64 lastCycleCount = __rdtsc();
                while(GlobalRunning)
                {
                    FILETIME newDLLWriteTime = Win32GetLastWriteTime(sourceDLLFullPath);
                    if(CompareFileTime(&newDLLWriteTime, &game.DLLLastWriteTime) != 0)
                    {
                        Win32UnloadGameCode(&game);
                        game = Win32LoadGameCode(sourceDLLFullPath, tempDLLFullPath, lockFullPath);
                    }
                    
                    MP_CONTROLLER_INPUT* oldKeyboardController = GetController(oldInput, 0);
                    MP_CONTROLLER_INPUT* newKeyboardController = GetController(newInput, 0);
                    MP_MOUSE_INPUT* mouseInput =  &newInput->Mouse;
                    *newKeyboardController = {};
                    newKeyboardController->IsConntected = true;
                    
                    for(int index = 0; index < ArrayCount(newKeyboardController->Buttons); index++)
                    {
                        newKeyboardController->Buttons[index].EndedDown = oldKeyboardController->Buttons[index].EndedDown;
                    }
                    
                    Win32ProcessPendingMessages(&win32State, newKeyboardController, mouseInput);
                    
                    if(GlobalPause)
                        continue;
                    
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
                            newController->IsAnalog = oldController->IsAnalog;
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
                    MP_THREAD_CONTEXT thread = {};
                    
                    MP_OFFSCREENBUFFER buffer = {};
                    buffer.Memory = GlobalBackbuffer.Memory;
                    buffer.Width = GlobalBackbuffer.Width;
                    buffer.Height = GlobalBackbuffer.Height;
                    buffer.Pitch = GlobalBackbuffer.Pitch;
                    
                    if(win32State.InputRecordingIndex)
                    {
                        Win32RecordInput(&win32State, newInput);
                    }
                    if(win32State.InputPlaybackIndex)
                    {
                        Win32PlaybackInput(&win32State, newInput);
                    }
                    
                    if(game.UpdateAndRender)
                    {
                        // Vulkan update call here
                        game.UpdateAndRender(&thread, &gameMemory, newInput, &buffer, msDeltaTime);
                    }
                    
                    LARGE_INTEGER audioClock = Win32GetClockValue();
                    float fromBeginToAudioSeconds = Win32GetSecondsElapsed(flipClock, audioClock);
                    
                    DWORD playCursor = 0;
                    DWORD writeCursor = 0;
                    if(GlobalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK)
                    {
                        /* Explanation of sound output computation:
                        * 
                        * We define a safety margin that is the number of samples we think our game
                        * update loop may vary by (e.g. up to 2ms).
                        * 
                        * When we wake up to write audio, we will look and see what the play cursor
                        * position is and we will forecast ahead where we think the play cursor will
                        * be on the next frame boundary.
                        * 
                        * We will then look to see if the write cursor is before that by at least the 
                        * safety margin. If it is, the target fill position is that frame boundary 
                        * plus one frame. This gives us perfect audio syn in the case of a card that 
                        * has low enough latency.
                        * 
                        * If the write cursor is after the safety margin, then we assume we can
                        * never sync the audio perfectly. So we will write one frames worth of audio
                        * plus the safety margin's worth of guard samples (1ms, or something
                        * determined to be safe, whatever we think the variability of our frame
                        * computation is).
                        */
                        if(!soundIsValid)
                        {
                            soundOutput.RunningSampleIndex = writeCursor / soundOutput.BytesPerSample;
                            soundIsValid = true;
                        }
                        
                        DWORD byteToLock = (soundOutput.RunningSampleIndex * soundOutput.BytesPerSample) % soundOutput.SecondaryBufferSize;
                        
                        DWORD expectedSoundBytesPerFrame = (DWORD)((float)soundOutput.SamplesPerSecond * (float)soundOutput.BytesPerSample / gameRefreshRate);
                        float secondsLeftUntilFlip = (expectedDeltaTime - fromBeginToAudioSeconds);
                        DWORD expectedBytesUntilFlip = (DWORD)((secondsLeftUntilFlip / expectedDeltaTime) * (float)expectedSoundBytesPerFrame);
                        DWORD expectedFrameBoundaryByte = playCursor + expectedBytesUntilFlip;
                        
                        DWORD safeWriteCursor = writeCursor;
                        if(safeWriteCursor < playCursor)
                            safeWriteCursor += soundOutput.SecondaryBufferSize;
                        MP_ASSERT(safeWriteCursor >= playCursor)
                        safeWriteCursor += soundOutput.SafetyBytes;
                        
                        bool32 audioCardIsLowLatency = (safeWriteCursor < expectedFrameBoundaryByte);
                        
                        DWORD targetCursor = 0;
                        if(audioCardIsLowLatency)
                        {
                            targetCursor = expectedFrameBoundaryByte + expectedSoundBytesPerFrame;
                        }
                        else
                        {
                            targetCursor = writeCursor + expectedSoundBytesPerFrame + soundOutput.SafetyBytes;
                        }
                        targetCursor %= soundOutput.SecondaryBufferSize;
                        
                        DWORD bytesToWrite = 0;
                        if(byteToLock > targetCursor)
                        {
                            bytesToWrite = soundOutput.SecondaryBufferSize - byteToLock;
                            bytesToWrite += targetCursor;
                        }
                        else
                        {
                            bytesToWrite = targetCursor - byteToLock;
                        }
                    
                        MP_SOUNDOUTPUTBUFFER soundBuffer = {};
                        soundBuffer.SamplesPerSecond = soundOutput.SamplesPerSecond;
                        soundBuffer.SampleCount = bytesToWrite / soundOutput.BytesPerSample;
                        soundBuffer.Samples = samples;
                        if(game.GetSoundSamples)
                            game.GetSoundSamples(&thread, &gameMemory, &soundBuffer);
                        
                        #if MP_INTERNAL
                        Win32DebugTimeMarker* marker = &debugTimeMarkers[debugTimeMarkerIndex];
                        marker->OutputPlayCursor = playCursor;
                        marker->OutputWriteCursor = writeCursor;
                        marker->OutputLocation = byteToLock;
                        marker->OutputByteCount = bytesToWrite;
                        marker->ExpectedFlipPlayCursor = expectedFrameBoundaryByte;
                        
                        DWORD unWrappedWriteCursor = writeCursor;
                        if(unWrappedWriteCursor < playCursor)
                            unWrappedWriteCursor += soundOutput.SecondaryBufferSize;
                            
                        audioLatencyBytes = unWrappedWriteCursor - playCursor;
                        audioLatencySeconds = (float)audioLatencyBytes / ((float)soundOutput.SamplesPerSecond * (float)soundOutput.BytesPerSample);
                        
                        /*char textBuffer[256];
                        _snprintf_s(textBuffer, sizeof(textBuffer), "BTL: %d, TC: %d, BTW: %d, PC: %d, WC: %d, DELTA: %d, AL: %.02f \n",
                                    byteToLock, targetCursor, bytesToWrite, playCursor, writeCursor, audioLatencyBytes, audioLatencySeconds);
                        OutputDebugStringA(textBuffer);*/
                        #endif
                        Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite, &soundBuffer);
                    }
                    else
                    {
                        soundIsValid = false;
                    }
                    // TODO: This becomes deprecated when we can enforce vSync with Vulkan
                    #if 0
                    LARGE_INTEGER workCounter = Win32GetClockValue();
                    float workSecondsElapsed = Win32GetSecondsElapsed(lastCounter, workCounter);
                    
                    float secondsElapsedForFrame = workSecondsElapsed;
                    if(secondsElapsedForFrame < expectedDeltaTime)
                    {
                        if(sleepIsGranular)
                        {
                            DWORD sleepMS = (DWORD)(1000.0f * (expectedDeltaTime - secondsElapsedForFrame));
                            if(sleepMS > 0)
                                Sleep(sleepMS);
                        }
                        
                        float testSecondsElapsedForFrame = Win32GetSecondsElapsed(lastCounter, Win32GetClockValue());
                        if(testSecondsElapsedForFrame < expectedDeltaTime)
                        {
                            // TODO: Log missed sleep
                        }
                        
                        while(secondsElapsedForFrame < expectedDeltaTime)
                        {
                            secondsElapsedForFrame = Win32GetSecondsElapsed(lastCounter, Win32GetClockValue());
                        }
                    }
                    else
                    {
                        // Log: Missed frame!
                    }
                    #endif
                    
                    LARGE_INTEGER endCounter = Win32GetClockValue();
                    msDeltaTime = 1000.0f * Win32GetSecondsElapsed(lastCounter, endCounter);
                    lastCounter = endCounter;
                    
                    Win32WindowDimensions dimensions = Win32GetWindowDimensions(window);
                    // TODO: This is replaced by better tools when we can render stuff with Vulkan
                    #if 0
                    // TODO: debugTimeMarkerIndex - 1 is wrong when debugTimeMarkerIndex = 0
                    Win32DebugSyncDisplay(&GlobalBackbuffer, ArrayCount(debugTimeMarkers), debugTimeMarkers,
                                            (debugTimeMarkerIndex - 1), &soundOutput, expectedDeltaTime);
                    #endif
                    
                    HDC deviceContext = GetDC(window);
                    Win32CopyBufferToWindow(deviceContext, &GlobalBackbuffer, dimensions.width, dimensions.height);
                    ReleaseDC(window, deviceContext);
                    
                    flipClock = Win32GetClockValue();
                    
                    // TODO: This is replaced by better tools when we can render stuff with Vulkan
                    #if 0
                    {
                        if(GlobalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK)
                        {
                            MP_ASSERT(debugTimeMarkerIndex < ArrayCount(debugTimeMarkers))
                            Win32DebugTimeMarker* marker = &debugTimeMarkers[debugTimeMarkerIndex];
                            marker->FlipPlayCursor = playCursor;
                            marker->FlipWriteCursor = writeCursor;
                        }
                    }
                    #endif
                    
                    // TODO: Swap macro
                    MP_INPUT* temp = newInput;
                    newInput = oldInput;
                    oldInput = temp;
                    
                    #if 0
                    uint64 endCycleCount = __rdtsc();
                    uint64 cyclesElapsed = endCycleCount - lastCycleCount;
                    lastCycleCount = endCycleCount;
                    
                    double fps = 1000.0f / msDeltaTime;
                    double MCPF = (double)cyclesElapsed / 1000000.0f;
                    
                    char fpsBuffer[256];
                    _snprintf_s(fpsBuffer, sizeof(fpsBuffer), "ms/frame: %.02f, FPS: %.02f, MCPF: %.02f\n", msDeltaTime, fps, MCPF);
                    OutputDebugStringA(fpsBuffer);
                    #endif
                    
                    // TODO: This is replaced by better tools when we can render stuff with Vulkan
                    #if 0
                    debugTimeMarkerIndex++;
                    if(debugTimeMarkerIndex == ArrayCount(debugTimeMarkers))
                    {
                        debugTimeMarkerIndex = 0;
                    }
                    #endif
                }
                
                VulkanCleanup(vkData);
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
