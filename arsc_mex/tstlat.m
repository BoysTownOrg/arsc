% tstlat.m - test soundcard loopback latency

function tstlat
nsec = 5;       % number of seconds
fclk = 7;       % clicks per second
rate = 48000;   % sampling rate
dvid = arsc_mex('find_dev', 'sync');
name = arsc_mex('dev_name', dvid);
fprintf('device = %s\n', name);
for i=1:nsec
   lat(i)=click(dvid, fclk, rate);
end
fprintf('latency range: %.3f to %.3f ms\n', min(lat), max(lat));
return

function lat=click(dvid, fclk, rate)
arsc_mex('io_open', dvid, rate);
rate=arsc_mex('get_rate', dvid);
nsmp = round(rate/fclk);        % buffer size
pw=8;                           % pulse width
so=zeros(nsmp,1);               % output buffer
so(1:pw)=1;                     % make click
si=arsc_mex('sync_avg', dvid, so, fclk);
arsc_mex('io_close', dvid);
j=(pw+1):nsmp;                  % avoid stimulus atifact
[mxv,mxi]=max(abs(si(j)));
lat=(mean(mxi)-1)*1000/rate;
plot(j,si(j));
drawnow;
return
