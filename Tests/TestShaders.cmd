@echo off
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x86\fxc.exe" /T vs_4_0 /E vertex_shader /O3 /Fo "TestShaders.vso" TestShaders.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x86\fxc.exe" /T ps_4_0 /E pixel_shader /O3 /Fo "TestShaders.pso" TestShaders.hlsl
earcmgr TestShaders.ecsal :arc
pause