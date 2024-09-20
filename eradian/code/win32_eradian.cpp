#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

#define internal static
#define local_persist static
#define global_variable static

// GlobalRunning controls the quit state of the application
global_variable bool GlobalRunning;

global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

// define the offscreen buffer struct
struct win32_offscreen_buffer {
    // Pixels are always 32-bits wide, Memory Order BB GG RR XX
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

global_variable win32_offscreen_buffer GlobalBackBuffer;

// define the xinput get state functions
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pStates)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) {
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// define the xinput set state function
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) {
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

// define the direct sound create function
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void Win32LoadXInput() {
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!XInputLibrary) {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    if (!XInputLibrary) {
        XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }
    if (XInputLibrary) {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
    else {
        // logging
    }
}

internal void Win32InitDSound(HWND Window, INT32 SamplesPerSecond, INT32 BufferSize) {
    // Load the library
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    if (DSoundLibrary) {
        // Get a DirectSound object
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND DirectSound;
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {

            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) {

                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0))) {
                    if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat))) {
                        OutputDebugStringA("Primary buffer format was set.\n");
                    }
                    else {
                        // logging
                    }
                }
                else {
                    // logging
                }
                
            }
            else {
                // logging
            }
            // Create a secondary buffer
            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
            if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0))) {
                OutputDebugStringA("Secondary buffer created.\n");
            }
            else {
                // logging
            }

            // Maybe hook into the buffer, write to it
            OutputDebugStringA("DirectSound created\n");
        }
        else {
            // logging
        }
        // DirectSoundCreate
    }
};


struct win32_window_dimension {
	int Width;
	int Height;
};

// function for geting the window dimension
internal win32_window_dimension Win32GetWindowDimension(HWND Window) {
	win32_window_dimension Result;
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	return(Result);
}

// function for rendering stuff to the offscreen buffer
// walkes each pixel size on memory and sets the color
internal void RenderStuff(win32_offscreen_buffer *Buffer, int XOffset, int YOffset) {      
    UINT8 *Row = (UINT8 *)Buffer->Memory;
    for(int Y = 0; Y < Buffer->Height; Y++) {
        UINT32 *Pixel = (UINT32 *)Row;
        for(int X = 0; X < Buffer->Width; X++) {
            UINT8 Blue = (X + XOffset);
            UINT8 Green = (Y + YOffset);
            UINT8 Red = 0;
            UINT8 Alpha = 0;
            *Pixel++ = ((Red << 16) | (Green << 8) | Blue | (Alpha << 24));
        }
        Row += Buffer->Pitch;
    }
}
// the function that runs every time the window is resized
internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height) {

    Buffer->Width = Width;
    Buffer->Height = Height;
    // free the memory if it is already allocated
    if(Buffer->Memory) {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    // create a new bitmap info based on the width and height, later it will be used to copy our content into the window
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    // allocate memory for to the buffer
    int BitmapMemorySize = Buffer->Width * Buffer->Height * 4;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    Buffer->Pitch = Buffer->Width * 4;

    // Should clear the screen to black maybe?
}

// function for copying the buffer to the window
internal void Win32CopyBufferToWindow(  HDC DeviceContext, int WindowWidth, int WindowHeight,
                                        win32_offscreen_buffer *Buffer) 
{
    StretchDIBits(
        DeviceContext,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer->Width, Buffer->Height,
        Buffer->Memory,
        &Buffer->Info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

// the main window callback function
LRESULT CALLBACK Win32MainWindowCallback(
    HWND Window,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
) {
    LRESULT Result = 0;

    switch(Message) {
        case WM_SIZE: {

        } break;
        case WM_DESTROY: {
            GlobalRunning = false;
        } break;
        case WM_CLOSE: {
            GlobalRunning = false;
        } break;
        case WM_ACTIVATEAPP: {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            UINT32 VKCode = WParam;
            bool WasDown = ((LParam & (1 << 30)) != 0);
            bool IsDown = ((LParam & (1 << 31)) == 0);

            if (WasDown != IsDown) {
                if (VKCode == 'W') {
                }
                else if (VKCode == 'A') {
                }
                else if (VKCode == 'S') {
                }
                else if (VKCode == 'D') {
                }
                else if (VKCode == 'Q') {
                }
                else if (VKCode == 'E') {
                }
                else if (VKCode == VK_UP) {
                }
                else if (VKCode == VK_DOWN) {
                }
                else if (VKCode == VK_LEFT) {
                }
                else if (VKCode == VK_RIGHT) {
                }
                else if (VKCode == VK_ESCAPE) {
                    OutputDebugStringA("ESCAPE: ");
                    if (IsDown) {
                        OutputDebugStringA("IsDown");
                    }
                    if (WasDown) {
                        OutputDebugStringA("WasDown");
                    }
                    OutputDebugStringA("\n");
                }
                else if (VKCode == VK_SPACE) {
                }
            }

            bool AltKeyWasDown = (LParam & (1 << 29));
            if ((VKCode == VK_F4) && AltKeyWasDown) {
                GlobalRunning = false;
            }
        } break;


        case WM_PAINT: {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32CopyBufferToWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackBuffer);
            EndPaint(Window, &Paint);
        } break;

        default: {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    
    return(Result);
}

int CALLBACK WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CommandLine,
    int ShowCode) 
{
    Win32LoadXInput();
    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    // WindowClass.hIcon = ; // for defining an icon
    WindowClass.lpszClassName = "EradianWindowClass";
    
    // register the window class
    if(RegisterClassA(&WindowClass)) {
        HWND Window = CreateWindowExA(
            0,
            WindowClass.lpszClassName,
            "Eradian",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            Instance,
            0
        );
        if(Window) {
            GlobalRunning = true;
            int XOffset = 0;
            int YOffset = 0;

            Win32InitDSound(Window, 48000, 48000 * sizeof(INT16) * 2);

            // the main loop for the application
            while(GlobalRunning) {

                MSG Message;
                while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
                    if(Message.message == WM_QUIT) {
                        GlobalRunning = false;
                    }
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }
                // this is where the fun resides right now
                
                for (DWORD ControllerIndex=0; ControllerIndex<XUSER_MAX_COUNT; ++ControllerIndex) {
                    XINPUT_STATE ControllerState;
                    if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {
                        // This controller is plugged in
                        XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                        bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                        INT16 StickX = Pad->sThumbLX;
                        INT16 StickY = Pad->sThumbLY;

                        XOffset += StickX >> 14;
                        YOffset += StickY >> 14;

                    }
                    else {
                        // This controller is not available
                    }
                }

                RenderStuff(&GlobalBackBuffer, XOffset, YOffset);

                // copy the buffer to the window
                HDC DeviceContext = GetDC(Window); 
                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                Win32CopyBufferToWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackBuffer);
                ReleaseDC(Window, DeviceContext); 
            }
        }
        else {
            // logging
        }
    }
    else {
    // logging
    }

    return(0);
}