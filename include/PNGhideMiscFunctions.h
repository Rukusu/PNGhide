/*! \brief Convert a binary number stored in a string to a decimal unsigned 32 bit number.
 */
uint32_t BinBitStrToUint (unsigned char *Input, uint16_t length){
    uint32_t Output;
    register short int i=0;
    uint64_t CurrentPower = 1;
    Output = 0;

    if (Input == NULL){
        return 0;
    }

    if (length > 32){
        printf ("Error: String too long\n");
        return 0;
    }

    for (i=0; i<length; i++){
        Output = Output + (Input[i] * CurrentPower);
        CurrentPower = CurrentPower*2;
    }
    return Output;
}

/*! \brief Convert a decimal unsigned 32 bit number to a binary number stored inside a string.
 */
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

/*! \brief Copies the content of an array to another.
 */
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

/*! \brief Compares the content of two arrays.
 */
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
