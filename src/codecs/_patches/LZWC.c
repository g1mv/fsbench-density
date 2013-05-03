/*  LZWC 0.4 - A Fast Experimental Tree Based LZW Compressor
    Copyright (C) 2013  David Catt

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */
	

	
//Could be further optimized by removing codes on dictionary entries with all subnodes filled.
//Could be further optimized by writing only the number of bits that the dictionary fill would take up (eg. if fill is 4090 then write 12 bits per code).
//Could be further optimized by allowing the rewriting of the dictionary when it is full.

//#define PROGRESS
#include <stdlib.h>
//#include <stdio.h>
#include <string.h>
#include <time.h>
//#include <BufferedFile.h>

struct LZWNODE {
	unsigned short num;
	unsigned char val;
	struct LZWNODE* parent;
	struct LZWNODE* subnodes[256];
};
typedef struct LZWNODE LZWNODE;
typedef struct LZWNODE* PLZWNODE;
__inline PLZWNODE lzwnode_init(PLZWNODE nparent, char nval, unsigned short nnum) {
	int i;
	PLZWNODE ln = (PLZWNODE) malloc(sizeof(LZWNODE));
	if(!ln) { return NULL; }
	ln->parent = nparent;
	ln->val = nval;
	ln->num = nnum;
	for(i=0;i<256;i++) { ln->subnodes[i] = NULL; }
	return ln;
}
void lzwnode_free(PLZWNODE node, int full) {
	int i;
	if(full) {
		for(i=0;i<256;i++) { 
			if(node->subnodes[i]) { lzwnode_free(node->subnodes[i], full); }
		}
	}
	free(node);
}
struct LZWBASE {
	LZWNODE* tree;
	LZWNODE* node;
	LZWNODE* dict[65536];
	int entries;
};
typedef struct LZWBASE LZWBASE;
typedef struct LZWBASE* PLZWBASE;
void lzwbase_free(PLZWBASE lb) {
	lzwnode_free(lb->tree, 1);
	free(lb);
}
__inline int lzwbase_addnode(PLZWBASE lb, PLZWNODE parent, char val) {
	if(lb->entries < 65536) {
		if(!(parent->subnodes[val] = lb->dict[lb->entries] = lzwnode_init(parent, val, lb->entries))) { return 0; }
		lb->entries++;
	}
	return 1;
}
PLZWBASE lzwbase_init(int full) {
	int i;
	PLZWBASE lb = malloc(sizeof(LZWBASE));
	if(!lb) { return NULL; }
	lb->tree = lzwnode_init(NULL, 0, 0);
	if(!lb->tree) { free(lb); return NULL; }
	lb->node = lb->tree;
	for(i=0;i<65536;i++) { lb->dict[i] = NULL; }
	if(full) {
		for(i=0;i<256;i++) {
			if(!(lb->dict[i] = lb->tree->subnodes[i] = lzwnode_init(lb->tree, i, i))) { lzwbase_free(lb); return NULL; };
		}
		lb->entries = 256;
	} else {
		lb->entries = 0;
	}
	return lb;
}

size_t lzwc_compress(const unsigned char* input, size_t input_size, unsigned char* output, size_t output_size) {
	//Initialize LZW
    const unsigned char* input_end = input + input_size;
    const unsigned char* output_end = output + output_size;
	int c;
	long long ib=0,ob=0;
	PLZWBASE lzw = lzwbase_init(1);
	if(!lzw) { return 0; }
	//Compress
	while(input < input_end) {
	    c = *input++;
		if(!lzw->node->subnodes[c]) {
			//Not in dictionary.
			if(lzw->entries < 65536) {
				if(!(lzw->node->subnodes[c] = lzw->dict[lzw->entries] = lzwnode_init(lzw->node, c, lzw->entries))) { lzwbase_free(lzw); return 0; }
				lzw->entries++;
			}
			//Write code
			if(output_end - output < 2) { lzwbase_free(lzw); return 0; }
            *output++ = (lzw->node->num >> 8);
            *output++ = lzw->node->num;
			ob += 2;
			//Reset
			lzw->node = lzw->tree->subnodes[c];
		} else {
			//In dictionary
			lzw->node = lzw->node->subnodes[c];
		}
		ib++;
#ifdef PROGRESS
		if(!(ib & 0xFFFFF)) { printf("Compressed    %lli -> %lli (%.1f%%)    \r", ib, ob, (double)(ob * 100) / ib); }
#endif
	}
	//Flush data
	bfputc((lzw->node->num >> 8) & 0xFF, output);
	bfputc(lzw->node->num & 0xFF, output);
	bfflush(output);
	ob += 2;
	printf("Compressed    %lli -> %lli (%.1f%%)    \n", ib, ob, (double)(ob * 100) / ib);
	//Return
	lzwbase_free(lzw);
	return 1;
}
int lzw_decompress(BufferedFile* input, BufferedFile* output) {
	//Initialize LZW
	int c,d,i,p=0;
	long long ib=0,ob=0;
	LZWNODE* last = NULL;
	LZWNODE* chain[65536];
	PLZWBASE lzw = lzwbase_init(1);
	if(!lzw) { return 0; }
	//Compress
	while(input < input_end - 1) {
        c = *input++;
        d = *input++;
		d |= c << 8;
		d &= 65535;
		//Find chain
		i = 0;
		if(d == lzw->entries) {
			//Node hasn't been added yet, add new dictionary entry
			if(last && (lzw->entries < 65536)) {
				if(!(last->subnodes[p] = lzw->dict[lzw->entries] = lzwnode_init(last, p, lzw->entries))) { lzwbase_free(lzw); return 0; }
				lzw->entries++;
				lzw->node = last->subnodes[p];
			}
			//Walk node chain
			while(lzw->node->parent) { 
				chain[i++] = lzw->node;
				lzw->node = lzw->node->parent;
			}
			//Update
			p = chain[i-1]->val;
			last = chain[0];
		} else {
			//Normal node
			if(!(lzw->node = lzw->dict[d])) { lzwbase_free(lzw); return 0; }
			//Walk node chain
			while(lzw->node->parent) { 
				chain[i++] = lzw->node;
				lzw->node = lzw->node->parent;
			}
			//Add new dictionary entry
			p = chain[i-1]->val;
			if(last && (lzw->entries < 65536)) {
				if(!(last->subnodes[p] = lzw->dict[lzw->entries] = lzwnode_init(last, p, lzw->entries))) { lzwbase_free(lzw); return 0; }
				lzw->entries++;
			}
			last = chain[0];			
		}
		//Decode
		ob += i;
		if(output_end - output < i) { lzwbase_free(lzw); return 0; }
		while(i--) *output++ = chain[i]->val;
		//Update progress
		ib += 2;
#ifdef PROGRESS
		if(!(ib & 0xFFFFF)) { printf("Decompressed  %lli -> %lli (%.1f%%)    \r", ib, ob, (double)(ib * 100) / ob); }
#endif
	}
	//Flush data
	//Return
	lzwbase_free(lzw);
	return output_size - (output_end - output);
}
#if 0
void dohelp(char* prog) {
	//printf("LZWC 0.4                            Copyright (c) 2013 by David Catt\n\n");
	printf("usage: %s c|d [infile] [outfile]\n", prog);
}
int main(int argc, char** argv) {
	int r;
	clock_t st;
	BufferedFile* in;
	BufferedFile* out;
	if((argc == 3) || (argc == 4)) {
		if(argv[1][0] == 'c') {
			if(argc == 3) { argv[3] = (char*) malloc((strlen(argv[2]) + 6) * sizeof(char));  strcpy(argv[3], argv[2]);  strcat(argv[3], ".lzwc"); }
			if(!(in = bfopen(argv[2], "rb"))) { printf("Error opening input file.\n"); return 1; }
			if(!(out= bfopen(argv[3], "wb"))) { bfclose(in); printf("Error opening output file.\n"); return 1; }
			st = clock();
			r = lzw_compress(in, out);
			bfclose(in);
			bfclose(out);
			if(!r) {
				printf("Errors occured while compressing.\n");
				printf("Operation completed in %.3f seconds.\n", (double)(clock() - st) / CLOCKS_PER_SEC);
				return 1;
			} else {
				printf("Operation completed in %.3f seconds.\n", (double)(clock() - st) / CLOCKS_PER_SEC);
				return 0;
			}
		} else if(argv[1][0] == 'd') {
			if(argc == 3) { argv[3] = (char*) malloc((strlen(argv[2]) + 6) * sizeof(char));  strcpy(argv[3], argv[2]);  strcat(argv[3], ".lzwd"); }
			if(!(in = bfopen(argv[2], "rb"))) { printf("Error opening input file.\n"); return 1; }
			if(!(out= bfopen(argv[3], "wb"))) { bfclose(in); printf("Error opening output file.\n"); return 1; }
			st = clock();
			r = lzw_decompress(in, out);
			bfclose(in);
			bfclose(out);
			if(!r) {
				printf("Errors occured while decompressing.\n");
				printf("Operation completed in %.3f seconds.\n", (double)(clock() - st) / CLOCKS_PER_SEC);
				return 1;
			} else {
				printf("Operation completed in %.3f seconds.\n", (double)(clock() - st) / CLOCKS_PER_SEC);
				return 0;
			}
		} else {
			dohelp(argv[0]);
			return 1;
		}
	} else {
		dohelp(argv[0]);
		return 1;
	}
	return 0;
}
#endif