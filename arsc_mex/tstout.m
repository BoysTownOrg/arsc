% tstout.m - test soundcard output

function tstout
nchn = 2;		% number of channels
nswp = 1;		% number of sweeps
rate = 44100;	% sampling rate (Hz)
ramp = 0.010;  % ramp duration (s)
f=1000;			% tone frequency (Hz)
t=0.5;			% tone duration (s)
a=0.1;			% tone amplitude
s=itone(a,f,t,rate,ramp);	% signal channel
o=itone(0,0,t,rate,0);		% other channel
g=itone(0,0,t*3/2,rate,0);	% gap
if (nchn == 2)
	s1=[s,o];
	s2=[o,s];
   gg=[g,g];
else
   s1=s;
   s2=s;
   gg=g;
end
out={gg,s1,gg,s2,gg};
nseg=length(out);
%
dvid = arsc_mex('find_dev', 0);
name = arsc_mex('dev_name', dvid);
fprintf('device = %s\n', name);
arsc_mex('out_open', dvid, rate, nchn);
arsc_mex('out_prepare', dvid, out, nswp);
arsc_mex('io_start', dvid);
seg=-1;
while (seg < nseg)
   cs=arsc_mex('io_cur_seg', dvid);
   if (cs ~= seg)
      seg = cs;
      fprintf('seg=%d\n', double(seg));
   end
end
arsc_mex('io_close', dvid);
return

function s=itone(a,f,t,r,d)	% generate int32 tone
a=a*((2^31)-1);					% tone amplitude
n=round(r*t);						% array length
s=a*sin(2*pi*f*t*(1:n)/n);
if (d > 0)
   m = round(r * d);
   i = 0:(m-1);
   j = 1 + i;
   k = n - i;
   w(j)=(1-cos(pi*i/m))/2;		% ramp
   s(j)=s(j).*w(j);				% ramp up
   s(k)=s(k).*w(j);				% ramp down
end
s=int32(round(s'));
return
