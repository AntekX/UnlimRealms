rem dxc -help

set "SHADER_MODEL=5_0"
set "SHADER_REGISTER_SPACE=0"
set "SHADER_REGISTER_BINDING=-fvk-b-shift 0 %SHADER_REGISTER_SPACE% -fvk-s-shift 256 %SHADER_REGISTER_SPACE% -fvk-t-shift 512 %SHADER_REGISTER_SPACE% -fvk-u-shift 768 %SHADER_REGISTER_SPACE%"
set "SHADER_ENTRYPOINT=main"
set "SHADER_PATH_SRC=..\Source"
set "SHADER_PATH_DST=..\Bin"
set "SHADER_NAME=__noname__"

set "SHADER_PATH_SRC=..\Source\SolutionVS\Demo"
set "SHADER_NAME=sample"
dxc -spirv -T vs_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_vs.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_vs.spv -fvk-invert-y -D VK_SAMPLE
dxc -spirv -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\ImguiRender"
set "SHADER_NAME=Imgui"
dxc -spirv -T vs_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_vs.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_vs.spv -fvk-invert-y
dxc -spirv -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\GenericRender"
set "SHADER_NAME=Generic"
dxc -spirv -T vs_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_vs.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_vs.spv -fvk-invert-y -I ..\Source\UnlimRealms\ShaderLib
dxc -spirv -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\GenericRender"
set "SHADER_NAME=FullScreenQuad"
dxc -spirv -T vs_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_vs.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_vs.spv -fvk-invert-y
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\Isosurface"
set "SHADER_NAME=Isosurface"
dxc -spirv -T vs_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_vs.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_vs.spv -fvk-invert-y -I ..\Source\UnlimRealms\ShaderLib
dxc -spirv -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv -I ..\Source\UnlimRealms\ShaderLib
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\Atmosphere"
set "SHADER_NAME=Atmosphere"
dxc -spirv -T vs_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_vs.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_vs.spv -fvk-invert-y -I ..\Source\UnlimRealms\ShaderLib
dxc -spirv -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv -I ..\Source\UnlimRealms\ShaderLib
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\HDRRender"
set "SHADER_NAME=HDRTargetLuminance"
dxc -spirv -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\HDRRender"
set "SHADER_NAME=AverageLuminance"
dxc -spirv -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\HDRRender"
set "SHADER_NAME=BloomLuminance"
dxc -spirv -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\HDRRender"
set "SHADER_NAME=Blur"
dxc -spirv -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\HDRRender"
set "SHADER_NAME=ToneMapping"
dxc -spirv -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

echo CompileHLSL_SPIRV finished

rem PAUSE