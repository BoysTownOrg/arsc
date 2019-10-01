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

int32_t _ar_find_dev(		// Find device given preferences
    int32_t flags			// - hlongs about desired device
    );				// > device identifier

int32_t _ar_find_dev_name(		// Find device given name
    char *name			// - name of desired device
    );				// > device identifier

int32_t _ar_xruns(			// Get xrun count
    int32_t dev 			// - device identifier
    );				// > number of xruns

int32_t _ar_num_devs ();		// Find the number of devices

int32_t _ar_io_open(		// Open device for i/o
    int32_t dev,			// - device identifier
    double rate,		// - desired sampling rate
    int32_t in_chan,		// - desired input channels
    int32_t out_chan		// - desired output channels
    );				// > error code

int32_t _ar_io_open_off(		// Open device for i/o with channel offset
    int32_t dev,			// - device identifier
    double rate,		// - desired sampling rate
    int32_t in_chan,		// - desired input channels
    int32_t out_chan,		// - desired output channels
    int32_t chnoff_i,		// - desired input channel offset
    int32_t chnoff_o		// - desired output channel offset
    );				// > error code

int32_t _ar_io_close(		// Close device
    int32_t dev 			// - device identifier
    );				// > error code

int32_t _ar_io_prep(		// Prepare device for i/o
    int32_t dev, 			// - device identifier
    void *in_data[],		// - input data for each segment
    void *out_data[],		// - output data for each segment
    int32_t size[], 		// - size of each segment
    int32_t nseg,			// - number of segments
    int32_t tseg			// - total number of segments
    );				// > error code

int32_t _ar_io_start(		// Start i/o
    int32_t dev 			// - device identifier
    );				// > error code

int32_t _ar_io_stop(		// Stop i/o
    int32_t dev 			// - device identifier
    );				// > error code

int32_t _ar_set_fmt(		// Specify app's data format
    int32_t dev, 			// - device identifier
    int32_t *fmt 			// - data format
    );				// > error code

int32_t _ar_get_fmt(		// Get dev's data format
    int32_t dev, 			// - device identifier
    int32_t *fmt 			// - data format
    );				// > error code

int32_t _ar_get_gdsr(		// Get dev's good sampling rates
    int32_t dev 			// - device identifier
    );				// > bitwise good rates

int32_t _ar_set_xfer(		// Specify app data transfer functions
    int32_t dev, 			// - device identifier
    void (*in_xfer)(int32_t),	// - input transfer function
    void (*out_xfer)(int32_t)	// - output tranfer function
    );				// > error code

int32_t _ar_io_cur_seg(		// Get current unwrapped segment
    int32_t dev 			// - device identifier
    );				// > segment number

int32_t _ar_dev_name(		// Get device name
    int32_t dev,			// - device identifier
    char *name,			// name array
    int32_t len			// array length
    );				// > error code

void _ar_err_msg(		// Get error message
    int32_t dev,			// - device identifier
    char *msg,			// message array
    int32_t len			// array length
    );

double _ar_get_rate(		// Get sampling rate
    int32_t dev			// - device identifier
    );				// > sampling rate

int32_t _ar_adjust_rate(		// Adjust sampling rate
    int32_t dev,			// - device identifier
    double rate			// - desired rate
    );				// > closest rate

// float-sample functions

void _ar_get_sfs(	    // get i/o sample-full-scale
    int32_t dev,		    // - device identifier
    double *i_sfs,	    // - input sample-full-scale
    double *o_sfs	    // - output sample-full-scale
    );			    //

void _ar_set_sfs(	    // set i/o sample-full-scale
    int32_t dev,		    // - device identifier
    double *i_sfs,	    // - input sample-full-scale
    double *o_sfs	    // - output sample-full-scale
    );			    //

// vfs functions

void _ar_get_vfs(		// set convertor volts-full-scale
    int32_t dev,			// - device identifier
    double *da_vfs,		// DAC volts-full-scale
    double *ad_vfs		// ADC volts-full-scale
    );				// > error code

void _ar_set_vfs(		// set convertor volts-full-scale
    int32_t dev,			// - device identifier
    double *da_vfs, 		// DAC volts-full-scale
    double *ad_vfs		// ADC volts-full-scale
    );				// > error code

// misc functions

void _ar_close_all(		// Close all devices
    );				//

void _ar_wind(			// Specify a window to receive messages
    int32_t wind			// - handle to window
    );				//

int32_t _ar_set_latency(		// Set soundcard latency
    int32_t dev,			// - device identifier
    int32_t nsmp			// - number of samples
    );

// output functions

int32_t _ar_out_seg_fill(		// Refill output segments
    int32_t dev			// - device identifier
    );				//

// CARDINFO function

int32_t _ar_get_cardinfo(          // Get card info
    int32_t dev,                   // - device identifier
    CARDINFO *ci                // - pointer to CARDINFO structure
    );                          //
