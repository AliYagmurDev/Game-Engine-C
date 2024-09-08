#include <windows.h>
#include <stdint.h>

// define the static keyword with different names based on their usecase
#define internal static
#define local_persist static
#define global_variable static

typedef unsigned char uint8;

// running controls the quit state of the application
global_variable bool Running;

// global variables for the bitmap to work
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;

internal void RenderStuff(int XOffset, int YOffset) {  
    int Pitch = BitmapWidth * 4;
    UINT8 *Row = (UINT8 *)BitmapMemory;
    for(int Y = 0; Y < BitmapHeight; Y++) {
        UINT32 *Pixel = (UINT32 *)Row;
        for(int X = 0; X < BitmapWidth; X++) {
            UINT8 Blue = (X + XOffset);
            UINT8 Green = (Y + YOffset);
            UINT8 Red = 0;
            UINT8 Alpha = 0;
            *Pixel++ = ((Red << 16) | (Green << 8) | Blue | (Alpha << 24));
        }
        Row += Pitch;
    }
}
// the function that runs every time the window is resized
internal void Win32ResizeDIBSection(int Width, int Height) {

    if(BitmapMemory) {
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    BitmapWidth = Width;
    BitmapHeight = Height;

    // create a new bitmap info based on the width and height, later it will be used to copy our content into the window
    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    // allocate memory for the bitmap
    int BitmapMemorySize = BitmapWidth * BitmapHeight * 4;
    BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    // Should clear the screen to black maybe?
}

internal void Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect, int X, int Y, int Width, int Height) {
    int WindowWidth = ClientRect->right - ClientRect->left;
    int WindowHeight = ClientRect->bottom - ClientRect->top;
    StretchDIBits(
        DeviceContext,
        0, 0, BitmapWidth, BitmapHeight,
        0, 0, WindowWidth, WindowHeight,
        BitmapMemory,
        &BitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

LRESULT CALLBACK Win32MainWindowCallback(
    HWND Window,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
) {
    LRESULT Result = 0;

    switch(Message) {
        case WM_SIZE: {
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            int Width = ClientRect.right - ClientRect.left;
            int Height = ClientRect.bottom - ClientRect.top;
            Win32ResizeDIBSection(Width, Height);
        } break;
        case WM_DESTROY: {
            // when the window is destroyed
            Running = false;
        } break;
        case WM_CLOSE: {
            // when user tries to quit by clicking the X button
            Running = false;
        } break;
        case WM_ACTIVATEAPP: {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        case WM_PAINT: {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

            RECT ClientRect;
            GetClientRect(Window, &ClientRect);

            Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
            EndPaint(Window, &Paint);
        } break;

        default: {
            //OutputDebugStringA("default\n");
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
    WNDCLASSA WindowClass = {};
    //WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    // WindowClass.hIcon = ;
    WindowClass.lpszClassName = "EradianWindowClass";
    
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
            Running = true;
            int XOffset = 0;
            int YOffset = 0;
            while(Running) {

                MSG Message;
                while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
                    if(Message.message == WM_QUIT) {
                        Running = false;
                    }
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }
                RenderStuff(XOffset, YOffset);
                HDC DeviceContext = GetDC(Window);
                RECT ClientRect;
                GetClientRect(Window, &ClientRect);
                int WindowWidth = ClientRect.right - ClientRect.left;
                int WindowHeight = ClientRect.bottom - ClientRect.top;

                Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);
                ReleaseDC(Window, DeviceContext);
                ++XOffset;
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