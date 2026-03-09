# Build singleinstance.exe and singleinstance_hidden.exe
# Finds Visual Studio, MSVC, and Windows SDK tools for verification.

# Locate vswhere.exe
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    $vswhere = "${env:ProgramFiles}\Microsoft Visual Studio\Installer\vswhere.exe"
}

if (-not (Test-Path $vswhere)) {
    Write-Error "Could not find vswhere.exe."
    exit 1
}

# Get VS and SDK Paths
$vsPath = & $vswhere -latest -property installationPath
# We need the SDK path for mt.exe
$sdkPath = & $vswhere -latest -getComponent Microsoft.VisualStudio.Component.Windows10SDK.19041 -property installationPath

if (-not $vsPath) {
    Write-Error "Visual Studio installation not found."
    exit 1
}

# Define paths
$msBuildPath = Join-Path $vsPath "MSBuild\Current\Bin\MSBuild.exe"
$msvcRoot = Join-Path $vsPath "VC\Tools\MSVC"

# Find MSVC tools
if (Test-Path $msvcRoot) {
    $latestMsvc = Get-ChildItem $msvcRoot | Sort-Object Name -Descending | Select-Object -First 1
    $dumpbinPath = Join-Path $latestMsvc.FullName "bin\Hostx64\x86\dumpbin.exe"
}

# Find mt.exe (Usually in Program Files (x86)\Windows Kits)
$mtPath = Get-ChildItem -Path "${env:ProgramFiles(x86)}\Windows Kits\10\bin" -Filter "mt.exe" -Recurive | 
          Where-Object { $_.FullName -match "x86" } | 
          Select-Object -First 1 -ExpandProperty FullName

# --- Execution ---

# Build
if (Test-Path $msBuildPath) {
    Write-Host "Building Release and ReleaseHidden..." -ForegroundColor Green
    & $msBuildPath /m:8 /t:Build /p:Configuration=Release .\singleinstance.sln
    & $msBuildPath /m:8 /t:Build /p:Configuration=ReleaseHidden .\singleinstance.sln
}

# Manifest Verification
Write-Host "`nVerifying Manifests..." -ForegroundColor Cyan
$exes = @(".\Release\singleinstance.exe", ".\Release\singleinstance_hidden.exe")

foreach ($exe in $exes) {
    if (Test-Path $exe) {
        # Check for the embedded string
        $manifestCheck = Select-String -Path $exe -Pattern "requestedExecutionLevel" -Quiet
        if ($manifestCheck) {
            Write-Host "[PASS] Manifest detected in: $(Split-Path $exe -Leaf)" -ForegroundColor Green
        } else {
            Write-Warning "[FAIL] No Manifest found in: $(Split-Path $exe -Leaf)"
        }
    }
}

# 5. Dependency Check
if ($dumpbinPath -and (Test-Path $dumpbinPath)) {
    Write-Host "`nChecking dependencies..." -ForegroundColor Green
    foreach ($exe in $exes) {
        Write-Host "--- $(Split-Path $exe -Leaf) ---" -ForegroundColor Gray
        & $dumpbinPath /dependents $exe | Select-String "image has the following dependencies", ".dll"
    }
}