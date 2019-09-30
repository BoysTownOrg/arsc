%
% ARSC_MEX  - Auditory Research SoundCard functions:
%
%  arsc_mex('close_all') closes all i/o devices opened by arsc_mex.
%
%  S=arsc_mex('dev_name',D) returns the name of the i/o device D.
%
%  D=arsc_mex('find_dev') returns device number of the default i/o device.
%  D=arsc_mex('find_dev',P) returns device number of the i/o device with
%     property P, which may be 'sync', 'asio', or 'alsa'.
%
%  D=arsc_mex('find_dev_name',N) returns device number of the i/o device
%     with name N.
%
%  C=arsc_mex('get_cardinfo',D) returns soundcard info for device D.
%
%  F=arsc_mex('get_fmt',D) returns device data format of device D.
%
%  R=arsc_mex('get_rate',D) returns sampling rate of device D.
%
%  [VO,VI]=arsc_mex('get_vfs',D) returns the full-scale voltages of device D.
%     VO and VI are arrays with length corresponding to the number of
%     of i/o channels.
%
%  arsc_mex('io_close',D) close device D.
%
%  N=arsc_mex('io_cur_seg',D) returns segment number currently being
%     processed by device D.
%
%  arsc_mex('io_open',D,R,NI,NO) opens device D for i/o with sampling
%     rate R, NI channels input, and NO channels output.
%
%  arsc_mex('io_open',D,R,NI,NO,OI,OO) opens device D for i/o with sampling
%     rate R, NI channels input, NO channels output, channel input offset OI,
%     and channel output offset OO.
%
%  arsc_mex('io_prepare',D,DI,DO,NA) prepares i/o data for device D.
%     DI and DO are cell arrays containing data arrays* for each i/o
%     segments. The number of rows in each data array equals the number 
%     of i/o samples in the corresponding segment. The number of columns in
%     each data array must equal the number of channels specified by the
%     io_open function. When started, NA sweeps (through all segments)
%     will be performed.
%
%  arsc_mex('io_start',D) start i/o on device D.
%
%  arsc_mex('io_stop',D) stop i/o on device D.
%
%  arsc_mex('io_wait_seg',D) wait until end of current segment on device D.
%
%  arsc_mex('num_devs') returns the number of devices avaiable for i/o.
%
%  arsc_mex('out_open',D,R,NO) opens device D for output (only) with
%     sampling rate R and NO channels output.
%
%  arsc_mex('out_prepare',D,DO,NA) prepares output data for device D.
%     DO is a cell array containing data arrays* for each i/o segment.
%     The number of colums in each data array equals the number of output
%     samples in the corresponding segment. The number of columns in
%     each data array must equal the number of channels specified by the
%     out_open function. When started, NA sweeps (through all segments)
%     will be performed.
%
%  arsc_mex('out_seg_fill',D) causes the output xfer function for device D
%     (specified by set_xfer) to be recalled for each pending
%     segment.
%
%  arsc_mex('set_xfer',D,FI,FO) specifies for device D the names of
%     M files to be called at the time that each i/o segment is processed.
%
%  arsc_mex('set_vfs',D,VI,VO) specifes the full-scale i/o voltages for 
%     device D. VI and VO are arrays and should contain values for
%     each i/o channel.
%
%  Y=arsc_mex('sync_avg',D,X,NA,SK) returns the synchronous-average response
%     Y to NA repetitions of a stimulus X using device D. Both X & Y are 
%     scaled so that 1 represents the full-scale voltage of the DAC & ADC,
%     respectively. SK specifies samples skipped.
%
%  arsc_mex('version') returns the version of the ARSC function library.
%
%  arsc_mex('xruns',D) returns the number of overruns and underrruns for
%     device number N.
%
% *Input/Output arrays may be either Int32 or Double.
%     Maximum voltage output when Int32 is 2^31-1.
%     Maximum voltage output when Double is 1.0.
%     Streaming I/O is better with Int32.
%
