@echo off
cls
SET DLL_NAME=game
set WARNINGS= -W4 -wd4201 -wd4505 -wd4003
set COMPILER_FLAGS= -MTd -nologo -GR- -EHa- -Od -Oi -FC -Z7 
set LINKER_FLAGS= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib Kernel32.lib D3d11.lib DXGI.lib D3DCompiler.lib dxguid.lib Ole32.lib Avrt.lib Dsound.lib
set PREPROCESSOR_DEFINITIONS= -D"APP_DLL_NAMES={\"%DLL_NAME%.dll\"}" -DUSE_MMCSS=0 -D"APP_HEADER_FILENAME=\"../code/game.h\""

set DllLinkerFlags=  -incremental:no -PDB:%DLL_NAME%%random%.pdb
set ExportedFunctions= -EXPORT:update -EXPORT:render -EXPORT:init -EXPORT:close_app

cd ..

IF NOT EXIST build mkdir build
pushd build
IF NOT EXIST shaders mkdir shaders
IF NOT EXIST data mkdir data

del *.pdb > NUL 2> NUL


@REM SHADER COMPILING


set COMPILE_SHADER= fxc /nologo  /Zi /T
set VS= vs_5_0 /E vs
set PS= ps_5_0 /E ps

%COMPILE_SHADER% %VS% ../source/template/shaders/3d_vs.hlsl /Fo shaders/3d_vs.cso	
%COMPILE_SHADER% %PS% ../source/template/shaders/simple_ps.hlsl /Fo shaders/simple_ps.cso
%COMPILE_SHADER% %VS% ../source/template/shaders/instancing_vs.hlsl /Fo shaders/instancing_vs.cso
%COMPILE_SHADER% %VS% ../source/template/shaders/simple_vs.hlsl /Fo shaders/simple_vs.cso


set COMPARE_TIMES= G:/boqui/projects/Code/misc/compare_times.bat

@REM //@@bitwise_enum@@, @@constructor@@
set start_time=%time%
call g:/boqui/projects/code/metaprogramming/build/metaprogramming ..\source\code
call %COMPARE_TIMES% %start_time% %time%

@REM COMPILING APP DLL



echo Waiting for PDB > lock.tmp
set start_time=%time%
cl %PREPROCESSOR_DEFINITIONS% %COMPILER_FLAGS% %WARNINGS% /Fe%DLL_NAME% "..\source\code\game_update.cpp" -Fm%DLL_NAME%.map -LD /link  %DllLinkerFlags% %ExportedFunctions% 
del lock.tmp
call %COMPARE_TIMES% %start_time% %time%


@REM COMPILING WIN LAYER


REM THIS IS FOR PRECOMPILED windows.h AND OTHER THINGS
@REM IF NOT EXIST pch.pch cl %COMPILER_FLAGS% %WARNINGS% /c /Ycpch.h ..\source\code\pch.cpp 

set start_time=%time%
@REM call compare_filetimes ..\source\code\win_main.cpp winmain.exeskip
@REM '1' if i want to skip compiling when i have not written into the win_main file, anything else otherwise
IF %ERRORLEVEL% EQU 0 (
   cl %PREPROCESSOR_DEFINITIONS% %COMPILER_FLAGS% %WARNINGS% /Fewinmain "..\source\template\win_main.cpp" -Fmwinmain.map /link -subsystem:windows %LINKER_FLAGS% 
)
call %COMPARE_TIMES% %start_time% %time%



@REM  DOCUMENTATION


REM PRECOMPILED HEADERS
REM /Yupch.h

REM flags
REM https://learn.microsoft.com/en-us/windows/win32/direct3dtools/dx-graphics-tools-fxc-syntax

REM /Yu 			use precompiled file (apparently it knows that pch.pch is the precompiled file of pch.h)
REM -P or /P 		Preprocesses C and C++ source files and writes the preprocessed output to a file
REM -MT 			use the static library and don't look for dll (for more compatibility)
REM -MD				use the dll
REM -nologo 		disables showing visual studio logo each time you compile
REM -GR-			turns off c++ runtime type information 
REM -EHa- 			turns off c++ exception handling (try, throw, catch)
REM -Od				turns off optimizations
REM -O2 			Maximum speed theorically
REM -Oi				compiler intrinsics, prevent calls to c runtime library whenever it can
REM -W4				warning level in the compiler
REM -wd4201 		nameless struct/union
REM -wd4100 		The formal parameter is not referenced in the body of the function. The unreferenced parameter is ignored
REM -wd4189			A variable is declared and initialized but not used.
REM -wd4505			unused function warning
REM -wd4003			not enough arguments for function-like macro invocation
REM -FC				Causes the compiler to display the full path of source code files passed to the compiler in diagnostics (instead of just the single filename).
REM -Z7				.pdb generation for debug info for debuggers
REM -FmFILENAME.map	to produce a map file that indicates where everything is in the executable
REM LINKER SWITCHES
REM /link			links :0
REM -opt:ref 		declaration to not put anything that the executable doesn't use
REM -subsystem	:windows or :console and 5.1 to have compatibility with windows xp
REM /FeNEWNAME		renames the output executable

popd
