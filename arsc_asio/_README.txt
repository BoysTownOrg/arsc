							Aug-2009:STN

These following header files are needed to support soundcard drivers that use WAVE_FORMAT_EXTENSIBLE
and are not included in VS6. Copy these two files to ...\VC98\include.

	ksmedia.h
	ksguid.h

The following files are part of Steinberg's ASIO SDK and  
should all be copied to the .../arsc/arsc_asio directory.
	
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
