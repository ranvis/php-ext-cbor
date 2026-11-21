echo ::group::Env test

if "%GH_WORKSPACE%" == "" (
	echo GH_WORKSPACE not set
	exit /b 1
)

if "%TARGET_ISA%" == "" (
	echo TARGET_ISA not set
	exit /b 1
)

if "%VC_TOOLSET%"=="" (
	echo VC_TOOLSET is not set
	exit /b 1
)

echo %GH_WORKSPACE% / %TARGET_ISA% / %VC_TOOLSET%

echo ::endgroup::
echo ::group::Configure

set "VSCMD_START_DIR=%CD%"
set __VSCMD_ARG_NO_LOGO=yes

echo ::endgroup::
echo ::group::VC setup

REM https://github.com/actions/runner-images/blob/main/images/windows/Windows2025-VS2026-Readme.md#visual-studio-enterprise-2026
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" %TARGET_ISA% -vcvars_ver=%VC_TOOLSET%

echo ::endgroup::
echo ::group::PHP SDK setup

set PHP_SDK_RUN_FROM_ROOT=1
set "PHP_SDK_ROOT_PATH=%GH_WORKSPACE%\php-sdk"
call "%PHP_SDK_ROOT_PATH%\bin\phpsdk_setvars.bat"

echo ::endgroup::

exit /b %ERRORLEVEL%
