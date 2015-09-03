//
//  main.c
//  ParseTLV_C
//
//  Created by Fritz on 25.08.15.
//  Copyleft 2015 Fritz.
//

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// testdata
unsigned char hex[1000] =
{
    0xe1,0x35,0x9f,0x1e,0x08,0x31,0x36,0x30,0x32,0x31,0x34,0x33,0x37,0xef,0x12,0xdf,
    0x0d,0x08,0x4d,0x30,0x30,0x30,0x2d,0x4d,0x50,0x49,0xdf,0x7f,0x04,0x31,0x2d,0x32,
    0x32,0xef,0x14,0xdf,0x0d,0x0b,0x4d,0x30,0x30,0x30,0x2d,0x54,0x45,0x53,0x54,0x4f,
    0x53,0xdf,0x7f,0x03,0x36,0x2d,0x35
    
};


// --------------------------------------------------------------------------
// calcTagBytesCount - calculates the number of bytes used by the TAG
// input    bytestream
// returns  the number of bytes
// --------------------------------------------------------------------------

int calcTagBytesCount(unsigned char *bytes)
{
    if((bytes[0] & 0x1F) == 0x1F)
    { // see subsequent bytes
        int len = 2;
        for(int i=1; i<10; i++)
        {
            if( (bytes[i] & 0x80) != 0x80)
            {
                break;
            }
            len++;
        }
        return len;
    }
    else
    {
        return 1;
    }
}

// --------------------------------------------------------------------------
// isConstructed - checks if it is constructed
// input    bytestream
// returns  True if constructed
// --------------------------------------------------------------------------

bool isConstructed(unsigned char *bytes)
{
    // 0x20
    //return (bytes[0] & 0b00100000) != 0;
    return (bytes[0] & 0x20) != 0;
}

// --------------------------------------------------------------------------
// calcLengthBytesCount - the number of bytes used by the LENGTH
// input    bytestream
// returns  number of bytes
// --------------------------------------------------------------------------

int calcLengthBytesCount(unsigned char *bytes)
{
    unsigned int len = (uint) bytes[0];
    if( (len & 0x80) == 0x80)
    {
        return (1 + (len & 0x7f));
    }
    else
    {
        return 1;
    }
}


// --------------------------------------------------------------------------
// calcDataLength - calculates the LENGTH of the following Value
// input    bytestream
// returns  length or -1 if an error occured
// --------------------------------------------------------------------------

int calcDataLength(unsigned char *bytes)
{
    unsigned int length = bytes[0];
    
    if((length & 0x80) == 0x80)
    {
        int numberOfBytes = length & 0x7f;
        // only max 3 bytes are supported
        if(numberOfBytes>3)
            return -1;
        
        length = 0;
        for(int i=1; i<1+numberOfBytes; i++)
        {
            length = length * 0x100 + bytes[i];
        }
        
    }
    return length;
}

// --------------------------------------------------------------------------
// GetTLVTag gets the TAG
// input    bytestream
// output   tag
// output   constructed
// returns  number of bytes for tag field
// --------------------------------------------------------------------------

int GetTLVTag(unsigned char *bytes, int *tag, bool *bConstructed)
{
    int tagBytesCount;
    int i;
    int tagData;
    //
    tagBytesCount = calcTagBytesCount(bytes);
    //
    tagData = 0;
    for (i=0; i<tagBytesCount; i++)
    {
        tagData =  tagData*0x100 + bytes[i];
    }
    *tag = tagData;
    *bConstructed = isConstructed(bytes);
    //printf("GetNextTag returns=%d tag=%x isConstructed=%d",tagBytesCount, *tag, *bConstructed);
    return tagBytesCount;
}

// --------------------------------------------------------------------------
// GetTLVLength gets the LENGTH
// input    bytestream
// output   length
// returns  number of bytes for length field or -1 if error occured
// --------------------------------------------------------------------------
int GetTLVLength(unsigned char *bytes, int *length)
{
    int valueLength = 0;
    int lengthBytesCount = 0;
    
    lengthBytesCount = calcLengthBytesCount(bytes);
    valueLength = calcDataLength(bytes);
    
    if (valueLength <0)
        return -1;
    
    *length = valueLength;
    //
    return (lengthBytesCount);
}

// --------------------------------------------------------------------------
// GetTLVValue gets the value
// input    bytestream
// input    length
// output   value bytestrem
// returns  length
// --------------------------------------------------------------------------
int GetTLVValue(unsigned char *bytes, unsigned char *value, int length)
{
    memcpy(value, bytes, length);
    //
    return (length);
}

// --------------------------------------------------------------------------
// parseTLVStream   TLV-Parser
// input    bytestream
// input    length of bytestream
// input    startlevel
// returns  length
// --------------------------------------------------------------------------
int parseTLVStream(unsigned char *data, int datasize, int level)
{
    bool constructed;
    //
    int valueLength = 0;
    int len=0;
    int tagBytesCount;
    int i;
    int tagData;
    
    unsigned char valueData[1000];
    
    if (datasize==0)
    {
        return 1;
    }
    
    while ((len = datasize) > 0)
    {
        
        tagBytesCount =  GetTLVTag(data, &tagData, &constructed);
        if (tagBytesCount < 0)
        {
            printf(" -> GetNextTag error=%d",tagBytesCount);
            return -2;
        }
        
        // progress
        // data = [data subdataWithRange:NSMakeRange(tagBytesCount, [data length]-tagBytesCount)];
        memcpy(data, data+tagBytesCount, datasize-tagBytesCount);
        datasize = datasize-tagBytesCount;
        
        //
        int lengthBytesCount = GetTLVLength(data, &valueLength);
        if (lengthBytesCount < 0)
        {
            printf(" -> GetTagLength error=%d",lengthBytesCount);
            return -2;
        }
        
        // progress
        // data = [data subdataWithRange:NSMakeRange(lengthBytesCount, [data length]-lengthBytesCount)];
        memcpy(data, data+lengthBytesCount, datasize-lengthBytesCount); //strLen
        datasize = datasize-lengthBytesCount;
        
        //memcpy(valueData, data, valueLength);
        GetTLVValue(data, valueData, valueLength);
        
        // progress
        // data = [data subdataWithRange:NSMakeRange(valueLength, [data length]-valueLength)];
        memcpy(data, data+valueLength, datasize-valueLength);
        datasize = datasize-valueLength;
        
        // -------- printout start
        printf("level        = %d\n",level);
        //printf("tagBytesCount      = %d\n",tagBytesCount);
        printf("tagData            = %x\n",tagData);
        printf("constructed        = %d\n",constructed);
        //printf("lengthBytesCount   = %d\n",lengthBytesCount);
        //printf("valueLength        = %d\n",valueLength);
        // -------- printout end
        
        if (constructed == false)
        {
            // -------- printout start
            printf("valueData          = ");
            for(i=0;i<valueLength;i++)
            {
                printf("%02x",valueData[i]);
            }
            printf("\n");
            // -------- printout end
        }
        
        if (constructed == true)
        {
            // recursion
            parseTLVStream(valueData, valueLength, level+1 );
        }
    }
    return 1;
}


int do_it(void)
{
    unsigned char buffer[1000];
    
    int buffersize = 0;
    int ret = 0;
    int iLevel = 0;
    //
    printf("BERTLV Parser\n\n");
    //
    memcpy (buffer,hex,55);
    buffersize = 55;
    ret = parseTLVStream(buffer, buffersize, iLevel);
    
    return 0;
}


int main(int argc, const char * argv[])
{
    do_it();
    
    return 0;
}
