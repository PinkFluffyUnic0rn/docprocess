# Global:
#	Tg_val []
#	Tg_valcount
#	Tg_var []
#	Tg_stacktop
#	Tg_depth
#	Tg_cmdres
#
#	TG_***

# Values and types management
#----------------------------------------------------------------------
function tg_newval(v, t,	nv)
{
	nv = ++Tg_valcount;

	Tg_val[nv] = v;
	Tg_val[nv,"type"] = t;

	return nv;
}

function tg_copyval(v)
{
	return tg_newval(Tg_val[v], Tg_val[v,"type"]);
}

function tg_printval(v)
{
	if (Tg_val[v,"type"] == TG_EMPTY)
		printf "empty";
	if (Tg_val[v,"type"] == TG_FUNCTION)
		printf "function";
	else if (Tg_val[v,"type"] == TG_INT)
		printf "integer: %s", Tg_val[v];
	else if (Tg_val[v,"type"] == TG_FLOAT)
		printf "float: %s", Tg_val[v];
	else if (Tg_val[v,"type"] == TG_STRING)
		printf "string: %s", Tg_val[v];
	else if (Tg_val[v,"type"] == TG_TABLE) {
		printf "table:\n";
		ta_printtable(Tg_val[v]);
	}
}

function tg_strtype(v)
{
	if (match(v, /^[-+]?[0-9]*\.[0-9]+([eE][-+]?[0-9]+)?$/))
		return TG_FLOAT;
	else if (match(v, /^[0-9]+$/))
		return TG_INT;

	return TG_STRING;
}

function tg_valtotable(v,	t)
{
	t = ++Tablecount;

	Table[t,"rows"] = Table[t,"cols"] = 1;
	Table[t,"attrs"] = 4;
	Table[t,"attrname",1] = "val";
	Table[t,"attrname",2] = "type";
	Table[t,"attrname",3] = "vspan";
	Table[t,"attrname",4] = "hspan";

	if (Tg_val[v,"type"] == TG_INT) {
		Table[t,1,1,"val"] = Tg_val[v] + 0;
		Table[t,1,1,"type"] = TA_INT;
	}
	else if (Tg_val[v,"type"] == TG_FLOAT) {
		Table[t,1,1,"val"] = Tg_val[v] + 0;
		Table[t,1,1,"type"] = TA_FLOAT;
	}
	else if (Tg_val[v,"type"] == TG_STRING) {
		Table[t,1,1,"val"] = Tg_val[v] "";
		Table[t,1,1,"type"] = TA_STRING;
	}

	Table[t,1,1,"hspan"] = Table[t,1,1,"vspan"] = 1;

	return t;
}

function tg_cast(v, t,	newv, vt, cv, st)
{
	vt = Tg_val[v,"type"];

	assert(vt != TG_FUNCTION, "Cannot cast a function id.");
	assert(vt != TG_SCRIPT, "Cannot cast a script.");

	assert(t != TG_EMPTY, "Cannot cast to empty value.");
	assert(t != TG_FUNCTION, "Cannot cast to a function.");
	assert(t != TG_SCRIPT, "Cannot cast to a script.");

	newv = ++Tg_valcount;

	if (vt == TG_EMPTY) {
		if (t == TG_INT)
			Tg_val[newv] = 0;
		else if (t == TG_FLOAT)
			Tg_val[newv] = 0.0;
		else if (t == TG_STRING)
			Tg_val[newv] = "";
		else if (t == TG_TABLE) {
			t = ++Tablecount;

			Table[t,"rows"] = Table[t,"cols"] = 0;
			Table[t,"attrs"] = 4;
			Table[t,"attrname",1] = "val";
			Table[t,"attrname",2] = "type";
			Table[t,"attrname",3] = "vspan";
			Table[t,"attrname",4] = "hspan";

			return tg_newval(t, TG_TABLE);
		}
	}
	else if (vt == TG_INT && t == TG_FLOAT)
		Tg_val[newv] = Tg_val[v];
	else if (vt == TG_FLOAT && t == TG_INT)
		Tg_val[newv] = int(Tg_val[v]);
	else if (vt == TG_INT && t == TG_STRING) {
		Tg_val[newv] = Tg_val[v] ""
	}
	else if (vt == TG_FLOAT && t == TG_STRING) {
		# float to string maybe in scientific
		# possible to add user option for it
		Tg_val[newv] = sprintf("%f", Tg_val[v]);
	}
	else if (vt == TG_STRING && t != TG_TABLE) {
		st = tg_strtype(Tg_val[v]);

		if (t == TG_INT)
			Tg_val[newv] = int(Tg_val[v] + 0);
		else if (t == TG_FLOAT) 
			Tg_val[newv] = Tg_val[v] + 0;
		else
			Tg_val[newv] = Tg_val[v];
	}
	else if (vt != TG_TABLE && t == TG_TABLE)
		Tg_val[newv] = tg_valtotable(v);
	else if (vt == TG_TABLE && t != TG_TABLE) {
		assert(Table[Tg_val[v],"rows"] <= 1 \
			&& Table[Tg_val[v],"cols"] <= 1,
			"Cannot convert table with more than " \
			"one cell to integer or string.");

		if (t == TG_INT) {
			cv = Table[Tg_val[v],1,1,"val"];
			Tg_val[newv] = int(cv + 0);
		}
		else if (t == TG_FLOAT) {
			cv = Table[Tg_val[v],1,1,"val"];
			Tg_val[newv] = cv + 0;
		}
		else if (t == TG_STRING)
			Tg_val[newv] = Table[Tg_val[v],1,1,"val"] "";
		else
			Tg_val[newv] = Tg_val[v];
	}
	else
		Tg_val[newv] = Tg_val[v];

	Tg_val[newv,"type"] = t;

	return newv;
}

function tg_typeprom1(v, mtype)
{
	if (Tg_val[v,"type"] > mtype)
		return tg_cast(v, mtype);

	return v;
}

function tg_typeprom2(v, v2type, mtype,		v1type)
{
	v1type = Tg_val[v,"type"];

	if (v1type > mtype || v2type > mtype)
		return tg_cast(v, mtype);
	else if (v1type < v2type)
		return tg_cast(v, v2type);

	return v;
}

function tg_istrue(v,	nv)
{
	if (Tg_val[v,"type"] == TG_INT)
		return (Tg_val[v] != 0);
	else if (Tg_val[v,"type"] == TG_FLOAT)
		return (Tg_val[v] != 0);
	else if (Tg_val[v,"type"] == TG_STRING)
		return (length(Tg_val[v]) != 0);
	else if (Tg_val[v,"type"] == TG_TABLE)
		return 1;

	return 0;
}

# statements
#----------------------------------------------------------------------
function tg_for(f, v, e, t, b, c, i, j, type, vv, r)
{
	if (node_type(node_child(f, 2)) == TP_N_EXPR) {
		return tg_fortable(f);
	}
	else
		return tg_forclassic(f);
}

function tg_fortable(f, v, e, t, b, c, i, j, type, vv, r)
{
	v = tg_lvalue(node_child(node_child(f, 1), 1));
	e = tg_cast(tg_expr(node_child(f, 2)), TG_TABLE);
	t = Tg_val[e];
	c = node_type(node_child(f, 3)) == TP_T_FORDIR ? 1 : 0;
	b = node_type(node_child(f, 3)) == TP_N_BLOCK ? 3 : 4;

	for (i = 1; i <= (c ? Table[t,"cols"] : Table[t,"rows"]); ++i) {
		vv = c ? ta_filter(t, "", i, "") \
			: ta_filter(t, i, "", "");

		Tg_var[Tg_depth + 1,v] = tg_newval(vv, TG_TABLE);

		r = tg_block(node_child(f, b));

		if (r > 0)
			return r;
		else if (r == TG_BREAK)
			break;
	}

	return TG_SUCCESS;
}

function tg_forclassic(f,	init, cond, iter, r)
{
	if (node_childrencount(node_child(f, 1)))
		init = node_child(node_child(f, 1), 1);
	
	if (node_childrencount(node_child(f, 2)))
		cond = node_child(node_child(f, 2), 1);

	if (node_childrencount(node_child(f, 3)))
		iter = node_child(node_child(f, 3), 1);

	if (init != "") tg_expr(init);

	while (!cond || tg_istrue(tg_expr(cond))) {
		r = tg_block(node_child(f, 4));

		if (r > 0)
			return r;
		else if (r == TG_BREAK)
			break;

		if (iter) tg_expr(iter);
	}
	
	return TG_SUCCESS;
}

function tg_if(f,	b)
{
	b = tg_expr(node_child(f, 1));

	if (tg_istrue(b))
		return tg_block(node_child(f, 2));
	else if (node_childrencount(f) > 2)
		return tg_block(node_child(f, 3));
}

function tg_block(b, oldvc, oldtc, i, s, r, v, t, rr)
{
	r = tg_newval("", TG_EMPTY);

	# !!! why + 1 is needed?
	oldvc = Tg_valcount;
#	oldtc = Tablecount;

	++Tg_depth;

	for (i = 1; i <= node_childrencount(b); ++i) {
		s = node_child(b, i);
		t = node_type(s);

		if (t == TP_N_EXPR)
			tg_expr(s);
		else if (t == TP_N_BLOCK)
			rr = tg_block(s);
		else if (t == TP_N_IF)
			rr = tg_if(s);
		else if (t == TP_N_FOR)
			rr = tg_for(s);
		else if (t == TP_T_BREAK)
			rr = TG_BREAK;
		else if (t == TP_T_CONTINUE)
			rr = TG_CONTINUE;
		else if (node_type(s) == TP_N_RETURN) {
			rr = r;

			if (node_childrencount(s) != 0) {
				v = tg_expr(node_child(s, 1));

				Tg_val[r] = Tg_val[v];
				Tg_val[r,"type"] = Tg_val[v,"type"];
			}
		}

		if (rr != 0)
			break;
	}

	for (i in Tg_var) {
		if (substr(i, 1, index(i, SUBSEP) - 1) == Tg_depth)
			delete Tg_var[i];
	}

	--Tg_depth;

	Tg_valcount = oldvc;
#	Tablecount = oldtc;

	return rr;
}

# built-in functions
#----------------------------------------------------------------------
function tg_dateformat(s)
{
	if (s in Dateformat)
		return Dateformat[s];
	else if (s ~ /^[0-9][0-9][0-9][0-9]\-[0-9][0-9]\-[0-9][0-9]$/)
		return (Dateformat[s] = TG_ISODATE);
	else if (s ~ /^[0-9][0-9]\.[0-9][0-9]\.[0-9][0-9][0-9][0-9]$/)
		return (Dateformat[s] = TG_RUDATE);
	else if (s ~ /^[0-9]+\/[0-9]+\/[0-9][0-9][0-9][0-9]$/)
		return (Dateformat[s] = TG_USDATE);

	return 0;
}

function _tg_day(d,	f, a)
{
	assert(f = tg_dateformat(d), "Wrong date format: " d);

	if (f == TG_ISODATE)
		return substr(d, 9, 2) + 0;
	else if (f == TG_RUDATE)
		return substr(d, 1, 2) + 0;
	else if (f == TG_USDATE) {
		split(d, a, /\//);
		return a[2] + 0;
	}
}

function _tg_month(d,	f, a)
{
	assert(f = tg_dateformat(d), "Wrong date format: " d);

	if (f == TG_ISODATE)
		return substr(d, 6, 2) + 0;
	else if (f == TG_RUDATE)
		return substr(d, 4, 2) + 0;
	else if (f == TG_USDATE) {
		split(d, a, /\//);
		return a[1];
	}
}

function _tg_year(d,	f, a)
{
	assert(f = tg_dateformat(d), "Wrong date format: " d);

	if (f == TG_ISODATE)
		return substr(d, 1, 4) + 0;
	else if (f == TG_RUDATE)
		return substr(d, 7, 4) + 0;
	else if (f == TG_USDATE) {
		split(d, a, /\//);
		return a[3];
	}
}

function tg_datedays(d,	M, s, y)
{
	y = _tg_year(d);
	split("0;31;59;90;120;151;181;212;243;273;304;334;365", M, ";");

	return (365 * (y - 1) + M[_tg_month(d)] + _tg_day(d) \
		+ int(y / 4) + int(y / 400) - int(y / 100));
}

function tg_datediff(d1, d2, s)
{
	d1s = Tg_val[d1];
	d2s = Tg_val[d2];

	assert(tg_dateformat(d1s), "Wrong date format: " d1s);
	assert(tg_dateformat(d2s), "Wrong date format: " d2s);

	if (Tg_val[s] == "day") {
		return tg_newval(tg_datedays(d2s) - tg_datedays(d1s),
			TG_INT);
	}
	else if (Tg_val[s] == "month") {
		return tg_newval((_tg_year(d2s) - _tg_year(d1s)) * 12 \
			+ (_tg_month(d2s) - _tg_month(d1s)), TG_INT);
	}
	else if (Tg_val[s] == "year")
		return tg_newval(_tg_year(d2s) - _tg_year(d1s), TG_INT);
}

function tg_datecmp(d1, d2)
{
	d1s = Tg_val[d1];
	d2s = Tg_val[d2];

	if (_tg_year(d1s) < _tg_year(d2s))
		return tg_newval(-1, TG_INT);
	else if (_tg_year(d1s) > _tg_year(d2s))
		return tg_newval(1, TG_INT);
	else {
		if (_tg_month(d1s) < _tg_month(d2s))
			return tg_newval(-1, TG_INT);
		else if (_tg_month(d1s) > _tg_month(d2s))
			return tg_newval(1, TG_INT);
		else {
			if (_tg_day(d1s) < _tg_day(d2s))
				return tg_newval(-1, TG_INT);
			else  if (_tg_day(d1s) > _tg_day(d2s))
				return tg_newval(1, TG_INT);
			else
				return tg_newval(0, TG_INT);
		}
	}
}

function _tg_monthend(m, y,	M, l)
{
	split("31;28;31;30;31;30;31;31;30;31;30;31", M, ";");

	return M[m] + (m == 2 && ((y % 4) ? 0 : ((y % 100) \
		? 1 : ((y % 400) ? 0 : 1))));
}

# dont work with negative values!!!
function tg_dateadd(dt, i, s, dts, is, d, m, y)
{
	dts = Tg_val[dt];
	is = Tg_val[i];

	d = _tg_day(dts);
	m = _tg_month(dts);
	y = _tg_year(dts);

	if (Tg_val[s] == "day")
		d += is;
	else if (Tg_val[s] == "month")
		m += is;
	else if (Tg_val[s] == "year")
		y += is;

	while (d <= 0 || m <= 0 || d > _tg_monthend(m, y) || m > 12) {
		if (d <= 0) {
			y = m > 1 ? y : y - 1;
			m = m > 1 ? m - 1 : 12;
			d += _tg_monthend(m, y);
		}

		if (m <= 0) {
			m += 12;
			--y;
		}

		if (d > _tg_monthend(m, y)) {
			d -= _tg_monthend(m, y);
			++m;
		}

		if (m > 12) {
			m -= 12;
			++y
		}

	}

	return tg_newval(sprintf("%02d.%02d.%04d", d, m, y), TG_STRING);
}

function _tg_weekday(dt,	d, m, y, j, k, h)
{
	d = _tg_day(dt);
	m = _tg_month(dt);
	y = _tg_year(dt);

	if (m == 1 || m == 2) {
		m += 12;
		--y;
	}

	j = int(y / 100);
	k = y % 100;

	h = (d + int(13 * (m + 1) / 5) + k \
		+ int(k / 4) + int(j / 4)  + 5 * j) % 7;

	return ((h + 5) % 7) + 1;
}

# expressions
#----------------------------------------------------------------------
function tg_accessvar(id,	i)
{
	for (i = Tg_depth; i >= Tg_stacktop; --i)
		if ((i SUBSEP id) in Tg_var)
			return Tg_var[i,id];

	if ((0 SUBSEP id) in Tg_var)
		return Tg_var[0,id];

	return (-1);
}

function tg_lvalue(e,	i)
{
	while (node_type(e) != TP_T_ID) {
		assert(node_childrencount(e) == 1,
			"Expected lvalue, but got an expression.");
		e = node_child(e, 1);
	}

	return node_val(e);
}

function tg_assign(e,	i, j, lv, rv, d, vv)
{
	rv = tg_ternary(node_child(e, node_childrencount(e)));

	for (i = 1; i < node_childrencount(e); ++i) {
		lv = tg_lvalue(node_child(e,i));
		
		if ((vv = tg_accessvar(lv)) >= 0) {
			Tg_val[vv] = Tg_val[rv];
			Tg_val[vv,"type"] = Tg_val[rv,"type"];
		}	
		else
			Tg_var[Tg_depth,lv] = tg_copyval(rv);
	}

	return rv;
}

function tg_expr(c,	i, r)
{
	for (i = 1; i < node_childrencount(c); ++i)
		tg_assign(node_child(c, i));

	return tg_assign(node_child(c, node_childrencount(c)));
}

function tg_ternary(t,		i, r)
{
	r = tg_or(node_child(t, 1));

	for (i = 2; i < node_childrencount(t); i += 2) {
		r = tg_istrue(r) \
			? tg_or(node_child(t, i)) \
			: tg_or(node_child(t, i + 1));
	}

	return r;
}

function tg_or(o,	i, a)
{
	if (node_childrencount(o) == 1)
		return tg_and(node_child(o, 1));

	for (i = 1; i <= node_childrencount(o); ++i) {
		a = tg_and(node_child(o, i));
		if (tg_istrue(a))
			return tg_newval(1, TG_INT);
	}

	return tg_newval(0, TG_INT);
}

function tg_and(a,	i, r)
{
	if (node_childrencount(a) == 1)
		return tg_rel(node_child(a, 1));

	for (i = 1; i <= node_childrencount(a); ++i) {
		r = tg_rel(node_child(a, i));

		if (!tg_istrue(r))
			return tg_newval(0, TG_INT);
	}

	return tg_newval(1, TG_INT);
}

function tg_rel(r,	n1, n2, v1, v2, op, t)
{
	n1 = tg_nextto(node_child(r, 1));

	if (node_childrencount(r) == 1)
		return n1;

	op = node_val(node_child(r, 2));
	n2 = tg_nextto(node_child(r, 3));

	v1 = tg_typeprom2(n1, Tg_val[n2,"type"], TG_STRING);
	v2 = tg_typeprom2(n2, Tg_val[n1,"type"], TG_STRING);

	if (op == "==")
		t = Tg_val[v1] == Tg_val[v2];
	else if (op == "!=")
		t = Tg_val[v1] != Tg_val[v2];
	else if (op == "<")
		t = Tg_val[v1] < Tg_val[v2];
	else if (op == ">")
		t = Tg_val[v1] > Tg_val[v2];
	else if (op == "<=")
		t = Tg_val[v1] <= Tg_val[v2];
	else if (op == ">=")
		t = Tg_val[v1] >= Tg_val[v2];

	return tg_newval(t, TG_INT);
}

function tg_nextto(n,	i, r, t, op, opt)
{
	if (node_childrencount(n) == 1)
		return tg_cat(node_child(n, 1));

	r = Tg_val[tg_cast(tg_cat(node_child(n, 1)), TG_TABLE)];

	for (i = 2; i < node_childrencount(n); i += 2) {
		op = node_val(node_child(n, i));

		t = tg_cat(node_child(n, i + 1));

		if (node_type(node_child(n, i + 2)) == TP_T_STRING) {
			opt = node_val(node_child(n, i + 2));
			++i;
		}

		t = Tg_val[tg_cast(t, TG_TABLE)];

		r = ta_tablenextto(r, t, op == "^", opt == "span");
	}

	return tg_newval(r, TG_TABLE);
}

function tg_cat(c,	s, ss, i)
{
	if (node_childrencount(c) == 1)
		return tg_add(node_child(c, 1)); 

	ss = tg_cast(tg_add(node_child(c, 1)), TG_STRING);
	for (i = 2; i <= node_childrencount(c); ++i) {
		s = tg_cast(tg_add(node_child(c, i)), TG_STRING);

		ss = tg_newval(Tg_val[ss] Tg_val[s], TG_STRING);
	}
	return ss;
}

function tg_add(a,	s, i, p, op, t)
{	
	s = tg_mult(node_child(a, 1));

	for (i = 2; i < node_childrencount(a); i += 2) {
		p = tg_mult(node_child(a, i + 1));
		op = node_val(node_child(a, i));
		
		t = tg_typeprom2(p, Tg_val[s,"type"], TG_FLOAT);
		s = tg_typeprom2(s, Tg_val[p,"type"], TG_FLOAT);

		s = tg_newval( \
			Tg_val[s] + ((op == "+") ? 1 : -1) * Tg_val[t],
			Tg_val[s,"type"]);
	}

	return s;
}

function tg_mult(m, 	p, v, i, t, op)
{
	p = tg_val(node_child(m, 1));

	for (i = 2; i < node_childrencount(m); i += 2) {
		v = tg_val(node_child(m, i + 1));
		op = node_val(node_child(m, i));

		t = tg_typeprom2(v, Tg_val[p,"type"], TG_FLOAT);
		p = tg_typeprom2(p, Tg_val[v,"type"], TG_FLOAT);

		p = tg_newval((op == "*") \
			? Tg_val[p] * Tg_val[t] : Tg_val[p] / Tg_val[t],
			Tg_val[p,"type"]);
	
		Tg_val[p] = (Tg_val[p,"type"] == TG_INT) \
			? int(Tg_val[p]) : Tg_val[p];
	}

	return p;
}

function tg_assertargs(fname, argscount, neededcount)
{
	assert(argscount == neededcount,
		"Cannot call function " fname " with " argscount \
		" arguments. It is defined with " neededcount \
		" arguments.");
}

function tg_argval(fnode, argn)
{
	return tg_assign(node_child(fnode, argn));
}

function tg_func(val, f,
	i, def, defargs, argname, argval, argscount, a, b, c, d, r, stackold)
{
	def = Tg_val[val];
	defargs = node_child(def, 2);
	argscount = node_childrencount(f);

	if (def == TG_BI_INT) {
		tg_assertargs("int", argscount, 1);

		return tg_cast(tg_argval(f, 1), TG_INT);
	}
	else if (def == TG_BI_FLOAT) {
		tg_assertargs("float", argscount, 1);

		return tg_cast(tg_argval(f, 1), TG_FLOAT);
	}
	else if (def == TG_BI_STRING) {
		tg_assertargs("string", argscount, 1);

		return tg_cast(tg_argval(f, 1), TG_STRING);
	}
	else if (def == TG_BI_TABLE) {
		tg_assertargs("table", argscount, 1);

		return tg_cast(tg_argval(f, 1), TG_TABLE);
	}
	else if (def == TG_BI_RUDATE) {
		tg_assertargs("rudate", argscount, 3);

		a = tg_cast(tg_argval(f, 1), TG_INT);
		b = tg_cast(tg_argval(f, 2), TG_INT);
		c = tg_cast(tg_argval(f, 3), TG_INT);

		return tg_newval(sprintf("%02d.%02d.%04d",
			Tg_val[a], Tg_val[b], Tg_val[c]),
			TG_STRING);
	}
	else if (def == TG_BI_DATECMP) {
		tg_assertargs("datecmp", argscount, 2);

		a = tg_cast(tg_argval(f, 1), TG_STRING);
		b = tg_cast(tg_argval(f, 2), TG_STRING);

		return tg_datecmp(a, b);
	}
	else if (def == TG_BI_DATEDIFF) {
		tg_assertargs("datediff", argscount, 3);

		a = tg_cast(tg_argval(f, 1), TG_STRING);
		b = tg_cast(tg_argval(f, 2), TG_STRING);
		c = tg_cast(tg_argval(f, 3), TG_STRING);

		return tg_datediff(a, b, c);
	}
	else if (def == TG_BI_YEAR) {
		tg_assertargs("year", argscount, 1);

		a = tg_cast(tg_argval(f, 1), TG_STRING)

		return tg_newval(_tg_year(Tg_val[a]), TG_INT);
	}
	else if (def == TG_BI_MONTH) {
		tg_assertargs("month", argscount, 1);

		a = tg_cast(tg_argval(f, 1), TG_STRING)

		return tg_newval(_tg_month(Tg_val[a]), TG_INT);
	}
	else if (def == TG_BI_DAY) {
		tg_assertargs("day", argscount, 1);

		a = tg_cast(tg_argval(f, 1), TG_STRING)

		return tg_newval(_tg_day(Tg_val[a]), TG_INT);
	}
	else if (def == TG_BI_MATCH) {
		tg_assertargs("match", argscount, 2);

		a = tg_cast(tg_argval(f, 1), TG_STRING);
		b = tg_cast(tg_argval(f, 2), TG_STRING);

		return tg_newval(match(Tg_val[a], Tg_val[b]), TG_INT);
	}
	else if (def == TG_BI_CAT) {
		tg_assertargs("cat", argscount, 2);

		a = tg_cast(tg_argval(f, 1), TG_STRING);
		b = tg_cast(tg_argval(f, 2), TG_STRING);

		return tg_newval(Tg_val[a] Tg_val[b], TG_STRING);
	}
	else if (def == TG_BI_SUBSTR) {
		tg_assertargs("substr", argscount, 3);

		a = tg_cast(tg_argval(f, 1), TG_STRING);
		b = tg_cast(tg_argval(f, 2), TG_INT);
		c = tg_cast(tg_argval(f, 3), TG_INT);

		return tg_newval( \
			substr(Tg_val[a], Tg_val[b], Tg_val[c]),
			TG_STRING);
	}
	else if (def == TG_BI_LENGTH) {
		tg_assertargs("length", argscount, 1);

		a = tg_cast(tg_argval(f, 1), TG_STRING);

		return tg_newval(length(Tg_val[a]), TG_INT);
	}
	else if (def == TG_BI_NEXTTO) {
		tg_assertargs("nextto", argscount, 4);

		a = tg_cast(tg_argval(f, 1), TG_TABLE);
		b = tg_cast(tg_argval(f, 2), TG_TABLE);
		c = tg_cast(tg_argval(f, 3), TG_STRING);
		d = tg_cast(tg_argval(f, 4), TG_STRING);

		return tg_newval(ta_tablenextto(Tg_val[a], Tg_val[b],
				Tg_val[c], Tg_val[d]), TG_TABLE);
	}
	else if (def == TG_BI_DEFVAL) {
		tg_assertargs("defval", argscount, 2);

		a = tg_argval(f, 1);
		b = tg_argval(f, 2);

		if (Tg_val[a,"type"] == TG_EMPTY)
			return b;

		return a;
	}
	else if (def == TG_BI_MONTHEND) {
		tg_assertargs("monthend", argscount, 2);

		a = tg_cast(tg_argval(f, 1), TG_INT);
		b = tg_cast(tg_argval(f, 2), TG_INT);

		return tg_newval(_tg_monthend(Tg_val[a], Tg_val[b]),
			TG_INT);
	}
	else if (def == TG_BI_WEEKDAY) {
		tg_assertargs("weekday", argscount, 1);

		a = tg_cast(tg_argval(f, 1), TG_STRING);

		return tg_newval(_tg_weekday(Tg_val[a]), TG_INT);
	}
	else if (def == TG_BI_DATEADD) {
		tg_assertargs("dateadd", argscount, 3);

		a = tg_cast(tg_argval(f, 1), TG_STRING);
		b = tg_cast(tg_argval(f, 2), TG_INT);
		c = tg_cast(tg_argval(f, 3), TG_STRING);

		return tg_dateadd(a, b, c);
	}

	tg_assertargs(node_val(node_child(def, 1)), argscount,
		node_childrencount(defargs));

	for (i = 1; i <= argscount; ++i)
		argval[i] = tg_argval(f, i);

	for (i = 1; i <= argscount; ++i) {
		argname = node_val(node_child(defargs, i));
		Tg_var[Tg_depth + 1,argname] = argval[i];
	}

	stackold = Tg_stacktop;
	Tg_stacktop = Tg_depth + 1;
	
	r = tg_block(node_child(def, 3));

	assert(r >= 0, "Error: break or continue not within a loop.");

	Tg_stacktop = stackold;

	return r;
}

function encodedef(fd,	v, t, j, s)
{
	# can mistreat func call or script as id. Ignore in this case.
	if ((v = tg_accessvar(fd)) < 0 \
		|| Tg_val[v,"type"] == TG_FUNCTION \
		|| Tg_val[v,"type"] == TG_SCRIPT \
		|| Tg_val[v,"type"] == TG_TEMPLATE)
		return "";

	if (Tg_val[v,"type"] == TG_INT)
		s = Tg_val[tg_cast(v, TG_INT)];
	else {
		s = Tg_val[tg_cast(v, TG_STRING)];

		gsub("\n", "\\n", s);
		gsub("\t", "\\t", s);
		gsub("\r", "\\r", s);
		gsub("\"", "\\\"", s);
		s = "\"" s "\"";
	}

	return (fd "=" s);
}

function tg_index(val, idx, rf, cf, fd, fdef, i, r, t)
{
	val = tg_cast(val, TG_TABLE);

	rf = node_val(node_child(idx, 1));

	if (node_type(node_child(idx, 2)) == TP_N_FILTER)
		cf = node_val(node_child(idx, 2));

	fd = node_val(node_child(idx, node_childrencount(idx)));

	split(fd, fdef, ";");

	fd = "";
	for (i in fdef)
		fd = sprintf("%s;%s", fd, encodedef(fdef[i]));
	fd = substr(fd, 2);
	
	r = ta_filter(Tg_val[val], rf, cf, fd);
	t = TG_TABLE;

	if (Table[r,"rows"] == 0 || Table[r,"cols"] == 0) {
		r = "";
		t = TG_EMPTY;
	}

	return tg_newval(r, t);
}

function tg_attr(val, attr,	v, id)
{
	assert(Tg_val[val,"type"] == TG_TABLE,
		"Cannot get attributes of non-table value .");

	id = node_val(node_child(attr, 1));
	v = Tg_val[val];

	if (id == "rows")
		return tg_newval(Table[v,"rows"], TG_INT);
	if (id == "cols")
		return tg_newval(Table[v,"cols"], TG_INT);

	assert(Table[v,"rows"] == 1 && Table[v,"cols"] == 1,
		"Cannot get cell attributes of subtable, " \
		"that not sigle-cell table");

	assert((v SUBSEP 1 SUBSEP 1 SUBSEP id) in Table,
		"There are no attribute " id "in table.");

	return tg_newval(Table[v,1,1,id], TG_STRING);
}

function tg_script(val, args,
	cmd, arg, t, sarg, tmpfile, i)
{
	arg = tg_cast(tg_argval(args, 1), TG_TABLE);
	t = Tg_val[arg];

	assert(Table[t,"rows"] == 1, Tg_val[val] ": table " \
		"that containing arguments to the script expected " \
		"to have only one row.");

	cmd = Tg_val[val];

	for (i = 1; i <= Table[t,"cols"]; ++i) {
		sarg = Table[t,1,i,"val"];

		gsub("'", "'\"'\"'", sarg);
		
		cmd = cmd " '" sarg "'";
	}

	if (!Tg_cmdres[cmd]) {
		"mktemp -u" | getline tmpfile;

		if (system(cmd " >" tmpfile) != 0) {
			system("rm " tmpfile);
			close("rm " tmpfile);
			close("mktemp -u");
			close(cmd " >" tmpfile "&");

			return tg_newval("", TG_EMPTY);
		}

		Tg_cmdres[cmd] = ta_csvtotable(tmpfile, Tg_defspan, 1);

		system("rm " tmpfile);
		close("rm " tmpfile);
		close("mktemp -u");
		close(cmd " >" tmpfile "&");
	}

	t = Tg_cmdres[cmd];

	if (Table[t,"rows"] == 0 || Table[t,"cols"] == 0)
		return tg_newval("", TG_EMPTY);

	return tg_newval(t, TG_TABLE);
}

function tg_args(val, args)
{
	if (Tg_val[val,"type"] == TG_SCRIPT)
		return tg_script(val, args);

	return tg_func(val, args);
}

function tg_val(v,	type, addr, i, id, val, t, s)
{
	type = node_type(v);
	if (type == TP_N_EXPR)
		return tg_expr(v);
	else if (type == TP_N_SIGN) {
		t = tg_typeprom1(tg_val(node_child(v, 2)), TG_FLOAT);

		s = node_val(node_child(v, 1));

		return tg_newval((s == "-") \
			? -Tg_val[t] : Tg_val[t], Tg_val[t,"type"]);
	}
	else if (type == TP_N_NOT) {
		t = tg_val(node_child(v,1));

		return tg_newval(tg_istrue(t) ? 0 : 1, TG_INT);
	}
	else if (type == TP_N_ADDRESS) {
		id = node_val(node_child(v, 1));

		val = tg_accessvar(id);

		if (Tg_val[val,"type"] == TG_TEMPLATE)
			val = tg_template(tp_template(Tg_val[val]));

		assert(val >= 0, "Variable " id " is not defined.");

		for (i = 2; i <= node_childrencount(v); ++i) {
			type = node_type(node_child(v, i));

			if (type == TP_N_ARGS)
				val = tg_args(val, node_child(v, i));
			else if (type == TP_N_INDEX)
				val = tg_index(val, node_child(v, i));
			else if (type == TP_N_ATTR)
				val = tg_attr(val, node_child(v,i));
		}

		assert(Tg_val[val,"type"] != TG_FUNCTION,
			"Id " id "is a function. Cannot treat "\
			"it like variable.");	

		return val;
	}
	else if (type == TP_T_INT)
		return tg_newval(node_val(v) + 0, TG_INT);
	else if (type == TP_T_FLOAT)
		return tg_newval(node_val(v) + 0, TG_FLOAT);
	else if (type == TP_T_STRING)
		return tg_newval(node_val(v), TG_STRING);
}

function tg_defs(n, i, c, name)
{
	for (i = 1; i <= node_childrencount(n); ++i) {
		c = node_child(n, i);

		name = node_val(node_child(c, 1));

		assert(tg_accessvar(name) < 0,
			"Cannot define user function \"" name "\". " \
			"Such name is already used.");

		Tg_var[0,name] = tg_newval(c, TG_FUNCTION);
	}
}

function tg_template(n,		i, c, r)
{
	tg_defs(node_child(n, 1));

	r = tg_block(node_child(n, 2));

	return tg_cast(r, TG_TABLE);
}

function tg_readsources(sourcestr,
	i, s, a, l, t, name, type, path, scount)
{
	scount = split(sourcestr, s, ";");

	for (i = 1; i <= scount; ++i) {
		split(s[i], a, ":");

		name = a[1];
		type = a[2];
		path = a[3];
		if (type == "text") {
			t = "";
			while (getline l < path)
				t = t "\n" l;
			t = substr(t, 2);

			Tg_var[0,name] = tg_newval(t, TG_STRING);
		}
		else if (type == "csv") {
			Tg_var[0,name] = tg_newval( \
				ta_csvtotable(path, Tg_defspan),
				TG_TABLE);	
		}
		else if (type == "script")
			Tg_var[0,name] = tg_newval(path, TG_SCRIPT);
		else if (type == "table")
			Tg_var[0,name] = tg_newval(path, TG_TEMPLATE);
	}
}

BEGIN {
	Tg_stacktop = Tg_depth = 0;
	Tg_valcount = 0;

	TG_EMPTY = 0;		TG_FUNCTION = 1;	TG_SCRIPT = 2;
	TG_INT = 3;		TG_FLOAT = 4;		TG_STRING = 5;
	TG_TABLE = 6;		TG_TEMPLATE = 7;

	TG_ISODATE = 1;		TG_RUDATE = 2;		TG_USDATE = 3;

	TG_BI_WAITCMD = -26;	TG_BI_DATEADD = -25;
	TG_BI_WEEKDAY = -24;	TG_BI_RUDATE = -23;
	TG_BI_INT = -22;	TG_BI_FLOAT = -21;
	TG_BI_STRING = -20;	TG_BI_TABLE = -19;
	TG_BI_DATECMP = -18;	TG_BI_DATEDIFF = -17;
	TG_BI_YEAR = -16;	TG_BI_MONTH = -15;
	TG_BI_DAY = -14;	TG_BI_MATCH = -13;
	TG_BI_CAT = -12;	TG_BI_SUBSTR = -11;
	TG_BI_LENGTH = -10;	TG_BI_NEXTTO = -9;
	TG_BI_DEFVAL = -8;	TG_BI_CMIN = -7;
	TG_BI_RMIN = -6;	TG_BI_CMAX = -5;
	TG_BI_RMAX = -4;	TG_BI_CSUM = -3;
	TG_BI_RSUM = -2;	TG_BI_UNIQ = -1;

	TG_SUCCESS = 0;		TG_BREAK = -1;		TG_CONTINUE = -2;

	Tg_var[0,"int"] = tg_newval(TG_BI_INT, TG_FUNCTION);
	Tg_var[0,"float"] = tg_newval(TG_BI_FLOAT, TG_FUNCTION);
	Tg_var[0,"string"] = tg_newval(TG_BI_STRING, TG_FUNCTION);
	Tg_var[0,"table"] = tg_newval(TG_BI_TABLE, TG_FUNCTION);
	Tg_var[0,"datecmp"] = tg_newval(TG_BI_DATECMP, TG_FUNCTION);
	Tg_var[0,"datediff"] = tg_newval(TG_BI_DATEDIFF, TG_FUNCTION);
	Tg_var[0,"year"] = tg_newval(TG_BI_YEAR, TG_FUNCTION);
	Tg_var[0,"month"] = tg_newval(TG_BI_MONTH, TG_FUNCTION);
	Tg_var[0,"day"] = tg_newval(TG_BI_DAY, TG_FUNCTION);
	Tg_var[0,"match"] = tg_newval(TG_BI_MATCH, TG_FUNCTION);
	Tg_var[0,"cat"] = tg_newval(TG_BI_CAT, TG_FUNCTION);
	Tg_var[0,"substr"] = tg_newval(TG_BI_SUBSTR, TG_FUNCTION);
	Tg_var[0,"length"] = tg_newval(TG_BI_LENGTH, TG_FUNCTION);
	Tg_var[0,"nextto"] = tg_newval(TG_BI_NEXTTO, TG_FUNCTION);
	Tg_var[0,"defval"] = tg_newval(TG_BI_DEFVAL, TG_FUNCTION);
	Tg_var[0,"monthend"] = tg_newval(TG_BI_MONTHEND, TG_FUNCTION);
	Tg_var[0,"rudate"] = tg_newval(TG_BI_RUDATE, TG_FUNCTION);
	Tg_var[0,"weekday"] = tg_newval(TG_BI_WEEKDAY, TG_FUNCTION);
	Tg_var[0,"dateadd"] = tg_newval(TG_BI_DATEADD, TG_FUNCTION);

	Tg_var[0,"EMPTY"] = tg_newval("", TG_EMPTY);

	Tg_defspan["hspan"] = Tg_defspan["vspan"] = 1;

	tg_readsources(A_sources);

	tg_outproc(Tg_val[tg_template(tp_template("/dev/stdin"))],
		A_outtype);
}
