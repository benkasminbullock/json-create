/* XS part of JSON::Create. */

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "ppport.h"

#include "unicode.h"
#include "json-create-perl.c"

MODULE=JSON::Create PACKAGE=JSON::Create

PROTOTYPES: DISABLE

SV *
create_json (input)
	SV * input;
PREINIT:
CODE:
	RETVAL = json_create (input);
OUTPUT:
	RETVAL
