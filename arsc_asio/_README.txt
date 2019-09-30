							16-Nov-2015:STN

The ARSC_ASIO distribution files are:

	_README.txt
		This file.

	arsc_asio.c
		Contains the ASIO specific code to integrate into the
		ARSC API.  It also includes bufferSwitchTimeInfo().

	arsc_asio_wrappers.cpp
		Wrapper calls to Steinberg's ASIO SDK.  These were
		necessary in order to maintain ANSI C implementation because
		the SDK uses C++ for interfaces, i.e. CLSIDs, etc.

	arsc_asio.vcproj
		Project file for Visual Studio.net.

The following files must be copied to this directory from Steinberg's ASIO SDK 2.3.
	
	asio.cpp
	asio.h
	asiodrivers.cpp
	asiodrivers.h
	asiolist.cpp
	asiolist.h
	asiosys.h
	ginclude.h
	iasiodrv.h

See http://www.steinberg.net/329+M52087573ab0.html for details about ASIO.
