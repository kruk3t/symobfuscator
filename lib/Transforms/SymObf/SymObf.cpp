/*
Author: Hui Xu @ CUHK
Feature: Taint LLVM IR instructions
==========================================================
Format of instructions

LoadInst
%0 = load i8**, i8*** %argv.addr, align 8
(Value*) dest = Opcode (Type*) destType, Operand(0) (Type*) operandType, 

CmpInst
---------
ICmpInst
%cmp = icmp eq i32 %2, 123
(Value*) dest = Opcode  Operand(0), Operand(1)
=========================================================
*/
#include "llvm/Transforms/SymObf/SymObf.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "mmap/MMap.h"

using namespace llvm;
using namespace std;

//STATISTIC(TaintedInst, "Number of tainted llvm instructions");
loglevel_e loglevel = L3_DEBUG;
Constant* printFunc;
Constant* mallocFunc;
Constant* multMatFunc;
Constant* multArMatFunc;
Constant* zeroTestFunc;

Type *boolType, *i8Type, *i32Type, *i64Type;
Type *doubleType;
PointerType *fpPT, *i64PT, *ptrPT, *ptrPPT, *ptrPPPT;
ConstantInt *ci0, *ci1, *ciMod;
vector<Value*> vec00,vec01;
int64_t mod;

namespace{

class TaintEngine : public InstVisitor<TaintEngine>{

  const DataLayout &DL;
  //const TargetLibraryInfo *TLI;

  list<Value*> workList;


public:

  list<Instruction*> taintedInstList;
  list<Value*> taintedValList;

  //TaintEngine(const DataLayout &dataLayout, const TargetLibraryInfo *targetLib): dataLayout(dataLayout), targetLib(targetLib) {}
  TaintEngine(const DataLayout &DL) : DL(DL) {}
  void SetSource(Value*);
  void Propagate();
  void VisitInst(Instruction*);


private:
  friend class InstVisitor<TaintEngine>;
  void visitStoreInst     (StoreInst &);
  void visitLoadInst      (LoadInst &);
  void visitPHINode(PHINode &);
  void visitReturnInst(ReturnInst &);
  void visitBranchInst(BranchInst &);
  void visitCastInst(CastInst &);
  void visitSelectInst(SelectInst &);
  void visitBinaryOperator(Instruction &);
  void visitCmpInst(CmpInst &);
  void visitExtractElementInst(ExtractElementInst &);
  void visitInsertElementInst(InsertElementInst &);
  void visitShuffleVectorInst(ShuffleVectorInst &);
  void visitExtractValueInst(ExtractValueInst &);
  void visitInsertValueInst(InsertValueInst &);
  void visitLandingPadInst(LandingPadInst &) {  }
  void visitFuncletPadInst(FuncletPadInst &) { }
  void visitCatchSwitchInst(CatchSwitchInst &);
  void visitGetElementPtrInst(GetElementPtrInst &);
  void visitCallInst      (CallInst &);
  void visitInvokeInst    (InvokeInst &);
  void visitCallSite      (CallSite);
  void visitResumeInst    (TerminatorInst &) {  }
  void visitUnreachableInst(TerminatorInst &) { }
  void visitFenceInst     (FenceInst &) { }
  void visitAtomicCmpXchgInst(AtomicCmpXchgInst &) { }
  void visitAtomicRMWInst (AtomicRMWInst &) {  }
  void visitAllocaInst    (Instruction &) {  }
  void visitVAArgInst     (Instruction &) { }
  void visitInstruction(Instruction &); 

  void MarkTainted(Value *, Instruction *); 
  void MarkTainted(Value *); 
};
//End TaintEngine Definition

void TaintEngine::MarkTainted(Value *val) {

}

void TaintEngine::MarkTainted(Value *val, Instruction *inst) {
  for(list<Instruction*>::iterator it = taintedInstList.begin(); it!=taintedInstList.end();it++){
    Instruction* tmpInst = *it;
    if(tmpInst == inst){
      LOG(L_WARNING) << "Instruction already tainted...";
      return ;
    }
  }
  LOG(L_WARNING) << "Taint a new instruction!!!";
  taintedInstList.push_back(inst);
  inst->print(errs()); errs()<<"\n";
  if(val == nullptr){
    return ;
  }
  LOG(L_WARNING) << "Taint a new value:" << val->getName().str(); 
  workList.push_back(val);
  taintedValList.push_back(val);
}

void TaintEngine::visitInvokeInst (InvokeInst &invokeInst) {
  visitCallSite(&invokeInst);
}

void TaintEngine::visitInstruction(Instruction &inst) {
  // If a new instruction is added to LLVM that we don't handle.
  LOG(L_WARNING) << "!!!!!!!!!!!!!!!!!!!!TaintEngine: Don't know how to handle:";
  inst.print(errs());
  LOG(L_WARNING) << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
}

void TaintEngine::SetSource(Value *val) {
  workList.push_back(val);
  taintedValList.push_back(val);
}


void TaintEngine::VisitInst(Instruction *inst) {
  visit(*inst);//Originally defined in native llvm/IR/InstVisitor.h
}

void TaintEngine::visitCatchSwitchInst(CatchSwitchInst &cwInst) {
  LOG(L2_DEBUG)<<"Entering visitCatchSwitchInst.......";
}

void TaintEngine::visitStoreInst(StoreInst &storeInst) {
  //TODO:
  LOG(L2_DEBUG) << "Entering TaintEngine::visitStoreInst...";
  Value* val = storeInst.getOperand(1);
  MarkTainted(val, &storeInst);
}

void TaintEngine::visitLoadInst(LoadInst &loadInst) {
  // TODO : 
  LOG(L2_DEBUG)<<"Entering visitLoadInst.......";
  Value* val = &loadInst;
  MarkTainted(val,&loadInst);
}

void TaintEngine::visitPHINode(PHINode &pNode) {
  // TODO: 
  LOG(L2_DEBUG)<<"Entering visitPHINode.......";
}

void TaintEngine::visitReturnInst(ReturnInst &retInst) {
  LOG(L2_DEBUG)<<"Entering visitReturnInst.......";
  // TODO: 
}

void TaintEngine::visitBranchInst(BranchInst &branchInst) {
  LOG(L2_DEBUG)<<"Entering visitBranchInst.......";
  LOG(L2_DEBUG)<<"BranchInst is not our interest.......";
}

void TaintEngine::visitCastInst(CastInst &castInst) {
  // TODO: 
  LOG(L2_DEBUG)<<"Entering visitCastInst.......";
  Value* val = &castInst;
  MarkTainted(val, &castInst);
}


void TaintEngine::visitExtractValueInst(ExtractValueInst &evInst) {
  // TODO: 
  LOG(L2_DEBUG)<<"Entering visitExtractValueInst.......";
}

void TaintEngine::visitInsertValueInst(InsertValueInst &ivInst) {
  // TODO: 
  LOG(L2_DEBUG)<<"Entering visitInsertValueInst.......";
}

void TaintEngine::visitSelectInst(SelectInst &selInst) {
  // TODO: 
  LOG(L2_DEBUG)<<"Entering visitSelectInst.......";
}

// Handle Binary Operators.
void TaintEngine::visitBinaryOperator(Instruction &bopInst) {
  // TODO: 
  LOG(L2_DEBUG)<<"Entering BinaryOperator.......";
  Value* val = &bopInst;
  MarkTainted(val, &bopInst);
}

// Handle ICmpInst instruction.
void TaintEngine::visitCmpInst(CmpInst &cmpInst) {
  // TODO: 
  LOG(L2_DEBUG)<<"Entering visitCmpInst.......";
  Value* val = &cmpInst;
  MarkTainted(val, &cmpInst);
}

void TaintEngine::visitExtractElementInst(ExtractElementInst &eeInst) {
  // TODO :
  LOG(L2_DEBUG)<<"Entering visitExtractInst.......";
  Value* val = &eeInst;
  MarkTainted(val, &eeInst);
}

void TaintEngine::visitCallInst (CallInst &callInst) {
  // TODO :
  LOG(L2_DEBUG)<<"Entering visitCallInst.......";
  Value* val = &callInst;
  MarkTainted(val, &callInst);
}

void TaintEngine::visitInsertElementInst(InsertElementInst &ieInst) {
  LOG(L2_DEBUG)<<"Entering visitInsertElementInst.......";
  // TODO : 
}

void TaintEngine::visitShuffleVectorInst(ShuffleVectorInst &svInst) {
  LOG(L2_DEBUG)<<"Entering visitShuffleVectorInst.......";
  // TODO : 
}

void TaintEngine::visitGetElementPtrInst(GetElementPtrInst &gepInst) {
  // TODO : 
  LOG(L2_DEBUG)<<"Entering visitGetElementPtrInst.......";
  Value* val = &gepInst;
  MarkTainted(val, &gepInst);
}


// Handle load instructions.  If the operand is a constant pointer to a constant
// global, we can replace the load with the loaded constant value!

void TaintEngine::visitCallSite(CallSite callSite) {
  LOG(L2_DEBUG)<<"Entering visitCallSite.......";
  Function *F = callSite.getCalledFunction();
  Instruction *inst = callSite.getInstruction();
  // TODO : 
}

/*Propagate the tainted value
*
*/
void TaintEngine::Propagate(){
  LOG(L2_DEBUG) << "Entering TaintEngine::Propagate...";
  while(!workList.empty()){
    Value *val = workList.front();
    for (User *taintUser : val->users()){
      if (Instruction *inst = dyn_cast<Instruction>(taintUser)){
        //inst->print(errs()); errs()<<"\n";
        VisitInst(inst);
      }
    }
    workList.pop_front();
  }
}

//end namespace
}


namespace {
struct SymObf : public ModulePass {
  static char ID; 
  SymObf() : ModulePass(ID) {} //initializeSymobfPass(*PassRegistry::getPassRegistry());

  //Entry function
  virtual bool runOnModule(Module &module){
    LOG(L2_DEBUG) << "Entering runOnModule...: " << module.getName().str();
    //Generate the function for matrix multiplication
    const DataLayout &dataLayout = module.getDataLayout();
	LLVMContext& context = module.getContext();


    i64Type = IntegerType::getInt64Ty(context);
    i32Type = IntegerType::getInt32Ty(context);
    i8Type = IntegerType::getInt8Ty(context);
    boolType = IntegerType::get(context,1);
	doubleType = Type::getDoubleTy(context);
    ci0 = (ConstantInt*) ConstantInt::getSigned(i64Type,0);
    ci1 = (ConstantInt*) ConstantInt::getSigned(i64Type,1);
    ciMod = (ConstantInt*) ConstantInt::getSigned(i64Type,1024);
    fpPT = PointerType::getUnqual(doubleType);
    i64PT = PointerType::getUnqual(i64Type);
    ptrPT = PointerType::getUnqual(i64PT);
    ptrPPT = PointerType::getUnqual(ptrPT);
    ptrPPPT = PointerType::getUnqual(ptrPPT);
	mod = 1024;

	vec00.push_back(ci0);
	vec00.push_back(ci0);
	ArrayRef<Value*> ar00(vec00);
	vec01.push_back(ci0);
	vec01.push_back(ci1);
	ArrayRef<Value*> ar01(vec01);

    //Define the printf function:
    printFunc = module.getFunction("printf"); 
    mallocFunc = module.getFunction("malloc"); 

    //Start the taint engine
    TaintEngine taintEngine(dataLayout);
	
    for(Module::iterator mIt = module.begin(); mIt != module.end(); ++mIt){
	  Function*  func = (Function*) mIt;
    // Set taint source: all arguments
      for (Function::arg_iterator argIt = func->arg_begin(); argIt != func->arg_end(); ++argIt){
        Value *argValue = &*argIt;
        LOG(L2_DEBUG) << "Argument: " << argValue->getName().str();
        taintEngine.SetSource(argValue);
      }
	}
    
    taintEngine.Propagate();
    //PrintIR(taintEngine.taintedInstList);
	//We generate the function of matrix multiplication
    // Function* funcMM = GenMatMulFunc(module.getContext(),module);

    //We declare the parameters of the function
    vector<Type*> paramVec;
    paramVec.push_back((Type *) ptrPT); //mat1
    paramVec.push_back((Type *) ptrPT); //mat2
    paramVec.push_back((Type *) ptrPT); //result
    paramVec.push_back(i64Type);		//m1Height
    paramVec.push_back(i64Type);		//m1Width
    paramVec.push_back(i64Type);		//m2Height
    paramVec.push_back(i64Type);		//m2Width
    paramVec.push_back(i64Type);		//mod
    ArrayRef<Type*> paramArrayType(paramVec);

    //We wrap the type of the function
    //Params: (Type *Result, ArrayRef< Type * > Params, bool isVarArg)
    FunctionType* funcType = FunctionType::get(i64Type, paramArrayType, false);
    multMatFunc = module.getOrInsertFunction("MultIntMatrix", funcType); 
    multArMatFunc = module.getOrInsertFunction("MultArMatrix", funcType); 

    MMapInitParam(1,1,1);
    vector<Type*> paramVec2;
    paramVec2.push_back(i64Type);
    paramVec2.push_back(i64Type);
    paramVec2.push_back(i64Type);
    paramVec2.push_back(i64Type);
    ArrayRef<Type*> paramAT(paramVec2);
    FunctionType* funcType2 = FunctionType::get(i64Type, paramAT, false);
    zeroTestFunc = module.getOrInsertFunction("ZeroTest", funcType2); 

	for(list<Instruction*>::iterator it = taintEngine.taintedInstList.begin(); it!=taintEngine.taintedInstList.end(); it++){
	  Instruction *inst = *it;
	  if(isa<ICmpInst> (*inst)){ //It is already boolean.
	    ConvertIcmp2Mbp(module, (ICmpInst*)inst);
		//inst->eraseFromParent();
	  }
	}
    //PrintIR(&F);
    return true;
  }
/*
  void getAnalysisUsage (AnalysisUsage& anaUsage) const override{
    //anaUsage.setPreservesCFG();
  }
  */
};

}

char SymObf::ID = 0;
static RegisterPass<SymObf> X("symobf", "symbolic obfuscation");

static void registerMyPass(const PassManagerBuilder &, legacy::PassManagerBase &PM) {
  PM.add(new SymObf());
}
static RegisterStandardPasses RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,registerMyPass);
