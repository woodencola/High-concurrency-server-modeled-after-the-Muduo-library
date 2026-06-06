#pragma once

#include <iostream>
#include <vector>
#include <sys/types.h>
#include<cstdint>
#include <assert.h>
#include <algorithm>
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
            //1.当前空间足够,写入空间小于末尾空闲空间
            if(len<=GetTailIdleSpaceSize())
            {
                return;
            }
            else if(len<=GetTailIdleSpaceSize()+GetHeadIdleSpaceSize())
            {
                //2.当前空间小于末尾加起始的空间空间
                //空闲空间 读位置 可读大小 写位置 空间空间
                uint64_t enablereadsize = CurrentEnableReadSpaceSize();
                std::copy(GetCurrentReadPosition(),GetCurrentReadPosition()+enablereadsize,begin());
                _read_index = 0;
                _write_index = enablereadsize;
            }
            else
            {
                _buffer.resize(_write_index+len);
            }
        }
        // 写入数据
        void Write(void *data, uint64_t len);
        // 读取数据
        void Read(void *buffer, uint64_t len);
        // 清理空间
        void clear();

        Buffer() : _read_index(0), _write_index(0), _buffer(BUFFERSIZE) {};
        ~Buffer() {};
    };

}