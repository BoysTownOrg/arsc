clear all
if (isunix)
   mex arsc_mex.c -larsc -lasound -lm
else
   mex -O arsc_mex.c arsc.lib ksguid.lib winmm.lib user32.lib advapi32.lib ole32.lib
end
