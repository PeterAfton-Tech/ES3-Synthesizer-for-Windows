@echo off
chcp 65001 >nul
net session >nul 2>&1
if %errorlevel% neq 0 (
  echo Pedindo permissao de administrador...
  powershell -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
  exit /b
)
echo Instalando o plugin ES3 em "%CommonProgramFiles%\VST3" ...
xcopy /E /I /Y "%~dp0ES3.vst3" "%CommonProgramFiles%\VST3\ES3.vst3" >nul
if %errorlevel% neq 0 ( echo ERRO ao copiar. & pause & exit /b 1 )
echo.
echo Pronto! Abra o FL Studio e faca: Options ^> Manage plugins ^> Find installed plugins.
echo O programa independente e o arquivo ES3.exe (nao precisa instalar).
pause
