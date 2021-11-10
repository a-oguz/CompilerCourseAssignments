
#include "default-defs.h"
#include <list>
#include <utility>
#include <map>
#include <ostream>
#include <iostream>
#include <sstream>
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Argument.h"

#ifndef YYTOKENTYPE
#include "decafcomp.tab.h"
#endif

using namespace std;

// this global variable contains all the generated code
static llvm::Module *TheModule;

// this is the method used to construct the LLVM intermediate code (IR)
static llvm::LLVMContext TheContext;
static llvm::IRBuilder<> Builder(TheContext);
static llvm::Function *TheFunction = 0;
// the calls to TheContext in the init above and in the
// following code ensures that we are incrementally generating
// instructions in the right order


typedef llvm::Value descriptor;
typedef map<string, descriptor* > symbol_table;
typedef list<symbol_table > symbol_table_list;
symbol_table_list symtbl;


descriptor* access_symtbl(string ident) {
    for (auto i : symtbl) {
        auto find_ident = i.find(ident);
        if (find_ident != i.end()) {
            return find_ident->second;
        }
    }
    return NULL;
}



llvm::Type *getLLVMType(string ty) {
	if(ty == "VoidType")
		return Builder.getVoidTy();
	else if(ty == "IntType")
		return Builder.getInt32Ty();
	else if(ty == "BoolType")
		return Builder.getInt1Ty();
	else if(ty == "StringType")
		return Builder.getInt8PtrTy();
	else
		throw runtime_error("unknown type");  
}

llvm::Constant *getZeroInit(string ty) {
		if(ty == "IntType") 
			return Builder.getInt32(0);
		else if(ty == "BoolType")
			return Builder.getInt1(0);
		else
			throw runtime_error("unknown type");
}


/// decafAST - Base class for all abstract syntax tree nodes.
class decafAST {
public:
  virtual ~decafAST() {}
  virtual string str() { return string(""); }
  virtual llvm::Value *Codegen() = 0;
};

string getString(decafAST *d) {
	if (d != NULL) {
		return d->str();
	} else {
		return string("None");
	}
}

template <class T>
string commaList(list<T> vec) {
    string s("");
    for (typename list<T>::iterator i = vec.begin(); i != vec.end(); i++) { 
        s = s + (s.empty() ? string("") : string(",")) + (*i)->str(); 
    }   
    if (s.empty()) {
        s = string("None");
    }   
    return s;
}

template <class T>
llvm::Value *listCodegen(list<T> vec) {
  llvm::Value *val = NULL;
  for (typename list<T>::iterator i = vec.begin(); i != vec.end(); i++) { 
     llvm::Value *j = (*i)->Codegen();
       if (j != NULL) { val = j; }
     } 
  return val;
}
class VarDefAST : public decafAST {
	string Name;
	string Type;
public:
	VarDefAST(string name, string type): Name(name), Type(type) {}
	string returnType() { return Type;}
	string returnName() { return Name;}
	string str() {return string("VarDef") + "(" + Name + "," + Type + ")" ;}
	llvm::Value* Codegen(){
		//if(Builder.GetInsertBlock()->getParent() == NULL)
		//	throw runtime_error("VarDefAST get parent error");
		llvm::Type* llType = getLLVMType(Type);
		llvm::AllocaInst*Alloca= Builder.CreateAlloca(llType, 0, Name.c_str());
		symtbl.front().insert(pair<string, descriptor*>(Name, Alloca));

		return Alloca;
	}
	llvm::Type* llvmTypeReturn(){
		return getLLVMType(Type);
	}

};

/// decafStmtList - List of Decaf statements
class decafStmtList : public decafAST {
	list<decafAST *> stmts;
public:
	decafStmtList() {}
	~decafStmtList() {
		for (list<decafAST *>::iterator i = stmts.begin(); i != stmts.end(); i++) { 
			delete *i;
		}
	}
	int size() { return stmts.size(); }
	decafAST* lastElement() { return stmts.back(); }
	void push_front(decafAST *e) { stmts.push_front(e); }
	void push_back(decafAST *e) { stmts.push_back(e); }
	string str() { return commaList<class decafAST *>(stmts); }
	vector<llvm::Type *> returnArgs() {
		vector<llvm::Type*> toReturn;
		VarDefAST* temp;
		for (list<decafAST *>::iterator i = stmts.begin(); i != stmts.end(); i++) { 
			temp = (VarDefAST*)(*i);
     		llvm::Type *j = temp->llvmTypeReturn();
       		if (j != NULL) { toReturn.push_back((llvm::Type*)j); }
     	} 
     	return toReturn;

	}
	vector<llvm::Type *> returnArgsE() {
		vector<llvm::Type*> toReturn;
		for (list<decafAST *>::iterator i = stmts.begin(); i != stmts.end(); i++) { 
     		llvm::Type *j = (llvm::Type *)(*i)->Codegen();
       		if (j != NULL) { toReturn.push_back((llvm::Type*)j); }
     	} 
     	return toReturn;

	}
	vector<llvm::Value *> returnArgsV() {
		vector<llvm::Value*> toReturn;
		for (list<decafAST *>::iterator i = stmts.begin(); i != stmts.end(); i++) { 
     		llvm::Value *j = (*i)->Codegen();
       		if (j != NULL) { toReturn.push_back(j); }
     	} 
     	return toReturn;

	}
	list<decafAST *> returnList(){
		return stmts;
	}
  	llvm::Value *Codegen() {
    	return listCodegen<decafAST *>(stmts);
  	}

};

class PackageAST : public decafAST {
	string Name;
	decafStmtList *FieldDeclList;
	decafStmtList *MethodDeclList;
public:
	PackageAST(string name, decafStmtList *fieldlist, decafStmtList *methodlist) 
		: Name(name), FieldDeclList(fieldlist), MethodDeclList(methodlist) {}
	~PackageAST() { 
		if (FieldDeclList != NULL) { delete FieldDeclList; }
		if (MethodDeclList != NULL) { delete MethodDeclList; }
	}
	string str() { 
		return string("Package") + "(" + Name + "," + getString(FieldDeclList) + "," + getString(MethodDeclList) + ")";
	}
	llvm::Value *Codegen() { 
		llvm::Value *val = NULL;
		if (NULL != FieldDeclList) {
			val = FieldDeclList->Codegen();
		}
		if (NULL != MethodDeclList) {
			val = MethodDeclList->Codegen();
		} 
		// Q: should we enter the class name into the symbol table?
		return val; 
	}
};

/// ProgramAST - the decaf program
class ProgramAST : public decafAST {
	decafStmtList *ExternList;
	PackageAST *PackageDef;
public:
	ProgramAST(decafStmtList *externs, PackageAST *c) : ExternList(externs), PackageDef(c) {}
	~ProgramAST() { 
		if (ExternList != NULL) { delete ExternList; } 
		if (PackageDef != NULL) { delete PackageDef; }
	}
	string str() { return string("Program") + "(" + getString(ExternList) + "," + getString(PackageDef) + ")"; }
	llvm::Value *Codegen() { 
		llvm::Value *val = NULL;
		if (NULL != ExternList) {
			val = ExternList->Codegen();
		}
		if (NULL != PackageDef) {
			val = PackageDef->Codegen();
		} else {
			throw runtime_error("no package definition in decaf program");
		}
		return val; 
	}
};

class BlockAST : public decafAST {
	decafStmtList *varDefList;
	decafStmtList *statement_list;
public:
	BlockAST(decafStmtList* vList, decafStmtList* sList): varDefList(vList), statement_list(sList) {} 
	~BlockAST(){ 
		if(varDefList != NULL) {delete varDefList;}
		if(statement_list != NULL) {delete statement_list;}
	}
	string str() { return string("Block") + "(" + getString(varDefList) + "," + getString(statement_list) + ")"; }
	llvm::Value* Codegen(){
		//llvm::BasicBlock*BB = llvm::BasicBlock::Create(TheContext, "entry", (llvm::Function*)access_symtbl("func"));
		//symtbl.front().insert(pair<string, descriptor*>(string("entry"),(llvm::Value*) BB));
		//Builder.SetInsertPoint(BB);
		llvm::Value *val = NULL;
		if (NULL != varDefList) {
			val = varDefList->Codegen();
		}else {
			throw runtime_error("Block AST Problem");
		}
		if (NULL != statement_list) {
			val = statement_list->Codegen();
		} else {
			throw runtime_error("Block AST Problem");
		}
		return val;
	}
};



class BreakStatementAST : public decafAST {
public:
	BreakStatementAST() {}
	string str() {return string("BreakStmt");}
	llvm::Value *Codegen() { return 0;}
};

class ContinueStatementAST: public decafAST {
public:
	ContinueStatementAST() {}
	string str() {return string("ContinueStmt");}
	llvm::Value *Codegen() { return 0;}
};

class ReturnStatementAST: public decafAST {
	decafAST *expr;
public:
	ReturnStatementAST(decafAST *input): expr(input) {}
	ReturnStatementAST(): expr(NULL) {}
	~ReturnStatementAST() {if (expr != NULL) { delete expr; } }
	string str() { return string("ReturnStmt") + "(" + getString(expr) + ")" ;}
	llvm::Value *Codegen() { 
		return Builder.CreateRet(expr->Codegen());
	}
};


class ForStmtAST: public decafAST {
	decafStmtList *pre_assign_list;
	decafStmtList *loop_assign;
	decafAST *expr;
	decafAST *block;
public:
	ForStmtAST(decafStmtList *pre, decafAST* constant, decafStmtList *loop, decafAST *inputBlock): pre_assign_list(pre), loop_assign(loop), expr(constant), block(inputBlock) {}
	~ForStmtAST() { 
		if (pre_assign_list != NULL) { delete pre_assign_list; } 
		if (loop_assign != NULL) { delete loop_assign; }
		if (expr != NULL) { delete expr; }
		if (block != NULL) { delete block; }
	}
	string str() {return string("ForStmt") + "(" + getString(pre_assign_list) + "," + getString(expr) + "," + getString(loop_assign) + ","+ getString(block)  + ")" ;}
	llvm::Value *Codegen() { return 0;}
};

class IfStmtAST: public decafAST {
	decafAST *expr;
	decafAST *block;
	decafAST *elseBlock;
public:
	IfStmtAST(decafAST* inputExpr, decafAST* inputBlock, decafAST* inputElse): expr(inputExpr), block(inputBlock), elseBlock(inputElse) {}
	IfStmtAST(decafAST* inputExpr, decafAST* inputBlock): expr(inputExpr), block(inputBlock) { elseBlock = NULL;}
	~IfStmtAST() { 
		if (block != NULL) { delete block; } 
		if (expr != NULL) { delete expr; }
		if (elseBlock != NULL) { delete elseBlock; }
	}
	string str() {return string("IfStmt") + "("+ getString(expr) + "," + getString(block) + "," + getString(elseBlock) +")";}
	llvm::Value *Codegen() { 
		llvm::Value* ifVal;
		llvm::BasicBlock* trueBB;
		llvm::BasicBlock* elseBB = NULL;
		llvm::BasicBlock* entryBB = NULL;
		llvm::BasicBlock* endBB = NULL;
		if(expr == NULL)
			throw runtime_error("Invalid ifstmt condition");
		if(elseBlock == NULL)
		{
			entryBB = llvm::BasicBlock::Create(TheContext, "ifentry",Builder.GetInsertBlock()->getParent());
			trueBB = llvm::BasicBlock::Create(TheContext, "iftrue",Builder.GetInsertBlock()->getParent());
			endBB = llvm::BasicBlock::Create(TheContext, "ifend",Builder.GetInsertBlock()->getParent());
			Builder.CreateBr(entryBB);
			Builder.SetInsertPoint(entryBB);
			ifVal = expr->Codegen();
			Builder.CreateCondBr(ifVal, trueBB, endBB);
			Builder.SetInsertPoint(trueBB);
			block->Codegen();
			Builder.CreateBr(endBB);
			Builder.SetInsertPoint(endBB);
		}
		else
		{
			entryBB = llvm::BasicBlock::Create(TheContext, "ifentry",Builder.GetInsertBlock()->getParent());
			trueBB = llvm::BasicBlock::Create(TheContext, "iftrue",Builder.GetInsertBlock()->getParent());
			elseBB = llvm::BasicBlock::Create(TheContext, "iffalse",Builder.GetInsertBlock()->getParent());
			endBB = llvm::BasicBlock::Create(TheContext, "ifend",Builder.GetInsertBlock()->getParent());
			Builder.CreateBr(entryBB);
			Builder.SetInsertPoint(entryBB);
			ifVal = expr->Codegen();
			Builder.CreateCondBr(ifVal, trueBB, elseBB);
			Builder.SetInsertPoint(trueBB);
			block->Codegen();
			Builder.CreateBr(endBB);
			Builder.SetInsertPoint(elseBB);
			elseBlock->Codegen();
			Builder.CreateBr(endBB);
			Builder.SetInsertPoint(endBB);
		}
		return endBB;
	}
};


class WhileStmtAST: public decafAST {
	decafAST *expr;
	decafAST *block;
public:
	WhileStmtAST(decafAST* inputExpr, decafAST* inputBlock): expr(inputExpr), block(inputBlock) {}
	~WhileStmtAST() { 
		if (block != NULL) { delete block; } 
		if (expr != NULL) { delete expr; }
	}
	string str() {return string("WhileStmt") + "("+ getString(expr) + "," + getString(block) + ")";}
	llvm::Value *Codegen() { 
		/*
			llvm::Value* ifVal = expr->Codegen();
			llvm::BasicBlock* trueBB;
			llvm::BasicBlock* entryBB = NULL;
			llvm::BasicBlock* endBB = NULL;			
			entryBB = llvm::BasicBlock::Create(TheContext, "whileentry",Builder.GetInsertBlock()->getParent());
			trueBB = llvm::BasicBlock::Create(TheContext, "whiletrue",Builder.GetInsertBlock()->getParent());
			endBB = llvm::BasicBlock::Create(TheContext, "whileend",Builder.GetInsertBlock()->getParent());
			Builder.CreateBr(entryBB);
			Builder.SetInsertPoint(entryBB);
			ifVal = expr->Codegen();
			Builder.CreateCondBr(ifVal, trueBB, endBB);
			Builder.SetInsertPoint(trueBB);
			block->Codegen();
			Builder.CreateBr(entryBB);
			Builder.SetInsertPoint(endBB);
			return endBB;
			*/
		return 0;
		}

};




class AssignVarAST : public decafAST { 
	string Name;
	decafAST* Expr;
public:
	AssignVarAST(string name, decafAST* expr): Name(name), Expr(expr) {}
	~AssignVarAST() { if(Expr != NULL) { delete Expr; }}
	string str() { return string("AssignVar") + "("+ Name + "," + getString(Expr) + ")" ;}
	llvm::Value* Codegen(){
		llvm::AllocaInst* Alloca;
		llvm::Value *val;
		llvm::Value* tempVal;
		llvm::Value* checkVal = access_symtbl(Name);
		if(checkVal != NULL)
			Alloca = (llvm::AllocaInst*)access_symtbl(Name);
		else{
			throw runtime_error("assigning to non existent variable");
		}
		tempVal = Expr->Codegen();

		//if(val == NULL){
		//val = Builder.CreateAlloca(tempVal->getType(),0,Name.c_str());
		val = Builder.CreateStore(tempVal, Alloca);
		symtbl.front().insert(pair<string, descriptor*>(Name, val));
//
//		/}
//		else{
//			val = Builder.CreateStore(tempVal, Alloca);
//		}
		return val;

	}
};

//AssignArrayLoc(identifier name, expr index, expr value)
class AssignArrayLocAST : public decafAST { 
	decafAST* Lval;
	decafAST* Expr;
public:
	AssignArrayLocAST(decafAST* lval ,decafAST* expr): Lval(lval) ,Expr(expr) {}
	~AssignArrayLocAST() { 
		if(Expr != NULL) { delete Expr; }
		if(Lval != NULL) { delete Lval; }
	}
	string str() { return string("AssignArrayLoc") + "("+ getString(Lval) +"," + getString(Expr) + ")" ;}
	 llvm::Value *Codegen() { 	llvm::AllocaInst* Alloca;
		llvm::Value* lVal = Lval->Codegen();
		llvm::Value* rVal = Expr->Codegen();
		if(lVal == NULL || rVal == NULL)
			throw runtime_error("AssignArrayLoc error");
		llvm::Value* storeVal= Builder.CreateStore(rVal,lVal); 
		//if(val == NULL){
		//val = Builder.CreateAlloca(tempVal->getType(),0,Name.c_str());
		symtbl.front().insert(pair<string, descriptor*>(lVal->getName(), storeVal));
//
//		/}
//		else{
//			val = Builder.CreateStore(tempVal, Alloca);
//		}
		return storeVal;
	}
};



class MethodArgAST: public decafAST {
	string Value;
public:
	MethodArgAST(string value): Value(value) {}
	string getValue() { return Value;}
	string str() { return string("StringConstant") + "(" + Value + ")" ;}
	llvm::Value* Codegen() {
		string temp = Value;
		temp.erase(temp.begin());
		temp.erase(temp.end()-1);
		size_t found= 0;
    	while((found = temp.find("\\n")) != std::string::npos) {
       		temp.replace(found, 2, "\n");
       	}
		found = 0;
    	while((found = temp.find("\\r")) != std::string::npos) {
       		temp.replace(found, 2, "\r");
       	}
		found = 0;
    	while((found = temp.find("\\v")) != std::string::npos) {
       		temp.replace(found, 2, "\v");
       	}
		found = 0;
    	while((found = temp.find("\\b")) != std::string::npos) {
       		temp.replace(found, 2, "\b");
       	}
		found = 0;
    	while((found = temp.find("\\a")) != std::string::npos) {
       		temp.replace(found, 2, "\a");
       	}
		found = 0;
    	while((found = temp.find("\\f")) != std::string::npos) {
       		temp.replace(found, 2, "\f");
       	}
		found = 0;
    	while((found = temp.find("\\t")) != std::string::npos) {
       		temp.replace(found, 2, "\t");
       	}
		found = 0;
    	while((found = temp.find("\\\"")) != std::string::npos) {
       		temp.replace(found, 2, "\"");
       	}
		found = 0;
    	while((found = temp.find("\\\\")) != std::string::npos) {
       		temp.replace(found, 2, "\\");
       	}
		llvm::GlobalVariable *GS = Builder.CreateGlobalString(temp, "globalstring");
		return Builder.CreateConstGEP2_32(GS->getValueType(),GS, 0, 0, "cast");
	}
};


class VariableExprAST: public decafAST {
	string Name;
public:
	VariableExprAST(string name): Name(name) {}
	string str() { return string("VariableExpr") + "(" + Name + ")" ;}
	 llvm::Value *Codegen() { 
	 	llvm::Value *V = access_symtbl(Name);
	 //	if(V != NULL)
	 		return Builder.CreateLoad(V, Name.c_str());
	 	//throw runtime_error("Variable not called");
	 }
};



class ArrayLocExprAST : public decafAST { 
	string Name;
	decafAST* Index;
public:
	ArrayLocExprAST(string name, decafAST* index): Name(name), Index(index) {}
	~ArrayLocExprAST() { if(Index != NULL) { delete Index; }}
	string str() { return string("ArrayLocExpr") + "("+ Name + "," + getString(Index) + ")" ;}
	llvm::Value *Codegen() { 
		llvm::GlobalVariable* array = (llvm::GlobalVariable*)access_symtbl(Name);
		llvm::ArrayType *arrayT = (llvm::ArrayType*)(array-> getValueType());
		llvm::Value *ArrayLoc = Builder.CreateStructGEP(arrayT, array, 0, "arrayloc");
		llvm::Value *index = Builder.getInt32(atoi(Index->str().c_str())); 	
		llvm::Value *ArrayIndex = Builder.CreateGEP(arrayT->getElementType(), ArrayLoc, index, "arrayindex");
		return ArrayIndex;
	}
};

 
class ArrayLValAST : public decafAST { 
	string Name;
	decafAST* Index;
public:
	ArrayLValAST(string name, decafAST* index): Name(name), Index(index) {}
	~ArrayLValAST() { if(Index != NULL) { delete Index; }}
	string str() { return string( Name + "," + getString(Index) ) ;}
	 llvm::Value *Codegen() { 		
	 	llvm::GlobalVariable* array = (llvm::GlobalVariable*)access_symtbl(Name);
		llvm::ArrayType *arrayT = (llvm::ArrayType*)(array-> getValueType());
		llvm::Value *ArrayLoc = Builder.CreateStructGEP(arrayT, array, 0, "arrayloc");
		llvm::Value *index = Builder.getInt32(atoi(Index->str().c_str())); 	
		llvm::Value *ArrayIndex = Builder.CreateGEP(arrayT->getElementType(), ArrayLoc, index, "arrayindex");
		return ArrayIndex;
	}
};






class NumberExprAST: public decafAST {
	string Value;
public:
	NumberExprAST(string value): Value(value) {}
	string str() { return string("NumberExpr") + "(" + Value + ")" ;}
	llvm::Value* Codegen(){
		return Builder.getInt32(stoi(Value));
	}
};


class BoolExprAST: public decafAST {
	string Value;
public:
	BoolExprAST(string value): Value(value) {}
	string str() { return string("BoolExpr") + "(" + Value + ")" ;}
	llvm::Value* Codegen(){
		if(Value == "True")
			return Builder.getInt1(1);
		else if(Value == "False")
			return Builder.getInt1(0);
		else
			throw runtime_error("BoolExpr error");
	}
};

//BinaryExpr(binary_operator op, expr left_value, expr right_value)

class BinaryExprAST : public decafAST { 
	string Op;
	decafAST* Left;
	decafAST* Right;
public:
	BinaryExprAST(string op,decafAST* left ,decafAST* right): Op(op), Left(left) , Right(right) {}
	~BinaryExprAST() { 
		if(Left != NULL) { delete Left; }
		if(Right != NULL) { delete Right; }
	}
	string str() { return string("BinaryExpr") + "("+ Op + "," + getString(Left) +"," + getString(Right) + ")" ;}
	llvm::Value *Codegen() {
	  llvm::Value *L = Left->Codegen();
	  llvm::Value *R = Right->Codegen();
	  if (L == 0 || R == 0) return 0;
	  if( L->getType() != R->getType() && (L->getType()->isIntegerTy() && R->getType()->isIntegerTy())){
	  	llvm::Value *promo = Builder.CreateZExt(L, Builder.getInt32Ty(), "zexttmp");
     	L = promo;
     	llvm::Value *promo1 = Builder.CreateZExt(R, Builder.getInt32Ty(), "zexttmp");
     	R = promo1;
	  }
	  
	  if(Op == "Minus")
	  	return Builder.CreateSub(L, R, "subtmp");
	  else if(Op == "Plus")
	  	return Builder.CreateAdd(L, R, "addtmp");
	  else if(Op == "Mult")
	  	return Builder.CreateMul(L, R, "multmp");
	  else if(Op == "Div")
	  	return Builder.CreateSDiv(L, R, "sdivtmp");
	  else if(Op == "Mod")
	  	return Builder.CreateSRem(L, R, "sremtmp");
	  else if(Op == "Leftshift")
	  	return Builder.CreateShl(L, R, "shltmp");
	  else if(Op == "Rightshift")
	  	return Builder.CreateLShr(L, R, "lshrtmp");
	  else if(Op == "Lt")
	  	return Builder.CreateICmpSLT(L, R, "cmpslttmp");
	  else if(Op == "Leq")
	  	return Builder.CreateICmpSLE(L, R, "cmpsletmp");
	  else if(Op == "Gt")
	  	return Builder.CreateICmpSGT(L, R, "cmpsgttmp");
	  else if(Op == "Geq")
	  	return Builder.CreateICmpSGE(L, R, "cmpsgetmp");
	  else if(Op == "And")
	  	return Builder.CreateAnd(L, R, "andtmp");
	  else if(Op == "Or")
	  	return Builder.CreateOr(L, R, "ortmp");
	  else if(Op == "Eq")
	  	return Builder.CreateICmpEQ(L, R, "cmpeqtmp");
	  else if(Op == "Neq")
	  	return Builder.CreateICmpNE(L, R, "cmpnetmp");
	  throw runtime_error("binary expr fault");
	}
};



class UnaryExprAST : public decafAST { 
	string Op;
	decafAST* Value;
public:
	UnaryExprAST(string op, decafAST* value): Op(op), Value(value) {}
	~UnaryExprAST() { if(Value != NULL) { delete Value; }}
	string str() { return string("UnaryExpr") + "("+ Op + "," + getString(Value) + ")" ;}
	llvm::Value* Codegen(){
		llvm::Value *V = Value->Codegen();
		if(Op == "UnaryMinus")
	  		return Builder.CreateNeg(V, "negtmp");
	  	else if(Op == "Not")
	  		return Builder.CreateNot(V, "nottmp");
	  	throw runtime_error("unary expr fault");
	}
};


class MethodBlockAST : public decafAST {
	decafStmtList *varDefList;
	decafStmtList *statement_list;
public:
	MethodBlockAST(decafStmtList* vList, decafStmtList* sList): varDefList(vList), statement_list(sList) {} 
	~MethodBlockAST(){ 
		if(varDefList != NULL) {delete varDefList;}
		if(statement_list != NULL) {delete statement_list;}
	}
	string str() { return string("MethodBlock") + "(" + getString(varDefList) + "," + getString(statement_list) + ")"; }
	llvm::Value* Codegen(){
		llvm::Value *val = NULL;
		llvm::Function *func= Builder.GetInsertBlock()->getParent();
		llvm::Function::arg_iterator iter = func -> arg_begin();
		/*
		while(iter != func->arg_end()){
			string iterName = string((iter)->getName());
			llvm::Type* iterType = (iter)->getType();
			llvm::AllocaInst* Alloca;
			val = access_symtbl(iterName);
			Alloca = Builder.CreateAlloca(iterType,0,iterName.c_str());
			val = Builder.CreateStore(val, Alloca);
			symtbl.front().insert(pair<string, descriptor*>(iterName, val));
			iter++;

		}
		*/
		if (NULL != varDefList) {
			val = varDefList->Codegen();
		}
		if (NULL != statement_list) {
			val = statement_list->Codegen();
		}

		return val;
	}

};

//Method(identifier name, method_type return_type, typed_symbol* param_list, method_block block)
class MethodDeclAST: public decafAST {
	string Name;
	decafStmtList* DecVarList;
	string MType;
	decafAST* MBlock;
public:
	MethodDeclAST(string name, decafStmtList* list, string type, decafAST* block ) : Name(name), DecVarList(list), MType(type), MBlock(block) {}
	~MethodDeclAST(){ 
		if(DecVarList != NULL) {delete DecVarList;}
		if(MBlock != NULL) {delete MBlock;}
	}
	string str() { return string("Method") + "("+ Name + "," + MType + "," + getString(DecVarList) + "," + getString(MBlock)+ ")"; }
	llvm::Value *Codegen(){
		llvm::Type *returnTy= getLLVMType(MType);
		llvm::Function *func = NULL;
		vector<llvm::Type *> args = DecVarList->returnArgs();
		if(Name == "main"){
			llvm::FunctionType *FT = llvm::FunctionType::get(llvm::IntegerType::get(TheContext, 32), false);
			TheFunction = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main", TheModule);
			symtbl.front().insert(pair<string,descriptor*>("main", (llvm::Value*) TheFunction));
			if (TheFunction == 0) {
				throw runtime_error("empty function block"); 
			}
			// Create a new basic block which contains a sequence of LLVM instructions
			llvm::BasicBlock *BB = llvm::BasicBlock::Create(TheContext, "entry", TheFunction);
			// All subsequent calls to IRBuilder will place instructions in this location
			Builder.SetInsertPoint(BB);
		}
		else{
			func= llvm::Function::Create(llvm::FunctionType::get(returnTy, args, false),llvm::Function::ExternalLinkage,Name,TheModule);
			list<decafAST *> argList = DecVarList->returnList();
			llvm::Function::arg_iterator iter = func -> arg_begin();
			for (list<decafAST *>::iterator i = argList.begin(); i != argList.end(); i++) { 
				iter->setName(((VarDefAST*)(*i))->returnName());
				iter++;
			}

			symtbl.front().insert(pair<string,descriptor*>(Name, func));
			llvm::BasicBlock*BB = llvm::BasicBlock::Create(TheContext, "entry", func);
			Builder.SetInsertPoint(BB);
			symtbl.front().insert(pair<string, descriptor*>(string("entry"),(llvm::Value*) BB));
			iter = func -> arg_begin();
			for (list<decafAST *>::iterator i = argList.begin(); i != argList.end(); i++) { 
				llvm::AllocaInst*Alloca= Builder.CreateAlloca(getLLVMType(((VarDefAST*)(*i))->returnType()), nullptr, string(iter->getName()).c_str());// Store the initial value into the alloca.
				Builder.CreateStore(static_cast<llvm::Value *>(&*iter), Alloca);// Add to symbol table
				symtbl.front().insert(pair<string, descriptor*>(iter->getName(), Alloca));
				iter++;
			}
		}
		symbol_table MblocTable;
		symtbl.push_front(MblocTable);
		llvm::Value* temp =MBlock->Codegen();
		if(MType == "BoolType" )
			Builder.CreateRet(Builder.getInt1(0));
		if(MType == "IntType" )
			Builder.CreateRet(Builder.getInt32(0));
		if(MType == "VoidType")
			Builder.CreateRetVoid();
		symtbl.pop_front();
		return func;
	}
};

class MethodCallAST	: public decafAST {
	string Name;
	decafStmtList *method_arg_list;
public:
	MethodCallAST(string name, decafStmtList *mArgList): Name(name), method_arg_list(mArgList) {}
	~MethodCallAST() { 
		if (method_arg_list != NULL) { delete method_arg_list; }
	}
	string str() { 
		return string("MethodCall") + "(" + Name + "," + getString(method_arg_list) + ")";
	}
	llvm::Value* Codegen(){


		llvm::Function *call= (llvm::Function*)access_symtbl(Name);
		// assign this to the pointer to the function to call, 
		// usually loaded from the symbol table
		vector<llvm::Value *> args = method_arg_list->returnArgsV();
		llvm::Argument* iter = call->arg_begin();
		for (vector<llvm::Value *>::iterator i = args.begin(); i != args.end(); i++) { 
     		if(((*i)->getType() != iter ->getType())&& (*i)->getType()->isIntegerTy() == true){
     			llvm::Value *promo = Builder.CreateZExt(*i, Builder.getInt32Ty(), "zexttmp");
     			*i = promo;
     		}
       		iter++;
     	}
		// argvals are the values in the method call, 
		// e.g. foo(1) would have a vector of size one with value of 1 with type i32.

		bool isVoid = call->getReturnType()->isVoidTy();
		llvm::Value *val = Builder.CreateCall(
		    call,
		    args,
		    isVoid ? "" : "calltmp"
		);

		return val;
	}
};

class ParenExprAST: public decafAST {
	decafAST* Value;
public:
	ParenExprAST(decafAST* value): Value(value) {}
	string str() { return "(" + getString(Value) + ")" ;}
	 llvm::Value *Codegen() { return Value->Codegen();}
};

// VarDef(StringType) | VarDef(decaf_type)
class ExternTypeAST : public decafAST {
	string Name;
public:
	ExternTypeAST(string name): Name(name) {}
	string str() { return string("VarDef") + "(" + Name + ")" ;}
	 llvm::Value *Codegen() { 
	 	return (llvm::Value*)getLLVMType(Name);
	}
};


//extern = ExternFunction(identifier name, method_type return_type, extern_type* typelist)


class ExternFunctionAST : public decafAST {
	string Name;
	string ReturnType;
	decafAST* InputType;
public:
	ExternFunctionAST(string name, string returnType, decafAST* inputType): Name(name), ReturnType(returnType), InputType(inputType) {}
	~ExternFunctionAST() { if(InputType != NULL) {delete InputType;}}
	string str() { return string("ExternFunction") + "(" + Name + "," + ReturnType + "," + getString(InputType) + ")" ;}
	 llvm::Value *Codegen() { 
	 	llvm::Type *returnTy = getLLVMType(ReturnType);
	 	vector<llvm::Type *> args = ((decafStmtList*)InputType)->returnArgsE();
	 	llvm::Function* val = llvm::Function::Create(llvm::FunctionType::get(returnTy, args, false), llvm::Function::ExternalLinkage, Name, TheModule);
	 	symtbl.front().insert(pair<string,descriptor*>(Name, val));
	 	return val;
	 }
};


//FieldDecl(identifier name, decaf_type type, field_size size)

class ArrayAST: public decafAST {
	string ArrSize;
public:
	ArrayAST(string arrSize): ArrSize(arrSize) {}
	string str() { return string("Array") + "(" + ArrSize + ")" ;}
	string retSize(){return ArrSize;}
	llvm::Value *Codegen() { return 0;}
};


class FieldDeclAST : public decafAST {
	decafAST* Name;
	string Type;
	decafAST* FSize;

public:
	FieldDeclAST(decafAST* name, string type, decafAST* fSize): Name(name), Type(type), FSize(fSize) {}
	~FieldDeclAST() {
		if(Name != NULL) {delete Name;}
	}
	string returnType() { return Type;}
	string returnArr() { return FSize->str();}
	string str() {return string("FieldDecl") + "(" + getString(Name) + "," + Type + "," + FSize->str() + ")" ;}
	llvm::Value *Codegen() {
		if(FSize->str() != "Scalar")
		{
			llvm::ArrayType *array;
			if(Type == "IntType")
			{
				array = llvm::ArrayType::get(Builder.getInt32Ty(), atoi((((ArrayAST*)(FSize))->retSize()).c_str()));
			}
			else if(Type == "BoolType")
			{
				array = llvm::ArrayType::get(Builder.getInt1Ty(), atoi((((ArrayAST*)(FSize))->retSize()).c_str()));
			}
			else if(Type == "StringType")
			{
				array = llvm::ArrayType::get(Builder.getInt8PtrTy(), atoi((((ArrayAST*)(FSize))->retSize()).c_str()));
			}
			llvm::Constant *zeroInit = llvm::Constant::getNullValue(array);
			llvm::GlobalVariable *Foo = new llvm::GlobalVariable(*TheModule, array, false, llvm::GlobalValue::ExternalLinkage, zeroInit, Name->str());
			symtbl.front().insert(pair<string,descriptor*>(Name->str(), Foo));
			return Foo;
		}
		else{
			llvm::GlobalVariable *Foo = new llvm::GlobalVariable(
			    *TheModule, 
			    getLLVMType(Type), 
			    false,  // variable is mutable
			    llvm::GlobalValue::InternalLinkage, 
			    getZeroInit(Type), 
			    Name->str()
    		);
    		symtbl.front().insert(pair<string,descriptor*>(Name->str(), Foo));
			return Foo;
 		}
		//throw runtime_error("FieldDecl");
	}

};


//Array(int array_size)



class ScalarAST: public decafAST {
public:
	ScalarAST() {}
	string str() {return string("Scalar");}
	llvm::Value *Codegen() { throw runtime_error("scalar");}

};

//AssignGlobalVar(identifier name, decaf_type type, expr value)
//TO BE FIXED
class AssignGlobalVarAST : public decafAST {
	string Name;
	string Type;
	decafAST* Expr;
public:
	AssignGlobalVarAST(string name, string type, decafAST* expr): Name(name), Type(type), Expr(expr) {}
	~AssignGlobalVarAST() {if(Expr != NULL) {delete Expr;}}
	string str() {return string("AssignGlobalVar") + "(" + Name + "," + Type + "," + getString(Expr) + ")" ;}
	 llvm::Value *Codegen() { 
	 	llvm::GlobalVariable *Foo = new llvm::GlobalVariable(
		    *TheModule, 
		    getLLVMType(Type), 
		    false,  // variable is mutable
		    llvm::GlobalValue::InternalLinkage, 
		    getZeroInit(Type), 
		    Name
		);
		symtbl.front().insert(pair<string,descriptor*>(Name, Foo));
		return Foo;
	}
};


//Method(identifier name, method_type return_type, typed_symbol* param_list, method_block block)
/*
class MethodAST : public decafAST {
	string Name;
	string Type;
	decafAST* MBlock;
public:
	MethodAST(string name, string type, decafAST* mBlock): Name(name), Type(Type), MBlock(mBlock) {}
	~MethodAST() {if(MBlock != NULL) {delete MBlock;}}
	string str() {return string("FieldDecl") + "(" + Name + "," + Type + "," + getString(MBlock) + ")" ;}
};
*/

class IdAST: public decafAST {
	string Name;
public:
	IdAST(string name): Name(name) {}
	string str() {return Name;}
	llvm::Value *Codegen() { throw runtime_error("idast");}
};

