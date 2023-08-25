#include "../core/commandloop.h"

//------------------------
#include "../core/using.h"
//------------------------

// 处理单行命令行输入
string CommandLoop::processSingleCommandLine(const string &s)
{
  // 1. 去除字符串两端空白
  string line = Global::trim(s);

  // 2. 过滤非ASCII可打印字符
  // 只保留ASCII码32-126之间的字符和制表符
  size_t newLen = 0;
  for (size_t i = 0; i < line.length(); i++)
    if (((int)line[i] >= 32 && (int)line[i] <= 126) || line[i] == '\t')
      line[newLen++] = line[i];

  line.erase(line.begin() + newLen, line.end());

  // 3. 去除注释
  size_t commentPos = line.find("#");
  if (commentPos != string::npos)
    line = line.substr(0, commentPos);

  // 4. 将制表符转换为空格
  for (size_t i = 0; i < line.length(); i++)
    if (line[i] == '\t')
      line[i] = ' ';

  // 5. 再次去除两端空白
  line = Global::trim(line);

  return line;
}