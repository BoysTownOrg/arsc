/* arsc_mac.c - soundcard functions for MacOS */

#include <CoreServices/CoreServices.h>
#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include "arscdev.h"
#include "strings.h"
#include "arsclib.h"

#define free_null(p)    if (p){free(p);p=NULL;}

void     _arsc_set_cardinfo(CARDINFO, int);

static int dio = 0;			// device identifier offset
static int nd = 0;			// number of devices
AudioBufferList *mInputBuffer = NULL;
static AudioDeviceID *adid = NULL;
static AudioUnit gOutputUnit;

static char      AU_open = 0;
static char      AU_prep = 0;
static char      AU_strt = 0;
static AudioUnit gOutputUnit = {0};

/**************************************************************************/
static CARDINFO card_info[] = {
    { "Default",	     16, 0, 2, 2, 2, 0, { 2.660}, { 2.560} },
    { "Babyface",            24, 0, 4, 2, 2, 0, { 1.600}, { 2.000} },
};

/* trim - remove whitespace from end of string */

static void 
trim(char *s)
{
    char *e;

    for (e = s; *e; e++)
	continue;
    while (e > s && e[-1] <= ' ')
	e--;
    *e = '\0';
}

/* atoh - ASCII to hexidecimal */

static int 
atoh(char *s)
{
    int i = 0;

    while (*s && *s <= ' ')	/* skip whitespace */
	s++;
    while (*s) {
	if (*s > '0' && *s < '9') {
	    i = 16 * i + *s - '0';
	} else if (*s >= 'A' && *s <= 'F') {
	    i = 16 * i + 10 + *s - 'A';
	} else if (*s >= 'a' && *s <= 'f') {
	    i = 16 * i + 10 + *s - 'a';
	}
	s++;
    }

    return (i);
}

/* getmv - get list of mvfs */

static void 
getmv(char *s, double *v)
{
    int ch, mv = 0;

    for (ch = 0; ch < MAXNCH; ch++) {
	if (*s) {
	    while (*s && *s <= ' ')	/* skip whitespace */
		s++;
	    mv = atoi(s);
	    while (*s > ' ')
		s++;
	}
	v[ch] = mv * 0.001;
    }
}

/* read_rc - read cardinfo file */

static void 
read_rc()
{
    char s[80];
    int i = -1;
    CARDINFO ci;
    FILE *fp;
    static char *fn = "/usr/local/etc/arscrc";
    void _arsc_set_cardinfo(CARDINFO, int);
    static CARDINFO zci = {{0},0,0,0,0,0,0,{0},{0}};

    fp = fopen(fn, "r");
    if (fp == NULL) {
	return;
    }
    while (fgets(s, 80, fp) != NULL) {
	trim(s);
	if (strncasecmp(s, "[CardType", 9) == 0) {
	    i = atoi(s + 9);
	    ci = zci;
	} else if (strncasecmp(s, "Name", 4) == 0) {
	    strncpy(ci.name, s + 5, MAX_CT_NAME);
	} else if (strncasecmp(s, "bits", 4) == 0) {
	    ci.bits = atoi(s + 5);
	} else if (strncasecmp(s, "left", 4) == 0) {
	    ci.left = atoi(s + 5);
	} else if (strncasecmp(s, "nbps", 4) == 0) {
	    ci.nbps = atoi(s + 5);
	} else if (strncasecmp(s, "ncad", 4) == 0) {
	    ci.ncad = atoi(s + 5);
	} else if (strncasecmp(s, "ncda", 4) == 0) {
	    ci.ncda = atoi(s + 5);
	} else if (strncasecmp(s, "gdsr", 4) == 0) {
	    ci.gdsr = atoh(s + 5);
	} else if (strncasecmp(s, "ad_mv_fs", 8) == 0) {
	    getmv(s + 9, ci.ad_vfs);
	} else if (strncasecmp(s, "da_mv_fs", 8) == 0) {
	    getmv(s + 9, ci.da_vfs);
	} else if (i >= 0 && i < MAXNCT) {
	    _arsc_set_cardinfo(ci, i);
	    i = -1;
	}
    }
    fclose(fp);
} 

static void 
set_cardinfo()
{
    int ct, nct = sizeof(card_info) / sizeof(card_info[0]);

    for (ct = 0; ct < nct; ct++) {
        _arsc_set_cardinfo(card_info[ct], ct);
    }
    read_rc();
}

/***************************************************************************/

int chk_err(int err, const char *msg)
{
    if (err) {
        printf("Error (%d): %s\n", err, msg);
    }
    return (err);
}

/***************************************************************************/

static char *q_idone = NULL;
static char *q_odone = NULL;
static void **q_idata = NULL;
static void **q_odata = NULL;
static int32_t q_oseg, q_iseg, q_ocnt, q_icnt, q_ncda, q_ncad;
static int32_t q_nseg, *q_size = NULL;

/* q_init - initialize queuing */

void q_init(void *odata[], void *idata[], int32_t *size, int ncda, int ncad, int nseg)
{
    int32_t i;

    free_null(q_odata);
    free_null(q_idata);
    free_null(q_size);
    free_null(q_odone);
    free_null(q_idone);
    q_odone = (char *) calloc(nseg, sizeof(char));
    q_idone = (char *) calloc(nseg, sizeof(char));
    q_size = (int32_t *) calloc(nseg, sizeof(int32_t));
    q_odata = (void **) calloc(nseg * ncda, sizeof(void *));
    q_idata = (void **) calloc(nseg * ncad, sizeof(void *));
    q_ncda = ncda;
    q_ncad = ncad;
    q_nseg = nseg;
    for (i = 0; i < nseg * ncda; i++) {
        q_odata[i] = odata[i];
    }
    for (i = 0; i < nseg * ncad; i++) {
        q_idata[i] = idata[i];
    }
    for (i = 0; i < nseg; i++) {
	q_size[i] = size[i];
	q_odone[i] = 1;
    }
    q_oseg = q_ocnt = 0;
    q_iseg = q_icnt = 0;
}

/* q_put - output data to AudioUnit from queue */

void q_put(AudioBufferList *ioData, int nfr)
{
    int32_t i, j, k, m, n, o = 0, nch, *d;
    UInt32 *c;

    nch = ioData ? ioData->mNumberBuffers : q_ncda;
    for (k = 0; k < nfr; k += o) {
        if (q_odone[q_oseg]) {
            o = nfr - k;
            for (j = 0; j < nch; j++) {
                c = (UInt32 *) ioData->mBuffers[j].mData + k;
                for (i = 0; i < o; i++) {
                    c[i] = 0;
                }
            }
        } else {
            m = q_size[q_oseg] - q_ocnt;
            n = nfr - k;
            o = (n < m) ? n : m;
            for (j = 0; j < nch; j++) {
                c = (UInt32 *) ioData->mBuffers[j].mData + k;
                d = (int32_t *) q_odata[q_oseg * q_ncda + j] + q_ocnt;
                for (i = 0; i < o; i++) {
                    c[i] = (UInt32) d[i] >> 8;
                }
            }
            if (o < m) {
                q_ocnt += o;
            } else {
                q_odone[q_oseg] = 1;
                q_oseg = (q_oseg + 1) % q_nseg;
                q_ocnt = 0;
            }
        }
    }
}

/* q_get - input data from AudioUnit to queue */

void q_get(AudioBufferList *ioData, int nfr)
{
    int32_t i, j, k, m, n, o = 0, nch, *d;
    UInt32 *c;
    
    nch = ioData ? ioData->mNumberBuffers : q_ncad;
    if (!q_idone[q_iseg]) {
        for (k = 0; k < nfr; k += o) {
            m = q_size[q_iseg] - q_icnt;
            n = nfr - k;
            o = (n < m) ? n : m;
            for (j = 0; j < nch; j++) {
                d = (int32_t *) q_idata[q_iseg * q_ncad + j];
                if (!d) continue;
                d += q_icnt;
                if (ioData) {
                    c = (UInt32 *) ioData->mBuffers[j].mData + k;
                    for (i = 0; i < o; i++) {
                        d[i] = (int32_t) c[i] << 8;
                    }
                } else {
                    for (i = 0; i < o; i++) {
                        d[i] = 0;
                    }
                }
            }
            if (o < m) {
                q_icnt += o;
            } else {
                q_idone[q_iseg] = 1;
                q_iseg = (q_iseg + 1) % q_nseg;
                q_icnt = 0;
            }
        }
    }
}

/* q_done - stop outputing data from queue */

void q_done()
{
    int i;
    
    for (i = 0; i < q_nseg; i++) {
        q_odone[i] = 1;
        q_idone[i] = 1;
    }
}

/* q_free - free memory allocated for queue */

void q_free()
{
    free_null(q_odata);
    free_null(q_idata);
    free_null(q_size);
    free_null(q_odone);
    free_null(q_idone);
}

/***************************************************************************/

OSStatus myOutputProc(
    void 			*inRefCon,
    AudioUnitRenderActionFlags 	*ioActionFlags,
    const AudioTimeStamp 	*inTimeStamp,
    UInt32 			inBusNumber,
    UInt32 			inNumberFrames,
    AudioBufferList 		*ioData)
{
    q_put(ioData, inNumberFrames);
    return noErr;
}

OSStatus myInputProc(
    void 			*inRefCon,
    AudioUnitRenderActionFlags 	*ioActionFlags,
    const AudioTimeStamp 	*inTimeStamp,
    UInt32 			inBusNumber,
    UInt32 			inNumberFrames,
    AudioBufferList 		*ioData)
{
    OSStatus err = 0;

    //Get the new audio data
    err = AudioUnitRender(gOutputUnit,
                          ioActionFlags,
                          inTimeStamp,
                          inBusNumber,
                          inNumberFrames,
                          mInputBuffer);// Audio Buffer List to hold data
    chk_err(err, "Render Input Buffer");
    q_get(mInputBuffer, inNumberFrames);
    return noErr;
}

/***************************************************************************/

/* num_dev - return number of devices */

static int32_t
num_dev()
{
    AudioObjectID aoid;
    AudioObjectPropertyAddress propaddr;
    UInt32 sz;

    if (adid == NULL) {
	aoid = kAudioObjectSystemObject;
	propaddr.mSelector = kAudioHardwarePropertyDevices;
	propaddr.mScope = kAudioObjectClassID;
	propaddr.mElement = 0;
	AudioObjectGetPropertyDataSize(aoid, &propaddr, 0, NULL, &sz);
	adid = (AudioDeviceID *) malloc(sz);
	AudioObjectGetPropertyData(aoid, &propaddr, 0, NULL, &sz, adid);
	nd = sz / sizeof(AudioDeviceID);
    }

    return (nd);
}

/* dev_name - return name of I/O device */

static char *
dev_name(int32_t di)
{
    int id;
    AudioObjectPropertyAddress propaddr;
    UInt32 sz;
    static char name[80];

    propaddr.mSelector = kAudioDevicePropertyDeviceName;
    propaddr.mScope = kAudioObjectClassID;
    propaddr.mElement = 0;
    id = adid[di];
    AudioObjectGetPropertyDataSize(id, &propaddr, 0, NULL, &sz);
    AudioObjectGetPropertyData(id, &propaddr, 0, NULL, &sz, name);

    return (name);
}

/* def_name - return name of default output device */

static const char *
def_name()
{
    const char * theDefaultOutputDeviceString = NULL;
    AudioDeviceID theDefaultOutputDeviceID;
    AudioDeviceID *theDeviceList = NULL;
    AudioObjectPropertyAddress propaddr = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster };
    CFStringRef theDefaultOutputDeviceName = NULL;
    OSStatus result = 0;
    UInt32 propsize;

    // get the ID of the default output device
    propsize = sizeof(theDefaultOutputDeviceID);
    result = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                        &propaddr,
                                        0,
                                        NULL,
                                        &propsize,
                                        &theDefaultOutputDeviceID);
    if (chk_err(result, "AudioObjectGetPropertyData - DefaultOutputDeviceID")) goto end;

    propsize = sizeof(CFStringRef);
    propaddr.mSelector = kAudioObjectPropertyName;
    propaddr.mScope = kAudioObjectPropertyScopeGlobal;
    propaddr.mElement = kAudioObjectPropertyElementMaster;

    // get the name of the default output device
    result = AudioObjectGetPropertyData(theDefaultOutputDeviceID,
                                        &propaddr,
                                        0,
                                        NULL,
                                        &propsize,
                                        &theDefaultOutputDeviceName);
    if (chk_err(result, "AudioObjectGetPropertyData - DefaultOutputDeviceName")) goto end;
    theDefaultOutputDeviceString =
        CFStringGetCStringPtr(theDefaultOutputDeviceName,
        CFStringGetSystemEncoding());

end:
    if (theDefaultOutputDeviceName) {
        CFRelease(theDefaultOutputDeviceName);
    }
    if (theDeviceList) {
        free(theDeviceList);
    }

    return theDefaultOutputDeviceString;
}

/* def_dev - return index of default output device */

static int
def_dev()
{
    const char *dfnm;
    char *dvnm;
    int i, n;
    
    n = (int) num_dev();
    dfnm = def_name();
    for (i = 0; i < nd; i++) {
        dvnm = dev_name(i);
        if (strcmp(dvnm, dfnm) == 0) {
            return (i);
        }
    }
    
    return (-1);
}

/***************************************************************************/

void dev_free_buffers()
{
    UInt32 i;

    if (mInputBuffer) {
        for (i = 0; i< mInputBuffer->mNumberBuffers; i++) {
            free(mInputBuffer->mBuffers[i].mData);
        }
        free(mInputBuffer);
        mInputBuffer = NULL;
    }
}

void dev_close()
{
    OSStatus err = 0;

    if (!AU_open) {
        printf("dev_close: device not open\n");
    } else {
        err = AudioUnitUninitialize(gOutputUnit);
        chk_err(err, "AudioUnitUninitialize");
        AudioComponentInstanceDispose(gOutputUnit);
        chk_err(err, "AudioUnitDispose");
        dev_free_buffers();
        AU_open = 0;
        AU_prep = 0;
    }
}

int dev_open(int32_t di)
{
    AudioComponent comp;
    AudioComponentDescription desc;
    AURenderCallbackStruct callback;
    AudioUnitScope outputBus = 0;
    AudioUnitScope inputBus = 1;
    OSStatus err = noErr;
    UInt32 enableFlag = 1;

    if (AU_open) {
        //printf("dev_open: device already open\n");
        dev_close();
    }
    // Open the output unit
    desc.componentType = kAudioUnitType_Output;

    //desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentSubType = kAudioUnitSubType_HALOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    // Finds a component that meets the desc spec's
    comp = AudioComponentFindNext(NULL, &desc);
    chk_err(comp == NULL, "AudioComponentFindNext");

    // gains access to the services provided by the component
    err = AudioComponentInstanceNew(comp, &gOutputUnit);
    chk_err(err, "OpenAComponent");

    // AUHAL needs to be initialized before anything is done to it
    err = AudioUnitInitialize(gOutputUnit);
    chk_err(err, "AudioUnitInitialize");

    if (_ardev[di]->ncda) {
        // enable output
        err = AudioUnitSetProperty(gOutputUnit,
                                   kAudioOutputUnitProperty_EnableIO,
                                   kAudioUnitScope_Output,
                                   outputBus,
                                   &enableFlag,
                                   sizeof(enableFlag));
        chk_err(err, "Couldn't enable output on I/O unit");
        
        // Set up a callback function to generate output to the output unit
        callback.inputProc = myOutputProc;
        callback.inputProcRefCon = NULL;
        err = AudioUnitSetProperty(gOutputUnit,
                                   kAudioUnitProperty_SetRenderCallback,
                                   kAudioUnitScope_Output,
                                   outputBus,
                                   &callback,
                                   sizeof(callback));
        chk_err(err, "AudioUnitSetProperty-OutputCallBack");
    }

    if (_ardev[di]->ncad) {
        // enable input
        err = AudioUnitSetProperty(gOutputUnit,
                                   kAudioOutputUnitProperty_EnableIO,
                                   kAudioUnitScope_Input,
                                   inputBus,
                                   &enableFlag,
                                   sizeof(enableFlag));
        chk_err(err, "Couldn't enable input on I/O unit");

        // Set up a callback function to retrive input from the output unit
        callback.inputProc = myInputProc;
        callback.inputProcRefCon = NULL;
        err = AudioUnitSetProperty(gOutputUnit,
                                   kAudioOutputUnitProperty_SetInputCallback,
                                   kAudioUnitScope_Output,
                                   inputBus,
                                   &callback,
                                   sizeof(callback));
        chk_err(err, "AudioUnitSetProperty-InputCallBack");
    }

    // set di as the current device

    err = AudioUnitSetProperty(gOutputUnit, 
                               kAudioOutputUnitProperty_CurrentDevice,
                               kAudioUnitScope_Global,
                               outputBus,
                               &adid[di],
                               sizeof(AudioDeviceID));
    if (chk_err(err, "AudioUnitSetProperty-CurentDevice")) return (err);
    
    AU_open = 1;
    AU_prep = 0;
    return (0);
}

void dev_prep(int32_t di)
{
    AudioStreamBasicDescription streamFormat;
    Float64 outputSampleRate;
    OSStatus err = noErr;
    UInt32 i, bufferSizeFrames, bufferSizeBytes, propsize;

    if (AU_prep) {
        //printf("dev_prep: device already prepared\n");
    }
    streamFormat.mSampleRate = _ardev[di]->rate;
    streamFormat.mFormatID = kAudioFormatLinearPCM;
    streamFormat.mFramesPerPacket = 1;
    streamFormat.mBytesPerPacket = 4;
    streamFormat.mBytesPerFrame = 4;
    streamFormat.mChannelsPerFrame = 2;
    streamFormat.mBitsPerChannel = 24;
    streamFormat.mFormatFlags = kAudioFormatFlagIsSignedInteger
        | kAudioFormatFlagsNativeEndian
        | kAudioFormatFlagIsNonInterleaved;

    err = AudioUnitSetProperty(gOutputUnit,
                               kAudioUnitProperty_StreamFormat,
                               kAudioUnitScope_Input,
                               0,
                               &streamFormat,
                               sizeof(AudioStreamBasicDescription));
    chk_err(err, "AudioUnitSetProperty-OutputStreamFormat");

    err = AudioUnitSetProperty(gOutputUnit,
                               kAudioUnitProperty_StreamFormat,
                               kAudioUnitScope_Output,
                               1,
                               &streamFormat,
                               sizeof(AudioStreamBasicDescription));
    chk_err(err, "AudioUnitSetProperty-InputStreamFormat");

    propsize = sizeof(Float64);
    err = AudioUnitGetProperty(gOutputUnit,
                               kAudioUnitProperty_SampleRate,
                               kAudioUnitScope_Input,
                               0,
                               &outputSampleRate,
                               &propsize);
    chk_err(err, "AudioUnitGetProperty-OutputSampleRate");
    
    if (_ardev[di]->ncad) {
        //Get the size of the IO buffer(s)
        propsize = sizeof(bufferSizeFrames);
        err = AudioUnitGetProperty(gOutputUnit,
                                   kAudioDevicePropertyBufferFrameSize,
                                   kAudioUnitScope_Global,
                                   0,
                                   &bufferSizeFrames,
                                   &propsize);
        chk_err(err, "AudioUnitGetProperty-BufferFrameSize");
        // malloc buffer lists
        bufferSizeBytes = bufferSizeFrames * sizeof(Float32);
        if (streamFormat.mFormatFlags & kAudioFormatFlagIsNonInterleaved) {
            //printf ("format is non-interleaved\n");
            propsize = offsetof(AudioBufferList, mBuffers[0])
            + (sizeof(AudioBuffer) * streamFormat.mChannelsPerFrame);
            mInputBuffer = (AudioBufferList *) malloc(propsize);
            mInputBuffer->mNumberBuffers = streamFormat.mChannelsPerFrame;
            for (i =0; i< mInputBuffer->mNumberBuffers ; i++) {
                mInputBuffer->mBuffers[i].mNumberChannels = 1;
                mInputBuffer->mBuffers[i].mDataByteSize = bufferSizeBytes;
                mInputBuffer->mBuffers[i].mData = malloc(bufferSizeBytes);
            }
        } else {
            //printf ("format is interleaved\n");
            propsize = offsetof(AudioBufferList, mBuffers[0])
            + (sizeof(AudioBuffer) * 1);
            mInputBuffer = (AudioBufferList *) malloc(propsize);
            mInputBuffer->mNumberBuffers = 1;
            mInputBuffer->mBuffers[0].mNumberChannels = streamFormat.mChannelsPerFrame;
            mInputBuffer->mBuffers[0].mDataByteSize = bufferSizeBytes;
            mInputBuffer->mBuffers[0].mData = malloc(bufferSizeBytes);
        }
    }

    AU_prep = 1;
}

void
dev_start()
{
    OSStatus err;

    if (!AU_open) {
        printf("dev_start: device not open\n");
        exit(1);
    }
    if (!AU_prep) {
        printf("dev_start: device not open\n");
        exit(1);
    }
    if (AU_strt) {
        printf("dev_start: device already started\n");
    } else {
        err = AudioOutputUnitStart(gOutputUnit);
        chk_err(err, "AudioOutputUnitStart");
        AU_strt = 1;
    }
}

void
dev_stop()
{
    OSStatus err = 0;

    if (AU_strt) {
        q_done();
        err = AudioOutputUnitStop(gOutputUnit);
        chk_err(err, "AudioOutputUnitStop");
        AU_strt = 0;
    }
}

void
dev_gdsr(int32_t di, int32_t *rate, int32_t *nr)
{
    int i, n, num_rates = 0;
    AudioObjectPropertyAddress propaddr = {
        kAudioDevicePropertyAvailableNominalSampleRates,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster };
    AudioValueRange *avr;
    OSStatus result = 0;
    UInt32 propsize = sizeof(Float64);

    // check whether device supports getting available sample rates
    if (AudioObjectHasProperty(adid[di],
                               &propaddr)) {
        result = AudioObjectGetPropertyDataSize(adid[di],
                                            &propaddr,
                                            0,
                                            NULL,
                                            &propsize);
        chk_err(result, "dev_gdsr - AudioObjectGetPropertyDataSize");
        num_rates = propsize / sizeof(AudioValueRange);
    }
    if (num_rates) {
        avr = (AudioValueRange *) malloc(propsize);
        result = AudioObjectGetPropertyData(adid[di],
                                                &propaddr,
                                                0,
                                                NULL,
                                                &propsize,
                                                avr);
        chk_err(result, "dev_grsr - AudioObjectGetPropertyData");
        n = (num_rates < *nr) ? num_rates : *nr;
        for (i = 0; i < n; i++) {
            rate[i] = avr[i].mMaximum;
        }
        free(avr);
    }
    *nr = num_rates;
}

double
dev_get_rate(int32_t di)
{
    Float64 device_rate = 0;
    AudioObjectPropertyAddress propaddr = {
        kAudioDevicePropertyActualSampleRate,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster };
    OSStatus result = 0;
    UInt32 propsize = sizeof(Float64);

    // check whether device supports getting sample rate
    if (AudioObjectHasProperty(adid[di],
                               &propaddr)) {
        result = AudioObjectGetPropertyData(adid[di],
                                            &propaddr,
                                            0,
                                            NULL,
                                            &propsize,
                                            &device_rate);
        chk_err(result, "dev_get_rate - AudioObjectGetPropertyData");
    }
    return (device_rate);
}

void
dev_set_rate(int32_t di, double rate)
{
    AudioObjectPropertyAddress propaddr = {
        kAudioDevicePropertyNominalSampleRate,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster };
    OSStatus result = 0;
    UInt32 propsize = sizeof(Float64);
    
    // check whether device supports setting sample rate
    if (AudioObjectHasProperty(adid[di],
                               &propaddr)) {
        result = AudioObjectSetPropertyData(adid[di],
                                            &propaddr,
                                            0,
                                            NULL,
                                            propsize,
                                            &rate);
        chk_err(result, "dev_set_rate - AudioObjectSetPropertyData");
    }
}

static void
dev_get_chan(int32_t di, int channels[])
{
    AudioBufferList *abl;
    AudioObjectPropertyAddress propaddr = {
        kAudioDevicePropertyStreamConfiguration,
        kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMaster };
    UInt32 propsize;
    
    // get number of channels of output
    AudioObjectGetPropertyDataSize(adid[di], &propaddr, 0, NULL, &propsize);
    if (propsize >= sizeof(AudioBufferList)) {
        abl = (AudioBufferList *) malloc(propsize);
        AudioObjectGetPropertyData(adid[di], &propaddr, 0, NULL, &propsize, abl);
        channels[0]= abl->mBuffers[0].mNumberChannels;
        free(abl);
    } else {
        channels[0] = 0;
    }
    // get number of channels of input
    propaddr.mScope = kAudioDevicePropertyScopeInput;
    AudioObjectGetPropertyDataSize(adid[di], &propaddr, 0, NULL, &propsize);
    if (propsize >= sizeof(AudioBufferList)) {
        abl = (AudioBufferList *) malloc(propsize);
        AudioObjectGetPropertyData(adid[di], &propaddr, 0, NULL, &propsize, abl);
        channels[1]= abl->mBuffers[0].mNumberChannels;
        free(abl);
    } else {
        channels[1] = 0;
    }
}

/***************************************************************************/

static int32_t
_ar_mac_list_rates(int32_t di)
{
    int32_t    i, j, gdsr;
    static  int32_t rate[32];
    static  int32_t nr = sizeof(rate) / sizeof(int32_t);

    dev_gdsr(di, rate, &nr);
    gdsr = 0;
    for (i = 0; i < SRLSTSZ; i++) {
        for (j = 0; j < nr; j++) {
           if (_ar_SRlist[i] == rate[j]) {
               gdsr |= 1 << i;
           }
        }
    }
    return (gdsr);
}

/***************************************************************************/

/* _ar_mac_num_dev - return number of devices */

static int32_t
_ar_mac_num_dev()
{
    return (num_dev());
}

static int32_t
_ar_mac_find_dev(int32_t flags)
{
    return (def_dev());
}

/* _ar_mac_dev_name - return name of I/O device */

static char   *
_ar_mac_dev_name(int32_t di)
{
    return ((char *) dev_name(di));
}

/* _ar_mac_close - close I/O device */

static void
_ar_mac_close(int32_t di)
{
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.01, false);
    dev_stop();
    dev_close();
    q_free();
}

/* _ar_mac_open - open I/O device */

static int32_t
_ar_mac_open(int32_t di)
{
    int chan[2];
    int32_t err = 0;
    ARDEV *a;

    a = _ardev[di];
    if (!a->gdsr)
        a->gdsr = _ar_mac_list_rates(di);
    a->rate = _ar_adjust_rate(di, a->a_rate);
    a->ntlv = 0;    // not interleaved
    a->bits = 24;   // I/O bits
    a->left = 0;    // left-shift bits within sample
    a->nbps = 4;    // number of bytes per sample
    dev_get_chan(di, chan);
    a->ncda = (chan[0] < 2) ? chan[0] : 2;  // number of channels of D/A
    a->ncad = (chan[1] < 2) ? chan[1] : 2;  // number of channels of A/D
    err = dev_open(di);
    if (a->rate > 0) dev_set_rate(di, a->rate);
    _ardev[di]->rate = dev_get_rate(di);

    if (err) return (160);    // OS X open error
    return (0);
}

/* _ar_mac_io_prepare - prepare device and buffers for I/O */

static int32_t
_ar_mac_io_prepare(int32_t di)
{
    ARDEV  *a;

    a = _ardev[di];
    q_init(a->o_data,
           a->i_data,
           a->sizptr,
           (int) a->ncda,
           (int) a->ncad,
           (int) a->segswp);
    dev_prep(di);
    return (0);
}

/* _ar_mac_xfer_seg - this segment is ready to go */

static int32_t
_ar_mac_xfer_seg(int32_t di, int32_t b)
{
    q_odone[b] = _ardev[di]->ncda ? 0 : 1;
    q_idone[b] = _ardev[di]->ncad ? 0 : 1;
    return (q_odone[b] && q_idone[b]);
}

/* _ar_mac_chk_seg - check for segment completion */

static int32_t
_ar_mac_chk_seg(int32_t di, int32_t b)
{
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.01, false);
    return (q_odone[b] && q_idone[b]);
}

/* _ar_mac_io_start - start I/O */

static void
_ar_mac_io_start(int32_t di)
{
    dev_start();
}

/* _ar_mac_io_stop - stop I/O */

static void
_ar_mac_io_stop(int32_t di)
{
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, false);
    dev_stop();
}

/* _ar_os_bind - bind OS functions */

int32_t
_ar_os_bind(int32_t ndt, int32_t tnd)
{
    int32_t nd;

    nd = _ar_mac_num_dev();
    if (nd > 0) {
        _ardvt[ndt].num_dev = _ar_mac_num_dev;
        _ardvt[ndt].find_dev = _ar_mac_find_dev;
	_ardvt[ndt].dev_name = _ar_mac_dev_name;
	_ardvt[ndt].io_stop = _ar_mac_io_stop;
	_ardvt[ndt].close = _ar_mac_close;
	_ardvt[ndt].open = _ar_mac_open;
	_ardvt[ndt].io_prepare = _ar_mac_io_prepare;
	_ardvt[ndt].io_start = _ar_mac_io_start;
	_ardvt[ndt].xfer_seg = _ar_mac_xfer_seg;
	_ardvt[ndt].chk_seg = _ar_mac_chk_seg;
	dio = (int) tnd;
    }
    set_cardinfo();

    return (nd);
}

/**************************************************************************/
