# Deploys a build into a mod folder for testing.
#
# Test builds go to a separate mod ("DragonSense - Test"), never over the stable
# install. Deploying to the stable mod requires -AllowStable, which is for release
# verification only.
param(
    [string]$ModsDir = 'C:\Modlists\Skyrim25-Myedit\mods',
    [string]$ModName = 'DragonSense - Test',
    [switch]$AllowStable
)

$ErrorActionPreference = 'Stop'

if ($ModName -eq 'DragonSense' -and -not $AllowStable) {
    throw "refusing to deploy over the stable mod '$ModName'; pass -AllowStable for a release deploy"
}

$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $projectRoot 'build'
$modRoot = Join-Path $ModsDir $ModName
$destDir = Join-Path $modRoot 'SKSE\Plugins'

$artifacts = @('DragonSense.dll', 'DragonSense.pdb')

foreach ($name in $artifacts) {
    $src = Join-Path $buildDir $name
    if (-not (Test-Path $src)) {
        throw "not built yet: $src"
    }
}

New-Item -ItemType Directory -Force -Path $destDir | Out-Null

foreach ($name in $artifacts) {
    Copy-Item (Join-Path $buildDir $name) $destDir -Force
    $item = Get-Item (Join-Path $destDir $name)
    Write-Output ("deployed {0}  ({1:N0} bytes, {2})" -f $item.Name, $item.Length, $item.LastWriteTime.ToString('HH:mm:ss'))
}

# The English content plugin, as a local external layer, for testing without the Hub.
$layerDest = Join-Path $destDir 'SkyrimNet\external\stoptryharding.dragonsense'
if (Test-Path $layerDest) {
    Remove-Item $layerDest -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $layerDest | Out-Null
Copy-Item (Join-Path $projectRoot 'plugins\en\manifest.json') $layerDest -Force
Copy-Item (Join-Path $projectRoot 'plugins\en\triggers') $layerDest -Recurse -Force
Write-Output "English content plugin synced to $layerDest"

Write-Output "destination: $modRoot"
Write-Output "Enable '$ModName' and disable the stable 'DragonSense' mod while testing, then restart the game."
