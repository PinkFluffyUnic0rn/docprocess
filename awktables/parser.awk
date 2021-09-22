# Global:
#	Fp_tokval
#	Fp_nexttoktype
#	Fp_nexttokval
#	Fp_strsym []
#	Fp_l
#
#	FP_T_***
#	FP_N_***

# lexical analizer
function fp_lgetchar(	c)
{
	c = substr(Fp_l, 1, 1);
	Fp_l = substr(Fp_l, 2);

	return c;
}

# for internal use
function fp_nexttoken(	toktype, c)
{	
	gsub(/^[ \t]*/, "", Fp_l);	
	
	if (Fp_l == "")
		return FP_T_EOF;	
	
	if (match(Fp_l, /^\"/)) {
		Fp_tokval = "";

		fp_lgetchar();
		while ((c = fp_lgetchar()) != "\"") {
			assert(c != "", "Not terminated quoted string: " Fp_l);

			if (c == "\\") {
				c = fp_lgetchar();
				if (c == "") return FP_T_EOF;
				else if (c == "n") c = "\n";
				else if (c == "r") c = "\r";
				else if (c == "t") c = "\t";
			}

			Fp_tokval = Fp_tokval c;
		}

		return FP_T_STRING;
	}

	else if (match(Fp_l, /^[a-zA-Z_]+[a-zA-Z_0-9]*/))
		toktype = FP_T_ID;
	else if (match(Fp_l, /^[0-9]*\.[0-9]+([eE][-+]?[0-9]+)?/))
		toktype = FP_T_FLOAT;
	else if (match(Fp_l, /^[0-9]+/))
		toktype = FP_T_INT;
	else if (match(Fp_l, /^=|^!=|^>=|^<=|^>|^</))
		toktype = FP_T_RELOP;
	else if (match(Fp_l, /^\(/))		toktype = FP_T_LPAR;
	else if (match(Fp_l, /^\)/))		toktype = FP_T_RPAR;
	else if (match(Fp_l, /^\&/))		toktype = FP_T_AND;
	else if (match(Fp_l, /^\|/))		toktype = FP_T_OR;
	else if (match(Fp_l, /^\!/))		toktype = FP_T_NOT;
	else if (match(Fp_l, /^\$/))		toktype = FP_T_DOL;
	else if (match(Fp_l, /^\~/))		toktype = FP_T_TILDA;
	else if (match(Fp_l, /^\,/))		toktype = FP_T_COMMA;
	else if (match(Fp_l, /^\:/))		toktype = FP_T_COLON;
	else if (match(Fp_l, /^\;/))		toktype = FP_T_SEMICOL;
	else if (match(Fp_l, /^\.\./))		toktype = FP_T_DDOT;
	else if (match(Fp_l, /^\*|^\//))	toktype = FP_T_MULTOP;
	else if (match(Fp_l, /^\+|^\-/))	toktype = FP_T_ADDOP;
	else
		error("Unknown token: " Fp_l);

	Fp_tokval = substr(Fp_l, 0, RLENGTH);
	Fp_l = substr(Fp_l, RLENGTH + 1);
	
	return toktype;
}

function fp_gettoken(	curtype, curval, curline)
{
	if (Fp_nexttoktype == "") {
		Fp_nexttoktype = fp_nexttoken();
		Fp_nexttokval = Fp_tokval;
	}

	curtype = Fp_nexttoktype;
	curval = Fp_nexttokval;

	Fp_nexttoktype = fp_nexttoken();
	Fp_nexttokval = Fp_tokval;

	Fp_tokval = curval;
	return curtype;
}

function fp_peektoken()
{
	if (Fp_nexttoktype == "") {
		Fp_nexttoktype = fp_nexttoken();
		Fp_nexttokval = Fp_tokval;
	}

	Fp_tokval = Fp_nexttokval;
	return Fp_nexttoktype;
}

function fp_gettokentype(type,	toktype)
{
	assert((toktype = fp_gettoken()) == type,
		sprintf("Wrong token type: expected %s, but got %s.",
			Fp_strsym[type], Fp_strsym[toktype]));
}

# syntax analizer
function fp_args(n,	t)
{
	fp_expr(node_add(n, FP_N_EXPR, "", ""));
	while (fp_peektoken() == FP_T_COMMA) {
		fp_gettokentype(FP_T_COMMA);
		fp_expr(node_add(n, FP_N_EXPR, "", ""));
	}
}

function fp_id(n,	t)
{
	fp_gettokentype(FP_T_ID);
	node_add(n, FP_T_ID, Fp_tokval, "");
	
	if (fp_peektoken() == FP_T_LPAR) {
		fp_gettokentype(FP_T_LPAR);
		fp_args(node_add(n, FP_N_ARGS, "", ""));
		fp_gettokentype(FP_T_RPAR);
	}
}

function fp_const(n,	t)
{
	t = fp_gettoken(t);

	assert(t == FP_T_STRING || t == FP_T_FLOAT || t == FP_T_INT,
		"Expected constant, but got " Fp_strsym[t] ".");

	node_add(n, t, Fp_tokval, "");
}

function fp_lineid(n,	t)
{
	t = fp_gettoken();
	
	assert(t == FP_T_ID || t == FP_T_STRING || t == FP_T_INT,
		"Expected line id, but got " Fp_strsym[t] ".");

	node_add(n, t, Fp_tokval, "");
}

function fp_ref(n)
{
	fp_gettokentype(FP_T_DOL);
	fp_lineid(n);
}

function fp_val(n,	t)
{
	t = fp_peektoken();
	if (t == FP_T_ID)
		fp_id(node_add(n, FP_N_ID, "", ""));
	else if (t == FP_T_DOL)
		fp_ref(node_add(n, FP_N_REF, "", ""));
	else if (t == FP_T_LPAR) {
		fp_gettoken();
		fp_expr(node_add(n, FP_N_EXPR, "", ""));
		fp_gettokentype(FP_T_RPAR);
	}
	else if (t == FP_T_FLOAT || t == FP_T_INT || t == FP_T_STRING)
		fp_const(n);
	else
		error("Expected value, but got " Fp_strsym[t] ".");
}

function fp_sign(n,	t)
{
	fp_gettoken(FP_T_ADDOP);
	node_add(n, FP_T_ADDOP, Fp_tokval, "");
	fp_val(n);
}

function fp_unary(n,	t)
{
	t = fp_peektoken();
	if (t == FP_T_NOT) {
		fp_gettoken(FP_T_NOT);
		fp_val(node_add(n, FP_N_NOT, "", ""));
	}
	else if (t == FP_T_ADDOP)
		fp_sign(node_add(n, FP_N_SIGN, "", ""));
	else
		fp_val(n);
}

function fp_mult(n,	t)
{
	fp_unary(n);
	while (fp_peektoken() == FP_T_MULTOP) {
		fp_gettokentype(FP_T_MULTOP);
		node_add(n, FP_T_MULTOP, Fp_tokval, "");
		fp_unary(n);
	}
}

function fp_add(n,	t)
{
	fp_mult(node_add(n, FP_N_MULT, "", ""));
	while (fp_peektoken() == FP_T_ADDOP) {
		fp_gettokentype(FP_T_ADDOP);
		node_add(n, FP_T_ADDOP, Fp_tokval, "");
		fp_mult(node_add(n, FP_N_MULT, "", ""));
	}
}

function fp_rel(n)
{
	fp_add(node_add(n, FP_N_ADD, "", ""));
	if (fp_peektoken() == FP_T_RELOP) {
		fp_gettokentype(FP_T_RELOP);
		node_add(n, FP_T_RELOP, Fp_tokval, "");
		fp_add(node_add(n, FP_N_ADD, "", ""));
	}
}

function fp_and(n)
{
	fp_rel(node_add(n, FP_N_REL, "", ""));
	while (fp_peektoken() == FP_T_AND) {
		fp_gettokentype(FP_T_AND);
		fp_rel(node_add(n, FP_N_REL, "", ""));
	}
}

function fp_expr(n)
{
	fp_and(node_add(n, FP_N_AND, "", ""));
	while (fp_peektoken() == FP_T_OR) {
		fp_gettokentype(FP_T_OR);
		fp_and(node_add(n, FP_N_AND, "", ""));
	}	
}

function fp_range(n)
{
	fp_lineid(n);
	fp_gettokentype(FP_T_DDOT);
	fp_lineid(n);
}

function fp_skipquoted(l, pos, spos)
{
	spos = pos;
	if (substr(l, pos, 1) == "\"")
		while (substr(l, pos, 1) != "\"") {
			if (l[pos] == "\\") ++pos;
			if (pos == length(pos)) {
				error("Not terminated quoted string: " \
					substr(l, spos));
			}
			++pos;
		}

	return pos;
}

function fp_filter(l, 	n, e, r, spos, pos)
{
	n = node_add(-1, FP_N_FILTER, "", "");

	e = r = 0;
	spos = pos = 1;
	do {
		pos = fp_skipquoted(l, pos);

		if (substr(l, pos, 1) == "$") e = 1;
		else if (substr(l, pos, 2) == "..") r = 1;
		else if (substr(l, pos + 1, 1) == ";" || pos == length(l)) {
			Fp_l = substr(l, spos, pos - spos + 1);
			Fp_nexttoktype = "";
			
			if (e) fp_expr(node_add(n, FP_N_EXPR, "", ""));
			else if (r) fp_range(node_add(n, FP_N_RANGE, "", ""));
			else fp_lineid(node_add(n, FP_N_LINEID, "", ""));
			
			fp_gettokentype(FP_T_EOF);
			
			e = r = 0;
			spos = pos + 2;
		}
	} while (++pos <= length(l));

	return n;
}

function fp_def(n)
{
	fp_gettokentype(FP_T_ID);
	node_add(n, FP_T_ID, Fp_tokval, "");
	
	fp_gettokentype(FP_T_RELOP);
	assert(Fp_tokval == "=",
		"Expected '=', but got '" Fp_tokval "'.");
	fp_const(n);
}

function fp_deflist(l,	n)
{
	n = node_add(-1, FP_N_DEFLIST, "", "");
	Fp_l = l;
	Fp_nexttoktype = "";

	if (fp_peektoken() == FP_T_EOF)
		return n;
	
	fp_def(node_add(n, FP_N_DEF, "", ""));
	while (fp_peektoken() == FP_T_SEMICOL) {
		fp_gettokentype(FP_T_SEMICOL);
		fp_def(node_add(n, FP_N_DEF, "", ""));
	}
	
	fp_gettokentype(FP_T_EOF);

	return n;
}

BEGIN {
	FP_T_ID = 1;		FP_T_INT = 2;		FP_T_FLOAT = 3;
	FP_T_STRING = 4;	FP_T_RELOP = 5;		FP_T_LPAR = 6;
	FP_T_RPAR = 7;		FP_T_AND = 8;		FP_T_OR = 9;
	FP_T_TILDA = 10;	FP_T_COMMA = 11;	FP_T_NOT = 12;
	FP_T_SEMICOL = 13;	FP_T_COLON = 14;	FP_T_DOL = 15;	
	FP_T_ADDOP = 16;	FP_T_MULTOP = 17;	FP_T_DDOT = 18;
	FP_T_EOF = 19;

	FP_N_FILTER = 21;	FP_N_EXPR = 22;		FP_N_AND = 23;
	FP_N_OR = 24;		FP_N_REL = 25;		FP_N_ADD = 26;
	FP_N_MULT = 27;		FP_N_UNARY = 28;	FP_N_VAL = 29;
	FP_N_ID = 30;		FP_N_ARGS = 31;		FP_N_NUM = 32;
	FP_N_CONST = 33;	FP_N_QUERY = 34;	FP_N_REF = 35;
	FP_N_NOT = 36;		FP_N_ARGS = 37;		FP_N_SIGN = 38;
	FP_N_LINEID = 39;	FP_N_RANGE = 40;	FP_N_DEF = 41;
	FP_N_DEFLIST = 42;
	
	Fp_strsym[FP_T_ID] = "identificator";
	Fp_strsym[FP_T_INT] = "integer";
	Fp_strsym[FP_T_FLOAT] = "float";
	Fp_strsym[FP_T_RELOP] = "relational operator";
	Fp_strsym[FP_T_LPAR] = "(";
	Fp_strsym[FP_T_RPAR] = ")";
	Fp_strsym[FP_T_AND] = "&";
	Fp_strsym[FP_T_OR] = "|";
	Fp_strsym[FP_T_NOT] = "!";
	Fp_strsym[FP_T_DOL] = "$";
	Fp_strsym[FP_T_TILDA] = "~";
	Fp_strsym[FP_T_COMMA] = ",";
	Fp_strsym[FP_T_COLON] = ":";
	Fp_strsym[FP_T_SEMICOL] = ";";
	Fp_strsym[FP_T_DDOT] = "..";
	Fp_strsym[FP_T_MULTOP] = "multiplication or division";
	Fp_strsym[FP_T_ADDOP] = "addition or substraction";
	Fp_strsym[FP_T_STRING] = "quoted string";
	Fp_strsym[FP_T_EOF] = "EOF";

	Fp_strsym[FP_N_FILTER] = "filter";
	Fp_strsym[FP_N_EXPR] = "expression";
	Fp_strsym[FP_N_AND] = "logical and";
	Fp_strsym[FP_N_OR] = "logical or";
	Fp_strsym[FP_N_REL] = "relation";
	Fp_strsym[FP_N_ADD] = "addtion or substraction";
	Fp_strsym[FP_N_MULT] = "multiplication or division";
	Fp_strsym[FP_N_UNARY] = "unary operator";
	Fp_strsym[FP_N_VAL] = "value";
	Fp_strsym[FP_N_ID] = "id usage";
	Fp_strsym[FP_N_ARGS] = "function arguments";
	Fp_strsym[FP_N_CONST] = "constant";
	Fp_strsym[FP_N_NUM] = "number";
	Fp_strsym[FP_N_QUERY] = "query";
	Fp_strsym[FP_N_REF] = "line reference";
	Fp_strsym[FP_N_NOT] = "not";
	Fp_strsym[FP_N_ARGS] = "function arguments";
	Fp_strsym[FP_N_SIGN] = "sign";
	Fp_strsym[FP_N_LINEID] = "line id";
	Fp_strsym[FP_N_RANGE] = "range";
	Fp_strsym[FP_N_DEFLIST] = "definition list";
	Fp_strsym[FP_N_DEF] = "definition";
}
