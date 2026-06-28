param(
    [string]$UniversalRoot = (Resolve-Path "$PSScriptRoot\..").Path
)

$ErrorActionPreference = 'Stop'

$readmePath = Join-Path $UniversalRoot 'README.md'
if (-not (Test-Path -LiteralPath $readmePath)) {
    throw 'Missing README.md'
}

$readme = Get-Content -Raw -LiteralPath $readmePath

function Assert-Contains {
    param(
        [string]$Text,
        [string[]]$Needles,
        [string]$Label
    )

    foreach ($needle in $Needles) {
        if ($Text -notmatch [regex]::Escape($needle)) {
            throw "$Label missing token: $needle"
        }
    }
}

function Assert-NotContains {
    param(
        [string]$Text,
        [string[]]$Needles,
        [string]$Label
    )

    foreach ($needle in $Needles) {
        if ($Text -match [regex]::Escape($needle)) {
            throw "$Label still contains removed token: $needle"
        }
    }
}

Assert-Contains $readme @(
    'External/spdlog',
    'OverlayLog.h/cpp',
    'Renderer/Backends',
    'OverlayTheme.h/cpp',
    'OverlayLayout.h/cpp',
    'OverlayDraw.h/cpp',
    'OverlayWidgets.h/cpp',
    'OverlayWindowManager.h/cpp',
    'OverlayFonts.h/cpp',
    'OverlayIcons.h/cpp',
    'OverlayGallery.h/cpp'
) 'README architecture map'

$exampleMatch = [regex]::Match(
    $readme,
    '### 3\. Basic Example[\s\S]*?```cpp\s*(?<code>[\s\S]*?)```',
    [System.Text.RegularExpressions.RegexOptions]::Singleline)

if (!$exampleMatch.Success) {
    throw 'README missing Basic Example C++ code block.'
}

$example = $exampleMatch.Groups['code'].Value

Assert-Contains $example @(
    '#include <Windows.h>',
    '#include <cstdio>',
    '#include <filesystem>',
    '#include <string>',
    '#include "UniversalOverlay.h"',
    '#include "imgui.h"',
    'GetConfigPath(HMODULE module)',
    'UniversalOverlay::SetMenuDefaultSize',
    'UniversalOverlay::RegisterConfigBool',
    'UniversalOverlay::RegisterConfigFloat',
    'UniversalOverlay::RegisterConfigInt',
    'UniversalOverlay::LoadConfig(configPath)',
    'UniversalOverlay::RegisterSettingsSection',
    'UniversalOverlay::RegisterTab',
    'UniversalOverlay::RegisterFloatingWindow',
    'UniversalOverlay::RegisterRenderCallback',
    'UniversalOverlay::MarkConfigDirty',
    'UniversalOverlay::SaveConfig(configPath)',
    'UniversalOverlay::Shutdown()',
    'FreeLibraryAndExitThread'
) 'README Basic Example'

foreach ($removed in @('g_esp_', 'DrawESP', 'ESP Configuration', 'injects into a target DirectX 11 game')) {
    if ($example -match [regex]::Escape($removed)) {
        throw "README Basic Example still contains stale wording: $removed"
    }
}

$registerBoolIndex = $example.IndexOf('UniversalOverlay::RegisterConfigBool')
$loadIndex = $example.IndexOf('UniversalOverlay::LoadConfig(configPath)')
$settingsIndex = $example.IndexOf('UniversalOverlay::RegisterSettingsSection')
$tabIndex = $example.IndexOf('UniversalOverlay::RegisterTab')

if ($registerBoolIndex -lt 0 -or $loadIndex -lt 0 -or $registerBoolIndex -gt $loadIndex) {
    throw 'README Basic Example must register config values before LoadConfig.'
}

if ($settingsIndex -lt 0 -or $tabIndex -lt 0 -or $settingsIndex -gt $tabIndex) {
    throw 'README Basic Example should register the built-in Settings section before custom tabs.'
}

Assert-NotContains $readme @(
    'D3D10',
    'Direct3D 10',
    'e-book'
) 'README'

$repoContractFiles = @(
    (Join-Path $UniversalRoot 'UniversalOverlay.vcxproj'),
    (Join-Path $UniversalRoot 'UniversalOverlay.vcxproj.filters')
)
$repoContractFiles += Get-ChildItem -LiteralPath (Join-Path $UniversalRoot 'Src') -Recurse -File |
    Where-Object { $_.Extension -in @('.h', '.cpp') } |
    ForEach-Object { $_.FullName }

$repoContractText = foreach ($file in $repoContractFiles) {
    Get-Content -Raw -LiteralPath $file
}

Assert-NotContains ($repoContractText -join "`n") @(
    'D3D10',
    'Direct3D 10',
    'e-book',
    'DrawEBook',
    'D3D10Backend',
    'GetD3D10Backend'
) 'overlay source/project contract'

Write-Host 'UniversalOverlay README contract check passed.'
