#include <windows.h>
#include <stdint.h>

// define the static keyword with different names based on their usecase
#define internal static
#define local_persist static
#define global_variable static

// GlobalRunning controls the quit state of the application
global_variable bool GlobalRunning;

// define the offscreen buffer struct
struct win32_offscreen_buffer {
    // Pixels are always 32-bits wide, Memory Order BB GG RR XX
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

// define the global back buffer (It might not be here later)
global_variable win32_offscreen_buffer GlobalBackBuffer;

// define the window dimension struct for easy access
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
internal void RenderStuff(win32_offscreen_buffer Buffer, int XOffset, int YOffset) {      
    UINT8 *Row = (UINT8 *)Buffer.Memory;
    for(int Y = 0; Y < Buffer.Height; Y++) {
        UINT32 *Pixel = (UINT32 *)Row;
        for(int X = 0; X < Buffer.Width; X++) {
            UINT8 Blue = (X + XOffset);
            UINT8 Green = (Y + YOffset);
            UINT8 Red = 0;
            UINT8 Alpha = 0;
            *Pixel++ = ((Red << 16) | (Green << 8) | Blue | (Alpha << 24));
        }
        Row += Buffer.Pitch;
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
    // -Height is for the top-down bitmap
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    // allocate memory for to the buffer
    int BitmapMemorySize = Buffer->Width * Buffer->Height * 4;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    Buffer->Pitch = Buffer->Width * 4;

    // Should clear the screen to black maybe?
}

// function for copying the buffer to the window
internal void Win32CopyBufferToWindow(  HDC DeviceContext, int WindowWidth, int WindowHeight,
                                        win32_offscreen_buffer Buffer) 
{
    StretchDIBits(
        DeviceContext,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer.Width, Buffer.Height,
        Buffer.Memory,
        &Buffer.Info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

// the main window callback function
// runs for every message that the window receives
// each case interrupts the message and initate our logic
// all other cases handled by the default windows procedure
LRESULT CALLBACK Win32MainWindowCallback(
    HWND Window,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
) {
    LRESULT Result = 0;

    switch(Message) {
        case WM_SIZE: {
            // when the window is resized

        } break;
        case WM_DESTROY: {
            // when the window is destroyed
            GlobalRunning = false;
        } break;
        case WM_CLOSE: {
            // when user tries to quit by clicking the X button
            GlobalRunning = false;
        } break;
        case WM_ACTIVATEAPP: {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        case WM_PAINT: {
            // when the window needs to be repainted
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32CopyBufferToWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackBuffer);
            EndPaint(Window, &Paint);
        } break;

        default: {
            // default windows procedure for everything else
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    
    return(Result);
}

// the main function where windows starts executing the code
int CALLBACK WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CommandLine,
    int ShowCode) 
{
    // create a window class for communication with the window
    WNDCLASSA WindowClass = {};

    //win32_window_dimension Dimension = Win32GetWindowDimension(Window);
    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    // fill the window class with the necessary information
    // current style is for redrawing the whole window when it is resized
    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    // the callback function for the window to manage the messages
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    // the instance of the application
    WindowClass.hInstance = Instance;
    // WindowClass.hIcon = ; // for defining an icon
    // Below is the name for the window class
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
        // if the window is created successfully
        if(Window) {
            GlobalRunning = true;
            int XOffset = 0;
            int YOffset = 0;
            // the main loop for the application
            while(GlobalRunning) {

                MSG Message;
                // run for all the messages in the message queue and dispatch them
                // exit when no message is left in the queue
                while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
                    if(Message.message == WM_QUIT) {
                        GlobalRunning = false;
                    }
                    //Turns messages into something useful
                    TranslateMessage(&Message);
                    //Sends the message to the callback function
                    DispatchMessageA(&Message);
                }
                // render some stuff to the offscreen buffer
                // this is where the fun resides right now
                RenderStuff(GlobalBackBuffer, XOffset, YOffset);

                // copy the buffer to the window
                HDC DeviceContext = GetDC(Window); // get the device context for the window
                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                Win32CopyBufferToWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackBuffer);
                ReleaseDC(Window, DeviceContext); // release the device context (must happen each time you get it)

                // part of the current fun, wont be here later
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