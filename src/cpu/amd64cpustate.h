#ifndef AMD64CPUSTATE_H
#define AMD64CPUSTATE_H

#include <iostream>
#include <stdint.h>
#include <sys/user.h>
#include <assert.h>
#include <map>

#include "vexcpustate.h"
extern "C" {
#include <valgrind/libvex_guest_amd64.h>
}

class SyscallParams;

class AMD64CPUState : public VexCPUState
{
public:
	const static unsigned REGFILE_BYTES = 921;

	AMD64CPUState();
	~AMD64CPUState();
	void setStackPtr(guest_ptr);
	guest_ptr getStackPtr(void) const;
	void setPC(guest_ptr);
	guest_ptr getPC(void) const;

	virtual unsigned int getFuncArgOff(unsigned int arg_num) const;
	virtual unsigned int getRetOff(void) const;
	virtual unsigned int getStackRegOff(void) const;
	virtual unsigned int getPCOff(void) const;

	virtual void resetSyscall(void);

#ifdef __amd64__
	void setRegs(
		const user_regs_struct& regs,
		const user_fpregs_struct& fpregs);
	static void setRegs(
		VexGuestAMD64State& v,
		const user_regs_struct& regs, 
		const user_fpregs_struct& fpregs);

#endif

	static uint64_t getRFLAGS(const VexGuestAMD64State& v);

	void setFSBase(uintptr_t base);
	uintptr_t getFSBase() const;

	virtual int cpu2gdb(int gdb_off) const;
};

#endif
