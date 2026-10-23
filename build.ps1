$ErrorActionPreference = "Stop"

cmake -S . -B build -A x64
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

cmake --build build --config Release
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

Write-Host "Build output: build\Release\WhatsAppWrapper.exe"
