#include "syscalls.h"
#include "guest.h"
#include <sys/syscall.h>

int Syscalls::translateI386Syscall(int sys_nr) {
	assert(guest->getArch() == Arch::getHostArch());
	return sys_nr;
}

uintptr_t Syscalls::applyI386Syscall(
	SyscallParams& args,
	GuestMem::Mapping& m)
{
	uintptr_t sc_ret = ~0ULL;

	/* special syscalls that we handle per arch, these
	   generally supersede any pass through or translated 
	   behaviors, but they can just alter the args and let
	   the other mechanisms finish the job */
	switch (args.getSyscall()) {
	default:
		break;
	}

	/* if the host and guest are identical, then just pass through */
	if(guest->getArch() == Arch::getHostArch()) {
		return passthroughSyscall(args, m);
	}

	std::cerr << "syscall(" << args.getSyscall() << ", "
		<< (void*)args.getArg(0) << ", "
		<< (void*)args.getArg(1) << ", "
		<< (void*)args.getArg(2) << ", "
		<< (void*)args.getArg(3) << ", "
		<< (void*)args.getArg(4) << ", ...) => ???"
		<< std::endl;
	assert(!"supporting translation between syscalls for these cpus");

	return sc_ret;
}