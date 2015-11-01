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
set_fformat_unsafe (jc, fformat)
	JSON::Create jc;
	SV * fformat;
CODE:
	PERLJCCALL (json_create_set_fformat (jc, fformat));
OUTPUT:

void
escape_slash (jc, onoff)
	JSON::Create jc;
	SV * onoff;
CODE:
	jc->escape_slash = SvTRUE (onoff) ? 1 : 0;

void
unicode_upper (jc, onoff)
	JSON::Create jc;
	SV * onoff;
CODE:
	jc->unicode_upper = SvTRUE (onoff) ? 1 : 0;

void
unicode_escape_all (jc, onoff)
	JSON::Create jc;
	SV * onoff;
CODE:
	jc->unicode_escape_all = SvTRUE (onoff) ? 1 : 0;

void
set_validate (jc, onoff)
	JSON::Create jc;
	SV * onoff;
CODE:
	jc->validate = SvTRUE (onoff) ? 1 : 0;

void
no_javascript_safe (jc, onoff)
	JSON::Create jc;
	SV * onoff;
CODE:
	jc->no_javascript_safe = SvTRUE (onoff) ? 1 : 0;

void
set_handlers (jc, handlers)
	JSON::Create jc
	HV * handlers
CODE:
        PERLJCCALL (json_create_remove_handlers (jc));
	SvREFCNT_inc ((SV*) handlers);
	jc->n_mallocs++;
	jc->handlers = handlers;
OUTPUT:

HV *
get_handlers (jc)
	JSON::Create jc
CODE:
	if (! jc->handlers) {
		jc->handlers = newHV();
		jc->n_mallocs++;
	}
	RETVAL = jc->handlers;
OUTPUT:
	RETVAL

void
code_ref_handler (jc, crh)
	JSON::Create jc;
	SV * crh;
CODE:
	if (SvTRUE (crh)) {
	    jc->code_ref_handler = crh;
	    SvREFCNT_inc (crh);
	    jc->n_mallocs++;
	}
	else {
	    PERLJCCALL (json_create_remove_code_handler (jc));
	}
