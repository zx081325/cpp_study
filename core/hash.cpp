#include "../core/hash.h"

/*
 * hash.cpp
 * Author: David Wu
 */

using namespace std;

// BITS-----------------------------------

uint32_t Hash::highBits(uint64_t x)
{
  return (uint32_t)((x >> 32) & 0xFFFFFFFFU); // 返回高32位
}

uint32_t Hash::lowBits(uint64_t x)
{
  return (uint32_t)(x & 0xFFFFFFFFU); // 返回低32位
}

uint64_t Hash::combine(uint32_t hi, uint32_t lo)
{
  return ((uint64_t)hi << 32) | (uint64_t)lo; // 合并为64位
}

// 一个简单的64位线性同余混合
uint64_t Hash::basicLCong(uint64_t x)
{
  return 2862933555777941757ULL * x + 3037000493ULL;
}

uint64_t Hash::basicLCong2(uint64_t x)
{
  return 6364136223846793005ULL * x + 1442695040888963407ULL;
}

// MurmurHash3 结尾混合 - 好的雪崩效应
// 可逆,但映射 0 -> 0
uint64_t Hash::murmurMix(uint64_t x)
{
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdULL;
  x ^= x >> 33;
  x *= 0xc4ceb9fe1a85ec53ULL;
  x ^= x >> 33;
  return x;
}

// Splitmix64 mixing step
uint64_t Hash::splitMix64(uint64_t x)
{
  x = x + 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

// 循环右移r位
static uint64_t rotateRight(uint64_t x, int r)
{
  return (x >> r) | (x << (64 - r));
}

// algorithm from Pelle Evensen https://mostlymangling.blogspot.com/
uint64_t Hash::rrmxmx(uint64_t x)
{
  // 异或和循环右移操作
  x ^= rotateRight(x, 49) ^ rotateRight(x, 24);

  // 乘法操作
  x *= 0x9fb21c651e98df25ULL;
  
  // 右移操作和异或操作
  x ^= x >> 28;

  // 再次乘法操作
  x *= 0x9fb21c651e98df25ULL;
  
  // 右移操作和异或操作
  return x ^ (x >> 28);
}



// algorithm from Pelle Evensen https://mostlymangling.blogspot.com/
uint64_t Hash::nasam(uint64_t x)
{
  // 异或和循环右移操作
  x ^= rotateRight(x, 25) ^ rotateRight(x, 47);
  
  // 乘法和异或操作
  x *= 0x9e6c63d0676a9a99ULL;
  x ^= (x >> 23) ^ (x >> 51);
  
  // 乘法和异或操作
  x *= 0x9e6d62d06f6a9a9bULL;
  x ^= (x >> 23) ^ (x >> 51);
  
  return x; // 返回最终的哈希值
}

// Splitmix64 混合步骤
uint32_t Hash::jenkinsMixSingle(uint32_t a, uint32_t b, uint32_t c)
{
  a -= b; a -= c; a ^= (c >> 13);
  b -= c; b -= a; b ^= (a <<  8);
  c -= a; c -= b; c ^= (b >> 13);
  a -= b; a -= c; a ^= (c >> 12);
  b -= c; b -= a; b ^= (a << 16);
  c -= a; c -= b; c ^= (b >>  5);
  a -= b; a -= c; a ^= (c >>  3);
  b -= c; b -= a; b ^= (a << 10);
  c -= a; c -= b; c ^= (b >> 15);
  return c;
}

void Hash::jenkinsMix(uint32_t &a, uint32_t &b, uint32_t &c)
{
  a -= b; a -= c; a ^= (c >> 13);
  b -= c; b -= a; b ^= (a <<  8);
  c -= a; c -= b; c ^= (b >> 13);
  a -= b; a -= c; a ^= (c >> 12);
  b -= c; b -= a; b ^= (a << 16);
  c -= a; c -= b; c ^= (b >>  5);
  a -= b; a -= c; a ^= (c >>  3);
  b -= c; b -= a; b ^= (a << 10);
  c -= a; c -= b; c ^= (b >> 15);
}

// 对char数组hash
uint64_t Hash::simpleHash(const char *str)
{
  uint64_t m1 = 123456789;
  uint64_t m2 = 314159265;
  uint64_t m3 = 958473711;
  while (*str != '\0')
  {
    char c = *str;
    m1 = m1 * 31 + (uint64_t)c;
    m2 = m2 * 317 + (uint64_t)c;
    m3 = m3 * 1609 + (uint64_t)c;
    str++;
  }
  uint32_t lo = jenkinsMixSingle(lowBits(m1), highBits(m2), lowBits(m3));
  uint32_t hi = jenkinsMixSingle(highBits(m1), lowBits(m2), highBits(m3));
  return combine(hi, lo);
}

// 对int数组hash
uint64_t Hash::simpleHash(const int *input, int len)
{
  uint64_t m1 = 123456789;
  uint64_t m2 = 314159265;
  uint64_t m3 = 958473711;
  uint32_t m4 = 0xCAFEBABEU;
  for (int i = 0; i < len; i++)
  {
    int c = input[i];
    m1 = m1 * 31 + (uint64_t)c;
    m2 = m2 * 317 + (uint64_t)c;
    m3 = m3 * 1609 + (uint64_t)c;
    m4 += (uint32_t)c;
    m4 += (m4 << 10);
    m4 ^= (m4 >> 6);
  }
  m4 += (m4 << 3);
  m4 ^= (m4 >> 11);
  m4 += (m4 << 15);

  uint32_t lo = jenkinsMixSingle(lowBits(m1), highBits(m2), lowBits(m3));
  uint32_t hi = jenkinsMixSingle(highBits(m1), lowBits(m2), highBits(m3));
  return combine(hi ^ m4, lo + m4);
}

// Hash128------------------------------------------------------------------------

// 重载运算符输出hash128
ostream &operator<<(ostream &out, const Hash128 other)
{
  out << Global::uint64ToHexString(other.hash1)
      << Global::uint64ToHexString(other.hash0);
  return out;
}

// 128位数字转为32位hex字符串
string Hash128::toString() const
{
  return Global::uint64ToHexString(hash1) + Global::uint64ToHexString(hash0);
}

// 32位hex字符串转128位数字
Hash128 Hash128::ofString(const string &s)
{
  if (s.size() != 32)
    throw IOError("Could not parse as Hash128: " + s);
  for (char c : s)
  {
    if (!(c >= '0' && c <= '9') &&
        !(c >= 'A' && c <= 'F') &&
        !(c >= 'a' && c <= 'f'))
    {
      throw IOError("Could not parse as Hash128: " + s);
    }
  }
  uint64_t h1 = Global::hexStringToUInt64(s.substr(0, 16));
  uint64_t h0 = Global::hexStringToUInt64(s.substr(16, 16));
  return Hash128(h0, h1);
}
