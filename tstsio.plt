; tstsio.plt
sizfac=5 : yhor=y : ticdir=in : clip=y
xllc=1 : yllc=1 : xlen=9 : ylen=6
xdat=$0 : ydat=$1
include tstsio.txt
stat 
xmin=0 : xmax=ceil($x_max/100)*100
%repeat (rate>0)&(size>0)
xmax=1000*size/rate/10
xdat=1000*$$0/rate
xlab=(msec)
data
include tstsio.txt
%%
xint=2 : xper=90
ymin=-200 : ymax=1000
yint=12 : yper=100 : yanskp=1
pltyp=line
plot
