param([Parameter(Mandatory = $true)][string]$RequestFile, [switch]$ElevationWorker)

$ErrorActionPreference = 'Stop'
$request = $null
$locked = $false
$mutex = New-Object System.Threading.Mutex($false, 'Local\BatchlyAudioUpdate')
$completed = New-Object System.Collections.Generic.List[object]
$pendingTemporaryFiles = New-Object System.Collections.Generic.List[string]
$logFolder = Join-Path $env:LOCALAPPDATA 'BatchlyAudio'
New-Item -ItemType Directory -Path $logFolder -Force | Out-Null
$logFile = Join-Path $logFolder 'update-status.json'

function Write-Status([string]$state, [string]$message) {
    @{ state = $state; message = $message; time = [DateTime]::UtcNow.ToString('o') } |
        ConvertTo-Json | Set-Content -LiteralPath $logFile -Encoding UTF8
}

function Get-Sha256([string]$path) {
    # .NET hashing also works when a DAW inherits a restricted PowerShell module path.
    $stream = [IO.File]::OpenRead($path)
    $algorithm = [Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-', '') }
    finally { $stream.Dispose(); $algorithm.Dispose() }
}

function Show-Completion {
    if ($request.restartApp -and $request.standaloneTarget) {
        # Restart from the original user process, never from the elevated installer.
        Start-Process -FilePath $request.standaloneTarget
    } elseif ($request.showResult) {
        Add-Type -AssemblyName System.Windows.Forms
        [System.Windows.Forms.MessageBox]::Show('Update installed. Reopen your DAW to use it.', 'Batchly Audio') | Out-Null
    }
}

try {
    $locked = $mutex.WaitOne(0)
    if (-not $locked) { throw 'An update is already waiting for your audio app to close.' }
    $requestPath = (Resolve-Path -LiteralPath $RequestFile).Path
    $requestFolder = Split-Path -LiteralPath $requestPath
    $allowedRoot = [IO.Path]::GetFullPath((Join-Path $env:LOCALAPPDATA 'BatchlyAudio\updates')) + '\'
    if (-not $requestPath.StartsWith($allowedRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'The update request is outside the Batchly Audio update folder.'
    }
    $request = Get-Content -LiteralPath $requestPath -Raw | ConvertFrom-Json
    if ($request.version -notmatch '^\d{1,5}\.\d{1,5}\.\d{1,5}(-preview\.\d{1,5})?$') { throw 'Invalid update version.' }
    $packageFolder = Join-Path $requestFolder 'package'
    $required = @('Batchly Audio.exe', 'Batchly Audio.vst3/Contents/x86_64-win/Batchly Audio.vst3', 'Batchly Audio.vst3/Contents/Resources/moduleinfo.json')
    foreach ($relative in $required) {
        $expected = $request.files.$relative
        if ($expected -notmatch '^[0-9a-fA-F]{64}$' -or
            (Get-Sha256 (Join-Path $packageFolder $relative)) -ne $expected) {
            throw 'The staged update was changed or damaged. Download it again with Updates.'
        }
    }

    $standaloneTarget = [string]$request.standaloneTarget
    if ($standaloneTarget -and ([IO.Path]::GetFileName($standaloneTarget) -ne 'Batchly Audio.exe' -or
        -not [IO.Path]::IsPathRooted($standaloneTarget))) { throw 'Invalid desktop app destination.' }
    if (-not $standaloneTarget -and -not $request.installPlugin) { throw 'The update has no installation target.' }
    'ready' | Set-Content -LiteralPath (Join-Path $requestFolder 'ready.signal')

    if ($request.hostProcessId -gt 0) {
        $audioProcess = Get-Process -Id $request.hostProcessId -ErrorAction SilentlyContinue
        if ($audioProcess -and $audioProcess.Path -eq [string]$request.hostExecutable) {
            Write-Status 'waiting' 'Update downloaded. Close your audio app normally to finish installation.'
            # Waiting preserves unsaved projects and avoids replacing a loaded plugin.
            $audioProcess.WaitForExit()
        }
    }

    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    $isAdmin = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
    $protectedDesktop = $standaloneTarget -and $standaloneTarget.StartsWith($env:ProgramFiles + '\', [StringComparison]::OrdinalIgnoreCase)
    if (($request.installPlugin -or $protectedDesktop) -and -not $isAdmin) {
        $mutex.ReleaseMutex(); $locked = $false
        $arguments = @('-NoProfile', '-File', ('"' + $PSCommandPath + '"'), '-RequestFile', ('"' + $requestPath + '"'), '-ElevationWorker')
        $installer = Start-Process -FilePath (Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe') -ArgumentList $arguments -Verb RunAs -WindowStyle Hidden -Wait -PassThru
        if ($installer.ExitCode -eq 0) { Show-Completion }
        exit $installer.ExitCode
    }

    $plan = @()
    if ($standaloneTarget) { $plan += @{ source = (Join-Path $packageFolder 'Batchly Audio.exe'); target = $standaloneTarget } }
    if ($request.installPlugin) {
        foreach ($relative in $required[1..2]) {
            $plan += @{ source = (Join-Path $packageFolder $relative); target = (Join-Path (Join-Path $env:CommonProgramFiles 'VST3') $relative) }
        }
    }
    Write-Status 'installing' 'Installing the verified update.'
    foreach ($file in $plan) {
        $parent = [IO.Path]::GetDirectoryName($file.target)
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
        $suffix = [Guid]::NewGuid().ToString('N')
        $temporary = $file.target + '.' + $suffix + '.new'
        $backup = $file.target + '.' + $suffix + '.previous'
        $pendingTemporaryFiles.Add($temporary)
        Copy-Item -LiteralPath $file.source -Destination $temporary
        $existed = Test-Path -LiteralPath $file.target -PathType Leaf
        if ($existed) { [IO.File]::Replace($temporary, $file.target, $backup) }
        else { [IO.File]::Move($temporary, $file.target) }
        $completed.Add(@{ target = $file.target; backup = $backup; existed = $existed })
        if ((Get-Sha256 $file.source) -ne (Get-Sha256 $file.target)) {
            throw 'Installed file verification failed.'
        }
    }
    Write-Status 'complete' ('Batchly Audio ' + $request.version + ' installed. Reopen your audio app.')
    $backups = @($completed | Where-Object { $_.existed } | ForEach-Object { $_.backup })
    $completed.Clear()
    foreach ($backup in $backups) { try { [IO.File]::Delete($backup) } catch { } }
    if (-not $ElevationWorker) { Show-Completion }
    exit 0
} catch {
    $failure = $_.Exception.Message
    for ($i = $completed.Count - 1; $i -ge 0; $i--) {
        $file = $completed[$i]
        try {
            if ($file.existed) { [IO.File]::Replace($file.backup, $file.target, $null) }
            else { [IO.File]::Delete($file.target) }
        } catch { $failure += ' A backup could not be restored: ' + $file.backup }
    }
    Write-Status 'failed' ($failure + ' Close all audio apps and retry Updates.')
    if ($request.showResult) {
        Add-Type -AssemblyName System.Windows.Forms
        [System.Windows.Forms.MessageBox]::Show($failure + "`nClose all audio apps and try Updates again.", 'Batchly Audio update') | Out-Null
    }
    Write-Error $failure
    exit 1
} finally {
    foreach ($temporary in $pendingTemporaryFiles) { if ([IO.File]::Exists($temporary)) { [IO.File]::Delete($temporary) } }
    if ($locked) { $mutex.ReleaseMutex() }
    $mutex.Dispose()
}
