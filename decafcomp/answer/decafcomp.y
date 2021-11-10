%{
#include <iostream>
#include <ostream>
#include <string>
#include <cstdlib>
#include "default-defs.h"

#define YYDEBUG 1


int yylex(void);
int yyerror(char *); 

// print AST?
bool printAST = false;

using namespace std;


typedef llvm::Value descriptor;



#include "decafcomp.cc"

llvm::Function *gen_main_def() {
  // create the top-level definition for main
  llvm::FunctionType *FT = llvm::FunctionType::get(Builder.getVoidTy(), false);
  llvm::Function *TheFunction = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "WillNeverBeUsed", TheModule);
  symtbl.front().insert(pair<string,descriptor*>("WillNeverBeUsed", (llvm::Value*) TheFunction));
  if (TheFunction == 0) {
    throw runtime_error("empty function block"); 
  }
  // Create a new basic block which contains a sequence of LLVM instructions
  llvm::BasicBlock *BB = llvm::BasicBlock::Create(TheContext, "entry", TheFunction);
  // All subsequent calls to IRBuilder will place instructions in this location
  Builder.SetInsertPoint(BB);
//  Builder.CreateRet(llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0)));
  return TheFunction;
}

%}

%union{
    class decafAST *ast;
    std::string *sval;
 }

%token T_BOOLTYPE
%token T_BREAK
%token T_CONTINUE
%token T_ELSE
%token T_EXTERN
%token T_FALSE
%token T_FOR
%token T_FUNC
%token T_IF
%token T_INTTYPE
%token T_NULL
%token T_PACKAGE
%token T_RETURN
%token T_STRINGTYPE
%token T_TRUE
%token T_VAR
%token T_VOID
%token T_WHILE
%token T_LCB
%token T_RCB
%token T_LSB
%token T_RSB
%token T_COMMA
%token T_SEMICOLON
%token T_LPAREN
%token T_RPAREN
%token T_ASSIGN
%token T_MINUS
%token T_NOT
%token T_PLUS
%token T_MULT
%token T_DIV
%token T_LBW
%token T_RBW
%token T_LT
%token T_GT
%token T_MOD
%token T_LEQ
%token T_GEQ
%token T_EQ
%token T_NEQ
%token T_AND
%token T_OR
%token T_DOT
%token <sval> T_ID

%token <sval> T_INTCONSTANT
%token <sval> T_CHARCONSTANT
%token <sval> T_STRINGCONSTANT

//%token T_COMMENT

%type <sval> decaf_type method_type unaryNot unaryMinus boolAnd boolOr boolRest arithRest plusMinus


%type <ast> extern_list extern externR block var-decl-list var-decl extern_type extern_typeR const bool_const assign assignR expr method_call method-arg-list method-arg statements statement method-dec-list method-dec arrayType dec-var-struct dec-var-structR decafpackage field-decR field-dec mBlock mul-field-decR mul-arr-decR id_list_var expr1 expr2 expr3 expr4 expr5 expr6 expr7 lvalue
 



%%

start: program


program: extern_list decafpackage
    { 
        ProgramAST *prog = new ProgramAST((decafStmtList *)$1, (PackageAST *)$2); 
		if (printAST) {
			cout << getString(prog) << endl;
		}
		try {
            prog->Codegen();
        } 
        catch (std::runtime_error &e) {
            cout << "semantic error: " << e.what() << endl;
            //cout << prog->str() << endl; 
            exit(EXIT_FAILURE);
        }
        delete prog;
    }

extern_list: externR
    {
    	decafStmtList *slist = new decafStmtList(); slist->push_front($1); $$ = slist;
    }
    | /* extern_list can be empty */
    { decafStmtList *slist = new decafStmtList(); $$ = slist; }
    ;

decafpackage: T_PACKAGE T_ID T_LCB field-decR method-dec-list T_RCB
    { $$ = new PackageAST(*$2, (decafStmtList*)$4, (decafStmtList*)$5); delete $2; }
    ;

block : T_LCB var-decl-list statements T_RCB {$$ = new BlockAST((decafStmtList *)$2, (decafStmtList *)$3);}

var-decl-list : var-decl var-decl-list 
	{
		decafStmtList *slist = (decafStmtList *)$2;
		slist -> push_front($1);
		$$ = slist;
	}
	| var-decl    {
    	decafStmtList *slist = new decafStmtList(); slist->push_front($1); $$ = slist;
    }
  	| { decafStmtList *slist = new decafStmtList(); $$ = slist; }
	;


var-decl: T_VAR id_list_var T_SEMICOLON { $$ = $2;}
 
id_list_var: T_ID T_COMMA id_list_var {
		decafStmtList *slist = (decafStmtList*) $3;
		VarDefAST* last = (VarDefAST*) (slist -> lastElement());
		string type = last -> returnType();
		$$ = new VarDefAST(*$1,type);
		slist -> push_front($$);
		$$ = slist;
	}
	| T_ID decaf_type { decafStmtList *slist = new decafStmtList(); $$ = new VarDefAST(*$1,*$2); slist -> push_front($$); $$ = slist; delete $1; delete $2;}
	;


decaf_type: T_INTTYPE {$$ = new std::string("IntType");}
	| T_BOOLTYPE	{$$ = new std::string("BoolType");}
	;

method_type: T_VOID {$$ = new std::string("VoidType");}
	| decaf_type	{$$ = $1;}
	;

extern_type: T_STRINGTYPE 
	{ 
		std::string temp = "StringType";
		$$ = new ExternTypeAST(temp); 
	}
	| decaf_type { $$ = new ExternTypeAST(*$1) ; delete $1;}
	;

extern_typeR: extern_type T_COMMA extern_typeR  
	{
		decafStmtList *slist = (decafStmtList *)$3;
		slist -> push_front($1);
		$$ = slist;
	}
	| extern_type 
	{
		decafStmtList* temp = new decafStmtList();
		temp -> push_front($1);
		$$ = temp;
	}
	| { decafStmtList *slist = new decafStmtList(); $$ = slist; }
	;


const: T_INTCONSTANT {$$ = new NumberExprAST(*$1); delete $1;}
	| T_CHARCONSTANT 
	{
		std::string str = *$1;
		char *c = new char();
		*c = str[1];
		int temp = int(*c);
		if(temp == 92){
		std::string escapeChars = "'\"nrvfabt\\";
		std::size_t charEsc = escapeChars.find(str[2]);
			if(charEsc == string::npos){
				*c = str[2];
				temp = int(*c);
			}
			else{
				if(str == string("'\\\"'")){
					 *c = '\"';
					 temp = int(*c);
				}
				else if(str == string("'\\''")){
					 *c = '\'';
					 temp = int(*c);
				}
				else if(str == string("'\\n'")){
					 *c = '\n';
					 temp = int(*c);
				}
				else if(str == string("'\\r'")){
					 *c = '\r';
					 temp = int(*c);
				}
				else if(str == string("'\\f'")){
					 *c = '\f';
					 temp = int(*c);
				}
				else if(str == string("'\\a'")){
					 *c = '\a';
					 temp = int(*c);
				}
				else if(str == string("'\\b'")){
					 *c = '\b';
					 temp = int(*c);
				}
				else if(str == string("'\\t'")){
					 *c = '\t';
					 temp = int(*c);
				}
				else if(str == string("'\\\\'")){
					 *c = '\\';
					 temp = int(*c);
				}
				else if(str == string("'\\v'")){
					 *c = '\v';
					 temp = int(*c);
				}
			}
		}
		str = std::to_string(temp);
		delete c;
		$$ = new NumberExprAST(str);
		delete $1;
	}
	| bool_const	 {$$ = $1;}
	;

bool_const: T_TRUE {$$ = new BoolExprAST("True");}
	| T_FALSE	{$$ = new BoolExprAST("False");}
	;

assignR: assign assignR
	{
		decafStmtList *slist = (decafStmtList *)$2;
		slist -> push_front($1);
		$$ = slist;
	}
	| assign {decafStmtList *slist =  new decafStmtList(); slist -> push_front($1); $$ = slist;}
	;

assign: T_ID T_ASSIGN expr 
	{
		$$ = new AssignVarAST(*$1,$3); delete $1;
	}
	| lvalue T_ASSIGN expr { $$ = new AssignArrayLocAST($1,$3);}
	;

lvalue: T_ID T_LSB expr T_RSB { $$ = new ArrayLValAST(*$1,$3); delete $1;}	



 expr : expr boolOr expr1  { $$ = new BinaryExprAST(*$2,$1,$3); delete $2;}
	| expr1
	;

expr1 : expr1 boolAnd expr2  { $$ = new BinaryExprAST(*$2,$1,$3); delete $2;}
	| expr2
	;

expr2 : expr2 boolRest expr3  { $$ = new BinaryExprAST(*$2,$1,$3); delete $2;}
	| expr3
	;

expr3 : expr3 plusMinus expr4  { $$ = new BinaryExprAST(*$2,$1,$3); delete $2;}
	| expr4
	;

expr4 : expr4 arithRest expr5  { $$ = new BinaryExprAST(*$2,$1,$3); delete $2;}
	| expr5
	;

expr5 : unaryNot expr5  { $$ = new UnaryExprAST(*$1,$2), delete $1;}
	| unaryNot expr6  { $$ = new UnaryExprAST(*$1,$2), delete $1;}
	| expr6
	;

expr6 : unaryMinus expr6  { $$ = new UnaryExprAST(*$1,$2), delete $1;} 
	| unaryMinus expr7  { $$ = new UnaryExprAST(*$1,$2), delete $1;}
	| expr7
	;

expr7 : T_ID T_LSB expr T_RSB { $$ = new ArrayLocExprAST(*$1,$3); delete $1;}
	| T_ID {$$ = new VariableExprAST(*$1); delete $1; }
	| const {$$ = $1;}
	| T_LPAREN expr T_RPAREN { $$ = $2;}
	| method_call {$$ = $1;}
	;

unaryNot: T_NOT {$$ = new std::string("Not");}

unaryMinus: T_MINUS {$$ = new std::string("UnaryMinus");}

arithRest : T_MULT	{$$ = new std::string("Mult");}
	| T_DIV		{$$ = new std::string("Div");}
	| T_MOD		{$$ = new std::string("Mod");}
	| T_LBW		{$$ = new std::string("Leftshift");}
	| T_RBW		{$$ = new std::string("Rightshift");}
	;

plusMinus : T_PLUS {$$ = new std::string("Plus");}
	| T_MINUS	{$$ = new std::string("Minus");}
	;

boolRest : T_EQ 	{$$ = new std::string("Eq");}
	| T_NEQ 		{$$ = new std::string("Neq");}
	| T_LT 			{$$ = new std::string("Lt");}
	| T_LEQ			{$$ = new std::string("Leq");}
	| T_GT 			{$$ = new std::string("Gt");}
	| T_GEQ 		{$$ = new std::string("Geq");}
	;

boolAnd : T_AND			{$$ = new std::string("And");}
boolOr  : T_OR 			{$$ = new std::string("Or");}


method_call: T_ID T_LPAREN method-arg-list T_RPAREN 
	{ $$ = new MethodCallAST(*$1,(decafStmtList*)$3); delete $1;}

method-arg-list: method-arg T_COMMA method-arg-list
	{
		decafStmtList *slist = (decafStmtList*)$3;
		slist -> push_front($1);
		$$ = slist;
	}
	| method-arg  {decafStmtList *slist =  new decafStmtList(); slist -> push_front($1); $$ = slist;}
	| { decafStmtList *slist = new decafStmtList(); $$ = slist; }
	;

method-arg: expr {$$ = $1;}
	| T_STRINGCONSTANT {$$ = new MethodArgAST(*$1); delete $1;}
	;

arrayType : T_LSB T_INTCONSTANT T_RSB
	{
		$$ = new ArrayAST(*$2);
		delete $2;
	}
	;

statements: statement statements
	{
		decafStmtList *slist = (decafStmtList*)$2;
		slist -> push_front($1);
		$$ = slist;
	}
	| statement  {decafStmtList *slist =  new decafStmtList(); slist -> push_front($1); $$ = slist;}
	|  { decafStmtList *slist = new decafStmtList(); $$ = slist; }
	;

statement: block { $$ = $1; }
	| assign T_SEMICOLON { $$ = $1; }
	| method_call T_SEMICOLON { $$ = $1; }
	| T_IF T_LPAREN expr T_RPAREN block T_ELSE block 
	{ 
		$$ =  new IfStmtAST($3, $5, $7);
	}
	| T_IF T_LPAREN expr T_RPAREN block 
	{ 
		$$ =  new IfStmtAST($3, $5);
	}
	| T_WHILE T_LPAREN expr T_RPAREN block
	{
		$$ = new WhileStmtAST($3,$5);
	}
	| T_FOR T_LPAREN  assignR T_SEMICOLON expr T_SEMICOLON assignR T_RPAREN block
	{
		$$ = new ForStmtAST((decafStmtList*)$3,$5,(decafStmtList*)$7,$9);
	}
	| T_BREAK T_SEMICOLON { $$ = new BreakStatementAST();}
	| T_CONTINUE T_SEMICOLON { $$ = new ContinueStatementAST();}
	| T_RETURN expr T_SEMICOLON { $$ = new ReturnStatementAST($2);}
	| T_RETURN T_SEMICOLON { $$ = new ReturnStatementAST();}
	| T_RETURN T_LPAREN T_RPAREN T_SEMICOLON { decafAST* temp = new decafStmtList(); $$ = new ReturnStatementAST(temp);}
	;

method-dec-list : method-dec method-dec-list
	{
		decafStmtList *slist = (decafStmtList*)$2;
		slist -> push_front($1);
		$$ = slist;
	}
	| method-dec {decafStmtList *slist =  new decafStmtList(); slist -> push_front($1); $$ = slist;}
	;

method-dec: T_FUNC T_ID T_LPAREN dec-var-structR T_RPAREN method_type mBlock
	{$$ = new MethodDeclAST(*$2,(decafStmtList*)$4,*$6,$7); delete $2; delete $6;}
	| { decafStmtList *slist = new decafStmtList(); $$ = slist; }
	;

mBlock: T_LCB var-decl-list statements T_RCB {$$ = new MethodBlockAST((decafStmtList *)$2, (decafStmtList *)$3);}


dec-var-structR: dec-var-struct T_COMMA dec-var-structR
	{
		decafStmtList *slist = (decafStmtList*)$3	;
		slist -> push_front($1);
		$$ = slist;
	}
	| dec-var-struct {decafStmtList *slist =  new decafStmtList(); slist -> push_front($1); $$ = slist;}
	| { decafStmtList *slist = new decafStmtList(); $$ = slist; }
	;

dec-var-struct: T_ID decaf_type { $$ = new VarDefAST(*$1,*$2); delete $1; delete $2;}

field-decR: field-dec field-decR
	{
		decafStmtList *slist = (decafStmtList*)$2;
		slist -> push_front($1);
		$$ = slist;
	}
	| field-dec {decafStmtList *slist =  new decafStmtList(); slist -> push_front($1); $$ = slist;}
	| { decafStmtList *slist = new decafStmtList(); $$ = slist; }
	;

field-dec: T_VAR mul-field-decR T_SEMICOLON
	{
		$$ = $2;
	}
	| T_VAR T_ID decaf_type T_SEMICOLON
	{
		IdAST* temp = new IdAST(*$2);
		IdAST* temp1= new IdAST("Scalar");
		$$ = new FieldDeclAST((decafAST*)temp,*$3, (decafAST*)temp1);
		delete $3;
	}
	| T_VAR mul-arr-decR T_SEMICOLON {$$ = $2;}
	| T_VAR T_ID arrayType decaf_type T_SEMICOLON { IdAST* temp = new IdAST(*$2); $$ = new FieldDeclAST((decafAST*)temp, *$4, (decafAST*)$3); delete $2;}
	| T_VAR T_ID decaf_type T_ASSIGN const T_SEMICOLON
	{
		$$ = new AssignGlobalVarAST(*$2,*$3,$5);
		delete $2;
		delete $3;
	}
	;

mul-field-decR: T_ID T_COMMA mul-field-decR { 
		decafStmtList* sList = (decafStmtList*) $3;
		FieldDeclAST* last = (FieldDeclAST*)(sList -> lastElement());
		string type = last -> returnType(); 
		IdAST* temp = new IdAST(*$1);
		IdAST* temp1= new IdAST("Scalar");
		$$ = new FieldDeclAST((decafAST*)temp,type, (decafAST*)temp1);
		sList -> push_front($$);
		$$ = sList;
	}
	| T_ID decaf_type	{ decafStmtList* sList = new decafStmtList();
		IdAST* temp = new IdAST(*$1);
		IdAST* temp1= new IdAST("Scalar");
		$$ = new FieldDeclAST((decafAST*)temp,*$2, (decafAST*)temp1);
		sList-> push_front($$); 
		$$ = sList; 
	}
	;

mul-arr-decR: T_ID T_COMMA mul-arr-decR { 
		decafStmtList* sList = (decafStmtList*) $3;
		FieldDeclAST* last = (FieldDeclAST*)(sList -> lastElement());
		string type = last -> returnType(); 
		string arrT = last -> returnArr();
		IdAST* temp = new IdAST(*$1);
		IdAST* temp1 = new IdAST(arrT);
		$$ = new FieldDeclAST((decafAST*)temp,type, (decafAST*)temp1);
		sList -> push_front($$);
		$$ = sList;
	}
	| T_ID arrayType decaf_type	{ 
		decafStmtList* sList = new decafStmtList();
		IdAST* temp = new IdAST(*$1); 
		$$ = new FieldDeclAST(temp,*$3, $2); 
		sList-> push_front($$); 
		$$ = sList; }
	;



externR: extern externR
	{
		decafStmtList *slist = (decafStmtList *)$2;
		slist -> push_front($1);
		$$ = slist;
	}
	| extern  	{
		decafStmtList* temp = new decafStmtList();
		temp -> push_front($1);
		$$ = temp;
	}
	;
extern: T_EXTERN T_FUNC T_ID T_LPAREN extern_typeR T_RPAREN method_type T_SEMICOLON
	{
		$$ = new ExternFunctionAST(*$3, *$7, $5);
		delete $3;
		delete $7;
	}
	
      




%%

int main() {
  // initialize LLVM
  llvm::LLVMContext &Context = TheContext;
  // Make the module, which holds all the code.
  TheModule = new llvm::Module("Test", Context);
  llvm::BasicBlock *BB = llvm::BasicBlock::Create(TheContext, "entry", TheFunction);
   Builder.SetInsertPoint(BB);
   // set up symbol table
  symbol_table a;
  symtbl.push_front(a);
  // set up dummy main function
  //TheFunction = gen_main_def();
  // parse the input and create the abstract syntax tree
  int retval = yyparse();
  // remove symbol table

  symtbl.pop_front();
  // Finish off the main function. (see the WARNING above)
  // return 0 from main, which is EXIT_SUCCESS
  Builder.CreateRet(llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0)));
  // Validate the generated code, checking for consistency.
  //verifyFunction(*TheFunction);
  // Print out all of the generated code to stderr
  TheModule->print(llvm::errs(), nullptr);

  
  return(retval >= 1 ? EXIT_FAILURE : EXIT_SUCCESS);
}

