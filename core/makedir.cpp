#include "../core/makedir.h"
#include "../core/os.h"

#ifdef OS_IS_WINDOWS
#include <windows.h>
#endif
#ifdef OS_IS_UNIX_OR_APPLE
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <cerrno>
#include <ghc/filesystem.hpp>

namespace gfs = ghc::filesystem;

//------------------------
#include "../core/using.h"
//------------------------

// WINDOWS IMPLMENTATIION-------------------------------------------------------------

#ifdef OS_IS_WINDOWS

// 创建目录
void MakeDir::make(const string &path)
{
  gfs::path gfsPath(gfs::u8path(path));          // 将路径转换为文件系统路径
  std::error_code ec;                            // 错误码用于接收创建目录操作的错误信息
  bool suc = gfs::create_directory(gfsPath, ec); // 尝试创建目录，并获取操作结果
  if (!suc && ec)                                // 如果创建目录失败且出现错误
  {
    if (ec == std::errc::file_exists)                     // 如果错误是目录已存在
      return;                                             // 目录已存在，无需操作，直接返回
    throw StringError("创建目录时出错：" + ec.message()); // 否则抛出错误信息
  }
}

#endif

// UNIX IMPLEMENTATION------------------------------------------------------------------

#ifdef OS_IS_UNIX_OR_APPLE

// 创建目录
void MakeDir::make(const string &path)
{
  // 调用 mkdir 函数尝试创建目录，设置所需的权限
  int result = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  if (result != 0) // 如果创建目录失败
  {
    if (errno == EEXIST) // 如果错误是目录已存在
      return; // 目录已存在，无需操作，直接返回
    throw StringError("创建目录时出错：" + path); // 否则抛出错误信息
  }
}

#endif
