#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Module.h>
#include <llvm/Instructions.h>

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/TypeBuilder.h>
#include <llvm/Support/system_error.h>

#include <iostream>
#include <stdio.h>
#include "Sugar.h"
#include "genllvm.h"

#include "vexhelpers.h"

#define VEXOP_BC_FILE "vexops.bc"

using namespace llvm;

VexHelpers* theVexHelpers;
extern void vexop_setup_fp(VexHelpers* vh);

VexHelpers::VexHelpers(Arch::Arch in_arch)
: arch(in_arch)
, helper_mod(0)
, vexop_mod(0)
{
	/* env not set => assume running from git root */
	bc_dirpath = getenv("VEXLLVM_HELPER_PATH");
	if (bc_dirpath == NULL) bc_dirpath = "bitcode";
}

void VexHelpers::loadDefaultModules(void)
{
	char		path_buf[512];

	const char* helper_file = NULL;
	switch(arch) {
	case Arch::X86_64:
		helper_file = "libvex_amd64_helpers.bc";
		break;
	case Arch::I386:
		helper_file = "libvex_x86_helpers.bc";
		break;
	case Arch::ARM:
		helper_file = "libvex_arm_helpers.bc";
		break;
	default:
		assert(!"known arch for helpers");
	}

	snprintf(path_buf, 512, "%s/%s", bc_dirpath, helper_file);
	helper_mod = loadMod(path_buf);
	snprintf(path_buf, 512, "%s/%s", bc_dirpath, VEXOP_BC_FILE);
	vexop_mod = loadMod(path_buf);

	vexop_setup_fp(this);

	assert (vexop_mod && helper_mod);
}

VexHelpers* VexHelpers::create(Arch::Arch arch)
{
	VexHelpers	*vh;

	vh = new VexHelpers(arch);
	vh->loadDefaultModules();

	return vh;
}

Module* VexHelpers::loadMod(const char* path)
{
	Module			*ret_mod;
	std::string		ErrorMsg;

	OwningPtr<MemoryBuffer> Buffer;
	bool			materialize_fail;

	MemoryBuffer::getFile(path, Buffer);

	if (!Buffer) {
		std::cerr <<  "Bad membuffer on " << path << std::endl;
		assert (Buffer && "Couldn't get mem buffer");
	}

	ret_mod = ParseBitcodeFile(Buffer.get(), getGlobalContext(), &ErrorMsg);
	if (ret_mod == NULL) {
		std::cerr
			<< "Error Parsing Bitcode File '"
			<< path << "': " << ErrorMsg << '\n';
	}
	assert (ret_mod && "Couldn't parse bitcode mod");
	materialize_fail = ret_mod->MaterializeAllPermanently(&ErrorMsg);
	if (materialize_fail) {
		std::cerr << "Materialize failed: " << ErrorMsg << std::endl;
		assert (0 == 1 && "BAD MOD");
	}

	if (ret_mod == NULL) {
		std::cerr << "OOPS: " << ErrorMsg
			<< " (path=" << path << ")\n";
	}
	return ret_mod;
}

mod_list VexHelpers::getModules(void) const
{
	mod_list	l = user_mods;
	l.push_back(helper_mod);
	l.push_back(vexop_mod);
	return l;
}

void VexHelpers::loadUserMod(const char* path)
{
	Module*	m;
	char	pathbuf[512];

	assert ((strlen(path)+strlen(bc_dirpath)) < 512);
	snprintf(pathbuf, 512, "%s/%s", bc_dirpath, path);

	m = loadMod(pathbuf);
	assert (m != NULL && "Could not load user module");
	user_mods.push_back(m);
}

VexHelpers::~VexHelpers()
{
	if (helper_mod) delete helper_mod;
	if (vexop_mod) delete vexop_mod;
	foreach (it, user_mods.begin(), user_mods.end())
		delete (*it);
}

Function* VexHelpers::getHelper(const char* s) const
{
	Function	*f;

	/* TODO why not start using a better algorithm sometime */
	if ((f = helper_mod->getFunction(s))) return f;
	if ((f = vexop_mod->getFunction(s))) return f;
	foreach (it, user_mods.begin(), user_mods.end()) {
		if ((f = (*it)->getFunction(s)))
			return f;
	}

	return NULL;
}

void VexHelpers::bindToExeEngine(ExecutionEngine* exe)
{
	exe->addModule(helper_mod);
	exe->addModule(vexop_mod);
}

llvm::Module* VexHelperDummy::loadMod(const char* path)
{
	fprintf(stderr, "FAILING LOAD MOD %s\n", path);
	return NULL;
}
