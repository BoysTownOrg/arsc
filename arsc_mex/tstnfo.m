% tstnfo.m - write soundcard info to file
%
function tstnfo(dev,vo,vi)
fprintf('%s\n\n',arsc_mex('version'));
if (nargin < 3)
   [nm,ci,vo,vi]=card_input;
else
   [nm,ci,vo,vi]=card_input(dev,vo,vi);
end
if (isunix)
   fn = write_rc(nm,ci,vo,vi);
else
   fn = write_reg(nm,ci,vo,vi);
end
fprintf('\nWrote soundcard info to file: %s.\n',fn);
return

function [nm,ci,vo,vi]=card_input(dev,vo,vi)
if (nargin < 3)
   dev = device_number;
   if (dev < 0) return; end
   [vo,vi]=arsc_mex('get_vfs', dev);
end
nm=arsc_mex('dev_name', dev);
ci=arsc_mex('get_cardinfo', dev);
s = input(sprintf('Device name ''%s'': ',nm));
if (~isempty(s)) nm = s; end
s = input(sprintf('Bits of converter resolution [%d]: ',ci(1)));
if (~isempty(s)&isnumeric(s)) ci(1) = s; end
s = input(sprintf('Bits sample shifted left [%d]: ',    ci(2)));
if (~isempty(s)&isnumeric(s)) ci(2) = s; end
s = input(sprintf('Bytes per sample [%d]: ',            ci(3)));
if (~isempty(s)&isnumeric(s)) ci(3) = s; end
s = input(sprintf('Channels of A/D [%d]: ',             ci(4)));
if (~isempty(s)&isnumeric(s)) ci(4) = s; end
s = input(sprintf('Channels of D/A [%d]: ',             ci(5)));
if (~isempty(s)&isnumeric(s)) ci(5) = s; end
ci(6) = 0;
if (length(vi) ~= ci(4))
   vi = zeros(1,ci(4));
   vi(1) = 2.5;
end
if (length(vo) ~= ci(5))
   vo = zeros(1,ci(5));
   vo(1) = 2.5;
end
u = 'full-scale volts';
for i=1:length(vi)
   s = input(sprintf('A/D Channel %d %s =[%6.3f]: ',i,u,vi(i)));
   if (~isempty(s)&isnumeric(s)) vi(i) = s; end
   if (vi(i) == 0) break; end
end
for i=1:length(vo)
   s = input(sprintf('D/A Channel %d %s =[%6.3f]: ',i,u,vo(i)));
   if (~isempty(s)&isnumeric(s)) vo(i) = s; end
   if (vo(i) == 0) break; end
end
return

% select device number from list
function dev=device_number
dev = arsc_mex('find_dev', 'sync');
nd = arsc_mex('num_devs');
for di=0:(nd-1)
   if (di == dev) sel = '>'; else sel = ' '; end;
   fprintf('%s%2d: %s\n',sel,di,arsc_mex('dev_name', di));
end
s = input('Select device number: ');
if (~isempty(s)&isnumeric(s))
    dev = s;
end
if (dev<0|dev>=nd)
    fprintf('*** Invalid device number.\n');
    dev = -1;
    return
end

function fn=write_rc(nm1,ci1,vo1,vi1)
fn='arscrc';
fp = fopen(fn, 'wt');
fprintf(fp, '# arscrc - ARSC card info\n\n', fp);
[nm0,ci0,vo0,vi0]=card_default;
card_rc(fp,0,nm0,ci0,vo0,vi0);
card_rc(fp,1,nm1,ci1,vo1,vi1);
fclose(fp);
return

function fn=write_reg(nm1,ci1,vo1,vi1)
[nm0,ci0,vo0,vi0]=card_default;
fn='arsc.reg';
fp = fopen(fn, 'wt');
fprintf(fp, 'REGEDIT4\n\n', fp);
fprintf(fp, '[HKEY_LOCAL_MACHINE\\SOFTWARE\\BTNRH]\n\n');
fprintf(fp, '[HKEY_LOCAL_MACHINE\\SOFTWARE\\BTNRH\\ARSC]\n\n');
fprintf(fp, '[HKEY_LOCAL_MACHINE\\SOFTWARE\\BTNRH\\ARSC\\CardInfo]\n\n');
[nm0,ci0,vo0,vi0]=card_default;
card_reg(fp,0,nm0,ci0,vo0,vi0);
card_reg(fp,1,nm1,ci1,vo1,vi1);
fclose(fp);
return

function [nm,ci,vo,vi]=card_default
nm='Default';
ci=[16 0 2 2 2 0];
vo=[2.5 2.5];
vi=[2.5 2.5];
return

function card_rc(fp,ct,nm,ci,vo,vi)
fprintf(fp, '[CardType%d]\n', ct);
fprintf(fp, 'Name=%s\n', nm);
fprintf(fp, 'bits=%d\n', ci(1));
fprintf(fp, 'left=%d\n', ci(2));
fprintf(fp, 'nbps=%d\n', ci(3));
fprintf(fp, 'ncad=%d\n', ci(4));
fprintf(fp, 'ncda=%d\n', ci(5));
fprintf(fp, 'gdsr=%08X\n', ci(6));
fprintf(fp, 'ad_mv_fs=');
for i=1:length(vi)
   if (vi(i) == 0)
      break;
   end
   vfs = vi(i) * 1000;
   fprintf(fp, ' %5d', vfs);
end
fprintf(fp, '\n');
fprintf(fp, 'da_mv_fs=');
for i=1:length(vo)
   if (vo(i) == 0)
      break;
   end
   vfs = vo(i) * 1000;
   fprintf(fp, ' %5d', vfs);
end
fprintf(fp, '\n\n');
return

function card_reg(fp,ct,nm,ci,vo,vi)
fprintf(fp, '[HKEY_LOCAL_MACHINE\\SOFTWARE\\BTNRH\\ARSC\\CardInfo', fp);
fprintf(fp, '\\CardType%d]\n', ct);
fprintf(fp, '"Name"="%s"\n', nm);
fprintf(fp, '"bits"=dword:%08X      ; %2d\n', ci(1), ci(1));
fprintf(fp, '"left"=dword:%08X\n', ci(2));
fprintf(fp, '"nbps"=dword:%08X\n', ci(3));
fprintf(fp, '"ncad"=dword:%08X\n', ci(4));
fprintf(fp, '"ncda"=dword:%08X\n', ci(5));
fprintf(fp, '"gdsr"=dword:%08X\n', ci(6));
for i=1:length(vi)
   if (vi(i) == 0)
      break;
   end
   vfs = vi(i) * 1000;
   vfsx = (vfs<0)*2^32 + vfs;
   fprintf(fp, 'ad%d_mv_fs=dword:%08X ; %5d\n', i, vfsx, vfs);
end
for i=1:length(vo)
   if (vo(i) == 0)
      break;
   end
   vfs = vo(i) * 1000;
   fprintf(fp, 'da%d_mv_fs=dword:%08X ; %5d\n', i, vfs, vfs);
end
fprintf(fp, '\n');
return
