# Global:
#	Node []
#	Nodescount
#	Strsym[] should be defined for error handling

function node_add(parent, nodetype, tval, tline,	n)
{
	n = ++Nodescount;
	
	Node[n,"parent"] = parent;
	Node[n,"type"] = nodetype;
	Node[n,"val"] = tval;
	Node[n,"line"] = tline;
	Node[n,"childrencount"] = 0; 

	Node[parent,"childrencount"] = Node[parent,"childrencount"] + 1; 
	Node[parent,"child",Node[parent,"childrencount"]] = n;

	return n;
}

function node_settype(n, nodetype)
{
	Node[n,"type"] = nodetype;
}

function node_val(n)
{
	return Node[n,"val"];
}

function node_type(n)
{
	return Node[n,"type"];
}

function node_child(n, c)
{
	return Node[n,"child",c];
}

function node_childrencount(n)
{
	return Node[n,"childrencount"];
}

function node_line(n)
{
	return Node[n,"line"];
}

function node_print(n, t,	i)
{
	for (i = 1; i <= t; ++i) printf("|\t");

#!! Tp_strsym

	printf("node %d: toktype = \"%s\", tokval = \"%s\", tokline = %d",
		n, Tp_strsym[Node[n,"type"]], Node[n,"val"], Node[n,"line"]);

	if (Node[n,"childrencount"]) {
		printf(" {\n");
		for (i = 1; i <= Node[n,"childrencount"]; ++i)
			node_print(Node[n,"child",i], t + 1);

		for (i = 1; i <= t; ++i) printf("|\t");

		printf("}");
	}

	printf("\n");
}

BEGIN {
	Nodescount = 0;
}
