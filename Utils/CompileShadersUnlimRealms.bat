
:: Include
call Utils\CompileShadersDefinitions.bat %*

:: ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
:: Shaders
:: ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

call:CompileShader "Source\UnlimRealms\ImguiRender" "Imgui" "vs" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"
call:CompileShader "Source\UnlimRealms\ImguiRender" "Imgui" "ps" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"

call:CompileShader "Source\UnlimRealms\GenericRender" "Generic" "vs" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"
call:CompileShader "Source\UnlimRealms\GenericRender" "Generic" "ps" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"
call:CompileShader "Source\UnlimRealms\GenericRender" "FullScreenQuad" "vs" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"

call:CompileShader "Source\UnlimRealms\Isosurface" "Isosurface" "vs" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"
call:CompileShader "Source\UnlimRealms\Isosurface" "Isosurface" "ps" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"

call:CompileShader "Source\UnlimRealms\Atmosphere" "Atmosphere" "vs" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"
call:CompileShader "Source\UnlimRealms\Atmosphere" "Atmosphere" "ps" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"
call:CompileShader "Source\UnlimRealms\Atmosphere" "LightShafts" "ps" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"

call:CompileShader "Source\UnlimRealms\HDRRender" "HDRTargetLuminance" "ps" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"
call:CompileShader "Source\UnlimRealms\HDRRender" "AverageLuminance" "ps" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"
call:CompileShader "Source\UnlimRealms\HDRRender" "BloomLuminance" "ps" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"
call:CompileShader "Source\UnlimRealms\HDRRender" "Blur" "ps" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"
call:CompileShader "Source\UnlimRealms\HDRRender" "ToneMapping" "ps" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"

echo CompileShadersUnlimRealms finished

EXIT /B %ERRORLEVEL%

:: Include
:CompileShader
Utils\CompileShadersFunctions.bat %*