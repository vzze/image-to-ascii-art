$check_dir = Test-Path ./build -PathType Container

if($check_dir -eq $false) {
    New-Item build -ItemType Directory
}

Write-Host "`n-------------- CMAKE --------------`n"

Invoke-Expression "cmake -S . -B build -G `"MinGW Makefiles`""
Invoke-Expression "cmake --build ./build"
