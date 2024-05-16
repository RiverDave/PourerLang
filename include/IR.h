//
// Created by David Rivera on 2024-04-30.
// Pourer's Intermediate Representation through LLVM

#ifndef IR_H
#define IR_H
#include "AST.h"
#include "KaleidoscopeJIT.h"
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Type.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Transforms/Scalar/Reassociate.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils/Cloning.h>


#include <map>
#include <memory>
#include <stack>

// static std::map<std::string, llvm::Value*> scopeVars;

// NOTE: Some of the code here was referenced from llvm.org

llvm::Value *LogErrorV(const char *Str);

struct fnInfo {
    llvm::Value *val;//Either a function or global variable ptr
    llvm::Type *type; //ret type
    llvm::FunctionType *fnType = nullptr;// if applies
    //    llvm::GlobalVariable* gvarAddr = nullptr;
};

// relevant variable information to be inserted
// into function block
struct varData {
    std::string name;
    llvm::Type *type;
    llvm::AllocaInst *allocaSpace = nullptr;// variable may have been declared outside
    llvm::Value *allocaVal = nullptr;       // variable may have been declared outside
};

// Represents Every block of code
// Wrapper around basic blocks
class CodeGenBlock {
public:
    llvm::BasicBlock *blockWrapper;
    llvm::Value *return_value;
    // Allocainst stores memory block occupied by that variable
    std::map<std::string, varData>
            locals;// Might be use to use another ptr here

    std::shared_ptr<CodeGenBlock> parent;// track parent block to access outer
                                         // scope variables

    CodeGenBlock() : blockWrapper(nullptr), return_value(nullptr), locals() {}
    CodeGenBlock(const CodeGenBlock &other) {
        // Shallow copy

        blockWrapper = other.blockWrapper;
        return_value = other.return_value;
        parent = other.parent;

        // NOTE: Might not be very memory efficient
        std::copy(other.locals.begin(), other.locals.end(),
                  std::inserter(locals, locals.end()));
    };
};

// Its purpose is to mainly store relevant information
// necessary to call a function


class CodeGenContext final {

public:
    std::unique_ptr<llvm::LLVMContext> TheContext;
    std::stack<std::shared_ptr<CodeGenBlock>> blocks;
    std::unique_ptr<llvm::Module> TheModule;
    std::unique_ptr<llvm::IRBuilder<>> Builder;
    llvm::ExecutionEngine *ee;
    std::map<std::string, fnInfo>
            globals;// will store Functions & global vaiables

    //Optimizations
    std::unique_ptr<llvm::orc::KaleidoscopeJIT> TheJIT;
    std::unique_ptr<llvm::legacy::FunctionPassManager> TheFPM;
    std::unique_ptr<llvm::LoopAnalysisManager> TheLAM;
    std::unique_ptr<llvm::FunctionAnalysisManager> TheFAM;
    std::unique_ptr<llvm::CGSCCAnalysisManager> TheCGAM;
    std::unique_ptr<llvm::ModuleAnalysisManager> TheMAM;
    std::unique_ptr<llvm::PassInstrumentationCallbacks> ThePIC;
    std::unique_ptr<llvm::StandardInstrumentations> TheSI;
    llvm::ExitOnError ExitOnErr;

    llvm::Function *globalFn;// Entry point function aka 'Main'

    CodeGenContext();
    void emitIR(NBlock &srcRoot);
    void setTargets();
    llvm::GenericValue runCode();

    llvm::AllocaInst *checkLocal(std::string &id,
                                 std::shared_ptr<CodeGenBlock> block);

    std::vector<std::pair<std::string, varData>> getOuterVars();

    // Insert memory block on fn block(used to instantiate variables)
    llvm::AllocaInst *insertMemOnFnBlock(llvm::Function *fn, std::string &id,
                                         llvm::Type *);

    llvm::Value *insertGlobal(std::string &id, llvm::Type *type);

    void pushBlock(llvm::BasicBlock *block) {

        std::optional<std::shared_ptr<CodeGenBlock>> current_node;
        std::shared_ptr<CodeGenBlock> next_assignable_node = nullptr;

        if (blocks.size() > 0 && blocks.top().get()) {
            current_node = blocks.top();
        }

        if (current_node.has_value()) {
            next_assignable_node =
                    std::make_shared<CodeGenBlock>(*current_node.value().get());
            // blocks.top()->parent->blockWrapper = current_node->blockWrapper;
        }

        blocks.push(std::make_shared<CodeGenBlock>());

        // Basically: Next block holds a reference to its parent block
        // This proves to be useful when searching for a variable defined in
        // another scope
        blocks.top()->parent = next_assignable_node;

        blocks.top()->return_value = nullptr;
        blocks.top()->blockWrapper = block;
    }

    void popBlock() { blocks.pop(); }
};

#endif
