// ---------------- IMPORTATIONS ----------------

//arch
#define ARCH64

//allocator
#include "lib/allocator.h"






/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prog [V.V.V] ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                       Allocator demonstration program

        Blablabla about the subject.

    Contact: .
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */






// ---------------- OUTPUT TOOLS ----------------

//syscalls
extern void syscall_write(uint fd, char* c, uint len);
extern void syscall_exit(uint err);



//bare metal tool (no std C lib)
uint str_len(char* s) {
	uint len = 0;
	while(s[0] != '\x00'){ s++; len++; }
	return len;
}

char u4_on_1hex(u8 b) {
	if(b < 10){ return '0' + b;      }
	if(b < 16){ return 'a' + b - 10; }
	return '#'; //means "err"
}
void u8_on2hex(u8 n, char* txt) {
	txt[0] = u4_on_1hex((n & '\xf0') >> 4);
	txt[1] = u4_on_1hex( n & '\x0f'      );
}
void u32_on8hex(u32 n, char* txt) {
	txt[0] = u4_on_1hex((n & 0xf0000000) >> 28);
	txt[1] = u4_on_1hex((n & 0x0f000000) >> 24);
	txt[2] = u4_on_1hex((n & 0x00f00000) >> 20);
	txt[3] = u4_on_1hex((n & 0x000f0000) >> 16);
	txt[4] = u4_on_1hex((n & 0x0000f000) >> 12);
	txt[5] = u4_on_1hex((n & 0x00000f00) >>  8);
	txt[6] = u4_on_1hex((n & 0x000000f0) >>  4);
	txt[7] = u4_on_1hex( n & 0x0000000f       );
}
#ifdef ARCH64
void u64_on16hex(u64 n, char* txt) {
	txt[ 0] = u4_on_1hex((n & 0xf000000000000000) >> 60);
	txt[ 1] = u4_on_1hex((n & 0x0f00000000000000) >> 56);
	txt[ 2] = u4_on_1hex((n & 0x00f0000000000000) >> 52);
	txt[ 3] = u4_on_1hex((n & 0x000f000000000000) >> 48);
	txt[ 4] = u4_on_1hex((n & 0x0000f00000000000) >> 44);
	txt[ 5] = u4_on_1hex((n & 0x00000f0000000000) >> 40);
	txt[ 6] = u4_on_1hex((n & 0x000000f000000000) >> 36);
	txt[ 7] = u4_on_1hex((n & 0x0000000f00000000) >> 32);
	txt[ 8] = u4_on_1hex((n & 0x00000000f0000000) >> 28);
	txt[ 9] = u4_on_1hex((n & 0x000000000f000000) >> 24);
	txt[10] = u4_on_1hex((n & 0x0000000000f00000) >> 20);
	txt[11] = u4_on_1hex((n & 0x00000000000f0000) >> 16);
	txt[12] = u4_on_1hex((n & 0x000000000000f000) >> 12);
	txt[13] = u4_on_1hex((n & 0x0000000000000f00) >>  8);
	txt[14] = u4_on_1hex((n & 0x00000000000000f0) >>  4);
	txt[15] = u4_on_1hex( n & 0x000000000000000f       );
}
#endif

void prt(char* s) {
	uint s_len = str_len(s);
	syscall_write(STDOUT, s, s_len);
}

void prt_u8(u8 b) {
	char out_u8[] = "  [bb]\n";
	u8_on2hex(b, out_u8+3);
	prt(out_u8);
}

void prt_u32(u32 i) {
	char out_u32[] = "  [ii..nn,,]\n";
	u32_on8hex(i, out_u32+3);
	prt(out_u32);
}

#ifdef ARCH64
void prt_u64(u64 l) {
	char out_u64[] = "  [--uu::ll;;oo++gg]\n";
	u64_on16hex(l, out_u64+3);
	prt(out_u64);
}
void prt_ulng(ulng l) { prt_u64(l); }
#else
void prt_ulng(ulng l) { prt_u32(l); }
#endif






// ---------------- EXECUTION ----------------

//main
void _start(){

	//presentation
	prt("Prog > This is a basic example of dynamic allocation using \"allocator.c/.h\".\n\n");



	// TEST 1: P1

	//allocating an uncommon nbr of bytes
	prt("Trying to allocate uint* p1\n");
	ref  r1 = heap__new(10);
	u32* p1 = r1;
	prt("Successfully allocated p1\n\n");

	//print out returned adr
	prt("p1 allocated under address\n");
	prt_ulng((ulng)p1);

	//print p1 content byte per byte <---- Try looking too far, it must crash with SEG FAULT
	prt("p1 start [\n");
	for(u32 u=0; u < 10; u++){ prt_u8( ((u8*)p1)[u] ); }
	for(u32 u=0; u < 10; u++){ ((u8*)p1)[u] = '1'; }
	prt("] p1 end\n\n");

	//read - write into p1
	prt("Trying to read/write from/to p1\n");
	p1[0]        = 5;
	u32 i1       = p1[1];
	((u8*)p1)[8] = '4';         //= '\x34' = 0b0011_0100
	u8  b1       = ((u8*)p1)[9];
	prt("Successfully read/wrote from/to p1\n\n");

	//re-display with modifications
	prt("i1 = p1[1]\n");
	prt_u32(i1);
	prt("b1 = p1(u8*)[9]\n");
	prt_u8(b1);
	prt("wrote p1[0] = 5 and ((u8*)p1)[8] = '\\x34'\n");
	prt("p1 now [\n");
	for(u32 u=0; u < 10; u++){ prt_u8( ((u8*)p1)[u] ); }
	prt("] p1 end\n\n");



	// TEST 2: P2

	//allocating an uncommon nbr of bytes
	prt("Trying to allocate u32* p2\n");
	ref  r2 = heap__new(14);
	u32* p2 = r2;
	prt("Successfully allocated p2\n\n");

	//print out returned adr
	prt("p2 allocated under address\n");
	prt_ulng((ulng)p2);

	//print p2 content byte per byte <---- Try looking too far, it must crash with SEG FAULT
	prt("p2 start [\n");
	for(u32 u=0; u < 14; u++){ prt_u8( ((u8*)p2)[u] ); }
	prt("] p2 end\n\n");

	//read - write into p2
	prt("Trying to read/write from/to p2\n");
	p2[1]          = 8;
	u32 i2        = p2[2];
	((u8*)p2)[1] = '\xc2';
	u8 b2        = ((u8*)p2)[13];
	prt("Successfully read/wrote from/to p2\n\n");

	//re-display with modifications
	prt("i2 = p2[2]\n");
	prt_u32(i2);
	prt("b2 = p2(u8*)[13]\n");
	prt_u8(b2);
	prt("wrote p2[0] = 8 and p2(u8*)[1] = '\\xc2'\n");
	prt("p2 now [\n");
	for(u32 u=0; u < 14; u++){ prt_u8( ((u8*)p2)[u] ); }
	prt("] p2 end\n\n");



	// TEST 3: FREE

	//free p1
	prt("Trying to free p1\n");
	heap__free(r1);
	prt("Successfully freed p1\n\n");
	prt("Trying to free p2\n");
	heap__free(r2);
	prt("Successfully freed p2\n");

	//end of exe
	syscall_exit(ERR__SUCCESS);

	/*//prepare main loop data
	char c;
	int temp;
	char* data;
	printf("\nProg > Entering in main loop...\n");
	int stand = 1;

	//main loop
	while(stand){
		printf("\n\n\n\nProg > Main-Loop : Please enter 'h' to get some help.\n");
		printf("You > ");
		temp = scanf(" %c", &c);
		switch(c){

			//help menu
			case 'h':
				printf("\nProg > Here is some help on available commands in this main loop.\n");
				printf("Prog >  - 'h' : Shows this help section.\n");
				printf("Prog >  - 'q' : Quit Prog1.\n");
				printf("Prog >  - 'r' : Read content of SHM segment.\n");
				printf("Prog >  - 'w' : Write in SHM segment.\n\n");
			break;

			//quit
			case 'q':
				stand = 0;
			break;

			//read
			case 'r':
				printf("Prog > Reading data from SHM segment...\n");

				data = shm_read(s1);

				printf("Prog > SHM segment contains data : \"");
				for(int i=0; i < s1->length; i++)
					printf("%c", data[i]);
				printf("\".\n\n");
			break;

			//write
			case 'w':
				printf("Prog > Enter a text to write inside shm (length limit : %u) : ", s1->length);

				data = malloc(s1->length);
				if(data == NULL){
					printf("FATAL ERROR > prog.c : main() : Computer refuses to give more memory.\n");
					exit(EXIT_FAILURE);
				}
				temp += scanf(" %[^\n]s", data);

				shm_write(s1, data);

				printf("Prog > Wrote in SHM segment.\n\n");
				free(data);
			break;
		}
	}

	//end
	printf("Prog > Exited main loop.\n");*/

	//return EXIT_SUCCESS;
}
