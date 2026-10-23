# Copies the Prisma menu assets (assets/ = mod-root layout: PrismaUI_F4/views/DiverseBodiesRedux and
# F4SE/Plugins/DiverseBodiesRedux/locales) over the mod roots used for testing. The DLL itself is
# deployed by the build (OutDir) and by hand to the other roots, see the project notes.
#
#   powershell -File tools/deploy_assets.ps1            # the three default test mods
#   powershell -File tools/deploy_assets.ps1 -Roots "D:\mods\DBR"
param(
    [string[]]$Roots = @(
        'E:\Games\Fallout 4\september2026\mods\DBR2.0',
        'E:\Games\Fallout 4\september2026\mods\CUSTOM Diverse Bodies',
        'E:\March2026\CUSTOM 39. Diverse Bodies'
    )
)

$ErrorActionPreference = 'Stop'
$assets = Join-Path (Split-Path -Parent $PSScriptRoot) 'assets'
if (-not (Test-Path $assets)) { throw "assets folder not found: $assets" }

foreach ($root in $Roots) {
    if (-not (Test-Path $root)) { Write-Warning "skipped (no such folder): $root"; continue }

    # The view folder is replaced as a whole: a stale bundle left from an older build must not stay.
    $view = Join-Path $root 'PrismaUI_F4\views\DiverseBodiesRedux'
    if (Test-Path $view) { Remove-Item $view -Recurse -Force }

    Copy-Item (Join-Path $assets '*') $root -Recurse -Force
    Write-Host "deployed -> $root"
}
