@echo off
rem Compile la ROM avec SGDK 2.11 (Windows).
rem Pre-requis : variable d'environnement GDK pointant vers le dossier SGDK
rem (ex: set GDK=C:\SGDK) et Java installe.
if "%GDK%"=="" (
  echo La variable GDK n'est pas definie. Exemple : set GDK=C:\SGDK
  pause
  exit /b 1
)
%GDK%\bin\make -f %GDK%\makefile.gen
if errorlevel 1 (
  echo.
  echo ECHEC de la compilation - copie le message ci-dessus a Claude.
  pause
  exit /b 1
)
echo.
echo OK : la ROM est dans out\rom.bin
pause
