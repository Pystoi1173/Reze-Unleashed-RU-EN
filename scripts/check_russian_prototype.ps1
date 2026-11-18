$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$utf8 = New-Object System.Text.UTF8Encoding($false, $true)
$files = @(
    'applications/services/locale/locale.h',
    'applications/services/locale/locale.c',
    'applications/services/gui/canvas.c',
    'applications/services/gui/elements.c',
    'applications/services/loader/loader_guide.c',
    'applications/services/loader/loader_guide.h',
    'applications/services/loader/loader_menu.c',
    'applications/settings/system/system_settings.c',
    'applications/settings/clock_settings/application.fam',
    'applications/services/desktop/application.fam',
    'applications/settings/user_guide/user_guide.c',
    'applications/settings/user_guide/application.fam',
    'targets/f7/api_symbols.csv',
    'RU_TEST_PLAN.md'
)
$applicationsRoot = Join-Path $root 'applications'
$files += Get-ChildItem -LiteralPath $applicationsRoot -Recurse -File | Where-Object {
    $_.Extension -in @('.c', '.h', '.fam') -and
    [System.IO.File]::ReadAllText($_.FullName, $utf8).Contains('locale')
} | ForEach-Object {
    $_.FullName.Substring($root.Length + 1).Replace('\', '/')
}
$files = $files | Sort-Object -Unique
foreach ($relative in $files) {
    $path = Join-Path $root $relative
    if (-not (Test-Path -LiteralPath $path)) { throw "Missing: $relative" }
    $text = $utf8.GetString([System.IO.File]::ReadAllBytes($path))
    if ($text.Contains([char]0xFFFD)) { throw "Invalid replacement character: $relative" }
    if ($relative.EndsWith('.c')) {
        if ($relative -ne 'applications/services/locale/locale.c' -and
            $text.Contains('locale_translate(') -and
            $text -notmatch '#include\s+[<"]locale/locale\.h[>"]') {
            throw "Missing locale header: $relative"
        }
        if ($relative -match '^applications/main/([^/]+)/' -and $text.Contains('locale_translate(')) {
            $appManifest = Join-Path $root ("applications/main/{0}/application.fam" -f $Matches[1])
            if (-not (Test-Path -LiteralPath $appManifest)) { throw "Missing app manifest: $relative" }
            $manifestText = [System.IO.File]::ReadAllText($appManifest)
            if ($manifestText -notmatch '"locale"') { throw "Missing locale dependency: $appManifest" }
        }
        $pairs = [regex]::Matches($text, 'locale_translate\(\s*"(?<en>(?:\\.|[^"\\])*)"\s*,\s*"(?<ru>(?:\\.|[^"\\])*)"\s*\)')
        $formatPattern = '%(?:%|(?:\d+\$)?[-+ #0]*(?:\d+|\*)?(?:\.(?:\d+|\*))?(?:hh|ll|[hljztL])?[diuoxXfFeEgGaAcspn])'
        foreach ($pair in $pairs) {
            $english = @([regex]::Matches($pair.Groups['en'].Value, $formatPattern) | ForEach-Object { $_.Value }) -join '|'
            $russian = @([regex]::Matches($pair.Groups['ru'].Value, $formatPattern) | ForEach-Object { $_.Value }) -join '|'
            if ($english -cne $russian) { throw "Translation format mismatch: $relative ($english / $russian)" }
        }
    }
}
$api = [System.IO.File]::ReadAllText((Join-Path $root 'targets/f7/api_symbols.csv'))
foreach ($symbol in @('locale_get_language', 'locale_set_language', 'locale_translate')) {
    if ($api -notmatch "Function,\+,$symbol,") { throw "Missing export: $symbol" }
}
$guide = [System.IO.File]::ReadAllText((Join-Path $root 'applications/settings/user_guide/user_guide.c'))
if ($guide -notmatch 'view_dispatcher_switch_to_view') { throw 'Guide initial view missing' }
if ($guide -notmatch 'guide:.*') { throw 'Automatic guide argument handling missing' }
if ($guide -notmatch 'loader_enqueue_launch') { throw 'Deferred application launch missing' }
$loaderMenu = [System.IO.File]::ReadAllText((Join-Path $root 'applications/services/loader/loader_menu.c'))
if ($loaderMenu -notmatch 'view_dispatcher_send_custom_event') { throw 'Loader menu event dispatch missing' }
if ($loaderMenu -notmatch 'loader_guide_should_show') { throw 'Loader guide policy check missing' }
if ($guide -notmatch 'return VIEW_NONE;') { throw 'Guide exit callback missing' }
Write-Output 'Static checks passed: UTF-8, required files, locale exports, guide entry/exit markers.'
Write-Output 'This is NOT a firmware build or a hardware test.'
