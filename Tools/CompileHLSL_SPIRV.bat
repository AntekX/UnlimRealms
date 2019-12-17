rem dxc -help

set "SHADER_MODEL=5_0"
set "SHADER_PATH_SRC=..\Source\SolutionVS\Demo"
set "SHADER_PATH_DST=..\Bin\"
set "SHADER_NAME=__noname__"

set "SHADER_NAME=sample"
dxc -spirv -fvk-invert-y -T vs_%SHADER_MODEL% -E main %SHADER_PATH_SRC%\%SHADER_NAME%_vs.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_vs.spv
dxc -spirv -T ps_%SHADER_MODEL% -E main %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

echo CompileHLSL_SPIRV finished

rem PAUSE