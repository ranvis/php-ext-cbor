if "%GH_WORKSPACE%" == "" (
	echo GH_WORKSPACE not set
	exit /b 1
)

if "%ARC_NAME%" == "" (
	echo ARC_NAME not set
	exit /b 1
)

REM set files=%PHP_DIR%\ext\%basename%.dll repo\README.md repo\LICENSE
REM zip -j9 %ARC_NAME% %files%
cd %GH_WORKSPACE%\%PHP_DIR%\ext
7z a %GH_WORKSPACE%\%ARC_NAME% %basename%.dll
cd %GH_WORKSPACE%\repo
7z a %GH_WORKSPACE%\%ARC_NAME% CHANGELOG.md LICENSE README.md stubs
echo arc_name=%ARC_NAME%>> %GITHUB_OUTPUT%
