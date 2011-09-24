@ECHO OFF
SETLOCAL ENABLEEXTENSIONS
CD /D %~dp0

rem Check for the help switches
IF /I "%~1"=="help"   GOTO SHOWHELP
IF /I "%~1"=="/help"  GOTO SHOWHELP
IF /I "%~1"=="-help"  GOTO SHOWHELP
IF /I "%~1"=="--help" GOTO SHOWHELP
IF /I "%~1"=="/?"     GOTO SHOWHELP


rem Check for the first switch
IF "%~1" == "" (
  SET "BUILDTYPE=Build"
) ELSE (
  IF /I "%~1" == "Build"     SET "BUILDTYPE=Build"   & GOTO CHECKSECONDARG
  IF /I "%~1" == "/Build"    SET "BUILDTYPE=Build"   & GOTO CHECKSECONDARG
  IF /I "%~1" == "-Build"    SET "BUILDTYPE=Build"   & GOTO CHECKSECONDARG
  IF /I "%~1" == "--Build"   SET "BUILDTYPE=Build"   & GOTO CHECKSECONDARG
  IF /I "%~1" == "Clean"     SET "BUILDTYPE=Clean"   & GOTO CHECKSECONDARG
  IF /I "%~1" == "/Clean"    SET "BUILDTYPE=Clean"   & GOTO CHECKSECONDARG
  IF /I "%~1" == "-Clean"    SET "BUILDTYPE=Clean"   & GOTO CHECKSECONDARG
  IF /I "%~1" == "--Clean"   SET "BUILDTYPE=Clean"   & GOTO CHECKSECONDARG
  IF /I "%~1" == "Rebuild"   SET "BUILDTYPE=Rebuild" & GOTO CHECKSECONDARG
  IF /I "%~1" == "/Rebuild"  SET "BUILDTYPE=Rebuild" & GOTO CHECKSECONDARG
  IF /I "%~1" == "-Rebuild"  SET "BUILDTYPE=Rebuild" & GOTO CHECKSECONDARG
  IF /I "%~1" == "--Rebuild" SET "BUILDTYPE=Rebuild" & GOTO CHECKSECONDARG

  ECHO.
  ECHO Unsupported commandline switch!
  ECHO Run "%~nx0 help" for details about the commandline switches.
  CALL :SUBMSG "ERROR" "Compilation failed!"
)


:CHECKSECONDARG
rem Check for the second switch
IF "%~2" == "" (
  SET "ARCH=all"
) ELSE (
  IF /I "%~2" == "x86"   SET "ARCH=x86" & GOTO START
  IF /I "%~2" == "/x86"  SET "ARCH=x86" & GOTO START
  IF /I "%~2" == "-x86"  SET "ARCH=x86" & GOTO START
  IF /I "%~2" == "--x86" SET "ARCH=x86" & GOTO START
  IF /I "%~2" == "x64"   SET "ARCH=x64" & GOTO START
  IF /I "%~2" == "/x64"  SET "ARCH=x64" & GOTO START
  IF /I "%~2" == "-x64"  SET "ARCH=x64" & GOTO START
  IF /I "%~2" == "--x64" SET "ARCH=x64" & GOTO START
  IF /I "%~2" == "all"   SET "ARCH=all" & GOTO START
  IF /I "%~2" == "/all"  SET "ARCH=all" & GOTO START
  IF /I "%~2" == "-all"  SET "ARCH=all" & GOTO START
  IF /I "%~2" == "--all" SET "ARCH=all" & GOTO START

  ECHO.
  ECHO Unsupported commandline switch!
  ECHO Run "%~nx0 help" for details about the commandline switches.
  CALL :SUBMSG "ERROR" "Compilation failed!"
)


:START
CALL "%VS100COMNTOOLS%vsvars32.bat"


:x86
IF "%ARCH%" == "x64" GOTO x64

CALL :SUBMSVC %BUILDTYPE% Release Win32
CALL installer\build_installer.bat x86

IF "%ARCH%" == "x86" GOTO END


:x64
IF "%ARCH%" == "x86" GOTO END

CALL :SUBMSVC %BUILDTYPE% Release x64
CALL installer\build_installer.bat x64


 :END
ECHO. & ECHO.
ENDLOCAL
PAUSE
EXIT /B


:SUBMSVC
ECHO.
TITLE Building vfw_vs2010 - %~1 "%~2|%~3"...
devenv /nologo vfw_vs2010.sln /%~1 "%~2|%~3"
IF %ERRORLEVEL% NEQ 0 CALL :SUBMSG "ERROR" "Compilation failed!"
EXIT /B


:SHOWHELP
TITLE "%~nx0 %1"
ECHO. & ECHO.
ECHO Usage:  %~nx0 [Clean^|Build^|Rebuild] [x86^|x64^|all]
ECHO.
ECHO Notes:  You can also prefix the commands with "-", "--" or "/".
ECHO         The arguments are not case sensitive.
ECHO. & ECHO.
ECHO Executing "%~nx0" will use the defaults: "%~nx0 build all"
ECHO.
ECHO If you skip the second argument the default one will be used.
ECHO Examples:
ECHO "%~nx0 rebuild" is the same as "%~nx0 rebuild all"
ECHO.
ECHO WARNING: "%~nx0 x86" or "%~nx0 x64" won't work.
ECHO.
ENDLOCAL
EXIT /B
