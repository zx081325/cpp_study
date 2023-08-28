#include "../core/mainargs.h"

#include "../core/os.h"

#ifdef OS_IS_WINDOWS
#include <codecvt>
#include <windows.h>
#include <processenv.h>
#include <shellapi.h>
#endif

std::vector<std::string> MainArgs::getCommandLineArgsUTF8(int argc, const char* const* argv) {
#ifdef OS_IS_WINDOWS
  // 如果是 Windows 环境
  // 完全忽略 argc 和 argv，直接调用 Windows 特定的函数来获取完整的命令行，以避免在非 ASCII 输入的情况下丢失信息。
  // 然后将其转换为 UTF8 编码
  LPWSTR commandLine = GetCommandLineW(); // 获取完整命令行
  LPWSTR* argvWide = CommandLineToArgvW(commandLine, &argc); // 转换为宽字符参数
  std::wstring_convert<std::codecvt_utf8<wchar_t>> converter; // 创建 UTF-8 转换器
  std::vector<std::string> args;
  args.reserve(argc);
  for (int i = 0; i < argc; i++)
    args.push_back(converter.to_bytes(argvWide[i])); // 将宽字符参数转换为 UTF-8 并存储
  return args; // 返回 UTF-8 编码的命令行参数列表
#else
  // 如果不是 Windows 环境，暂时假设使用 UTF-8 编码。
  // 如果需要为其他操作系统添加处理逻辑，可以在这里添加。
  std::vector<std::string> args;
  args.reserve(argc);
  for (int i = 0; i < argc; i++)
    args.push_back(std::string(argv[i])); // 直接使用命令行参数，假设是 UTF-8 编码
  return args; // 返回命令行参数列表
#endif
}

#ifdef OS_IS_WINDOWS

// Converts utf8 used in the rest of the program to windows wide char encoding
// just before output to console.
class ConsoleStreamBufWindows : public std::streambuf {
public:
  ConsoleStreamBufWindows(DWORD handleId);

protected:
  virtual int sync();
  virtual int_type overflow(int_type c = traits_type::eof());

private:
  const HANDLE handle;
  std::string buf;
  bool isConsole;
};

ConsoleStreamBufWindows::ConsoleStreamBufWindows(DWORD handleId)
  : handle(GetStdHandle(handleId)),
    buf()
{
  DWORD lpMode;
  // Check if we are a console or not so that we know whether to call
  // WriteConsole or not, as mentioned in
  // https://docs.microsoft.com/en-us/windows/console/writeconsole
  // Stdout and Stderr could be a file if redirected in the windows terminal.
  bool success = GetConsoleMode(handle, &lpMode);
  if(success)
    isConsole = true;
  else
    isConsole = false;
}

int ConsoleStreamBufWindows::sync()
{
  if(buf.empty())
    return 0;

  if(isConsole) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring toWrite = converter.from_bytes(buf);
    while(toWrite.size() > 0) {
      //DWORD is 32 bits, size_t is often larger.
      DWORD sizeToWrite = (DWORD)std::min((size_t)0xFFFFFFFFU,toWrite.size());

      DWORD writtenSize;
      bool success = WriteConsoleW(handle, toWrite.c_str(), sizeToWrite, &writtenSize, NULL);
      if(!success)
        break;

      if((size_t)writtenSize >= toWrite.size())
        break;
      toWrite = toWrite.substr((size_t)writtenSize);
    }
  }
  else {
    // Just write raw utf8 to file, no conversion.
    while(buf.size() > 0) {
      //DWORD is 32 bits, size_t is often larger.
      DWORD sizeToWrite = (DWORD)std::min((size_t)0xFFFFFFFFU,buf.size());

      DWORD writtenSize;
      bool success = WriteFile(handle, buf.c_str(), sizeToWrite, &writtenSize, NULL);
      if(!success)
        break;

      if((size_t)writtenSize >= buf.size())
        break;
      buf = buf.substr((size_t)writtenSize);
    }
  }

  buf.clear();
  return 0;
}

ConsoleStreamBufWindows::int_type ConsoleStreamBufWindows::overflow(int_type c)
{
  buf += traits_type::to_char_type(c);
  return traits_type::not_eof(c);
}

void MainArgs::makeCoutAndCerrAcceptUTF8() {
  if(GetFileType(GetStdHandle(STD_OUTPUT_HANDLE)) == FILE_TYPE_CHAR)
    std::cout.rdbuf(new ConsoleStreamBufWindows(STD_OUTPUT_HANDLE));
  if(GetFileType(GetStdHandle(STD_ERROR_HANDLE)) == FILE_TYPE_CHAR)
    std::cerr.rdbuf(new ConsoleStreamBufWindows(STD_ERROR_HANDLE));
}
#else
void MainArgs::makeCoutAndCerrAcceptUTF8() {
  // For non-windows, do nothing, assume they already are happy with utf8
}
#endif
