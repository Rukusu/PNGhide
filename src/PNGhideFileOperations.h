
short int GetTotalChannels (Picture *Image) {
    short int Channels = 0;
    switch (Image->ColorSpace){
        default:
            Channels -3;
            break;
        case PNG_COLOR_TYPE_RGBA:
            Channels = 4;
            break;
        case PNG_COLOR_TYPE_RGB:
            Channels = 3;
            break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            Channels = 2;
            break;
        case PNG_COLOR_TYPE_GRAY:
        case PNG_COLOR_TYPE_PALETTE:
            Channels = 1;
            break;
    }
return (Channels);
}

short int GetUsableChannels (Picture *Image) {
    short int UsableChannels = 0;

    switch (Image->ColorSpace){
        default:
            UsableChannels = -3;
            break;
        case PNG_COLOR_TYPE_RGB:
        case PNG_COLOR_TYPE_RGBA:
            UsableChannels = 3;
            break;
        case PNG_COLOR_TYPE_GRAY:
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            UsableChannels = 1;
            break;
    }
    return (UsableChannels);
}

/*! \brief Checks if the image we want to hide fits inside the image we want to hide it in.
 */
short int CheckFit (Picture OriginalImage, Picture HiddenImage){
    char OriginalUsableChannels;
    char HiddenNeededChannels;
    uint64_t AvailableBits;
    uint64_t NeededBits;


    OriginalUsableChannels = GetUsableChannels(&OriginalImage);

    AvailableBits = (OriginalImage.Width * OriginalImage.Height) * OriginalUsableChannels;

    HiddenNeededChannels = GetTotalChannels(&HiddenImage);

    NeededBits = 2*PNGHIDE_HEADER_SIZE;
    NeededBits = NeededBits + (HiddenImage.Width * HiddenImage.Height * HiddenNeededChannels * HiddenImage.BitDeph)+ 4;

    if (NeededBits > AvailableBits){
        printf ("Error: Not enough space available to hide image\n");
        printf ("Needed Bits = %lu\n",NeededBits);
        printf ("Available Bits = %lu\n",AvailableBits);

        return -1;
    }
return 0;
}

/*! \brief Read an image's header in order to load it to ram.
 */
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

/*! \brief Alocates ram space in order to store an image.
 */
short int AllocatePictureSpace (Picture *Image){
    uint32_t i;
    uint32_t j;
    short int Channels;

    Channels = GetTotalChannels(Image);

    Image->ImageStart = (png_bytep*) malloc(sizeof(png_bytep) * Image->Height);
    if (Image->ImageStart == NULL){
        printf ("Malloc Error\n");
        return -2;
    }
    for (i=0; i<Image->Height; i++){
        Image->ImageStart[i] = (png_byte*) malloc (sizeof(png_byte)*Image->Width*Image->BitDeph*Channels);//malloc(RowBytes);
        if (Image->ImageStart[i] == NULL){
            for (j=0; j<i; j++){
                free (Image->ImageStart[j]);
            }
            printf ("Malloc Error\n");
            return -2;
        }
    }
    return 0;
}

/*! \brief Load an image to ram.
 */
short int readPicture (Picture *Image) {
    short int err;
    err = AllocatePictureSpace (Image);
    if (err != 0)
        return err;
    png_read_image(Image->Payload, Image->ImageStart);

    return 0;

}


/*! \brief Frees the ram space an image was stored in.
 */
void FreeImage (Picture *Image) {
    uint32_t i;
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


