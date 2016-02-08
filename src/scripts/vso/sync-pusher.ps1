param (
    [string] $Milestone = "nightly",
    [int] $SyncHoursAgo = 0,
    [string] $RepoPath = $env:BUILD_REPOSITORY_LOCALPATH
)

$PstZone = [System.TimeZoneInfo]::FindSystemTimeZoneById("Pacific Standard Time")
$PstTime = [System.TimeZoneInfo]::ConvertTimeFromUtc((Get-Date).AddHours(-$SyncHoursAgo).ToUniversalTime(), $PstZone)
$SyncTime = $PstTime.ToString("HH:00:00")

. "$PSScriptRoot/sync-to-time.ps1" -Time "$SyncTime"

If (-Not (Test-Path "$NuGetDir\nuget.exe")) {
    Invoke-WebRequest -UseBasicParsing "https://api.nuget.org/downloads/nuget.exe" -OutFile "$RepoPath\packages\nuget.exe"
}

& "${env:ProgramFiles(x86)}\MSBuild\14.0\Bin\MSBuild.exe" "$RepoPath\src\packaging\packages.targets" /t:BuildNuGetPackages /p:RelativeProductBinDir="bin\Product\Windows_NT.x64.Release" /p:ToolchainMilestone=$Milestone /p:BuildOS=Windows_NT /p:BuildArch=x64 /p:BuildType=Release /p:PUSH_REDIR_ONLY=1 /p:RepoPath="$RepoPath"

