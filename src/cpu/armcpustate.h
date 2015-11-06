#ifndef ARMCPUSTATE_H
#define ARMCPUSTATE_H

#include <iostream>
#include <stdint.h>
#include <sys/user.h>
#include <assert.h>
#include <map>
#include "vexcpustate.h"

class SyscallParams;

class ARMCPUState : public VexCPUState
{
public:
	ARMCPUState();
	~ARMCPUState();
	void setStackPtr(guest_ptr);
	guest_ptr getStackPtr(void) const;
	void setPC(guest_ptr);
	guest_ptr getPC(void) const;

#ifdef __arm__
	void setRegs(
		const user_regs* regs, const uint8_t* vfpregs,
		void* thread_area);
#endif
	void setThreadPointer(uint32_t v);
	virtual void print(std::ostream& os, const void*) const;

	const char* off2Name(unsigned int off) const;

	virtual unsigned int getRetOff(void) const;
	virtual unsigned int getStackRegOff(void) const;

	const struct guest_ctx_field* getFields(void) const;

	static const char* abi_linux_scregs[];
};

/* its possible that the actual instruction can encode
   something in the non-eabi case, but we are restricting
   our selves to eabi for now */
#define	ABI_LINUX_ARM ARMCPUState::abi_linux_scregs, "R0", "R0", true


#endif
