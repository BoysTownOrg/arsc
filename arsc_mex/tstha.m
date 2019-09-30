% tstha.m - test soundcard streaming output through simulated hearing aid

function tstha(di)
% di - device identifier
global rate nsmp nseg ph out1 out2 dvid
nchn = 1;       % number of channels
rate = 44100;   % sampling rate (Hz)
nsmp = 4000;
out1 = {int32(zeros(1,nsmp)),int32(zeros(1,nsmp))};
out2 = {int32(zeros(1,nsmp)),int32(zeros(1,nsmp))};
nseg=length(out1);
%
if (nargin > 0)
   dvid = di;
else
   dvid = arsc_mex('find_dev', 0);
end
name = arsc_mex('dev_name', dvid);
fprintf('device = %s\n', name);
arsc_mex('out_open', dvid, rate, nchn);
arsc_mex('out_prepare', dvid, out2, 0);
arsc_mex('io_start', dvid);
ph=0;
ps=-1;
while (1)
   cs=arsc_mex;
   if (cs ~= ps)
      b = 1 + mod(cs,nseg);
      out1{b}=tone_seg(cs,nsmp,rate);
      out2{b}=ha_sim(out1{b});
      ps = cs;
   end
end
arsc_mex('io_close', dvid);
return

function out=tone_seg(s,nsmp,rate)
global ph
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
out=w;
return
