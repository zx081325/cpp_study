#include "../core/fileutils.h"

#include <fstream>
#include <iomanip>
#include <limits>
#include <zlib.h>
#include <ghc/filesystem.hpp>

#include "../core/global.h"
#include "../core/sha2.h"
#include "../core/test.h"

namespace gfs = ghc::filesystem;

//------------------------
#include "../core/using.h"
//------------------------

// 判断给定路径是否存在文件或目录
bool FileUtils::exists(const string &path)
{
  try
  {
    // 将路径字符串转换为 gfs::path 对象
    gfs::path gfsPath(gfs::u8path(path));
    // 判断 gfs::path 对象对应的文件或目录是否存在
    return gfs::exists(gfsPath);
  }
  catch (const gfs::filesystem_error &)
  {
    // 捕获 gfs::filesystem_error 异常，表示路径无效或出现错误
    return false;
  }
}
}

// 尝试以给定模式打开文件输入流，如果成功则返回 true，否则返回 false
bool FileUtils::tryOpen(std::ifstream &in, const char *filename, std::ios_base::openmode mode)
{
  in.open(gfs::u8path(filename), mode); // 使用 UTF-8 编码打开文件路径
  return in.good();                     // 返回文件流的状态
}

// 尝试以给定模式打开文件输出流，如果成功则返回 true，否则返回 false
bool FileUtils::tryOpen(std::ofstream &out, const char *filename, std::ios_base::openmode mode)
{
  out.open(gfs::u8path(filename), mode); // 使用 UTF-8 编码打开文件路径
  return out.good();                     // 返回文件流的状态
}

// 尝试以给定模式打开文件输入流，如果成功则返回 true，否则返回 false
bool FileUtils::tryOpen(std::ifstream &in, const std::string &filename, std::ios_base::openmode mode)
{
  return tryOpen(in, filename.c_str(), mode); // 调用前面的 char* 版本的函数
}

// 尝试以给定模式打开文件输出流，如果成功则返回 true，否则返回 false
bool FileUtils::tryOpen(std::ofstream &out, const std::string &filename, std::ios_base::openmode mode)
{
  return tryOpen(out, filename.c_str(), mode); // 调用前面的 char* 版本的函数
}

// 打开文件输入流，如果操作失败则抛出 IOError 异常
void FileUtils::open(std::ifstream &in, const char *filename, std::ios_base::openmode mode)
{
  in.open(gfs::u8path(filename), mode);                                                   // 使用 UTF-8 编码打开文件路径
  if (!in.good())                                                                         // 如果文件流状态不好
    throw IOError("无法打开文件 " + std::string(filename) + " - 文件不存在或权限无效？"); // 抛出异常
}

// 打开文件输出流，如果操作失败则抛出 IOError 异常
void FileUtils::open(std::ofstream &out, const char *filename, std::ios_base::openmode mode)
{
  out.open(gfs::u8path(filename), mode);                                                // 使用 UTF-8 编码打开文件路径
  if (!out.good())                                                                      // 如果文件流状态不好
    throw IOError("无法写入文件 " + std::string(filename) + " - 路径无效或权限无效？"); // 抛出异常
}

// 打开文件输入流，如果操作失败则抛出 IOError 异常
void FileUtils::open(std::ifstream &in, const std::string &filename, std::ios_base::openmode mode)
{
  open(in, filename.c_str(), mode); // 调用前面的 char* 版本的函数
}

// 打开文件输出流，如果操作失败则抛出 IOError 异常
void FileUtils::open(std::ofstream &out, const std::string &filename, std::ios_base::openmode mode)
{
  open(out, filename.c_str(), mode); // 调用前面的 char* 版本的函数
}

// 返回弱规范化后的路径（将路径转换为其绝对路径），如果操作失败则返回原路径
std::string FileUtils::weaklyCanonical(const std::string &path)
{
  gfs::path srcPath(gfs::u8path(path)); // 以 UTF-8 编码创建文件路径
  try
  {
    return gfs::weakly_canonical(srcPath).u8string(); // 尝试进行弱规范化并返回 UTF-8 编码的字符串
  }
  catch (const gfs::filesystem_error &) // 如果操作失败
  {
    return path; // 返回原路径
  }
}

// 检查给定路径是否为目录，如果操作失败则返回 false
bool FileUtils::isDirectory(const std::string &filename)
{
  gfs::path srcPath(gfs::u8path(filename)); // 以 UTF-8 编码创建文件路径
  try
  {
    return gfs::is_directory(srcPath); // 尝试判断路径是否为目录
  }
  catch (const gfs::filesystem_error &) // 如果操作失败
  {
    return false; // 返回 false
  }
}

// 尝试移除指定文件，如果操作失败则返回 false
bool FileUtils::tryRemoveFile(const std::string &filename)
{
  gfs::path srcPath(gfs::u8path(filename)); // 以 UTF-8 编码创建文件路径
  try
  {
    gfs::remove(srcPath); // 尝试移除文件
  }
  catch (const gfs::filesystem_error &) // 如果操作失败
  {
    return false; // 返回 false
  }
  return true; // 如果操作成功，则返回 true
}

// 尝试重命名文件或目录，如果操作失败则返回 false
bool FileUtils::tryRename(const std::string &src, const std::string &dst)
{
  gfs::path srcPath(gfs::u8path(src)); // 使用 UTF-8 编码创建源路径
  gfs::path dstPath(gfs::u8path(dst)); // 使用 UTF-8 编码创建目标路径
  try
  {
    gfs::rename(srcPath, dstPath); // 尝试重命名
  }
  catch (const gfs::filesystem_error &) // 如果操作失败
  {
    return false; // 返回 false
  }
  return true; // 如果操作成功，则返回 true
}

// 重命名文件或目录，如果操作失败则抛出 IOError 异常
void FileUtils::rename(const std::string &src, const std::string &dst)
{
  gfs::path srcPath(gfs::u8path(src)); // 使用 UTF-8 编码创建源路径
  gfs::path dstPath(gfs::u8path(dst)); // 使用 UTF-8 编码创建目标路径
  try
  {
    gfs::rename(srcPath, dstPath); // 尝试重命名
  }
  catch (const gfs::filesystem_error &e) // 如果操作失败
  {
    throw IOError("无法将 " + src + " 重命名为 " + dst + "，错误信息：" + e.what()); // 抛出异常
  }
}

// 从文件加载内容到字符串中，并进行 SHA-256 哈希校验，如果期望的哈希值为空或者实际哈希值缓冲不为空，则加载哈希值到缓冲中
void FileUtils::loadFileIntoString(const string &filename, const string &expectedSha256, string &str)
{
  loadFileIntoString(filename, expectedSha256, str, nullptr);
}

// 从文件加载内容到字符串中，并进行 SHA-256 哈希校验，如果期望的哈希值为空或者实际哈希值缓冲不为空，则加载哈希值到缓冲中
void FileUtils::loadFileIntoString(const string &filename, const string &expectedSha256, string &str, string *actualSha256Buf)
{
  ifstream in;                                                         // 创建文件输入流
  open(in, filename, std::ios::in | std::ios::binary | std::ios::ate); // 以二进制只读模式打开文件

  ifstream::pos_type fileSize = in.tellg(); // 获取文件大小
  if (fileSize < 0)
    throw StringError("tellg 无法确定文件大小");

  in.seekg(0, std::ios::beg); // 移动文件指针到文件开头
  str.resize(fileSize);       // 调整字符串大小以匹配文件大小
  in.read(&str[0], fileSize); // 读取文件内容到字符串
  in.close();                 // 关闭文件输入流

  if (expectedSha256 != "" || actualSha256Buf != nullptr) // 如果期望的哈希值不为空，或者实际哈希值缓冲不为空
  {
    char hashResultBuf[65];                                               // 存储哈希结果的缓冲区
    SHA2::get256((const uint8_t *)str.data(), str.size(), hashResultBuf); // 计算字符串内容的 SHA-256 哈希值
    string hashResult(hashResultBuf);                                     // 将哈希结果转换为字符串
    if (expectedSha256 != "")                                             // 如果期望的哈希值不为空
    {
      bool matching = Global::toLower(expectedSha256) == Global::toLower(hashResult); // 比较期望的哈希值和计算得到的哈希值是否相等
      if (!matching)
        throw StringError("文件 " + filename + " 的 sha256 哈希值为 " + hashResult + "，与期望的哈希值 " + expectedSha256 + " 不匹配");
    }
    if (actualSha256Buf != nullptr)  // 如果实际哈希值缓冲不为空
      *actualSha256Buf = hashResult; // 将计算得到的哈希值加载到缓冲中
  }
}

// 解压缩并加载文件内容到字符串中，并进行 SHA-256 哈希校验，如果期望的哈希值为空或者实际哈希值缓冲不为空，则加载哈希值到缓冲中
void FileUtils::uncompressAndLoadFileIntoString(const string &filename, const string &expectedSha256, string &uncompressed)
{
  uncompressAndLoadFileIntoString(filename, expectedSha256, uncompressed, nullptr);
}

// 解压缩并加载文件内容到字符串中，并进行 SHA-256 哈希校验，如果期望的哈希值为空或者实际哈希值缓冲不为空，则加载哈希值到缓冲中
void FileUtils::uncompressAndLoadFileIntoString(const string &filename, const string &expectedSha256, string &uncompressed, string *actualSha256Buf)
{
  std::unique_ptr<string> compressed = std::make_unique<string>();            // 创建字符串的智能指针
  loadFileIntoString(filename, expectedSha256, *compressed, actualSha256Buf); // 加载文件内容到 compressed

  static constexpr size_t CHUNK_SIZE = 262144; // 解压缩时使用的块大小

  int zret;
  z_stream zs;
  zs.zalloc = Z_NULL;
  zs.zfree = Z_NULL;
  zs.opaque = Z_NULL;
  zs.avail_in = 0;
  zs.next_in = Z_NULL;
  int windowBits = 15 + 32;             // 根据 zlib 文档，添加 32 启用 gzip 解码
  zret = inflateInit2(&zs, windowBits); // 初始化解压缩流
  if (zret != Z_OK)
  {
    (void)inflateEnd(&zs); // 清理解压缩流资源
    throw StringError("解压缩文件时出错。文件可能无效。文件：" + filename);
  }

  // zlib 只能处理最大为 unsigned int 的输入块，通常为 32 位或 4 GB。
  // 我们选择稍小于这个值的最大块大小。
  constexpr size_t INPUT_CHUNK_SIZE = 1073741824;
  testAssert(std::numeric_limits<unsigned int>::max() > INPUT_CHUNK_SIZE); // 断言确保 INPUT_CHUNK_SIZE 不超过 unsigned int 的最大值
  size_t totalSizeLeft = compressed->size();                               // 剩余待解压缩的数据大小
  size_t totalAmountOfOutputProduced = 0;                                  // 解压缩后的数据总大小
  zs.avail_in = 0;

  {
    size_t amountMoreInputToProvide = std::min(INPUT_CHUNK_SIZE - zs.avail_in, totalSizeLeft);
    zs.avail_in += (unsigned int)amountMoreInputToProvide;
    totalSizeLeft -= amountMoreInputToProvide;
  }

  zs.next_in = (Bytef *)(&(*compressed)[0]);
  while (true)
  {
    uncompressed.resize(totalAmountOfOutputProduced + CHUNK_SIZE); // 调整 uncompressed 大小以匹配解压缩后的数据
    zs.next_out = (Bytef *)(&uncompressed[totalAmountOfOutputProduced]);
    zs.avail_out = CHUNK_SIZE;

    zret = inflate(&zs, (totalSizeLeft > 0 ? Z_SYNC_FLUSH : Z_FINISH)); // 解压缩数据块

    assert(zret != Z_STREAM_ERROR); // 断言确保解压缩没有错误
    switch (zret)
    {
    case Z_NEED_DICT:
      (void)inflateEnd(&zs);
      throw StringError("解压缩文件时出错，Z_NEED_DICT。文件可能无效。文件：" + filename);
    case Z_DATA_ERROR:
      (void)inflateEnd(&zs);
      throw StringError("解压缩文件时出错，Z_DATA_ERROR。文件可能无效。文件：" + filename);
    case Z_MEM_ERROR:
      (void)inflateEnd(&zs);
      throw StringError("解压缩文件时出错，Z_MEM_ERROR。文件可能无效。文件：" + filename);
    default:
      break;
    }

    // 还有更多的输入需要消耗吗？
    if (totalSizeLeft > 0)
    {
      size_t amountMoreInputToProvide = std::min(INPUT_CHUNK_SIZE - zs.avail_in, totalSizeLeft);
      assert(amountMoreInputToProvide > 0);
      zs.avail_in += (unsigned int)amountMoreInputToProvide;
      totalSizeLeft -= amountMoreInputToProvide;
      assert(zs.avail_out < CHUNK_SIZE);
      size_t amountOfOutputProduced = CHUNK_SIZE - zs.avail_out;
      assert(amountOfOutputProduced > 0);
      totalAmountOfOutputProduced += amountOfOutputProduced;
      continue;
    }

    // 积累我们产生的输出，如果有的话。
    assert(zs.avail_out <= CHUNK_SIZE);
    size_t amountOfOutputProduced = CHUNK_SIZE - zs.avail_out;
    totalAmountOfOutputProduced += amountOfOutputProduced;

    // 没有输出空间了吗？我们肯定填满了 avail_out 中设置的整个 CHUNK_SIZE，所以再次循环以防还有更多数据。
    if (zs.avail_out == 0)
    {
      continue;
    }

    assert(zs.avail_out > 0);
    // 必须是我们已经完成了
    if (zret == Z_STREAM_END)
    {
      assert(zs.next_in == (Bytef *)(&(*compressed)[0]) + compressed->size());
      break;
    }
    // 否则，我们有问题了
    (void)inflateEnd(&zs); // 清理解压缩流资源
    throw StringError("解压缩文件时出错，到达了意外的输入结尾。文件：" + filename);
  }
  // 裁剪字符串，只保留所需的部分。
  assert(totalAmountOfOutputProduced <= uncompressed.size());
  uncompressed.resize(totalAmountOfOutputProduced);
  // 清理资源
  (void)inflateEnd(&zs);
}

// 从文件读取内容到字符串中
string FileUtils::readFile(const char *filename)
{
  ifstream ifs;
  open(ifs, filename);                                                                 // 打开文件流
  string str((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>()); // 读取文件内容到字符串
  return str;
}

// 从文件读取内容到字符串中
string FileUtils::readFile(const string &filename)
{
  return readFile(filename.c_str());
}

// 以二进制模式从文件读取内容到字符串中
string FileUtils::readFileBinary(const char *filename)
{
  ifstream ifs;
  open(ifs, filename, std::ios::binary);                                               // 以二进制模式打开文件流
  string str((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>()); // 读取文件内容到字符串
  return str;
}

// 以二进制模式从文件读取内容到字符串中
string FileUtils::readFileBinary(const string &filename)
{
  return readFileBinary(filename.c_str());
}

// 从文件按行读取内容到字符串向量中，使用指定的分隔字符
vector<string> FileUtils::readFileLines(const char *filename, char delimiter)
{
  ifstream ifs;
  open(ifs, filename); // 打开文件流

  vector<string> vec;
  string line;
  while (getline(ifs, line, delimiter)) // 逐行读取内容到向量中
    vec.push_back(line);
  return vec;
}

// 从文件按行读取内容到字符串向量中，使用指定的分隔字符
vector<string> FileUtils::readFileLines(const string &filename, char delimiter)
{
  return readFileLines(filename.c_str(), delimiter);
}

// 列出指定目录下的文件名
vector<string> FileUtils::listFiles(const std::string &dirname)
{
  vector<string> collected;
  try
  {
    for (const gfs::directory_entry &entry : gfs::directory_iterator(gfs::u8path(dirname))) // 遍历目录中的文件
    {
      const gfs::path &path = entry.path();
      string fileName = path.filename().u8string();
      collected.push_back(fileName); // 将文件名添加到向量中
    }
  }
  catch (const gfs::filesystem_error &e)
  {
    cerr << "列出文件时出错：" << e.what() << endl;
    throw StringError(string("列出文件时出错：") + e.what());
  }
  return collected;
}

// 递归地收集指定目录下符合条件的文件
void FileUtils::collectFiles(const string &dirname, std::function<bool(const string &)> fileFilter, vector<string> &collected)
{
  namespace gfs = ghc::filesystem; // 使用命名空间别名

  try
  {
    for (const gfs::directory_entry &entry : gfs::recursive_directory_iterator(gfs::u8path(dirname))) // 递归遍历目录中的文件
    {
      if (!gfs::is_directory(entry.status())) // 如果不是目录
      {
        const gfs::path &path = entry.path();
        string fileName = path.filename().u8string();
        if (fileFilter(fileName)) // 根据条件进行过滤
        {
          collected.push_back(path.u8string()); // 将路径添加到向量中
        }
      }
    }
  }
  catch (const gfs::filesystem_error &e)
  {
    cerr << "递归收集文件时出错：" << e.what() << endl;
    throw StringError(string("递归收集文件时出错：") + e.what());
  }
}