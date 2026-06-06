#pragma once

#include <iostream>
#include <string>
#include <cstdio>
#include <ctime>
#include <sys/time.h>
#include<unistd.h>
#include <sys/types.h>
#include "Mutex.hpp"
#include <cstring>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <memory>

namespace LOG_MODULE
{
    // 策略模式构建的日志模块
    enum class LOGLEVEL
    {
        INFO,
        DEBUG,
        WARNNING,
        ERROR,
        FATAL

    };
    std::string LogLevel2String(LOGLEVEL lv)
    {
        switch (lv)
        {
        case LOGLEVEL::INFO:
            return "INFO";
            break;
        case LOGLEVEL::DEBUG:
            return "DEBUG";
            break;
        case LOGLEVEL::WARNNING:
            return "WARNNING";
            break;
        case LOGLEVEL::ERROR:
            return "ERROR";
            break;
        case LOGLEVEL::FATAL:
            return "FATAL";
            break;
        default:
            return "UNKNORWN";
            break;
        }
    }
    std::string gettime()
    {
        // 1.拿到时间戳
        struct timeval current_time;
        int n = gettimeofday(&current_time, nullptr);
        // 2.拿到格式化信息
        (void)n;
        struct tm struct_time;
        localtime_r(&current_time.tv_sec, &struct_time);
        char time[1024];
        memset(time, 0, sizeof time);
        snprintf(time, sizeof time, "%04d-%02d-%02d: %02d-%0d-%02d",
                 struct_time.tm_year + 1900,
                 struct_time.tm_mon + 1,
                 struct_time.tm_mday,
                 struct_time.tm_hour,
                 struct_time.tm_min,
                 struct_time.tm_sec);
        return time;
    }

    class LogStrategy
    {
    public:
        virtual void SyncStrategy(const std::string &s) = 0;
        virtual ~LogStrategy() = default;

    private:
    };

    // 1.向显示器刷新
    class WindowSyncStregy : public LogStrategy
    {
    public:
        WindowSyncStregy() {}
        ~WindowSyncStregy() =default;
        void SyncStrategy(const std::string &s) override
        {
            {
                Mutex_Moudle::Mutex_Grard guard(lock);
                std::cout << s << std::endl;
            }
        }

    private:
        Mutex_Moudle::Mutex lock;
    };
    // 像文件刷新
    // 缺省
    std::string defaultname = "log.txt";
    std::string defaultpath = "./log";
    class FileSyncStregy : public LogStrategy
    {
    public:
        FileSyncStregy(const std::string name = defaultname, const std::string path = defaultpath)
            : _name(name), _path(path)
        {
            {
                Mutex_Moudle::Mutex_Grard guard(lock);
                if (std::filesystem::exists(_path))
                    return;
                try
                {
                    std::filesystem::create_directories(_path);
                }
                catch (const std::filesystem::filesystem_error &e)
                {
                    std::cerr << e.what() << '\n';
                }
            }
        }
        virtual void SyncStrategy(const std::string &s) override
        {
            {
                Mutex_Moudle::Mutex_Grard guard(lock);
                // 判断路径最后一个字符是否带/
                if (!_path.empty() && _path.back() != '/')
                {
                    _path += '/';
                }
                // 最终文件名称
                std::string filename = _path + _name;
                std::ofstream ofs(filename, std::ios::app);
                if (!ofs.is_open())
                {
                    std::cerr << "file not open" << std::endl;
                    return;
                }
                ofs << s << "\n";
                ofs.close();
            }
        }
         ~FileSyncStregy() =default;

    private:
        std::string _name;
        std::string _path;
        Mutex_Moudle::Mutex lock;
    };

    class logger
    {

    public:
        logger()
        {
            _strategy = nullptr;
        }
        ~logger() {}
        void usewindowstrategy()
        {
            _strategy = std::make_unique<WindowSyncStregy>();
        }
        void usefilestrategy()
        {
            _strategy = std::make_unique<FileSyncStregy>();
        }
        class loggermassger
        {
        public:
            loggermassger(LOGLEVEL level, size_t line, const std::string &filename, logger &Logger)
                : _level(level), _line(line), _filename(filename), _logger(Logger), _pid(getpid()), current_time(gettime())
            {
                std::stringstream ss;
                ss << "[" << current_time << "] "
                   << "[" << LogLevel2String(_level) << "] "
                   << "[" << _pid << "] "
                   << "[" << _line << "] "
                   << "[" << _filename << "] ";
                loginfo = ss.str();
            }
            // 模板类型支持各自输出
            template <typename T>
            loggermassger &operator<<(const T &data)
            {
                std::stringstream ss;
                ss << data;

                loginfo += ss.str();
                return *this;
            }
            ~loggermassger()
            {
                // 最后刷新在这里
                _logger._strategy->SyncStrategy(loginfo);
            }

        private:
            std::string current_time;
            LOGLEVEL _level;
            pid_t _pid;
            size_t _line;
            std::string _filename;
            logger &_logger; // 使用刷新策略
            std::string loginfo;
        };
        // LOG(日志等级)<<xxxxxxx;
        //  1.LOG用宏定义封装
        //   2.重载()创建loggermassger,这里必须拷贝,为了刷新
        //  3. 在loggermassger里重载<<输出
        // 4.<<返回自身类型的引用,支持多次输出
        // 5.loggermassger析构使用外部传进来的刷新策略
        loggermassger operator()(LOGLEVEL level, const std::string &filename, size_t line)
        {
            return loggermassger(level, line, filename, *this);
        }

    private:
        std::unique_ptr<LogStrategy> _strategy;
    };

    logger log;
#define WINDOWS_LOG_INITAL()     \
    do                           \
    {                            \
        log.usewindowstrategy(); \
    } while (0)
#define FILE_LOG_INITAL()      \
    do                         \
    {                          \
        log.usefilestrategy(); \
    } while (0)
#define LOG(level) log(level, __FILE__, __LINE__)

}