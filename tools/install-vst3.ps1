param([string]$PluginSource = '')

$ErrorActionPreference = 'Stop'
$logDirectory = Join-Path $env:LOCALAPPDATA 'BatchlyAudio'
New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
$logPath = Join-Path $logDirectory 'installation.log'

try {
    if (-not $PluginSource) {
        $PluginSource = Join-Path $PSScriptRoot 'Batchly Audio.vst3'
        if (-not (Test-Path -LiteralPath $PluginSource)) {
            $PluginSource = Join-Path $PSScriptRoot '..\build\BatchlyAudio_artefacts\Release\VST3\Batchly Audio.vst3'
        }
    }
    $source = (Resolve-Path -LiteralPath $PluginSource).Path
    $binaryRelative = 'Contents\x86_64-win\Batchly Audio.vst3'
    if (-not (Test-Path -LiteralPath (Join-Path $source $binaryRelative) -PathType Leaf)) {
        throw 'Choose the entire Batchly Audio.vst3 bundle from the Windows package.'
    }

    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        # FL Studio uses a protected system folder, so Windows must approve the copy.
        $arguments = @('-NoProfile', '-File', ('"' + $PSCommandPath + '"'), '-PluginSource', ('"' + $source + '"'))
        $process = Start-Process -FilePath (Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe') -ArgumentList $arguments -Verb RunAs -WindowStyle Hidden -Wait -PassThru
        if ($process.ExitCode -ne 0) { throw "Installation failed. See $logPath" }
        Write-Output 'Installed Batchly Audio. In FL Studio, choose Manage plugins > Find installed plugins.'
        exit 0
    }

    $pluginDirectory = Join-Path $env:CommonProgramFiles 'VST3'
    $destination = Join-Path $pluginDirectory 'Batchly Audio.vst3'
    if ($source -eq $destination) { throw 'The source is already in the installation folder.' }
    New-Item -ItemType Directory -Path $pluginDirectory -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $pluginDirectory -Recurse -Force
    foreach ($file in Get-ChildItem -LiteralPath $source -File -Recurse) {
        $relative = $file.FullName.Substring($source.Length + 1)
        $installedFile = Join-Path $destination $relative
        if ((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath $installedFile).Hash) {
            throw "Installed file verification failed: $relative"
        }
    }
    'SUCCESS: Batchly Audio installed and file hashes verified.' | Set-Content -LiteralPath $logPath
} catch {
    $_.Exception.Message | Set-Content -LiteralPath $logPath
    Write-Error $_
    exit 1
}
