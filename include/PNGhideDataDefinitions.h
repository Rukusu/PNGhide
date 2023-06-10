#define PNGHIDE_WIDTH_VAR_LEN 32
#define PNGHIDE_HEIGHT_VAR_LEN 32
#define PNGHIDE_COLORSPACE_VAR_LEN 8
#define PNGHIDE_BIT_DEPH_VAR_LEN 8
#define PNGHIDE_HEADER_SIZE ( PNGHIDE_WIDTH_VAR_LEN + PNGHIDE_HEIGHT_VAR_LEN + PNGHIDE_COLORSPACE_VAR_LEN + PNGHIDE_BIT_DEPH_VAR_LEN + 2 )

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
