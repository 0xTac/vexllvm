#!/bin/bash

for op in `cat opcodes.txt`; do
	echo $op
	mkdir -p tests/$op
	for r1 in `cat registers.txt`; do
		cat > "tests/$op/$r1.c"  <<InputComesFromHERE
unsigned int xma[4] __attribute__((aligned(0x10))) = {0x92847232,0xAD2314DA,0xC412409A,0x0D219477 };
unsigned int xmb[4] __attribute__((aligned(0x10))) = {0x322AC6E1,0x82FFB313,0x6612788B,0x56A3B321 };
unsigned long foo = 0xF1263826472926452ULL;
unsigned long bar = 0x8AB2382017B23AC12ULL;
int main() {
	asm volatile("movdqa (%0), %%xmm0" : : "a" (&xma));
	asm volatile("movdqa (%0), %%xmm3" : : "a" (&xmb));
	asm volatile("nop" : : "a" (&foo), "b" (bar));
        asm volatile("$op $r1");
        asm volatile("mov %rax, %rci");
        asm volatile("mov \$0xe7, %rax");
        asm volatile("mov \$0x3c, %rsi");
        asm volatile("xor %rdi, %rdi");
        asm volatile("syscall");
}
InputComesFromHERE
		gcc "tests/$op/$r1.c" -Os -o "tests/$op/$r1" &> /dev/null
		# if [ $? -ne 0 ]; then
		# 	echo "tests/$op/$r1 is bad"
		# 	cat "tests/$op/$r1.c"
		# fi
		rm "tests/$op/$r1.c"
		for r2 in `cat registers.txt`; do
			cat > "tests/$op/$r1-$r2.c"  <<InputComesFromHERE
unsigned int xma[4] __attribute__((aligned(0x10))) = {0x92847232,0xAD2314DA,0xC412409A,0x0D219477 };
unsigned int xmb[4] __attribute__((aligned(0x10))) = {0x322AC6E1,0x82FFB313,0x6612788B,0x56A3B321 };
unsigned long foo = 0xF1263826472926452ULL;
unsigned long bar = 0x8AB2382017B23AC12ULL;
int main() {
	asm volatile("movdqa (%0), %%xmm0" : : "a" (&xma));
	asm volatile("movdqa (%0), %%xmm3" : : "a" (&xmb));
	asm volatile("nop" : : "a" (&foo), "b" (bar));
        asm volatile("$op $r1, $r2");
        asm volatile("mov %rax, %rcx");
        asm volatile("mov \$0xe7, %rax");
        asm volatile("mov \$0x3c, %rsi");
        asm volatile("xor %rdi, %rdi");
        asm volatile("syscall");
}
InputComesFromHERE
		gcc "tests/$op/$r1-$r2.c" -Os -o "tests/$op/$r1-$r2" &> /dev/null
		# if [ $? -ne 0 ]; then
		# 	echo "tests/$op/$r1-$r2 is bad"
		# 	cat "tests/$op/$r1-$r2.c"
		# fi
		rm "tests/$op/$r1-$r2.c"
		done
	done
done