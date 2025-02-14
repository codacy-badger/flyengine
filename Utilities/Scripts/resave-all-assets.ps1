. "$PSScriptRoot\common-functions.ps1"

$appPath = Find-EditorProcessor
"Using $appPath"

# Re-save all assets
Get-ChildItem -Path $PSScriptRoot\..\..\. -Filter plProject -Recurse -File | ForEach-Object {
    $projectDir = $_.Directory.FullName
    
    "Re-saving assets in project $projectDir"

    & $appPath -project $projectDir -resave | Out-Null
}