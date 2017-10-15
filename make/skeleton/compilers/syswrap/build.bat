@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86_amd64
cl.exe /nologo /I "C:\Audio\VstPlugins\APE x64\includes" /Oi /O2 /Ot /GL /MT /fp:precise /Gd /O2 %2 /link /MACHINE:x64 /DEF:"C:\Audio\VstPlugins\APE x64\compilers\syswrap\syswrap.def" /DLL /OUT:%1
