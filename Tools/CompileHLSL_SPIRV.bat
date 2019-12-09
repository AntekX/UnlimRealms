rem dxc -help
dxc -T vs_5_0 -E main ..\Source\UnlimRealms\GenericRender\Generic_vs.hlsl -Fo ..\Bin\Generic_vs.spv -I ..\Source\UnlimRealms\ShaderLib
dxc -T ps_5_0 -E main ..\Source\UnlimRealms\GenericRender\Generic_ps.hlsl -Fo ..\Bin\Generic_ps.spv -I ..\Source\UnlimRealms\ShaderLib
PAUSE