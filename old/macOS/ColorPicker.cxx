#include "ColorPicker.hxx"

#include "../Instance.hxx"
#include "../Predefined.hxx"


BOOL should_log_out_central_pixel_color = YES;
ScreenPixelData* recorded_screen_render_data_buffer = nullptr;


@implementation AppDelegate
{
    NSWindow* main_window;
    WindowIDList excluded_window_list;
    NSTimer* fresh_timer;
}

-(id) init
{
    self = [super init];

    // create the main window
    main_window = [MainWindow new];

    // cache excluded window list
    const auto main_window_id = (WindowID)(uintptr_t)[main_window windowNumber];
    excluded_window_list.push_back(main_window_id);
    fprintf(stderr, "Main Window ID 0x%08X\n", main_window_id);

    // alloc data buffer
    const auto data_size = CAPTURE_WIDTH*CAPTURE_HEIGHT;
    recorded_screen_render_data_buffer = new struct ScreenPixelData[data_size];

    // set fresh timer
    const auto fresh_time_interval = 1.0f/CURSOR_REFRESH_FREQUENCY;
    fresh_timer = [NSTimer scheduledTimerWithTimeInterval: fresh_time_interval
                          target: self
                          selector: @selector(onRefreshTimerTick:)
                          userInfo: nil
                          repeats: YES];

    [[NSRunLoop currentRunLoop] addTimer: fresh_timer
                                forMode: NSDefaultRunLoopMode];

    return self;
}

-(BOOL) applicationShouldTerminateAfterLastWindowClosed: (NSApplication*)sender
{
    return NO;
}

-(void) onRefreshTimerTick: (NSTimer*)timer;
{
    // fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    float x = 0.f, y = 0.f;
    GetCurrentCursorPosition(&x, &y);
    // fprintf(stderr, "Current Cursor Position: %9.4f %9.4f\n", x, y);

    static uint32_t record_screen_render_data_fresh_ratio_counter = 0;

    if( record_screen_render_data_fresh_ratio_counter == 0 )
    {
        // update recoded data
        RefreshScreenPixelDataWithinBound( \
                x, y, CAPTURE_WIDTH, CAPTURE_HEIGHT, \
                    excluded_window_list, recorded_screen_render_data_buffer );

        // force redraw
        [[main_window contentView] display];
    }

    record_screen_render_data_fresh_ratio_counter += 1;
    record_screen_render_data_fresh_ratio_counter %= \
        SCREEN_CAPTURE_FREQUENCY_TO_CURSOR_REFRESH_RATIO;

    CGPoint mouse_pos;
    mouse_pos.x = x - UI_WINDOW_SIZE/2.0f;
    mouse_pos.y = y - UI_WINDOW_SIZE/2.0f;
    [main_window setFrameOrigin: mouse_pos];
}

@end


@implementation MainWindow

-(BOOL) canBecomeMainWindow
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    return YES;
}

-(BOOL) canBecomeKeyWindow
{
    // make sure our window can recive keyboard event
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    return YES;
}

-(id) init
{
    float x = 0.f, y = 0.f;
    GetCurrentCursorPosition(&x, &y);

    CGRect wnd_rect;
    wnd_rect.origin.x    = x - UI_WINDOW_SIZE/2.0f;
    wnd_rect.origin.y    = y - UI_WINDOW_SIZE/2.0f;
    wnd_rect.size.width  = UI_WINDOW_SIZE;
    wnd_rect.size.height = UI_WINDOW_SIZE;

    self = [super initWithContentRect: wnd_rect
                  styleMask: NSBorderlessWindowMask
                  backing: NSBackingStoreBuffered
                  defer: NO];

    [self setBackgroundColor: [NSColor clearColor]];
    // [self setBackgroundColor: [NSColor redColor]];

    [self setLevel: NSStatusWindowLevel];
    [self makeKeyAndOrderFront: nil];
    [self setAlphaValue: 1.0];
    [self setOpaque: NO];
    [self setContentView: [MainView new]];

    return self;
}

-(void) keyDown: (NSEvent*)event
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    switch(event.keyCode)
    {
        case 0x35: // escape
        {
            // TODO:
            should_log_out_central_pixel_color = NO;
            [self close];
        }
        break;
        case 0x43: // enter
        case 0x24: // return
        {
            // TODO:
            [self close];
        }
        break;
        default:
        break;
    }
    [super keyDown:event];
}

-(void) close
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    [super close];
    [NSApp stop: nil];
}

@end


@implementation MainView
{
    CGImageRef image_cicle_mask;
}

-(id) init
{
    self = [super init];

    // load circle mask
    image_cicle_mask = [](auto data_buffer, auto data_buffer_size)
    {
        auto data_provide = ::CGDataProviderCreateWithData( \
                            NULL, data_buffer, data_buffer_size, NULL);

        return ::CGImageCreateWithPNGDataProvider(\
                        data_provide, NULL, NO, kCGRenderingIntentDefault);

    }((char*)RES_Circle_Mask, RES_Circle_Mask_len);

    return self;
}

-(void) drawRect: (NSRect)dirtRect
{
    // fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    CGRect wnd_rect;
    wnd_rect.origin.x = 0;
    wnd_rect.origin.y = 0;
    wnd_rect.size.width  = UI_WINDOW_SIZE;
    wnd_rect.size.height = UI_WINDOW_SIZE;

    CGRect mask_bound;
    mask_bound.origin.x = 8 + 2;
    mask_bound.origin.y = 8 + 2;
    mask_bound.size.width = UI_WINDOW_SIZE - (8+2)*2;
    mask_bound.size.height = UI_WINDOW_SIZE - (8+2)*2;

    auto ctx = [[NSGraphicsContext currentContext] CGContext];

    // clip circle
    CGContextSaveGState(ctx);

      auto clip_path = CGPathCreateWithEllipseInRect(mask_bound, nil);
      CGContextAddPath(ctx, clip_path);
      CGContextClip(ctx);
      CGContextSetLineWidth(ctx, 1.0f);
      CGContextSetShouldAntialias(ctx, NO);


      // draw background for grid
      auto grid_color = CGColorCreateGenericRGB(0.72f, 0.72f, 0.72f, 0.98f);
      CGContextSetFillColorWithColor(ctx, grid_color);
      CGContextFillRect(ctx, wnd_rect);
      CFRelease(grid_color);

      // draw every pixels
      for(int y = 0; y < CAPTURE_HEIGHT; ++y)
      {
        for(int x = 0; x < CAPTURE_WIDTH; ++x)
        {
          auto pixel = recorded_screen_render_data_buffer[
                               (CAPTURE_HEIGHT-1-y)*CAPTURE_WIDTH + x];

          CGContextSetRGBFillColor(ctx, pixel.r, pixel.g, pixel.b, 1.0);

          CGRect rect;
          // 8 for boarder shadow, 1 for box boarder
          rect.origin.x = 8 + 1 + (1+GRID_PIXEL)*x;
          rect.origin.y = 8 + 1 + (1+GRID_PIXEL)*y;
          rect.size.width  = GRID_PIXEL;
          rect.size.height = GRID_PIXEL;
          CGContextFillRect(ctx, rect);
        }
      }

      // draw black and white box around the center pixel color
      {
        int x = GRID_NUMUBER_L;
        int y = GRID_NUMUBER_L;
        CGRect rect;
        rect.origin.x = 8 + 1 + (1+GRID_PIXEL)*x - 1;
        rect.origin.y = 8 + 1 + (1+GRID_PIXEL)*y - 1;
        rect.size.width = GRID_PIXEL + 2;
        rect.size.height = GRID_PIXEL + 2;

        CGContextSetRGBFillColor(ctx, 0.0f, 0.0f, 0.0f, 1.0f);
        CGContextFillRect(ctx, rect);

        rect.origin.x = 8 + 1 + (1+GRID_PIXEL)*x;
        rect.origin.y = 8 + 1 + (1+GRID_PIXEL)*y;
        rect.size.width = GRID_PIXEL;
        rect.size.height = GRID_PIXEL;

        CGContextSetRGBFillColor(ctx, 1.0f, 1.0f, 1.0f, 1.0f);
        CGContextFillRect(ctx, rect);

        rect.origin.x = 8 + 1 + (1+GRID_PIXEL)*x + 1;
        rect.origin.y = 8 + 1 + (1+GRID_PIXEL)*y + 1;
        rect.size.width = GRID_PIXEL - 2;
        rect.size.height = GRID_PIXEL - 2;

        auto pixel = recorded_screen_render_data_buffer[
                               (CAPTURE_HEIGHT-1-y)*CAPTURE_WIDTH + x];

        CGContextSetRGBFillColor(ctx, pixel.r, pixel.g, pixel.b, 1.0);
        CGContextFillRect(ctx, rect);
      }

      CFRelease(clip_path);

    CGContextRestoreGState(ctx);

    CGContextDrawImage(ctx, wnd_rect, image_cicle_mask);
}

-(void) mouseDown: (NSEvent*)event
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    [[self window] close];
    [super mouseDown: event];
}

@end


void
GetCurrentCursorPosition
(
    float* const x, float* const y
)
{
    CGPoint mouse_pos;
    {
        auto event = ::CGEventCreate(nullptr);
        mouse_pos = ::CGEventGetUnflippedLocation(event);
        ::CFRelease(event);
    }
    *x = mouse_pos.x;
    *y = mouse_pos.y;
}


bool
RefreshScreenPixelDataWithinBound
(
    float central_x, float central_y,
    float bound_width, float bound_height,
    const WindowIDList& excluded_window_list,
    struct ScreenPixelData* const off_screen_render_data
)
{
    struct cf_object_releaser
    {
        CFTypeRef who;
    public:
        cf_object_releaser(CFTypeRef w): who(w) {}
    public:
        ~cf_object_releaser() { ::CFRelease(who); }
    };

    struct ns_object_releaser
    {
        NSObject* who;
    public:
        ns_object_releaser(NSObject* w): who(w) {}
    public:
        ~ns_object_releaser() { [who dealloc] ; }
    };

    auto window_list = \
    []()
    {
        auto list = ::CGWindowListCreate(kCGWindowListOptionOnScreenOnly, \
                                                             kCGNullWindowID);
        struct cf_object_releaser releaser(list);
        return ::CFArrayCreateMutableCopy(NULL, ::CFArrayGetCount(list), list);
    }();
    struct cf_object_releaser window_list_releaser(window_list);

    const auto window_list_size = ::CFArrayGetCount(window_list);
    const auto invalida_window_id = CGWindowID(kCGNullWindowID);
    for(const auto filter_wnd_id : excluded_window_list )
    {
        for( int idx = window_list_size - 1; idx >= 0; --idx )
        {
            auto crt_wnd_id = (CGWindowID)(uintptr_t)\
                    ::CFArrayGetValueAtIndex(window_list, idx);
            if( crt_wnd_id == filter_wnd_id )
            {
                ::CFArraySetValueAtIndex(window_list, idx, &invalida_window_id);
                break;
            }
        }
    }
    // fprintf(stderr, "window list size %lu\n", window_list_size);

    static auto main_display_height = []()
    {
        auto bound = ::CGDisplayBounds(::CGMainDisplayID());
        return bound.size.height;
    }();

    CGRect bound;
    bound.origin.x = central_x - bound_width/2.0;
    //! need this to correct cursor position !//
    bound.origin.y = main_display_height - central_y - bound_height/2.0;
    bound.size.width = bound_width;
    bound.size.height = bound_height;

    auto image = ::CGWindowListCreateImageFromArray(bound, \
                        window_list, kCGWindowImageNominalResolution);
    struct cf_object_releaser image_releaser(image);

    /*! cache this !*/ static const auto image_size_ok = \
    [](auto image, auto bound)
    {
        auto width = ::CGImageGetWidth(image);
        auto height = ::CGImageGetHeight(image);
        if( (width != bound.width) || (height != bound.height) )
        {
            fprintf(stderr, "captured image size error %lu %lu  \n", \
                                                            width, height);
            return NO;
        }
        return YES;
    }(image, bound.size);

    if( image_size_ok != YES )
    {
        return false; // return early
    }

    auto calculate_then_flush_result_to_the_buffer = \
    [](auto image, auto off_screen_render_data)
    {
        const auto src_width = ::CGImageGetWidth(image);
        const auto src_height = ::CGImageGetHeight(image);
        const auto src_colorspace = ::CGImageGetColorSpace(image);

        auto ns_image = [[NSBitmapImageRep alloc] initWithCGImage: image];
        struct ns_object_releaser ns_image_releaser(ns_image);

        auto ns_cs_sRGB = [NSColorSpace sRGBColorSpace];

        @autoreleasepool
        {
        auto ns_image_sRGB = [ns_image \
                        bitmapImageRepByConvertingToColorSpace: ns_cs_sRGB
                        renderingIntent: NSColorRenderingIntentDefault];

        for( size_t y = 0; y < src_height; ++y )
        {
            for( size_t x = 0; x < src_width; ++x )
            {
                auto color = [ns_image colorAtX: x y: y];

                CGFloat color_values[4] = {};
                color_values[0] = [color redComponent  ];
                color_values[1] = [color greenComponent];
                color_values[2] = [color blueComponent ];
                color_values[3] = [color alphaComponent];

                auto cg_color = ::CGColorCreate(src_colorspace, color_values);
                struct cf_object_releaser cg_color_releaser(cg_color);

                auto fixed_color = [[NSColor colorWithCGColor: cg_color]
                                         colorUsingColorSpace: ns_cs_sRGB];

                auto& pixel = off_screen_render_data[y*src_width+x];
                pixel.r = [fixed_color redComponent  ];
                pixel.g = [fixed_color greenComponent];
                pixel.b = [fixed_color blueComponent ];
                pixel.a = [fixed_color alphaComponent];
            }
        }
        }; // autoreleasepool

        // for( size_t y = 0; y < src_height; ++y )
        // {
        //     for( size_t x = 0; x < src_width; ++x )
        //     {
        //         const float f = 255.0f;
        //         auto& p = off_screen_render_data[y*src_width+x];
        //         printf("%f %f %f %f \n", (p.r*f), (p.g*f), (p.b*f), (p.a*f) );
        //     }
        // }
    };

    calculate_then_flush_result_to_the_buffer(image, off_screen_render_data);

    return true;
}


void
PreRun_Mode_Normal
(
    const class InstanceInfo* const instance_info
)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    fprintf(stderr, "screen record size: %4u %4u\n", \
                                CAPTURE_WIDTH, CAPTURE_HEIGHT);

    [NSApplication sharedApplication];

    // application does not appear in the Dock and does not have a menu bar
    [NSApp setActivationPolicy: NSApplicationActivationPolicyAccessory];
    [NSApp activateIgnoringOtherApps: YES];
    [NSApp setDelegate: [AppDelegate new]];
    [NSCursor hide];
}

void
PostRun_Mode_Normal
(
    const class InstanceInfo* const instance_info
)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    int x = GRID_NUMUBER_L;
    int y = GRID_NUMUBER_L;
    auto pixel = recorded_screen_render_data_buffer[y*CAPTURE_WIDTH+x];

    int r = pixel.r*255.0f;
    int g = pixel.g*255.0f;
    int b = pixel.b*255.0f;

    if( should_log_out_central_pixel_color == YES )
    {
        fprintf(stdout, "#%02X%02X%02X\n", r, g, b);
    }
    else
    {
        fprintf(stderr, "#%02X%02X%02X\n", r, g, b);
    }

    [NSCursor unhide];
}


void
PreRun_Mode_CheckScreenRecordPermision
(
    const class InstanceInfo* const instance_info
)
{
    class log_helper
    {
        const char* prefix = "Screen Record Permission Granted";
    public:
        BOOL permission_granted = NO;
    public:
        ~log_helper()
        {
            if( permission_granted == YES ) {
                fprintf(stdout, "%s: %s\n", prefix, "YES");
            } else {
                fprintf(stdout, "%s: %s\n", prefix, "NO");
            }
        }
    }
    the_log_helper;

    the_log_helper.permission_granted = CheckScreenRecordPermision();
}


void
PreRun_Mode_PromoteScreenRecordPermisionGrantWindow
(
    const class InstanceInfo* const instance_info
)
{
    class log_helper
    {
        const char* prefix = \
            "Promote Screen Record Permission Grant Window Succeeded";
    public:
        BOOL action_successed = NO;
    public:
        ~log_helper()
        {
            if( action_successed == YES ) {
                fprintf(stdout, "%s: %s\n", prefix, "YES");
            } else {
                fprintf(stdout, "%s: %s\n", prefix, "NO");
            }
        }
    }
    the_log_helper;

    @autoreleasepool
    {
        auto bundle_id = instance_info->\
                    CommandLineParameter<std::string>("--bundle-id=");

        auto white_space = NSCharacterSet.whitespaceAndNewlineCharacterSet;

        auto ns_bundle_id = \
                [[[NSString alloc]
                        initWithUTF8String: bundle_id.c_str()]
                             stringByTrimmingCharactersInSet: white_space];

        if( [ns_bundle_id length] == 0 )
        {
            return;
        }

        auto task = [NSTask new];
        auto pipe_p2c_stdin  = [NSPipe new];
        auto pipe_c2p_stdout = [NSPipe new];
        auto pipe_c2p_stderr = [NSPipe new];
        [task setStandardInput: pipe_p2c_stdin];
        [task setStandardOutput: pipe_c2p_stdout];
        [task setStandardError: pipe_c2p_stderr];

        auto arguments = [[NSArray alloc] initWithObjects:
                        @"reset", @"ScreenCapture", ns_bundle_id, nil];

        [task setArguments: arguments];

        [task setLaunchPath: @"/usr/bin/tccutil"];
        [task launch];
        [task waitUntilExit];
        int status = [task terminationStatus];


        auto pipe_c2p_stdout_data =  \
                [[pipe_c2p_stdout fileHandleForReading] availableData];

        auto cs_stdout = (char*)[pipe_c2p_stdout_data bytes];
        fprintf(stderr, "%s", cs_stdout);

        auto ns_stdout = \
                [[[NSString alloc]
                        initWithUTF8String: cs_stdout]
                             stringByTrimmingCharactersInSet: white_space];

        if
        (
            status == 0
            &&
            [ns_stdout hasPrefix: @"successfully reset ScreenCapture"]
            &&
            [ns_stdout hasSuffix: ns_bundle_id]
        )
        {
            the_log_helper.action_successed = YES;
        }
    }

    float central_x = 100, central_y = 100;
    float bound_width = 4, bound_height = 4;

    WindowIDList no_exclude_window_list;

    auto off_screen_render_data = new \
            ScreenPixelData[bound_width*bound_height];

    RefreshScreenPixelDataWithinBound( \
        central_x, central_y, bound_width, bound_height, \
                no_exclude_window_list, off_screen_render_data );

    delete[] off_screen_render_data;
}


void
PreRun_Mode_Unit_Test_1
(
    const class InstanceInfo* const instance_info
)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    float central_x = 200, central_y = 200;
    float bound_width = 4, bound_height = 4;
    // float bound_width = 100, bound_height = 100;

    WindowIDList no_exclude_window_list;

    auto off_screen_render_data = new \
                ScreenPixelData[bound_width*bound_height];

    RefreshScreenPixelDataWithinBound( \
        central_x, central_y, bound_width, bound_height, \
                no_exclude_window_list, off_screen_render_data );

    delete[] off_screen_render_data;
}


void
PreRun_Mode_Unit_Test_2
(
    const class InstanceInfo* const instance_info
)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    struct MonitorInfo::Initializer init;
}


void
PreRun(class Instance* instance)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    auto inst_info = instance->InstanceInfo();
    auto exec_mode = inst_info->CommandLineParameter<int>("--mode=");

    switch(exec_mode)
    {
        case 0:
            PreRun_Mode_Normal(inst_info);
        break;
        case 1:
            PreRun_Mode_CheckScreenRecordPermision(inst_info);
        break;
        case 2:
            PreRun_Mode_PromoteScreenRecordPermisionGrantWindow(inst_info);
        break;
        case 3:
        {
            auto exec_time = inst_info->CommandLineParameter<int>("--time=");
            auto test_what = inst_info->CommandLineParameter<int>("--what=");

            switch(test_what)
            {
                case 1:
                {
                    while(exec_time--) {
                        PreRun_Mode_Unit_Test_1(inst_info);
                    }
                }
                break;
                case 2:{
                    while(exec_time--) {
                        PreRun_Mode_Unit_Test_2(inst_info);
                    }
                }
                break;
                default:
                    // pass
                break;
            }
        }
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
    auto exec_mode = inst_info->CommandLineParameter<int>("--mode=");

    switch(exec_mode)
    {
        case 0:
            PostRun_Mode_Normal(inst_info);
        break;
        default:
            // pass
        break;
    }
}

