function csv_getrecord(file, a,	asize, quoted, prevtock, tock)
{
	asize = 1;
	a[asize] = "";
	while ((tock = csv_nexttocken(file)) != "\4") {
		if (tock == ";")
			a[++asize] = "";
		else if (tock == "\n")
			return asize;
		else
			a[asize] = a[asize] tock;
	}

	return (-1);
}

function csv_getrecorda(file, fieldname, a,	asize, quoted, prevtock, tock)
{
	asize = 1;
	a[fieldname[asize]] = "";
	while ((tock = csv_nexttocken(file)) != "\4") {
		if (tock == ";")
			a[fieldname[++asize]] = "";
		else if (tock == "\n")
			return asize;
		else
			a[fieldname[asize]] = a[fieldname[asize]] tock;
	}

	return (-1);
}

function csv_isspecial(c)
{
	return (c == ";" || c == "\n" || c == "\"")
}

# get next tocken. Tokens are ';', '\n', '"'
# and words beetween two of them
function csv_nexttocken(file,	c, cc, word, nextc, tmp)
{
	c = file_getchar(file);

	if (c == "")
		return "\4";
	if (c == "\"") {
		while ((cc = file_getchar(file))) {
			if (cc == "\"") {
				if (file_peekchar(file) == "\"") 
					word = word file_getchar(file);
				else if (file_peekchar(file) == "")
					return "\4";
				else
					return word;
			}
			else
				word = word cc;
		}
	}
	else if (csv_isspecial(c))
		return c;
	else {
		word = c;

		while (!csv_isspecial(file_peekchar(file)) \
			&& ((cc = file_getchar(file)) != ""))
			word = word cc;

		return word;
	}

	return "\4";
}

function csv_write(t, path, i, j, v)
{
	for (i = 1; i <= Table[t,"cols"]; ++i) {
		printf("%s%c", Table[t,"colname",i],
			(i == Table[t,"cols"]) ? "\n" : ";") > path;
	}

	for (i = 1; i <= Table[t,"rows"]; ++i ) {
		for (j = 1; j <= Table[t,"cols"]; ++j) {
			v = Table[t,i,j,"val"];

			gsub("\"", "\"\"", v);
			
			if (index(v, ";"))
				v = "\"" v "\"";

			printf("%s%c", v,
				(j == Table[t,"cols"]) ? "\n" : ";") \
				> path;
		}
	}

}
