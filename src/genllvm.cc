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
		return VectorType::get(
			Type::getInt8Ty(getGlobalContext()), 16);
	default:
		std::cout << "COULDN'T HANDLE " << ty << std::endl;
	}
	return NULL;
}

Value* GenLLVM::readCtx(unsigned int byteOff, IRType ty)
{
	Value	*ret;

	ret = getCtxGEP(byteOff, vexTy2LLVM(ty));
	ret = builder->CreateLoad(ret);

	return ret;
}

Value* GenLLVM::getCtxGEP(unsigned int byteOff, const Type* accessTy)
{
	const Type	*ptrTy;
	unsigned int	tyBits;
	Value		*addr_ptr, *ret; /* XXX assuming access are aligned */

	ptrTy = PointerType::get(accessTy, 0);
	tyBits = accessTy->getPrimitiveSizeInBits();

	assert (tyBits && "Access type is 0 bits???");

	addr_ptr = builder->CreateBitCast(cur_guest_ctx, ptrTy, "accessCtxPtr");

	ret = builder->CreateGEP(
		addr_ptr, 
		ConstantInt::get(
			getGlobalContext(),
			APInt(
				32, 
				(byteOff*8)/tyBits)),
		"accessCtx");
	return ret;
}

Value* GenLLVM::writeCtx(unsigned int byteOff, Value* v)
{
	Value		*ret, *addr;
	StoreInst	*si;

	addr = getCtxGEP(byteOff, v->getType());
	si = builder->CreateStore(v, addr);
	ret = si;
	return ret;
}

void GenLLVM::store(Value* addr_v, Value* data_v)
{
	Type		*ptrTy;
	Value		*addr_ptr;
	StoreInst	*si;

	ptrTy = PointerType::get(data_v->getType(), 0);
	addr_v = guestState->addrVal2Host(addr_v);
	addr_ptr = builder->CreateBitCast(addr_v, ptrTy, "storePtr");
	si = builder->CreateStore(data_v, addr_ptr);
}

Value* GenLLVM::load(Value* addr_v, const Type* ty)
{
	Type		*ptrTy;
	Value		*addr_ptr;
	LoadInst	*loadInst;

	ptrTy = PointerType::get(ty, 0);
	addr_v = guestState->addrVal2Host(addr_v);
	addr_ptr = builder->CreateBitCast(addr_v, ptrTy, "loadPtr");
	loadInst = builder->CreateLoad(addr_ptr);
	loadInst->setAlignment(8);
	return loadInst;
}

Value* GenLLVM::load(Value* addr_v, IRType vex_type)
{
	return load(addr_v, vexTy2LLVM(vex_type));
}

/* gets i8* of base ptr */
Value* GenLLVM::getCtxBase(void)
{
	Value		*intptr_v;
	const Type	*ptrty;

	ptrty = PointerType::get(builder->getInt8Ty(), 0);
	intptr_v = ConstantInt::get(
		getGlobalContext(),
		APInt(	sizeof(intptr_t),
			(uintptr_t)guestState->getCPUState()->getStateData()));

	return builder->CreateBitCast(intptr_v, ptrty, "ctxbaseptr");
}

void GenLLVM::setExitType(uint8_t exit_type)
{
	writeCtx(
		guestState->getCPUState()->getExitTypeOffset(),
		ConstantInt::get(
			getGlobalContext(),
			APInt(8, exit_type)));
}

/* llvm-ized VexSB functions take form of 
 * guestaddr_t f(gueststate*) {  ...bullshit...; return ctrl_xfer_addr; } */
void GenLLVM::mkFuncTy(void)
{
	std::vector<const Type*>	f_args;
	f_args.push_back(PointerType::get(
		guestState->getCPUState()->getTy(), 0));
	funcTy = FunctionType::get(
		builder->getInt64Ty(),
		f_args,
		false);
}

Value* GenLLVM::to16x8i(Value* v) const
{
	return builder->CreateBitCast(
		v, 
		VectorType::get(
			Type::getInt8Ty(getGlobalContext()), 16));
}

void GenLLVM::memFence(void)
{
	Function  *fence_f;

	/* fence everything! */
	Value* args[5];
	for (int i = 0; i < 5; i++)
		args[i] = ConstantInt::get(builder->getInt1Ty(), 1),

	fence_f = Intrinsic::getDeclaration(mod, Intrinsic::memory_barrier);
	builder->CreateCall(fence_f, args, args+5);
}
