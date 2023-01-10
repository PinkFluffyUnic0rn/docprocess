function substitute(s, of,	idx, r)
{
	assert(Source[s,"type"] != "",
		"Datasource " s " does not exists.");

	of = (of == "") ? "html" : of

	if (Source[s,"type"] == "table") {
		cmd = sprintf("%s '%s/include/week.tg;%s/include/aggregate.tg;%s' '%s' '%s'",
			A_tablegenpath, A_scriptpath, A_scriptpath,
			Source[s,"path"], wrapsources(), of);
	}
	else if (Source[s,"type"] == "text")
		cmd = "cat " Source[s,"path"] " | head -c -1";

	assert(system(cmd) == 0, "Subtitution failed.");
}

BEGIN {
	Dscount = 0;
	unwrapsources();

	while ((r = getline l) > 0) {
		Line[NR] = l;
	
		while (l != "") {
			if (substr(l, 1, 1) == "\\") {
				printf("%c", substr(l, 2, 1));
				l = substr(l, 3);
			}
			else if (substr(l, 1, 1) == "*") {
				l = substr(l, 2);

				if (substr(l, 1, 1) == "{") {
					len = match(l, /}/);

					s = index(substr(l, 1, len), ":");
					
					subsname = substr(l, 2, (s ? s : len) - 2);
					outformat = substr(l, s + 1, s ? len - (s + 1) : 0);
				}
				else {
					match(l, /^[a-zA-Z]+[a-zA-Z0-9]*(:[a-z]+)?/);
					len = RLENGTH;

					s = index(substr(l, 1, len), ":");
					
					subsname = substr(l, 1, (s ? s - 1 : len));
					outformat = substr(l, s + 1, s ? len - s : 0);
				}

				substitute(subsname, outformat);
					
				l = substr(l, len + 1);
			}
			else {
				printf("%c", substr(l, 1, 1));
				l = substr(l, 2);
			}
		}

		print;
	}

	assert(r >= 0, "Error while reading template ");

	exit(0);
}
