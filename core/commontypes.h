#ifndef COMMONTYPES_H
#define COMMONTYPES_H

// 启用状态枚举
struct enabled_t {
  enum value { False, True, Auto };
  value x;

  enabled_t() = default;
  constexpr enabled_t(value a) : x(a) { }

  // 禁用向bool的隐式转换
  explicit operator bool() = delete;
  
  // 相等比较操作符
  constexpr bool operator==(enabled_t a) const { return x == a.x; }
  
  // 不等比较操作符
  constexpr bool operator!=(enabled_t a) const { return x != a.x; }

  // 转换为字符串
  std::string toString() {
    return x == True ? "true" : x == False ? "false" : "auto";
  }

  // 从字符串解析enabled_t
  static bool tryParse(const std::string& v, enabled_t& buf) {
    if(v == "1" || v == "t" || v == "true" || v == "enabled" || v == "y" || v == "yes")
      buf = True;
    else if(v == "0" || v == "f" || v == "false" || v == "disabled" || v == "n" || v == "no")
      buf = False;
    else if(v == "auto")
      buf = Auto;
    else
      return false;
    return true;
  }

};

#endif //COMMONTYPES_H