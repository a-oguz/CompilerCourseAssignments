
#include "default-defs.h"
#include <list>
#include <ostream>
#include <iostream>
#include <sstream>

#ifndef YYTOKENTYPE
#include "decafast.tab.h"
#endif

using namespace std;

/// decafAST - Base class for all abstract syntax tree nodes.
class decafAST {
public:
  virtual ~decafAST() {}
  virtual string str() { return string(""); }
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

};

class VarDefAST : public decafAST {
	string Name;
	string Type;
public:
	VarDefAST(string name, string type): Name(name), Type(type) {}
	string returnType() { return Type;}
	string str() {return string("VarDef") + "(" + Name + "," + Type + ")" ;}

};

class BreakStatementAST : public decafAST {
public:
	BreakStatementAST() {}
	string str() {return string("BreakStmt");}
};

class ContinueStatementAST: public decafAST {
public:
	ContinueStatementAST() {}
	string str() {return string("ContinueStmt");}
};

class ReturnStatementAST: public decafAST {
	decafAST *expr;
public:
	ReturnStatementAST(decafAST *input): expr(input) {}
	ReturnStatementAST(): expr(NULL) {}
	~ReturnStatementAST() {if (expr != NULL) { delete expr; } }
	string str() { return string("ReturnStmt") + "(" + getString(expr) + ")" ;}
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
};


class AssignVarAST : public decafAST { 
	string Name;
	decafAST* Expr;
public:
	AssignVarAST(string name, decafAST* expr): Name(name), Expr(expr) {}
	~AssignVarAST() { if(Expr != NULL) { delete Expr; }}
	string str() { return string("AssignVar") + "("+ Name + "," + getString(Expr) + ")" ;}
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
};



class MethodArgAST: public decafAST {
	string Value;
public:
	MethodArgAST(string value): Value(value) {}
	string str() { return string("StringConstant") + "(" + Value + ")" ;}
};


class VariableExprAST: public decafAST {
	string Name;
public:
	VariableExprAST(string name): Name(name) {}
	string str() { return string("VariableExpr") + "(" + Name + ")" ;}
};



class ArrayLocExprAST : public decafAST { 
	string Name;
	decafAST* Index;
public:
	ArrayLocExprAST(string name, decafAST* index): Name(name), Index(index) {}
	~ArrayLocExprAST() { if(Index != NULL) { delete Index; }}
	string str() { return string("ArrayLocExpr") + "("+ Name + "," + getString(Index) + ")" ;}
};

 
class ArrayLValAST : public decafAST { 
	string Name;
	decafAST* Index;
public:
	ArrayLValAST(string name, decafAST* index): Name(name), Index(index) {}
	~ArrayLValAST() { if(Index != NULL) { delete Index; }}
	string str() { return string( Name + "," + getString(Index) ) ;}
};






class NumberExprAST: public decafAST {
	string Value;
public:
	NumberExprAST(string value): Value(value) {}
	string str() { return string("NumberExpr") + "(" + Value + ")" ;}
};


class BoolExprAST: public decafAST {
	string Value;
public:
	BoolExprAST(string value): Value(value) {}
	string str() { return string("BoolExpr") + "(" + Value + ")" ;}
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
};



class UnaryExprAST : public decafAST { 
	string Op;
	decafAST* Value;
public:
	UnaryExprAST(string op, decafAST* value): Op(op), Value(value) {}
	~UnaryExprAST() { if(Value != NULL) { delete Value; }}
	string str() { return string("UnaryExpr") + "("+ Op + "," + getString(Value) + ")" ;}
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

};

class ParenExprAST: public decafAST {
	decafAST* Value;
public:
	ParenExprAST(decafAST* value): Value(value) {}
	string str() { return "(" + getString(Value) + ")" ;}
};

// VarDef(StringType) | VarDef(decaf_type)
class ExternTypeAST : public decafAST {
	string Name;
public:
	ExternTypeAST(string name): Name(name) {}
	string str() { return string("VarDef") + "(" + Name + ")" ;}
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
};


//FieldDecl(identifier name, decaf_type type, field_size size)



class FieldDeclAST : public decafAST {
	decafAST* Name;
	string Type;
	string FSize;

public:
	FieldDeclAST(decafAST* name, string type, string fSize): Name(name), Type(type), FSize(fSize) {}
	~FieldDeclAST() {
		if(Name != NULL) {delete Name;}
	}
	string returnType() { return Type;}
	string returnArr() { return FSize;}
	string str() {return string("FieldDecl") + "(" + getString(Name) + "," + Type + "," + FSize + ")" ;}
};


//Array(int array_size)

class ArrayAST: public decafAST {
	string ArrSize;
public:
	ArrayAST(string arrSize): ArrSize(arrSize) {}
	string str() { return string("Array") + "(" + ArrSize + ")" ;}
};


class ScalarAST: public decafAST {
public:
	ScalarAST() {}
	string str() {return string("Scalar");}
};

//AssignGlobalVar(identifier name, decaf_type type, expr value)

class AssignGlobalVarAST : public decafAST {
	string Name;
	string Type;
	decafAST* Expr;
public:
	AssignGlobalVarAST(string name, string type, decafAST* expr): Name(name), Type(type), Expr(expr) {}
	~AssignGlobalVarAST() {if(Expr != NULL) {delete Expr;}}
	string str() {return string("AssignGlobalVar") + "(" + Name + "," + Type + "," + getString(Expr) + ")" ;}
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
};


class DecVarAST : public decafAST {
	string Name;
	string Type;
public:
	DecVarAST(string name, string type): Name(name), Type(type) {}
	string str() {return string("(") + Name + "," + Type + ")" ;}

};