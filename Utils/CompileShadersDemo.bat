
:: Include
call Utils\CompileShadersDefinitions.bat %*

:: ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
:: Shaders
:: ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

call:CompileShader "Source\SolutionVS\Demo" "GPUWorkGraphs_cs" "lib" "6_8" "%SHADER_ENTRYPOINT%"

call:CompileShader "Source\SolutionVS\Demo" "sample" "vs" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"
call:CompileShader "Source\SolutionVS\Demo" "sample" "ps" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"
call:CompileShader "Source\SolutionVS\Demo" "sample_raytracing" "lib" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"

call:CompileShader "Source\SolutionVS\Demo" "HybridRendering_cs" "lib" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"
call:CompileShader "Source\SolutionVS\Demo" "HybridRendering_rt" "lib" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"
call:CompileShader "Source\SolutionVS\Demo" "HybridRendering" "vs" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"
call:CompileShader "Source\SolutionVS\Demo" "HybridRendering" "ps" "%SHADER_MODEL%" "%SHADER_ENTRYPOINT%"

echo CompileShadersDemo finished

EXIT /B %ERRORLEVEL%

:: Include
:CompileShader
Utils\CompileShadersFunctions.bat %*