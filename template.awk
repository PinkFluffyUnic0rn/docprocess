# Global
#	Source
#	Sourcename
#	Sourcecount
#	Tmp
#	Tmpcount

function sqltoscript(file,	l, r, ll, d, n)
{
	++Tmpcount
	filenew = Tmp[Tmpcount] \
		= sprintf("/tmp/%s.%s", Tmpcount, PROCINFO["pid"]);

	print "#!/bin/sh\n\n" >filenew;
	print "echo 'set list on;\n" >filenew;
	
	while ((r = getline l <file) > 0) {
		gsub("'", "'\"'\"'", l);

		ll = "";
		while ((d = index(l, "$"))) {
			assert(n = match(substr(l, d + 1), /^[0-9]+/),
				"Argument addressing without number: " \
				l);
			
			ll = ll substr(l, 1, d - 1) \
				"'$" substr(l, d + 1, RLENGTH) + 3 "'";
			
			l = substr(l, d + RLENGTH + 1);
		}

		print ll l >filenew;
	}
	
	printf "' | %s/isqlreq \"$1\" \"$2\" \"$3\" | %s/sqltocsv",
		A_utilspath, A_utilspath >filenew;
	
	close(file);
	close(filenew);

	return filenew;
}

function sourcesubst(file,	cmd, filenew)
{
	++Tmpcount
	filenew = Tmp[Tmpcount] \
		= sprintf("/tmp/%s.%s", Tmpcount, PROCINFO["pid"]);
	
	system(sprintf("LC_ALL=C cat %s | awk -f %s/error.awk \
		-f %s/substitute.awk -f %s/sources.awk \
		-v 'A_scriptpath=%s' -v 'A_source=%s' \
		-v 'A_tablegenpath=%s' -v 'A_utilspath=%s' >%s",
		file, A_scriptpath, A_scriptpath, A_scriptpath,
		A_scriptpath, wrapsources(), A_tablegenpath,
		A_utilspath, filenew));
	
	close(filenew)

	return filenew;
}

function source(l, 	name, type, s, file, i, a, c, r, ll, subst)
{
	gsub(/^[ \t]+/, "", l);
	gsub(/[ \t]+$/, "", l);
	gsub(/[ \t]+/, " ", l);

	subst = 1;
	
	if ((s = index(l, ":")) != 0) {
		if (substr(l, s + 1, 1) != ":")
			subst = 0;
		else {
			gsub(/::/, ":", l);
		}

		file = substr(l, s + 1);
		gsub(/^[ \t]+/, "", file);
		gsub(/[ \t]+$/, "", file);

		l = substr(l, 1, s - 1);
		
	}
	else {
		++Tmpcount
		file = Tmp[Tmpcount] = sprintf("/tmp/%s.%s",
			Tmpcount, PROCINFO["pid"]);

		while ((r = getline ll) > 0) {
			if (ll ~ /#end[ \t]*/)
				break;

			print ll >file;
		}

		assert(r != 0, "Unexpected end of source definition " l);
		assert(r > 0,  "Error while reading source definition" l);

		close(file);
	}
	c = split(l, a, " ");
	
	for (i in a) {
		assert(a[i] ~ /^[a-zA-Z][a-zA-Z0-9]*$/,
			"Wrong identificator name: " a[i]);
	}

	if (a[1] == "document") {
		assert(c == 1, a[1] ": Wrong number of arguments");

		type = name = a[1];

		if (subst) file = sourcesubst(file);
	}
	else if (a[1] == "input") {
		assert(c == 3, a[1] ": Wrong number of arguments");
		
		name = a[2];

		if (a[3] == "sql") {
			a[3] = "script";
			file = sqltoscript(file);		
		}

		type = a[3];
	
		if ((a[3] == "text" || a[3] == "script") && subst) {
			file = sourcesubst(file);
			if (a[3] == "script") {
				assert(system("chmod u+x " file) == 0,
					"cannot make " file " executable.");	
			}
		}
	}
	else if (a[1] == "table") {
		assert(c == 2, a[1] ": Wrong number of arguments");
		
		name = a[2];
		
		type = "table";
	}
	else
		error("Unknown input type: " a[1]);	

	Sourcename[Sourcecount] = name;
	Source[name,"type"] = type;
	Source[name,"path"] = file;

	++Sourcecount;
}

BEGIN {
	Sourcecount = 0;
	unwrapsources();
	
	while ((r = getline l) > 0) {
		if (l ~ /^[ \t]*$/)
			continue;

		assert(substr(l, 1, 1) == "#",
			"Text outside definition");

		l = substr(l, 2);

		source(l);
	}

	assert(r >= 0, "Error while reading template ");

	system("cat " Source["document","path"]);

	exit(0);
}

END {
	for (i in Tmp)
		system("rm " Tmp[i]);
}
