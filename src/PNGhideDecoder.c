#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <png.h>
#include <zlib.h>

#include "PNGhideDataDefinitions.h"
#include "PNGhideFileOperations.h"
#include "PNGhideMiscFunctions.h"

short int FindHeader (Picture *OriginalImage, Picture *OutputImage);
short int DecodeImages (Picture *OriginalImage, Picture *HiddenImage);

int main (int argc, char **argv) {
    if (argc != 3){
        printf ("Error: Ussage: <Original Image.png> <Output Image.png>\n");
        return (-3);
    }
    Picture OriginalImage;
    Picture OutputImage;
    int err;

    OriginalImage.FileLocation = argv[1];
    OutputImage.FileLocation = argv [2];

    if (err = loadPicture (&OriginalImage)!= 0)
        return (err);

    if (OriginalImage.ColorSpace == PNG_COLOR_TYPE_PALETTE){
        printf ("Error: Palette mode is not supported\n");
        return -3;
    }

    if (OriginalImage.BitDeph < 8){
        printf ("Error: Bit Deph must be >= 8\n");
        return -3;
    }

    if (err = readPicture (&OriginalImage) != 0)
        return err;

    FindHeader (&OriginalImage, &OutputImage);
    AllocatePictureSpace (&OutputImage);

    DecodeImages(&OriginalImage, &OutputImage);


    if (err = WriteOutput (&OutputImage) !=0 )
        return err;
    FreeImage (&OriginalImage);
    FreeImage (&OutputImage);

    fclose (OriginalImage.ImagePointer);
    fclose (OutputImage.ImagePointer);
return 0;
}

short int GetHeaderDetails (Picture *Image){
    if (!Image){
        return -1;
    }
    printf ("Width <px>: ");
    scanf("%lu",&(Image->Width));
    printf ("Height <px>: ");
    scanf("%lu",&(Image->Height));
    printf ("Bit Deph: ");
    scanf("%d",&(Image->BitDeph));
    printf ("Color Space <dec>: ");
    scanf("%d",&(Image->ColorSpace));
    return 0;
}

short int PrintHeader (Picture *Image){
    if (!Image){
        return -1;
    }
    printf("With = %lu px\n",Image->Width);
    printf("Height = %lu px\n",Image->Height);
    printf("Bit Deph = %d\n",Image->BitDeph);
    printf("ColorSpace = %d ",Image->ColorSpace);
    switch (Image->ColorSpace){
        case PNG_COLOR_TYPE_RGBA:
            printf ("(RGBA)\n");
            break;
        case PNG_COLOR_TYPE_RGB:
            printf ("(RGB)\n");
            break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            printf ("(Grayscale + Alpha)\n");
            break;
        case PNG_COLOR_TYPE_GRAY:
            printf ("(Grayscale)\n");
            break;
        case PNG_COLOR_TYPE_PALETTE:
            printf ("(Palette)\n");
            break;
        default:
            printf ("(Unknown)\n");
            break;
    }
    return 0;
}

/*! \brief Function that tries to read the output image's header information.
 *         If there is a problem while reading, it tries to do some troubleshooting.
 */
short int FindHeader (Picture *OriginalImage, Picture *OutputImage){
    short int OriginalUsableChannels;///<Stores the amount of color channels in the input image that can be used to store information.
    short int OriginalTotalChannels; ///<Stores the total amount of color channels in the input image.
    short int err;
    uint64_t ReadChunks;///<Counts how many image channels we have stracted information from.
    int64_t X,Y;///<Our current X,Y position on the input image.
    png_byte* CurrentOriginalRow; ///<Pointer to the beginning of a row's array of pixels.
    png_byte* CurrentOriginalPixel; ///<Pointer to the input image's pixel struct.

    char opc;
    register char i;
    unsigned char EndSignal;
    unsigned char KeyOK = 0;

    unsigned char CurrentColorValue [OriginalImage->BitDeph];
    unsigned char KeyBitsStart [2][2]; ///<Stores the read key bits at the beggining of the input image.
    unsigned char BinWidthStart[PNGHIDE_WIDTH_VAR_LEN]; ///<Stores the read hidden image's width at the start of the input image as a binary number array.
    unsigned char BinHeightStart[PNGHIDE_HEIGHT_VAR_LEN]; ///<Stores the read hidden image's height at the start of the input image as a binary number array.
    unsigned char BinColorSpaceStart[PNGHIDE_COLORSPACE_VAR_LEN]; ///<Stores the read hidden image's color space at the start of the input image as a binary number array.
    unsigned char BinBitDephStart[PNGHIDE_BIT_DEPH_VAR_LEN]; ///<Stores the read hidden image's bit deph at the start of the input image as a binary number array.

    unsigned char KeyBitsEnd [2][2]; ///<Stores the read key bits at the end of the input image.
    unsigned char BinWidthEnd[PNGHIDE_WIDTH_VAR_LEN];///<Stores the read hidden image's width at the end of the input image as a binary number array.
    unsigned char BinHeightEnd[PNGHIDE_HEIGHT_VAR_LEN];///<Stores the read hidden image's height at the end of the input image as a binary number array.
    unsigned char BinColorSpaceEnd[PNGHIDE_COLORSPACE_VAR_LEN];///<Stores the read hidden image's color space at the end of the input image as a binary number array.
    unsigned char BinBitDephEnd[PNGHIDE_BIT_DEPH_VAR_LEN];///<Stores the read hidden image's bit deph at the end of the input image as a binary number array.

    switch (OriginalImage->ColorSpace){
        default:
            return -3;
            break;
        case PNG_COLOR_TYPE_RGBA:
            OriginalTotalChannels = 4;
            OriginalUsableChannels = 3;
            break;
        case PNG_COLOR_TYPE_RGB:
            OriginalTotalChannels = 3;
            OriginalUsableChannels = 3;
            break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            OriginalTotalChannels = 2;
            OriginalUsableChannels = 1;
            break;
        case PNG_COLOR_TYPE_GRAY:
            OriginalTotalChannels = 1;
            OriginalUsableChannels = 1;
            break;

    }
    ///<Start reading from the beginning of the input image.
    ReadChunks = 0;
    EndSignal = 0;
    for (Y=0; Y<(OriginalImage->Height) && EndSignal == 0; Y++) {
        CurrentOriginalRow = OriginalImage->ImageStart[Y];
        for (X=0; X<(OriginalImage->Width) && EndSignal == 0; X++) {
            CurrentOriginalPixel = &(CurrentOriginalRow[X*OriginalTotalChannels]);
            for (i=0;i<OriginalUsableChannels;i++){

                if (ReadChunks == PNGHIDE_HEADER_SIZE){
                    EndSignal = 1; ///<We finished reading the header at the beginning of the image
                }
                if (ReadChunks>=0 && ReadChunks <= 1){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);///<Read key bits.
                    BinCopy (KeyBitsStart[ReadChunks],CurrentColorValue,2);
                }
                if (ReadChunks>1 && ReadChunks<PNGHIDE_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);///<Convert the current decimal color value to a binary array.
                    BinWidthStart[ReadChunks-2] = CurrentColorValue[0];///<Store the LSB into an array.
                }
                if (ReadChunks>=PNGHIDE_WIDTH_VAR_LEN+2 && ReadChunks<PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    BinHeightStart[ReadChunks-PNGHIDE_WIDTH_VAR_LEN-2] = CurrentColorValue[0];
                }
                if (ReadChunks>=PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2 && ReadChunks<PNGHIDE_COLORSPACE_VAR_LEN+PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    BinColorSpaceStart[ReadChunks-PNGHIDE_HEIGHT_VAR_LEN-PNGHIDE_WIDTH_VAR_LEN-2] = CurrentColorValue[0];
                }
                if (ReadChunks>=PNGHIDE_COLORSPACE_VAR_LEN+PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2 && ReadChunks<PNGHIDE_BIT_DEPH_VAR_LEN+PNGHIDE_COLORSPACE_VAR_LEN+PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    BinBitDephStart[ReadChunks-PNGHIDE_COLORSPACE_VAR_LEN-PNGHIDE_HEIGHT_VAR_LEN-PNGHIDE_WIDTH_VAR_LEN-2] = CurrentColorValue[0];
                }
                ReadChunks ++;
            }
        }
    }

    ReadChunks = 0;
    EndSignal = 0;
    ///<Start reading from the ending of the input image.
    for (Y=OriginalImage->Height-1; Y>=0 && EndSignal == 0; Y--) {
        CurrentOriginalRow = OriginalImage->ImageStart[Y];
        for (X=OriginalImage->Width-1; X>=0 && EndSignal == 0; X--) {
            CurrentOriginalPixel = &(CurrentOriginalRow[X*OriginalTotalChannels]);
            for (i=OriginalUsableChannels-1; i>=0; i--){

                if (ReadChunks == PNGHIDE_HEADER_SIZE){
                    EndSignal = 1; ///<We finished reading the header at the end of the image.
                }
                if (ReadChunks>=0 && ReadChunks <= 1){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);///<Read key bits.
                    BinCopy (KeyBitsEnd[ReadChunks],CurrentColorValue,2);
                }
                if (ReadChunks>1 && ReadChunks<PNGHIDE_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    BinWidthEnd[ReadChunks-2] = CurrentColorValue[0];
                }
                if (ReadChunks>=PNGHIDE_WIDTH_VAR_LEN+2 && ReadChunks<PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    BinHeightEnd[ReadChunks-PNGHIDE_WIDTH_VAR_LEN-2] = CurrentColorValue[0];
                }
                if (ReadChunks>=PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2 && ReadChunks<PNGHIDE_COLORSPACE_VAR_LEN+PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    BinColorSpaceEnd[ReadChunks-PNGHIDE_HEIGHT_VAR_LEN-PNGHIDE_WIDTH_VAR_LEN-2] = CurrentColorValue[0];
                }
                if (ReadChunks>=PNGHIDE_COLORSPACE_VAR_LEN+PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2 && ReadChunks<PNGHIDE_BIT_DEPH_VAR_LEN+PNGHIDE_COLORSPACE_VAR_LEN+PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    BinBitDephEnd[ReadChunks-PNGHIDE_COLORSPACE_VAR_LEN-PNGHIDE_HEIGHT_VAR_LEN-PNGHIDE_WIDTH_VAR_LEN-2] = CurrentColorValue[0];
                }
                ReadChunks ++;
            }
        }
    }
    if (KeyBitsStart[0][0] == 0 && KeyBitsStart[0][1] == 1 && KeyBitsStart[1][0] == 0 && KeyBitsStart[1][1] == 1){///<Check the key at the beggining of the input image.
        KeyOK++;
    }
    if (KeyBitsEnd[0][0] == 0 && KeyBitsEnd[0][1] == 1 && KeyBitsEnd[1][0] == 0 && KeyBitsEnd[1][1] == 1){///<Check the key at the beginning of the input image.
        KeyOK = KeyOK +2;
    }

    switch (KeyOK){
        case 0: ///<None of the keys passed the check.
            printf ("Error: Hidden Image Header is Corrupted\n");
            printf ("Enter Manual Settings?(y/n) ");
            scanf("%c",&opc);
            getchar();
            do {
                if (opc == 'y' || opc == 'Y'){
                    GetHeaderDetails (OutputImage);
                    return (-11);
                }
                else {
                    printf ("Aborting...\n");
                    return (-10);
                }
            }while (opc != 'n' && opc != 'N' && opc != 'y' && opc != 'y');
            break;
        case 1:///<The key at the end of the input image didn't pass the check.
        case 2:///<The key at the beginning of the input image didn't pass the check.
            printf ("Error: One of the hidden image headers is corrupted.\n");
            printf ("This probably means that the host image has been modified.\n");
            printf ("We might no be able to retrive the hidden image\n");
            if (KeyOK == 1){
                OutputImage->Height = BinBitStrToUint (BinHeightStart, PNGHIDE_HEIGHT_VAR_LEN);
                OutputImage->Width = BinBitStrToUint (BinWidthStart, PNGHIDE_WIDTH_VAR_LEN);
                OutputImage->ColorSpace = BinBitStrToUint (BinColorSpaceStart, PNGHIDE_COLORSPACE_VAR_LEN);
                OutputImage->BitDeph = BinBitStrToUint (BinBitDephStart, PNGHIDE_BIT_DEPH_VAR_LEN);
            }
            else {
                OutputImage->Height = BinBitStrToUint (BinHeightEnd, PNGHIDE_HEIGHT_VAR_LEN);
                OutputImage->Width = BinBitStrToUint (BinWidthEnd, PNGHIDE_WIDTH_VAR_LEN);
                OutputImage->ColorSpace = BinBitStrToUint (BinColorSpaceEnd, PNGHIDE_COLORSPACE_VAR_LEN);
                OutputImage->BitDeph = BinBitStrToUint (BinBitDephEnd, PNGHIDE_BIT_DEPH_VAR_LEN);
            }
            printf("Redundant Header information is:\n");
            PrintHeader (OutputImage);
            do{
                printf ("Proceed with this settings?(y/n) ");
                scanf("%c",&opc);
                getchar();
                if (opc == 'y' || opc == 'Y'){
                    return ((short int)KeyOK);
                }
                else {
                    printf ("Enter New Settings?(y/n) ");
                    scanf("%c",&opc);
                    getchar();
                    do {
                        if (opc == 'y' || opc == 'Y'){
                            GetHeaderDetails (OutputImage);
                            return (-11);
                        }
                        else {
                            printf ("Aborting...\n");
                            return (-10);
                        }
                    }while (opc != 'n' && opc != 'N' && opc != 'y' && opc != 'y');
                }
            }while (opc != 'n' && opc != 'N' && opc != 'y' && opc != 'y');

            break;
        case 3:///<Both keys passed the test.

            ///<Check both headers' information for missmatches.
            if (CompareBin (BinHeightStart,BinHeightEnd,PNGHIDE_HEIGHT_VAR_LEN) != 0){
                err = 1;
            }
            else{
                if (CompareBin (BinWidthStart,BinWidthEnd,PNGHIDE_WIDTH_VAR_LEN) != 0){
                    err = 1;
                }
                else {
                    if (CompareBin (BinColorSpaceStart,BinColorSpaceEnd,PNGHIDE_COLORSPACE_VAR_LEN) != 0){
                        err = 1;
                    }
                    else {
                        if (CompareBin (BinBitDephStart,BinBitDephEnd,PNGHIDE_BIT_DEPH_VAR_LEN) != 0){
                            err = 1;
                        }
                    }
                }
            }
            if (err ==  1){///<If the information of both headers doesn't match.
                printf ("Error: Image Header Mismatch\n");
                printf ("This probably means that the host image has been modified.\n");
                printf ("We might no be able to retrive the hidden image\n");
                printf ("Possible Oprions are:\n1:\n");

                OutputImage->Height = BinBitStrToUint (BinHeightStart, PNGHIDE_HEIGHT_VAR_LEN);
                OutputImage->Width = BinBitStrToUint (BinWidthStart, PNGHIDE_WIDTH_VAR_LEN);
                OutputImage->ColorSpace = BinBitStrToUint (BinColorSpaceStart, PNGHIDE_COLORSPACE_VAR_LEN);
                OutputImage->BitDeph = BinBitStrToUint (BinBitDephStart, PNGHIDE_BIT_DEPH_VAR_LEN);

                PrintHeader (OutputImage);

                printf ("2:\n");
                OutputImage->Height = BinBitStrToUint (BinHeightEnd, PNGHIDE_HEIGHT_VAR_LEN);
                OutputImage->Width = BinBitStrToUint (BinWidthEnd, PNGHIDE_WIDTH_VAR_LEN);
                OutputImage->ColorSpace = BinBitStrToUint (BinColorSpaceEnd, PNGHIDE_COLORSPACE_VAR_LEN);
                OutputImage->BitDeph = BinBitStrToUint (BinBitDephEnd, PNGHIDE_BIT_DEPH_VAR_LEN);

                PrintHeader (OutputImage);
                printf ("Please Enter the settings you wich to use:\n");
                GetHeaderDetails (OutputImage);

            }
            else {
                OutputImage->Height = BinBitStrToUint (BinHeightStart, PNGHIDE_HEIGHT_VAR_LEN);
                OutputImage->Width = BinBitStrToUint (BinWidthStart, PNGHIDE_WIDTH_VAR_LEN);

                OutputImage->ColorSpace = BinBitStrToUint (BinColorSpaceEnd, PNGHIDE_COLORSPACE_VAR_LEN);
                OutputImage->BitDeph = BinBitStrToUint (BinBitDephEnd, PNGHIDE_BIT_DEPH_VAR_LEN);

                PrintHeader (OutputImage);

            }

    }



    return 0;
}

/*! \brief Function that decodes the hidden image form the original.
 */
short int DecodeImages (Picture *OriginalImage, Picture *HiddenImage){
    unsigned char OriginalUsableChannels;///<Stores the amount of color channels in the input image that can be used to store information.
    unsigned char OriginalTotalChannels;///<Stores the total amount of color channels in the input image.
    unsigned char HiddenNeededChannels; ///<Stores the amount of channels needed by each pixel of the hidden image.

    switch (OriginalImage->ColorSpace){
        default:
            return -3;
            break;
        case PNG_COLOR_TYPE_RGBA:
            OriginalTotalChannels = 4;
            OriginalUsableChannels = 3;
            break;
        case PNG_COLOR_TYPE_RGB:
            OriginalTotalChannels = 3;
            OriginalUsableChannels = 3;
            break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            OriginalTotalChannels = 2;
            OriginalUsableChannels = 1;
            break;
        case PNG_COLOR_TYPE_GRAY:
            OriginalTotalChannels = 1;
            OriginalUsableChannels = 1;
            break;
    }
        switch (HiddenImage->ColorSpace){
        default:
            return -3;
            break;
        case PNG_COLOR_TYPE_RGBA:
            HiddenNeededChannels = 4;
            break;
        case PNG_COLOR_TYPE_RGB:
            HiddenNeededChannels = 3;
            break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            HiddenNeededChannels = 2;
            break;
        case PNG_COLOR_TYPE_GRAY:
        case PNG_COLOR_TYPE_PALETTE:
            HiddenNeededChannels = 1;
            break;
    }

    png_byte* CurrentOriginalRow,*CurrentHiddenRow;
    png_byte* CurrentOriginalPixel,*CurrentHiddenlPixel;
    int64_t OriginalX,OriginalY; ///<Our current position on the input image.
    int64_t HiddenX = 0,HiddenY = 0;///<Our current position on the output image.
    uint64_t ReadChunks = 0;///<Counts how many image channels we have stracted information from.
    register short int i;
    short int CurrentHiddenBit = 0;


    unsigned char CurrentColorValue [OriginalImage->BitDeph];
    unsigned char HiddenColorValue [HiddenImage->BitDeph];
    unsigned char EndSignal = 0;
    unsigned char CurrentHiddenChannel;

    CurrentHiddenRow = HiddenImage->ImageStart[HiddenY];

    CurrentHiddenlPixel = &(CurrentHiddenRow[HiddenX*HiddenNeededChannels]);

    //IDA
    for (OriginalY=0; OriginalY<(OriginalImage->Height) && EndSignal == 0; OriginalY++) {
        CurrentOriginalRow = OriginalImage->ImageStart[OriginalY];
        for (OriginalX=0; OriginalX<(OriginalImage->Width) && EndSignal == 0; OriginalX++) {
            CurrentOriginalPixel = &(CurrentOriginalRow[OriginalX*OriginalTotalChannels]);
            for (i=0;i<OriginalUsableChannels;i++){

                if (ReadChunks > PNGHIDE_HEADER_SIZE-1){

                    if (CurrentHiddenBit == HiddenImage->BitDeph){
                        CurrentHiddenlPixel [CurrentHiddenChannel] = (png_byte) BinBitStrToUint(HiddenColorValue,HiddenImage->BitDeph);
                        CurrentHiddenBit =0;
                        CurrentHiddenChannel++;
                    }
                    if (CurrentHiddenChannel == HiddenNeededChannels){
                        CurrentHiddenChannel = 0;
                        HiddenX++;
                        if (HiddenX == HiddenImage->Width){
                            HiddenX = 0;
                            HiddenY = HiddenY +2;
                            if (HiddenY >= HiddenImage->Height){
                                EndSignal =1;
                                break;
                            }

                        CurrentHiddenRow = HiddenImage->ImageStart[HiddenY];
                        }

                    CurrentHiddenlPixel = &(CurrentHiddenRow[HiddenX*HiddenNeededChannels]);
                    }
                    IntToBitBinStr(CurrentOriginalPixel[i],CurrentColorValue,OriginalImage->BitDeph);
                    HiddenColorValue[CurrentHiddenBit] = CurrentColorValue[0];



                    CurrentHiddenBit++;

                }
                else{
                    ReadChunks++;
                }
            }
        }
    }

    ReadChunks = 0;
    EndSignal = 0;
    CurrentHiddenBit = 0;
    CurrentHiddenChannel = 0;
    HiddenX = 0;
    HiddenY = 1;
    CurrentHiddenRow = HiddenImage->ImageStart[HiddenY];
    CurrentHiddenlPixel = &(CurrentHiddenRow[HiddenX*HiddenNeededChannels]);
    //REGRESO
    for (OriginalY=OriginalImage->Height-1; OriginalY>=0 && EndSignal == 0; OriginalY--) {
        CurrentOriginalRow = OriginalImage->ImageStart[OriginalY];
        for (OriginalX=OriginalImage->Width-1; OriginalX>=0 && EndSignal == 0; OriginalX--) {
            CurrentOriginalPixel = &(CurrentOriginalRow[OriginalX*OriginalTotalChannels]);
            for (i=OriginalUsableChannels-1; i>=0 && EndSignal == 0;i--){

                if (ReadChunks > PNGHIDE_HEADER_SIZE-1){
                    if (CurrentHiddenBit == HiddenImage->BitDeph){
                        CurrentHiddenlPixel[CurrentHiddenChannel] = (png_byte)BinBitStrToUint(HiddenColorValue,HiddenImage->BitDeph);
                        CurrentHiddenBit =0;
                        CurrentHiddenChannel++;
                    }
                    if (CurrentHiddenChannel == HiddenNeededChannels){
                        CurrentHiddenChannel = 0;
                        HiddenX++;
                        if (HiddenX == HiddenImage->Width){
                            HiddenX = 0;
                            HiddenY = HiddenY +2;
                            if (HiddenY >= HiddenImage->Height){
                                EndSignal =1;
                                break;
                            }

                        CurrentHiddenRow = HiddenImage->ImageStart[HiddenY];

                        }

                    CurrentHiddenlPixel = &(CurrentHiddenRow[HiddenX*HiddenNeededChannels]);

                    }
                    IntToBitBinStr(CurrentOriginalPixel[i],CurrentColorValue,OriginalImage->BitDeph);
                    HiddenColorValue[CurrentHiddenBit] = CurrentColorValue[0];

                    CurrentHiddenBit++;




                }
                else{
                    ReadChunks++;
                }
            }
        }
    }


return 0;
}

