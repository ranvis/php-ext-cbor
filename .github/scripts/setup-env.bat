if "%GH_WORKSPACE%" == "" (
	echo GH_WORKSPACE not set
	exit /b 1
)

if "%TARGET_ISA%" == "" (
	echo TARGET_ISA not set
	exit /b 1
)

set "VSCMD_START_DIR=%CD%"
set __VSCMD_ARG_NO_LOGO=yes

REM https://github.com/actions/runner-images/blob/main/images/windows/Windows2022-Readme.md#visual-studio-enterprise-2022
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" %TARGET_ISA% -vcvars_ver=14.29

set PHP_SDK_RUN_FROM_ROOT=1
set "PHP_SDK_ROOT_PATH=%GITHUB_WORKSPACE%\php-sdk"
call "%PHP_SDK_ROOT_PATH%\bin\phpsdk_setvars.bat"

exit /b %ERRORLEVEL%
