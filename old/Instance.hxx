#pragma once

#include <string>
#include <sstream>
#include <string_view>

#include <functional>

#include <forward_list>

#ifdef _MSC_VER
    #define __PRETTY_FUNCTION__ __FUNCTION__
#endif // _MSC_VER

class ResourceSink
{
public:
    ResourceSink()
    {
        fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    }
public:
    ~ResourceSink()
    {
        for(const auto& deletor : resource_deleter_list_ ) {
            deletor();
        }
        fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    }
public:
    void TakeCare(std::function<void()>&& deletor)
    {
        // last in, first out, that's why we use push_front
        resource_deleter_list_.push_front(deletor);
    }
private:
    std::forward_list<std::function<void()>> resource_deleter_list_;
};


class InstanceInfo
{
public:
    InstanceInfo();
    ~InstanceInfo();
public:
#ifdef _WIN32
    typedef std::wstring native_str_t;
    typedef std::wstring_view native_str_view_t;
    typedef std::wstringstream native_str_stream_t;
#else
    typedef std::string native_str_t;
    typedef std::string_view native_str_view_t;
    typedef std::stringstream native_str_stream_t;
#endif
    //?? assume the OS argv,argc are const and not freed in memory block ??//
    typedef std::forward_list<native_str_view_t> command_line_parameters_t;
private:
    command_line_parameters_t command_line_parameters_;
    uint32_t command_line_parameters_count_ = 0;
private:
    native_str_t feasible_execuable_file_path_;
public:
    const auto& FeasibleExecuableFilePath() const {
        return feasible_execuable_file_path_;
    }
public:
    const auto CommandLineParametersCount() const {
        return command_line_parameters_count_;
    }
public:
    template<typename T>
    T CommandLineParameter(const native_str_t& param_name) const
    {
        T result{};
        for(const auto& item: command_line_parameters_)
        {
            if( (item.length() >= param_name.length()) && \
                (item.substr(0, param_name.length()) == param_name) )
            {
                native_str_stream_t parser;
                parser << item.substr(param_name.length());
                parser >> result;
            }
        }
        return result;
    };
};


class RuntimeHost
{
public:
    virtual ~RuntimeHost() = default;
protected:
    class RuntimeHost* the_runtime_host_ = nullptr;
public:
    virtual int Execute() const {
        return the_runtime_host_->Execute();
    }
};


class Instance
{
    friend class RuntimeHostCute;
    friend class RuntimeHostNative; // ui/io ??
private:
    /*********************************************/
    /* important !! keep these lines in order !! */
    /*                                           */
    class ResourceSink resource_sink_;    /* 1st */
    class InstanceInfo instance_info_;    /* 2nd */
    /*                                           */
    /*********************************************/
public:
    const class InstanceInfo* const InstanceInfo() const {
        return &instance_info_;
    }
private:
    static inline std::function<void(class Instance*)> fun_pre_init_;
    static inline std::function<void(class Instance*)> fun_post_init_;
public:
    static auto& PreInit() {
        return Instance::fun_pre_init_;
    }
    static auto& PostInit() {
        return Instance::fun_post_init_;
    }
private:
    Instance()
    {
        fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
        if( fun_pre_init_ ) fun_pre_init_(this);
        Instance::instance_ = this;
        if( fun_post_init_ ) fun_post_init_(this);
    }
    ~Instance()
    {
        fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    }
private:
    static inline class Instance* instance_;
public:
    [[nodiscard]]
    static Instance* Init()
    {
        static class Instance instance;
        return &instance;
    }
private:
    class RuntimeHost* runtime_host_ = nullptr;
public:
    static auto& RuntimeHost() { return instance_->runtime_host_; }
private:
    std::function<void(class Instance*)> fun_pre_run_;
    std::function<void(class Instance*)> fun_post_run_;
public:
    auto& PreRun() { return fun_pre_run_; }
    auto& PostRun() { return fun_post_run_; }
private:
    int run()
    {
        if( fun_pre_run_ ) fun_pre_run_(instance_);
        const int result = runtime_host_->Execute();
        if( fun_post_run_ ) fun_post_run_(instance_);
        return result;
    }
public:
    static int Run() {
        return instance_->run();
    }
};

