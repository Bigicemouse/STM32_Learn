@echo off
setlocal EnableExtensions

cd /d "%~dp0" || exit /b 1

echo Cleaning project...
echo Project root: %CD%
echo.

rem EIDE / Keil build output directories.
for %%d in (
    build
    DebugConfig
    Listings
    Objects
    bin
    obj
    out
) do (
    if exist "%%d\" (
        echo Removing dir  %%d
        rd /s /q "%%d"
    )
)

rem VS Code / EIDE generated cache files. Keep real editor/debug config files.
for %%d in (
    ".settings"
    ".eide\log"
) do (
    if exist "%%~d\" (
        echo Removing dir  %%~d
        rd /s /q "%%~d"
    )
)

rem Root-level compiler/debug artifacts. Do not recurse into source folders.
for %%f in (
    ".eide.usr.ctx.json"
    ".vscode\c_cpp_properties.json"
    ".vscode\*.log"
    ".vscode\*.lock"
    "*.o"
    "*.d"
    "*.crf"
    "*.lst"
    "*.map"
    "*.lnp"
    "*.axf"
    "*.elf"
    "*.hex"
    "*.bin"
    "*.htm"
    "*.scvd"
    "*.uvguix.*"
    "*.ept"
    "*.eide-template"
) do (
    if exist %%f (
        echo Removing file %%~f
        del /f /q /a %%f
    )
)

echo.
echo Clean done.
exit /b 0
