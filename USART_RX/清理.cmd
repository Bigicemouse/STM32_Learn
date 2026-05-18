@echo off
setlocal

echo Cleaning project...
echo.

for %%d in (build DebugConfig Listings Objects bin obj out) do (
    if exist "%%d" (
        echo Removing %%d
        rd /s /q "%%d"
    )
)

if exist ".vscode\keil-assistant.log" (
    echo Removing .vscode\keil-assistant.log
    del /q ".vscode\keil-assistant.log"
)

if exist ".vscode\uv4.log.lock" (
    echo Removing .vscode\uv4.log.lock
    del /q ".vscode\uv4.log.lock"
)

if exist ".vscode\c_cpp_properties.json" (
    echo Removing .vscode\c_cpp_properties.json
    del /q ".vscode\c_cpp_properties.json"
)

if exist "*.scvd" (
    echo Removing *.scvd
    del /q "*.scvd"
)

if exist "*.uvguix.*" (
    echo Removing *.uvguix.*
    del /q "*.uvguix.*"
)

echo.
echo Clean done.
exit /b 0
