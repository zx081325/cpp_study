#include "../core/global.h"

/*
 * global.cpp
 * Author: David Wu
 */

#include <cctype>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <cinttypes>
#include <iomanip>
#include <sstream>

using namespace std;

// errors----------------------------------

// 致命错误处理函数,打印错误信息并退出程序
void Global::fatalError(const char *s)
{
  cout << "\n致命错误:\n"
       << s << endl;
  exit(EXIT_FAILURE);
}

void Global::fatalError(const string &s)
{
  cout << "\n致命错误:\n"
       << s << endl;
  exit(EXIT_FAILURE);
}

// STRINGS---------------------------------

// 将bool类型转换为字符串
string Global::boolToString(bool b)
{
  return b ? string("true") : string("false");
}

// 将char类型转换为字符串
string Global::charToString(char c)
{
  char buf[2];
  buf[0] = c;
  buf[1] = 0;
  return string(buf);
}

// 将int类型转换为字符串
string Global::intToString(int x)
{
  stringstream ss;
  ss << x;
  return ss.str();
}

// 将float类型转换为字符串
string Global::floatToString(float x)
{
  stringstream ss;
  ss << x;
  return ss.str();
}

// 将double类型转换为字符串
string Global::doubleToString(double x)
{
  stringstream ss;
  ss << x;
  return ss.str();
}

// 将double类型转换为高精度字符串
string Global::doubleToStringHighPrecision(double x)
{
  stringstream ss;
  ss.precision(17);
  ss << x;
  return ss.str();
}

// 将int64_t类型转换为字符串
string Global::int64ToString(int64_t x)
{
  stringstream ss;
  ss << x;
  return ss.str();
}

// 将uint32_t类型转换为字符串
string Global::uint32ToString(uint32_t x)
{
  stringstream ss;
  ss << x;
  return ss.str();
}

// 将uint64_t类型转换为字符串
string Global::uint64ToString(uint64_t x)
{
  stringstream ss;
  ss << x;
  return ss.str();
}

// 将uint32_t类型转换为16进制字符串
string Global::uint32ToHexString(uint32_t x)
{
  static const char *digits = "0123456789ABCDEF";
  size_t hex_len = sizeof(uint32_t) * 2;
  string s(hex_len, '0');
  for (size_t i = 0; i < hex_len; i++)
    s[hex_len - i - 1] = digits[(x >> (i * 4)) & 0x0f];
  return s;
}

// 将uint64_t类型转换为16进制字符串
string Global::uint64ToHexString(uint64_t x)
{
  static const char *digits = "0123456789ABCDEF";
  size_t hex_len = sizeof(uint64_t) * 2;
  string s(hex_len, '0');
  for (size_t i = 0; i < hex_len; i++)
    s[hex_len - i - 1] = digits[(x >> (i * 4)) & 0x0f];
  return s;
}

// 尝试将字符串转换为int类型
bool Global::tryStringToInt(const string &str, int &x)
{
  int val = 0;
  istringstream in(trim(str));
  in >> val;
  if (in.fail() || in.peek() != EOF)
    return false;
  x = val;
  return true;
}

// 将字符串强制转换为int类型,转换失败则抛出异常
int Global::stringToInt(const string &str)
{
  int val = 0;
  istringstream in(trim(str));
  in >> val;
  if (in.fail() || in.peek() != EOF)
    throw IOError(string("could not parse int: ") + str);
  return val;
}

// 尝试将字符串转换为int64_t类型
bool Global::tryStringToInt64(const string &str, int64_t &x)
{
  int64_t val = 0;
  istringstream in(trim(str));
  in >> val;
  if (in.fail() || in.peek() != EOF)
    return false;
  x = val;
  return true;
}

// 将字符串强制转换为int64_t类型,转换失败则抛出异常
int64_t Global::stringToInt64(const string &str)
{
  int64_t val = 0;
  istringstream in(trim(str));
  in >> val;
  if (in.fail() || in.peek() != EOF)
    throw IOError(string("could not parse int: ") + str);
  return val;
}

// 尝试将字符串转换为bool类型
bool Global::tryStringToBool(const string &str, bool &x)
{
  string s = toLower(trim(str));
  if (s == "false")
  {
    x = false;
    return true;
  }
  if (s == "true")
  {
    x = true;
    return true;
  }
  return false;
}

// 将字符串强制转换为bool类型,转换失败则抛出异常
bool Global::stringToBool(const string &str)
{
  string s = toLower(trim(str));
  if (s == "false")
    return false;
  if (s == "true")
    return true;

  throw IOError(string("could not parse bool: ") + str);
  return false;
}

// 尝试将字符串转换为uint64_t类型
bool Global::tryStringToUInt64(const string &str, uint64_t &x)
{
  uint64_t val = 0;
  string s = trim(str);
  if (s.size() > 0 && s[0] == '-')
    return false;
  istringstream in(s);
  in >> val;
  if (in.fail() || in.peek() != EOF)
    return false;
  x = val;
  return true;
}

// 尝试将16进制字符串转换为uint64_t类型
bool Global::tryHexStringToUInt64(const string &str, uint64_t &x)
{
  uint64_t val = 0;
  for (char c : str)
  {
    if (!(c >= '0' && c <= '9') &&
        !(c >= 'A' && c <= 'F') &&
        !(c >= 'a' && c <= 'f'))
    {
      return false;
    }
  }
  istringstream in(str);
  in >> std::hex >> val;
  if (in.fail() || in.peek() != EOF)
    return false;
  x = val;
  return true;
}

// 将字符串强制转换为uint64_t类型,转换失败则抛出异常
uint64_t Global::stringToUInt64(const string &str)
{
  uint64_t val;
  bool suc = tryStringToUInt64(str, val);
  if (!suc)
    throw IOError(string("could not parse uint64: ") + str);
  return val;
}

// 将16进制字符串强制转换为uint64_t类型,转换失败则抛出异常
uint64_t Global::hexStringToUInt64(const string &str)
{
  uint64_t val;
  bool suc = tryHexStringToUInt64(str, val);
  if (!suc)
    throw IOError(string("could not parse uint64 from hex: ") + str);
  return val;
}

// 尝试将字符串转换为float类型
bool Global::tryStringToFloat(const string &str, float &x)
{
  float val = 0;
  istringstream in(trim(str));
  in >> val;
  if (in.fail() || in.peek() != EOF)
    return false;
  x = val;
  return true;
}

// 将字符串强制转换为float类型,转换失败则抛出异常
float Global::stringToFloat(const string &str)
{
  float val = 0;
  istringstream in(trim(str));
  in >> val;
  if (in.fail() || in.peek() != EOF)
    throw IOError(string("could not parse float: ") + str);
  return val;
}

// 尝试将字符串转换为double类型
bool Global::tryStringToDouble(const string &str, double &x)
{
  double val = 0;
  istringstream in(trim(str));
  in >> val;
  if (in.fail() || in.peek() != EOF)
    return false;
  x = val;
  return true;
}

// 将字符串强制转换为double类型,转换失败则抛出异常
double Global::stringToDouble(const string &str)
{
  double val = 0;
  istringstream in(trim(str));
  in >> val;
  if (in.fail() || in.peek() != EOF)
    throw IOError(string("could not parse double: ") + str);
  return val;
}

// 判断字符是否为空白符
bool Global::isWhitespace(char c)
{
  return contains(" \t\r\n\v\f", c);
}

// 判断字符串是否全部为空白符
bool Global::isWhitespace(const string &s)
{
  size_t p = s.find_first_not_of(" \t\r\n\v\f");
  return p == string::npos;
}

// 判断字符串s是否以prefix为前缀
bool Global::isPrefix(const string &s, const string &prefix)
{
  if (s.length() < prefix.length())
    return false;
  int result = s.compare(0, prefix.length(), prefix);
  return result == 0;
}

// 判断字符串s是否以suffix为后缀
bool Global::isSuffix(const string &s, const string &suffix)
{
  if (s.length() < suffix.length())
    return false;
  int result = s.compare(s.length() - suffix.length(), suffix.length(), suffix);
  return result == 0;
}

// 去掉字符串s的前缀prefix并返回剩余部分
string Global::chopPrefix(const string &s, const string &prefix)
{
  if (!isPrefix(s, prefix))
    throw StringError("Global::chopPrefix: \n" + prefix + "\nis not a prefix of\n" + s);
  return s.substr(prefix.size());
}

// 去掉字符串s的后缀suffix并返回剩余部分
string Global::chopSuffix(const string &s, const string &suffix)
{
  if (!isSuffix(s, suffix))
    throw StringError("Global::chopSuffix: \n" + suffix + "\nis not a suffix of\n" + s);
  return s.substr(0, s.size() - suffix.size());
}

// 去除字符串两端的指定字符
string Global::trim(const std::string &s, const char *delims)
{
  size_t p2 = s.find_last_not_of(delims);
  if (p2 == string::npos)
    return string();
  size_t p1 = s.find_first_not_of(delims);
  if (p1 == string::npos)
    p1 = 0;

  return s.substr(p1, (p2 - p1) + 1);
}

// 将字符串按空格分割成vector
vector<string> Global::split(const string &s)
{
  istringstream in(s);
  string token;
  vector<string> tokens;
  while (in >> token)
  {
    token = Global::trim(token);
    tokens.push_back(token);
  }
  return tokens;
}

// 拼接字符串数组成新的字符串，使用delim作为分隔符。
string Global::concat(const char *const *strs, size_t len, const char *delim)
{
  size_t totalLen = 0;
  size_t delimLen = strlen(delim);
  for (size_t i = 0; i < len; i++)
  {
    if (i > 0)
      totalLen += delimLen;
    totalLen += strlen(strs[i]);
  }
  string s;
  s.reserve(totalLen);
  for (size_t i = 0; i < len; i++)
  {
    if (i > 0)
      s += delim;
    s += strs[i];
  }
  return s;
}

// 拼接字符串vector成新的字符串
string Global::concat(const vector<string> &strs, const char *delim)
{
  return concat(strs, delim, 0, strs.size());
}

// 拼接字符串vector的一部分成新的字符串
string Global::concat(const vector<string> &strs, const char *delim, size_t start, size_t end)
{
  size_t totalLen = 0;
  size_t delimLen = strlen(delim);
  for (size_t i = start; i < end; i++)
  {
    if (i > start)
      totalLen += delimLen;
    totalLen += strs[i].size();
  }
  string s;
  s.reserve(totalLen);
  for (size_t i = start; i < end; i++)
  {
    if (i > start)
      s += delim;
    s += strs[i];
  }
  return s;
}

// 拼接字符串set成新的字符串
string Global::concat(const set<string> &strs, const char *delim)
{
  vector<string> v;
  std::copy(strs.begin(), strs.end(), std::back_inserter(v));
  return concat(v, delim, 0, v.size());
}

// 按分隔符delim分割字符串
vector<string> Global::split(const string &s, char delim)
{
  istringstream in(s);
  string token;
  vector<string> tokens;
  while (getline(in, token, delim))
    tokens.push_back(token);
  return tokens;
}

// 字符串转大写
string Global::toUpper(const string &s)
{
  string t = s;
  size_t len = t.length();
  for (size_t i = 0; i < len; i++)
    t[i] = toupper(t[i]);
  return t;
}

// 字符串转小写
string Global::toLower(const string &s)
{
  string t = s;
  size_t len = t.length();
  for (size_t i = 0; i < len; i++)
    t[i] = tolower(t[i]);
  return t;
}

// 比较两个字符串,忽略大小写
bool Global::isEqualCaseInsensitive(const string &s0, const string &s1)
{
  size_t len = s0.length();
  if (len != s1.length())
    return false;
  for (size_t i = 0; i < len; i++)
    if (tolower(s0[i]) != tolower(s1[i]))
      return false;
  return true;
}

// 格式化字符串
static string vformat(const char *fmt, va_list ap)
{
  size_t size = 4096;
  char stackbuf[4096];
  std::vector<char> dynamicbuf;
  char *buf = &stackbuf[0];

  int needed;
  while (true)
  {
    needed = vsnprintf(buf, size, fmt, ap);

    if (needed <= (int)size && needed >= 0)
      break;

    size = (needed > 0) ? (needed + 1) : (size * 2);
    dynamicbuf.resize(size + 1);
    buf = &dynamicbuf[0];
  }
  return std::string(buf, (size_t)needed);
}

// 格式化字符串
string Global::strprintf(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  std::string buf = vformat(fmt, ap);
  va_end(ap);
  return buf;
}

// 判断字符是否是数字
bool Global::isDigit(char c)
{
  return c >= '0' && c <= '9';
}

// 判断字符是否是字母
bool Global::isAlpha(char c)
{
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

// 判断字符串是否全部是数字
bool Global::isDigits(const string &str)
{
  return isDigits(str, 0, str.size());
}

// 判断字符串的一部分是否全部是数字
bool Global::isDigits(const string &str, size_t start, size_t end)
{
  if (end <= start)
    return false;
  if (end - start > 9)
    return false;

  size_t size = str.size();
  int64_t value = 0;
  for (size_t i = start; i < end && i < size; i++)
  {
    char c = str[i];
    if (!isDigit(c))
      return false;
    value = value * 10 + (c - '0');
  }

  if ((value & 0x7FFFFFFFLL) != value)
    return false;

  return true;
}

// 解析字符串中的数字部分
int Global::parseDigits(const string &str)
{
  return parseDigits(str, 0, str.size());
}

// 解析字符串一部分中的数字
int Global::parseDigits(const string &str, size_t start, size_t end)
{
  if (end <= start)
    throw IOError("Could not parse digits, end <= start, or empty string");
  if (end - start > 9)
    throw IOError("Could not parse digits, overflow: " + str.substr(start, end - start));

  size_t size = str.size();
  int64_t value = 0;
  for (size_t i = start; i < end && i < size; i++)
  {
    char c = str[i];
    if (!isDigit(c))
      return 0;
    value = value * 10 + (c - '0');
  }

  if ((value & 0x7FFFFFFFLL) != value)
    throw IOError("Could not parse digits, overflow: " + str.substr(start, end - start));

  return (int)value;
}

// 判断字符串中是否包含某个字符
bool contains(const char *str, char c)
{
  return strchr(str, c) != NULL;
}

// 判断字符串中是否包含某个字符
bool contains(const string &str, char c)
{
  return strchr(str.c_str(), c) != NULL;
}

// 判断vector中是否包含某个元素
bool contains(const vector<string> &vec, const char *elt)
{
  for (const string &x : vec)
    if (x == elt)
      return true;
  return false;
}

// 判断set中是否包含某个元素
bool contains(const set<string> &set, const char *elt)
{
  return set.find(elt) != set.end();
}

// 在vector中查找元素的索引
size_t indexOf(const vector<string> &vec, const char *elt)
{
  size_t size = vec.size();
  for (size_t i = 0; i < size; i++)
    if (vec[i] == elt)
      return i;
  return string::npos;
}

// 判断字符串中字符是否全部在允许字符集内
bool Global::stringCharsAllAllowed(const string &str, const char *allowedChars)
{
  for (size_t i = 0; i < str.size(); i++)
  {
    if (!contains(allowedChars, str[i]))
      return false;
  }
  return true;
}

// 解析键值对字符串
map<string, string> Global::readKeyValues(const string &contents)
{
  istringstream lineIn(contents);
  string line;
  map<string, string> keyValues;
  while (getline(lineIn, line))
  {
    if (line.length() <= 0)
      continue;
    istringstream commaIn(line);
    string commaChunk;
    while (getline(commaIn, commaChunk, ','))
    {
      if (commaChunk.length() <= 0)
        continue;
      size_t equalsPos = commaChunk.find_first_of('=');
      if (equalsPos == string::npos)
        continue;
      string leftChunk = Global::trim(commaChunk.substr(0, equalsPos));
      string rightChunk = Global::trim(commaChunk.substr(equalsPos + 1));
      if (leftChunk.length() == 0)
        throw IOError("readKeyValues: key value pair without key: " + line);
      if (rightChunk.length() == 0)
        throw IOError("readKeyValues: key value pair without value: " + line);
      if (keyValues.find(leftChunk) != keyValues.end())
        throw IOError("readKeyValues: duplicate key: " + leftChunk);
      keyValues[leftChunk] = rightChunk;
    }
  }
  return keyValues;
}

// 去掉字符串里的注释
string Global::stripComments(const string &str)
{
  if (str.find_first_of('#') == string::npos)
    return str;

  // Turn str into a stream so we can go line by line
  istringstream in(str);
  string line;
  string result;

  while (getline(in, line))
  {
    size_t pos = line.find_first_of('#');
    if (pos != string::npos)
      result += line.substr(0, pos);
    else
      result += line;
    result += "\n";
  }
  return result;
}

// 解析内存大小字符串
uint64_t Global::readMem(const string &str)
{
  if (str.size() < 2)
    throw IOError("Global::readMem: Could not parse amount of memory: " + str);

  size_t end = str.size() - 1;
  size_t snd = str.size() - 2;

  string numericPart;
  int shiftFactor;
  if (str.find_first_of("K") == end)
  {
    shiftFactor = 10;
    numericPart = str.substr(0, end);
  }
  else if (str.find_first_of("KB") == snd)
  {
    shiftFactor = 10;
    numericPart = str.substr(0, snd);
  }
  else if (str.find_first_of("M") == end)
  {
    shiftFactor = 20;
    numericPart = str.substr(0, end);
  }
  else if (str.find_first_of("MB") == snd)
  {
    shiftFactor = 20;
    numericPart = str.substr(0, snd);
  }
  else if (str.find_first_of("G") == end)
  {
    shiftFactor = 30;
    numericPart = str.substr(0, end);
  }
  else if (str.find_first_of("GB") == snd)
  {
    shiftFactor = 30;
    numericPart = str.substr(0, snd);
  }
  else if (str.find_first_of("T") == end)
  {
    shiftFactor = 40;
    numericPart = str.substr(0, end);
  }
  else if (str.find_first_of("TB") == snd)
  {
    shiftFactor = 40;
    numericPart = str.substr(0, snd);
  }
  else if (str.find_first_of("P") == end)
  {
    shiftFactor = 50;
    numericPart = str.substr(0, end);
  }
  else if (str.find_first_of("PB") == snd)
  {
    shiftFactor = 50;
    numericPart = str.substr(0, snd);
  }
  else if (str.find_first_of("B") == end)
  {
    shiftFactor = 0;
    numericPart = str.substr(0, end);
  }
  else
  {
    shiftFactor = 0;
    numericPart = str;
  }

  if (!isDigits(numericPart))
    throw IOError("Global::readMem: Could not parse amount of memory: " + str);
  uint64_t mem = 0;
  istringstream in(numericPart);
  in >> mem;
  if (in.bad())
    throw IOError("Global::readMem: Could not parse amount of memory: " + str);

  for (int i = 0; i < shiftFactor; i++)
  {
    uint64_t newMem = mem << 1;
    if (newMem < mem)
      throw IOError("Global::readMem: Could not parse amount of memory (too large): " + str);
    mem = newMem;
  }
  return mem;
}

// 解析内存大小字符串
uint64_t Global::readMem(const char *str)
{
  return readMem(string(str));
}

// 等待任意键继续
void Global::pauseForKey()
{
  cout << "Press any key to continue..." << endl;
  cin.get();
}

// 四舍五入到指定精度
double Global::roundStatic(double x, double inverseScale)
{
  return round(x * inverseScale) / inverseScale;
}

// 动态精度四舍五入
double Global::roundDynamic(double x, int precision)
{
  double absx = std::fabs(x);
  if (absx <= 1e-60)
    return x;
  int orderOfMagnitude = (int)floor(log10(absx));
  int roundingMagnitude = orderOfMagnitude - precision;
  if (roundingMagnitude >= 0)
    return round(x);
  double inverseScale = pow(10.0, -roundingMagnitude);
  return roundStatic(x, inverseScale);
}
