param(
    [string]$Version = "v0.6"
)

$src = "c:\Users\USUARIO\Desktop\ES3 Synth"
$atualizacoesDir = Get-ChildItem -Path $src -Directory | Where-Object { $_.Name -like "atualiza*" } | Select-Object -First 1
if (-not $atualizacoesDir) {
    $atualizacoesDir = New-Item -ItemType Directory -Path (Join-Path $src "atualizações") -Force
}
$zipDest = Join-Path $atualizacoesDir.FullName "ES3_projeto_$Version.zip"
$staging = Join-Path $atualizacoesDir.FullName "staging_$Version"

if (Test-Path $staging) {
    Remove-Item -Recurse -Force $staging
}
if (Test-Path $zipDest) {
    Remove-Item -Force $zipDest
}

New-Item -ItemType Directory -Force $staging | Out-Null

$items = @(
    ".github",
    ".gitignore",
    "CMakeLists.txt",
    "README.md",
    "Source",
    "Tests",
    "docs",
    "packaging",
    "tools",
    "ES2 UI.jpg",
    "ES2-Default-Patch.jpg"
)

foreach ($i in $items) {
    $p = Join-Path $src $i
    if (Test-Path $p) {
        Copy-Item -Path $p -Destination $staging -Recurse -Force
        Write-Host "Copiado: $i"
    }
}

# Cria o arquivo ZIP incluindo arquivos e diretórios ocultos (.github)
$zipItems = Get-ChildItem -Path $staging -Force
Compress-Archive -Path $zipItems.FullName -DestinationPath $zipDest -Force

Remove-Item -Recurse -Force $staging

Write-Host "Arquivo ZIP gerado com sucesso em: $zipDest"
Get-Item $zipDest | Select-Object FullName, Length
