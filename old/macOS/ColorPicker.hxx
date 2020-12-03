#pragma once

#include <vector>

#include <AppKit/AppKit.h>


@interface AppDelegate: NSObject <NSApplicationDelegate>

@end


@interface MainWindow : NSWindow

-(id) init;
-(BOOL) canBecomeMainWindow;
-(BOOL) canBecomeKeyWindow;
-(void) keyDown: (NSEvent*)event;

@end


@interface MainView : NSView

-(id) init;
-(void) drawRect: (NSRect)dirtRect;
-(void) mouseDown: (NSEvent*)event;

@end


void
GetCurrentCursorPosition
(
    float* const x, float* const y
);

typedef CGWindowID WindowID ;
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

bool
RefreshScreenPixelDataWithinBound
(
    float central_x, float central_y,
    float bound_width, float bound_height,
    const WindowIDList& excluded_window_list,
    struct ScreenPixelData* const off_screen_render_data
);

struct MonitorInfo
{
    static const uint32_t LIST_SIZE = 16; //!! 16 is enought

    inline static auto ID_LIST = new UInt32[LIST_SIZE]();
    inline static auto BOUND_LIST = new CGRect[LIST_SIZE]();
    inline static auto COLOR_SPACE_LIST = new CGColorSpaceRef[LIST_SIZE]();
    inline static auto NS_COLOR_SPACE_LIST_PIN = new NSColorSpace*[LIST_SIZE]();
    inline static auto NS_COLOR_SPACE_LIST = new NSColorSpace*[LIST_SIZE]();

    inline static uint32_t AVAILABLE_COUNT = 0;

    struct Initializer
    {
        Initializer();
        ~Initializer();
    } Initialized;

    auto ID(const uint32_t idx) { return ID_LIST[idx]; }
    auto Bound(const uint32_t idx) { return BOUND_LIST[idx]; }
    auto ColorSpace(const uint32_t idx) { return COLOR_SPACE_LIST[idx]; }

}; // struct MonitorInfo

MonitorInfo::Initializer::Initializer()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    if( auto result = ::CGGetActiveDisplayList(LIST_SIZE, ID_LIST, \
                                                    &AVAILABLE_COUNT);
        result != kCGErrorSuccess )
    {
        fprintf(stderr, "::CGGetActiveDisplayList Failed\n");
        throw std::runtime_error("::CGGetActiveDisplayList Failed\n");
    }

    for(uint32_t idx=0; idx < AVAILABLE_COUNT; ++idx)
    {
        fprintf(stderr, "monitor %u id: 0x%08X ", idx, ID_LIST[idx]);
        //! in the global display coordinate space !//
        BOUND_LIST[idx] = ::CGDisplayBounds(ID_LIST[idx]);
        fprintf(stderr, "bound: %+6.0f %+6.0f %+6.0f %+6.0f ",
                BOUND_LIST[idx].origin.x, BOUND_LIST[idx].origin.y, \
                BOUND_LIST[idx].size.width, BOUND_LIST[idx].size.height\
            );
        //! need this to correct color value !//
        COLOR_SPACE_LIST[idx] = ::CGDisplayCopyColorSpace(ID_LIST[idx]);

        NS_COLOR_SPACE_LIST_PIN[idx] = [NSColorSpace alloc];
        NS_COLOR_SPACE_LIST[idx] = \
            [ NS_COLOR_SPACE_LIST_PIN[idx]
                 initWithCGColorSpace: COLOR_SPACE_LIST[idx]];

        // fprintf(stderr, "\n");
        // fprintf(stderr, "--%p--\n", NS_COLOR_SPACE_LIST_PIN[idx]);
        // fprintf(stderr, "--%p--\n", NS_COLOR_SPACE_LIST[idx]);

        @autoreleasepool
        {
            auto ns_cs_name = [NS_COLOR_SPACE_LIST[idx] localizedName];
            if( ns_cs_name != nil )
            {
                auto cs_name = (CFStringRef)ns_cs_name;
                auto str_len = ::CFStringGetLength(cs_name);
                auto max_str_len = 4 * str_len + 1;
                auto cs_name_str = new char[max_str_len]();
                ::CFStringGetCString(cs_name, cs_name_str, max_str_len, \
                                                    kCFStringEncodingUTF8);
                fprintf(stderr, "colorspace: %s", cs_name_str);
                delete[] cs_name_str;
            }
            else
            {
                fprintf(stderr, "colorspace: unknow");
            }
        } // autoreleasepool
        fprintf(stderr, "\n");
    }
}

MonitorInfo::Initializer::~Initializer()
{
    for(uint32_t idx=0; idx < AVAILABLE_COUNT; ++idx)
    {
        //!! When these two values are equal, it means the two NSColorSpace
        //!! objects are both newly created, NOT the system cached ones, so
        //!! we need to free them. Since they are just referring to the same
        //!! object, therefore this dealloc() will and must get called only
        //!! once
        if( NS_COLOR_SPACE_LIST[idx] == NS_COLOR_SPACE_LIST_PIN[idx] )
        {
            [NS_COLOR_SPACE_LIST_PIN[idx] dealloc];
        }
    }

    for(uint32_t idx=0; idx < AVAILABLE_COUNT; ++idx)
    {
        ::CGColorSpaceRelease(COLOR_SPACE_LIST[idx]);
    }

    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}


BOOL
CheckScreenRecordPermision()
{
    BOOL permission_granted = YES;
    BOOL should_check_avilable = NO;

    if( @available(macOS 10.15, *) )
    {
        should_check_avilable = YES;
        permission_granted = NO;
    }
    else
    {
        should_check_avilable = NO;
        permission_granted = YES;
    }

    if( should_check_avilable == NO )
    {
        return YES; // allways return YES when not need to check
    }

    auto crt_app = [NSRunningApplication currentApplication];
    auto crt_pid = [NSNumber numberWithInteger: [crt_app processIdentifier]];

    auto wnd_list = ::CGWindowListCopyWindowInfo( \
                            kCGWindowListOptionOnScreenOnly, kCGNullWindowID);

    for(CFIndex idx = 0; idx < ::CFArrayGetCount(wnd_list); ++idx)
    {
        auto wnd_info = (CFDictionaryRef)::CFArrayGetValueAtIndex( \
                                                            wnd_list, idx);
        auto wnd_name = ::CFDictionaryGetValue(wnd_info, kCGWindowName);
        auto wnd_pid = ::CFDictionaryGetValue(wnd_info, kCGWindowOwnerPID);

        auto ns_wnd_name = (NSString*)wnd_name;
        if( [(NSNumber*)wnd_pid isEqual: crt_pid] )
        {
            continue;
        }

        pid_t pid = [(NSNumber*)wnd_pid intValue];
        auto wnd_app = [NSRunningApplication
                            runningApplicationWithProcessIdentifier:pid];
        if( wnd_app == nil )
        {
            continue;
        }

        auto wnd_exe_name = [[wnd_app executableURL] lastPathComponent];
        if( wnd_name )
        {
            if( [wnd_exe_name isEqual:@"Dock" ] )
            {
                // skip
            }
            else
            {
                permission_granted = YES;
                break;
            }
        }
    }

    ::CFRelease(wnd_list);

    return permission_granted;
}

