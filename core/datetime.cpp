#include "../core/datetime.h"

#include <chrono>
#include <iomanip>
#include <sstream>

#include "../core/os.h"
#include "../core/multithread.h"

// 获取当前时间戳
time_t DateTime::getNow() {
  time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  return time; 
}

// 将时间戳转换为UTC时间
std::tm DateTime::gmTime(time_t time) {
  std::tm buf{};

#if defined(OS_IS_UNIX_OR_APPLE)
  gmtime_r(&time, &buf);
#elif defined(OS_IS_WINDOWS)
  gmtime_s(&buf, &time);  
#else
  // 其他平台使用互斥锁保证线程安全
  static std::mutex gmTimeMutex;
  std::lock_guard<std::mutex> lock(gmTimeMutex);
  buf = *(std::gmtime(&time));
#endif

  return buf;
}

// 将时间戳转换为本地时间
std::tm DateTime::localTime(time_t time) {
  std::tm buf{};

#if defined(OS_IS_UNIX_OR_APPLE)
  localtime_r(&time, &buf);
#elif defined(OS_IS_WINDOWS)
  localtime_s(&buf, &time);
#else
  // 其他平台使用互斥锁保证线程安全
  static std::mutex localTimeMutex;
  std::lock_guard<std::mutex> lock(localTimeMutex);
  buf = *(std::localtime(&time));  
#endif

  return buf;
}

// 获取日期字符串,格式YYYY-MM-DD
std::string DateTime::getDateString() {
  time_t time = getNow();
  std::tm ptm = gmTime(time);
  std::ostringstream out;
  out << (ptm.tm_year+1900) << "-"
      << (ptm.tm_mon+1) << "-"
      << (ptm.tm_mday);
  return out.str();
}

// 获取紧凑格式的日期时间字符串,格式YYYYMMDD-HHMMSS
std::string DateTime::getCompactDateTimeString() {
  time_t time = getNow();
  std::tm tm = localTime(time);
  std::ostringstream out;
  out << std::put_time(&tm, "%Y%m%d-%H%M%S"); 
  return out.str();
}

// 将时间写入流中
void DateTime::writeTimeToStream(std::ostream& out, const char* fmt, time_t time) {
  std::tm tm = localTime(time);
  out << std::put_time(&tm, fmt);
}