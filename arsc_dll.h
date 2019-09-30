/*
arsc_dll.h

This is an internal file to the DLL, only.  Not to be used in calling code.
*/

/*
These are internal prototypes for the DLL.  The list is made
by taking the prototypes in arsclib.h and prepending an underscore.
*/

char *_ar_version (		// Get ARSC version string
    );				// > version string

SINT4 _ar_find_dev(		// Find device given preferences
    SINT4 flags			// - hlongs about desired device
    );				// > device identifier

SINT4 _ar_find_dev_name(		// Find device given name
    char *name			// - name of desired device
    );				// > device identifier

SINT4 _ar_xruns(			// Get xrun count
    SINT4 dev 			// - device identifier
    );				// > number of xruns

SINT4 _ar_num_devs ();		// Find the number of devices

SINT4 _ar_io_open(		// Open device for i/o
    SINT4 dev,			// - device identifier
    double rate,		// - desired sampling rate
    SINT4 in_chan,		// - desired input channels
    SINT4 out_chan		// - desired output channels
    );				// > error code

SINT4 _ar_io_open_off(		// Open device for i/o with channel offset
    SINT4 dev,			// - device identifier
    double rate,		// - desired sampling rate
    SINT4 in_chan,		// - desired input channels
    SINT4 out_chan,		// - desired output channels
    SINT4 chnoff_i,		// - desired input channel offset
    SINT4 chnoff_o		// - desired output channel offset
    );				// > error code

SINT4 _ar_io_close(		// Close device
    SINT4 dev 			// - device identifier
    );				// > error code

SINT4 _ar_io_prep(		// Prepare device for i/o
    SINT4 dev, 			// - device identifier
    void *in_data[],		// - input data for each segment
    void *out_data[],		// - output data for each segment
    SINT4 size[], 		// - size of each segment
    SINT4 nseg,			// - number of segments
    SINT4 tseg			// - total number of segments
    );				// > error code

SINT4 _ar_io_start(		// Start i/o
    SINT4 dev 			// - device identifier
    );				// > error code

SINT4 _ar_io_stop(		// Stop i/o
    SINT4 dev 			// - device identifier
    );				// > error code

SINT4 _ar_set_fmt(		// Specify app's data format
    SINT4 dev, 			// - device identifier
    SINT4 *fmt 			// - data format
    );				// > error code

SINT4 _ar_get_fmt(		// Get dev's data format
    SINT4 dev, 			// - device identifier
    SINT4 *fmt 			// - data format
    );				// > error code

SINT4 _ar_get_gdsr(		// Get dev's good sampling rates
    SINT4 dev 			// - device identifier
    );				// > bitwise good rates

SINT4 _ar_set_xfer(		// Specify app data transfer functions
    SINT4 dev, 			// - device identifier
    void (*in_xfer)(SINT4),	// - input transfer function
    void (*out_xfer)(SINT4)	// - output tranfer function
    );				// > error code

SINT4 _ar_io_cur_seg(		// Get current unwrapped segment
    SINT4 dev 			// - device identifier
    );				// > segment number

SINT4 _ar_dev_name(		// Get device name
    SINT4 dev,			// - device identifier
    char *name,			// name array
    SINT4 len			// array length
    );				// > error code

void _ar_err_msg(		// Get error message
    SINT4 dev,			// - device identifier
    char *msg,			// message array
    SINT4 len			// array length
    );

double _ar_get_rate(		// Get sampling rate
    SINT4 dev			// - device identifier
    );				// > sampling rate

SINT4 _ar_adjust_rate(		// Adjust sampling rate
    SINT4 dev,			// - device identifier
    double rate			// - desired rate
    );				// > closest rate

// float-sample functions

void _ar_get_sfs(	    // get i/o sample-full-scale
    SINT4 dev,		    // - device identifier
    double *i_sfs,	    // - input sample-full-scale
    double *o_sfs	    // - output sample-full-scale
    );			    //

void _ar_set_sfs(	    // set i/o sample-full-scale
    SINT4 dev,		    // - device identifier
    double *i_sfs,	    // - input sample-full-scale
    double *o_sfs	    // - output sample-full-scale
    );			    //

// vfs functions

void _ar_get_vfs(		// set convertor volts-full-scale
    SINT4 dev,			// - device identifier
    double *da_vfs,		// DAC volts-full-scale
    double *ad_vfs		// ADC volts-full-scale
    );				// > error code

void _ar_set_vfs(		// set convertor volts-full-scale
    SINT4 dev,			// - device identifier
    double *da_vfs, 		// DAC volts-full-scale
    double *ad_vfs		// ADC volts-full-scale
    );				// > error code

// misc functions

void _ar_close_all(		// Close all devices
    );				//

void _ar_wind(			// Specify a window to receive messages
    SINT4 wind			// - handle to window
    );				//

SINT4 _ar_set_latency(		// Set soundcard latency
    SINT4 dev,			// - device identifier
    SINT4 nsmp			// - number of samples
    );

// output functions

SINT4 _ar_out_seg_fill(		// Refill output segments
    SINT4 dev			// - device identifier
    );				//

// CARDINFO function

SINT4 _ar_get_cardinfo(          // Get card info
    SINT4 dev,                   // - device identifier
    CARDINFO *ci                // - pointer to CARDINFO structure
    );                          //
