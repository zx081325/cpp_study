#include "../core/timer.h"
#include "../core/os.h"

/*
 * timer.cpp
 * 作者: David Wu
 */

// WINDOWS平台实现 -------------------------------------------------------------

#ifdef OS_IS_WINDOWS
#include <windows.h>
#include <ctime>

ClockTimer::ClockTimer()
{
  reset(); // 构造函数中重置计时
}

ClockTimer::~ClockTimer()
{

}

void ClockTimer::reset()
{
  initialTime = (int64_t)GetTickCount64(); // 记录初始时间戳
}

double ClockTimer::getSeconds() const
{
  int64_t newTime = (int64_t)GetTickCount64(); // 获取当前时间
  return (double)(newTime-initialTime)/1000.0; // 计算时间差,转换为秒
}

int64_t ClockTimer::getPrecisionSystemTime() 
{
  return (int64_t)GetTickCount64(); // Windows使用毫秒级时间戳
}

#endif

// UNIX平台实现 --------------------------------------------------------------

#ifdef OS_IS_UNIX_OR_APPLE
#include <chrono>

ClockTimer::ClockTimer()
{
  reset(); // 构造函数中重置计时
}

ClockTimer::~ClockTimer()
{

}

void ClockTimer::reset()
{
  auto d = std::chrono::steady_clock::now().time_since_epoch(); // 获取初始时间点
  initialTime = std::chrono::duration<int64_t,std::nano>(d).count(); // 转换为纳秒级时间戳
}

double ClockTimer::getSeconds() const
{
  auto d = std::chrono::steady_clock::now().time_since_epoch(); // 获取当前时间点
  int64_t newTime = std::chrono::duration<int64_t,std::nano>(d).count(); // 转换为纳秒级时间戳
  
  return (double)(newTime-initialTime) / 1000000000.0; // 计算时间差,转换为秒
}

int64_t ClockTimer::getPrecisionSystemTime()
{
  auto d = std::chrono::steady_clock::now().time_since_epoch();
  int64_t newTime = std::chrono::duration<int64_t,std::nano>(d).count();
  return newTime; // Unix使用纳秒级时间戳
}

#endif