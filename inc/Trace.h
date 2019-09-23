// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : Trace.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 13d
// * Last Altered: 2019y 09m 13d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * Description :
// *
// *
// *
// *
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

#ifndef DW_TRACE_H
#define DW_TRACE_H

#include <iostream>
#include <fstream>

class Trace {
public:
  static constexpr const char* TRACE_FILENAME = "trace.log";

  enum class WarningLevel {
    All,
    Minimal,
    Warnings,
    Errors
  };

  static void Init(WarningLevel level = WarningLevel::All);
  static void Shutdown();

  struct TraceStop {
    int dummy{0};
  };

  template <typename T>
  friend Trace& operator<<(Trace& tr, T const& m);

  static Trace     All;
  static Trace     Info;
  static Trace     Warn;
  static Trace     Error;
  static TraceStop Stop;

private:
  Trace(WarningLevel l);

  bool shouldPrint() const {
    return m_currentLevel >= s_globalLevel;
  }

  [[nodiscard]] std::ostream& stream() const {
    return m_currentLevel == WarningLevel::Errors ? std::cerr : std::cout;
  }

  WarningLevel        m_currentLevel{WarningLevel::All};
  static std::fstream s_traceFile;
  static WarningLevel s_globalLevel;
};

#include <vector>


template <typename T, std::enable_if_t<std::is_compound<T>::value, int> = 0>
Trace & operator<<(Trace & os, const std::vector<T> & v) {
  os << "{\n";
  for (size_t i = 0; i < v.size(); ++i) {
    os << "  [\n" << v[i];
    if (i != v.size() - 1)
      os << "  ],\n";
    else os << "  ]\n";
  }
  os << "}\n";
  return os;
}

template <typename T, std::enable_if_t<!std::is_compound<T>::value, int> = 0>
Trace& operator<<(Trace& os, const std::vector<T>& v) {
  os << "[";
  for (size_t i = 0; i < v.size(); ++i) {
    os << v[i];
    if (i != v.size() - 1)
      os << ", ";
  }
  os << "]\n";
  return os;
}

template <>
inline Trace& operator<<<const char*>(Trace& os, const std::vector<const char*>& v) {
  for (auto& str : v)
    os << v << "\n";

  return os;
}

template <>
inline Trace& operator<<<std::string>(Trace& os, const std::vector<std::string>& v) {
  for (auto& str : v)
    os << str << "\n";

  return os;
}

template <typename T>
Trace& operator<<(Trace& tr, T const& m) {
  if (tr.shouldPrint()) {
    tr.stream() << m;

    if (Trace::s_traceFile.is_open())
      Trace::s_traceFile << m;
  }

  return tr;
}

template <>
inline Trace& operator<<<Trace::TraceStop>(Trace& tr, Trace::TraceStop const& m) {
  return operator<<(tr, "\n");
}

#include "TraceVK.inl"

#endif
