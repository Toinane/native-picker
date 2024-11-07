#pragma once

#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include "WinPicker.h"

const int GRID_SIZE = 10;
const int RADIUS = 100;

std::string WinPicker::GetColor(int relativeX, int relativeY) {
    HDC dc = GetDC(NULL);
    
    POINT _cursor;

    GetCursorPos(&_cursor);
    COLORREF color = GetPixel(dc, _cursor.x + relativeX, _cursor.y + relativeY);
    ReleaseDC(NULL, dc);

    DWORD dwR = GetRValue(color);
    DWORD dwG = GetGValue(color);
    DWORD dwB = GetBValue(color);

    std::string str = std::to_string(dwR) + "," + std::to_string(dwG) + "," + std::to_string(dwB);

    return str;
}

void WinPicker::SendColor(Napi::Env env) {
    

    for(int x = 0; x <= sizeGrid; x++) {
        for(int y = 0; y <= sizeGrid; y++) {
            std::string color = GetColor(x, y);
            emit.Call({
                Napi::String::New(env, "update"),
                Napi::String::New(env, color)
            });
        }
    }

    emit.Call({
        Napi::String::New(env, "update"),
        Napi::String::New(env, "#############################")
    });
}

void WinPicker::Test() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return;
    }

    // Create a window
    GLFWwindow* window = glfwCreateWindow(RADIUS * 2, RADIUS * 2, "Pixel Grid", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return;
    }

    // Set the window position to the center of the screen
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwSetWindowPos(window, (mode->width - RADIUS * 2) / 2, (mode->height - RADIUS * 2) / 2);

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window)) {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Get the pixels from the screen
        int x, y;
        glfwGetWindowPos(window, &x, &y);
        std::vector<unsigned char> pixels(RADIUS * RADIUS * 3); // 3 channels for RGB
        glReadPixels(x, y, RADIUS, RADIUS, GL_RGB, GL_UNSIGNED_BYTE, &pixels[0]);

        // Draw the pixels in a circle
        glBegin(GL_TRIANGLE_FAN);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex2f(RADIUS, RADIUS); // center of the circle
        for (int i = 0; i <= 360; i += 360 / GRID_SIZE) {
            int pixelIndex = ((i / (360 / GRID_SIZE)) * RADIUS * 3) + ((GRID_SIZE / 2) * 3);
            float r = pixels[pixelIndex] / 255.0f;
            float g = pixels[pixelIndex + 1] / 255.0f;
            float b = pixels[pixelIndex + 2] / 255.0f;
            glColor3f(r, g, b);
            float angle = i * 3.14159265358979323846f / 180.0f;
            glVertex2f(RADIUS + RADIUS * std::cos(angle), RADIUS + RADIUS * std::sin(angle));
        }
        glEnd();

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Clean up
    glfwTerminate();
    return;
}

void WinPicker::Init(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    emit = info[0].As<Napi::Function>();
    Napi::Object params = info[1].As<Napi::Object>();

    WinPicker::Test();

    // previousColor = (std::string) params.Get("previousColor").ToString();
    // sizeGrid = (int) params.Get("sizeGrid").ToNumber();

    // emit.Call({
    //     Napi::String::New(env, "start")
    // });

    // PICKER_ACTIVATED = true;

    // while(PICKER_ACTIVATED) {

    //     if((GetKeyState(VK_LBUTTON) & 0x8000) != 0) {
    //         emit.Call({
    //                 Napi::String::New(env, "update"),
    //                 Napi::String::New(env, "left click")
    //             });
    //     }



    // }


    emit.Call({
        Napi::String::New(env, "end")
    });
}

void WinPicker::KeyEventProc(KEY_EVENT_RECORD ker, Napi::Env env) {
    if (ker.bKeyDown)
        emit.Call({
            Napi::String::New(env, "update"),
            Napi::String::New(env, "key pressed")
        });
    else
        emit.Call({
            Napi::String::New(env, "update"),
            Napi::String::New(env, "key released")
        });
}

void WinPicker::MouseEventProc(MOUSE_EVENT_RECORD mer, Napi::Env env) {
    switch (mer.dwEventFlags) {
        case 0:
            if (mer.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED) {
                emit.Call({
                    Napi::String::New(env, "update"),
                    Napi::String::New(env, "left click")
                });
                SendColor(env);
            }
            else if (mer.dwButtonState == RIGHTMOST_BUTTON_PRESSED) {
                emit.Call({
                    Napi::String::New(env, "update"),
                    Napi::String::New(env, "right click")
                });
                //PICKER_ACTIVATED = false;
            }
            break;
        case MOUSE_MOVED:
            emit.Call({
                    Napi::String::New(env, "update"),
                    Napi::String::New(env, "mouse move")
                });
            break;
    }
}