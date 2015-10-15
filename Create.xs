#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "ppport.h"

#include "json-create-perl.c"

typedef json_create_t * JSON__Create;

MODULE=JSON::Create PACKAGE=JSON::Create

PROTOTYPES: DISABLE

BOOT:
	/* JSON__Create_error_handler = perl_error_handler; */

