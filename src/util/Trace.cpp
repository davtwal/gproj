// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : Trace.cpp
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

#include "util/Trace.h"

Trace Trace::All = Trace(WarningLevel::All);
Trace Trace::Info = Trace(WarningLevel::Minimal);
Trace Trace::Warn = Trace(WarningLevel::Warnings);
Trace Trace::Error = Trace(WarningLevel::Errors);
Trace::TraceStop Trace::Stop;
Trace::WarningLevel Trace::s_globalLevel;

std::fstream Trace::s_traceFile;

Trace::Trace(WarningLevel l)
  : m_currentLevel(l) {}

void Trace::Init(WarningLevel level) {
  s_globalLevel = level;

  if(!s_traceFile.is_open()) {
    s_traceFile.open(TRACE_FILENAME);
    if(!s_traceFile.is_open()) {
      Error << "Could not open trace file!" << Stop;
    }
  }
}

void Trace::Shutdown() {
  if (s_traceFile.is_open()) {
    s_traceFile << "\n";
    s_traceFile.close();
  }
}
