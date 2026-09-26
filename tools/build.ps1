$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
& cmake -S $projectRoot -B (Join-Path $projectRoot 'build') -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) { throw 'Configuration failed.' }
& cmake --build (Join-Path $projectRoot 'build') --config Release --parallel 6
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
& ctest --test-dir (Join-Path $projectRoot 'build') -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'DSP checks failed.' }
