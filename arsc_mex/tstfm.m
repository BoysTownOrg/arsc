% tstfm.m - test soundcard streaming output

function tstfm(di)
% di - device identifier
global rate nsmp nseg ph out dvid

nchn = 1;		% number of channels
rate = 44100;	% sampling rate (Hz)
nsmp = 4000;
out = {int32(zeros(1,nsmp)),int32(zeros(1,nsmp))};
nseg=length(out);
%
if (nargin > 0)
   dvid = di;
else
   dvid = arsc_mex('find_dev', 0);
end
name = arsc_mex('dev_name', dvid);
fprintf('device = %s\n', name);
arsc_mex('out_open', dvid, rate, nchn);
arsc_mex('out_prepare', dvid, out, 0);
arsc_mex('io_start', dvid);
ph=0;
ps=-1;
while (1)
   cs=arsc_mex;
   if (cs ~= ps)
      tone_seg(cs);	% uses infile function
      ps = cs;
   end
end
arsc_mex('io_close', dvid);
return

function tone_seg(s)
global rate nseg ph out

s = double(s);
b = 1 + mod(s,nseg);
nsmp = length(out{b});
mr = 0.05;
m = (2^31) - 1;
a = m / 10;
d = 1 / rate;
c = d * nsmp;
t = c * s;
w=zeros([1 nsmp]);
for i=1:nsmp
   oct = sin(2 * pi * mr * t);
   fr = 1000 * (2 ^ oct);
   dp = 2 * pi * fr * d;
   ph = ph + dp;
   w(i) = int32(round(a * sin(ph)));
   t = t + d;
end
ph = mod(ph, 2 * pi);
out{b}(1,1:nsmp)=w;
return
