param(
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')]
    [string]$Configuration = 'Release',
    [ValidateSet('gui', 'media', 'audio', 'platform', 'full')][string]$Composition = 'gui',
    [ValidateSet('native', 'platform')][string]$HostKind = 'native',
    [ValidateRange(1, 64)][int]$Jobs = 2
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path

Push-Location $Root
try {
    & "$Root\generate_msvc.ps1" -Composition $Composition -HostKind $HostKind
    if ($LASTEXITCODE -ne 0) {
        throw "MSVC project generation failed with exit code $LASTEXITCODE."
    }

    $Selected = (Get-Content "$Root\build\.active_generator" -Raw).Trim()
    $BuildDirectory = "$Root\build\$Selected"
    $Executable = "$BuildDirectory\$Configuration\EpochGuiDemo.exe"

    & cmake --build $BuildDirectory --config $Configuration --parallel $Jobs
    if ($LASTEXITCODE -ne 0) {
        throw "MSVC build failed with exit code $LASTEXITCODE. No executable was produced."
    }

    & ctest --test-dir $BuildDirectory -C $Configuration --output-on-failure
    if ($LASTEXITCODE -ne 0) {
        throw "EpochGui tests failed with exit code $LASTEXITCODE."
    }

    if (-not (Test-Path $Executable)) {
        throw "MSVC reported success, but the expected executable was not found: $Executable"
    }

    Write-Host "Executable: $Executable"
}
finally {
    Pop-Location
}
