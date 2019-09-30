function resp=sysres(npnt,navg,rate,nskp,ochn,ichn)
% resp = sysres(npnt,navg,rate,nskp,ochn,ichn)
%
% npnt - buffer size (or stimulus waveform) [4096]
% navg - buffer repetitions [50]
% rate - sampling rate (Hz) [48000]
% nskp - transient size [rate/10]
% ochn - output channel
% ichn - input channels (1 or 2) [1]
% resp - impulse response
%
% System-response transfer function plotted
%   when output argument omitted.

% fetch input arguments
if (nargin > 0)
   if (length(npnt)>2)
      [np,nc] = size(npnt);
      st = npnt(:,1);
   else
      np = npnt;
      st = wn(np,1);
   end
else
   np = 4096;
   st = wn(np,1);
end
if (nargin > 1)
   na = navg;
else
   na = 50;
end
if (nargin > 2)
   sr = rate;
else
   sr = 48000;
end
if (nargin > 3)
   sk = nskp;
else
   sk = round(sr / 10); % default skip = 0.1 s
end
if (nargin > 4)
   co = ochn;
else
   co = 1;
end
if (nargin > 5)
   ci = ichn;
else
   ci = 1;
end

% find i/o device
id = arsc_mex('find_dev', 'sync');

% measure system response
rs = system_response(id, st, na, sr, sk, co, ci);
[nr,nc]=size(rs);
for k=1:nc
   H(:,k) = ffa(rs(:,k))./ffa(st);    % transfer function
end
if (nargout > 0)
   resp = ffs(H);        % impulse response
else
   bode_plot(H,sr/2);
   fprintf('   i/o device = %s\n', arsc_mex('dev_name', id));
   fprintf('sampling rate = %d\n', arsc_mex('get_rate', id));
end
return

% generate white noise stimulus
function y=wn(n,c)
m=1+n/2;                % assume n is even
p=2*pi*rand(m,c);       % random phase
Y=exp(i*p);             % magnitude=1
y=ffs(Y)*10;            % 20-dB boost
return

% measure system response using synchronous average
function rs=system_response(id, st, na, sr, sk, co, ci)
% id - device id
% st - stimulus waveform
% na - number of repetitions
% sk - number of initial samples to skip
% sr - sampling rate
% co - output channel
% ci - input channel
% rs - average system response
nc = 2;
no = length(co);
ni = length(ci);
np = length(st);
s = zeros(np,nc);
rs = zeros(np,no*ni);
for i=1:no
   for j=1:ni
      s(:,co(i)) = st(:);
      arsc_mex('io_open', id, sr, nc);
      r = arsc_mex('sync_avg', id, s, na, sk);
      rs(:,i + no*(j-1)) = r(:,ci(j));
   end
end
arsc_mex('io_close', id);
return

% plot transfer function vs log frequency
function bode_plot(X,fs)
% X  - complex transfer function
% fs - maximum frequency in X
[nr,nc]=size(X);    % number of components
n=2:(nr-1);         % range to plot
f=fs*n/nr;          % frequency vector
fr=[fs/500 fs];     % frequency range
% gain (upper plot)
M=20*log10(max(abs(X(n,:)),1e-99));
subplot(2,1,1);
semilogx(f,M);
mx=max(max(M))+3;        % 3 dB above max gain
axis([fr mx-100 mx]);
ylabel('gain (dB)');
% phase (lower plot)
P=unwrap(angle(X(n,:)))/(2*pi);
subplot(2,1,2);
semilogx(f,P);
axis([fr -22 2]);
xlabel('frequency (Hz)');
ylabel('phase (cyc)');
return

% fast Fourier analyze real signal
function H=ffa(h)
H=fft(real(h));
n=length(H);
m=1+n/2;            % assume n is even
H(1,:)=real(H(1,:));
H(m,:)=real(H(m,:));
H((m+1):n,:)=[];    % remove upper frequencies
return

% fast Fourier synthesize real signal
function h=ffs(H)
m=length(H);
n=2*(m-1);
H(1,:)=real(H(1,:));
H(m,:)=real(H(m,:));
H((m+1):n,:)=conj(H((m-1):-1:2,:));
h=real(ifft(H));
return
