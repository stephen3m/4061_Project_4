CC=gcc
CFLAGS=-g -Wall -pthread

SRCDIR=src
INCDIR=include
LIBDIR=lib

all: outdir $(LIBDIR)/utils.o image_rotation

image_rotation: $(LIBDIR)/utils.o $(INCDIR)/utils.h $(SRCDIR)/image_rotation.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $@ $(LIBDIR)/utils.o $(SRCDIR)/image_rotation.c -lm


.PHONY: clean outdir

test: clean all
	@read -p "Please a Dir number: " dir_num; \
	./image_rotation img/$$dir_num output/$$dir_num 10 180;\


outdir: 
	for i in `seq 0 10`; do mkdir -p -m 0777 "output/$$i"; done;




# PA4
server: $(LIBDIR)/utils.o $(INCDIR)/utils.h $(SRCDIR)/server.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $@ $(LIBDIR)/utils.o $(SRCDIR)/server.c -lm

client: $(LIBDIR)/utils.o $(INCDIR)/utils.h $(SRCDIR)/client.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $@ $(LIBDIR)/utils.o $(SRCDIR)/client.c -lm
	
	
test_pa4: 
	./client ./img/0 180
	./client ./img/1 270
	./client ./img/2 180

clean:
	rm -f image_rotation
	rm -rf output
