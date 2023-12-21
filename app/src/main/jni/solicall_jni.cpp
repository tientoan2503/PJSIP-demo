#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>
#include <ctype.h>
#include <jni.h>
#include <android/log.h>
#include <zconf.h>
#include "solicall_api.h"


#define FREQUENCY 16000 // Number between 8000 to 64000
#define FRAME_MULTIPLIER 1
#define SAMPLES_IN_FRAME (int(ceil((FRAME_MULTIPLIER*32.0*FREQUENCY)/8000)))
#define BYTES_PER_SAMPLE 2
#define BYTES_PER_FRAME  (SAMPLES_IN_FRAME * BYTES_PER_SAMPLE)

#define CHANNEL_ID 0 // the first channel. The channel number can be 0,1,...,(MAX_NUM_SOLICALL_SDK_AEC_CHANNELS-1)

#define OUTPUT_FRAME_TO_FILE {\
            SHORT sDummy; \
            BYTE pbDummy[2]; \
            for (i = 0 ; i < iTmpOut/((BYTES_PER_SAMPLE == 2)?2:1); i++) \
            { \
                {   sDummy = psIO[i]; \
                    if (fwrite(&sDummy,2,1,fp2) != 1) \
                    {	fprintf(stderr,"error during output\n"); \
                        exit(1); \
                    } \
                } \
            } \
        }



extern "C"
JNIEXPORT jint JNICALL Java_com_example_pjsipgo_CallActivity_getVersion
(JNIEnv *env, jobject obj) {
	unsigned int ver;
	SoliCallVersion(&ver);
	return (jint)ver;
}

extern "C"
JNIEXPORT jint JNICALL Java_com_example_pjsipgo_MainActivity_packageInit
(JNIEnv *env, jobject obj) {
    sSoliCallPackageInit mySoliCallPackageInit;

	mySoliCallPackageInit.sVersion = 7;    
    mySoliCallPackageInit.pcSoliCallBin = "/data/data/com.example.pjsipgo/files/";

    int iRes = SoliCallPackageInit(&mySoliCallPackageInit);
    
    return (jint) iRes;
}

extern "C"
JNIEXPORT jint JNICALL Java_com_example_pjsipgo_MainActivity_AECInit
(JNIEnv *env, jobject obj) {

    sSoliCallInit mySoliCallInit;

    // Advanced AEC
    mySoliCallInit.iCPUPower = SOLICALL_LOWEST_CPU_POWER;
    mySoliCallInit.sBitsPerSample = (BYTES_PER_SAMPLE==1)?8:16;
    mySoliCallInit.iFrequency = FREQUENCY;
    mySoliCallInit.sFrameSize = FRAME_MULTIPLIER;
    mySoliCallInit.sLookAheadSize = 0;
    mySoliCallInit.bNeedToCheckDTMF = false;
    mySoliCallInit.bDoNotChangeTheOutput = false;
    mySoliCallInit.iNumMsecFromStartToKeepTheOutput = 0;
    mySoliCallInit.sAECTypeParam = 1;
    mySoliCallInit.sDelaySize = 2;
    mySoliCallInit.sMaxAsyncSpeakerDelayAECParam = 3;
    mySoliCallInit.sMaxAsyncMicDelayAECParam = 3;
    mySoliCallInit.sSensitivityLevelAECParam = 6;
    mySoliCallInit.sAggressiveLevelAECParam = 10;
    mySoliCallInit.sAECHowlingLevelTreatment = 10;
    mySoliCallInit.sMaxCoefInAECParam = 100;
    mySoliCallInit.sMinCoefInAECParam = 1;
    mySoliCallInit.sAECTailType = 0;
    mySoliCallInit.sAECMinTailType = 0;

    mySoliCallInit.sAECMinRobustnessLevel = 0;
    mySoliCallInit.sAECMaxRobustnessLevel = 15;
    mySoliCallInit.sAECStabilityLevel = 10;
    mySoliCallInit.sAECAdvancedAggressiveLevel = 10;
    mySoliCallInit.sAECEnvironmentType = 10;

    mySoliCallInit.iNumberOfSamplesInAECBurst = 2000;
    mySoliCallInit.iNumberOfSamplesInHighConfidenceAECBurst = 2000;
    mySoliCallInit.sAECMinOutputPercentageDuringEcho = 0;
    mySoliCallInit.sAecStartupAggressiveLevel = 10; // startup heuristic
    mySoliCallInit.sComfortNoisePercent = 100;  //comfort noise

    
	int iRes = SoliCallAECInit(CHANNEL_ID,&mySoliCallInit);
    
    return (jint) iRes;
}


extern "C"
JNIEXPORT jint JNICALL Java_com_example_pjsipgo_MainActivity_processFile
(JNIEnv *env, jobject obj) {

    int iNumValues = 0;
    SHORT sTmpValue;
    BYTE  pbTmpValue[2];
    BYTE *pbIO = (BYTE *) malloc (BYTES_PER_FRAME);
    SHORT *psIO = (SHORT *) pbIO;
    BYTE *pbSpeaker = (BYTE *) malloc (BYTES_PER_FRAME);
    SHORT *psSpeaker = (SHORT *) pbSpeaker;
    int iTmpIn,iTmpOut,iTmpCurrEchoAmplitude;
    int iTmpTotalIn =0;
    int iTmpTotalOut =0;
    int i;

    iTmpIn = BYTES_PER_FRAME;

    FILE *fp1 = fopen("/sdcard/data/tmp/test_input_mic","rb");
    FILE *fp2 = fopen("/sdcard/data/tmp/test_output","wb");
    FILE *fp3 = fopen("/sdcard/data/tmp/test_input_spk","rb");
    if (fp1  == NULL)
    {	fprintf(stderr,"error opening mic input file\n");
        exit(1);
    }
    if (fp2 == NULL)
    {	fprintf(stderr,"error opening output file\n");
        exit(1);
    }
    if (fp3 == NULL)
    {	fprintf(stderr,"error opening speaker file\n");
        exit(1);
    }


    while (1)
    {   // read from the input file

        if (fread(&sTmpValue,2,1,fp1) != 1)
            break; // no more data

        psIO[iNumValues] = sTmpValue;

        // read from the speaker file
        if (fread(&sTmpValue,2,1,fp3) != 1)
            break; // no more data

        psSpeaker[iNumValues] = sTmpValue;


        ++iNumValues;

        if (iNumValues == SAMPLES_IN_FRAME)
        {   // another frame was filled so send it to cleaning
            if (SoliCallAECProcessSpkFrame(CHANNEL_ID,pbSpeaker,iTmpIn) != SOLICALL_RC_SUCCESS)
            {   fprintf(stderr,"Error in process frame. Did you pass the call length limit?\n");
                exit(1);
            }

            if (SoliCallAECProcessMicFrame(CHANNEL_ID,pbIO,iTmpIn,pbIO,&iTmpOut,&iTmpCurrEchoAmplitude) != SOLICALL_RC_SUCCESS)
            {   fprintf(stderr,"Error in process frame. Did you pass the call length limit?\n");
                exit(1);
            }

            iTmpTotalIn += iTmpIn;
            iTmpTotalOut += iTmpOut;

            // cleaning was good so write the result to the output file
            OUTPUT_FRAME_TO_FILE;

            iNumValues = 0; // we start a new frame
        }
    } // while true
    fclose(fp1);
    fclose(fp3);
    fclose(fp2);

    if (SoliCallAECTerminate(CHANNEL_ID) != SOLICALL_RC_SUCCESS)
    {  fprintf(stderr,"error in terminate\n");
        exit(1);
    }

    free(pbIO);
    free(pbSpeaker);

    return 1;
}


//extern "C"
//JNIEXPORT jint JNICALL Java_com_example_nativedemo_MainActivity_AECTerminate
//        (JNIEnv *env, jobject obj) {
//    return SoliCallAECTerminate(CHANNEL_ID);
//}

extern "C"
JNIEXPORT jint JNICALL Java_com_example_pjsipgo_CallActivity_processSpeakerFrame
        (JNIEnv *env, jobject obj, jbyteArray input) {

    int length = env->GetArrayLength(input);
    jbyte *rawData = env->GetByteArrayElements(input, NULL);

    __android_log_print(ANDROID_LOG_DEBUG, "solicall", "process ProcessSpkFrame1 %d", length);

    if (SoliCallAECProcessSpkFrame(CHANNEL_ID, (BYTE *) rawData, length) != SOLICALL_RC_SUCCESS)
    {
        __android_log_print(ANDROID_LOG_ERROR, "solicall", "process ProcessSpkFrame2");
        //fprintf(stderr,"Error in process frame. Did you pass the call length limit?\n");
        //exit(1);
    }
    __android_log_print(ANDROID_LOG_INFO, "solicall", "process ProcessSpkFrame3");

    env->ReleaseByteArrayElements(input, rawData, JNI_ABORT);
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL Java_com_example_pjsipgo_CallActivity_processMicFrame
        (JNIEnv *env, jobject obj, jbyteArray input) {

    jbyte *in;
    int bytesOut;
    int iTmpCurrEchoAmplitude;
    in = env->GetByteArrayElements(input, JNI_FALSE);

    __android_log_print(ANDROID_LOG_DEBUG, "solicall", "process processMicFrame1");

    // AEC
    if (SoliCallAECProcessMicFrame(CHANNEL_ID, (BYTE *)in, 8000, (BYTE *)in, &bytesOut, &iTmpCurrEchoAmplitude) != SOLICALL_RC_SUCCESS)
    {
        __android_log_print(ANDROID_LOG_ERROR, "solicall", "process processMicFrame2");
//        fprintf(stderr,"Error in process frame. Did you pass the call length limit?\n");
//        exit(1);
    }
    __android_log_print(ANDROID_LOG_INFO, "solicall", "process processMicFrame3");

    env->ReleaseByteArrayElements(input, in, JNI_ABORT);

    return bytesOut;
}
