; tstlat.plt
sizfac=5 : yhor=y : ticdir=in : clip=y
xllc=1 : yllc=1 : xlen=9 : ylen=6.5
xdat=$0 : ydat=$1/1e6
include tstlat.txt
stat
xmin=0 : xmax=40 : yavg=$sum_y/$n
xint=4.5 : xper=90
%repeat (rate>0)&(size>0)
xdat=1000*$$0/rate
xlab=(msec)
tlab=tstlat
data
include tstlat.txt
%%
ymin=-200 : ymax=1000
yint=12 : yper=100 : yanskp=1
pltyp=line
plot
