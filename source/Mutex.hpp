#pragma once 

#include<iostream>
#include<pthread.h>



namespace Mutex_Moudle
{
    class Mutex
    {
        public:
        Mutex(){
            //参数可修改成进程版本
            pthread_mutex_init(&_mutex,nullptr);
        }
        void Lock()
        {
            pthread_mutex_lock(&_mutex);
        }
        pthread_mutex_t* Ptr()
        {
            return &_mutex;
        }

        void unLock()
        {
            pthread_mutex_unlock(&_mutex);
        }

        ~Mutex(){
            pthread_mutex_destroy(&_mutex);
        }
        private:
        pthread_mutex_t _mutex;
    };
    class Mutex_Grard
    {
        //RAII
        public:
     
        
        Mutex_Grard(Mutex& mutexref):_mutexref(mutexref)
        {
            _mutexref.Lock();
        }
        ~Mutex_Grard()
        {
            _mutexref.unLock();
        }
        private:
        Mutex& _mutexref;
    };
}
