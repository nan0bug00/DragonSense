# Builds the release archive layout: the DLL plus every language plugin, with a
# FOMOD that installs exactly one language.
param(
    [string]$PluginsDir = (Join-Path (Split-Path -Parent $PSScriptRoot) 'plugins'),
    [string]$Output     = (Join-Path (Split-Path -Parent $PSScriptRoot) 'package\DragonSense'),
    [string]$Dll        = (Join-Path (Split-Path -Parent $PSScriptRoot) 'build\DragonSense.dll'),
    [string]$Version    = '0.0.0'
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path $Dll)) { throw "missing DLL: $Dll" }
if (-not (Test-Path $PluginsDir)) { throw "missing plugins: $PluginsDir" }

$names = [ordered]@{
    en      = 'English'
    ru      = 'Russian'
    ja      = 'Japanese'
    pt      = 'Portuguese'
    es      = 'Spanish'
    fr      = 'French'
    de      = 'German'
    it      = 'Italian'
    pl      = 'Polish'
    ko      = 'Korean'
    vi      = 'Vietnamese'
    zh      = 'Chinese (Simplified)'
    'zh-TW' = 'Chinese (Traditional)'
}

if (Test-Path $Output) { Remove-Item $Output -Recurse -Force }
$core = Join-Path $Output 'core\SKSE\Plugins'
New-Item -ItemType Directory -Force -Path $core | Out-Null
Copy-Item $Dll $core -Force

$languageCodes = @()
foreach ($dir in Get-ChildItem $PluginsDir -Directory) {
    $code = $dir.Name
    $layer = Join-Path $Output ("langs\{0}\SKSE\Plugins\SkyrimNet\external\stoptryharding.dragonsense" -f $code)
    New-Item -ItemType Directory -Force -Path $layer | Out-Null
    Copy-Item (Join-Path $dir.FullName 'manifest.json') $layer -Force
    Copy-Item (Join-Path $dir.FullName 'triggers') $layer -Recurse -Force
    if (Test-Path (Join-Path $dir.FullName 'cover.png')) {
        Copy-Item (Join-Path $dir.FullName 'cover.png') $layer -Force
    }
    $languageCodes += $code
}

# English first (it is the default), then the rest alphabetically.
$languageCodes = @('en') + @($languageCodes | Where-Object { $_ -ne 'en' } | Sort-Object)

$fomod = Join-Path $Output 'fomod'
New-Item -ItemType Directory -Force -Path $fomod | Out-Null

$info = @"
<?xml version="1.0" encoding="UTF-8"?>
<fomod>
  <Name>DragonSense</Name>
  <Author>stoptryharding</Author>
  <Version>$Version</Version>
  <Description>NPCs hear a dragon's roar and the Greybeards' summons. Choose one language.</Description>
  <Website>https://github.com/nan0bug00/DragonSense</Website>
</fomod>
"@
Set-Content -Path (Join-Path $fomod 'info.xml') -Value $info -Encoding utf8

$options = foreach ($code in $languageCodes) {
    $label = if ($names.Contains($code)) { $names[$code] } else { $code }
    $type = if ($code -eq 'en') {
        '<typeDescriptor><type name="Recommended"/><defaultType name="Recommended"/></typeDescriptor>'
    } else {
        '<typeDescriptor><type name="Optional"/><defaultType name="Optional"/></typeDescriptor>'
    }
    @"
            <plugin name="$label">
              <description>Narration in $label.</description>
              <files>
                <folder source="langs/$code" destination=""/>
              </files>
              $type
            </plugin>
"@
}

$config = @"
<?xml version="1.0" encoding="UTF-8"?>
<config xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://qconsulting.ca/fo3/ModConfig5.0.xsd">
  <moduleName>DragonSense</moduleName>
  <requiredInstallFiles>
    <folder source="core" destination=""/>
  </requiredInstallFiles>
  <installSteps order="Explicit">
    <installStep name="Language">
      <optionalFileGroups order="Explicit">
        <group name="Language" type="SelectExactlyOne">
          <plugins order="Explicit">
$($options -join "`n")
          </plugins>
        </group>
      </optionalFileGroups>
    </installStep>
  </installSteps>
</config>
"@
Set-Content -Path (Join-Path $fomod 'ModuleConfig.xml') -Value $config -Encoding utf8

Write-Output "built $Output with $($languageCodes.Count) languages"
Write-Output "languages: $($languageCodes -join ', ')"
