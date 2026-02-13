param(
  [switch]$Reconfigure
)

$ErrorActionPreference = "Stop"

if ($Reconfigure -or -not (Test-Path "build")) {
  cmake -S . -B build -G Ninja
}

cmake --build build
& "./build/gui_pc.exe"
