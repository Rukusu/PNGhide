#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <png.h>
#include <zlib.h>

#include "PNGhideDataDefinitions.h"
#include "PNGhideFileOperations.h"
#include "PNGhideMiscFunctions.h"

short int EncodeImages (Picture *OriginalImage, Picture *HiddenImage);
int main (int argc, char **argv) {
    if (argc != 4){
        printf ("Error: Ussage: <Original Image.png> <Image to Hide.png> <Output Image.png>\n");
        return (-3);
    }
    Picture OriginalImage;
    Picture HiddenImage;
    Picture OutputImage;
    int err;

    OriginalImage.FileLocation = argv[1];
    HiddenImage.FileLocation = argv [2];

    if (err = loadPicture(&HiddenImage) != 0)
        return (err);
    if (err = loadPicture (&OriginalImage)!= 0)
        return (err);

    if (OriginalImage.ColorSpace == PNG_COLOR_TYPE_PALETTE || HiddenImage.ColorSpace == PNG_COLOR_TYPE_PALETTE){
        printf ("Error: Palette mode is not supported\n");
        return -3;
    }

    if (OriginalImage.BitDeph < 8){
        printf ("Error: Bit Deph must be >= 8\n");
        return -3;
    }

    if (CheckFit(OriginalImage,HiddenImage)!=0){
        return -2;
    }

    if (err = readPicture (&OriginalImage) != 0){
        printf ("Could not read original\n");
        return err;
        }

    if (err = readPicture (&HiddenImage) != 0){
        printf ("Could not read hidden\n");
        return err;
        }

    OutputImage = OriginalImage;
    OutputImage.ImagePointer = NULL;
    OutputImage.FileLocation = argv [3];

    if (err = EncodeImages (&OriginalImage, &HiddenImage) != 0){
        return err;
    }

    if (err = WriteOutput (&OutputImage) !=0 ){
        return err;
    }


    FreeImage (&OriginalImage);
    FreeImage (&HiddenImage);

    fclose (OriginalImage.ImagePointer);
    fclose (OutputImage.ImagePointer);
    fclose (HiddenImage.ImagePointer);
return 0;
}

short int EncodeImages (Picture *OriginalImage, Picture *HiddenImage){

    png_byte* CurrentOriginalRow,*CurrentHiddenRow;
    png_byte* CurrentOriginalPixel,*CurrentHiddenPixel;
    int64_t OriginalX,OriginalY,HiddenX = 0,HiddenY = 0;
    short int  ReadChunks = 0;

    unsigned char EncodedWidth[PNGHIDE_WIDTH_VAR_LEN];
    unsigned char EncodedHeight[PNGHIDE_HEIGHT_VAR_LEN];
    unsigned char EncodedColorSpace[PNGHIDE_COLORSPACE_VAR_LEN];
    unsigned char EncodedBitDeph[PNGHIDE_BIT_DEPH_VAR_LEN];
    unsigned char CurrentColorValue [OriginalImage->BitDeph];
    unsigned char HiddenColorValue [HiddenImage->BitDeph];

    unsigned char HiddenNeededChannels;
    unsigned char CurrentHiddenBit = 0;
    unsigned char CurrentHiddenChannel = 0;
    unsigned char EndSignal = 0;

    unsigned char OriginalUsableChannels;
    unsigned char OriginalTotalChannels;
    char CurrentOriginalChannel;

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

    IntToBitBinStr (HiddenImage->Width, EncodedWidth, PNGHIDE_WIDTH_VAR_LEN);
    IntToBitBinStr (HiddenImage->Height, EncodedHeight, PNGHIDE_HEIGHT_VAR_LEN);
    IntToBitBinStr (HiddenImage->ColorSpace, EncodedColorSpace, PNGHIDE_COLORSPACE_VAR_LEN);
    IntToBitBinStr (HiddenImage->BitDeph, EncodedBitDeph, PNGHIDE_BIT_DEPH_VAR_LEN);

    CurrentHiddenRow = HiddenImage->ImageStart[HiddenY];
    CurrentHiddenPixel = &(CurrentHiddenRow[HiddenX*HiddenNeededChannels]);
    //IDA
    for (OriginalY=0; OriginalY<(OriginalImage->Height) && EndSignal == 0; OriginalY++) {
        CurrentOriginalRow = OriginalImage->ImageStart[OriginalY];
        for (OriginalX=0; OriginalX<(OriginalImage->Width) && EndSignal == 0; OriginalX++) {
            CurrentOriginalPixel = &(CurrentOriginalRow[OriginalX*OriginalTotalChannels]);
            for (CurrentOriginalChannel=0;CurrentOriginalChannel<OriginalUsableChannels;CurrentOriginalChannel++){


                if (ReadChunks>=0 && ReadChunks <=1){
                    IntToBitBinStr (CurrentOriginalPixel[CurrentOriginalChannel], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue[0] = 0;
                    CurrentColorValue[1] = 1;
                    CurrentOriginalPixel[CurrentOriginalChannel] = (png_byte)BinBitStrToUint (CurrentColorValue, OriginalImage->BitDeph);
                }
                if (ReadChunks>1 && ReadChunks<PNGHIDE_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[CurrentOriginalChannel], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue [0] = EncodedWidth[ReadChunks-2];
                    CurrentOriginalPixel[CurrentOriginalChannel] = (png_byte)BinBitStrToUint (CurrentColorValue, OriginalImage->BitDeph);
                }
                if (ReadChunks>=PNGHIDE_WIDTH_VAR_LEN+2 && ReadChunks<PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[CurrentOriginalChannel], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue [0] = EncodedHeight[ReadChunks-PNGHIDE_WIDTH_VAR_LEN-2];
                    CurrentOriginalPixel[CurrentOriginalChannel] = (png_byte)BinBitStrToUint (CurrentColorValue, OriginalImage->BitDeph);
                }
                if (ReadChunks>=PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2 && ReadChunks<PNGHIDE_COLORSPACE_VAR_LEN+PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[CurrentOriginalChannel], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue [0] = EncodedColorSpace[ReadChunks-PNGHIDE_HEIGHT_VAR_LEN-PNGHIDE_WIDTH_VAR_LEN-2];
                    CurrentOriginalPixel[CurrentOriginalChannel] = (png_byte)BinBitStrToUint (CurrentColorValue, OriginalImage->BitDeph);
                }
                if (ReadChunks>=PNGHIDE_COLORSPACE_VAR_LEN+PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2 && ReadChunks<PNGHIDE_BIT_DEPH_VAR_LEN+PNGHIDE_COLORSPACE_VAR_LEN+PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[CurrentOriginalChannel], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue [0] = EncodedBitDeph[ReadChunks-PNGHIDE_COLORSPACE_VAR_LEN-PNGHIDE_HEIGHT_VAR_LEN-PNGHIDE_WIDTH_VAR_LEN-2];
                    CurrentOriginalPixel[CurrentOriginalChannel] = (png_byte)BinBitStrToUint (CurrentColorValue, OriginalImage->BitDeph);
                }
                if (ReadChunks>=PNGHIDE_BIT_DEPH_VAR_LEN+PNGHIDE_COLORSPACE_VAR_LEN+PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2){
                        if (CurrentHiddenBit == HiddenImage->BitDeph){
                            CurrentHiddenBit = 0;
                            CurrentHiddenChannel ++;
                        }
                        if (CurrentHiddenChannel == HiddenNeededChannels){
                            CurrentHiddenChannel = 0;
                            HiddenX++;
                            if (HiddenX == HiddenImage->Width){
                                HiddenX = 0;
                                HiddenY = HiddenY+2;
                            }
                            if (HiddenY>=HiddenImage->Height){
                                EndSignal = 1;
                                break;
                            }
                            CurrentHiddenRow = HiddenImage->ImageStart[HiddenY];
                        }


                        CurrentHiddenPixel = &(CurrentHiddenRow[HiddenX*HiddenNeededChannels]);
                        IntToBitBinStr (CurrentOriginalPixel[CurrentOriginalChannel], CurrentColorValue, OriginalImage->BitDeph);
                        IntToBitBinStr (CurrentHiddenPixel[CurrentHiddenChannel], HiddenColorValue, HiddenImage->BitDeph);
                        CurrentColorValue[0] = HiddenColorValue[CurrentHiddenBit];
                        CurrentOriginalPixel[CurrentOriginalChannel] = (png_byte)BinBitStrToUint(CurrentColorValue, OriginalImage->BitDeph);
                        CurrentHiddenBit++;


                    }
                else{
                    ReadChunks++;
                }

            }
        }
    }

    EndSignal = 0;
    ReadChunks = 0;
    CurrentHiddenBit = 0;
    CurrentHiddenChannel = 0;
    HiddenX = 0;
    HiddenY = 1;
    CurrentHiddenRow = HiddenImage->ImageStart[HiddenY];
    CurrentHiddenPixel = &(CurrentHiddenRow[HiddenX*HiddenNeededChannels]);
    //REGRESO
    for (OriginalY=OriginalImage->Height-1; OriginalY>=0 && EndSignal == 0; OriginalY--) {
        CurrentOriginalRow = OriginalImage->ImageStart[OriginalY];
        for (OriginalX=OriginalImage->Width-1; OriginalX>=0 && EndSignal == 0; OriginalX--) {
            CurrentOriginalPixel = &(CurrentOriginalRow[OriginalX*OriginalTotalChannels]);
            for (CurrentOriginalChannel=OriginalUsableChannels-1; CurrentOriginalChannel>=0 && EndSignal == 0;CurrentOriginalChannel--){

                if (ReadChunks>=0 && ReadChunks<=1){
                    IntToBitBinStr (CurrentOriginalPixel[CurrentOriginalChannel], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue[0] = 0;
                    CurrentColorValue[1] = 1;
                    CurrentOriginalPixel[CurrentOriginalChannel] = (png_byte)BinBitStrToUint(CurrentColorValue, OriginalImage->BitDeph);
                }
                if (ReadChunks>1 && ReadChunks<PNGHIDE_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[CurrentOriginalChannel], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue [0] = EncodedWidth[ReadChunks-2];
                    CurrentOriginalPixel[CurrentOriginalChannel] = (png_byte)BinBitStrToUint(CurrentColorValue, OriginalImage->BitDeph);
                }
                if (ReadChunks>=PNGHIDE_WIDTH_VAR_LEN+2 && ReadChunks<PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[CurrentOriginalChannel], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue [0] = EncodedHeight[ReadChunks-PNGHIDE_WIDTH_VAR_LEN-2];
                    CurrentOriginalPixel[CurrentOriginalChannel] = (png_byte)BinBitStrToUint(CurrentColorValue, OriginalImage->BitDeph);
                }
                if (ReadChunks>=PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2 && ReadChunks<PNGHIDE_COLORSPACE_VAR_LEN+PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[CurrentOriginalChannel], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue [0] = EncodedColorSpace[ReadChunks-PNGHIDE_HEIGHT_VAR_LEN-PNGHIDE_WIDTH_VAR_LEN-2];
                    CurrentOriginalPixel[CurrentOriginalChannel] = (png_byte)BinBitStrToUint(CurrentColorValue, OriginalImage->BitDeph);
                }
                if (ReadChunks>=PNGHIDE_COLORSPACE_VAR_LEN+PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2 && ReadChunks<PNGHIDE_BIT_DEPH_VAR_LEN+PNGHIDE_COLORSPACE_VAR_LEN+PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[CurrentOriginalChannel], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue [0] = EncodedBitDeph[ReadChunks-PNGHIDE_COLORSPACE_VAR_LEN-PNGHIDE_HEIGHT_VAR_LEN-PNGHIDE_WIDTH_VAR_LEN-2];
                    CurrentOriginalPixel[CurrentOriginalChannel] = (png_byte)BinBitStrToUint(CurrentColorValue, OriginalImage->BitDeph);
                }
                if (ReadChunks>=PNGHIDE_BIT_DEPH_VAR_LEN+PNGHIDE_COLORSPACE_VAR_LEN+PNGHIDE_HEIGHT_VAR_LEN+PNGHIDE_WIDTH_VAR_LEN+2){
                        if (CurrentHiddenBit == HiddenImage->BitDeph){
                            CurrentHiddenBit = 0;
                            CurrentHiddenChannel ++;
                        }
                        if (CurrentHiddenChannel == HiddenNeededChannels){
                            CurrentHiddenChannel = 0;
                            HiddenX++;
                            if (HiddenX == HiddenImage->Width){
                                HiddenX = 0;
                                HiddenY = HiddenY+2;
                            }
                            if (HiddenY>=HiddenImage->Height){
                                EndSignal = 1;
                                break;
                            }
                            CurrentHiddenRow = HiddenImage->ImageStart[HiddenY];
                        }



                        CurrentHiddenPixel = &(CurrentHiddenRow[HiddenX*HiddenNeededChannels]);
                        IntToBitBinStr (CurrentOriginalPixel[CurrentOriginalChannel], CurrentColorValue, OriginalImage->BitDeph);
                        IntToBitBinStr (CurrentHiddenPixel[CurrentHiddenChannel], HiddenColorValue, HiddenImage->BitDeph);
                        CurrentColorValue[0] = HiddenColorValue[CurrentHiddenBit];
                        CurrentOriginalPixel[CurrentOriginalChannel] = (png_byte)BinBitStrToUint(CurrentColorValue, OriginalImage->BitDeph);
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




