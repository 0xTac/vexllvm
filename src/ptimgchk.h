#ifndef PTIMGCHK_H
#define PTIMGCHK_H

#include <sys/types.h>
#include <sys/user.h>
#include "guestptimg.h"
#include "ptimgarch.h"
#include "ptctl.h"

class MemLog;
class SyscallsMarshalled;

class PTImgChk : public GuestPTImg, public PTCtl
{
public:
	PTImgChk(
		const char* binname,
		bool use_entry = true/* so templates work */);
	virtual ~PTImgChk();

	void printShadow(std::ostream& os) const;

	void stackTraceShadow(
		std::ostream& os,
		guest_ptr b = guest_ptr(0),
		guest_ptr e = guest_ptr(0));
	void printTraceStats(std::ostream& os);

	bool isMatch() const;

	bool fixup(const std::vector<InstExtent>& insts);
	MemLog* getMemLog(void) { return mem_log; }
	unsigned getNumFixups(void) const { return fixup_c; }
protected:
	virtual void handleChild(pid_t pid);

private:
	bool isStackMatch(void) const;
	bool isMatchMemLog() const;

	void waitForSingleStep(void);

	void printMemory(std::ostream& os) const;
	void printRootTrace(std::ostream& os) const;
	guest_ptr getPageMismatch(guest_ptr p) const;

	void readMemLogData(char* data) const;

	/* guest => clobber guest; native => clobber native */
	void doFixupGuest(void);
	void doFixupNative(void);

	pid_t		child_pid;

	uint64_t	blocks;

	bool		log_steps;

	MemLog		*mem_log;
	bool		xchk_stack;

	unsigned	fixup_c;
};

#endif
