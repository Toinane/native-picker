#include "Instance.hxx"

#include <vector>
#include <iostream>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <Windows.h>
#endif

#ifdef __APPLE__
    #include <AppKit/AppKit.h>
    #include <crt_externs.h> // for _NSGetArgv, _NSGetArgc
    // #include <mach-o/dyld.h> // for _NSGetExecutablePath
#endif


InstanceInfo::InstanceInfo()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

#if defined(_WIN32)

    auto shell32_dll = ::LoadLibrary(L"Shell32.dll");
    auto build_command_line =
        reinterpret_cast<LPWSTR*(*)(LPCWSTR, int*)>(
            ::GetProcAddress(shell32_dll, "CommandLineToArgvW"));

    const native_str_view_t full_cmd_line_in_one_str(::GetCommandLineW());

    int arguments_number = 0;
    auto arguments_buffer_list = build_command_line( \
                         full_cmd_line_in_one_str.data(), &arguments_number);
    if( (arguments_buffer_list == NULL) || (arguments_number == 0) )
    {
        fprintf(stderr, "Fetech Command Lines Parameters Error\n");
        throw std::runtime_error("Fetech Command Line Parameters Error");
    }

    for( int idx = 0 ; idx < arguments_number ; ++idx )
    {
        native_str_view_t argument(arguments_buffer_list[idx]);
        fwprintf(stderr, L"ARGV[%d]:%s\n", idx, argument.data());
        //! Because CommandLineToArgvW actually alloc a continue memory block
        //! inside itself, and since we do not release this memory, so I just
        //! use std::move here.
        command_line_parameters_.push_front(std::move(argument));
    }
    command_line_parameters_.reverse();
    command_line_parameters_count_ = uint32_t(arguments_number-1);

    const auto possible_name = command_line_parameters_.front();
    command_line_parameters_.pop_front(); // !! remove the first argv !!

    switch( full_cmd_line_in_one_str.find_first_of(possible_name) )
    {
        case 0:
        {
            //* the possible_name is just the cmd line argument, usually
            //* happens when this execuable file get called directly
            //* from cmd.exe
            if( possible_name == full_cmd_line_in_one_str )
            {
                feasible_execuable_file_path_ = native_str_t(possible_name);
            }
            //* the possible_name is just the first part of the whole cmd
            //* line argument, usually happens when this execuable get
            //* called directly from cmd.exe but with extra arguments
            if
            (
                (full_cmd_line_in_one_str.size() > possible_name.size())
                  &&
                (full_cmd_line_in_one_str[possible_name.size()] == L' ')
            )
            {
                feasible_execuable_file_path_ = native_str_t(possible_name);
            }
        }
        break;
        case 1:
        {
            //* sometime the execuable name is wrapper with quot chars, usually
            //* happens when this execuable get launched from explorer.exe in
            //* folder path with white space or special chars
            if
            (
                (full_cmd_line_in_one_str[0] == L'\"')
                &&
                (full_cmd_line_in_one_str[possible_name.size()+1] == L'\"')
            )
            {
                feasible_execuable_file_path_ = native_str_t( \
                  full_cmd_line_in_one_str.substr(0, possible_name.size()+2) );
            }
        }
        break;
    }

    if( feasible_execuable_file_path_.empty() == true ){
        fprintf(stderr, "Fetch Feasible Executable Name Failed\n");
        throw std::runtime_error("Fetch Feasible Executable Name Failed");
    }

    fwprintf(stderr, L"!!! Feasible Executable Name: ->>>%s<<<-\n",
                                     feasible_execuable_file_path_.data());

#elif defined(__APPLE__)
    auto arguments_number = *_NSGetArgc();
    auto arguments_buffer_list = *_NSGetArgv();
    for( int idx = 1 ; idx < arguments_number ; ++idx )
    {
        native_str_view_t argument(arguments_buffer_list[idx]);
        fprintf(stderr, "ARGV[%d]:%s\n", idx, argument.data());
        command_line_parameters_.push_front(std::move(argument));
    }
    command_line_parameters_.reverse();
    command_line_parameters_count_ = uint32_t(arguments_number-1);

    feasible_execuable_file_path_ = native_str_t(arguments_buffer_list[0]);

    fprintf(stderr, "!!! Feasible Executable Name: ->>>%s<<<-\n",
                                     feasible_execuable_file_path_.data());
#else
    #error Unknow OS
#endif
}


InstanceInfo::~InstanceInfo()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}


class RuntimeHostNative final : public RuntimeHost
{
public:
    RuntimeHostNative(const Instance* const instance);
    ~RuntimeHostNative();
private:
    int Execute() const;
private:
#if defined(_WIN32)
    HINSTANCE instance_handle_ = NULL;
#elif defined(__APPLE__)
    // TODO:
    NSAutoreleasePool* auto_release_pool_;
#else
    #error Unknow OS
#endif
};


RuntimeHostNative::RuntimeHostNative(const Instance* const instance)
{
#if defined(_WIN32)
    instance_handle_ = ::GetModuleHandle(NULL);
#elif defined(__APPLE__)
    auto_release_pool_ = [NSAutoreleasePool new];
#else
    #error Unknow OS
#endif
}


RuntimeHostNative::~RuntimeHostNative()
{
#if defined(_WIN32)

#elif defined(__APPLE__)
    // #TODO
#else
    #error Unknow OS
#endif
}


int
RuntimeHostNative::Execute() const
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
#if defined(_WIN32)
    MSG msg;
    while( ::GetMessage(&msg, nullptr, 0, 0) ) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    return (int) msg.wParam;
#elif defined(__APPLE__)
    [NSApp run];
    [auto_release_pool_ drain];
    return 0;
#else
    #error Unknow OS
#endif
}


extern void PreRun(class Instance* instance);
extern void PostRun(class Instance* instance);


int
main(int argc, char* argv[])
{
    auto instance = Instance::Init();

    instance->RuntimeHost() = new RuntimeHostNative(instance);

    instance->PreRun() = PreRun;
    instance->PostRun() = PostRun;

    return Instance::Run();
}

