@echo off
Setlocal EnableDelayedExpansion

FOR %%f IN (*.*) DO (
  SET fext=%%~xf
  SET fname=%%~nf
  IF /I NOT "!fext!"==".spv" (
    IF /I NOT "!fext!"==".bat" (
      IF /I NOT "!fext!"==".glsl" (
        ECHO Compiling %%f...
        glslc %%f -o spv/!fname!_!fext:~1!.spv
      )
    )
  )
)

PAUSE