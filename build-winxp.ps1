param(
    [string]$VisualStudioRoot = 'E:\Program Files\Microsoft Visual Studio\2022\Community',
    [string]$WindowsSdk71A = 'E:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A',
    [string]$UniversalCrtRoot = 'E:\Program Files (x86)\Windows Kits\10',
    [string]$OutputRoot = 'D:\Project\DrvInst\WinXP'
)

$ErrorActionPreference = 'Stop'
$repository = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectSource = Join-Path $repository 'kitsuneDrvInstaller'
$buildRoot = Join-Path $repository 'XPBuild'
$isolatedProject = Join-Path $buildRoot 'src\kitsuneDrvInstaller'
$msbuild = Join-Path $VisualStudioRoot 'MSBuild\Current\Bin\MSBuild.exe'
$project = Join-Path $isolatedProject 'kitsuneDrvInstaller.vcxproj'

foreach ($required in @($msbuild, (Join-Path $WindowsSdk71A 'Include\WinSDKVer.h'),
    (Join-Path $UniversalCrtRoot 'Include\10.0.10240.0\ucrt\corecrt.h'))) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required XP build component is missing: $required"
    }
}

New-Item -ItemType Directory -Force -Path $isolatedProject | Out-Null
Copy-Item -Path (Join-Path $projectSource '*') -Destination $isolatedProject -Recurse -Force
New-Item -ItemType Directory -Force -Path (Join-Path $isolatedProject 'compat') | Out-Null
Set-Content -LiteralPath (Join-Path $isolatedProject 'compat\new.h') -Encoding Ascii -Value @(
    '#pragma once',
    '#include <new>'
)

$projectText = Get-Content -LiteralPath $project -Raw
$includeDirectories = '$(ProjectDir)compat;' +
    $UniversalCrtRoot + '\Include\10.0.10240.0\ucrt;%(AdditionalIncludeDirectories)'
$projectText = $projectText.Replace('<PrecompiledHeader>Use</PrecompiledHeader>',
    '<PrecompiledHeader>Use</PrecompiledHeader>' + "`r`n      " +
    '<AdditionalIncludeDirectories>' + $includeDirectories + '</AdditionalIncludeDirectories>')
$librarySettings = "`r`n      <AdditionalLibraryDirectories Condition=`"'`$(Platform)'=='Win32'`">" +
    $UniversalCrtRoot + '\Lib\10.0.10240.0\ucrt\x86;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>' +
    "`r`n      <AdditionalLibraryDirectories Condition=`"'`$(Platform)'=='x64'`">" +
    $UniversalCrtRoot + '\Lib\10.0.10240.0\ucrt\x64;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>'
$projectText = $projectText.Replace('<OptimizeReferences>true</OptimizeReferences>',
    '<OptimizeReferences>true</OptimizeReferences>' + $librarySettings)
Set-Content -LiteralPath $project -Value $projectText -Encoding Utf8

$sdk = $WindowsSdk71A.TrimEnd('\') + '\'
foreach ($build in @(
    @{ Platform = 'Win32'; Folder = 'x86' },
    @{ Platform = 'x64'; Folder = 'x64' }
)) {
    $out = Join-Path $buildRoot ($build.Folder + '\')
    $obj = Join-Path $buildRoot ('obj-' + $build.Folder + '\')
    & $msbuild $project /m /t:Rebuild /p:Configuration=Release /p:Platform=$($build.Platform) `
        /p:PlatformToolset=v141_xp /p:WindowsSdkDir_71A="$sdk" /p:OutDir="$out" /p:IntDir="$obj"
    if ($LASTEXITCODE -ne 0) { throw "XP $($build.Folder) build failed: $LASTEXITCODE" }
    $destination = Join-Path $OutputRoot $build.Folder
    New-Item -ItemType Directory -Force -Path $destination | Out-Null
    Copy-Item -LiteralPath (Join-Path $out 'kitsuneDrvInstaller.exe') -Destination $destination -Force
}

Write-Output "Windows XP x86/x64 build deployed to $OutputRoot"
