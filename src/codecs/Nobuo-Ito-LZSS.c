/*
 ============================================================================
 Name        : LZSS.c
 Author      : Nobuo Ito
 Version     : 1.0
 Copyright   : (C) 2012 Nobuo Ito
 Description : LZSS in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

void searchSW(unsigned int* retIndex, unsigned int* retMatch, char* refStart,char* refEnd,char* encStart,char* encEnd);
unsigned int matchString( char* refStart, char* refEnd, char* encStart,char* encEnd);
unsigned int uncompressLZSS(char* ret, char* src, unsigned int srcSize);
unsigned int compressLZSS(char* ret, char* src, unsigned int srcSize);

//12bit
#define REF_SIZE (4095+3)
//3bit
#define ENC_SIZE (7+3)
/*
int main(void) {
	FILE *fp;
	int fileSize =0;
	char* src=NULL;
	int compresSize=0;
	char* compres=NULL;
	char* uncompres=NULL;
	//圧縮するファイルを開く
	if ((fp = fopen("AgileShibuya.jpg", "rb")) == NULL) {
		printf("file open error!!\n");
		exit(EXIT_FAILURE);
	}
	fseek(fp , 0 , SEEK_END);
	fileSize = ftell(fp);
	fseek(fp , 0 , SEEK_SET);
	src= (char*)malloc(fileSize);
	uncompres = (char*)malloc(fileSize);
	compres= (char*)malloc(fileSize+(int)ceil((float)fileSize*2.0f));
	fread(src,1,fileSize,fp);
	fclose(fp);

	compresSize=compressLZSS(compres,src, fileSize);
	uncompressLZSS(uncompres,compres, compresSize);
	//解凍したファイルを保存
	if ((fp = fopen("uncompres_AgileShibuya.jpg", "wb")) == NULL) {
			printf("file open error!!\n");
			exit(EXIT_FAILURE);
	}
	fwrite(uncompres,1,fileSize,fp);
	fclose(fp);

	free(uncompres);
	free(src);
	free(compres);
	return EXIT_SUCCESS;
}*/
/*
　　　　　　　　文字検索関数

unsigned int* retIndex スライド辞書の後ろから数えたbyte数
unsigned int* retMatch 合致したbyte数
char* refStart スライド辞書開始位置
char* refEnd スライド辞書終了位置
char* encStart エンコ開始位置
char* encEnd エンコ終了位置
*/
void searchSW(unsigned int* retIndex, unsigned int* retMatch, char* refStart,char* refEnd,char* encStart,char* encEnd){
	unsigned int match=0;
	*retIndex=0;
	*retMatch=0;
	while(refStart < refEnd){
		match=matchString(refStart,refEnd,encStart,encEnd);
		if(match > *retMatch){
			*retMatch=match;
			*retIndex=refEnd-refStart;
		}
		refStart++;
	}
}
/*
     合致文字数取得
戻り値　合致byte数
char* refStart スライド辞書開始位置
char* refEnd スライド辞書終了位置
char* encStart エンコ開始位置
char* encEnd エンコ終了位置
*/
unsigned int matchString( char* refStart, char* refEnd, char* encStart,char* encEnd){
	int ret = 0;
	while(refStart[ret] == encStart[ret]){
		if(&refStart[ret] >= refEnd)
			break;
		if(&encStart[ret] >= encEnd)
			break;
		ret++;
	}
	return ret;
}
/*
          圧縮
戻り値 圧縮後サイズ
char* ret 圧縮先(メモリ確保しない)
char* src 圧縮元
unsigned int srcSize 圧縮元サイズ
*/
unsigned int compressLZSS(char* ret, char* src, unsigned int srcSize){
	unsigned int srcIndex=0;
	unsigned int retIndex=0;
	unsigned int index=0;
	unsigned int match=0;
	unsigned int notMatch=0;
	char* notMatchStart=NULL;
	char* refStart=NULL;
	char* encEnd=NULL;

	while(srcIndex < srcSize){
		if(srcIndex>=REF_SIZE)
			refStart=&src[srcIndex-REF_SIZE];
		else
			refStart=src;
		if(srcIndex>=srcSize-ENC_SIZE)
			encEnd=&src[srcSize];
		else
			encEnd=&src[srcIndex+ENC_SIZE];

		searchSW(&index,&match,refStart,&src[srcIndex],&src[srcIndex],encEnd);
		//圧縮かからない場合
		//127byteごとに制御コード1byteを挿入
		//0x80と127byteまでの文字数を1byteにおさめる
		if(match<3){
			if(notMatch==0){
				notMatchStart=&ret[retIndex];
				retIndex++;
				ret[retIndex]=src[srcIndex];
				retIndex++;
			}
			else if(notMatch==127){
				*notMatchStart=127;
				notMatch=0;

				notMatchStart=&ret[retIndex];
				retIndex++;
				ret[retIndex]=src[srcIndex];
				retIndex++;
			}
			else{
				ret[retIndex]=src[srcIndex];
				retIndex++;
			}
			notMatch++;
			srcIndex++;
		}
		//圧縮できる場合は2byteにslide windowの位置と合致数をおさめる
		else{
			if(notMatch > 0){
				*notMatchStart=notMatch;
				notMatch=0;
			}
			ret[retIndex] = 0x80;
			ret[retIndex] = ret[retIndex] | ((match-3)<<4);
			ret[retIndex] = ret[retIndex] | (((index-3) & 0x0F00)>>8);
			retIndex++;
			ret[retIndex] = (char)((index-3) & 0x00FF);
			retIndex++;

			srcIndex+=match;
		}
	}
	if(notMatch > 0){
		*notMatchStart=notMatch;
	}
	return retIndex;
}
/*
      　解凍
戻り値 解凍後サイズ
char* ret 解凍先
char* src　圧縮ファイル
unsigned int srcSize　圧縮ファイルサイズ
*/
unsigned int uncompressLZSS(char* ret, char* src, unsigned int srcSize){
	unsigned int srcIndex=0;
	unsigned int retIndex=0;
	unsigned int index=0;
	unsigned int match=0;

	while(srcIndex < srcSize){
		if((unsigned char)src[srcIndex] <= 127){
			memcpy(&ret[retIndex],&src[srcIndex+1],src[srcIndex]);
			retIndex+=src[srcIndex];
			srcIndex+=(src[srcIndex]+1);
		}
		else{
			match = ((src[srcIndex] & 0x7F) >> 4)+3;
			index = (src[srcIndex] & 0x0F) << 8;
			srcIndex++;
			index = (index | (unsigned int)(0x00FF & src[srcIndex])) + 3;
			srcIndex++;
			memcpy(&ret[retIndex],&ret[retIndex-index],match);
			retIndex+=match;
		}
	}
	return retIndex;
}
