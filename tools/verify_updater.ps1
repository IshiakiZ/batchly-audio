$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
$testFolder = Join-Path $env:LOCALAPPDATA ('BatchlyAudio\updates\selftest-' + [Guid]::NewGuid().ToString('N'))
$package = Join-Path $testFolder 'package'
New-Item -ItemType Directory -Path $package -Force | Out-Null
$relativeFiles = @('Batchly Audio.exe', 'Batchly Audio.vst3/Contents/x86_64-win/Batchly Audio.vst3', 'Batchly Audio.vst3/Contents/Resources/moduleinfo.json')
$sourceFiles = @(
    (Join-Path $project 'build\BatchlyAudio_artefacts\Release\Standalone\Batchly Audio.exe'),
    (Join-Path $project 'build\BatchlyAudio_artefacts\Release\VST3\Batchly Audio.vst3\Contents\x86_64-win\Batchly Audio.vst3'),
    (Join-Path $project 'build\BatchlyAudio_artefacts\Release\VST3\Batchly Audio.vst3\Contents\Resources\moduleinfo.json')
)
$hashes = @{}
for ($i = 0; $i -lt $relativeFiles.Count; $i++) {
    $target = Join-Path $package $relativeFiles[$i]
    New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($target)) -Force | Out-Null
    Copy-Item -LiteralPath $sourceFiles[$i] -Destination $target
    $hashes[$relativeFiles[$i]] = (Get-FileHash -LiteralPath $target).Hash
}
$appTarget = Join-Path $testFolder 'portable\Batchly Audio.exe'
New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($appTarget)) -Force | Out-Null
'Previous version fixture' | Set-Content -LiteralPath $appTarget
$oldHash = (Get-FileHash -LiteralPath $appTarget).Hash
$requestPath = Join-Path $testFolder 'request.json'
$request = @{ version = '0.1.0-preview.2'; files = $hashes; hostProcessId = 0; hostExecutable = '';
              standaloneTarget = $appTarget; installPlugin = $false; restartApp = $false; showResult = $false }
$powershell = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
$helper = Join-Path $PSScriptRoot 'apply-update.ps1'
function Start-Helper {
    Start-Process -FilePath $powershell -WindowStyle Hidden -ArgumentList @('-NoProfile', '-File', ('"' + $helper + '"'), '-RequestFile', ('"' + $requestPath + '"')) -PassThru
}

$expected = $hashes['Batchly Audio.exe']
$hashes['Batchly Audio.exe'] = '0' * 64
$request | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $requestPath
$bad = Start-Helper
if (-not $bad.WaitForExit(20000)) { throw 'Invalid-package check did not finish.' }
if ($bad.ExitCode -eq 0 -or (Get-FileHash -LiteralPath $appTarget).Hash -ne $oldHash) { throw 'A damaged update was accepted.' }
$hashes['Batchly Audio.exe'] = $expected

$gate = Join-Path $testFolder 'close-host.signal'
$hostScript = Join-Path $testFolder 'host-fixture.ps1'
'param([string]$SignalFile) while (-not (Test-Path -LiteralPath $SignalFile)) { Start-Sleep -Milliseconds 100 }' | Set-Content -LiteralPath $hostScript
$hostFixture = Start-Process -FilePath $powershell -WindowStyle Hidden -ArgumentList @('-NoProfile', '-File', ('"' + $hostScript + '"'), '-SignalFile', ('"' + $gate + '"')) -PassThru
try {
    $request.hostProcessId = $hostFixture.Id
    $request.hostExecutable = $powershell
    $request | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $requestPath
    $valid = Start-Helper
    $statusPath = Join-Path $env:LOCALAPPDATA 'BatchlyAudio\update-status.json'
    $waiting = $false
    for ($attempt = 0; $attempt -lt 100; $attempt++) {
        if ((Test-Path -LiteralPath $statusPath) -and (Get-Content -LiteralPath $statusPath -Raw | ConvertFrom-Json).state -eq 'waiting') { $waiting = $true; break }
        Start-Sleep -Milliseconds 100
    }
    if (-not $waiting -or $valid.HasExited -or (Get-FileHash -LiteralPath $appTarget).Hash -ne $oldHash) { throw 'The updater did not wait safely for the host.' }
    New-Item -ItemType File -Path $gate | Out-Null
    if (-not $valid.WaitForExit(20000) -or $valid.ExitCode -ne 0) { throw 'The verified portable update failed.' }
    if ((Get-FileHash -LiteralPath $appTarget).Hash -ne $expected) { throw 'The installed update differs from its verified source.' }
    if ((Get-Content -LiteralPath $statusPath -Raw | ConvertFrom-Json).state -ne 'complete') { throw 'Success was not recorded.' }
    Write-Output 'PASS: damaged update rejected; host exit respected; portable app updated and hash verified.'
} finally {
    if (-not (Test-Path -LiteralPath $gate)) { New-Item -ItemType File -Path $gate | Out-Null }
}
