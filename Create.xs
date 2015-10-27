/* XS part of JSON::Create. */

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "ppport.h"

#include "unicode.h"
#include "json-create-perl.c"

#define PERLJCCALL(x) {					\
	json_create_status_t jcs;			\
	jcs = x;					\
	if (jcs != json_create_ok) {			\
	    warn ("%s:%d: bad status %d from %s",	\
		  __FILE__, __LINE__, jcs, #x);		\
	}						\
    }

typedef json_create_t * JSON__Create;

MODULE=JSON::Create PACKAGE=JSON::Create

PROTOTYPES: DISABLE

SV *
create_json (input)
	SV * input;
CODE:
	RETVAL = json_create (input);
OUTPUT:
	RETVAL

void
DESTROY (jc)
	JSON::Create jc;
CODE:
	PERLJCCALL (json_create_free (jc));

JSON::Create
new (class, ...)
	char * class;
CODE:
	PERLJCCALL (json_create_new (& RETVAL));
OUTPUT:
	RETVAL

SV *
run (jc, input)
	JSON::Create jc;
	SV * input
CODE:
	RETVAL = json_create_run (jc, input);
OUTPUT:
	RETVAL

void
set_fformat (jc, fformat)
	JSON::Create jc;
	SV * fformat;
CODE:
	PERLJCCALL (json_create_set_fformat (jc, fformat));
OUTPUT:
