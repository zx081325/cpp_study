#include "../core/config_parser.h"

#include "../core/fileutils.h"

#include <cmath>
#include <fstream>
#include <sstream>

using namespace std;

ConfigParser::ConfigParser(bool keysOverride, bool keysOverrideFromIncludes_)
    : initialized(false), fileName(), contents(), keyValues(),
      keysOverrideEnabled(keysOverride), keysOverrideFromIncludes(keysOverrideFromIncludes_),
      curLineNum(0), curFilename(), includedFiles(), baseDirs(), logMessages(),
      usedKeysMutex(), usedKeys()
{
}

ConfigParser::ConfigParser(const string &fname, bool keysOverride, bool keysOverrideFromIncludes_)
    : ConfigParser(keysOverride, keysOverrideFromIncludes_)
{
  initialize(fname);
}

ConfigParser::ConfigParser(const char *fname, bool keysOverride, bool keysOverrideFromIncludes_)
    : ConfigParser(std::string(fname), keysOverride, keysOverrideFromIncludes_)
{
}

ConfigParser::ConfigParser(istream &in, bool keysOverride, bool keysOverrideFromIncludes_)
    : ConfigParser(keysOverride, keysOverrideFromIncludes_)
{
  initialize(in);
}

ConfigParser::ConfigParser(const map<string, string> &kvs)
    : ConfigParser(false, true)
{
  initialize(kvs);
}

ConfigParser::ConfigParser(const ConfigParser &source)
{
  if (!source.initialized)
    throw StringError("Can only copy a ConfigParser which has been initialized.");
  std::lock_guard<std::mutex> lock(source.usedKeysMutex);
  initialized = source.initialized;
  fileName = source.fileName;
  baseDirs = source.baseDirs;
  contents = source.contents;
  keyValues = source.keyValues;
  keysOverrideEnabled = source.keysOverrideEnabled;
  keysOverrideFromIncludes = source.keysOverrideFromIncludes;
  usedKeys = source.usedKeys;
}

void ConfigParser::initialize(const string &fname)
{
  // 如果已经初始化，则抛出错误
  if (initialized)
    throw StringError("ConfigParser已经初始化，无法再次初始化");

  // 打开文件流以读取配置文件内容
  ifstream in;
  FileUtils::open(in, fname);

  // 设置当前文件名为传递进来的文件名
  fileName = fname;

  // 提取基础目录并将其添加到基础目录列表中
  string baseDir = extractBaseDir(fname);
  if (!baseDir.empty())
    baseDirs.push_back(baseDir);

  // 调用内部初始化函数以解析配置文件内容
  initializeInternal(in);

  // 标记配置解析器为已初始化
  initialized = true;
}

// 初始化函数，从流中初始化配置解析器
void ConfigParser::initialize(istream &in)
{
  if (initialized)
    throw StringError("ConfigParser已经初始化，无法再次初始化");
  initializeInternal(in); // 调用内部初始化函数
  initialized = true;
}

// 初始化函数，使用键值对初始化配置解析器
void ConfigParser::initialize(const map<string, string> &kvs)
{
  if (initialized)
    throw StringError("ConfigParser已经初始化，无法再次初始化");
  keyValues = kvs; // 保存键值对
  initialized = true;
}

// 内部初始化函数，从流中初始化配置解析器
void ConfigParser::initializeInternal(istream &in)
{
  keyValues.clear();      // 清空键值对
  contents.clear();       // 清空内容
  curFilename = fileName; // 设置当前文件名为配置文件名
  readStreamContent(in);  // 读取流中的内容
}

// 处理被包含的文件
void ConfigParser::processIncludedFile(const std::string &fname)
{
  // 检查循环包含和重复包含
  if (fname == fileName || find(includedFiles.begin(), includedFiles.end(), fname) != includedFiles.end())
  {
    throw ConfigParsingError("循环或重复包含同一文件: '" + fname + "'" + lineAndFileInfo());
  }
  includedFiles.push_back(fname); // 将文件名加入已包含列表
  curFilename = fname;            // 更新当前文件名为正在处理的文件名

  string fpath;
  for (const std::string &p : baseDirs)
  {
    fpath += p; // 构建文件路径
  }
  fpath += fname;

  string baseDir = extractBaseDir(fname); // 提取基础目录路径
  if (!baseDir.empty())
  {
    if (baseDir[0] == '\\' || baseDir[0] == '/')
      throw ConfigParsingError("包含的文件中不支持绝对路径");
    baseDirs.push_back(baseDir); // 将基础目录路径加入列表
  }

  ifstream in;
  FileUtils::open(in, fpath); // 打开文件流
  readStreamContent(in);      // 读取文件内容

  if (!baseDir.empty())
    baseDirs.pop_back(); // 弹出基础目录路径
}

// 解析键值对
bool ConfigParser::parseKeyValue(const std::string &trimmedLine, std::string &key, std::string &value)
{
  key.clear();   // 清空键
  value.clear(); // 清空值

  // 解析键
  bool foundAnyKey = false;
  size_t i = 0;
  for (; i < trimmedLine.size(); i++)
  {
    char c = trimmedLine[i];
    if (Global::isAlpha(c) || Global::isDigit(c) || c == '_' || c == '-')
    {
      key += c;           // 将字符添加到键中
      foundAnyKey = true; // 表示已找到键的一部分
      continue;
    }
    else if (c == '#')
    {
      if (foundAnyKey)
        throw ConfigParsingError("无法解析键值对" + lineAndFileInfo());
      return false;
    }
    else if (Global::isWhitespace(c) || c == '=')
    {
      break;
    }
    else
    {
      throw ConfigParsingError("无法解析键值对" + lineAndFileInfo());
    }
  }

  // 跳过键后的空白字符
  for (; i < trimmedLine.size(); i++)
  {
    char c = trimmedLine[i];
    if (Global::isWhitespace(c))
      continue;
    else if (c == '#')
    {
      if (foundAnyKey)
        throw ConfigParsingError("无法解析键值对" + lineAndFileInfo());
      return false;
    }
    else if (c == '=')
    {
      break;
    }
    else
    {
      throw ConfigParsingError("无法解析键值对" + lineAndFileInfo());
    }
  }

  // 跳过等号
  bool foundEquals = false;
  if (i < trimmedLine.size())
  {
    assert(trimmedLine[i] == '=');
    foundEquals = true;
    i++;
  }

  // 跳过等号后的空白字符
  for (; i < trimmedLine.size(); i++)
  {
    char c = trimmedLine[i];
    if (Global::isWhitespace(c))
      continue;
    else if (c == '#')
    {
      break;
    }
    else
    {
      break;
    }
  }

  // 解析值
  bool foundAnyValue = false;
  for (; i < trimmedLine.size(); i++)
  {
    char c = trimmedLine[i];
    value += c;           // 将字符添加到值中
    foundAnyValue = true; // 表示已找到值的一部分
  }

  value = Global::trim(value); // 去除值首尾的空白字符

  if (foundAnyKey != foundAnyValue) // 如果找到的键和值数量不一致，抛出错误
    throw ConfigParsingError("无法解析键值对" + lineAndFileInfo());

  return foundAnyKey; // 返回是否找到键
}

// 读取输入流内容
void ConfigParser::readStreamContent(istream &in)
{
  curLineNum = 0;              // 初始化当前行号为0
  string line;                 // 用于存储每行内容的字符串
  ostringstream contentStream; // 用于构建整个内容的字符串流
  set<string> curFileKeys;     // 存储当前文件中已经出现的键的集合

  while (getline(in, line))
  {                                // 逐行读取输入流内容
    contentStream << line << "\n"; // 将当前行添加到内容字符串流中
    curLineNum += 1;               // 增加当前行号

    line = Global::trim(line); // 去除行首尾的空白字符
    if (line.length() <= 0 || line[0] == '#')
      continue; // 如果行为空或以'#'开头，跳过处理

    if (line[0] == '@')
    { // 处理以'@'开头的指令
      size_t commentPos = line.find("#");
      if (commentPos != string::npos)
        line = line.substr(0, commentPos); // 去除注释部分

      if (line.size() < 9)
      {
        throw ConfigParsingError("不支持的 @ 指令" + lineAndFileInfo());
      }

      size_t pos0 = line.find_first_of(" \t\v\f=");
      if (pos0 == string::npos)
        throw ConfigParsingError("@ 指令没有值（找不到键值分隔符）" + lineAndFileInfo());

      string key = Global::trim(line.substr(0, pos0));
      if (key != "@include")
        throw ConfigParsingError("不支持的 @ 指令 '" + key + "'" + lineAndFileInfo());

      string value = line.substr(pos0 + 1);
      size_t pos1 = value.find_first_not_of(" \t\v\f=");
      if (pos1 == string::npos)
        throw ConfigParsingError("@ 指令没有值（找不到键值分隔符后的值）" + lineAndFileInfo());

      value = Global::trim(value.substr(pos1));
      value = Global::trim(value, "'");  // 去除单引号作为文件名的标识
      value = Global::trim(value, "\""); // 去除双引号作为文件名的标识

      int lineNum = curLineNum;
      processIncludedFile(value); // 处理包含的文件
      curLineNum = lineNum;       // 恢复行号
      continue;
    }

    string key;
    string value;
    bool foundKeyValue = parseKeyValue(line, key, value); // 解析键值对
    if (!foundKeyValue)
      continue; // 如果解析失败，跳过处理

    if (curFileKeys.find(key) != curFileKeys.end())
    {
      if (!keysOverrideEnabled)
        throw ConfigParsingError("键 '" + key + "' 在 " +
                                 curFilename + " 中出现多次，可能是误操作，请删除其中一个");
      else
        logMessages.push_back("键 '" + key + "' 被新值 '" + value + "' 覆盖" + lineAndFileInfo());
    }

    if (keyValues.find(key) != keyValues.end())
    {
      if (!keysOverrideFromIncludes)
        throw ConfigParsingError("键 '" + key + "' 在 " +
                                 curFilename + " 或其包含的文件中出现多次，且键的覆盖被禁用");
      else
        logMessages.push_back("键 '" + key + "' 被新值 '" + value + "' 覆盖" + lineAndFileInfo());
    }

    keyValues[key] = value;  // 存储键值对
    curFileKeys.insert(key); // 将键添加到当前文件的键集合中
  }
  contents += contentStream.str(); // 将解析的内容添加到总内容中
}

// 返回包含当前行号和文件信息的字符串
string ConfigParser::lineAndFileInfo() const
{
  return ", line " + Global::intToString(curLineNum) + " in '" + curFilename + "'";
}

// 提取文件的基本目录路径
string ConfigParser::extractBaseDir(const std::string &fname)
{
  size_t slash = fname.find_last_of("/\\"); // 查找最后一个斜杠或反斜杠的位置
  if (slash != string::npos)
  {
    return fname.substr(0, slash + 1); // 返回基本目录路径
  }
  else
  {
    return std::string(); // 如果没有斜杠或反斜杠，则返回空字符串
  }
}

ConfigParser::~ConfigParser()
{
}

// 获取配置文件的文件名
string ConfigParser::getFileName() const
{
  return fileName;
}

// 获取配置文件的内容
string ConfigParser::getContents() const
{
  return contents;
}

// 获取所有键值对的字符串表示
string ConfigParser::getAllKeyVals() const
{
  ostringstream ost;
  for (auto it = keyValues.begin(); it != keyValues.end(); ++it)
  {
    ost << it->first + " = " + it->second << endl;
  }
  return ost.str();
}

// 标记指定的键未使用
void ConfigParser::unsetUsedKey(const string &key)
{
  std::lock_guard<std::mutex> lock(usedKeysMutex);
  usedKeys.erase(key);
}

// 将一个键的值应用到另一个键
void ConfigParser::applyAlias(const string &mapThisKey, const string &toThisKey)
{
  if (contains(mapThisKey) && contains(toThisKey))
    throw IOError("Cannot specify both " + mapThisKey + " and " + toThisKey + " in the same config");
  if (contains(mapThisKey))
  {
    keyValues[toThisKey] = keyValues[mapThisKey]; // 使用mapThisKey的值覆盖toThisKey的值
    keyValues.erase(mapThisKey);                  // 删除mapThisKey的键值对
    std::lock_guard<std::mutex> lock(usedKeysMutex);
    if (usedKeys.find(mapThisKey) != usedKeys.end())
    {
      usedKeys.insert(toThisKey); // 更新已使用的键集合
      usedKeys.erase(mapThisKey);
    }
  }
}

// 覆盖指定键的值
void ConfigParser::overrideKey(const std::string &key, const std::string &value)
{
  // 假设零长度的值意味着要删除一个键
  if (value.length() <= 0)
  {
    if (keyValues.find(key) != keyValues.end())
      keyValues.erase(key); // 删除指定的键
  }
  else
  {
    keyValues[key] = value; // 使用新的值来覆盖键值对
  }
}

// 使用新的配置文件来覆盖键值对
void ConfigParser::overrideKeys(const std::string &fname)
{
  // 新的配置文件，因此baseDir不再相关
  baseDirs.clear();

  // 处理包含的配置文件
  processIncludedFile(fname);
}

// 使用新的键值对映射来覆盖键值对
void ConfigParser::overrideKeys(const map<string, string> &newkvs)
{
  for (auto iter = newkvs.begin(); iter != newkvs.end(); ++iter)
  {
    // 假设零长度的值意味着要删除一个键
    if (iter->second.length() <= 0)
    {
      if (keyValues.find(iter->first) != keyValues.end())
        keyValues.erase(iter->first); // 删除指定的键
    }
    else
    {
      keyValues[iter->first] = iter->second; // 使用新的键值对进行覆盖
    }
  }
  fileName += " and/or command-line and query overrides"; // 更新文件名以反映覆盖操作
}

// 使用新的键值对映射来覆盖键值对，并考虑互斥的键集
void ConfigParser::overrideKeys(const map<string, string> &newkvs, const vector<pair<set<string>, set<string>>> &mutexKeySets)
{
  for (size_t i = 0; i < mutexKeySets.size(); i++)
  {
    const set<string> &a = mutexKeySets[i].first;
    const set<string> &b = mutexKeySets[i].second;
    bool hasA = false;

    // 检查是否有a中的键存在于新的键值对中
    for (auto iter = a.begin(); iter != a.end(); ++iter)
    {
      if (newkvs.find(*iter) != newkvs.end())
      {
        hasA = true;
        break;
      }
    }

    bool hasB = false;

    // 检查是否有b中的键存在于新的键值对中
    for (auto iter = b.begin(); iter != b.end(); ++iter)
    {
      if (newkvs.find(*iter) != newkvs.end())
      {
        hasB = true;
        break;
      }
    }

    // 如果有a中的键存在于新的键值对中，删除b中的键
    if (hasA)
    {
      for (auto iter = b.begin(); iter != b.end(); ++iter)
        keyValues.erase(*iter);
    }

    // 如果有b中的键存在于新的键值对中，删除a中的键
    if (hasB)
    {
      for (auto iter = a.begin(); iter != a.end(); ++iter)
        keyValues.erase(*iter);
    }
  }

  // 调用上面的函数来使用新的键值对映射来覆盖键值对
  overrideKeys(newkvs);
}

// 解析逗号分隔的键值对字符串，并返回一个键值对的映射
map<string, string> ConfigParser::parseCommaSeparated(const string &commaSeparatedValues)
{
  map<string, string> keyValues;                                    // 存储键值对映射
  vector<string> pieces = Global::split(commaSeparatedValues, ','); // 将字符串以逗号分隔为多个部分

  for (size_t i = 0; i < pieces.size(); i++)
  {
    string s = Global::trim(pieces[i]); // 去除部分的前后空格

    // 如果修剪后的部分长度为0，则跳过
    if (s.length() <= 0)
      continue;

    size_t pos = s.find("="); // 查找键值对中的等号位置

    if (pos == string::npos)
      throw ConfigParsingError("无法解析键值对，无法在 " + s + " 中找到 '='");

    string key = Global::trim(s.substr(0, pos));    // 获取键部分并去除前后空格
    string value = Global::trim(s.substr(pos + 1)); // 获取值部分并去除前后空格
    keyValues[key] = value;                         // 将键值对添加到映射中
  }

  return keyValues; // 返回解析后的键值对映射
}

// 标记具有指定前缀的所有键为已使用
void ConfigParser::markAllKeysUsedWithPrefix(const string &prefix)
{
  std::lock_guard<std::mutex> lock(usedKeysMutex); // 锁定互斥量以确保线程安全

  // 遍历配置中的键值对
  for (auto iter = keyValues.begin(); iter != keyValues.end(); ++iter)
  {
    const string &key = iter->first;

    // 如果键具有指定前缀，将其标记为已使用
    if (Global::isPrefix(key, prefix))
      usedKeys.insert(key);
  }
}

// 发出未使用的键警告，将警告信息写入输出流和日志记录器（如果提供）
void ConfigParser::warnUnusedKeys(ostream &out, Logger *logger) const
{
  vector<string> unused = unusedKeys(); // 获取未使用的键列表
  vector<string> messages;              // 存储警告消息

  if (unused.size() > 0)
  {
    messages.push_back("--------------");
    messages.push_back("警告：配置中有未使用的键！您可能有拼写错误，指定的选项未从 " + fileName + " 使用");
  }

  for (size_t i = 0; i < unused.size(); i++)
  {
    messages.push_back("警告：未使用的键 '" + unused[i] + "' 在 " + fileName);
  }

  if (unused.size() > 0)
  {
    messages.push_back("--------------");
  }

  // 如果提供了日志记录器，将警告消息写入日志
  if (logger)
  {
    for (size_t i = 0; i < messages.size(); i++)
      logger->write(messages[i]);
  }

  // 将警告消息写入输出流
  for (size_t i = 0; i < messages.size(); i++)
    out << messages[i] << endl;
}

// 获取未使用的键列表
vector<string> ConfigParser::unusedKeys() const
{
  std::lock_guard<std::mutex> lock(usedKeysMutex); // 锁定互斥量以确保线程安全

  vector<string> unused; // 存储未使用的键

  // 遍历配置中的键值对
  for (auto iter = keyValues.begin(); iter != keyValues.end(); ++iter)
  {
    const string &key = iter->first;

    // 如果键不在usedKeys集合中，表示未使用，将其添加到未使用列表中
    if (usedKeys.find(key) == usedKeys.end())
      unused.push_back(key);
  }

  return unused; // 返回未使用的键列表
}

// 判断配置中是否包含指定键
bool ConfigParser::contains(const string &key) const
{
  return keyValues.find(key) != keyValues.end(); // 查找键是否在keyValues中，返回对应结果
}

// 判断配置中是否包含给定的任一键
bool ConfigParser::containsAny(const std::vector<std::string> &possibleKeys) const
{
  for (const string &key : possibleKeys)
  {
    if (contains(key))
      return true; // 只要有一个键在配置中，就返回true
  }
  return false; // 遍历完仍然没有找到任何键，返回false
}

// 获取首个在配置中找到的键，如果没有则抛出错误
std::string ConfigParser::firstFoundOrFail(const std::vector<std::string> &possibleKeys) const
{
  for (const string &key : possibleKeys)
  {
    if (contains(key))
      return key; // 返回第一个找到的键
  }

  // 构建错误信息
  string message = "无法在配置文件 " + fileName + " 中找到键";
  for (const string &key : possibleKeys)
  {
    message += " '" + key + "'";
  }
  throw IOError(message); // 抛出错误
}

// 获取首个在配置中找到的键，如果没有则返回空字符串
std::string ConfigParser::firstFoundOrEmpty(const std::vector<std::string> &possibleKeys) const
{
  for (const string &key : possibleKeys)
  {
    if (contains(key))
      return key; // 返回第一个找到的键
  }
  return string(); // 没有找到任何键，返回空字符串
}

// 从配置中获取字符串值
string ConfigParser::getString(const string &key)
{
  auto iter = keyValues.find(key);

  // 如果找不到指定键，抛出错误
  if (iter == keyValues.end())
    throw IOError("无法在配置文件 " + fileName + " 中找到键 '" + key + "'");

  {
    std::lock_guard<std::mutex> lock(usedKeysMutex);
    usedKeys.insert(key); // 将使用过的键添加到集合中，确保线程安全
  }

  return iter->second; // 返回获取到的字符串值
}

// 从配置中获取字符串值，并检查是否在给定的可能值集合中
string ConfigParser::getString(const string &key, const set<string> &possibles)
{
  // 获取指定键的字符串值
  string value = getString(key);

  // 如果字符串值不在可能值集合中
  if (possibles.find(value) == possibles.end())
    throw IOError("键 '" + key + "' 在配置文件 " + fileName + " 中必须是 (" + Global::concat(possibles, "|") + ") 中的一个");

  return value; // 返回获取到的字符串值
}

// 从配置中获取一组以逗号分隔的字符串值
vector<string> ConfigParser::getStrings(const string &key)
{
  // 获取指定键的字符串值，并通过逗号分隔为多个字符串
  return Global::split(getString(key), ',');
}

// 从配置中获取一组以逗号分隔的非空、修剪后的字符串值
vector<string> ConfigParser::getStringsNonEmptyTrim(const string &key)
{
  vector<string> raw = Global::split(getString(key), ','); // 获取指定键的字符串值列表并以逗号分隔
  vector<string> trimmed;                                  // 存储修剪后的字符串结果

  for (size_t i = 0; i < raw.size(); i++)
  {
    string s = Global::trim(raw[i]); // 修剪字符串值

    // 如果修剪后的字符串长度不为0，则添加到结果向量中
    if (s.length() > 0)
      trimmed.push_back(s);
  }
  return trimmed; // 返回修剪后的字符串结果向量
}

// 从配置中获取一组字符串值，并检查是否在给定的可能值集合中
vector<string> ConfigParser::getStrings(const string &key, const set<string> &possibles)
{
  vector<string> values = getStrings(key); // 获取指定键的字符串值列表

  for (size_t i = 0; i < values.size(); i++)
  {
    const string &value = values[i]; // 获取当前值的引用

    // 如果当前值不在可能值集合中
    if (possibles.find(value) == possibles.end())
      throw IOError("键 '" + key + "' 在配置文件 " + fileName + " 中必须是 (" + Global::concat(possibles, "|") + ") 中的一个");
  }
  return values; // 返回获取到的字符串值列表
}

// 从配置中获取布尔值
bool ConfigParser::getBool(const string &key)
{
  // 获取指定键的字符串值
  string value = getString(key);

  bool x;
  // 尝试将字符串转换为布尔值
  if (!Global::tryStringToBool(value, x))
    throw IOError("无法将'" + value + "'解析为布尔值，用于配置键'" + key + "'，位于配置文件 " + fileName);

  return x; // 返回获取到的布尔值
}

// 从配置中获取一组布尔值
vector<bool> ConfigParser::getBools(const string &key)
{
  // 获取指定键的字符串值列表
  vector<string> values = getStrings(key);

  vector<bool> ret; // 存储转换后的布尔值结果
  for (size_t i = 0; i < values.size(); i++)
  {
    const string &value = values[i]; // 获取当前值的引用

    bool x;
    // 尝试将字符串转换为布尔值
    if (!Global::tryStringToBool(value, x))
      throw IOError("无法将'" + value + "'解析为布尔值，用于配置键'" + key + "'，位于配置文件 " + fileName);

    ret.push_back(x); // 将转换后的值添加到结果向量中
  }
  return ret; // 返回布尔值结果向量
}

// 从配置中获取启用状态
enabled_t ConfigParser::getEnabled(const string &key)
{
  // 获取指定键的字符串值，并将其转换为小写并去除首尾空格
  string value = Global::trim(Global::toLower(getString(key)));

  enabled_t x;
  // 尝试将字符串解析为启用状态（bool 或 auto）
  if (!enabled_t::tryParse(value, x))
    throw IOError("无法将'" + value + "'解析为布尔值或自动状态，用于配置键'" + key + "'，位于配置文件 " + fileName);

  return x; // 返回获取到的启用状态
}

// 从配置中获取整数
int ConfigParser::getInt(const string &key)
{
  // 获取指定键的字符串值
  string value = getString(key);

  int x;
  // 尝试将字符串转换为整数
  if (!Global::tryStringToInt(value, x))
    throw IOError("无法将'" + value + "'解析为整数，用于配置键'" + key + "'，位于配置文件 " + fileName);

  return x; // 返回获取到的整数
}

// 从配置中获取整数，满足特定范围条件
int ConfigParser::getInt(const string &key, int min, int max)
{
  assert(min <= max); // 断言，确保最小值小于等于最大值

  // 获取指定键的字符串值
  string value = getString(key);

  int x;
  // 尝试将字符串转换为整数
  if (!Global::tryStringToInt(value, x))
    throw IOError("无法将'" + value + "'解析为整数，用于配置键'" + key + "'，位于配置文件 " + fileName);

  // 检查是否在指定范围内
  if (x < min || x > max)
    throw IOError("键'" + key + "'必须在范围 " + Global::intToString(min) + " 到 " + Global::intToString(max) + " 内，位于配置文件 " + fileName);

  return x; // 返回获取到的整数
}

// 从配置中获取一组整数
vector<int> ConfigParser::getInts(const string &key)
{
  // 获取指定键的字符串值列表
  vector<string> values = getStrings(key);

  vector<int> ret; // 存储转换后的整数结果
  for (size_t i = 0; i < values.size(); i++)
  {
    const string &value = values[i]; // 获取当前值的引用
    int x;

    // 尝试将字符串转换为整数
    if (!Global::tryStringToInt(value, x))
      throw IOError("无法将'" + value + "'解析为整数，用于配置键'" + key + "'，位于配置文件 " + fileName);

    ret.push_back(x); // 将转换后的值添加到结果向量中
  }
  return ret; // 返回整数结果向量
}

// 从配置中获取一组整数，满足特定范围条件
vector<int> ConfigParser::getInts(const string &key, int min, int max)
{
  // 获取指定键的字符串值列表
  vector<string> values = getStrings(key);

  vector<int> ret; // 存储转换后的整数结果
  for (size_t i = 0; i < values.size(); i++)
  {
    const string &value = values[i]; // 获取当前值的引用
    int x;

    // 尝试将字符串转换为整数
    if (!Global::tryStringToInt(value, x))
      throw IOError("无法将'" + value + "'解析为整数，用于配置键'" + key + "'，位于配置文件 " + fileName);

    // 检查是否在指定范围内
    if (x < min || x > max)
      throw IOError("键'" + key + "'必须在范围 " + Global::intToString(min) + " 到 " + Global::intToString(max) + " 内，位于配置文件 " + fileName);

    ret.push_back(x); // 将转换后的值添加到结果向量中
  }
  return ret; // 返回整数结果向量
}

// 从配置中获取一组非负整数对，每对整数满足特定范围条件，通过短划线分隔
vector<std::pair<int, int>> ConfigParser::getNonNegativeIntDashedPairs(const string &key, int min, int max)
{
  std::vector<string> pairStrs = getStrings(key); // 获取指定键的字符串值列表
  std::vector<std::pair<int, int>> ret;           // 存储转换后的非负整数对结果

  for (const string &pairStr : pairStrs)
  {
    if (Global::trim(pairStr).size() <= 0)
      continue; // 跳过空字符串

    std::vector<string> pieces = Global::split(Global::trim(pairStr), '-'); // 用短划线分隔字符串
    if (pieces.size() != 2)
    {
      throw IOError("无法将'" + pairStr + "'解析为由短划线分隔的整数对，用于配置键'" + key + "'，位于配置文件 " + fileName);
    }

    bool suc;
    int p0;
    int p1;
    suc = Global::tryStringToInt(pieces[0], p0);
    if (!suc)
      throw IOError("无法将'" + pairStr + "'解析为由短划线分隔的整数对，用于配置键'" + key + "'，位于配置文件 " + fileName);
    suc = Global::tryStringToInt(pieces[1], p1);
    if (!suc)
      throw IOError("无法将'" + pairStr + "'解析为由短划线分隔的整数对，用于配置键'" + key + "'，位于配置文件 " + fileName);

    if (p0 < min || p0 > max || p1 < min || p1 > max)
      throw IOError("期望键'" + key + "'的所有值在范围 " + Global::intToString(min) + " 到 " + Global::intToString(max) + " 内，位于配置文件 " + fileName);

    ret.push_back(std::make_pair(p0, p1)); // 将转换后的值添加到结果向量中
  }
  return ret; // 返回非负整数对结果向量
}

// 从配置中获取64位有符号整数
int64_t ConfigParser::getInt64(const string &key)
{
  // 获取指定键的字符串值
  string value = getString(key);

  int64_t x;
  // 尝试将字符串转换为64位有符号整数
  if (!Global::tryStringToInt64(value, x))
    throw IOError("无法将'" + value + "'解析为64位有符号整数，用于配置键'" + key + "'，位于配置文件 " + fileName);

  return x; // 返回获取到的64位有符号整数
}

// 从配置中获取64位有符号整数，满足特定范围条件
int64_t ConfigParser::getInt64(const string &key, int64_t min, int64_t max)
{
  assert(min <= max); // 断言，确保最小值小于等于最大值

  // 获取指定键的字符串值
  string value = getString(key);

  int64_t x;
  // 尝试将字符串转换为64位有符号整数
  if (!Global::tryStringToInt64(value, x))
    throw IOError("无法将'" + value + "'解析为64位有符号整数，用于配置键'" + key + "'，位于配置文件 " + fileName);

  // 检查是否在指定范围内
  if (x < min || x > max)
    throw IOError("键'" + key + "'必须在范围 " + Global::int64ToString(min) + " 到 " + Global::int64ToString(max) + " 内，位于配置文件 " + fileName);

  return x; // 返回获取到的64位有符号整数
}

// 从配置中获取一组64位有符号整数
vector<int64_t> ConfigParser::getInt64s(const string &key)
{
  // 获取指定键的字符串值列表
  vector<string> values = getStrings(key);

  vector<int64_t> ret; // 存储转换后的64位有符号整数结果
  for (size_t i = 0; i < values.size(); i++)
  {
    const string &value = values[i]; // 获取当前值的引用
    int64_t x;

    // 尝试将字符串转换为64位有符号整数
    if (!Global::tryStringToInt64(value, x))
      throw IOError("无法将'" + value + "'解析为64位有符号整数，用于配置键'" + key + "'，位于配置文件 " + fileName);

    ret.push_back(x); // 将转换后的值添加到结果向量中
  }
  return ret; // 返回64位有符号整数结果向量
}

// 从配置中获取一组64位有符号整数，满足特定范围条件
vector<int64_t> ConfigParser::getInt64s(const string &key, int64_t min, int64_t max)
{
  // 获取指定键的字符串值列表
  vector<string> values = getStrings(key);

  vector<int64_t> ret; // 存储转换后的64位有符号整数结果
  for (size_t i = 0; i < values.size(); i++)
  {
    const string &value = values[i]; // 获取当前值的引用
    int64_t x;

    // 尝试将字符串转换为64位有符号整数
    if (!Global::tryStringToInt64(value, x))
      throw IOError("无法将'" + value + "'解析为64位有符号整数，用于配置键'" + key + "'，位于配置文件 " + fileName);

    // 检查是否在指定范围内
    if (x < min || x > max)
      throw IOError("键'" + key + "'必须在范围 " + Global::int64ToString(min) + " 到 " + Global::int64ToString(max) + " 内，位于配置文件 " + fileName);

    ret.push_back(x); // 将转换后的值添加到结果向量中
  }
  return ret; // 返回64位有符号整数结果向量
}

// 从配置中获取64位无符号整数
uint64_t ConfigParser::getUInt64(const string &key)
{
  // 获取指定键的字符串值
  string value = getString(key);

  uint64_t x;
  // 尝试将字符串转换为64位无符号整数
  if (!Global::tryStringToUInt64(value, x))
    throw IOError("无法将'" + value + "'解析为64位无符号整数，用于配置键'" + key + "'，位于配置文件 " + fileName);

  return x; // 返回获取到的64位无符号整数
}

// 从配置中获取64位无符号整数，满足特定范围条件
uint64_t ConfigParser::getUInt64(const string &key, uint64_t min, uint64_t max)
{
  assert(min <= max); // 断言，确保最小值小于等于最大值

  // 获取指定键的字符串值
  string value = getString(key);

  uint64_t x;
  // 尝试将字符串转换为64位无符号整数
  if (!Global::tryStringToUInt64(value, x))
    throw IOError("无法将'" + value + "'解析为64位无符号整数，用于配置键'" + key + "'，位于配置文件 " + fileName);

  // 检查是否在指定范围内
  if (x < min || x > max)
    throw IOError("键'" + key + "'必须在范围 " + Global::uint64ToString(min) + " 到 " + Global::uint64ToString(max) + " 内，位于配置文件 " + fileName);

  return x; // 返回获取到的64位无符号整数
}

// 从配置中获取一组64位无符号整数
vector<uint64_t> ConfigParser::getUInt64s(const string &key)
{
  // 获取指定键的字符串值列表
  vector<string> values = getStrings(key);

  vector<uint64_t> ret; // 存储转换后的64位无符号整数结果
  for (size_t i = 0; i < values.size(); i++)
  {
    const string &value = values[i]; // 获取当前值的引用
    uint64_t x;

    // 尝试将字符串转换为64位无符号整数
    if (!Global::tryStringToUInt64(value, x))
      throw IOError("无法将'" + value + "'解析为64位无符号整数，用于配置键'" + key + "'，位于配置文件 " + fileName);

    ret.push_back(x); // 将转换后的值添加到结果向量中
  }
  return ret; // 返回64位无符号整数结果向量
}

// 从配置中获取一组64位无符号整数，满足特定范围条件
vector<uint64_t> ConfigParser::getUInt64s(const string &key, uint64_t min, uint64_t max)
{
  // 获取指定键的字符串值列表
  vector<string> values = getStrings(key);

  vector<uint64_t> ret; // 存储转换后的64位无符号整数结果
  for (size_t i = 0; i < values.size(); i++)
  {
    const string &value = values[i]; // 获取当前值的引用
    uint64_t x;

    // 尝试将字符串转换为64位无符号整数
    if (!Global::tryStringToUInt64(value, x))
      throw IOError("无法将'" + value + "'解析为64位无符号整数，用于配置键'" + key + "'，位于配置文件 " + fileName);

    // 检查是否在指定范围内
    if (x < min || x > max)
      throw IOError("键'" + key + "'必须在范围 " + Global::uint64ToString(min) + " 到 " + Global::uint64ToString(max) + " 内，位于配置文件 " + fileName);

    ret.push_back(x); // 将转换后的值添加到结果向量中
  }
  return ret; // 返回64位无符号整数结果向量
}

// 从配置中获取单精度浮点数
float ConfigParser::getFloat(const string &key)
{
  // 获取指定键的字符串值
  string value = getString(key);

  float x;
  // 尝试将字符串转换为单精度浮点数
  if (!Global::tryStringToFloat(value, x))
    throw IOError("无法将'" + value + "'解析为单精度浮点数，用于配置键'" + key + "'，位于配置文件 " + fileName);

  return x; // 返回获取到的单精度浮点数
}

// 从配置中获取单精度浮点数，满足特定范围条件
float ConfigParser::getFloat(const string &key, float min, float max)
{
  assert(min <= max); // 断言，确保最小值小于等于最大值

  // 获取指定键的字符串值
  string value = getString(key);

  float x;
  // 尝试将字符串转换为单精度浮点数
  if (!Global::tryStringToFloat(value, x))
    throw IOError("无法将'" + value + "'解析为单精度浮点数，用于配置键'" + key + "'，位于配置文件 " + fileName);

  // 检查是否为 NaN
  if (isnan(x))
    throw IOError("键'" + key + "'在配置文件 " + fileName + " 中是 NaN（非数值）");

  // 检查是否在指定范围内
  if (x < min || x > max)
    throw IOError("键'" + key + "'必须在范围 " + Global::floatToString(min) + " 到 " + Global::floatToString(max) + " 内，位于配置文件 " + fileName);

  return x; // 返回获取到的单精度浮点数
}

// 从配置中获取一组单精度浮点数
vector<float> ConfigParser::getFloats(const string &key)
{
  // 获取指定键的字符串值列表
  vector<string> values = getStrings(key);

  vector<float> ret; // 存储转换后的单精度浮点数结果
  for (size_t i = 0; i < values.size(); i++)
  {
    const string &value = values[i]; // 获取当前值的引用
    float x;

    // 尝试将字符串转换为单精度浮点数
    if (!Global::tryStringToFloat(value, x))
      throw IOError("无法将'" + value + "'解析为单精度浮点数，用于配置键'" + key + "'，位于配置文件 " + fileName);

    ret.push_back(x); // 将转换后的值添加到结果向量中
  }
  return ret; // 返回单精度浮点数结果向量
}

// 从配置中获取一组单精度浮点数，满足特定范围条件
vector<float> ConfigParser::getFloats(const string &key, float min, float max)
{
  // 获取指定键的字符串值列表
  vector<string> values = getStrings(key);

  vector<float> ret; // 存储转换后的单精度浮点数结果
  for (size_t i = 0; i < values.size(); i++)
  {
    const string &value = values[i]; // 获取当前值的引用
    float x;

    // 尝试将字符串转换为单精度浮点数
    if (!Global::tryStringToFloat(value, x))
      throw IOError("无法将'" + value + "'解析为单精度浮点数，用于配置键'" + key + "'，位于配置文件 " + fileName);

    // 检查是否为 NaN
    if (isnan(x))
      throw IOError("键'" + key + "'在配置文件 " + fileName + " 中是 NaN（非数值）");

    // 检查是否在指定范围内
    if (x < min || x > max)
      throw IOError("键'" + key + "'必须在范围 " + Global::floatToString(min) + " 到 " + Global::floatToString(max) + " 内，位于配置文件 " + fileName);

    ret.push_back(x); // 将转换后的值添加到结果向量中
  }
  return ret; // 返回单精度浮点数结果向量
}

// 从配置中获取双精度浮点数
double ConfigParser::getDouble(const string &key)
{
  // 获取指定键的字符串值
  string value = getString(key);

  double x;
  // 尝试将字符串转换为双精度浮点数
  if (!Global::tryStringToDouble(value, x))
    throw IOError("无法将'" + value + "'解析为双精度浮点数，用于配置键'" + key + "'，位于配置文件 " + fileName);

  return x; // 返回获取到的双精度浮点数
}

// 从配置中获取双精度浮点数，满足特定范围条件
double ConfigParser::getDouble(const string &key, double min, double max)
{
  assert(min <= max); // 断言，确保最小值小于等于最大值

  // 获取指定键的字符串值
  string value = getString(key);

  double x;
  // 尝试将字符串转换为双精度浮点数
  if (!Global::tryStringToDouble(value, x))
    throw IOError("无法将'" + value + "'解析为双精度浮点数，用于配置键'" + key + "'，位于配置文件 " + fileName);

  // 检查是否为 NaN
  if (isnan(x))
    throw IOError("键'" + key + "'在配置文件 " + fileName + " 中是 NaN（非数值）");

  // 检查是否在指定范围内
  if (x < min || x > max)
    throw IOError("键'" + key + "'必须在范围 " + Global::doubleToString(min) + " 到 " + Global::doubleToString(max) + " 内，位于配置文件 " + fileName);

  return x; // 返回获取到的双精度浮点数
}

// 从配置中获取一组双精度浮点数
vector<double> ConfigParser::getDoubles(const string &key)
{
  // 获取指定键的字符串值列表
  vector<string> values = getStrings(key);

  vector<double> ret; // 存储转换后的双精度浮点数结果
  for (size_t i = 0; i < values.size(); i++)
  {
    const string &value = values[i]; // 获取当前值的引用
    double x;

    // 尝试将字符串转换为双精度浮点数
    if (!Global::tryStringToDouble(value, x))
      throw IOError("无法将'" + value + "'解析为双精度浮点数，用于配置键'" + key + "'，位于配置文件 " + fileName);

    ret.push_back(x); // 将转换后的值添加到结果向量中
  }
  return ret; // 返回双精度浮点数结果向量
}

// 从配置中获取一组双精度浮点数，满足特定范围条件
vector<double> ConfigParser::getDoubles(const string &key, double min, double max)
{
  // 获取指定键的字符串值列表
  vector<string> values = getStrings(key);

  vector<double> ret; // 存储转换后的双精度浮点数结果
  for (size_t i = 0; i < values.size(); i++)
  {
    const string &value = values[i]; // 获取当前值的引用
    double x;

    // 尝试将字符串转换为双精度浮点数
    if (!Global::tryStringToDouble(value, x))
      throw IOError("无法将'" + value + "'解析为双精度浮点数，用于配置键'" + key + "'，位于配置文件 " + fileName);

    // 检查是否为 NaN
    if (isnan(x))
      throw IOError("键'" + key + "'在配置文件 " + fileName + " 中是 NaN（非数值）");

    // 检查是否在指定范围内
    if (x < min || x > max)
      throw IOError("键'" + key + "'在配置文件 " + fileName + " 中必须在范围 " + Global::doubleToString(min) + " 到 " + Global::doubleToString(max) + " 内");

    ret.push_back(x); // 将转换后的值添加到结果向量中
  }
  return ret; // 返回双精度浮点数结果向量
}
