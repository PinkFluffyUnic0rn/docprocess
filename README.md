# docprocess

Document generating language interpretor

## Installation

   To run the terminal interpretor you suppose to have AWK and
Shell interpretors in your system. Most of UNIX-like systems have them
by default. To start work with this interpretator you need to copy
this repo's content to any directory in your system.

## Installation for debian/ubuntu

   For debian based systems this repo has a .deb package builder. To run
it you can go to repo's directory and run:

	make deb

After this you can install the package:

	dpkg -i docprocess.deb

## How to run the interpretor

	./template [template] [format] [parameters]

, where [template] -- the path to a template, [format] -- an output
format, and [parameters] -- a parameters list separated by ';'
character. The path to a template can be absolute or relative. An output
format for now can be one of two: html or xlsx. Notice that the xlsx
can be used only for tables.

   Every parameter is considered as separate syntax construction
(see substitution language manual) and can be set this way:

	[name]:[type]:[path]

, where [name] -- a name of a construction, [type] -- the type of a
construction, and [path] -- the path to a file that contains the body of
a contruction.

   For example, to process the template 'bankrejections.txt', that
placed in 'template' directory giving it additional parameters
'datefrom' and 'dateto' you need to run the list of shell commands:

	echo '01.01.2019' >/tmp/datefrom.$$
	echo '30.11.2019' >/tmp/dateto.$$

	./template 'template/bankrejections.txt' 'html' \
		"datefrom:text:/tmp/datefrom.$$;dateto:text:/tmp/dateto.$$" \
		> result.html

   Most of templates need at least one parameter. Usualy such
parameters are borders of a report's period and list of identificators
of servers you ask for information.
