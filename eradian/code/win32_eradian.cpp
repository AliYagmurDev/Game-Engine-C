#include <windows.h>

// define the static keyword with different names based on their usecase
#define internal static
#define local_persist static
#define global_variable static

// running controls the quit state of the application
global_variable bool Running;

// global variables for the bitmap to work
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;

// the function that runs every time the window is resized
internal void Win32ResizeDIBSection(int Width, int Height) {

    // free the bitmap memory if it exists
    if(BitmapHandle) {
        DeleteObject(BitmapHandle);
    }
    // create a new device context if it doesn't exist
    if(!BitmapDeviceContext) {
        BitmapDeviceContext = CreateCompatibleDC(0);
    }

    // create a new bitmap info based on the width and height, later it will be used to copy our content into the window
    BITMAPINFO BitmapInfo;
    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = Width;
    BitmapInfo.bmiHeader.biHeight = Height;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    // will work further on this, free first or free after

    BitmapHandle = CreateDIBSection(
        BitmapDeviceContext,
        &BitmapInfo,
        DIB_RGB_COLORS,
        &BitmapMemory,
        0,
        0
    );
}

internal void Win32UpdateWindow(HDC DeviceContext, int X, int Y, int Width, int Height) {
    StretchDIBits(
        DeviceContext,
        X, Y, Width, Height,
        X, Y, Width, Height,
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
            Win32UpdateWindow(DeviceContext, X, Y, Width, Height);
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
        HWND WindowHandle = CreateWindowExA(
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
        if(WindowHandle) {
            Running = true;
            while(Running) {
                MSG Message;
                BOOL MessageResult = GetMessageA(&Message, 0, 0, 0);
                if(MessageResult > 0) {
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }
                else {
                    break;
                }
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