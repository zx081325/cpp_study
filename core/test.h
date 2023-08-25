#ifndef CORE_TEST_H_
#define CORE_TEST_H_

#include <sstream>

#include "../core/global.h"

// 一个始终定义的断言版本，不受 NDEBUG 影响
#define testAssert(EX) (void)((EX) || (TestCommon::testAssertFailed(#EX, __FILE__, __LINE__), 0))

namespace TestCommon
{
  // 当测试断言失败时调用，输出错误信息并退出
  inline void testAssertFailed(const char *msg, const char *file, int line)
  {
    Global::fatalError(std::string("Failed test assert: ") + std::string(msg) + "\n" + std::string("file: ") + std::string(file) + "\n" + std::string("line: ") + Global::intToString(line));
  }

  // 对比实际输出和预期输出，如果不匹配，输出详细信息并退出
  inline void expect(const char *name, const std::string &actual, const std::string &expected)
  {
    using namespace std;
    string a = Global::trim(actual);                // 去除实际输出的空格
    string e = Global::trim(expected);              // 去除预期输出的空格
    vector<string> alines = Global::split(a, '\n'); // 按行分割实际输出
    vector<string> elines = Global::split(e, '\n'); // 按行分割预期输出

    bool matches = true;
    int firstLineDiff = 0;
    for (int i = 0; i < std::max(alines.size(), elines.size()); i++)
    {
      if (i >= alines.size() || i >= elines.size())
      {
        firstLineDiff = i;
        matches = false;
        break;
      }
      if (Global::trim(alines[i]) != Global::trim(elines[i]))
      { // 比较每行内容是否匹配
        firstLineDiff = i;
        matches = false;
        break;
      }
    }

    if (!matches)
    {
      cout << "Expect test failure!" << endl; // 输出测试失败信息
      cout << name << endl;
      cout << "Expected===============================================================" << endl;
      cout << expected << endl;
      cout << "Got====================================================================" << endl;
      cout << actual << endl;

      cout << "=======================================================================" << endl;
      cout << "First line different (0-indexed) = " << firstLineDiff << endl;
      string actualLine = firstLineDiff >= alines.size() ? string() : alines[firstLineDiff];
      string expectedLine = firstLineDiff >= elines.size() ? string() : elines[firstLineDiff];
      cout << "Actual  : " << actualLine << endl;
      cout << "Expected: " << expectedLine << endl;
      int j;
      for (j = 0; j < actualLine.size() && j < expectedLine.size(); j++)
        if (actualLine[j] != expectedLine[j])
          break;
      cout << "Char " << j << " differs" << endl;
    }
  }

  // 对比实际输出流和预期输出，如果不匹配，输出详细信息并退出
  inline void expect(const char *name, std::ostringstream &actual, const std::string &expected)
  {
    expect(name, actual.str(), expected);
    actual.str("");
    actual.clear();
  }

}

#endif // CORE_TEST_H_
