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
double CalcPSNR (int64_t MSE, short int bitDeph);
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

    double MSE, PSNR, SSIM;
    MSE = CalcMSE(OriginalImage,AlteredImage);
    PSNR = CalcPSNR(MSE,OriginalImage.BitDeph);
    SSIM = CalcSSIM(OriginalImage,AlteredImage);

    printf ("MSE = %f\n",MSE);
    printf ("PSNR = %f\n",PSNR);
    printf ("SSIM = %f\n",SSIM);

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
    short int CurrentChannel;

    png_byte* CurrentOriginalRow; ///<Pointer to the beginning of a row's array of pixels.
    png_byte* CurrentOriginalPixel; ///<Pointer to the input image's pixel struct.

    png_byte* CurrentAlteredRow; ///<Pointer to the beginning of a row's array of pixels.
    png_byte* CurrentAlteredPixel; ///<Pointer to the input image's pixel struct.

    for (X=0; X<(OriginalImage.Width); X++) {
        for (Y=0; Y<(OriginalImage.Height); Y++) {
            CurrentOriginalRow = OriginalImage.ImageStart[Y];
            CurrentAlteredRow = AlteredImage.ImageStart[Y];
            CurrentOriginalPixel = &(CurrentOriginalRow[X*ImageChannels]);
            CurrentAlteredPixel = &(CurrentAlteredRow[X*ImageChannels]);
            for (CurrentChannel=0;CurrentChannel<ImageChannels;CurrentChannel++){
                MSE = MSE + pow((CurrentAlteredPixel[CurrentChannel]-CurrentOriginalPixel[CurrentChannel]),2);
            }
        }
    }
    //printf ("var = %f\n",MSE);
    //printf ("div = %d\n",(OriginalImage.Width*OriginalImage.Height*ImageChannels));
    MSE = MSE/(OriginalImage.Width*OriginalImage.Height*ImageChannels);


    return (MSE);
}

double CalcPSNR (int64_t MSE, short int bitDeph) {
    double PSNR = 0;
    //double MaxVal = (pow(2,bitDeph)-1);

    //printf("MAX = %f\n",MaxVal);

    //double ps1 = (20*log10(pow(MaxVal,2)));
    //double ps2 = (10*log10(MSE));
    //printf ("%f - %f\n",ps1,ps2);

    PSNR =  ((20*log10(pow((pow(2,bitDeph)-1),2)))-(10*log10(MSE)));
    return PSNR;
}


double CalcSSIM (Picture OriginalImage, Picture AlteredImage) {
    int i,N = 8;
//printf ("Hello\n");
    double SSIM = 0;
    int64_t X,Y;
    png_byte* CurrentOriginalRow; ///<Pointer to the beginning of a row's array of pixels.
    png_byte* CurrentOriginalPixel; ///<Pointer to the input image's pixel struct.
    png_byte* CurrentAlteredRow; ///<Pointer to the beginning of a row's array of pixels.
    png_byte* CurrentAlteredPixel; ///<Pointer to the input image's pixel struct.

    float C1 = pow ((pow (2,OriginalImage.BitDeph)-1) * .001,2);
    float C2 = C1;


    unsigned char CurrentColorSpace = 0;
    unsigned char UsableColorSpaces = GetUsableChannels (&OriginalImage);

    int AlteredValues [OriginalImage.Height][OriginalImage.Width-N][UsableColorSpaces];
    int OriginalValues[OriginalImage.Height][OriginalImage.Width-N][UsableColorSpaces];

    SSIMVals Im1; //[OriginalImage.Height][OriginalImage.Width-N][UsableColorSpaces];
    SSIMVals Im2; //[OriginalImage.Height][OriginalImage.Width-N][UsableColorSpaces];
    double ChannelSSIM[UsableColorSpaces];
    double Covar;// [OriginalImage.Height][OriginalImage.Width-N][UsableColorSpaces];



//printf ("F1\n");


    for (Y=0; Y<(OriginalImage.Height); Y++) {
        CurrentOriginalRow = OriginalImage.ImageStart[Y];
        CurrentAlteredRow = AlteredImage.ImageStart[Y];
        for (X=0; X<(OriginalImage.Width-N); X++) {
            CurrentOriginalPixel = &(CurrentOriginalRow[X*UsableColorSpaces]);
            CurrentAlteredPixel = &(CurrentAlteredRow[X*UsableColorSpaces]);
            for (CurrentColorSpace = 0; CurrentColorSpace<UsableColorSpaces; CurrentColorSpace++){
                OriginalValues [Y][X][CurrentColorSpace] = (int)CurrentOriginalPixel[CurrentColorSpace];
                AlteredValues [Y][X][CurrentColorSpace] = (int)CurrentAlteredPixel[CurrentColorSpace];
            }
        }
    }

    //double Den1,Den2, Num1,Num2;

    for (CurrentColorSpace = 0; CurrentColorSpace<UsableColorSpaces; CurrentColorSpace++){
            ChannelSSIM[CurrentColorSpace] = 0;
    }

    for (Y=0; Y<(OriginalImage.Height); Y++) {
        for (X=0; X<(OriginalImage.Width-N); X++) {
            for (CurrentColorSpace = 0; CurrentColorSpace<UsableColorSpaces; CurrentColorSpace++){
                    //printf ("X = %lu ",X);
                    //printf ("Y = %lu ",Y);
                    //printf ("CS = %d\n",CurrentColorSpace);
                Im1.Mu = 0;
                Im2.Mu = 0;
                Im1.Sigma = 0;
                Im2.Sigma = 0;
                Covar = 0;
                for (i=0;i<N;i++){
                    Im1.Mu = Im1.Mu + OriginalValues[Y][X+i][CurrentColorSpace];
                    Im2.Mu = Im2.Mu + AlteredValues[Y][X+i][CurrentColorSpace];
                }
                Im1.Mu = Im1.Mu / N;
                Im2.Mu = Im2.Mu / N;
                //printf ("Mu = %f %f\n",Im1.Mu,Im2.Mu);
                for (i=0;i<N;i++){
                    Im1.Sigma = Im1.Sigma + pow(OriginalValues[Y][X+i][CurrentColorSpace] - Im1.Mu,2);
                    Im2.Sigma = Im2.Sigma + pow(AlteredValues[Y][X+i][CurrentColorSpace] - Im2.Mu,2);
                    Covar = Covar + (OriginalValues[Y][X+i][CurrentColorSpace] - Im1.Mu)*(AlteredValues[Y][X+i][CurrentColorSpace] - Im2.Mu);
                }
                Im1.Sigma = sqrt (Im1.Sigma/(N-1));
                Im2.Sigma = sqrt (Im2.Sigma/(N-1));
                Covar = Covar/(N-1);
                //printf ("Sigma = %f %f\n",Im1.Sigma,Im2.Sigma);

                //Num1 = ((2*Im1.Mu*Im2.Mu) + C1);
                //Num2 = ((2*Covar)+C2);
                //Den1 = ((pow(Im1.Mu,2)+pow(Im2.Mu,2))+C1);
                //Den2 = ((pow(Im1.Sigma,2)+pow(Im2.Sigma,2))+C2);

                //printf ("Num %f * %f, Den = %f * %f\n",Num1,Num2,Den1,Den2);

                ChannelSSIM[CurrentColorSpace] = ChannelSSIM[CurrentColorSpace] + ((((2*Im1.Mu*Im2.Mu) + C1)*((2*Covar)+C2))/(((pow(Im1.Mu,2)+pow(Im2.Mu,2))+C1)*((pow(Im1.Sigma,2)+pow(Im2.Sigma,2))+C2)));
                //printf ("SSIM = %f\n",ChannelSSIM[CurrentColorSpace]);
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
