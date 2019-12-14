dxc -help
rem dxc -T vs_5_0 -E main ..\Source\UnlimRealms\GenericRender\Generic_vs.hlsl -Fo ..\Bin\Generic_vs.spv -I ..\Source\UnlimRealms\ShaderLib
rem dxc -T ps_5_0 -E main ..\Source\UnlimRealms\GenericRender\Generic_ps.hlsl -Fo ..\Bin\Generic_ps.spv -I ..\Source\UnlimRealms\ShaderLib
dxc -spirv -fvk-invert-y -T vs_5_0 -E main ..\Source\SolutionVS\Demo\sample_vs.hlsl -Fo ..\Bin\sample_vs.spv
dxc -spirv -T ps_5_0 -E main ..\Source\SolutionVS\Demo\sample_ps.hlsl -Fo ..\Bin\sample_ps.spv
PAUSE