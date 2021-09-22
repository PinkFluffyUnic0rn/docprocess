# Global:
#	File_nextchar []
#	File_line []

function file_peekchar(file)
{
	return File_nextchar[file];
}

function file_getchar(file,	c)
{
	if (File_nextchar[file] == "")
		File_nextchar[file] = file_readchar(file);
	
	c = File_nextchar[file];
	File_nextchar[file] = file_readchar(file);

	return c;
}

function file_close(file)
{
	File_line[file] = "";
	File_nextchar[file] = "";
	
	close(file);
}

# getc() immitation. For internal use.
function file_readchar(file,	c, r)
{
	if (File_line[file] == "") {
		if ((r = (getline File_line[file] <file)) == 0)
			return "";
		
		assert(r > 0, "Error while reading .csv file: " file);
	
		File_line[file] = File_line[file] "\n";
	}

	c = substr(File_line[file], 1, 1);
	File_line[file] = substr(File_line[file], 2);

	return c;
}
