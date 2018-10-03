
#include <stdio.h>
#include <string.h>


#define  FILE_READ_PATH  "D:/"
#define  FILE_NAME  "pic.jpg"
#define  FILE_WRITE_PATH  "E:\\"


int main()
{
	FILE *pfw, *pfr;
	char data[4096];
	int br, bw;
	int ret;
	//char pathname[32];
	//char filename[32];
	char fname[64];
	int slen;

	//strcpy(filename, FILE_NAME);
	fname[0] = 0;
	strcat(fname, FILE_READ_PATH);
	strcat(fname, FILE_NAME);
	pfr = fopen(fname, "rb");
	if(!pfr) {
		printf("1:file open error\n");
		printf("code should not be here\n");
		while(1);
	}

	fname[0] = 0;
	strcat(fname, FILE_WRITE_PATH);
	strcat(fname, FILE_NAME);
	pfw = fopen(fname, "w");
	if(!pfw) {
		printf("2:file open error\n");
		printf("code should not be here\n");
		while(1);
	}

	br = fread(data, 1, 4096, pfr);
	bw = fwrite(data, 1, br, pfw);
	while(br < 4096) {
		br = fread(data, 1, 4096, pfr);
		bw = fwrite(data, 1, br, pfw);
	}

	fclose(pfr);
	fclose(pfw);
}

