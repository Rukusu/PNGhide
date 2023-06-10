#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <png.h>
#include <zlib.h>
#include <errno.h>

#define PNG_WIDTH_VAR_LEN 32
#define PNG_HEIGHT_VAR_LEN 32
#define PNG_COLORSPACE_VAR_LEN 8
#define PNG_BIT_DEPH_VAR_LEN 8
#define HeaderSize ( PNG_WIDTH_VAR_LEN + PNG_HEIGHT_VAR_LEN + PNG_COLORSPACE_VAR_LEN + PNG_BIT_DEPH_VAR_LEN + 2 )

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
short int AllocatePictureSpace (Picture *Image);
short int readPicture (Picture *Image);
short int IntToBitBinStr (uint32_t Input, unsigned char *Output, uint16_t length);
short int BitBinStrToInt (unsigned char *Input, uint32_t *Output, uint16_t length);
short int FindHeader (Picture *OriginalImage, Picture *OutputImage);
short int DecodeImages (Picture *OriginalImage, Picture *HiddenImage);
void FreeImage (Picture *Image);
short int WriteOutput (Picture *Image);

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

short int AllocatePictureSpace (Picture *Image){
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

short int IntToBitBinStr (uint32_t Input, unsigned char *Output, uint16_t length){

    register short int i=0;

    if (Output == NULL){
        return -3;
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
    Output = 0;

    if (Input == NULL){
        return -3;
    }

    if (length > 32){
        printf ("Error: String too long\n");
        return -7;
    }
    for (i=0; i<length; i++){
        Output = Output + (Input[i] * CurrentPower);
        CurrentPower = CurrentPower*2;
    }
   // printf ("Convertion = %lu\n",*Output);
    return 0;
}

uint32_t BinBitStrToUint (unsigned char *Input, uint16_t length){
    uint32_t Output;
    register short int i=0;
    uint64_t CurrentPower = 1;
    Output = 0;

    if (Input == NULL){
        return -3;
    }

    if (length > 32){
        printf ("Error: String too long\n");
        return -7;
    }
    for (i=0; i<length; i++){
        Output = Output + (Input[i] * CurrentPower);
        CurrentPower = CurrentPower*2;
    }
    //printf ("Convertion = %lu\n",Output);
    return Output;
}

short int BinCopy (unsigned char *Output, unsigned char *Input, int Length){
    if (Length < 1 || Input == NULL || Output == NULL){
        return -3;
    }
    int i;

    for (i=0; i<Length;i++){
        *(Output+i)=*(Input+i);
    }

    return 0;
}

short int CompareBin (unsigned char *A, unsigned char *B, int Length){
    if (Length < 1 || A == NULL || B == NULL){
        return -3;
    }
    int i;

    for (i=0; i<Length;i++){
        if (*(A+i) != *(B+i)){
            return 1;//Not Equal
        }
    }

    return 0;
}

short int FindHeader (Picture *OriginalImage, Picture *OutputImage){
    short int OriginalUsableChannels;
    short int OriginalTotalChannels;
    short int err;
    uint64_t ReadChunks;
    int64_t X,Y;
    png_byte* CurrentOriginalRow;
    png_byte* CurrentOriginalPixel;

    char opc;
    register char i;
    unsigned char EndSignal;
    unsigned char KeyOK = 0;

    unsigned char CurrentColorValue [OriginalImage->BitDeph];
    unsigned char KeyBitsStart [2][2];
    unsigned char BinWidthStart[PNG_WIDTH_VAR_LEN];
    unsigned char BinHeightStart[PNG_HEIGHT_VAR_LEN];
    unsigned char BinColorSpaceStart[PNG_COLORSPACE_VAR_LEN];
    unsigned char BinBitDephStart[PNG_BIT_DEPH_VAR_LEN];

    unsigned char KeyBitsEnd [2][2];
    unsigned char BinWidthEnd[PNG_WIDTH_VAR_LEN];
    unsigned char BinHeightEnd[PNG_HEIGHT_VAR_LEN];
    unsigned char BinColorSpaceEnd[PNG_COLORSPACE_VAR_LEN];
    unsigned char BinBitDephEnd[PNG_BIT_DEPH_VAR_LEN];

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

    ReadChunks = 0;
    EndSignal = 0;
    for (Y=0; Y<(OriginalImage->Height) && EndSignal == 0; Y++) {
        CurrentOriginalRow = OriginalImage->ImageStart[Y];
        for (X=0; X<(OriginalImage->Width) && EndSignal == 0; X++) {
            CurrentOriginalPixel = &(CurrentOriginalRow[X*OriginalTotalChannels]);
            for (i=0;i<OriginalUsableChannels;i++){
            //printf("X %lu ",X);
            //printf("Y %lu ",Y);
            //printf("i %d \n",i);
                if (ReadChunks == HeaderSize){
                    EndSignal = 1;
                }
                if (ReadChunks>=0 && ReadChunks <= 1){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    BinCopy (KeyBitsStart[ReadChunks],CurrentColorValue,2);
                }
                if (ReadChunks>1 && ReadChunks<PNG_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    BinWidthStart[ReadChunks-2] = CurrentColorValue[0];
                }
                if (ReadChunks>=PNG_WIDTH_VAR_LEN+2 && ReadChunks<PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+1){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    BinHeightStart[ReadChunks-PNG_WIDTH_VAR_LEN-2] = CurrentColorValue[0];
                }
                if (ReadChunks>=PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2 && ReadChunks<PNG_COLORSPACE_VAR_LEN+PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    BinColorSpaceStart[ReadChunks-PNG_HEIGHT_VAR_LEN-PNG_WIDTH_VAR_LEN-2] = CurrentColorValue[0];
                }
                if (ReadChunks>=PNG_COLORSPACE_VAR_LEN+PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2 && ReadChunks<PNG_BIT_DEPH_VAR_LEN+PNG_COLORSPACE_VAR_LEN+PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    BinBitDephStart[ReadChunks-PNG_COLORSPACE_VAR_LEN-PNG_HEIGHT_VAR_LEN-PNG_WIDTH_VAR_LEN-2] = CurrentColorValue[0];
                }
                ReadChunks ++;
            }
        }
    }

    ReadChunks = 0;
    EndSignal = 0;
    for (Y=OriginalImage->Height-1; Y>=0 && EndSignal == 0; Y--) {
        CurrentOriginalRow = OriginalImage->ImageStart[Y];
        for (X=OriginalImage->Width-1; X>=0 && EndSignal == 0; X--) {
            CurrentOriginalPixel = &(CurrentOriginalRow[X*OriginalTotalChannels]);
            for (i=OriginalUsableChannels-1; i>=0; i--){
            //printf("X %lu ",X);
            //printf("Y %lu ",Y);
            //printf("i %d \n",i);
                if (ReadChunks == HeaderSize){
                    EndSignal = 1;
                }
                if (ReadChunks>=0 && ReadChunks <= 1){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    BinCopy (KeyBitsEnd[ReadChunks],CurrentColorValue,2);
                }
                if (ReadChunks>1 && ReadChunks<PNG_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    BinWidthEnd[ReadChunks-2] = CurrentColorValue[0];
                }
                if (ReadChunks>=PNG_WIDTH_VAR_LEN+2 && ReadChunks<PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    BinHeightEnd[ReadChunks-PNG_WIDTH_VAR_LEN-2] = CurrentColorValue[0];
                }
                if (ReadChunks>=PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2 && ReadChunks<PNG_COLORSPACE_VAR_LEN+PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    BinColorSpaceEnd[ReadChunks-PNG_HEIGHT_VAR_LEN-PNG_WIDTH_VAR_LEN-2] = CurrentColorValue[0];
                }
                if (ReadChunks>=PNG_COLORSPACE_VAR_LEN+PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2 && ReadChunks<PNG_BIT_DEPH_VAR_LEN+PNG_COLORSPACE_VAR_LEN+PNG_HEIGHT_VAR_LEN+PNG_WIDTH_VAR_LEN+2){
                    IntToBitBinStr (CurrentOriginalPixel[i], CurrentColorValue, OriginalImage->BitDeph);
                    BinBitDephEnd[ReadChunks-PNG_COLORSPACE_VAR_LEN-PNG_HEIGHT_VAR_LEN-PNG_WIDTH_VAR_LEN-2] = CurrentColorValue[0];
                }
                ReadChunks ++;
            }
        }
    }
    if (KeyBitsStart[0][0] == 0 && KeyBitsStart[0][1] == 1 && KeyBitsStart[1][0] == 0 && KeyBitsStart[1][1] == 1){
        KeyOK++;
    }
    if (KeyBitsEnd[0][0] == 0 && KeyBitsEnd[0][1] == 1 && KeyBitsEnd[1][0] == 0 && KeyBitsEnd[1][1] == 1){
        KeyOK = KeyOK +2;
    }
    //printf ("Key 1 = %d %d %d %d\n",KeyBitsStart[0][0],KeyBitsStart[0][1],KeyBitsStart[1][0],KeyBitsStart[1][1]);
    //printf ("Key 2 = %d %d %d %d\n",KeyBitsEnd[0][0],KeyBitsEnd[0][1],KeyBitsEnd[1][0],KeyBitsEnd[1][1]);

    switch (KeyOK){
        case 0:
            printf ("Error: Hidden Image Header is Corrupted\n");
            printf ("Enter Manual Settings?(y/n) ");
            scanf("%c",&opc);
            getchar();
            do {
                if (opc == 'y' || opc == 'Y'){
                    printf ("Width <px>: ");
                    scanf("%lu",&(OutputImage->Width));
                    printf ("Height <px>: ");
                    scanf("%lu",&(OutputImage->Height));
                    printf ("Bit Deph: ");
                    scanf("%d",&(OutputImage->BitDeph));
                    printf ("Color Space <dec>: ");
                    scanf("%d",&(OutputImage->ColorSpace));
                    return (-11);
                }
                else {
                    printf ("Aborting...\n");
                    return (-10);
                }
            }while (opc != 'n' && opc != 'N' && opc != 'y' && opc != 'y');
            break;
        case 1:
        case 2:
            printf ("Error: One of the hidden image headers is corrupted.\n");
            printf ("This probably means that the host image has been modified.\n");
            printf ("We might no be able to retrive the hidden image\n");
            if (KeyOK == 1){
                BitBinStrToInt (BinHeightStart, &(OutputImage->Height), PNG_HEIGHT_VAR_LEN);
                BitBinStrToInt (BinWidthStart, &(OutputImage->Width), PNG_WIDTH_VAR_LEN);
                BitBinStrToByte (BinColorSpaceStart, &(OutputImage->ColorSpace), PNG_COLORSPACE_VAR_LEN);
                BitBinStrToByte (BinBitDephStart, &(OutputImage->BitDeph), PNG_BIT_DEPH_VAR_LEN);
            }
            else {
                BitBinStrToInt (BinHeightEnd, &(OutputImage->Height), PNG_HEIGHT_VAR_LEN);
                BitBinStrToInt (BinWidthEnd, &(OutputImage->Width), PNG_WIDTH_VAR_LEN);
                BitBinStrToByte (BinColorSpaceEnd, &(OutputImage->ColorSpace), PNG_COLORSPACE_VAR_LEN);
                BitBinStrToByte (BinBitDephEnd, &(OutputImage->BitDeph), PNG_BIT_DEPH_VAR_LEN);
            }
            printf("Redundant Header information is:\n");
            printf("With = %lu px\n",OutputImage->Width);
            printf("Height = %lu px\n",OutputImage->Height);
            printf("Bit Deph = %d\n",OutputImage->BitDeph);
            printf("ColorSpace = %d ",OutputImage->ColorSpace);
            switch (OutputImage->ColorSpace){
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
                            printf ("Width <px>: ");
                            scanf("%lu",&(OutputImage->Width));
                            printf ("Height <px>: ");
                            scanf("%lu",&(OutputImage->Height));
                            printf ("Bit Deph: ");
                            scanf("%d",&(OutputImage->BitDeph));
                            printf ("Color Space <dec>: ");
                            scanf("%d",&(OutputImage->ColorSpace));
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
        case 3:
            if (CompareBin (BinHeightStart,BinHeightEnd,PNG_HEIGHT_VAR_LEN) != 0){
                err = 1;
            }
            else{
                if (CompareBin (BinWidthStart,BinWidthEnd,PNG_WIDTH_VAR_LEN) != 0){
                    err = 1;
                }
                else {
                    if (CompareBin (BinColorSpaceStart,BinColorSpaceEnd,PNG_COLORSPACE_VAR_LEN) != 0){
                        err = 1;
                    }
                    else {
                        if (CompareBin (BinBitDephStart,BinBitDephEnd,PNG_BIT_DEPH_VAR_LEN) != 0){
                            err = 1;
                        }
                    }
                }
            }
            if (err ==  1){
                printf ("Error: Image Header Mismatch\n");
                printf ("This probably means that the host image has been modified.\n");
                printf ("We might no be able to retrive the hidden image\n");
                printf ("Possible Oprions are:\n1:\n");

                BitBinStrToInt (BinHeightStart, &(OutputImage->Height), PNG_HEIGHT_VAR_LEN);
                BitBinStrToInt (BinWidthStart, &(OutputImage->Width), PNG_WIDTH_VAR_LEN);
                BitBinStrToByte (BinColorSpaceStart, &(OutputImage->ColorSpace), PNG_COLORSPACE_VAR_LEN);
                BitBinStrToByte (BinBitDephStart, &(OutputImage->BitDeph), PNG_BIT_DEPH_VAR_LEN);
                printf("With = %lu px\n",OutputImage->Width);
                printf("Height = %lu px\n",OutputImage->Height);
                printf("Bit Deph = %d\n",OutputImage->BitDeph);
                printf("ColorSpace = %d ",OutputImage->ColorSpace);
                switch (OutputImage->ColorSpace){
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
                printf ("2:\n");
                BitBinStrToInt (BinHeightEnd, &(OutputImage->Height), PNG_HEIGHT_VAR_LEN);
                BitBinStrToInt (BinWidthEnd, &(OutputImage->Width), PNG_WIDTH_VAR_LEN);
                BitBinStrToByte (BinColorSpaceEnd, &(OutputImage->ColorSpace), PNG_COLORSPACE_VAR_LEN);
                BitBinStrToByte (BinBitDephEnd, &(OutputImage->BitDeph), PNG_BIT_DEPH_VAR_LEN);
                printf("With = %lu px\n",OutputImage->Width);
                printf("Height = %lu px\n",OutputImage->Height);
                printf("Bit Deph = %d\n",OutputImage->BitDeph);
                printf("ColorSpace = %d ",OutputImage->ColorSpace);
                switch (OutputImage->ColorSpace){
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
                printf ("Please Enter the settings you wich to use:\n");
                printf ("Width <px>: ");
                scanf("%lu",&(OutputImage->Width));
                printf ("Height <px>: ");
                scanf("%lu",&(OutputImage->Height));
                printf ("Bit Deph: ");
                scanf("%d",&(OutputImage->BitDeph));
                printf ("Color Space <dec>: ");
                scanf("%d",&(OutputImage->ColorSpace));
                return (-11);
            }
            else {
                OutputImage->Height = BinBitStrToUint (BinHeightStart, PNG_HEIGHT_VAR_LEN);
                OutputImage->Width = BinBitStrToUint (BinWidthStart, PNG_WIDTH_VAR_LEN);

                OutputImage->ColorSpace = BinBitStrToUint (BinColorSpaceEnd, PNG_COLORSPACE_VAR_LEN);
                OutputImage->BitDeph = BinBitStrToUint (BinBitDephEnd, PNG_BIT_DEPH_VAR_LEN);

                printf ("Hidden Image Characteristics\n");
                printf("With = %lu px\n",OutputImage->Width);
                printf("Height = %lu px\n",OutputImage->Height);
                printf("Bit Deph = %d\n",OutputImage->BitDeph);
                printf("ColorSpace = %d ",OutputImage->ColorSpace);
                switch (OutputImage->ColorSpace){
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

            }

    }



    return 0;
}

short int DecodeImages (Picture *OriginalImage, Picture *HiddenImage){
    unsigned char OriginalUsableChannels;
    unsigned char OriginalTotalChannels;
    unsigned char HiddenNeededChannels;

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
    uint64_t ReadChunks = 0;
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
                //printf ("i = %d ",i);
                //printf ("OX = %ld ",OriginalX);
                //printf ("OY = %ld\n",OriginalY);
                //printf ("HX = %ld ",HiddenX);
                //printf ("HY = %ld\n",HiddenY);
                //printf ("Current Bit %d ",CurrentHiddenBit);
                //printf ("Current Cha %d\n",CurrentHiddenChannel);
                if (ReadChunks > HeaderSize-1){

                    if (CurrentHiddenBit == HiddenImage->BitDeph){
                        CurrentHiddenlPixel[CurrentHiddenChannel] = BinBitStrToUint(HiddenColorValue,HiddenImage->BitDeph);
                        //printf ("VAL = %d\n",CurrentHiddenlPixel[CurrentHiddenChannel]);

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
                                //printf ("END!!\n");
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
                //printf ("i = %d ",i);
                //printf ("OX = %ld ",OriginalX);
                //printf ("OY = %ld\n",OriginalY);
                //printf ("HX = %ld ",HiddenX);
                //printf ("HY = %ld\n",HiddenY);
                //printf ("Current Bit %d ",CurrentHiddenBit);
                //printf ("Current Cha %d\n",CurrentHiddenChannel);

                if (ReadChunks > HeaderSize-1){
                    if (CurrentHiddenBit == HiddenImage->BitDeph){
                        CurrentHiddenlPixel[CurrentHiddenChannel] = BinBitStrToUint(HiddenColorValue,HiddenImage->BitDeph);
                        //printf ("VAL = %d\n",CurrentHiddenlPixel[CurrentHiddenChannel]);

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
                                //printf ("END!!\n");
                                EndSignal =1;
                                break;
                            }

                        CurrentHiddenRow = HiddenImage->ImageStart[HiddenY];

                        }

CurrentHiddenlPixel = &(CurrentHiddenRow[HiddenX*HiddenNeededChannels]);

                    }
                    IntToBitBinStr(CurrentOriginalPixel[i],CurrentColorValue,OriginalImage->BitDeph);
                    HiddenColorValue[CurrentHiddenBit] = CurrentColorValue[0];
                    //CurrentHiddenlPixel = &(CurrentHiddenRow[HiddenX*HiddenNeededChannels]);

                    CurrentHiddenBit++;




                }
                else{
                    ReadChunks++;
                }
            }
        }
    }
    EndSignal = 0;

    /*for (OriginalY=0; OriginalY<(HiddenImage->Height) && EndSignal == 0; OriginalY++) {
        CurrentOriginalRow = HiddenImage->ImageStart[OriginalY];
        printf ("\n");
        for (OriginalX=0; OriginalX<(HiddenImage->Width) && EndSignal == 0; OriginalX++) {
            CurrentOriginalPixel = &(CurrentOriginalRow[OriginalX*2]);
            for (i=0;i<2;i++){
                printf ("%d\t",CurrentOriginalPixel[i]);
            }
        }
    }*/
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
        if (Image->ImagePointer == NULL){
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
