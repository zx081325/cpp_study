#include "../core/md5.h"

/*
 * md5.cpp
 * Implementation from wikipedia
 *
 * Author: David Wu
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

//32位数循环左移实现函数
#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))

// 注意：所有变量都是无符号的 32 位，在计算时会在模 2^32 下进行运算
// r 指定了每轮的位移量
static const uint32_t r[] =
    {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
     5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
     4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
     6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

static const uint32_t k[] = {
 0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
 0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
 0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
 0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
 0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

void MD5::get(const char *initial_msg, size_t initial_len, uint32_t hash[4])
{
  return get((const uint8_t *)initial_msg, initial_len, hash);
}

void MD5::get(const uint8_t *initial_msg, size_t initial_len, uint32_t hash[4])
{
  // 使用正弦整数部分的二进制表示（弧度制）作为常数
  // 初始化变量:
  uint32_t h0, h1, h2, h3;
  h0 = 0x67452301;
  h1 = 0xefcdab89;
  h2 = 0x98badcfe;
  h3 = 0x10325476;

  // 预处理：添加一个单独的 1 位
  // 在消息末尾添加 "1" 位
  /* 注意：输入字节被视为位字符串，其中第一位是字节的最高位。[37] */

  // 预处理：用零填充
  // 在消息末尾添加 "0" 位，直到消息长度（以位计）= 448（mod 512）
  // 将长度 mod（2 的 64 次幂）附加到消息

  size_t new_len;
  for (new_len = initial_len * 8 + 1; new_len % 512 != 448; new_len++)
    ;
  new_len /= 8;

  // （额外分配了 128 个字节，只作为安全缓冲区）
  uint8_t *msg = (uint8_t *)calloc(new_len + 128, 1); // calloc会初始化为0
  memcpy(msg, initial_msg, initial_len);
  msg[initial_len] = 128; // 写入 "1" 位

  // 在缓冲区末尾附加位长度
  uint64_t bits_len = 8 * (uint64_t)initial_len;
  uint32_t lower_bits_len = (uint32_t)(bits_len);
  uint32_t upper_bits_len = (uint32_t)(bits_len >> 32);
  memcpy(msg + new_len, &lower_bits_len, 4);
  memcpy(msg + new_len + 4, &upper_bits_len, 4);

  // 对消息进行连续的 512 位块处理：
  // 对于每个 512 位消息块：
  size_t offset;
  for (offset = 0; offset < new_len; offset += (512 / 8))
  {
    // 将块分成十六个 32 位的单词 w[j]，0 ≤ j ≤ 15
    uint32_t *w = (uint32_t *)(msg + offset);

    // 初始化此块的哈希值：
    uint32_t a = h0;
    uint32_t b = h1;
    uint32_t c = h2;
    uint32_t d = h3;

    // 主循环：
    uint32_t i;
    for (i = 0; i < 64; i++)
    {
      uint32_t f, g;

      if (i < 16)
      {
        f = (b & c) | ((~b) & d);
        g = i;
      }
      else if (i < 32)
      {
        f = (d & b) | ((~d) & c);
        g = (5 * i + 1) % 16;
      }
      else if (i < 48)
      {
        f = b ^ c ^ d;
        g = (3 * i + 5) % 16;
      }
      else
      {
        f = c ^ (b | (~d));
        g = (7 * i) % 16;
      }

      uint32_t temp = d;
      d = c;
      c = b;
      b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
      a = temp;
    }

    // 将此块的哈希添加到结果中：
    h0 += a;
    h1 += b;
    h2 += c;
    h3 += d;
  }

  // 清理内存
  free(msg);

  hash[0] = h0;
  hash[1] = h1;
  hash[2] = h2;
  hash[3] = h3;
}
