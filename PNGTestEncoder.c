#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <png.h>
#include <zlib.h>

#define PNG_WIDTH_VAR_LEN 32
#define PNG_HEIGHT_VAR_LEN 32
#define PNG_COLORSPACE_VAR_LEN 8
#define PNG_BIT_DEPH_VAR_LEN 8

struct Picture {
    FILE *ImagePointer;
    char *FileLocation;
    uint32_t Width;
    uint32_t Height;
    png_byte ColorSpace;
    png_byte BitDeph;
    png_structp Payload;
    png_infop Header;
    png_bytep *ImageStart;
};
typedef struct Picture Picture;

short int loadPicture (Picture *Image);
short int readPicture (Picture *Image);
short int CheckFit (Picture OriginalImage, Picture HiddenImage);
short int IntToBitBinStr (uint32_t Input, unsigned char *Output, uint16_t length);
short int BitBinStrToInt (unsigned char *Input, uint32_t *Output, uint16_t length);
short int EncodeImages (Picture *OriginalImage, Picture *HiddenImage);
void FreeImage (Picture *Image);
short int WriteOutput (Picture *Image);

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

    if (OriginalImage.ColorSpace == PNG_COLOR_TYPE_PALETTE){
        printf ("Error: Palette mode is not supported\n");
        return -3;
    }

    if (OriginalImage.BitDeph < 8){
        printf ("Error: Bit Deph must be >= 8\n");
        return -3;
    }

    if (CheckFit(OriginalImage,HiddenImage)!=0){
        printf ("Error: Not enough space available to hide image\n");
        return -2;
    }

    if (err = readPicture (&OriginalImage) != 0)
        return err;

    if (err = readPicture (&HiddenImage) != 0)
        return err;

    OutputImage = OriginalImage;
    OutputImage.ImagePointer = NULL;
    OutputImage.FileLocation = argv [3];

    if (err = EncodeImages (&OriginalImage, &HiddenImage) != 0)
        return err;

    if (err = WriteOutput (&OutputImage) !=0 )
        return err;

    FreeImage (&OriginalImage);
    FreeImage (&HiddenImage);

    fclose (OriginalImage.ImagePointer);
    fclose (OutputImage.ImagePointer);
    fclose (HiddenImage.ImagePointer);
return 0;
}

short int loadPicture (Picture *Image) {
        Image->ImagePointer = fopen (Image->FileLocation,"rb");
        if (!Image->ImagePointer){
            printf ("Error: Couldn't open image\n");
            return (-1);
        }
        char FileSignature[8];
        if (fread(FileSignature, 1, 8, Image->ImagePointer) != 8){
            printf ("Error: Couldn't read image\n");
            return (-4);
        }
        if (png_sig_cmp(FileSignature, 0, 8) != 0){
            printf ("Error: Unsuported file type\n");
            return (-5);
        }

        Image->Payload = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        Image->Header = png_create_info_struct(Image->Payload);
        if (Image->Payload == NULL || Image->Header == NULL){
            printf ("Malloc Error\n");
            if (Image->Header == NULL){
                  png_destroy_read_struct(&(Image->Payload),NULL,NULL);
            }
            return (-2);
        }
        png_init_io(Image->Payload, Image->ImagePointer);
        png_set_sig_bytes(Image->Payload, 8);
        png_read_info(Image->Payload, Image->Header);

        Image->Width = png_get_image_width(Image->Payload, Image->Header);
        Image->Height = png_get_image_height(Image->Payload, Image->Header);
        Image->ColorSpace = png_get_color_type(Image->Payload, Image->Header);
        Image->BitDeph = png_get_bit_depth(Image->Payload, Image->Header);

    return 0;
}

short int readPicture (Picture *Image) {
    register uint32_t i;
    uint32_t j;
    uint64_t RowBytes;
    RowBytes = png_get_rowbytes(Image->Payload,Image->Header);


    Image->ImageStart = (png_bytep*) malloc(sizeof(png_bytep) * Image->Height);
    if (Image->ImageStart == NULL){
        printf ("Malloc Error\n");
        return -2;
    }
    for (i=0; i<Image->Height; i++){
        Image->ImageStart[i] = (png_byte*) malloc(RowBytes);
        if (Image->ImageStart[i] == NULL){
            for (j=0; j<i; j++){
                free (Image->ImageStart[j]);
            }
            printf ("Malloc Error\n");
            return -2;
        }
    }
    png_read_image(Image->Payload, Image->ImageStart);

    return 0;

}

short int CheckFit (Picture OriginalImage, Picture HiddenImage){
    short int OriginalUsableChannels;
    short int HiddenNeededChannels;
    unsigned long AvailableBits;
    unsigned long NeededBits;
    switch (OriginalImage.ColorSpace){
        default:
            return -3;
            break;
        case PNG_COLOR_TYPE_RGB:
        case PNG_COLOR_TYPE_RGBA:
            OriginalUsableChannels = 3;
            break;
        case PNG_COLOR_TYPE_GRAY:
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            OriginalUsableChannels = 1;
            break;
    }
    AvailableBits = (OriginalImage.Width * OriginalImage.Height) * OriginalUsableChannels;

    switch (HiddenImage.ColorSpace){
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
    NeededBits = 2*(PNG_WIDTH_VAR_LEN + PNG_HEIGHT_VAR_LEN + PNG_COLORSPACE_VAR_LEN + PNG_BIT_DEPH_VAR_LEN);
    NeededBits = NeededBits + (HiddenImage.Width * HiddenImage.Height * HiddenNeededChannels * HiddenImage.BitDeph)+ 4;
    printf ("Needed Bits = %lu\n",NeededBits);
    printf ("Available Bits = %lu\n",AvailableBits);

    if (NeededBits > AvailableBits){
        return -1;
    }
return 0;
}

short int IntToBitBinStr (uint32_t Input, unsigned char *Output, uint16_t length){

    register short int i=0;

    if (Output == NULL){
        return -3;
    }

    if (length == 1) {
        Output[0] = Input%2;
        return 0;
    }

    while (Input > 0){
        if (i>length){
            printf ("Error: String too short\n");
            return -7;
        }
        Output[i] = Input%2;
        Input = Input/2;
        i++;
    }

    while (i < length){
        Output[i]=0;
        i++;
    }
    return 0;
}

short int BitBinStrToInt (unsigned char *Input, uint32_t *Output, uint16_t length){

    register short int i=0;
    uint64_t CurrentPower = 1;
    *Output = 0;

    if (Input == NULL){
        return -3;
    }

    if (length > 32){
        printf ("Error: String too long\n");
        return -7;
    }
    for (i=0; i<length; i++){
        *Output = *Output + (Input[i] * CurrentPower);
        CurrentPower = CurrentPower*2;
    }

    return 0;
}

short int BitBinStrToByte (unsigned char *Input, png_byte *Output, uint16_t length){

    register short int i=0;
    uint64_t CurrentPower = 1;
    *Output = 0;

    if (Input == NULL){
        return -3;
    }

    if (length > 8){
        printf ("Error: String too long\n");
        return -7;
    }
    for (i=0; i<length; i++){
        *Output = *Output + (Input[i] * CurrentPower);
        CurrentPower = CurrentPower*2;
    }

    return 0;
}

short int EncodeImages (Picture *OriginalImage, Picture *HiddenImage){
    short int OriginalUsableChannels;
    short int OriginalTotalChannels;
    short int HiddenNeededChannels;

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
    int64_t OriginalX,OriginalY,HiddenX = 0,HiddenY = 0;
    register short int i,j = 0,k = 0,l = 0;
    short int CurrentHiddenBit = 0,CurrentHiddenChannel=0;

    unsigned char EncodedWidth[PNG_WIDTH_VAR_LEN];
    unsigned char EncodedHeight[PNG_HEIGHT_VAR_LEN];
    unsigned char EncodedColorSpace[PNG_COLORSPACE_VAR_LEN];
    unsigned char EncodedBitDeph[PNG_BIT_DEPH_VAR_LEN];
    unsigned char CurrentColorValue [OriginalImage->BitDeph];
    unsigned char HiddenColorValue [HiddenImage->BitDeph];
    unsigned char EndSignal = 0;

    IntToBitBinStr (HiddenImage->Width, EncodedWidth, PNG_WIDTH_VAR_LEN);
    IntToBitBinStr (HiddenImage->Height, EncodedHeight, PNG_HEIGHT_VAR_LEN);
    IntToBitBinStr (HiddenImage->ColorSpace, EncodedColorSpace, PNG_COLORSPACE_VAR_LEN);
    IntToBitBinStr (HiddenImage->BitDeph, EncodedBitDeph, PNG_BIT_DEPH_VAR_LEN);

    CurrentHiddenRow = HiddenImage->ImageStart[HiddenY];
    CurrentHiddenlPixel = &(CurrentHiddenRow[HiddenX*HiddenNeededChannels]);
    //IDA
    for (OriginalY=0; OriginalY<(OriginalImage->Height) && EndSignal == 0; OriginalY++) {
        CurrentOriginalRow = OriginalImage->ImageStart[OriginalY];
        for (OriginalX=0; OriginalX<(OriginalImage->Width) && EndSignal == 0; OriginalX++) {
            CurrentOriginalPixel = &(CurrentOriginalRow[OriginalX*OriginalTotalChannels]);
            for (i=0;i<OriginalUsableChannels;i++){


                //printf ("i = %d ",i);
                //printf ("OX = %ld ",OriginalX);
                //printf ("OY = %ld\n",OriginalY);
                if (j>=0 && j <=1){
printf ("X = %lu ",OriginalX);
            printf ("X = %lu ",OriginalY);
            printf("ColorOr %d ",CurrentOriginalPixel[i]);
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue[0] = 0;
                    CurrentColorValue[1] = 1;
                    BitBinStrToByte (CurrentColorValue, &(CurrentOriginalPixel[i]), OriginalImage->BitDeph);
                    printf("ColorNEW %d\n",CurrentOriginalPixel[i]);
                }
                if (j>1 && j<PNG_WIDTH_VAR_LEN+2){
                   printf ("X = %lu ",OriginalX);
            printf ("X = %lu ",OriginalY);
            printf("ColorOr %d ",CurrentOriginalPixel[i]);
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue [0] = EncodedWidth[j-2];
                    BitBinStrToByte (CurrentColorValue, &(CurrentOriginalPixel[i]), OriginalImage->BitDeph);
                    printf("ColorNEW %d\n",CurrentOriginalPixel[i]);
                }
                if (j>=PNG_WIDTH_VAR_LEN+2 && j<PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2){
                   printf ("X = %lu ",OriginalX);
            printf ("X = %lu ",OriginalY);
            printf("ColorOr %d ",CurrentOriginalPixel[i]);
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue [0] = EncodedHeight[j-PNG_WIDTH_VAR_LEN-2];
                    BitBinStrToByte (CurrentColorValue, &(CurrentOriginalPixel[i]), OriginalImage->BitDeph);
                    printf("ColorNEW %d\n",CurrentOriginalPixel[i]);
                }
                if (j>=PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2 && j<PNG_COLORSPACE_VAR_LEN+PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2){
                    printf ("X = %lu ",OriginalX);
            printf ("X = %lu ",OriginalY);
            printf("ColorOr %d ",CurrentOriginalPixel[i]);
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue [0] = EncodedColorSpace[j-PNG_HEIGHT_VAR_LEN-PNG_WIDTH_VAR_LEN-2];
                    BitBinStrToByte (CurrentColorValue, &(CurrentOriginalPixel[i]), OriginalImage->BitDeph);
                    printf("ColorNEW %d\n",CurrentOriginalPixel[i]);
                }
                if (j>=PNG_COLORSPACE_VAR_LEN+PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2 && j<PNG_BIT_DEPH_VAR_LEN+PNG_COLORSPACE_VAR_LEN+PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2){
                    printf ("X = %lu ",OriginalX);
            printf ("X = %lu ",OriginalY);
            printf("ColorOr %d ",CurrentOriginalPixel[i]);
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue [0] = EncodedBitDeph[j-PNG_COLORSPACE_VAR_LEN-PNG_HEIGHT_VAR_LEN-PNG_WIDTH_VAR_LEN-2];
                    BitBinStrToByte (CurrentColorValue, &(CurrentOriginalPixel[i]), OriginalImage->BitDeph);
                    printf("ColorNEW %d\n",CurrentOriginalPixel[i]);
                }
                if (j>=PNG_BIT_DEPH_VAR_LEN+PNG_COLORSPACE_VAR_LEN+PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2){
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
                            printf ("oX = %lu ",OriginalX);
                        printf ("oY = %lu ",OriginalY);
                        printf ("oC = %d ",i);

                        printf ("hX = %lu ",HiddenX);
                        printf ("hY = %lu ",HiddenY);
                        printf ("hC = %d ",CurrentHiddenChannel);

                        printf ("ov = %lu ",CurrentOriginalPixel[i]);
                        printf ("nv = %lu\n",CurrentOriginalPixel[i]);

                                printf ("END!!!!\n");
                                EndSignal = 1;
                                break;
                            }
                            CurrentHiddenRow = HiddenImage->ImageStart[HiddenY];
                        }


                        CurrentHiddenlPixel = &(CurrentHiddenRow[HiddenX*HiddenNeededChannels]);
                        IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                        IntToBitBinStr (CurrentHiddenlPixel[CurrentHiddenChannel], HiddenColorValue, HiddenImage->BitDeph);
                        CurrentColorValue[0] = HiddenColorValue[CurrentHiddenBit];
                        BitBinStrToByte (CurrentColorValue, &(CurrentOriginalPixel[i]), OriginalImage->BitDeph);
                        CurrentHiddenBit++;


                    }
                else{
                    j++;
                }

            }
        }
    }
printf ("==================================================\n");
    EndSignal = 0;
    j = 0;
    k = 0;
    l = 0;
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
                //printf ("i = %d ",i);
                //printf ("OX = %ld ",OriginalX);
                //printf ("OY = %ld\n",OriginalY);
                if (j>=0 && j <=1){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue[0] = 0;
                    CurrentColorValue[1] = 1;
                    BitBinStrToByte (CurrentColorValue, &(CurrentOriginalPixel[i]), OriginalImage->BitDeph);
                }
                if (j>1 && j<PNG_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue [0] = EncodedWidth[j-2];
                    BitBinStrToByte (CurrentColorValue, &(CurrentOriginalPixel[i]), OriginalImage->BitDeph);
                }
                if (j>=PNG_WIDTH_VAR_LEN+2 && j<PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue [0] = EncodedHeight[j-PNG_WIDTH_VAR_LEN-2];
                    BitBinStrToByte(CurrentColorValue, &(CurrentOriginalPixel[i]), OriginalImage->BitDeph);
                }
                if (j>=PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2 && j<PNG_COLORSPACE_VAR_LEN+PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue [0] = EncodedColorSpace[j-PNG_HEIGHT_VAR_LEN-PNG_WIDTH_VAR_LEN-2];
                    BitBinStrToByte (CurrentColorValue, &(CurrentOriginalPixel[i]), OriginalImage->BitDeph);
                }
                if (j>=PNG_COLORSPACE_VAR_LEN+PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2 && j<PNG_BIT_DEPH_VAR_LEN+PNG_COLORSPACE_VAR_LEN+PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    CurrentColorValue [0] = EncodedBitDeph[j-PNG_COLORSPACE_VAR_LEN-PNG_HEIGHT_VAR_LEN-PNG_WIDTH_VAR_LEN-2];
                    BitBinStrToByte (CurrentColorValue, &(CurrentOriginalPixel[i]), OriginalImage->BitDeph);
                }
                if (j>=PNG_BIT_DEPH_VAR_LEN+PNG_COLORSPACE_VAR_LEN+PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2){
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
                                    printf ("oX = %lu ",OriginalX);
                        printf ("oY = %lu ",OriginalY);
                        printf ("oC = %d ",i);

                        printf ("hX = %lu ",HiddenX);
                        printf ("hY = %lu ",HiddenY);
                        printf ("hC = %d ",CurrentHiddenChannel);

                        printf ("ov = %lu ",CurrentOriginalPixel[i]);
                        printf ("nv = %lu\n",CurrentOriginalPixel[i]);
                                printf ("END!!!!\n");
                                EndSignal = 1;
                                break;
                            }
                            CurrentHiddenRow = HiddenImage->ImageStart[HiddenY];
                        }



                        CurrentHiddenlPixel = &(CurrentHiddenRow[HiddenX*HiddenNeededChannels]);
                        IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                        IntToBitBinStr (CurrentHiddenlPixel[CurrentHiddenChannel], HiddenColorValue, HiddenImage->BitDeph);
                        CurrentColorValue[0] = HiddenColorValue[CurrentHiddenBit];
                        BitBinStrToByte (CurrentColorValue, &(CurrentOriginalPixel[i]), OriginalImage->BitDeph);
                        CurrentHiddenBit++;


                }
                else{
                    j++;
                }
                //printf("iteration Ox = %lu  Oy = %lu i = %d\n",OriginalX,OriginalY,i);
            }
        }
    }
    printf ("H bd = %d\n",HiddenImage->BitDeph);
return 0;
}

void FreeImage (Picture *Image) {
    uint32_t i;
    uint64_t RowBytes;
    RowBytes = png_get_rowbytes(Image->Payload,Image->Header);

    for (i=0; i<Image->Height; i++){
        free (Image->ImageStart[i]);
    }
    free (Image->ImageStart);
}

short int WriteOutput (Picture *Image) {
        Image->ImagePointer = fopen (Image->FileLocation,"wb");
        if (!Image->ImagePointer){
            printf ("Error: Couldn't write image\n");
            return (-1);
        }

        Image->Payload = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        Image->Header = png_create_info_struct(Image->Payload);
        if (Image->Payload == NULL || Image->Header == NULL){
            printf ("Malloc Error\n");
            if (Image->Header == NULL){
                  png_destroy_write_struct(&(Image->Payload),NULL);
            }
            return (-2);
        }
        png_init_io(Image->Payload, Image->ImagePointer);

        png_set_IHDR(Image->Payload, Image->Header, Image->Width, Image->Height, Image->BitDeph, Image->ColorSpace, PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

        png_write_info(Image->Payload, Image->Header);

        png_write_image(Image->Payload, Image->ImageStart);
        png_write_end(Image->Payload, NULL);

    return 0;
}
