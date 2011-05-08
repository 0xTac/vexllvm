#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Intrinsics.h>

#include <vector>
#include <iostream>

#include "gueststate.h"
#include "guestcpustate.h"

#include "genllvm.h"

GenLLVM* theGenLLVM;

using namespace llvm;

GenLLVM::GenLLVM(const GuestState* gs, const char* name)
: guestState(gs), 
  funcTy(NULL),
  cur_guest_ctx(NULL),
  cur_f(NULL),
  cur_bb(NULL)
{
	builder = new IRBuilder<>(getGlobalContext());
	mod = new Module(name, getGlobalContext());
	mod->addTypeName("guestCtxTy", guestState->getCPUState()->getTy());
	mkFuncTy();
}

GenLLVM::~GenLLVM(void)
{
	delete guestState;
	delete mod;
	delete builder;
}

void GenLLVM::beginBB(const char* name)
{
	assert (cur_bb == NULL && "nested beginBB");
	cur_f = Function::Create(
		funcTy,
		Function::ExternalLinkage,
		name,
		mod);
			
	cur_bb = BasicBlock::Create(getGlobalContext(), "entry", cur_f);
	builder->SetInsertPoint(cur_bb);
	cur_guest_ctx = cur_f->arg_begin();
}

Function* GenLLVM::endBB(Value* retVal)
{
	Function	*ret_f;

	assert (cur_bb != NULL && "ending missing bb");

	/* FIXME. Should return next addr to jump to */
	builder->CreateRet(retVal);

	ret_f = cur_f;
	cur_f = NULL;
	cur_bb = NULL;
	cur_guest_ctx = NULL;
	return ret_f;
}

const Type* GenLLVM::vexTy2LLVM(IRType ty) const
{
	switch(ty) {
	case Ity_I1:	return builder->getInt1Ty();
	case Ity_I8:	return builder->getInt8Ty();
	case Ity_I16:	return builder->getInt16Ty();
	case Ity_I32:	return builder->getInt32Ty();
	case Ity_I64:	return builder->getInt64Ty();
	case Ity_F32:	return builder->getFloatTy();
	case Ity_F64:	return builder->getDoubleTy();
//	case Ity_I128:	 TODO
	case Ity_V128:
		return llvm::VectorType::get(
			Type::getInt16Ty(getGlobalContext()), 8);
	default:
		std::cout << "COULDN'T HANDLE " << ty << std::endl;
	}
	return NULL;
}

Value* GenLLVM::readCtx(unsigned int byteOff, IRType ty)
{
	const Type*	readTy, *ptrTy;
	unsigned int	tyBits;
	Value		*addr_ptr, *ret; /* XXX assuming access are aligned */

	readTy = vexTy2LLVM(ty);
	ptrTy = llvm::PointerType::get(readTy, 0);
	tyBits = readTy->getPrimitiveSizeInBits();

	if (!tyBits) std::cerr << "AAAAAAAAAAAAAAAAAAAAAAA" << std::endl;
	assert (tyBits);

	addr_ptr = builder->CreateBitCast(cur_guest_ctx, ptrTy, "readCtxPtr");

	ret = builder->CreateGEP(
		addr_ptr, 
		ConstantInt::get(
			getGlobalContext(),
			llvm::APInt(
				32, 
				(byteOff*8)/tyBits)),
		"readCtx");
	ret = builder->CreateLoad(ret);

	return ret;
}

Value* GenLLVM::writeCtx(unsigned int byteOff, Value* v)
{
	unsigned int 	idx;
	Value		*ret;
	
	idx = guestState->getCPUState()->byteOffset2ElemIdx(byteOff);
	ret = builder->CreateStructGEP(cur_guest_ctx, idx, "writeCtx");
	ret = builder->CreateStore(v, ret);
	
	return ret;
}

void GenLLVM::store(llvm::Value* addr_v, llvm::Value* data_v)
{
	Type	*ptrTy;
	Value	*addr_ptr;

	ptrTy = llvm::PointerType::get(data_v->getType(), 0);
	addr_v = guestState->addr2Host(addr_v);
	addr_ptr = builder->CreateBitCast(addr_v, ptrTy, "storePtr");
	builder->CreateStore(data_v, addr_ptr);
}

Value* GenLLVM::load(llvm::Value* addr_v, IRType vex_type)
{
	Type		*ptrTy;
	Value		*addr_ptr;
	LoadInst	*loadInst;

	ptrTy = llvm::PointerType::get(vexTy2LLVM(vex_type), 0);
	addr_v = guestState->addr2Host(addr_v);
	addr_ptr = builder->CreateBitCast(addr_v, ptrTy, "loadPtr");
	loadInst = builder->CreateLoad(addr_ptr);
	loadInst->setAlignment(8);
	return loadInst;
}

/* llvm-ized VexSB functions take form of 
 * guestaddr_t f(gueststate*) {  ...bullshit...; return ctrl_xfer_addr; } */
void GenLLVM::mkFuncTy(void)
{
	std::vector<const llvm::Type*>	f_args;
	f_args.push_back(llvm::PointerType::get(
		guestState->getCPUState()->getTy(), 0));
	funcTy = FunctionType::get(
		builder->getInt64Ty(),
		f_args,
		false);
}
