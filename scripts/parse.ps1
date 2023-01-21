$check_dir = Test-Path ./build -PathType Container

if($check_dir -eq $false) {
    New-Item build -ItemType Directory
}

cmake -S . -B build -G "MinGW Makefiles"
