
#ifndef SQL_EXPRESSIONS
#define SQL_EXPRESSIONS

#include "MyDB_AttType.h"
#include <string>
#include <vector>
#include "MyDB_Catalog.h"

#include <cstring>

// create a smart pointer for database tables
using namespace std;
class ExprTree;
typedef shared_ptr <ExprTree> ExprTreePtr;

// this class encapsules a parsed SQL expression (such as "this.that > 34.5 AND 4 = 5")

// class ExprTree is a pure virtual class... the various classes that implement it are below
class ExprTree {

public:
	virtual string toString () = 0;
	virtual string getType() = 0;
	virtual ~ExprTree () {}
	virtual bool semanticChecking(MyDB_CatalogPtr catalog, vector<pair<string, string>> tablesToProcess) = 0;
};

class BoolLiteral : public ExprTree {

private:
	bool myVal;
public:
	
	BoolLiteral (bool fromMe) {
		myVal = fromMe;
	}

	string toString () {
		if (myVal) {
			return "bool[true]";
		} else {
			return "bool[false]";
		}
	}	
	string getType() {
		return "BOOL";
	}
	bool semanticChecking(MyDB_CatalogPtr catalog, vector<pair<string, string>> tablesToProcess) {
		return true;
	}
};

class DoubleLiteral : public ExprTree {

private:
	double myVal;
public:

	DoubleLiteral (double fromMe) {
		myVal = fromMe;
	}

	string toString () {
		return "double[" + to_string (myVal) + "]";
	}	
	string getType() {
		return "NUMERIC";
	}

	bool semanticChecking(MyDB_CatalogPtr catalog, vector<pair<string, string>> tablesToProcess) {
		return true;
	}

	~DoubleLiteral () {}
};

// this implement class ExprTree
class IntLiteral : public ExprTree {

private:
	int myVal;
public:

	IntLiteral (int fromMe) {
		myVal = fromMe;
	}

	string toString () {
		return "int[" + to_string (myVal) + "]";
	}

	string getType() {
		return "NUMERIC";
	}

	bool semanticChecking(MyDB_CatalogPtr catalog, vector<pair<string, string>> tablesToProcess) {
		return true;
	}

	~IntLiteral () {}
};

class StringLiteral : public ExprTree {

private:
	string myVal;
public:

	StringLiteral (char *fromMe) {
		fromMe[strlen (fromMe) - 1] = 0;
		myVal = string (fromMe + 1);
	}

	string toString () {
		return "string[" + myVal + "]";
	}

	string getType() {
		return "STRING";
	}

	bool semanticChecking(MyDB_CatalogPtr catalog, vector<pair<string, string>> tablesToProcess) {
		return true;
	}

	~StringLiteral () {}
};

// SELECT {employees.name, empoyees.salary} FROM employees WHERE employees.salary > 1000;
class Identifier : public ExprTree {

private:
	string tableName;
	string attName;
	string attType;
public:

	Identifier (char *tableNameIn, char *attNameIn) {
		tableName = string (tableNameIn);
		attName = string (attNameIn);
		attType = "IDENTIFIER";
	}

	// table name = employees;
	// att name = name/salary;
	string toString () {
		return "[" + tableName + "_" + attName + "]";
	}	

	string getType() {
		return attType;
	}

	bool semanticChecking(MyDB_CatalogPtr catalog, vector<pair<string, string>> tablesToProcess) {
		bool found = false;
		string tbl2process_1 = "";
		for(auto table : tablesToProcess) {
			if(table.second == tableName) {
				tbl2process_1 = table.first;
				found = true;
				break;
			}
		}
		if(!found) {
			cout << "Table " << tableName << " is not in the from clause.\n";
			return false;
		}
		string attributeType;
		bool attributesExist = catalog->getString(tbl2process_1+"."+attName+".type", attributeType);

		// cout<<"attributeType of "<<attName<<" is "<<attributeType<<endl;

		if(!attributesExist) {
			cout << "Attribute " << attName << " does not exist in table " << tableName << ".\n";
			return false;
		}

		if(attributeType == "int" || attributeType == "double") {
			attType = "NUMERIC";
		} else if(attributeType == "string") {
			attType = "STRING";
		} else if(attributeType == "bool") {
			attType = "BOOL";
		}
		
		return true;
	}

	~Identifier () {}
};

class MinusOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	MinusOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "- (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	string getType() {
		return "NUMERIC";
	}

	bool semanticChecking(MyDB_CatalogPtr catalog, vector<pair<string, string>> tablesToProcess) {
		if(!lhs->semanticChecking(catalog, tablesToProcess) || !rhs->semanticChecking(catalog, tablesToProcess)) {
			return false;
		}
		if(lhs->getType() != "NUMERIC" || rhs->getType() != "NUMERIC") {
			cout<<"rhs type is "<<rhs->getType()<<endl;
			cout << "Operands of - operator must be numeric.\n";
			return false;
		}
		if (lhs->getType() != rhs->getType()) {
			cout << "Operands of - operator must have the same type. lhs's type: "<<lhs->getType()<<" rhs's type: "<<rhs->getType()<<endl;
			return false;
		}
		return true;
	}

	~MinusOp () {}
};

class PlusOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	string attType;
	
public:

	PlusOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
		string attType = "NUMERIC";
	}

	string toString () {
		return "+ (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	string getType() {
		return attType;
	}

	bool semanticChecking(MyDB_CatalogPtr catalog, vector<pair<string, string>> tablesToProcess) {
		if(!lhs->semanticChecking(catalog, tablesToProcess) || !rhs->semanticChecking(catalog, tablesToProcess)) {
			return false;
		}
		if(lhs->getType() != rhs->getType()) {
			cout << "Operands of + operator must have the same type. lhs's type: "<<lhs->getType()<<" rhs's type: "<<rhs->getType()<<endl;
			return false;
		}
		if(lhs->getType() == "STRING") {
			attType = "STRING";
			return true;
		} else if(lhs->getType() == "NUMERIC") {
			attType = "NUMERIC";
			return true;
		} else {
			cout << "Operands of + operator must be numeric or string.\n";
			return false;
		}
		return true;
	}


	~PlusOp () {}
};

class TimesOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	TimesOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "* (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	string getType() {
		return "NUMERIC";
	}

	bool semanticChecking(MyDB_CatalogPtr catalog, vector<pair<string, string>> tablesToProcess) {
		if(!lhs->semanticChecking(catalog, tablesToProcess) || !rhs->semanticChecking(catalog, tablesToProcess)) {
			return false;
		}
		if(lhs->getType() != "NUMERIC" || rhs->getType() != "NUMERIC") {
			cout << "Operands of * operator must be numeric. lhs's type: "<<lhs->getType()<<" rhs's type: "<<rhs->getType()<<endl;
			return false;
		}
		if(lhs->getType() != rhs->getType()) {
			cout << "Operands of * operator must have the same type. lhs's type: "<<lhs->getType()<<" rhs's type: "<<rhs->getType()<<endl;
			return false;
		}
		return true;
	}

	~TimesOp () {}
};

class DivideOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	DivideOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "/ (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	string getType() {
		return "NUMERIC";
	}

	bool semanticChecking(MyDB_CatalogPtr catalog, vector<pair<string, string>> tablesToProcess) {
		if(!lhs->semanticChecking(catalog, tablesToProcess) || !rhs->semanticChecking(catalog, tablesToProcess)) {
			return false;
		}

		if(rhs->toString() == "0") {
			cout << "Division by zero.\n";
			return false;
		}

		if(lhs->getType() != "NUMERIC" || rhs->getType() != "NUMERIC") {
			cout << "Operands of / operator must be numeric. lhs's type: "<<lhs->getType()<<" rhs's type: "<<rhs->getType()<<endl;
			return false;
		}
		if(lhs->getType() != rhs->getType()) {
			cout << "Operands of / operator must have the same type. lhs's type: "<<lhs->getType()<<" rhs's type: "<<rhs->getType()<<endl;
			return false;
		}
		return true;
	}

	~DivideOp () {}
};

class GtOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	GtOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "> (" + lhs->toString () + ", " + rhs->toString () + ")";
	}

	string getType() {
		return "BOOL";
	}	

	bool semanticChecking(MyDB_CatalogPtr catalog, vector<pair<string, string>> tablesToProcess) {
		if(!lhs->semanticChecking(catalog, tablesToProcess) || !rhs->semanticChecking(catalog, tablesToProcess)) {
			return false;
		}
		
		if(lhs->getType() != rhs->getType()) {
			cout << "Operands of > operator must have the same type. lhs's type: "<<lhs->getType()<<" rhs's type: "<<rhs->getType()<<endl;
			return false;
		}

		// if(lhs->getType() != "NUMERIC" || rhs->getType() != "NUMERIC") {
		// 	cout << "Operands of > operator must be numeric.
		// 	return false;
		// }
		
		return true;
	}

	~GtOp () {}
};

class LtOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	LtOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "< (" + lhs->toString () + ", " + rhs->toString () + ")";
	}

	string getType() {
		return "BOOL";
	}

	bool semanticChecking(MyDB_CatalogPtr catalog, vector<pair<string, string>> tablesToProcess) {
		if(!lhs->semanticChecking(catalog, tablesToProcess) || !rhs->semanticChecking(catalog, tablesToProcess)) {
			return false;
		}
		
		if(lhs->getType() != rhs->getType()) {
			cout << "Operands of < operator must have the same type. lhs's type: "<<lhs->getType()<<" rhs's type: "<<rhs->getType()<<endl;
			return false;
		}

		// if(lhs->getType() != "NUMERIC" || rhs->getType() != "NUMERIC") {
		// 	cout << "Operands of < operator must be numeric.\n";
		// 	return false;
		// }
		
		return true;
	}


	~LtOp () {}
};

class NeqOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	NeqOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "!= (" + lhs->toString () + ", " + rhs->toString () + ")";
	}

	string getType() {
		return "BOOL";
	}	

	bool semanticChecking(MyDB_CatalogPtr catalog, vector<pair<string, string>> tablesToProcess) {
		if(!lhs->semanticChecking(catalog, tablesToProcess) || !rhs->semanticChecking(catalog, tablesToProcess)) {
			return false;
		}
		
		if(lhs->getType() != rhs->getType()) {
			cout << "Operands of != operator must have the same type. lhs's type: "<<lhs->getType()<<" rhs's type: "<<rhs->getType()<<endl;
			return false;
		}

		// if(lhs->getType() != "NUMERIC" || rhs->getType() != "NUMERIC") {
		// 	cout << "Operands of != operator must be numeric.\n";
		// 	return false;
		// }
		
		return true;
	}

	~NeqOp () {}
};

class OrOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	OrOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "|| (" + lhs->toString () + ", " + rhs->toString () + ")";
	}

	string getType() {
		return "BOOL";
	}

	bool semanticChecking(MyDB_CatalogPtr catalog, vector<pair<string, string>> tablesToProcess) {
		if(!lhs->semanticChecking(catalog, tablesToProcess) || !rhs->semanticChecking(catalog, tablesToProcess)) {
			return false;
		}

		if(lhs->getType() != "BOOL" || rhs->getType() != "BOOL") {
			cout << "Operands of || operator must be boolean. lhs's type: "<<lhs->getType()<<" rhs's type: "<<rhs->getType()<<endl;
			return false;
		}

		if(lhs->getType() != rhs->getType()) {
			cout << "Operands of || operator must have the same type. lhs's type: "<<lhs->getType()<<" rhs's type: "<<rhs->getType()<<endl;
			return false;
		}
		
		return true;
	}	

	~OrOp () {}
};

class EqOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	EqOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "== (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	string getType() {
		return "BOOL";
	}

	bool semanticChecking(MyDB_CatalogPtr catalog, vector<pair<string, string>> tablesToProcess) {
		if(!lhs->semanticChecking(catalog, tablesToProcess) || !rhs->semanticChecking(catalog, tablesToProcess)) {
			return false;
		}
		
		if(lhs->getType() != rhs->getType()) {
			cout << "Operands of == operator must have the same type. lhs's type: "<<lhs->getType()<<" rhs's type: "<<rhs->getType()<<endl;
			return false;
		}

		// if(lhs->getType() != "NUMERIC" && lhs->getType() != "STRING" && lhs->getType() != "BOOL") {
		// 	cout << "Operands of == operator must be numeric, string or boolean.\n";
		// 	return false;
		// }
		
		return true;
	}

	~EqOp () {}
};

class NotOp : public ExprTree {

private:

	ExprTreePtr child;
	
public:

	NotOp (ExprTreePtr childIn) {
		child = childIn;
	}

	string toString () {
		return "!(" + child->toString () + ")";
	}

	string getType() {
		return "BOOL";
	}

	bool semanticChecking(MyDB_CatalogPtr catalog, vector<pair<string, string>> tablesToProcess) {
		if(!child->semanticChecking(catalog, tablesToProcess)) {
			return false;
		}
		if(child->getType() != "BOOL") {
			cout << "Operand of ! operator must be boolean. Operand's type: "<<child->getType()<<endl;
			return false;
		}
		return true;
	}	

	~NotOp () {}
};

class SumOp : public ExprTree {

private:

	ExprTreePtr child;
	
public:

	SumOp (ExprTreePtr childIn) {
		child = childIn;
	}

	string toString () {
		return "sum(" + child->toString () + ")";
	}

	string getType() {
		return "NUMERIC";
	}	

	bool semanticChecking(MyDB_CatalogPtr catalog, vector<pair<string, string>> tablesToProcess) {
		if(!child->semanticChecking(catalog, tablesToProcess)) {
			return false;
		}
		if(child->getType() != "NUMERIC") {
			cout << "Operand of sum operator must be numeric. Operand's type: "<<child->getType()<<endl;
			return false;
		}
		return true;
	}

	~SumOp () {}
};

class AvgOp : public ExprTree {

private:

	ExprTreePtr child;
	
public:

	AvgOp (ExprTreePtr childIn) {
		child = childIn;
	}

	string toString () {
		return "avg(" + child->toString () + ")";
	}

	string getType() {
		return "NUMERIC";
	}

	bool semanticChecking(MyDB_CatalogPtr catalog, vector<pair<string, string>> tablesToProcess) {
		if(!child->semanticChecking(catalog, tablesToProcess)) {
			return false;
		}
		if(child->getType() != "NUMERIC") {
			cout << "Operand of avg operator must be numeric. Operand's type: "<<child->getType()<<endl;
			return false;
		}
		return true;
	}	

	~AvgOp () {}
};

#endif
