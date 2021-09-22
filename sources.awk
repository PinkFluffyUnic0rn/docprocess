function unwrapsources(	a, aa, i, scount)
{
	scount = split(A_source, a, ";");

	for (i = 1; i <= scount; ++i) {
		split(a[i], aa, ":");
	
		Sourcename[Sourcecount++] = aa[1];
		Source[aa[1],"type"] = aa[2];
		Source[aa[1],"path"] = aa[3];
	}
}

function wrapsources(	i, wi)
{
	for (i = 0; i < Sourcecount; ++i) {
		wi = wi sprintf("%s:%s:%s;",
			Sourcename[i],
			Source[Sourcename[i],"type"],
			Source[Sourcename[i],"path"]);
	}
	
	gsub(/;$/, "", wi);

	return wi;
}
