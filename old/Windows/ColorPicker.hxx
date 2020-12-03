#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <objidl.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

class GDI_PLUS_INITIALIZER
{
private:
    ULONG_PTR gdiplusToken;
    ::Gdiplus::GdiplusStartupInput gdiplusStartupInput;
public:
    GDI_PLUS_INITIALIZER()
    {
        ::Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    }
    ~GDI_PLUS_INITIALIZER()
    {
        ::Gdiplus::GdiplusShutdown(gdiplusToken);
    }
}
gdi_plus_initializer;


#include <vector>

typedef HWND WindowID ;
typedef std::vector<WindowID> WindowIDList;

//! always using this format: (r, g, b, x) = (float, float, float, float)
struct ScreenPixelData
{
    float r = 0, g = 0, b = 0, a = 0;

    static constexpr auto BitsPerChannel()
    {
        return sizeof(struct ScreenPixelData)*8/4;
    }

    static constexpr auto BitsAllChannel()
    {
        return sizeof(struct ScreenPixelData)*8;
    }
};

static_assert( ScreenPixelData::BitsPerChannel() == sizeof(float)*8 );
static_assert( ScreenPixelData::BitsAllChannel() == sizeof(float)*8*4 );


class ScreenLens
{
public:
    ScreenLens();
    ~ScreenLens();
private:
    HMODULE module_handle_ = nullptr;
private:
    wchar_t* window_class_name_ = nullptr;
    HWND hwnd_magnifier_host_ = nullptr;
    HWND hwnd_magnifier_ = nullptr;
private:
    typedef struct
    {
        UINT width;
        UINT height;
        GUID format;
        UINT stride;
        UINT offset;
        SIZE_T cbSize;
    } DataInfo;
private:
    typedef
    BOOL
    (CALLBACK* ZoomCallbackT)
    (
        HWND hwnd,
        void* src_data, DataInfo src_info,
        void* dest_data, DataInfo dest_info,
        RECT unclipped, RECT clipped, HRGN dirty
    );
private:
    BOOL
    (WINAPI* setSourceRect)
    (
        HWND,
        RECT rect
    );
private:
    BOOL
    (WINAPI* setFilterList)
    (
        HWND,
        DWORD filter_mode, int hwnd_list_count, const HWND* hwnd_list
    );
private:
    BOOL (WINAPI* setZoomCallback)
    (
        HWND hwnd,
        ZoomCallbackT callback
    );
private:
    BOOL CALLBACK ZoomCallback
    (
        HWND hwnd,
        void* src_data, DataInfo src_info,
        void* dest_data, DataInfo dest_info,
        RECT unclipped, RECT clipped, HRGN dirty
    );
public:
    bool
    SetExcludedWindowList(const WindowIDList& list)
    {
        return setFilterList(hwnd_magnifier_, 0, \
                    (int)list.size(), list.data() ) == TRUE ? true : false;
    }
private:
    void* current_screen_data_ = nullptr;
    DataInfo current_screen_data_info_ = {};
public:
    bool RefreshScreenPixelDataWithinBound(
        int central_x, int central_y,
        int bound_width, int bound_height,
        struct ScreenPixelData* const off_screen_render_data
    );
};


bool
RefreshScreenPixelDataWithinBound
(
    int central_x, int central_y,
    int bound_width, int bound_height,
    const WindowIDList& excluded_window_list,
    struct ScreenPixelData* const off_screen_render_data
);


class MainWindow
{
public:
    MainWindow();
    ~MainWindow();
    void PrintPixelColor();
private:
    friend LRESULT CALLBACK WindowProcess(HWND, UINT, WPARAM, LPARAM);
private:
    LRESULT CALLBACK Callback(UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
    wchar_t* window_class_name_ = nullptr;
    HWND window_handle_ = nullptr;
private:
    HDC window_dc_ = nullptr;
public:
    void Show() { ::ShowWindow(window_handle_, SW_SHOW); }
    void Hide() { ::ShowWindow(window_handle_, SW_HIDE); };
private:
    void onRefreshTimerTick();
private:
    UINT_PTR refresh_timer_;
private:
    class Gdiplus::Bitmap* mask_bitmap_ = nullptr;
private:
    void drawClientContent();
};


template <typename T>
class PerformanceCounter
{
    LARGE_INTEGER start_tick, end_tick, frequency;
    LARGE_INTEGER elapsed_tick;
    T* const elapsed_microseconds_;
public:
    PerformanceCounter(T* elapsed_microseconds)
    :elapsed_microseconds_(elapsed_microseconds)
    {
        ::QueryPerformanceFrequency(&frequency);
        ::QueryPerformanceCounter(&start_tick);
    }
public:
    ~PerformanceCounter()
    {
        ::QueryPerformanceCounter(&end_tick);
        elapsed_tick.QuadPart = end_tick.QuadPart - start_tick.QuadPart;
        elapsed_tick.QuadPart *= 1000000;
        *elapsed_microseconds_ = elapsed_tick.QuadPart / frequency.QuadPart;
    }
};


