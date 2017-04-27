
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
