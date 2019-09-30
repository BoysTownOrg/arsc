				ARSC_MEX

The following MATLAB scripts test the basic ARSC_MEX functions and provide 
examples of how to use these functions:

tstlst - Lists the names of the soundcard devices. The device marked 
         with '>' will be used for synchronous i/o. Under Windows,
         the list of ASIO devices, if any, is appended to the list
         of Windows WDM/MME devices.

tstout - Outputs two 1-sec tones, first on the left channel, then on the 
         right channel. This demonstrates how sounds can be coordinated 
         with visual prompts, as required in psychoacoustic tests.

tstfm - Outputs a continuous tone that continuously changes frequency. 
        You may hear occasional glitches.

tstlat - Measures loopback latency of the soundcard. This tests synchronous 
         averaging and assumes that the DAC is connected to the ADC.
         
tstnfo - Queries user for soundcard info writes info to file.

tstvfs - Helps measure soundcard D/A & A/D volts-full-scale.
         
sysres - Measures the "system response" between the soundcard output and input.
	 This script provides a more complete example of using ARSC_MEX to 
         perform synchronous averaging.

A quick summary of the various arsc_mex functions and their arguments
is printed when "help arsc_mex" is typed within MATLAB. 

The Windows version of arsc_mex requires Microsoft C-runtime libraries on your
computer. If Windows complains that 'This application has failed to start 
because the application configuration is incorrect.', download the C-runtime 
installation program from http://audres.org/downloads/vcredist_x86.exe.

The arsc_mex.mex file is rebuilt by typing "make" at the MATLAB command line,
which compiles the C source code (arsc_mex.c) into a MEX file. This assumes 
that MATLAB is configured to use a C compiler and that the appropriate ARSC 
library is in the current directory (see make.m).

For more information about the ARSC library, see http://audres.org/rc/arsc/ 
or http://audres.org/downloads/arsc_api.pdf.
