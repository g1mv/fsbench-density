void new_zip(void);
unsigned int add_file(char *filename);
unsigned int add_data(char *source, char *destination, unsigned int size, unsigned int eof);
unsigned int end_file(char *destination);
unsigned int end_zip(char *destination);

unsigned int qlz_deflate(unsigned char *source, unsigned char *destination, unsigned int bitoffset, unsigned int size, unsigned int last);
