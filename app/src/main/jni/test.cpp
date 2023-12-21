/* The below file demonstrates usage of the SoliCallSDK in order to filter raw PCM files.

     This program works with audio files containing PCM, 16 bit, mono. Frequency could be either in the range of 8000 to 64000.

     If you require to swap the byte order (e.g. the files are Little Endian format but you have Big Endian machine like SPARC) change the value of SOLICALL_KEEP_BYTE_ORDER to 0

     If you have 8 bits per sample (in unsigned linear format) , change the value of BYTES_PER_SAMPLE to 1

     This evaluation executable can process audio files that contain up to 2 minutes worth of data.

     Befoe compiling this file:
     (1) Modify the below include directive to point to the right location of "solicall_api.h"
     (2) Define the right compilation directives as described in section "Compilation Directive" of the user manual.

*/


#include <jni.h>
#include "solicall_api.h"

int iFrequency;
int iDetectAggressiveness;
int iCleanAggressiveness;



#define FREQUENCY iFrequency // Number between 8000 to 64000
#define FRAME_MULTIPLIER 1
#define SAMPLES_IN_FRAME (int(ceil((FRAME_MULTIPLIER*32.0*FREQUENCY)/8000)))
#define BYTES_PER_SAMPLE 2
#define BYTES_PER_FRAME  (SAMPLES_IN_FRAME * BYTES_PER_SAMPLE)
#define SOLICALL_KEEP_BYTE_ORDER 1

#define CHANNEL_ID 0 // the first channel. The channel number can be 0,1,...,(MAX_NUM_SOLICALL_SDK_AEC_CHANNELS-1)

#define OUTPUT_FRAME_TO_FILE {\
            SHORT sDummy; \
            BYTE pbDummy[2]; \
            for (i = 0 ; i < iTmpOut/((BYTES_PER_SAMPLE == 2)?2:1); i++) \
            { \
                if (BYTES_PER_SAMPLE == 1) \
                { \
                    if (fwrite(pbIO+i,1,1,fp2) != 1) \
                    {	fprintf(stderr,"error during output\n"); \
                        exit(1); \
                    } \
                } \
                else if (SOLICALL_KEEP_BYTE_ORDER) \
                {   sDummy = psIO[i]; \
                    if (fwrite(&sDummy,2,1,fp2) != 1) \
                    {	fprintf(stderr,"error during output\n"); \
                        exit(1); \
                    } \
                } \
                else \
                {   pbDummy[0] = pbIO[i*2+1]; \
                    pbDummy[1] = pbIO[i*2]; \
                    if (fwrite(pbDummy,1,2,fp2) != 2) \
                    {	fprintf(stderr,"error during output\n"); \
                        exit(1); \
                    } \
                } \
            } \
        }




void fillSoliCallInitAdvanced(sSoliCallInit *pmySoliCallInit)
{
    pmySoliCallInit->iCPUPower = 2;
    pmySoliCallInit->sBitsPerSample = (BYTES_PER_SAMPLE==1)?8:16;
    pmySoliCallInit->iFrequency = FREQUENCY;
    pmySoliCallInit->sFrameSize = FRAME_MULTIPLIER;
    pmySoliCallInit->sLookAheadSize = 0;
    pmySoliCallInit->bNeedToCheckDTMF = false;
    pmySoliCallInit->bDoNotChangeTheOutput = false;
    pmySoliCallInit->iNumMsecFromStartToKeepTheOutput = 0;
    pmySoliCallInit->sAECTypeParam = 8;
    pmySoliCallInit->sDelaySize = 2;
    pmySoliCallInit->sMaxAsyncSpeakerDelayAECParam = 5;
    pmySoliCallInit->sMaxAsyncMicDelayAECParam = 5;
    pmySoliCallInit->sSensitivityLevelAECParam = 6;
    pmySoliCallInit->sAggressiveLevelAECParam = 10;
    pmySoliCallInit->sAECHowlingLevelTreatment = 10;
    pmySoliCallInit->sMaxCoefInAECParam = 100;
    pmySoliCallInit->sMinCoefInAECParam = 1;
    pmySoliCallInit->sAECTailType = 0;
    pmySoliCallInit->sAECMinTailType = 0;

    pmySoliCallInit->sAECMinRobustnessLevel = 0;
    pmySoliCallInit->sAECMaxRobustnessLevel = 15;
    pmySoliCallInit->sAECStabilityLevel = 10;
    pmySoliCallInit->sAECAdvancedAggressiveLevel = 10;
    pmySoliCallInit->sAECEnvironmentType = 10;

    pmySoliCallInit->iNumberOfSamplesInAECBurst = 2000;
    pmySoliCallInit->iNumberOfSamplesInHighConfidenceAECBurst = 2000;
    pmySoliCallInit->sAECMinOutputPercentageDuringEcho = 0;
    pmySoliCallInit->sAecStartupAggressiveLevel = 10; // startup heuristic
    pmySoliCallInit->sComfortNoisePercent = 100;  //comfort noise
}

void testAEC(char *pcInputFile,char *pcSpeakerFile, char *pcOutputFile)
{
    int i;
    int iNumValues = 0;
    SHORT sTmpValue;
    BYTE  pbTmpValue[2];
    BYTE *pbIO = (BYTE *) malloc(BYTES_PER_FRAME);
    SHORT *psIO = (SHORT *) pbIO;
    BYTE *pbSpeaker = (BYTE *) malloc(BYTES_PER_FRAME) ;
    SHORT *psSpeaker = (SHORT *) pbSpeaker;
    int iTmpIn,iTmpOut,iTmpCurrEchoAmplitude;
    int iTmpTotalIn =0;
    int iTmpTotalOut =0;

    iTmpIn = BYTES_PER_FRAME;

    sSoliCallInit mySoliCallInit;

    // use the advanced algorithm
    fillSoliCallInitAdvanced(&mySoliCallInit);

    if (SoliCallAECInit(CHANNEL_ID,&mySoliCallInit) != SOLICALL_RC_SUCCESS)
    {  fprintf(stderr,"error in init  - did you pass the evaluation period ?\n");
        exit(1);
    }


    FILE *fp1 = fopen(pcInputFile,"rb");
    FILE *fp2 = fopen(pcOutputFile,"wb");
    FILE *fp3 = fopen(pcSpeakerFile,"rb");
    if (fp1  == NULL)
    {	fprintf(stderr,"error opening input file %s\n",pcInputFile);
        exit(1);
    }
    if (fp2 == NULL)
    {	fprintf(stderr,"error opening output file %s\n",pcOutputFile);
        exit(1);
    }
    if (fp3 == NULL)
    {	fprintf(stderr,"error opening speaker file %s\n",pcSpeakerFile);
        exit(1);
    }


    while (1)
    {   // read from the input file

        if (BYTES_PER_SAMPLE == 1)
        {
            if (fread(pbTmpValue,1,1,fp1) != 1)
                break; // no more data
            pbIO[iNumValues] = pbTmpValue[0];
        }
        else if (SOLICALL_KEEP_BYTE_ORDER)
        {
            if (fread(&sTmpValue,2,1,fp1) != 1)
                break; // no more data

            psIO[iNumValues] = sTmpValue;
        }
        else
        {
            if (fread(pbTmpValue,1,2,fp1) != 2)
                break; // no more data
            pbIO[iNumValues*2] = pbTmpValue[1];
            pbIO[iNumValues*2+1] = pbTmpValue[0];
        }

        // read from the speaker file
        if (BYTES_PER_SAMPLE == 1)
        {
            if (fread(pbTmpValue,1,1,fp3) != 1)
                break; // no more data
            pbSpeaker[iNumValues] = pbTmpValue[0];
        }
        else if (SOLICALL_KEEP_BYTE_ORDER)
        {
            if (fread(&sTmpValue,2,1,fp3) != 1)
                break; // no more data

            psSpeaker[iNumValues] = sTmpValue;
        }
        else
        {
            if (fread(pbTmpValue,1,2,fp3) != 2)
                break; // no more data
            pbSpeaker[iNumValues*2] = pbTmpValue[1];
            pbSpeaker[iNumValues*2+1] = pbTmpValue[0];
        }



        ++iNumValues;

        if (iNumValues == SAMPLES_IN_FRAME)
        {   // another frame was filled so send it to cleaning
            if (SoliCallAECProcessSpkFrame(CHANNEL_ID,pbSpeaker,iTmpIn) != SOLICALL_RC_SUCCESS)
            {   fprintf(stderr,"Error in process frame. Did you pass the call length limit?\n");
                fprintf(stderr,"In addition, please consult the manual for guidelines when using the advanced algorithm.\n");
                exit(1);
            }

            if (SoliCallAECProcessMicFrame(CHANNEL_ID,pbIO,iTmpIn,pbIO,&iTmpOut,&iTmpCurrEchoAmplitude) != SOLICALL_RC_SUCCESS)
            {   fprintf(stderr,"Error in process frame. Did you pass the call length limit?\n");
                fprintf(stderr,"In addition, please consult the manual for guidelines when using the advanced algorithm.\n");
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

    printf("Total In=%d Out=%d\n",iTmpTotalIn,iTmpTotalOut);


    fclose(fp2);

    if (SoliCallAECTerminate(CHANNEL_ID) != SOLICALL_RC_SUCCESS)
    {  fprintf(stderr,"error in terminate\n");
        exit(1);
    }

    free(pbIO);
    free(pbSpeaker);

}

void printHelpAndExit(char **argv)
{
    fprintf(stderr,"Usage: %s  [-f frequency] <-i file_to_clean> <-s speaker_file> <-o output_file> \n",argv[0]);
    fprintf(stderr,"frequency -  Between 8000 to 64000. Multiply of 8000. Default=8000\n");
    fprintf(stderr,"All audio files are raw PCM with two bytes per sample.\n");
    exit(1);
}

#ifdef _WINCE
int wmain(int argc, char **argv)
#else // windows & Linux
int main(int argc, char **argv)
#endif
{
    iFrequency = 8000;
    char *pcRegistration = NULL;
    char *pcInput = NULL;
    char *pcOutput = NULL;
    char *pcSpeaker = NULL;

#ifdef _WINCE
    // Fix arguments (used to debug WinCE)
    pcRegistration = "p_original.raw";
    pcInput = "p_pink.raw";
    pcOutput = "tmp.raw";
    pcSpeaker = "p_pink.raw";
#else
    // For all other environments (except WinCE)

    // Parse the command line arguments
    int iNumArguments = (argc-1)/2;
    while (iNumArguments > 0)
    {
        if (strcmp(argv[iNumArguments*2-1],"-f") == 0)
        {   iFrequency = atoi(argv[iNumArguments*2]);
            if ((iFrequency < 8000) || (iFrequency > 64000))
                printHelpAndExit(argv);
        }
        else if (strcmp(argv[iNumArguments*2-1],"-i") == 0)
        {   pcInput = argv[iNumArguments*2];
        }
        else if (strcmp(argv[iNumArguments*2-1],"-s") == 0)
        {   pcSpeaker = argv[iNumArguments*2];
        }
        else if (strcmp(argv[iNumArguments*2-1],"-o") == 0)
        {   pcOutput = argv[iNumArguments*2];
        }
        else
            printHelpAndExit(argv);

        iNumArguments--;
    }

#endif

    if ( (pcInput == NULL) || (pcOutput == NULL) || (pcSpeaker == NULL) )
        printHelpAndExit(argv); // missing either input or output file


    sSoliCallPackageInit mySoliCallPackageInit;

    mySoliCallPackageInit.sVersion = 7;
    mySoliCallPackageInit.pcSoliCallBin = NULL;


    if (SoliCallPackageInit(&mySoliCallPackageInit) != SOLICALL_RC_SUCCESS)
    {  fprintf(stderr,"error in package init\n");
        exit(1);
    }


    testAEC(pcInput,pcSpeaker,pcOutput);

    exit(0);

}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_solicalldemo_MainActivity_test(JNIEnv *env, jobject obj, jstring inputFile, jstring speakerFile, jstring outputFile) {
    const char* pcInputFile = env->GetStringUTFChars(inputFile, 0);
    const char* pcSpeakerFile = env->GetStringUTFChars(speakerFile, 0);
    const char* pcOutputFile = env->GetStringUTFChars(outputFile, 0);

    int i;
    int iNumValues = 0;
    SHORT sTmpValue;
    BYTE  pbTmpValue[2];
    BYTE *pbIO = (BYTE *) malloc(BYTES_PER_FRAME);
    SHORT *psIO = (SHORT *) pbIO;
    BYTE *pbSpeaker = (BYTE *) malloc(BYTES_PER_FRAME) ;
    SHORT *psSpeaker = (SHORT *) pbSpeaker;
    int iTmpIn,iTmpOut,iTmpCurrEchoAmplitude;
    int iTmpTotalIn =0;
    int iTmpTotalOut =0;

    iTmpIn = BYTES_PER_FRAME;

    sSoliCallInit mySoliCallInit;

    // use the advanced algorithm
    fillSoliCallInitAdvanced(&mySoliCallInit);

    if (SoliCallAECInit(CHANNEL_ID,&mySoliCallInit) != SOLICALL_RC_SUCCESS)
    {  fprintf(stderr,"error in init  - did you pass the evaluation period ?\n");
        exit(1);
    }


    FILE *fp1 = fopen(pcInputFile,"rb");
    FILE *fp2 = fopen(pcOutputFile,"wb");
    FILE *fp3 = fopen(pcSpeakerFile,"rb");
    if (fp1  == NULL)
    {	fprintf(stderr,"error opening input file %s\n",pcInputFile);
        exit(1);
    }
    if (fp2 == NULL)
    {	fprintf(stderr,"error opening output file %s\n",pcOutputFile);
        exit(1);
    }
    if (fp3 == NULL)
    {	fprintf(stderr,"error opening speaker file %s\n",pcSpeakerFile);
        exit(1);
    }


    while (1)
    {   // read from the input file

        if (BYTES_PER_SAMPLE == 1)
        {
            if (fread(pbTmpValue,1,1,fp1) != 1)
                break; // no more data
            pbIO[iNumValues] = pbTmpValue[0];
        }
        else if (SOLICALL_KEEP_BYTE_ORDER)
        {
            if (fread(&sTmpValue,2,1,fp1) != 1)
                break; // no more data

            psIO[iNumValues] = sTmpValue;
        }
        else
        {
            if (fread(pbTmpValue,1,2,fp1) != 2)
                break; // no more data
            pbIO[iNumValues*2] = pbTmpValue[1];
            pbIO[iNumValues*2+1] = pbTmpValue[0];
        }

        // read from the speaker file
        if (BYTES_PER_SAMPLE == 1)
        {
            if (fread(pbTmpValue,1,1,fp3) != 1)
                break; // no more data
            pbSpeaker[iNumValues] = pbTmpValue[0];
        }
        else if (SOLICALL_KEEP_BYTE_ORDER)
        {
            if (fread(&sTmpValue,2,1,fp3) != 1)
                break; // no more data

            psSpeaker[iNumValues] = sTmpValue;
        }
        else
        {
            if (fread(pbTmpValue,1,2,fp3) != 2)
                break; // no more data
            pbSpeaker[iNumValues*2] = pbTmpValue[1];
            pbSpeaker[iNumValues*2+1] = pbTmpValue[0];
        }



        ++iNumValues;

        if (iNumValues == SAMPLES_IN_FRAME)
        {   // another frame was filled so send it to cleaning
            if (SoliCallAECProcessSpkFrame(CHANNEL_ID,pbSpeaker,iTmpIn) != SOLICALL_RC_SUCCESS)
            {   fprintf(stderr,"Error in process frame. Did you pass the call length limit?\n");
                fprintf(stderr,"In addition, please consult the manual for guidelines when using the advanced algorithm.\n");
                exit(1);
            }

            if (SoliCallAECProcessMicFrame(CHANNEL_ID,pbIO,iTmpIn,pbIO,&iTmpOut,&iTmpCurrEchoAmplitude) != SOLICALL_RC_SUCCESS)
            {   fprintf(stderr,"Error in process frame. Did you pass the call length limit?\n");
                fprintf(stderr,"In addition, please consult the manual for guidelines when using the advanced algorithm.\n");
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

    printf("Total In=%d Out=%d\n",iTmpTotalIn,iTmpTotalOut);


    fclose(fp2);

    if (SoliCallAECTerminate(CHANNEL_ID) != SOLICALL_RC_SUCCESS)
    {  fprintf(stderr,"error in terminate\n");
        exit(1);
    }

    free(pbIO);
    free(pbSpeaker);

    return env->NewStringUTF(pcOutputFile);
}
