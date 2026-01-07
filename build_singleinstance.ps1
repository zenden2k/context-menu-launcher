# Build singleinstance.exe and singleinstance_hidden.exe
# Finds Visual Studio and the latest MSVC toolkit, saves errors from version changes.

# 1. Locate vswhere.exe (Checking both x86 and 64-bit Program Files)
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    $vswhere = "${env:ProgramFiles}\Microsoft Visual Studio\Installer\vswhere.exe"
}

if (-not (Test-Path $vswhere)) {
    Write-Error "Could not find vswhere.exe. Is Visual Studio Installer installed?"
    exit 1
}

# 2. Get the VS Installation Path
$vsPath = & $vswhere -latest -property installationPath

if (-not $vsPath) {
    Write-Error "Visual Studio installation not found."
    exit 1
}

# 3. Define paths dynamically
$msBuildPath = Join-Path $vsPath "MSBuild\Current\Bin\MSBuild.exe"
$msvcRoot = Join-Path $vsPath "VC\Tools\MSVC"

# Find the latest MSVC toolset folder (e.g., 14.44.35207)
if (Test-Path $msvcRoot) {
    $latestMsvc = Get-ChildItem $msvcRoot | Sort-Object Name -Descending | Select-Object -First 1
    $dumpbinPath = Join-Path $latestMsvc.FullName "bin\Hostx64\x64\dumpbin.exe"
}

# --- Execution ---

# Build the solution
if (Test-Path $msBuildPath) {
    Write-Host "Building Release..." -ForegroundColor Green
    & $msBuildPath /m:8 /property:Configuration=Release .\singleinstance.sln
    & $msBuildPath /m:8 /property:Configuration=ReleaseHidden .\singleinstance.sln
}
else {
    Write-Error "MSBuild not found at: $msBuildPath"
}

# Run dumpbin on the result
if ($dumpbinPath -and (Test-Path $dumpbinPath)) {
    Write-Host "Checking dependencies..." -ForegroundColor Green
    & $dumpbinPath /dependents .\Release\singleinstance.exe
    & $dumpbinPath /dependents .\Release\singleinstance_hidden.exe
}
else {
    Write-Error "Dumpbin.exe not found. Check if C++ build tools are installed."
}