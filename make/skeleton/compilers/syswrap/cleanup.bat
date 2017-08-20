@echo off
echo cleaning up files...
set "path1=%1%"
set "path2=%path1%.dll"
del "%path2%"
set "path2=%path1%.exp"
del "%path2%"
set "path2=%path1%.lib"
del "%path2%"