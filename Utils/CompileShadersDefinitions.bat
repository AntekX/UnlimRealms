
:: Compiler
set "DXC=Source\3rdParty\DXCompiler\bin\x64\dxc.exe"
rem set "DXC=%VULKAN_SDK%\Bin\dxc.exe"

:: Shader default parameters:
set "SHADER_MODEL=6_3"
set "SHADER_ENTRYPOINT=main"
set "SHADER_PATH_SRC=Source"
set "SHADER_PATH_DST=Bin"
set "SHADER_NAME=__noname__"
set "SHADER_INCLUDE=-I Source\UnlimRealms -I Source\UnlimRealms\ShaderLib"

:: HLSL to SPRIV specific:
set "SHADER_REGISTER_SPACE=0"
set "SHADER_REGISTER_BINDING=-fvk-b-shift 0 %SHADER_REGISTER_SPACE% -fvk-s-shift 256 %SHADER_REGISTER_SPACE% -fvk-u-shift 512 %SHADER_REGISTER_SPACE% -fvk-t-shift 768 %SHADER_REGISTER_SPACE%"
set "SPIRV_TARGET=vulkan1.2"
set "SPIRV=-spirv"
set "SPIRV=-spirv -fspv-target-env=%SPIRV_TARGET%"