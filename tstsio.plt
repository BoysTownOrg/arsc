; tstsio.plt
sizfac=5 : yhor=y : ticdir=in : clip=y
xllc=1 : yllc=1 : xlen=9 : ylen=6
xdat=$0 : ydat=$1
include tstsio.txt
stat 
xmin=(max(0,round(tpk)-2))
xmax=xmin+4
%repeat (rate>0)&(size>0)
xdat=1000*$$0/rate
xlab=(msec)
data
include tstsio.txt
%%
xint=4 : xper=90
ymin=0 : ymax=3000
yint=6 : yper=75
pltyp=line
plot
