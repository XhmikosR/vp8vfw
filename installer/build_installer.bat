@ECHO OFF
SETLOCAL ENABLEEXTENSIONS
CD /D %~dp0

REM You can set here the Inno Setup path if for example you have Inno Setup Unicode
REM installed and you want to use the ANSI Inno Setup which is in another location
SET "InnoSetupPath=H:\progs\thirdparty\isetup"


rem Check for the help switches
IF /I "%~1"=="help"   GOTO SHOWHELP
IF /I "%~1"=="/help"  GOTO SHOWHELP
IF /I "%~1"=="-help"  GOTO SHOWHELP
IF /I "%~1"=="--help" GOTO SHOWHELP
IF /I "%~1"=="/?"     GOTO SHOWHELP


rem Check for the first switch
IF "%~1" == "" (
  SET "ARCH=all"
) ELSE (
  IF /I "%~1" == "x86"   SET "ARCH=x86" & GOTO START
  IF /I "%~1" == "/x86"  SET "ARCH=x86" & GOTO START
  IF /I "%~1" == "-x86"  SET "ARCH=x86" & GOTO START
  IF /I "%~1" == "--x86" SET "ARCH=x86" & GOTO START
  IF /I "%~1" == "x64"   SET "ARCH=x64" & GOTO START
  IF /I "%~1" == "/x64"  SET "ARCH=x64" & GOTO START
  IF /I "%~1" == "-x64"  SET "ARCH=x64" & GOTO START
  IF /I "%~1" == "--x64" SET "ARCH=x64" & GOTO START
  IF /I "%~1" == "all"   SET "ARCH=all" & GOTO START
  IF /I "%~1" == "/all"  SET "ARCH=all" & GOTO START
  IF /I "%~1" == "-all"  SET "ARCH=all" & GOTO START
  IF /I "%~1" == "--all" SET "ARCH=all" & GOTO START

  ECHO.
  ECHO Unsupported commandline switch!
  ECHO Run "%~nx0 help" for details about the commandline switches.
  CALL :SUBMSG "ERROR" "Compilation failed!"
)

:START
CALL :SubGetInnoSetupPath

IF NOT EXIST %InnoSetupPath% (
  ECHO. & ECHO.
  CALL :SUBMSG "ERROR" "Inno Setup not found!"
  GOTO END
)

:x86
IF "%ARCH%" == "x64" GOTO x64
CALL :SubInno x86
IF "%ARCH%" == "x86" GOTO END


:x64
IF "%ARCH%" == "x86" GOTO END
CALL :SubInno x64


 :END
ECHO. & ECHO.
ENDLOCAL
EXIT /B


:SubInno
ECHO.
TITLE Building vfw_vs2010 %~1 installer...
"%InnoSetupPath%\iscc.exe" /Q "setup.iss" /D%~1Build
IF %ERRORLEVEL% NEQ 0 (
  CALL :SUBMSG "ERROR" "Build failed!
) ELSE (
  CALL :SUBMSG "INFO" "%~1 installer compiled successfully!
)
EXIT /B


:SubGetInnoSetupPath
REM Detect if we are running on 64bit WIN and use Wow6432Node, set the path
REM of Inno Setup accordingly and compile the installer
IF "%PROGRAMFILES(x86)%zzz"=="zzz" (
  SET "U_=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"
) ELSE (
  SET "U_=HKLM\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall"
)

IF NOT DEFINED InnoSetupPath (
  FOR /F "delims=" %%a IN (
    'REG QUERY "%U_%\Inno Setup 5_is1" /v "Inno Setup: App Path"2^>Nul^|FIND "REG_"') DO (
    SET "InnoSetupPath=%%a" & CALL :SubInnoSetupPath %%InnoSetupPath:*Z=%%)
)
EXIT /B


:SubInnoSetupPath
SET InnoSetupPath=%*
EXIT /B


:SHOWHELP
TITLE "%~nx0 %1"
ECHO. & ECHO.
ECHO Usage:  %~nx0 [x86^|x64^|all]
ECHO.
ECHO Notes:  You can also prefix the commands with "-", "--" or "/".
ECHO         The arguments are not case sensitive.
ECHO. & ECHO.
ECHO Executing "%~nx0" will use the defaults: "%~nx0 all"
ECHO.
ENDLOCAL
EXIT /B


:SUBMSG
ECHO. & ECHO ______________________________
ECHO [%~1] %~2
ECHO ______________________________ & ECHO.
IF /I "%~1" == "ERROR" (
  PAUSE
  EXIT
) ELSE (
  EXIT /B
)
