setlocal enabledelayedexpansion

echo ::group::Tests
nmake test > ..\test_output.log 2>&1
set exit_status=%errorlevel%

type ..\test_output.log
echo ::endgroup::

set failed=0
for %%F in (tests\*.log) do (
	if "!failed!"=="0" (
		echo ::group::Failed tests
		set failed=1
	)
	echo ==^> %%F ^<==
	type "%%F"
	echo.
)
if "!failed!"=="0" (
	echo No failed tests
) else (
	echo ::endgroup::
)

exit /b %exit_status%
