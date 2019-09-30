rem build arsc.lib using VC6
@ECHO OFF

set CC="C:\Program Files\Microsoft Visual Studio\VC98\Bin\CL"
set DEFS=/D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "WIND" /D "ASIO" 
set CFLAGS=/nologo /W3 /GX /O2 /I "../../include" %DEFS% /FD /c
set INCLUDE=C:\Program Files\Microsoft Visual Studio\VC98\atl\include;C:\Program Files\Microsoft Visual Studio\VC98\mfc\include;C:\Program Files\Microsoft Visual Studio\VC98\include
set LIB=C:\Program Files\Microsoft Visual Studio\VC98\mfc\lib;C:\Program Files\Microsoft Visual Studio\VC98\lib
set CFILES=..\arsc_api.c ..\arsc_asio\arsc_asio.c ..\arsc_helper.c ..\arsc_win.c ..\arsc_xfer.c
set CPPFILES=..\arsc_asio\arsc_asio_wrappers.cpp ..\arsc_asio\asio.cpp  ..\arsc_asio\asiodrivers.cpp  ..\arsc_asio\asiolist.cpp 
set LIB="C:\Program Files\Microsoft Visual Studio\VC98\Bin\LIB"
set OFILES=arsc_api.obj arsc_asio.obj arsc_helper.obj arsc_win.obj arsc_xfer.obj arsc_asio_wrappers.obj asio.obj  asiodrivers.obj  asiolist.obj 

%CC% %CFLAGS% %CFILES%
%CC% %CFLAGS% %CPPFILES%
%LIB% /OUT:arsc.lib %OFILES%

del %OFILES%
del *.idb

set CC=
set DEFS=
set CFLAGS=
set INCLUDE=
set LIB=
set CFILES=
set CPPFILES=
set LIB=
set OFILES=

