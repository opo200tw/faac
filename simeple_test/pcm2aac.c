// https://blog.csdn.net/zydlyq/article/details/50848237
#include <faac.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct AAC_T
{
    unsigned long nSampleRate;
    unsigned int nChannels; 
    unsigned long nInputSamples;
    unsigned long nMaxOutputBytes;
    faacEncHandle hEncoder;
    unsigned int nPCMBitSize;
    faacEncConfigurationPtr pConfiguration;
    int nPCMBufferSize;
    unsigned char *pbPCMBuffer;
    unsigned char *pbAACBuffer;
};

struct AAC_T faac;

void faacSetting(struct AAC_T *aac, int step)
{
    if (step == 1)
    {
        aac->nSampleRate = 44100UL;
        aac->nChannels = 1;
    }
    else if(step == 2)
    {
        aac->nPCMBitSize = 16;
        aac->pConfiguration->inputFormat = FAAC_INPUT_16BIT;
        aac->pConfiguration->quantqual = 200;
        aac->pConfiguration->mpegVersion = 1;
        // aac->pConfiguration->useTns = 1;
        aac->pConfiguration->useLfe = 1;

        aac->nPCMBufferSize = aac->nInputSamples * aac->nPCMBitSize / 8;
        aac->pbPCMBuffer = (unsigned char*) malloc(aac->nPCMBufferSize);
        aac->pbAACBuffer = (unsigned char*) malloc(aac->nMaxOutputBytes);
    }
    else
    {
        memset(aac, 0, sizeof(struct AAC_T));
        if(aac->pbPCMBuffer) free(aac->pbPCMBuffer);// printf("free pbPCMBuffer\n");
        if(aac->pbAACBuffer) free(aac->nMaxOutputBytes);// printf("free pbAACBuffer\n");
    }
}

void faacPrintConfig(struct AAC_T *aac)
{
    // setting
    printf("==========faacEncConfigurationPtr============\n"
        "aacObjectType: %d\n"
        "mpegVersion: %d\n"
        "shortctl: %d\n"
        // "useTns: %d\n"
        "useLfe: %d\n"
        "jointmode: %d\n"
        "pnslevel: %d\n"
        "bitRate: %lu\n"
        "quantqual: %lu\n"
        "bandWidth: %d\n"
        "outputFormat: %d\n"
        "inputFormat: %d\n"
        ,aac->pConfiguration->aacObjectType
        ,aac->pConfiguration->mpegVersion
        ,aac->pConfiguration->shortctl
        // ,aac->pConfiguration->useTns
        ,aac->pConfiguration->useLfe
        ,aac->pConfiguration->jointmode // enum {JOINT_NONE = 0, JOINT_MS, JOINT_IS};
        ,aac->pConfiguration->pnslevel
        ,aac->pConfiguration->bitRate
        ,aac->pConfiguration->quantqual
        ,aac->pConfiguration->bandWidth
        ,aac->pConfiguration->outputFormat
        ,aac->pConfiguration->inputFormat);
}

int main(int argc, char* argv[])
{
    FILE* fpIn; // PCM file for input
    FILE* fpOut; // AAC file for output
 
    fpIn = fopen("./in_pcm", "rb");
    fpOut = fopen("./out.aac", "wb");
    
    struct AAC_T * aac = &faac;
    faacSetting(aac, 1);
    // (1) Open FAAC engine
    aac->hEncoder = faacEncOpen(aac->nSampleRate, aac->nChannels, &aac->nInputSamples, &aac->nMaxOutputBytes);
    if(aac->hEncoder == NULL)
    {
        printf("[ERROR] Failed to call faacEncOpen()\n");
        return -1;
    }
    printf("==========faacEncOpen============\n");
    printf("aac->nInputSamples = %lu\n", aac->nInputSamples);
    printf("aac->nMaxOutputBytes = %lu\n", aac->nMaxOutputBytes);
 
    // (2.1) Get current encoding configuration    
    aac->pConfiguration = faacEncGetCurrentConfiguration(aac->hEncoder);
    // (2.2) Set encoding configuration
    if (!faacEncSetConfiguration(aac->hEncoder, aac->pConfiguration))
    {
        fprintf(stderr, "Unsupported output format!\n");
        return 1;
    }
    faacSetting(aac, 2);
    faacPrintConfig(aac);
    while(1)
    {
        static int count = 0;
        // 读入的实际字节数，最大不会超过nPCMBufferSize，一般只有读到文件尾时才不是这个值
        int nBytesRead = fread(aac->pbPCMBuffer, 1, aac->nPCMBufferSize, fpIn);
        // 输入样本数，用实际读入字节数计算，一般只有读到文件尾时才不是nPCMBufferSize/(aac->nPCMBitSize/8);
        aac->nInputSamples = nBytesRead / (aac->nPCMBitSize / 8);
        // printf("aac->nInputSamples=%d, nBytesRead=%d, aac->nPCMBitSize=%d \n", aac->nInputSamples, nBytesRead, aac->nPCMBitSize);
        // (3) Encode
        int nRet = faacEncEncode(aac->hEncoder, (int*) aac->pbPCMBuffer, aac->nInputSamples, aac->pbAACBuffer, aac->nMaxOutputBytes);
        fwrite(aac->pbAACBuffer, 1, nRet, fpOut);
        printf("nRet=%d\n",nRet);
        count += nRet;
        if(nBytesRead <= 0)
        {
            printf("count = %d",count);
            break;
        }
    }
 
    /*
    while(1)
    {
        // (3) Flushing
        nRet = faacEncEncode(
        hEncoder, (int*) pbPCMBuffer, 0, pbAACBuffer, nMaxOutputBytes);
        if(nRet <= 0)
        {
            break;
        }
    }
    */
    // (4) Close FAAC engine
    int nRet = faacEncClose(aac->hEncoder);
 
    free(aac->pbPCMBuffer);
    free(aac->pbAACBuffer);
    fclose(fpIn);
    fclose(fpOut);
 
    //getchar();
 
    return 0;
}

