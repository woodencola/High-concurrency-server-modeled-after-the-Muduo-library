#pragma once
#include<cstdio>
#include<ctime>
#include<pthread.h>

#define INF 0
#define DBG 1
#define ERR 2
#define LOG_LEVEL DBG
#define LOG(LEVEL,format,...) do\
{ if(LEVEL<LOG_LEVEL)break;          \
  time_t t = time(nullptr);\
  struct tm* time = localtime(&t);\
  char formattime[32] ={0};\
  strftime(formattime,31,"%H:%M:%S",time);\
  fprintf(stdout,"[thread id is %lu:%s]""[%s:%d]:" format "\n",(unsigned long)pthread_self(),formattime,__FILE__,__LINE__,##__VA_ARGS__);  \
} while (0);

#define INF_LOG(format,...) LOG(INF,format,##__VA_ARGS__)
#define DBG_LOG(format,...) LOG(DBG,format,##__VA_ARGS__)
#define ERR_LOG(format,...) LOG(ERR,format,##__VA_ARGS__)
 
