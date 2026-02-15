@echo off
echo [INFO] Generating C++ Protocol...

REM === 请确认下面这个路径是你电脑上 protoc.exe 的真实路径 ===
set PROTOC_PATH=F:\vcpkg\installed\x64-windows\tools\protobuf\protoc.exe

REM 执行编译
"%PROTOC_PATH%" --cpp_out=. game.proto

if exist game.pb.h (
    echo [SUCCESS] Protocol generated successfully!
    echo Files: game.pb.h, game.pb.cc
) else (
    echo [ERROR] Failed to generate protocol. Check the path!
)
pause