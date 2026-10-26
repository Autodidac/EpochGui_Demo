param(
    [ValidateSet('gui', 'media', 'audio', 'platform', 'full')][string]$Composition = 'gui',
    [ValidateSet('native', 'platform')][string]$HostKind = 'native'
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

$Generators = (& cmake --help | Out-String)
if ($LASTEXITCODE -ne 0) {
    throw "CMake generator discovery failed with exit code $LASTEXITCODE."
}

if ($Generators -match 'Visual Studio 18 2026') {
    & cmake --preset msvc-vs2026 "-DEPOCHGUI_DEMO_COMPOSITION=$Composition" "-DEPOCHGUI_DEMO_HOST=$HostKind"
    if ($LASTEXITCODE -ne 0) {
        throw "Visual Studio 2026 project generation failed with exit code $LASTEXITCODE."
    }

    New-Item -ItemType Directory -Force "$Root\build" | Out-Null
    Set-Content -Path "$Root\build\.active_generator" -Value 'vs2026'
    Write-Host "Generated: $Root\build\vs2026\EpochGuiDemo.sln"
} elseif ($Generators -match 'Visual Studio 17 2022') {
    & cmake --preset msvc-vs2022 "-DEPOCHGUI_DEMO_COMPOSITION=$Composition" "-DEPOCHGUI_DEMO_HOST=$HostKind"
    if ($LASTEXITCODE -ne 0) {
        throw "Visual Studio 2022 project generation failed with exit code $LASTEXITCODE."
    }

    New-Item -ItemType Directory -Force "$Root\build" | Out-Null
    Set-Content -Path "$Root\build\.active_generator" -Value 'vs2022'
    Write-Host "Generated: $Root\build\vs2022\EpochGuiDemo.sln"
} else {
    throw 'No supported Visual Studio generator was found. Install Visual Studio 2022 or 2026 with Desktop development with C++.'
}
