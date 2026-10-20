# Copies the freshly built plugin and the English content plugin into the MO2 mod folder.
# Run after a build; no arguments needed for the normal case.
param(
    [string]$ModsDir = 'C:\Modlists\Skyrim25-Myedit\mods',
    [string]$ModName = 'DragonSense'
)

$ErrorActionPreference = 'Stop'

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
Copy-Item (Join-Path $projectRoot 'plugins\en\*') $layerDest -Recurse -Force
Write-Output "English content plugin synced to $layerDest"

Write-Output "destination: $modRoot"
Write-Output "MOD folder is '$ModName' - enable it in MO2 if it is not already enabled, and restart the game (SKSE loads plugins only at startup)."
