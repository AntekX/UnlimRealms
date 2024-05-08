
:: ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
:: Functions
:: ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

:: Expected input args: 1 - shader path, 2 - shader name, 3 shader type (vs,ps,cs,lib), 4 - shader model (e.g 6_3), 5 - entry point
:CompileShader
SETLOCAL ENABLEDELAYEDEXPANSION
	set "SHADER_PATH_SRC=%~1"
	set "SHADER_NAME=%~2"
	set "SHADER_TYPE=%~3"
	set "SHADER_MODEL=%~4"
	set "SHADER_ENTRYPOINT=%~5"
	set "SHADER_SPIRV_ARGS="
	if "%SHADER_TYPE%"=="vs" set "SHADER_SPIRV_ARGS=-fvk-invert-y"
	%DXC% -T %SHADER_TYPE%_%SHADER_MODEL% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_%SHADER_TYPE%.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_%SHADER_TYPE%.cso %SHADER_INCLUDE%
	if "%SHADER_MODEL%"=="6_8" goto SkipSPIRV
	%DXC% %SPIRV% -T %SHADER_TYPE%_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_%SHADER_TYPE%.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_%SHADER_TYPE%.spv %SHADER_SPIRV_ARGS% %SHADER_INCLUDE%
	:SkipSPIRV
	echo CompileHLSL_SPIRV compiled %SHADER_NAME%_%SHADER_TYPE% %SHADER_MODEL%
ENDLOCAL
EXIT /B 0