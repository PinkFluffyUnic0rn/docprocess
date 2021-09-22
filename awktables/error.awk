function error(msg)
{
	print msg >"/dev/stderr";
	exit(1);
}

function warning(msg)
{
	print msg >"/dev/stderr";
}

function assert(cond, msg)
{
	if (!cond)
		error(msg);
}
