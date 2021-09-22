function tg_htmlout(t,	i, j, row, val)
{
	print("<table>\n");
	for (i = 1; i <= Table[t,"rows"]; ++i) {
		row = "";
		for (j = 1; j <= Table[t,"cols"]; ++j) {
			if (Table[t,i,j,"hspan"] < 0 \
				|| Table[t,i,j,"vspan"] < 0)
				continue;

			val = Table[t,i,j,"val"];
			
			gsub(/\&/, "\\&amp;", val);
			gsub(/\"/, "\\&quot;", val);
#			gsub(/\%/, "\\&percnt;", val);
			gsub(/'/, "\\&apos;", val);
#			gsub(/\+/, "\\&add;", val);
			gsub(/=/, "\\&equal;", val);
			gsub(/</, "\\&lt;", val);
			gsub(/>/, "\\&gt;", val);
			gsub(/\n/, "<br>", val);

			row = row sprintf("\
			<td rowspan=\"%d\" colspan=\"%d\">",
				Table[t,i,j,"vspan"],
				Table[t,i,j,"hspan"]);

		#	if (Table[t,i,j,"type"] == TA_FLOAT)
		#		row = row sprintf("%f", val);
		#	else
				row = row sprintf("%s", val);
		
			row = row sprintf("</td>\n");
		}
		if (row != "") {
			printf("\
		<tr>\n%s\
		</tr>\n",
			row);
		}

	}
	printf("</table>\n");
}

function tg_outproc(t, format)
{
	if (format == "html")
		tg_htmlout(t);
	else if (format == "csv")
		csv_write(t, "/dev/stdout");
	else if (format == "txt") {
		assert(Table[t,"rows"] == 1 && Table[t,"cols"] == 1,
			"Only tables with single cell " \
			"can be converted to txt");

		printf("%s", Table[t,1,1,"val"]) >"/dev/stdout";
	}
	else {
		printf("Unknown file extension: %s.", format);
		exit;	
	}
}
