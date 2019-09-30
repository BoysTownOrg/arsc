% tstlst.m - list i/o devices

function tstlst
fprintf('%s\n',arsc_mex('version'));
dvid = arsc_mex('find_dev', 'sync');
nd = arsc_mex('num_devs');
for di=0:(nd-1)
   if (di == dvid) sel = '>'; else sel = ' '; end;
   fprintf('%s%2d: %s\n',sel,di,arsc_mex('dev_name', di));
end
