# Global:
#	Tp_line
#	Tp_lnum
#	Tp_tokval
#	Tp_tokline
#	Tp_nexttoktype
#	Tp_nexttokval
#	Tp_strsym []
#	Tp_l
#	Tp_file
#
#	TP_T_***
#	TP_N_***

# lexical analizer
function tp_lgetchar(	c)
{
	c = substr(Tp_l, 1, 1);
	Tp_l = substr(Tp_l, 2);

	return c;
}

# only for internal use by gettoken and peektoken
function tp_nexttoken(	toktype, c, t)
{	
	gsub(/^[ \t]*/, "", Tp_l);
	
	while (Tp_l == "") {
		if ((getline Tp_l <Tp_file) <= 0)
			return TP_T_EOF;
		
		gsub(/^[ \t]*/, "", Tp_l);
	
		if (substr(Tp_l, 1, 1) == "#")
			Tp_l = "";
	
		Tp_line[++Tp_lnum] = Tp_l;
	}

	if (match(Tp_l, /^\"/)) {
		Tp_tokval = "";

		tp_lgetchar();
		while ((c = tp_lgetchar()) != "\"") {
			assert(c != "", sprintf(\
				"Not terminated quoted string at line:\n%d: %s\n",
				NR, Tp_line[NR]));

			if (c == "\\") {
				c = tp_lgetchar();
				if (c == "") {
					if ((getline Tp_l <Tp_file) <= 0)
						return TP_T_EOF;

					Tp_line[++Tp_lnum] = Tp_l;
				}
				else if (c == "n") c = "\n";
				else if (c == "r") c = "\r";
				else if (c == "t") c = "\t";
			}

			Tp_tokval = Tp_tokval c;
		}

		return TP_T_STRING;
	}

	else if (match(Tp_l, /^[a-zA-Z_]+[a-zA-Z_0-9]*/)) {
		t = substr(Tp_l, 0, RLENGTH);

		if (t == "global")		toktype = TP_T_GLOBAL;
		else if (t == "function")	toktype = TP_T_FUNCTION;
		else if (t == "for")		toktype = TP_T_FOR;
		else if (t == "if")		toktype = TP_T_IF;
		else if (t == "else")		toktype = TP_T_ELSE;
		else if (t == "continue")	toktype = TP_T_CONTINUE;
		else if (t == "break")		toktype = TP_T_BREAK;
		else if (t == "return")		toktype = TP_T_RETURN;
		else if (t == "in")		toktype = TP_T_IN;
		else if (t == "columns")	toktype = TP_T_FORDIR;
		else				toktype = TP_T_ID;
	}
	else if (match(Tp_l, /^[0-9]*\.[0-9]+([eE][-+]?[0-9]+)?/))
		toktype = TP_T_FLOAT;
	else if (match(Tp_l, /^[0-9]+/))
		toktype = TP_T_INT;
	else if (match(Tp_l, /^\^|^<\-/))
		toktype = TP_T_NEXTTOOP;
	else if (match(Tp_l, /^==|^!=|^>=|^<=|^>|^</))
		toktype = TP_T_RELOP;
	else if (match(Tp_l, /^\(/))		toktype = TP_T_LPAR;
	else if (match(Tp_l, /^\)/))		toktype = TP_T_RPAR;
	else if (match(Tp_l, /^\&/))		toktype = TP_T_AND;
	else if (match(Tp_l, /^\|/))		toktype = TP_T_OR;
	else if (match(Tp_l, /^\!/))		toktype = TP_T_NOT;
	else if (match(Tp_l, /^\$/))		toktype = TP_T_DOL;
	else if (match(Tp_l, /^\~/))		toktype = TP_T_TILDA;
	else if (match(Tp_l, /^\.\./))		toktype = TP_T_DDOT;
	else if (match(Tp_l, /^\./))		toktype = TP_T_POINT;
	else if (match(Tp_l, /^\,/))		toktype = TP_T_COMMA;
	else if (match(Tp_l, /^\:/))		toktype = TP_T_COLON;
	else if (match(Tp_l, /^\;/))		toktype = TP_T_SEMICOL;
	else if (match(Tp_l, /^\*|^\//))	toktype = TP_T_MULTOP;
	else if (match(Tp_l, /^\+|^\-/))	toktype = TP_T_ADDOP;
	else if (match(Tp_l, /^\{/))		toktype = TP_T_LBRC;
	else if (match(Tp_l, /^\}/))		toktype = TP_T_RBRC;
	else if (match(Tp_l, /^\[/))		toktype = TP_T_LBRK;
	else if (match(Tp_l, /^\]/))		toktype = TP_T_RBRK;
	else if (match(Tp_l, /^\=/))		toktype = TP_T_ASSIGN;
	else if (match(Tp_l, /^\?/))		toktype = TP_T_QUEST;
	else {
		error(sprintf("Unknown token at line:\n%d: %s\n", NR,
			Tp_line[Tp_lnum]));
	}

	Tp_tokval = substr(Tp_l, 0, RLENGTH);

	Tp_tokline = Tp_lnum;
	Tp_l = substr(Tp_l, RLENGTH + 1);

	return toktype;
}

function tp_gettoken(	curtype, curval, curline)
{
	if (Tp_nexttoktype == "") {
		Tp_nexttoktype = tp_nexttoken();
		Tp_nexttokval = Tp_tokval;
		Tp_nexttokline = Tp_tokline;

	}

	curtype = Tp_nexttoktype;
	curval = Tp_nexttokval;
	curline = Tp_nexttokline;

	Tp_nexttoktype = tp_nexttoken();
	Tp_nexttokval = Tp_tokval;
	Tp_nexttokline = Tp_tokline;

	Tp_tokval = curval;
	Tp_tokline = curline;
	return curtype;
}

function tp_peektoken()
{
	if (Tp_nexttoktype == "") {
		Tp_nexttoktype = tp_nexttoken();
		Tp_nexttokval = Tp_tokval;
	}

	Tp_tokval = Tp_nexttokval;
	Tp_tokline = Tp_nexttokline;
	return Tp_nexttoktype;
}

function tp_gettokentype(type,	toktype)
{
	assert((toktype = tp_gettoken()) == type,
		sprintf("Wrong token type at line:\n%d: %s\n"\
			"expected %s, but got %s.\n",
			Tp_tokline, Tp_line[Tp_tokline],
			Tp_strsym[type], Tp_strsym[toktype]));
}

# syntax analizer
function tp_args(n)
{
	# are there can be no argument, check filter parser too!!!
	tp_gettokentype(TP_T_LPAR);

	if (tp_peektoken() == TP_T_RPAR) {
		tp_gettokentype(TP_T_RPAR);
		return;
	}

	tp_assign(node_add(n, TP_N_EXPR, "", ""));
	while (tp_peektoken() == TP_T_COMMA) {
		tp_gettokentype(TP_T_COMMA);
		tp_assign(node_add(n, TP_N_EXPR, "", ""));
	}

	tp_gettokentype(TP_T_RPAR);
}

function tp_filter(n,	f, t, ids)
{
	while (tp_peektoken() != TP_T_RBRK) {
		t = tp_gettoken();

		if (t == TP_T_STRING) {
			gsub("\n", "\\n", Tp_tokval);
			gsub("\t", "\\t", Tp_tokval);
			gsub("\r", "\\r", Tp_tokval);
			gsub("\"", "\\\"", Tp_tokval);
			Tp_tokval = "\"" Tp_tokval "\"";
		}

		if (t == TP_T_ID)
			ids = ids ";" Tp_tokval;

		f = f Tp_tokval;
	}

	Node[n,"val"] = f;

	return ids;
}

function tp_index(n,	t, f, ids)
{
	tp_gettokentype(TP_T_LBRK);
	ids = tp_filter(node_add(n, TP_N_FILTER, "", Tp_tokline));
	tp_gettokentype(TP_T_RBRK);

	tp_gettokentype(TP_T_LBRK);
	ids = ids tp_filter(node_add(n, TP_N_FILTER, "", ""));
	tp_gettokentype(TP_T_RBRK);

	node_add(n, TP_T_FDEFS, substr(ids, 2), "");
}

function tp_attr(n)
{
	tp_gettokentype(TP_T_POINT);
	tp_gettokentype(TP_T_ID);

	node_add(n, TP_T_ID, Tp_tokval, Tp_tokline);
}

function tp_address(n,	t)
{
	tp_gettokentype(TP_T_ID);
	node_add(n, TP_T_ID, Tp_tokval, Tp_tokline);

	if (tp_peektoken() == TP_T_LPAR)
		tp_args(node_add(n, TP_N_ARGS, "", ""));

	while (tp_peektoken() == TP_T_LBRK)
		tp_index(node_add(n, TP_N_INDEX, "", ""));

	if (tp_peektoken() == TP_T_POINT)
		tp_attr(node_add(n, TP_N_ATTR, "", ""));
}

function tp_val(n,	t)
{
	t = tp_peektoken();
	if (t == TP_T_ID)
		tp_address(node_add(n, TP_N_ADDRESS, "", ""));
	else if (t == TP_T_LPAR) {
		tp_gettoken();
		tp_expr(node_add(n, TP_N_EXPR, "", ""));
		tp_gettokentype(TP_T_RPAR);
	}
	else if (t == TP_T_FLOAT \
			|| t == TP_T_INT || t == TP_T_STRING) {
		t = tp_gettoken(t);
		node_add(n, t, Tp_tokval, Tp_tokline);
	}
	else {
		error(sprintf("Expected value, but got %s:\n\t%s",
			Tp_strsym[t], Tp_line[Tp_tokline]));
	}
}

function tp_sign(n,	t)
{
	tp_gettoken(TP_T_ADDOP);
	node_add(n, TP_T_ADDOP, Tp_tokval, Tp_tokline);
	tp_val(n);
}

function tp_unary(n,	t)
{
	t = tp_peektoken();
	if (t == TP_T_NOT) {
		tp_gettoken(TP_T_NOT);
		tp_val(node_add(n, TP_N_NOT, "", ""));
	}
	else if (t == TP_T_ADDOP)
		tp_sign(node_add(n, TP_N_SIGN, "", ""));
	else
		tp_val(n);
}

function tp_mult(n,	t)
{
	tp_unary(n);
	while (tp_peektoken() == TP_T_MULTOP) {
		tp_gettokentype(TP_T_MULTOP);
		node_add(n, TP_T_MULTOP, Tp_tokval, Tp_tokline);
		tp_unary(n);
	}
}

function tp_add(n,	t)
{
	tp_mult(node_add(n, TP_N_MULT, "", ""));
	while (tp_peektoken() == TP_T_ADDOP) {
		tp_gettokentype(TP_T_ADDOP);
		node_add(n, TP_T_ADDOP, Tp_tokval, Tp_tokline);
		tp_mult(node_add(n, TP_N_MULT, "", ""));
	}
}

function tp_cat(n, 	t)
{
	tp_add(node_add(n, TP_N_ADD, "", ""));
	while (tp_peektoken() == TP_T_TILDA) {
		tp_gettokentype(TP_T_TILDA);
		tp_add(node_add(n, TP_N_ADD, "", ""));
	}
}

function tp_nexttoopts(n)
{
	tp_gettokentype(TP_T_COLON);

	tp_gettokentype(TP_T_STRING);
	node_add(n, TP_T_STRING, Tp_tokval, Tp_tokline);
}

function tp_nextto(n,	t)
{
	tp_cat(node_add(n, TP_N_CAT, "", ""));
	while (tp_peektoken() == TP_T_NEXTTOOP) {
		tp_gettokentype(TP_T_NEXTTOOP);
		node_add(n, TP_T_NEXTTOOP, Tp_tokval, Tp_tokline);
		tp_cat(node_add(n, TP_N_CAT, "", ""));

		if (tp_peektoken() == TP_T_COLON)
			tp_nexttoopts(n);
	}
}
function tp_rel(n)
{
	tp_nextto(node_add(n, TP_N_NEXTTO, "", ""));
	if (tp_peektoken() == TP_T_RELOP) {
		tp_gettokentype(TP_T_RELOP);
		node_add(n, TP_T_RELOP, Tp_tokval, Tp_tokline);
		tp_nextto(node_add(n, TP_N_NEXTTO, "", ""));
	}
}

function tp_and(n)
{
	tp_rel(node_add(n, TP_N_REL, "", ""));
	while (tp_peektoken() == TP_T_AND) {
		tp_gettokentype(TP_T_AND);
		tp_rel(node_add(n, TP_N_REL, "", ""));
	}
}

function tp_or(n)
{
	tp_and(node_add(n, TP_N_AND, "", ""));
	while (tp_peektoken() == TP_T_OR) {
		tp_gettokentype(TP_T_OR);
		tp_and(node_add(n, TP_N_AND, "", ""));
	}
}

function tp_ternary(n)
{
	tp_or(node_add(n, TP_N_OR, "", ""));
	while (tp_peektoken() == TP_T_QUEST) {
		tp_gettokentype(TP_T_QUEST);
		tp_or(node_add(n, TP_N_OR, "", ""));
		tp_gettokentype(TP_T_COLON);
		tp_or(node_add(n, TP_N_OR, "", ""));
	}
}

function tp_assign(n)
{
	tp_ternary(node_add(n, TP_N_TERNARY, "", ""));
	while (tp_peektoken() == TP_T_ASSIGN) {
		tp_gettokentype(TP_T_ASSIGN);
		tp_ternary(node_add(n, TP_N_TERNARY, "", ""));
	}
}

function tp_expr(n)
{
	tp_assign(node_add(n, TP_N_ASSIGN, "", ""));
	while (tp_peektoken() == TP_T_COMMA) {
		tp_gettokentype(TP_T_COMMA);
		tp_assign(node_add(n, TP_N_ASSIGN, "", ""));
	}
}

function tp_block(n)
{
	tp_gettokentype(TP_T_LBRC);

	while (tp_peektoken() != TP_T_RBRC)
		tp_stmt(n);

	tp_gettokentype(TP_T_RBRC);
}

function tp_body(n,	t)
{
	t = tp_peektoken();
	if (t == TP_T_LBRC)
		tp_block(n);
	else
		tp_stmt(n);
}

function tp_for(n,	t)
{
	tp_gettokentype(TP_T_FOR);
	tp_gettokentype(TP_T_LPAR);

	tp_forexpr(node_add(n, TP_N_FOREXPR, "", ""));

	t = tp_peektoken();
	if (t == TP_T_IN)
		tp_fortable(n);
	else
		tp_forclassic(n);

	tp_gettokentype(TP_T_RPAR);
	
	tp_body(node_add(n, TP_N_BLOCK, "", ""));
}

function tp_forexpr(n)
{
	t = tp_peektoken();
	if (t == TP_T_SEMICOL || t == TP_T_RPAR)
		return;
	
	tp_expr(node_add(n, TP_N_EXPR, "", ""));
}

function tp_forclassic(n)
{
	tp_gettokentype(TP_T_SEMICOL);

	tp_forexpr(node_add(n, TP_N_FOREXPR, "", ""));
	
	tp_gettokentype(TP_T_SEMICOL);
	
	tp_forexpr(node_add(n, TP_N_FOREXPR, "", ""));	
}

function tp_fortable(n)
{
	tp_gettokentype(TP_T_IN);
	tp_expr(node_add(n, TP_N_EXPR, "", ""));

	t = tp_peektoken();
	if (t == TP_T_FORDIR) {
		tp_gettokentype(TP_T_FORDIR);
		node_add(n, t, Tp_tokval, Tp_tokline);
	}
}

function tp_if(n)
{
	tp_gettokentype(TP_T_IF);
	tp_gettokentype(TP_T_LPAR);

	tp_expr(node_add(n, TP_N_EXPR, "", ""));

	tp_gettokentype(TP_T_RPAR);

	tp_body(node_add(n, TP_N_BLOCK, "", ""));

	if (tp_peektoken() == TP_T_ELSE) {
		tp_gettokentype(TP_T_ELSE);
		tp_body(node_add(n, TP_N_BLOCK, "", ""));
	}
}

function tp_return(n)
{
	tp_gettokentype(TP_T_RETURN);

	if (tp_peektoken() != TP_T_SEMICOL)
		tp_expr(node_add(n, TP_N_EXPR, "", ""));

	tp_gettokentype(TP_T_SEMICOL);
}

function tp_stmt(n,	t)
{
	t = tp_peektoken();

	if (t == TP_T_FOR)
		tp_for(node_add(n, TP_N_FOR, "", ""));
	else if (t == TP_T_IF)
		tp_if(node_add(n, TP_N_IF, "", ""));
	else if (t == TP_T_LBRC)
		tp_block(node_add(n, TP_N_BLOCK, "", ""));
	else if (t == TP_T_RETURN)
		tp_return(node_add(n, TP_N_RETURN, "", ""));
	else if (t == TP_T_BREAK) {
		tp_gettokentype(TP_T_BREAK);
		node_add(n, TP_T_BREAK, Tp_tokval, Tp_tokline);
		tp_gettokentype(TP_T_SEMICOL);
	}
	else if (t == TP_T_CONTINUE) {
		tp_gettokentype(TP_T_CONTINUE);
		node_add(n, TP_T_CONTINUE, Tp_tokval, Tp_tokline);
		tp_gettokentype(TP_T_SEMICOL);
	}
	else {
		tp_expr(node_add(n, TP_N_EXPR, "", ""));
		tp_gettokentype(TP_T_SEMICOL);
	}
}

function tp_defargs(n)
{
	if (tp_peektoken() != TP_T_ID)
		return;

	tp_gettokentype(TP_T_ID);
	node_add(n, TP_T_ID, Tp_tokval, Tp_tokline);

	while (tp_peektoken() == TP_T_COMMA) {
		tp_gettokentype(TP_T_COMMA);
		tp_gettokentype(TP_T_ID);
		node_add(n, TP_T_ID, Tp_tokval, Tp_tokline);
	}
}

function tp_funcdef(n)
{
	tp_gettokentype(TP_T_FUNCTION);

	tp_gettokentype(TP_T_ID);
	node_add(n, TP_T_ID, Tp_tokval, Tp_tokline);

	tp_gettokentype(TP_T_LPAR);

	tp_defargs(node_add(n, TP_N_DEFARGS, "", ""));

	tp_gettokentype(TP_T_RPAR);

	tp_block(node_add(n, TP_N_BLOCK, "", ""));
}

function tp_defs(n,	t)
{
	while ((t = tp_peektoken()) == TP_T_FUNCTION)
		tp_funcdef(node_add(n, TP_N_FUNCDEF, "", ""));
}

function tp_template(path,	n, b)
{
	Tp_file = path;

	n = node_add(-1, TP_N_TEMPLATE, "", "");

	tp_defs(node_add(n, TP_N_DEFS, "", ""));


	b = node_add(n, TP_N_BLOCK, "", "");
	while (tp_peektoken() != TP_T_EOF)
		tp_stmt(b);

	tp_gettokentype(TP_T_EOF);

	Tp_lnum = 0;
	Tp_l = "";
	Tp_nexttoktype = Tp_nexttokval = Tp_nexttokline = "";
	close(Tp_file);

	return n;
}

BEGIN {
	TP_T_ID = 1;		TP_T_INT = 2;		TP_T_FLOAT = 3;
	TP_T_STRING = 4;	TP_T_RELOP = 5;		TP_T_LPAR = 6;
	TP_T_RPAR = 7;		TP_T_AND = 8;		TP_T_OR = 9;
	TP_T_TILDA = 10;	TP_T_COMMA = 11;	TP_T_NOT = 12;
	TP_T_SEMICOL = 13;	TP_T_COLON = 14;	TP_T_DOL = 15;
	TP_T_ADDOP = 16;	TP_T_MULTOP = 17;	TP_T_LBRC = 18;
	TP_T_RBRC = 19;		TP_T_LBRK = 20;		TP_T_RBRK = 21;
	TP_T_ASSIGN = 22;	TP_T_NEXTTOOP = 23;	TP_T_FOR = 24;
	TP_T_FUNCTION = 25;	TP_T_GLOBAL = 26;	TP_T_IF = 27;
	TP_T_ELSE = 28;		TP_T_RETURN = 29;	TP_T_IN = 30;
	TP_T_FDEFS = 32;	TP_T_FORDIR = 33;	TP_T_POINT = 34;
	TP_T_CONTINUE = 35;	TP_T_BREAK = 36;	TP_T_DDOT = 37;
	TP_T_QUEST = 38;	TP_T_EOF = 39;

	TP_N_TEMPLATE = 51;	TP_N_DEFS = 52;		TP_N_DEF = 54;
	TP_N_FUNCDEF = 55;	TP_N_DEFARGS = 56;	TP_N_GLOBDEF = 57;
	TP_N_STMTS = 58;	TP_N_EXPR = 59;		TP_N_AND = 60;
	TP_N_OR = 61;		TP_N_REL = 62;		TP_N_NEXTTO = 63;
	TP_N_ADD = 64;		TP_N_MULT = 65;		TP_N_UNARY = 66;
	TP_N_NOT = 67;		TP_N_SIGN = 68;		TP_N_VAL = 69;
	TP_N_CONST = 70;	TP_N_ADDRESS = 71;	TP_N_ARGS = 72;
	TP_N_INDEX = 73;	TP_N_FILTER = 74;	TP_N_ATTR = 75;
	TP_N_IF = 76;		TP_N_FOR = 77;		TP_N_OPTEXPR = 78;
	TP_N_BODY = 79;		TP_N_BLOCK = 80;	TP_N_RETURN = 81;
	TP_N_NEXTTOOPTS = 82;	TP_N_CAT = 83;		TP_N_TERNARY = 84;
	TP_N_ASSIGN = 85;

	Tp_strsym[TP_T_ID] = "identificator";
	Tp_strsym[TP_T_INT] = "integer";
	Tp_strsym[TP_T_FLOAT] = "float";
	Tp_strsym[TP_T_RELOP] = "relational operator";
	Tp_strsym[TP_T_LPAR] = "(";
	Tp_strsym[TP_T_RPAR] = ")";
	Tp_strsym[TP_T_AND] = "&";
	Tp_strsym[TP_T_OR] = "|";
	Tp_strsym[TP_T_NOT] = "!";
	Tp_strsym[TP_T_DOL] = "$";
	Tp_strsym[TP_T_TILDA] = "~";
	Tp_strsym[TP_T_COMMA] = ",";
	Tp_strsym[TP_T_COLON] = ":";
	Tp_strsym[TP_T_SEMICOL] = ";";
	Tp_strsym[TP_T_DDOT] = "..";
	Tp_strsym[TP_T_QUEST] = "?";
	Tp_strsym[TP_T_MULTOP] = "multiplication or division";
	Tp_strsym[TP_T_ADDOP] = "addition or substraction";
	Tp_strsym[TP_T_STRING] = "quoted string";
	Tp_strsym[TP_T_LBRC] = "{";
	Tp_strsym[TP_T_RBRC] = "}";
	Tp_strsym[TP_T_LBRK] = "[";
	Tp_strsym[TP_T_RBRK] = "]";
	Tp_strsym[TP_T_ASSIGN] = "=";
	Tp_strsym[TP_T_FORDIR] = "for direction";
	Tp_strsym[TP_T_NEXTTOOP] = "next to operator";
	Tp_strsym[TP_T_FOR] = "for keyword";
	Tp_strsym[TP_T_FUNCTION] = "function keyword";
	Tp_strsym[TP_T_GLOBAL] = "global keyword";
	Tp_strsym[TP_T_IF] = "if keyword";
	Tp_strsym[TP_T_ELSE] = "else";
	Tp_strsym[TP_T_CONTINUE] = "continue keyword";
	Tp_strsym[TP_T_BREAK] = "break keyword";
	Tp_strsym[TP_T_RETURN] = "return keyword";
	Tp_strsym[TP_T_IN] = "in";
	Tp_strsym[TP_T_EOF] = "EOF";

	Tp_strsym[TP_N_TEMPLATE] = "template";
	Tp_strsym[TP_N_DEFS] = "definitions";
	Tp_strsym[TP_N_DEF] = "definition";
	Tp_strsym[TP_N_FUNCDEF] = "function definition";
	Tp_strsym[TP_N_DEFARGS] = "function definition arguments";
	Tp_strsym[TP_N_GLOBDEF] = "global variable definition";
	Tp_strsym[TP_N_STMTS] = "statements";
	Tp_strsym[TP_N_EXPR] = "expression";
	Tp_strsym[TP_N_TERNARY] = "ternary operator";
	Tp_strsym[TP_N_OR] = "logical or";
	Tp_strsym[TP_N_AND] = "logical and";
	Tp_strsym[TP_N_REL] = "relation";
	Tp_strsym[TP_N_NEXTTO] = "next to";
	Tp_strsym[TP_N_CAT] = "string concatenation";
	Tp_strsym[TP_N_ADD] = "addtion or substraction";
	Tp_strsym[TP_N_MULT] = "multiplication or division";
	Tp_strsym[TP_N_UNARY] = "unary operator";
	Tp_strsym[TP_N_NOT] = "not";
	Tp_strsym[TP_N_SIGN] = "sign";
	Tp_strsym[TP_N_VAL] = "value";
	Tp_strsym[TP_N_CONST] = "constant";
	Tp_strsym[TP_N_ADDRESS] = "id usage";
	Tp_strsym[TP_N_ARGS] = "function arguments";
	Tp_strsym[TP_N_INDEX] = "table cells access";
	Tp_strsym[TP_N_FILTER] = "filter";
	Tp_strsym[TP_N_ATTR] = "table attribute access";
	Tp_strsym[TP_N_IF] = "if";
	Tp_strsym[TP_N_FOR] = "for";
	Tp_strsym[TP_N_FOREXPR] = "for expression";
	Tp_strsym[TP_N_BODY] = "statement body";
	Tp_strsym[TP_N_BLOCK] = "block";
	Tp_strsym[TP_N_RETURN] = "return";
	Tp_strsym[TP_N_NEXTTOOPTS] = "next to options";
	Tp_strsym[TP_N_ASSIGN] = "assignment";

#	Tp_file = "test.txt";

#	Root = node_add(-1, TP_N_TEMPLATE, "", "");
#	tp_template(Root);

#	node_print(Root);

#	printf "-------------------------------------------\n";
#	while ((Toktype = tp_gettoken()) != TP_T_EOF) {
#		printf "|%-20s|%20s|\n", Tp_strsym[Toktype], Tp_tokval;
#		printf "-------------------------------------------\n";
#	}

# build a tree
#	Root = node_add(-1, N_BLOCK, "", "");
#	node_add(Root, N_ITER, "", "");
#	node_add(Root, N_BODY, "", "");
#	node_add(Root, N_GROW, "", "");
#
#	body(Node[Root,"child",1]);

#	printnode(0, 0);}
}
