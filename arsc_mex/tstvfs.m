% tstvfs - determine soundcard volt-full-scale
% function tstvfs(t)
% t - tone duration
%
function tstvfs(t)
if (nargin < 1)
   t = 1;  % tone duration
end
fprintf('%s\n\n',arsc_mex('version'));
[vo,vi] = card_vfs(t);
return

function [vo,vi] = card_vfs(t)
% t - tone duration
co = 1;   % initial output channel
ci = 1;   % initial input channel
dev = device_number;
if (dev < 0) return; end
nm = arsc_mex('dev_name', dev);
fprintf('\nDevice name: %s\n\n',nm);
[vo,vi] = arsc_mex('get_vfs', dev);
if(round(vo(1))) po = sign(vo(1)); else po = 1; end % output polarity
if(round(vi(1))) pi = sign(vi(1)); else pi = 1; end % input polarity
while (1)
   fprintf('1 - Check D/A polarity\n');
   fprintf('2 - Check A/D polarity\n');
   fprintf('3 - Check D/A volts-full-scale\n');
   fprintf('4 - Check A/D volts-full-scale\n');
   fprintf('5 - Save souncard info\n');
   fprintf('6 - Quit\n');
   s = input('Select option: ');
   if (~isempty(s)&isnumeric(s))
      opt = s;
   elseif (isempty(s))
      opt = 0;
   else
      return
   end
   v = 1;  % target volts-rms
   switch opt
   case 0       % Do nothing
      fprintf('\n');
   case 1       % Check D/A polarity
      s = input(sprintf('Confirm D/A channel [%d]: ',co));
      if (~isempty(s)&isnumeric(s)) co = s; end
      a = sqrt(2) * abs(v / vo(co)); % amplitude (re full scale)
      fprintf('Observe D/A polarity on scope...\n');flush;
      round(polarity(dev,a,t,0.1,co,ci));
      s = input(sprintf('Confirm D/A polarity (1 or -1) [%d]: ',po));
      if (~isempty(s)&isnumeric(s)) po = s; end
      fprintf('\n  D/A polarity = %d\n\n',po);
   case 2       % Check A/D polarity
      s = input(sprintf('Confirm D/A channel [%d]: ',co));
      if (~isempty(s)&isnumeric(s)) co = s; end
      s = input(sprintf('Confirm A/D channel [%d]: ',ci));
      if (~isempty(s)&isnumeric(s)) ci = s; end
      a = sqrt(2) * abs(v / vo(co)); % amplitude (re full scale)
      fprintf('Measuring A/D polarity...\n');flush;
      r = round(polarity(dev,a,1,0.5,co,ci));
      if (r) pi = r; end
      s = input(sprintf('Confirm A/D polarity (1 or -1) [%d]: ',pi));
      if (~isempty(s)&isnumeric(s)) pi = s; end
      fprintf('\n  A/D polarity = %d\n\n',pi);
   case 3       % Set D/A vfs
      s = input(sprintf('Confirm D/A channel [%d]: ',co));
      if (~isempty(s)&isnumeric(s)) co = s; end
      a = sqrt(2) * abs(v / vo(co)); % amplitude (re full scale)
      if (a > 1) a = 1; v = abs(a * vo(co)) / sqrt(2); end
      fprintf('Observe D/A volts-rms on DVM...\n');flush;
      play_tone(dev,a,t,co,ci);
      s = input(sprintf('Confirm D/A channel %d volts-rms [%.3f]: ',co,v));
      if (~isempty(s)&isnumeric(s)) vrms = s; else vrms = v; end
      vo(co) = po * abs(vo(co) * vrms / v);
      fprintf('\n  D/A channel %d volts-full-scale = %.3f\n\n',co,vo(co));
   case 4       % Set A/D vfs
      s = input(sprintf('Confirm D/A channel [%d]: ',co));
      if (~isempty(s)&isnumeric(s)) co = s; end
      s = input(sprintf('Confirm A/D channel [%d]: ',ci));
      if (~isempty(s)&isnumeric(s)) ci = s; end
      a = sqrt(2) * abs(v / vo(co)); % amplitude (re full scale)
      if (a > 1) a = 1; v = abs(a * vo(co)) / sqrt(2); end
      fprintf('Observe A/D volts-rms on DVM...\n');flush;
      r = play_tone(dev,a,t,co,ci);
      s = input(sprintf('Confirm A/D channel %d volts-rms [%.3f]: ',ci,v));
      if (~isempty(s)&isnumeric(s)) vrms = s; else vrms = v; end
      vfs = pi * abs(vo(co) * vrms / v) / r;
      if (vfs < 100)
         vi(ci) = vfs;
         fprintf('\n  A/D channel %d volts-full-scale = %.3f\n\n',ci,vi(ci));
      else
         fprintf('\n  A/D channel %d volts-rms too low.\n\n');
      end
   case 5       % Save soundard info
      tstnfo(dev,vo,vi);
      return
   otherwise    % Quit
      return
   end
end
return

% select device number from list
function dev = device_number
dev = arsc_mex('find_dev', 'sync');
nd = arsc_mex('num_devs');
for di = 0:(nd-1)
   if (di == dev) sel = '>'; else sel = ' '; end;
   fprintf('%s%2d: %s\n',sel,di,arsc_mex('dev_name', di));
end
s = input('Select device number: ');
if (~isempty(s)&isnumeric(s))
    dev = s;
end
if (dev<0|dev >= nd)
    fprintf('*** Invalid device number.\n');
    dev = -1;
    return
end

function r = play_tone(di,a,t,co,ci)
% di - device ID
% a  - output amplitude (re full scale)
% t  - output duration (s)
% co - output channel
% ci - input channel
% r  - ratio of response to stimulus amplitude
sr = 32000;            % sampling rate
nc = 2;                % number of channels
f = 1000;              % tone frequency
d = 0.01;              % ramp duration
s = tone(a,f,t,sr,d);  % generate tone signal
[co,oo] = chan_pair(co);
[ci,oi] = chan_pair(ci);
st = zeros(length(s),nc);
st(:,co) = s;
rs = system_response(di, st, 1, 0, sr, nc, oi, oo);
if (nargout > 0)
   r = rs(:,ci);
   r = rms(r)/rms(s);
end
return

% reduce channel number by pairwise offset
function [c,o] = chan_pair(c)
o = 0;
while (c > 2)
   c = c - 2;
   o = o + 2;
end
return

% calculate rms
function y = rms(x)
y = sqrt(mean(mean(x.^2)));
return

function flush
if (isoctave)
   fflush(stdout);
end
return

function o = isoctave
o = 1;eval('OCTAVE_VERSION;','o=0;');
return

% generate tone
function s = tone(a,f,t,r,d)
n = round(r*t);                 % array length
s = a*sin(2*pi*f*t*(1:n)/n);    % unramped tone
m = round(r * d);               % ramp length
if (m>1 & m<n)                  % apply ramp ?
   i = 0:(m-1);
   j = 1 + i;
   k = n - i;
   w(j) = (1-cos(pi*i/m))/2;    % ramp
   s(j) = s(j).*w(j);           % ramp up
   s(k) = s(k).*w(j);           % ramp down
end
s = s(:);                       % retrun column vector
return

% measure system response using synchronous average
function rs = system_response(di, st, na, sk, sr, nc, oi, oo)
% di - device ID
% st - multi-channel stimulus waveform
% na - number of repetitions
% sk - number of initial samples to skip
% sr - sampling rate
% nc - number of channels
% oi - input channel offset
% oo - output channel offset
% rs - average system response
arsc_mex('io_open', di, sr, nc, nc, oi, oo);
rs = arsc_mex('sync_avg', di, st, na, sk);
arsc_mex('io_close', di);
return

function r = polarity(di,a,t,dc,co,ci)
% di - device ID
% a  - output amplitude (re full scale)
% t  - output duration (s)
% dc - duty cycle
% co - output channel
% ci - input channel
% r  - correlation of response and stimulus
sr = 32000;            % sampling rate (Hz)
nc = 2;                % number of channels
p = 0.1;               % period (s)
s = square(a,p,round(t/p),dc,sr);
[co,oo] = chan_pair(co);
[ci,oi] = chan_pair(ci);
st = zeros(length(s),nc);
st(:,co) = s;
rs = system_response(di, st, 1, 0, sr, nc, oi, oo);
if (nargout > 0)
   r = rs(:,ci);
   r = sum(r.*s) / sqrt(sum(r.*r)*sum(s.*s));
end
return

function s = square(a,p,n,d,sr)
% a  - amplitude
% p  - period (s)
% n  - number of periods
% d  - duty cycle
% sr - sampling rate (Hz)
n1 = round(sr*p*d);
n2 = round(sr*p*(1-d));
s = a * [ones(n1,1);-ones(n2,1)];
m = length(s);
i = 1:m;
for k = 1:(n-1)
   s(i+k*m) = s(i);
end
return
