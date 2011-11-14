#include <stdio.h>
#include "Sugar.h"

#include <iostream>
#include <cstring>
#include <vector>

#include "amd64cpustate.h"
extern "C" {
#include <valgrind/libvex_guest_amd64.h>
}

#define state2amd64()	((VexGuestAMD64State*)(state_data))

#define FS_SEG_OFFSET	(24*8)

AMD64CPUState::AMD64CPUState()
{
	mkRegCtx();
	state_data = new uint8_t[state_byte_c+1];
	memset(state_data, 0, state_byte_c+1);
	exit_type = &state_data[state_byte_c];
	state2amd64()->guest_DFLAG = 1;
	state2amd64()->guest_CC_OP = 0 /* AMD64G_CC_OP_COPY */;
	state2amd64()->guest_CC_DEP1 = (1 << 2);
}

AMD64CPUState::~AMD64CPUState()
{
	delete [] state_data;
}

void AMD64CPUState::setPC(guest_ptr ip) {
	state2amd64()->guest_RIP = ip;
}

guest_ptr AMD64CPUState::getPC(void) const {
	return guest_ptr(state2amd64()->guest_RIP);
}

/* ripped from libvex_guest_amd64 */
static struct guest_ctx_field amd64_fields[] =
{
	{64, 16, "GPR"},	/* 0-15, 8*16 = 128*/
	{64, 1, "CC_OP"},	/* 16, 128 */
	{64, 1, "CC_DEP1"},	/* 17, 136 */
	{64, 1, "CC_DEP2"},	/* 18, 144 */
	{64, 1, "CC_NDEP"},	/* 19, 152 */

	{64, 1, "DFLAG"},	/* 20, 160 */
	{64, 1, "RIP"},		/* 21, 168 */
	{64, 1, "ACFLAG"},	/* 22, 176 */
	{64, 1, "IDFLAG"},	/* 23 */

	{64, 1, "FS_ZERO"},	/* 24 */

	{64, 1, "SSEROUND"},
	{128, 17, "XMM"},	/* there is an XMM16 for valgrind stuff */

	{32, 1, "FTOP"},
	{64, 8, "FPREG"},
	{8, 8, "FPTAG"},

	{64, 1, "FPROUND"},
	{64, 1, "FC3210"},

	{32, 1, "EMWARN"},

	{64, 1, "TISTART"},
	{64, 1, "TILEN"},

	/* unredirected guest addr at start of translation whose
	 * start has been redirected */
	{64, 1, "NRADDR"},

	/* darwin hax */
	{64, 1, "SC_CLASS"},
	{64, 1, "GS_0x60"},
	{64, 1, "IP_AT_SYSCALL"},

	{64, 1, "pad"},
	/* END VEX STRUCTURE */

	{0}	/* time to stop */
};

void AMD64CPUState::mkRegCtx(void)
{
	guestCtxTy = mkFromFields(amd64_fields, off2ElemMap);
}

extern void dumpIRSBs(void);


const char* AMD64CPUState::off2Name(unsigned int off) const
{
	switch (off) {
#define CASE_OFF2NAME(x)	\
	case offsetof(VexGuestAMD64State, guest_##x) ... 7+offsetof(VexGuestAMD64State, guest_##x): \
	return #x;
#define CASE_OFF2NAMEN(x,y)	\
	case offsetof(VexGuestAMD64State, guest_##x##y) ... 7+offsetof(VexGuestAMD64State, guest_##x##y): \
	return #x"["#y"]";


	CASE_OFF2NAME(RAX)
	CASE_OFF2NAME(RCX)
	CASE_OFF2NAME(RDX)
	CASE_OFF2NAME(RBX)
	CASE_OFF2NAME(RSP)
	CASE_OFF2NAME(RBP)
	CASE_OFF2NAME(RSI)
	CASE_OFF2NAME(RDI)
	CASE_OFF2NAMEN(R,8)
	CASE_OFF2NAMEN(R,9)
	CASE_OFF2NAMEN(R,10)
	CASE_OFF2NAMEN(R,11)
	CASE_OFF2NAMEN(R,12)
	CASE_OFF2NAMEN(R,13)
	CASE_OFF2NAMEN(R,14)
	CASE_OFF2NAMEN(R,15)
	CASE_OFF2NAME(CC_OP)
	CASE_OFF2NAMEN(CC_DEP,1)
	CASE_OFF2NAMEN(CC_DEP,2)
	CASE_OFF2NAME(CC_NDEP)
	CASE_OFF2NAME(DFLAG)
	CASE_OFF2NAME(RIP)
	CASE_OFF2NAME(ACFLAG)
	CASE_OFF2NAME(IDFLAG)
	CASE_OFF2NAMEN(XMM,0)
	CASE_OFF2NAMEN(XMM,1)
	CASE_OFF2NAMEN(XMM,2)
	CASE_OFF2NAMEN(XMM,3)
	CASE_OFF2NAMEN(XMM,4)
	CASE_OFF2NAMEN(XMM,5)
	CASE_OFF2NAMEN(XMM,6)
	CASE_OFF2NAMEN(XMM,7)
	CASE_OFF2NAMEN(XMM,8)
	CASE_OFF2NAMEN(XMM,9)
	CASE_OFF2NAMEN(XMM,10)
	CASE_OFF2NAMEN(XMM,11)
	CASE_OFF2NAMEN(XMM,12)
	CASE_OFF2NAMEN(XMM,13)
	CASE_OFF2NAMEN(XMM,14)
	CASE_OFF2NAMEN(XMM,15)
	CASE_OFF2NAMEN(XMM,16)
	default: return NULL;
	}
	return NULL;
}

/* gets the element number so we can do a GEP */
unsigned int AMD64CPUState::byteOffset2ElemIdx(unsigned int off) const
{
	byte2elem_map::const_iterator it;
	it = off2ElemMap.find(off);
	if (it == off2ElemMap.end()) {
		unsigned int	c = 0;
		fprintf(stderr, "WTF IS AT %d\n", off);
		dumpIRSBs();
		for (int i = 0; amd64_fields[i].f_len; i++) {
			fprintf(stderr, "%s@%d\n", amd64_fields[i].f_name, c);
			c += (amd64_fields[i].f_len/8)*
				amd64_fields[i].f_count;
		}
		assert (0 == 1 && "Could not resolve byte offset");
	}
	return (*it).second;
}

void AMD64CPUState::setStackPtr(guest_ptr stack_ptr)
{
	state2amd64()->guest_RSP = (uint64_t)stack_ptr;
}

guest_ptr AMD64CPUState::getStackPtr(void) const
{
	return guest_ptr(state2amd64()->guest_RSP);
}

/**
 * %rax = syscall number
 * rdi, rsi, rdx, r10, r8, r9
 */
SyscallParams AMD64CPUState::getSyscallParams(void) const
{
	return SyscallParams(
		state2amd64()->guest_RAX,
		state2amd64()->guest_RDI,
		state2amd64()->guest_RSI,
		state2amd64()->guest_RDX,
		state2amd64()->guest_R10,
		state2amd64()->guest_R8,
		state2amd64()->guest_R9);
}


void AMD64CPUState::setSyscallResult(uint64_t ret)
{
	state2amd64()->guest_RAX = ret;
}

uint64_t AMD64CPUState::getExitCode(void) const
{
	/* exit code is from call to exit(), which passes the exit
	 * code through the first argument */
	return state2amd64()->guest_RDI;
}

// 208 == XMM base
#define get_xmm_lo(x,i)	((uint64_t*)(&(((uint8_t*)s)[208+16*i])))[0]
#define get_xmm_hi(x,i)	((uint64_t*)(&(((uint8_t*)s)[208+16*i])))[1]

void AMD64CPUState::print(std::ostream& os, const void* regctx) const
{
	VexGuestAMD64State	*s;

	s = (VexGuestAMD64State*)regctx;

	os << "RIP: " << (void*)s->guest_RIP << "\n";
	os << "RAX: " << (void*)s->guest_RAX << "\n";
	os << "RBX: " << (void*)s->guest_RBX << "\n";
	os << "RCX: " << (void*)s->guest_RCX << "\n";
	os << "RDX: " << (void*)s->guest_RDX << "\n";
	os << "RSP: " << (void*)s->guest_RSP << "\n";
	os << "RBP: " << (void*)s->guest_RBP << "\n";
	os << "RDI: " << (void*)s->guest_RDI << "\n";
	os << "RSI: " << (void*)s->guest_RSI << "\n";
	os << "R8: " << (void*)s->guest_R8 << "\n";
	os << "R9: " << (void*)s->guest_R9 << "\n";
	os << "R10: " << (void*)s->guest_R10 << "\n";
	os << "R11: " << (void*)s->guest_R11 << "\n";
	os << "R12: " << (void*)s->guest_R12 << "\n";
	os << "R13: " << (void*)s->guest_R13 << "\n";
	os << "R14: " << (void*)s->guest_R14 << "\n";
	os << "R15: " << (void*)s->guest_R15 << "\n";

	os << "EFLAGS: " << (void*)LibVEX_GuestAMD64_get_rflags(s) << '\n';

	for (int i = 0; i < 16; i++) {
		os
		<< "XMM" << i << ": "
		<< (void*) get_xmm_hi(s,i) << "|"
		<< (void*)get_xmm_lo(s,i) << std::endl;
	}

	for (int i = 0; i < 8; i++) {
		int r  = (state2amd64()->guest_FTOP + i) & 0x7;
		os
		<< "ST" << i << ": "
		<< (void*)s->guest_FPREG[r] << std::endl;
	}

	os << "fs_base = " << (void*)s->guest_FS_ZERO << std::endl;
}

void AMD64CPUState::setFSBase(uintptr_t base) {
	state2amd64()->guest_FS_ZERO = base;
}

uintptr_t AMD64CPUState::getFSBase() const {
	return state2amd64()->guest_FS_ZERO;
}

static const int arg2reg[] =
{
	offsetof(VexGuestAMD64State, guest_RDI),
	offsetof(VexGuestAMD64State, guest_RSI),
	offsetof(VexGuestAMD64State, guest_RDX),
	offsetof(VexGuestAMD64State, guest_RCX)
};

/* set a function argument */
void AMD64CPUState::setFuncArg(uintptr_t arg_val, unsigned int arg_num)
{
	assert (arg_num <= 3);
	*((uint64_t*)((uintptr_t)state_data + arg2reg[arg_num])) = arg_val;
}

#ifdef __amd64__
void AMD64CPUState::setRegs(
	const user_regs_struct& regs, 
	const user_fpregs_struct& fpregs)
{
	state2amd64()->guest_RAX = regs.rax;
	state2amd64()->guest_RCX = regs.rcx;
	state2amd64()->guest_RDX = regs.rdx;
	state2amd64()->guest_RBX = regs.rbx;
	state2amd64()->guest_RSP = regs.rsp;
	state2amd64()->guest_RBP = regs.rbp;
	state2amd64()->guest_RSI = regs.rsi;
	state2amd64()->guest_RDI = regs.rdi;
	state2amd64()->guest_R8  = regs.r8;
	state2amd64()->guest_R9  = regs.r9;
	state2amd64()->guest_R10 = regs.r10;
	state2amd64()->guest_R11 = regs.r11;
	state2amd64()->guest_R12 = regs.r12;
	state2amd64()->guest_R13 = regs.r13;
	state2amd64()->guest_R14 = regs.r14;
	state2amd64()->guest_R15 = regs.r15;
	state2amd64()->guest_RIP = regs.rip;

	//TODO: some kind of eflags, checking but i don't yet understand this
	//mess of broken apart state.

	//valgrind/vex seems to not really fully segments them, how sneaky
	assert (regs.fs_base != 0 && "TLS is important to have!!!");
	state2amd64()->guest_FS_ZERO = regs.fs_base;

	memcpy(	&state2amd64()->guest_XMM0,
		&fpregs.xmm_space[0],
		sizeof(fpregs.xmm_space));


	//TODO: this is surely wrong, the sizes don't even match...
	memcpy(	&state2amd64()->guest_FPREG[0],
		&fpregs.st_space[0],
		sizeof(state2amd64()->guest_FPREG));

	//TODO: floating point flags and extra fp state, sse  rounding
	//

	state2amd64()->guest_CC_OP = 0 /* AMD64G_CC_OP_COPY */;
	state2amd64()->guest_CC_DEP1 = regs.eflags & (0xff | (3 << 10)) ;

}
#endif

unsigned int AMD64CPUState::getFuncArgOff(unsigned int arg_num) const
{
	assert (arg_num <= 3);
	return arg2reg[arg_num];
}

unsigned int AMD64CPUState::getStackRegOff(void) const
{
	return offsetof(VexGuestAMD64State, guest_RSP);
}

unsigned int AMD64CPUState::getRetOff(void) const
{
	return offsetof(VexGuestAMD64State, guest_RAX);
}
