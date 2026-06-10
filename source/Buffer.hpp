#pragma once

#include <iostream>
#include <vector>
#include <sys/types.h>
#include <cstdint>
#include <assert.h>
#include <algorithm>
#include <string.h>
#include <string>
namespace Buffer_Module
{
    const static uint64_t BUFFERSIZE = 1024;
    class Buffer
    {
    private:
        std::vector<char> _buffer;
        uint64_t _read_index;
        uint64_t _write_index;

    private:
        char *begin()
        {
            // return &*_buffer.begin();
            return _buffer.data();
        }

    public:
        // 获取当前可写位置
        char *GetCurrentWritePosition()
        {
            return begin() + _write_index;
        }

        // 获取当前读位置
        char *GetCurrentReadPosition()
        {
            return begin() + _read_index;
        }
        // 获取写入位置之后到末尾的空闲空间大小
        uint64_t GetTailIdleSpaceSize()
        {
            return _buffer.size() - _write_index;
        }
        // 获取起始位置到读取位置的空闲空间大小
        uint64_t GetHeadIdleSpaceSize()
        {
            return _read_index;
        }
        // 获取当前可读空间大小
        uint64_t CurrentEnableReadSpaceSize()
        {
            return _write_index - _read_index;
        }
        // 移动写入位置到指定长度
        void MoveWritePosition(uint64_t len)
        {
            assert(len <= GetTailIdleSpaceSize());
            _write_index += len;
        }
        // 移动读取位置到指定长度
        void MoveReadPosition(uint64_t len)
        {
            assert(len <= CurrentEnableReadSpaceSize());
            _read_index += len;
        }
        // 确保写入空间是否足够 (移动或者扩容,或者从当前位置直接写)
        void EnsureWriteSpaceSize(uint64_t len)
        {
            // 1.当前空间足够,写入空间小于末尾空闲空间
            if (len <= GetTailIdleSpaceSize())
            {
                return;
            }
            else if (len <= GetTailIdleSpaceSize() + GetHeadIdleSpaceSize())
            {
                // 2.当前空间小于末尾加起始的空间空间
                // 空闲空间 读位置 可读大小 写位置 空间空间
                uint64_t enablereadsize = CurrentEnableReadSpaceSize();
                std::copy(GetCurrentReadPosition(), GetCurrentReadPosition() + enablereadsize, begin());
                _read_index = 0;
                _write_index = enablereadsize;
            }
            else
            {
                _buffer.resize(_write_index + len);
            }
        }

    private:
        // 写入数据
        void Write(const void *data, uint64_t len)
        {
            if(len==0) return;
            // 1.需要确保当前空间足够
            EnsureWriteSpaceSize(len);
            const char *d = (char *)data;
            std::copy(d, d + len, GetCurrentWritePosition());
        }
        // 读取数据
        void Read(void *buffer, uint64_t len)
        {
            // 读取的空间一定要小于可读取的大小
            // 我底层也不知道有多少可读取的空间,后面仍然需要修改
            assert(len <= CurrentEnableReadSpaceSize());
            char *b = (char *)buffer;
            std::copy(GetCurrentReadPosition(), GetCurrentReadPosition() + len, b);
        }

        void WriteAsString(std::string &s)
        {
            Write(&s[0], s.size());
        }
        void WriteAsBuffer(Buffer &b)
        {
            // 这里不能用const buffer ,我们的函数都是const 的调用不动
            Write(b.GetCurrentReadPosition(), b.CurrentEnableReadSpaceSize());
        }
        std::string ReadAsString(uint64_t len)
        {
            assert(len <= CurrentEnableReadSpaceSize());
            std::string ret;
            ret.resize(len);
            Read(&ret[0], len);
            return ret;
        }
        char * GetCRLF()
        {
            
            char* ret = (char*)memchr(GetCurrentReadPosition(),'\n',CurrentEnableReadSpaceSize());
            return ret;
        }
        std::string GetLine()
        {
            char* LF = GetCRLF();
            if(LF==nullptr)
            {
                return "";
            }
            //从\n的位置-读的起始位置就是我们的需要的+1是为了将\n读出来,这里两个位置可以理解成下标
            //而我们的参数都是个数
            return ReadAsString(LF-GetCurrentReadPosition()+1);
        }
    public:
        std::string GetLineAndAdd()
        {
            std::string ret =GetLine();
            MoveReadPosition(ret.size());
            return ret;

        }
        void WriteAsBufferAndAdd(Buffer &b)
        {
            uint64_t size = b.CurrentEnableReadSpaceSize();
            WriteAsBuffer(b);
            MoveWritePosition(b.CurrentEnableReadSpaceSize());
            b.MoveReadPosition(size);
        }
        void ReadAndPop(void *buffer, uint64_t len)
        {
            assert(len <= CurrentEnableReadSpaceSize());
            Read(buffer, len);
            MoveReadPosition(len);
        }
        void WriteAndAdd(const void *data, uint64_t len)
        {
            Write(data, len);
            MoveWritePosition(len);
        }
        void WtStringAndAdd(std::string &s)
        {
            WriteAsString(s);
            MoveWritePosition(s.size());
        }

        std::string RdStringAndPop(uint64_t len)
        {
            assert(len <= CurrentEnableReadSpaceSize());
            
            std::string ret =  ReadAsString(len);
            MoveReadPosition(len);
            return ret;
        }
        // 清理空间
        void clear()
        {
            _read_index = 0;
            _write_index = 0;
        }

        Buffer() : _read_index(0), _write_index(0), _buffer(BUFFERSIZE) {};
        ~Buffer() {};
    };

}