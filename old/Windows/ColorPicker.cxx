#include "ColorPicker.hxx"

#include "../Instance.hxx"
#include "../Predefined.hxx"


BOOL should_log_out_central_pixel_color = TRUE;
ScreenPixelData* recorded_screen_render_data_buffer = nullptr;

WindowIDList excluded_window_list;


void
GetCurrentCursorPosition
(
    int* const x, int* const y
)
{
    POINT mousePos;
    ::GetCursorPos(&mousePos);

    *x = mousePos.x;
    *y = mousePos.y;
}


ScreenLens::ScreenLens()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    module_handle_ = ::LoadLibrary(L"Magnification.dll");

    reinterpret_cast<BOOL(WINAPI*)(void)>( \
        ::GetProcAddress(module_handle_, "MagInitialize") )();

    auto runtime_host = Instance::RuntimeHost();
    auto instance_handle = (HINSTANCE)(((void**)runtime_host)[2]);

    window_class_name_ = (wchar_t*)L"MG_WINCLS#???????"; // ?? TODO:

    WNDCLASSEX wcex = {};

    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc    = ::DefWindowProc;
    wcex.hInstance      = instance_handle;
    wcex.lpszClassName  = window_class_name_;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(1 + COLOR_BTNFACE);

    if( 0 == ::RegisterClassEx(&wcex) ) {
        const auto error_code = ::GetLastError();
        fprintf(stderr, "ScreenLens Constructor Error 0 %ld\n", error_code);
        throw std::runtime_error("ScreenLens Constructor Error 0");
    }

    hwnd_magnifier_host_ = ::CreateWindow( \
                                        window_class_name_,
                                        L"MagnifierWindowHost",
                                        WS_POPUP,
                                        CW_USEDEFAULT, CW_USEDEFAULT,
                                        CAPTURE_WIDTH, CAPTURE_HEIGHT,
                                        nullptr,
                                        nullptr,
                                        instance_handle,
                                        nullptr);
    if( hwnd_magnifier_host_ == NULL )
    {
        const auto error_code = ::GetLastError();
        fprintf(stderr, "ScreenLens Constructor Error 1 %ld\n", error_code);
        throw std::runtime_error("ScreenLens Constructor Error 1");
    }

    hwnd_magnifier_ = ::CreateWindow( \
                                    L"Magnifier",
                                    L"MagnifierWindow",
                                    WS_CHILD | WS_VISIBLE,
                                    0, 0, CAPTURE_WIDTH, CAPTURE_HEIGHT,
                                    hwnd_magnifier_host_,
                                    nullptr,
                                    instance_handle,
                                    nullptr);
    if( hwnd_magnifier_ == NULL )
    {
        const auto error_code = ::GetLastError();
        fprintf(stderr, "ScreenLens Constructor Error 2 %ld\n", error_code);
        throw std::runtime_error("ScreenLens Constructor Error 2");
    }

    // set len magnification factor
    typedef struct
    {
        float v[3][3];
    } transform_matrix_t;

    transform_matrix_t matrix = {};
    matrix.v[0][0] = 1.00;
    matrix.v[1][1] = 1.00;
    matrix.v[2][2] = 1.00;

    BOOL(WINAPI *set_lens_transform_matrix)(HWND, transform_matrix_t*);

    set_lens_transform_matrix = \
            reinterpret_cast<decltype(set_lens_transform_matrix)>( \
                ::GetProcAddress(module_handle_, "MagSetWindowTransform") );

    if( FALSE == set_lens_transform_matrix(hwnd_magnifier_, &matrix) )
    {
        const auto error_code = ::GetLastError();
        fprintf(stderr, "ScreenLens Constructor Error 3 %ld\n", error_code);
        throw std::runtime_error("ScreenLens Constructor Error 3");
    }

    setSourceRect = \
            reinterpret_cast<decltype(setSourceRect)>( \
                ::GetProcAddress(module_handle_, "MagSetWindowSource") );

    setFilterList = \
            reinterpret_cast<decltype(setFilterList)>( \
                ::GetProcAddress(module_handle_, "MagSetWindowFilterList") );

    setZoomCallback = \
            reinterpret_cast<decltype(setZoomCallback)>( \
                ::GetProcAddress(module_handle_, "MagSetImageScalingCallback") );


    static auto who = this;

    static auto callback =
    []
    (
        HWND hwnd,
        void* src_data, DataInfo src_info,
        void* dest_data, DataInfo dest_info,
        RECT unclipped, RECT clipped, HRGN dirty
    )
    {
        return who->ZoomCallback(
            hwnd,
            src_data, src_info,
            dest_data, dest_info,
            unclipped, clipped, dirty
        );
        return TRUE;
    };

    if( TRUE != setZoomCallback(hwnd_magnifier_, callback) )
    {
        const auto error_code = ::GetLastError();
        fprintf(stderr, "ScreenLens Constructor Error 4 %ld\n", error_code);
        throw std::runtime_error("ScreenLens Constructor Error 4");
    }
}


ScreenLens::~ScreenLens()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    ::DestroyWindow(hwnd_magnifier_);
    ::DestroyWindow(hwnd_magnifier_host_);

    auto runtime_host = Instance::RuntimeHost();
    auto instance_handle = (HINSTANCE)(((void**)runtime_host)[2]);
    if( 0 == ::UnregisterClass(window_class_name_, instance_handle) )
    {
        fprintf(stderr, "RuntimeHostNative Deconstructor Error\n");
    }

    reinterpret_cast<BOOL(WINAPI*)(void)>( \
            ::GetProcAddress(module_handle_, "MagUninitialize") )();

}


DEFINE_GUID(GUID_WICPixelFormat32bppRGBA, \
    0xf5c7ad2d, 0x6a8d, 0x43dd, 0xa7, 0xa8, 0xa2, 0x99, 0x35, 0x26, 0x1a, 0xe9);


BOOL
CALLBACK
ScreenLens::ZoomCallback
(
    HWND hwnd,
    void* src_data, DataInfo src_info,
    void* dest_data, DataInfo dest_info,
    RECT unclipped, RECT clipped, HRGN dirty
)
{
    // fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    current_screen_data_ = src_data;
    current_screen_data_info_ = src_info;
    return TRUE;
}


bool
ScreenLens::RefreshScreenPixelDataWithinBound
(
    int central_x, int central_y,
    int bound_width, int bound_height,
    struct ScreenPixelData* const off_screen_render_data
)
{
    RECT bound_box;
    bound_box.left = central_x - bound_width/2;
    bound_box.right = bound_width - bound_box.left;
    bound_box.top = central_y - bound_height/2;
    bound_box.bottom = bound_height - bound_box.top;

    /*
     * call this will trigeer zoom callback, therefor the
     * current_screen_data_** will get updated
     */
    if( TRUE != setSourceRect(hwnd_magnifier_, bound_box) )
    {
        fprintf(stderr, "%s Error 1\n", __PRETTY_FUNCTION__);
        return false;
    }

    static auto rgba_data_to_raw_data = []
    (void* rgba_data, auto rgba_data_info, auto dst_data)
    {
        const auto cursor_base = (uint8_t*)rgba_data + rgba_data_info.offset;
        auto dst_data_cursor = (struct ScreenPixelData*)dst_data;

        for(UINT y = 0; y < rgba_data_info.height ; ++y)
        {
            for(UINT x = 0; x < rgba_data_info.width ; ++x)
            {
                auto cursor = cursor_base + y*rgba_data_info.stride + x*4;
                auto b = cursor[0];
                auto g = cursor[1];
                auto r = cursor[2];
                auto a = cursor[3];
                // fprintf(stderr, "0x%02X%02X%02X ", r, g, b);

                dst_data_cursor->b = cursor[0];
                dst_data_cursor->g = cursor[1];
                dst_data_cursor->r = cursor[2];

                dst_data_cursor++;
            }
            // fprintf(stderr, "\n");
        }
        // fprintf(stderr, "\n");
    };

    if( IsEqualGUID(current_screen_data_info_.format, \
                                GUID_WICPixelFormat32bppRGBA) )
    {
        rgba_data_to_raw_data(current_screen_data_, \
                                current_screen_data_info_, \
                                          off_screen_render_data);
        return true;
    }
    else
    {
        fprintf(stderr, "%s Error 2\n", __PRETTY_FUNCTION__);
        return false;
    }
}


LRESULT CALLBACK
WindowProcess
(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam
)
{
    class MainWindow* window = nullptr;

    if( uMsg == WM_NCCREATE )
    {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        if( cs->lpCreateParams != nullptr )
        {
            window = *(class MainWindow**)(cs->lpCreateParams);
            window->window_handle_ = hWnd;

            ::SetLastError(ERROR_SUCCESS);
            auto result = ::SetWindowLongPtr(hWnd, \
                    GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
            if( result == 0 && ::GetLastError() != ERROR_SUCCESS )
            {
                fprintf(stderr, "RuntimeHostNative::WndProc Error 1\n");
            }
        }
    }
    else
    {
        window = (class MainWindow*)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }

    if( window != nullptr )
    {
        return window->Callback(uMsg, wParam, lParam);
    }
    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}


MainWindow::MainWindow()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    auto runtime_host = Instance::RuntimeHost();
    auto instance_handle = (HINSTANCE)(((void**)runtime_host)[2]);

    window_class_name_ = (wchar_t*)L"WINCLS#???????"; // ?? TODO:

    WNDCLASSEX wcex = {};

    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc    = WindowProcess;
    wcex.hInstance      = instance_handle;
    wcex.lpszClassName  = window_class_name_;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(1 + COLOR_BTNFACE);
    wcex.style          = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;

    if( 0 == ::RegisterClassEx(&wcex) )
    {
        const auto error_code = ::GetLastError();
        fprintf(stderr, "MainWindow Constructor Error 0 %ld\n", error_code);
        throw std::runtime_error("MainWindow Constructor Error 0");
    }

    CREATESTRUCT create_struct = {};
    create_struct.lpCreateParams = this;

    window_handle_ = ::CreateWindowEx( \
                        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
                        window_class_name_,
                        L"ColorPicker",
                        WS_POPUP,
                        400, 400, UI_WINDOW_SIZE, UI_WINDOW_SIZE,
                        nullptr,
                        nullptr,
                        instance_handle,
                        &create_struct);

    if( window_handle_ == NULL)
    {
        const auto error_code = ::GetLastError();
        fprintf(stderr, "MainWindow Constructor Error 1 %ld\n", error_code);
        throw std::runtime_error("MainWindow Constructor Error 1");
    }

    window_dc_ = ::GetDC(window_handle_); // GetDC must pair with ReleaseDC

    const auto fresh_time_interval = 1.0f/CURSOR_REFRESH_FREQUENCY;
    refresh_timer_ = ::SetTimer(window_handle_, \
                        (UINT_PTR)this, fresh_time_interval*1000, nullptr);

    // cache excluded window list
    excluded_window_list.push_back(window_handle_);

    // alloc data buffer
    const auto data_size = CAPTURE_WIDTH*CAPTURE_HEIGHT;
    recorded_screen_render_data_buffer = new struct ScreenPixelData[data_size];

    //! load mask image with zoom factor 1
    //! therefor it may look odd when in hdpi screen
    mask_bitmap_ = []()
    {
        Gdiplus::Bitmap* bitmap = nullptr;

        auto hBuffer = ::GlobalAlloc(GMEM_MOVEABLE, RES_Circle_Mask_len);
        auto pBuffer = ::GlobalLock(hBuffer);

        IStream* pStream = NULL;
        ::CreateStreamOnHGlobal(hBuffer, FALSE, &pStream);

        ::CopyMemory(pBuffer, RES_Circle_Mask, RES_Circle_Mask_len);

        bitmap = Gdiplus::Bitmap::FromStream(pStream);

        pStream->Release();
        ::GlobalUnlock(hBuffer);
        ::GlobalFree(hBuffer);

        return bitmap;
    }();
}


MainWindow::~MainWindow()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    ::KillTimer(window_handle_, refresh_timer_);

    ::ReleaseDC(window_handle_, window_dc_);

    ::DestroyWindow(window_handle_);

    auto runtime_host = Instance::RuntimeHost();
    auto instance_handle = (HINSTANCE)(((void**)runtime_host)[2]);
    if( 0 == ::UnregisterClass(window_class_name_, instance_handle) )
    {
        fprintf(stderr, "RuntimeHostNative Deconstructor Error\n");
    }
}


LRESULT CALLBACK
MainWindow::Callback(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
    break;
    case WM_TIMER:
        onRefreshTimerTick();
    break;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    {
        fprintf(stderr, "Mouse Button Down\n");
    }
    break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    {
        fprintf(stderr, "Mouse Button Up\n");
        PostQuitMessage(0);
    }
    break;
    case WM_KEYUP:
    {
        fprintf(stderr, "Key Up %d\n", (int)wParam);
    }
    break;
    case WM_KEYDOWN:
    {
        fprintf(stderr, "Key Down %d\n", (int)wParam);

        if(wParam == VK_ESCAPE)
        {
            should_log_out_central_pixel_color = FALSE;
            PostQuitMessage(0);
        }
        if(wParam == VK_RETURN || wParam == VK_SPACE )
        {
            PostQuitMessage(0);
        }
    }
    break;
    }
    return ::DefWindowProc(window_handle_, uMsg, wParam, lParam);
}


void
MainWindow::onRefreshTimerTick()
{
    // fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    int cursor_x = 0, cursor_y = 0;
    GetCurrentCursorPosition(&cursor_x, &cursor_y);
    // fprintf(stderr, "Current Cursor : %4d %4d\n", cursor_x, cursor_y);

    int central_x = cursor_x;
    int central_y = cursor_y;

    static uint32_t record_screen_render_data_fresh_ratio_counter = 0;

    if( record_screen_render_data_fresh_ratio_counter == 0 )
    {
        RefreshScreenPixelDataWithinBound( \
            central_x, central_y, CAPTURE_WIDTH, CAPTURE_HEIGHT, \
                excluded_window_list, recorded_screen_render_data_buffer );

        float painting_time = 0;
        {
            class PerformanceCounter<float> counter(&painting_time);
            drawClientContent();
        }
        // fprintf(stderr, "Paint Time %f ms\n", painting_time/1000);
        PrintPixelColor();
    }

    record_screen_render_data_fresh_ratio_counter += 1;
    record_screen_render_data_fresh_ratio_counter %= \
        SCREEN_CAPTURE_FREQUENCY_TO_CURSOR_REFRESH_RATIO;

    // move window
    int new_window_position_x = cursor_x - UI_WINDOW_SIZE/2;
    int new_window_position_y = cursor_y - UI_WINDOW_SIZE/2;

    ::SetWindowPos(window_handle_, 0, \
            new_window_position_x, new_window_position_y, 0, 0, \
            SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER
        );
}

void
MainWindow::PrintPixelColor()
{
    int x = GRID_NUMUBER_L;
    int y = GRID_NUMUBER_L;
    auto pixel = recorded_screen_render_data_buffer[y*CAPTURE_WIDTH+x];

    int r = pixel.r;
    int g = pixel.g;
    int b = pixel.b;

    if( should_log_out_central_pixel_color == TRUE )
    {
        fprintf(stdout, "#%02X%02X%02X\n", r, g, b);
    }
    else
    {
        fprintf(stderr, "#%02X%02X%02X\n", r, g, b);
    }
}


void
MainWindow::drawClientContent()
{
    auto mem_dc = ::CreateCompatibleDC(window_dc_);

    BITMAPINFO bitmap_info = {};
    bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmap_info.bmiHeader.biWidth = UI_WINDOW_SIZE;
    bitmap_info.bmiHeader.biHeight = -UI_WINDOW_SIZE;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;

    auto mem_bitmap = ::CreateDIBSection( \
            mem_dc, &bitmap_info, DIB_RGB_COLORS, NULL, NULL, NULL);

    ::SelectObject(mem_dc, mem_bitmap);

    /*
    {
        Gdiplus::Graphics graph(mem_dc);

        auto color = Gdiplus::Color::MakeARGB(0xff, 0xff, 0x00, 0x00);
        graph.FillRectangle(&Gdiplus::SolidBrush(color), \
                            0, 0, UI_WINDOW_SIZE, UI_WINDOW_SIZE);
    }
    */

    {
        Gdiplus::Graphics graph(mem_dc);

        // auto color = Gdiplus::Color::MakeARGB(0xff, 0xff, 0x00, 0x00);
        // graph.FillRectangle(&Gdiplus::SolidBrush(color), \
        //                     0, 0, UI_WINDOW_SIZE, UI_WINDOW_SIZE);

        auto graphics_state = graph.Save();

        // clip the circle
        Gdiplus::GraphicsPath path(Gdiplus::FillMode::FillModeWinding);
        path.AddEllipse(4, 4, 163, 163);
        Gdiplus::Region region(&path);
        graph.SetClip(&region, Gdiplus::CombineMode::CombineModeReplace);

        Gdiplus::RectF window_rect(0, 0, UI_WINDOW_SIZE, UI_WINDOW_SIZE);

        // background as grid
        auto background_color = Gdiplus::Color::MakeARGB( \
                    0.98f *0xFF, 0.72f *0xFF, 0.72f *0xFF, 0.72f *0xFF );
        Gdiplus::SolidBrush background_brush(background_color);

        graph.FillRectangle(&background_brush, window_rect);

        // draw every pixels
        for(int idx_y = 0; idx_y < CAPTURE_HEIGHT; ++idx_y)
        {
            for(int idx_x = 0; idx_x < CAPTURE_WIDTH; ++idx_x)
            {
                auto pixel = recorded_screen_render_data_buffer[ \
                                            idx_y*CAPTURE_WIDTH+idx_x];

                int r = pixel.r, g = pixel.g, b = pixel.b;

                auto pixel_color = Gdiplus::Color::MakeARGB(0xff, r, g, b);
                Gdiplus::SolidBrush brush(pixel_color);

                int x = 1 + (GRID_PIXEL + 1)*idx_x;
                int y = 1 + (GRID_PIXEL + 1)*idx_y;
                graph.FillRectangle(&brush, x, y, GRID_PIXEL, GRID_PIXEL);
            }
        }

        // draw center grid
        int x = 1 + (1+GRID_PIXEL)*GRID_NUMUBER_L - 1;
        int y = 1 + (1+GRID_PIXEL)*GRID_NUMUBER_L - 1;

        Gdiplus::Pen black_pen(Gdiplus::Color(0xFF, 0x00, 0x00, 0x00), 1);
        Gdiplus::Pen white_pen(Gdiplus::Color(0xFF, 0xFF, 0xFF, 0xFF), 1);

        graph.DrawRectangle(&black_pen, x, y, GRID_PIXEL+1, GRID_PIXEL+1);
        graph.DrawRectangle(&white_pen, x+1, y+1, GRID_PIXEL-1, GRID_PIXEL-1);

        graph.Restore(graphics_state);

        // draw circle mask
        graph.DrawImage(mask_bitmap_, window_rect);
    }

    POINT src_point = {0, 0};
    SIZE wnd_size = {UI_WINDOW_SIZE, UI_WINDOW_SIZE};
    BLENDFUNCTION blend_fun = {AC_SRC_OVER, 0, 0xFF, AC_SRC_ALPHA};

    ::UpdateLayeredWindow(
            window_handle_,
            window_dc_,
            nullptr,
            &wnd_size,
            mem_dc,
            &src_point,
            0,
            &blend_fun,
            ULW_ALPHA
        );

    ::DeleteObject(mem_bitmap);
    ::DeleteDC(mem_dc);
}


bool
RefreshScreenPixelDataWithinBound
(
    int central_x, int central_y,
    int bound_width, int bound_height,
    const WindowIDList& excluded_window_list,
    struct ScreenPixelData* const off_screen_render_data
)
{
    static auto screen_lens = new class ScreenLens;

    screen_lens->SetExcludedWindowList(excluded_window_list);
    screen_lens->RefreshScreenPixelDataWithinBound( \
                                     central_x, central_y, \
                                         bound_width, bound_height, \
                                                off_screen_render_data );

    return true;
}

class MainWindow* main_window;

void
PreRun_Mode_Normal()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    fprintf(stderr, "screen record size: %4u %4u\n", \
                                CAPTURE_WIDTH, CAPTURE_HEIGHT);

    main_window = new MainWindow();
    main_window->Show();
    ::ShowCursor(FALSE); // hide cursor
}

void
PostRun_Mode_Normal()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    ::ShowCursor(TRUE); // show cursor
    main_window->PrintPixelColor();

    delete main_window;
}

void
PreRun(class Instance* instance)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    auto inst_info = instance->InstanceInfo();
    auto exec_mode = inst_info->CommandLineParameter<int>(L"--mode=");

    switch(exec_mode)
    {
        case 0:
            PreRun_Mode_Normal();
        break;
        default:
            // pass
        break;
    }
}


void
PostRun(class Instance* instance)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    auto inst_info = instance->InstanceInfo();
    auto exec_mode = inst_info->CommandLineParameter<int>(L"--mode=");

    switch(exec_mode)
    {
        case 0:
            PostRun_Mode_Normal();
        break;
        default:
            // pass
        break;
    }
}

