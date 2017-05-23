#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <png.h>
#include <zlib.h>

#include <PNGhideDataDefinitions.h>
#include <PNGhideFileOperations.h>
#include <PNGhideMiscFunctions.h>

double CalcMSE (Picture OriginalImage, Picture AlteredImage);
double CalcPSNR (double MSE, short int bitDeph);
double CalcSSIM (Picture OriginalImage, Picture AlteredImage);

struct SSIMVals {
    double Mu;
    double Sigma;
};

typedef struct SSIMVals SSIMVals;

int main (int argc, char **argv) {
    if (argc != 3){
        printf ("Error: Ussage: <Image.png> <Image.png>\n");
        return (-3);
    }
    Picture OriginalImage;
    Picture AlteredImage;
    int err;

    OriginalImage.FileLocation = argv[1];
    AlteredImage.FileLocation = argv [2];

    err = loadPicture (&OriginalImage);
    if (err != 0)
        return (err);

    err = loadPicture (&AlteredImage);
    if (err != 0)
        return (err);

    if (OriginalImage.ColorSpace == PNG_COLOR_TYPE_PALETTE || AlteredImage.ColorSpace == PNG_COLOR_TYPE_PALETTE){
        printf ("Error: Palette mode is not supported\n");
        return -3;
    }

    err = readPicture (&OriginalImage);
    if (err != 0)
        return err;

    err = readPicture (&AlteredImage);
    if (err != 0)
        return err;

    if (OriginalImage.Width != AlteredImage.Width){
        printf("Error: Image width missmath\n");
        err = -10;
    }
    if (OriginalImage.Height != AlteredImage.Height){
        printf("Error: Image height missmath\n");
        err = -10;
    }
    if (OriginalImage.BitDeph != AlteredImage.BitDeph){
        printf("Error: Image bit deph missmath\n");
        err = -10;
    }
    if (OriginalImage.ColorSpace != AlteredImage.ColorSpace){
        printf("Error: Image color space missmath\n");
        err = -10;
    }

    double MSE, PSNR, SSIM;

    if (err == 0){
        MSE = CalcMSE(OriginalImage,AlteredImage);
        PSNR = CalcPSNR(MSE,OriginalImage.BitDeph);
        SSIM = CalcSSIM(OriginalImage,AlteredImage);

        printf ("MSE = %f\n",MSE);
        printf ("PSNR = %f\n",PSNR);
        printf ("SSIM = %f\n",SSIM);
    }
    FreeImage (&OriginalImage);
    FreeImage (&AlteredImage);

    fclose (OriginalImage.ImagePointer);
    fclose (AlteredImage.ImagePointer);

return 0;
}

double CalcMSE (Picture OriginalImage, Picture AlteredImage){
    double MSE = 0;
    uint32_t X,Y;
    short int ImageChannels = GetUsableChannels(&OriginalImage);
    short int TotalChannels = GetTotalChannels(&OriginalImage);
    short int CurrentChannel;

    png_byte* CurrentOriginalRow; ///<Pointer to the beginning of a row's array of pixels.
    png_byte* CurrentOriginalPixel; ///<Pointer to the input image's pixel struct.

    png_byte* CurrentAlteredRow; ///<Pointer to the beginning of a row's array of pixels.
    png_byte* CurrentAlteredPixel; ///<Pointer to the input image's pixel struct.



    for (Y=0; Y<(OriginalImage.Height); Y++) {

        CurrentOriginalRow = OriginalImage.ImageStart[Y];
        CurrentAlteredRow = AlteredImage.ImageStart[Y];
        for (X=0; X<(OriginalImage.Width); X++) {
            CurrentOriginalPixel = &(CurrentOriginalRow[X*TotalChannels]);
            CurrentAlteredPixel = &(CurrentAlteredRow[X*TotalChannels]);

            for (CurrentChannel=0;CurrentChannel<ImageChannels;CurrentChannel++){
                MSE = MSE + pow((CurrentOriginalPixel[CurrentChannel]-CurrentAlteredPixel[CurrentChannel]),2);

            }
        }
    }
    MSE = MSE/(OriginalImage.Width*OriginalImage.Height*ImageChannels);


    return (MSE);
}

double CalcPSNR (double MSE, short int bitDeph) {
    double PSNR = 0;
    PSNR = 10*log10(pow((pow(2,bitDeph)-1),2)/MSE);
    return PSNR;
}


double CalcSSIM (Picture OriginalImage, Picture AlteredImage) {
    int i,N = 8;
//printf ("Hello\n");
    double SSIM = 0;
    int64_t X,Y;
    png_byte* CurrentOriginalRow; ///<Pointer to the beginning of a row's array of pixels.
    png_byte* CurrentOriginalPixel[N]; ///<Pointer to the input image's pixel struct.
    png_byte* CurrentAlteredRow; ///<Pointer to the beginning of a row's array of pixels.
    png_byte* CurrentAlteredPixel[N]; ///<Pointer to the input image's pixel struct.

    float C1 = pow ((pow (2,OriginalImage.BitDeph)-1) * .001,2);
    float C2 = C1;

    unsigned char CurrentColorSpace = 0;
    unsigned char ImageChannels = GetTotalChannels (&OriginalImage);
    unsigned char UsableColorSpaces = GetUsableChannels (&OriginalImage);

    SSIMVals Im1;
    SSIMVals Im2;
    double ChannelSSIM[UsableColorSpaces];
    double Covariance;


    for (CurrentColorSpace = 0; CurrentColorSpace<UsableColorSpaces; CurrentColorSpace++){
            ChannelSSIM[CurrentColorSpace] = 0;
    }

    for (Y=0; Y<(OriginalImage.Height); Y++) {
        CurrentOriginalRow = OriginalImage.ImageStart[Y];
        CurrentAlteredRow = AlteredImage.ImageStart[Y];
        for (X=0; X<(OriginalImage.Width-N); X++) {
            for (i = 0; i<N;i++){
                CurrentOriginalPixel[i] = &(CurrentOriginalRow[(X+i)*ImageChannels]);
                CurrentAlteredPixel[i] = &(CurrentAlteredRow[(X+i)*ImageChannels]);
            }
            for (CurrentColorSpace = 0; CurrentColorSpace<UsableColorSpaces; CurrentColorSpace++){
                Im1.Mu = 0;
                Im2.Mu = 0;
                Im1.Sigma = 0;
                Im2.Sigma = 0;
                Covariance = 0;
                for (i=0;i<N;i++){
                    Im1.Mu = Im1.Mu + CurrentOriginalPixel[i][CurrentColorSpace];
                    Im2.Mu = Im2.Mu + CurrentAlteredPixel[i][CurrentColorSpace];
                }
                Im1.Mu = Im1.Mu / N;
                Im2.Mu = Im2.Mu / N;
                //printf ("Mu = %f %f\n",Im1.Mu,Im2.Mu);
                for (i=0;i<N;i++){
                    Im1.Sigma = Im1.Sigma + pow(CurrentOriginalPixel[i][CurrentColorSpace] - Im1.Mu,2);
                    Im2.Sigma = Im2.Sigma + pow(CurrentAlteredPixel[i][CurrentColorSpace] - Im2.Mu,2);
                    Covariance = Covariance + (CurrentOriginalPixel[i][CurrentColorSpace] - Im1.Mu)*(CurrentAlteredPixel[i][CurrentColorSpace] - Im2.Mu);
                }
                Im1.Sigma = sqrt (Im1.Sigma/(N-1));
                Im2.Sigma = sqrt (Im2.Sigma/(N-1));
                Covariance = Covariance/(N-1);
                ChannelSSIM[CurrentColorSpace] = ChannelSSIM[CurrentColorSpace] + ((((2*Im1.Mu*Im2.Mu) + C1)*((2*Covariance)+C2))/(((pow(Im1.Mu,2)+pow(Im2.Mu,2))+C1)*((pow(Im1.Sigma,2)+pow(Im2.Sigma,2))+C2)));

            }
        }
    }
    for (CurrentColorSpace = 0; CurrentColorSpace<UsableColorSpaces; CurrentColorSpace++){
            ChannelSSIM[CurrentColorSpace] = ChannelSSIM[CurrentColorSpace]/(OriginalImage.Height*(OriginalImage.Width-N));
    }
    for (CurrentColorSpace = 0; CurrentColorSpace<UsableColorSpaces; CurrentColorSpace++){
            SSIM = SSIM + ChannelSSIM[CurrentColorSpace];
    }
    SSIM = SSIM/UsableColorSpaces;

    return SSIM;
}
