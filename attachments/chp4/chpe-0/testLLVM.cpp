// compiled with: gcc ./whatsUrName.c -o ./whatsUrName
// author: @eastXueLian
// date:   2023-03-26

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace llvm;

namespace {
int edit_flag = 0;
char *studentList[0x10] = {0};
long long helper_regs[3] = {0};

std::string getCallArgName(CallInst *CInst, int arg_idx) {
  Value *op0 = CInst->getArgOperand(arg_idx);
  if (isa<ConstantExpr>(op0)) {
    auto constExpr = dyn_cast<ConstantExpr>(op0);
    if (constExpr->isGEPWithNoNotionalOverIndexing()) {
      if (constExpr->getOpcode() == Instruction::GetElementPtr) {
        if (isa<GlobalVariable>(constExpr->getOperand(arg_idx))) {
          auto var = dyn_cast<GlobalVariable>(constExpr->getOperand(arg_idx));
          auto a5 = dyn_cast<ConstantDataArray>(var->getInitializer());
          if (a5) {
            auto a6 = a5->getAsString();
            // errs() << "String: " << a6 << "\n";
            return a6;
          }
        }
      }
    }
  }
  return NULL;
}

struct bianYiYuanLiXiTiKe : public FunctionPass {
  static char ID;
  bianYiYuanLiXiTiKe() : FunctionPass(ID) {}
  bool runOnFunction(Function &F) override {
    if (F.getName() == "classBegin") {
      for (Function::iterator B_iter = F.begin(); B_iter != F.end(); ++B_iter) {
        BasicBlock *curBB = &*B_iter;
        std::string name = curBB->getName().str();
        errs() << name << "\n";
        for (BasicBlock::iterator I_iter = curBB->begin();
             I_iter != curBB->end(); ++I_iter) {
          Instruction *I = &*I_iter;
          if (CallInst *inst = dyn_cast<CallInst>(I)) {
            Function *called = inst->getCalledFunction();
            unsigned int arg_count = inst->getNumOperands();
            std::string Name = called->getName().str();
            if (Name == "qiaoKe") {
              // delet
              if (arg_count != 2) {
                return false;
              }
              unsigned int stu_id =
                  dyn_cast<ConstantInt>(inst->getArgOperand(0))->getZExtValue();
              if (stu_id >= 0x10 || !studentList[stu_id]) {
                return false;
              }
              free(studentList[stu_id]);
              // studentList[stu_id] = 0;
            } else if (Name == "daBian") {
              // show
              if (arg_count != 2) {
                return false;
              }
              unsigned int stu_id =
                  dyn_cast<ConstantInt>(inst->getArgOperand(0))->getZExtValue();
              if (stu_id >= 0x10 || !studentList[stu_id]) {
                return false;
              }
              helper_regs[0] = *(long long *)(studentList[stu_id]);
              helper_regs[1] = *(long long *)(studentList[stu_id] + 8);
            } else if (Name == "dianMing") {
              // add
              if (arg_count != 3) {
                return false;
              }
              auto name_str = getCallArgName(inst, 0);
              unsigned int stu_id =
                  dyn_cast<ConstantInt>(inst->getArgOperand(1))->getZExtValue();
              unsigned int name_size = name_str.size();
              if (stu_id >= 0x10 || studentList[stu_id]) {
                return false;
              }
              studentList[stu_id] = (char *)malloc(name_size);
              memcpy(studentList[stu_id], name_str.c_str(), name_size);
            } else if (Name == "xiaoCe") {
              // edit
              if (arg_count != 2 || edit_flag) {
                return false;
              }
              unsigned int stu_id =
                  dyn_cast<ConstantInt>(inst->getArgOperand(0))->getZExtValue();
              if (stu_id >= 0x10 || !studentList[stu_id]) {
                return false;
              }
              *(long long *)(studentList[stu_id]) = helper_regs[0];
              *(long long *)(studentList[stu_id] + 8) = helper_regs[1];
              // edit_flag = 1;
            } else if (Name == "add") {
              // edit
              if (arg_count != 3) {
                return false;
              }
              unsigned int reg_id =
                  dyn_cast<ConstantInt>(inst->getArgOperand(0))->getZExtValue();
              long long data =
                  dyn_cast<ConstantInt>(inst->getArgOperand(1))->getZExtValue();
              if (reg_id >= 3) {
                return false;
              }
              helper_regs[reg_id] = helper_regs[reg_id] + data;
            } else if (Name == "mov") {
              // edit
              if (arg_count != 3) {
                return false;
              }
              unsigned int reg_id_dest =
                  dyn_cast<ConstantInt>(inst->getArgOperand(0))->getZExtValue();
              unsigned int reg_id_src =
                  dyn_cast<ConstantInt>(inst->getArgOperand(1))->getZExtValue();
              if (reg_id_dest >= 3) {
                return false;
              }
              if (reg_id_src >= 3) {
                return false;
              }
              helper_regs[reg_id_dest] = helper_regs[reg_id_src];
            }
          }
        }
      }
    }
    return false;
  }
};
} // namespace

char bianYiYuanLiXiTiKe::ID = 0;

// Register for opt
static RegisterPass<bianYiYuanLiXiTiKe> X("bianYiYuanLiXiTiKe",
                                          "Do You Love Bianyiyuanli?");

// Register for clang
static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new bianYiYuanLiXiTiKe());
                                });
