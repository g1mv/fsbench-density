GROFF = groff

SLIDEROBJ = hash_slide.o
OBJS =  lzx_layer.o lz_slide.o $(SLIDEROBJ)
OBJS2 =  lzx_layer_nonslide.o lz_nonslide.o
#CPROFFLAGS = -pg
CWARNFLAGS = -Wall -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Winline
CWARNFLAGS += -Wno-uninitialized 
CDEBUGFLAGS = -g
COPTFLAGS = -O
CFLAGS = $(CPROFFLAGS) $(CDEBUGFLAGS) $(COPTFLAGS) $(CWARNFLAGS) $(CPROGDEFS)
CPROGDEFS = -DLZ_ONEBUFFER -DLAZY
#-DDEBUG_ANALYZE_BLOCK 
#-DDEBUG_MATCHES_2
# -DSHORTSLIDE 
#-DDEBUG_MATCHES -DDEBUG_LZ 
LIBS= -lm

all: liblzxcomp.a

liblzxcomp.a: $(OBJS2)
	rm -f $@
	ar -q $@ $^
	ranlib $@

lz_slide_main.o: lz_slide.c
	$(CC) $(CFLAGS) -DLZSLIDE_MAIN -c -o $@ $<

lz_nonslide_main.o: lz_nonslide.c
	$(CC) $(CFLAGS) -DLZNONSLIDE_MAIN -c -o $@ $<

lz_slide: lz_slide_main.o $(SLIDEROBJ)
	$(CC) $(CFLAGS) -o $@ $^

lz_nonslide: lz_nonslide_main.o 
	$(CC) $(CFLAGS) -o $@ $^

lzx_layer_nonslide.o: lzx_layer.c
	$(CC) $(CFLAGS) -DNONSLIDE -c -o $@ $^

lzx_test: lzx_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) 

lzx_test2: lzx_test.o $(OBJS2)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) 

.DELETE_ON_ERROR:
lzx_compress.html: lzx_compress.mdoc3
	$(GROFF) -Tascii -mdoc2html -P-o -P-u -P-b $< > $@

clean:
	rm -f *.o *~ \#*\# lzx_test lz_slide liblzxcomp.a