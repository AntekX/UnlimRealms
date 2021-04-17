rem dxc -help

set "SPIRV_TARGET=vulkan1.2"
set "SPIRV=-spirv"
rem set "SPIRV=-spirv -fspv-target-env=%SPIRV_TARGET%"
set "SHADER_MODEL=6_3"
set "SHADER_REGISTER_SPACE=0"
set "SHADER_REGISTER_BINDING=-fvk-b-shift 0 %SHADER_REGISTER_SPACE% -fvk-s-shift 256 %SHADER_REGISTER_SPACE% -fvk-u-shift 512 %SHADER_REGISTER_SPACE% -fvk-t-shift 768 %SHADER_REGISTER_SPACE%"
set "SHADER_ENTRYPOINT=main"
set "SHADER_PATH_SRC=..\Source"
set "SHADER_PATH_DST=..\Bin"
set "SHADER_NAME=__noname__"

set "SHADER_PATH_SRC=..\Source\SolutionVS\Demo"
set "SHADER_NAME=sample"
dxc %SPIRV% -T vs_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_vs.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_vs.spv -fvk-invert-y -D VK_SAMPLE
dxc %SPIRV% -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\SolutionVS\Demo"
set "SHADER_NAME=sample_raytracing"
dxc %SPIRV% -T lib_%SHADER_MODEL% %SHADER_REGISTER_BINDING% %SHADER_PATH_SRC%\%SHADER_NAME%_lib.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_lib.spv -I ..\Source\UnlimRealms -I ..\Source\UnlimRealms\ShaderLib
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\SolutionVS\Demo"
set "SHADER_NAME=HybridRendering"
dxc %SPIRV% -T lib_%SHADER_MODEL% %SHADER_REGISTER_BINDING% %SHADER_PATH_SRC%\%SHADER_NAME%_cs_lib.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_cs_lib.spv -I ..\Source\UnlimRealms -I ..\Source\UnlimRealms\ShaderLib
dxc %SPIRV% -T lib_%SHADER_MODEL% %SHADER_REGISTER_BINDING% %SHADER_PATH_SRC%\%SHADER_NAME%_rt_lib.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_rt_lib.spv -I ..\Source\UnlimRealms -I ..\Source\UnlimRealms\ShaderLib
dxc %SPIRV% -T vs_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_vs.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_vs.spv -I ..\Source\UnlimRealms -I ..\Source\UnlimRealms\ShaderLib -fvk-invert-y
dxc %SPIRV% -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv -I ..\Source\UnlimRealms -I ..\Source\UnlimRealms\ShaderLib
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\ImguiRender"
set "SHADER_NAME=Imgui"
dxc %SPIRV% -T vs_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_vs.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_vs.spv -fvk-invert-y
dxc %SPIRV% -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\GenericRender"
set "SHADER_NAME=Generic"
dxc %SPIRV% -T vs_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_vs.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_vs.spv -fvk-invert-y -I ..\Source\UnlimRealms\ShaderLib
dxc %SPIRV% -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\GenericRender"
set "SHADER_NAME=FullScreenQuad"
dxc %SPIRV% -T vs_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_vs.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_vs.spv -fvk-invert-y
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\Isosurface"
set "SHADER_NAME=Isosurface"
dxc %SPIRV% -T vs_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_vs.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_vs.spv -fvk-invert-y -I ..\Source\UnlimRealms\ShaderLib
dxc %SPIRV% -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv -I ..\Source\UnlimRealms\ShaderLib
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\Atmosphere"
set "SHADER_NAME=Atmosphere"
dxc %SPIRV% -T vs_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_vs.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_vs.spv -fvk-invert-y -I ..\Source\UnlimRealms\ShaderLib
dxc %SPIRV% -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv -I ..\Source\UnlimRealms\ShaderLib
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\Atmosphere"
set "SHADER_NAME=LightShafts"
dxc %SPIRV% -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv -I ..\Source\UnlimRealms\ShaderLib
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\HDRRender"
set "SHADER_NAME=HDRTargetLuminance"
dxc %SPIRV% -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\HDRRender"
set "SHADER_NAME=AverageLuminance"
dxc %SPIRV% -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\HDRRender"
set "SHADER_NAME=BloomLuminance"
dxc %SPIRV% -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\HDRRender"
set "SHADER_NAME=Blur"
dxc %SPIRV% -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

set "SHADER_PATH_SRC=..\Source\UnlimRealms\HDRRender"
set "SHADER_NAME=ToneMapping"
dxc %SPIRV% -T ps_%SHADER_MODEL% %SHADER_REGISTER_BINDING% -E %SHADER_ENTRYPOINT% %SHADER_PATH_SRC%\%SHADER_NAME%_ps.hlsl -Fo %SHADER_PATH_DST%\%SHADER_NAME%_ps.spv
echo CompileHLSL_SPIRV compiled %SHADER_NAME% shader(s)

echo CompileHLSL_SPIRV finished

rem PAUSE