if "%PHP_DIR%" == "" (
	echo PHP_DIR not set
	exit /b 1
)

echo ::group::Configure
call phpize
call configure.bat --with-prefix=%PHP_DIR% %configure_args%
perl -pi .\msvc_mod_make.pl Makefile
echo ::endgroup::

echo ::group::Make
nmake /nologo
echo ::endgroup::

echo ::group::Install
nmake /n /nologo install
nmake /nologo install
echo ::endgroup::

echo ::group::Miscellaneous
REM Remove placeholders in changelog
perl -pi -e "$_ = '' if (/^\[Unreleased\]/../^-?$/)" CHANGELOG.md
dos2unix CHANGELOG.md
echo ::endgroup::
