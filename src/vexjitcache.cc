#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include "llvm/Target/TargetSelect.h"
#include "llvm/ExecutionEngine/JIT.h"

#include "Sugar.h"
#include "vexjitcache.h"

using namespace llvm;

VexJITCache::VexJITCache(VexXlate* xlate, ExecutionEngine *in_exe)
: VexFCache(xlate),
  exeEngine(in_exe)
{
}

VexJITCache::~VexJITCache(void)
{
	flush();
	delete exeEngine;
}

vexfunc_t VexJITCache::getCachedFPtr(guest_ptr guest_addr)
{
	jit_map::const_iterator it;
	vexfunc_t		*fptr;

	fptr = jit_dc.get(guest_addr);
	if (fptr) return (vexfunc_t)fptr;

	it = jit_cache.find(guest_addr);
	if (it == jit_cache.end()) return NULL;

	fptr = (vexfunc_t*)(*it).second;
	jit_dc.put(guest_addr, fptr);

	return (vexfunc_t)fptr;
}

vexfunc_t VexJITCache::getFPtr(void* host_addr, guest_ptr guest_addr)
{
	Function	*llvm_f;
	vexfunc_t	ret_f;

	ret_f = getCachedFPtr(guest_addr);
	if (ret_f) return ret_f;

	llvm_f = getFunc(host_addr, guest_addr);
	assert (llvm_f != NULL && "Could not get function");

	ret_f = (vexfunc_t)exeEngine->getPointerToFunction(llvm_f);
	assert (ret_f != NULL && "Could not JIT");

	jit_cache[guest_addr] = ret_f;
	jit_dc.put(guest_addr, (vexfunc_t*)ret_f);

	return ret_f;
}

void VexJITCache::evict(guest_ptr guest_addr)
{
	Function	*f;
	if ((f = getCachedFunc(guest_addr)) != NULL) {
		exeEngine->freeMachineCodeForFunction(f);
	}
	jit_cache.erase(guest_addr);
	jit_dc.put(guest_addr, NULL);
	VexFCache::evict(guest_addr);
}

void VexJITCache::flush(void)
{
	foreach (it, funcBegin(), funcEnd()) {
		Function	*f = it->second;
		exeEngine->freeMachineCodeForFunction(f);
	}
	jit_cache.clear();
	jit_dc.flush();
	VexFCache::flush();
}

void VexJITCache::flush(guest_ptr begin, guest_ptr end)
{
	foreach (it, funcBegin(begin), funcEnd(end)) {
		Function	*f = it->second;
		exeEngine->freeMachineCodeForFunction(f);
		jit_cache.erase(it->first);
		jit_dc.put(it->first, NULL);
	}
	VexFCache::flush(begin, end);
}
