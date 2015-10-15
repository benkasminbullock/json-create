typedef enum {
    json_create_ok,
    /* Unknown Perl svtype within the structure. */
    json_create_unknown_type,
}
json_create_status_t;

typedef struct {

}
json_create_t;

static int
perl_error_handler (const char * file, int line_number, const char * msg, ...)
{
    va_list args;
    va_start (args, msg);
    vcroak (msg, & args);
    va_end (args);
    return 0;
}

/* Print an error to stderr. */

static int
json_create_error_handler_default (const char * file, int line_number, const char * msg, ...)
{
    int printed;
    va_list vargs;
    va_start (vargs, msg);
    printed = 0;
    printed += fprintf (stderr, "%s:%d: ", file, line_number);
    printed += vfprintf (stderr, msg, vargs);
    printed += fprintf (stderr, "\n");
    va_end (vargs);
    return printed;
}

static int (* json_create_error_handler) (const char * file, int line_number, const char * msg, ...) = json_create_error_handler_default;

#define JCEH json_create_error_handler
#define ADD_LITERAL(x,y) sv_catpvn (x, #y, (STRLEN) strlen (#y))

#define CALL(x) {						\
	json_create_status_t status;				\
	status = x;						\
	if (status != json_create_ok) {				\
	    if (JCEH) {						\
		(*JCEH) (__FILE__, __LINE__,			\
			 "call to %s failed with status %d",	\
			 #x, status);				\
	    }							\
	    return status;					\
	}							\
    }

static json_create_status_t json_create_recursively (SV * input, SV * output);

static json_create_status_t
json_create_add_string (SV * input, SV * output)
{

    return json_create_ok;
}

#define ADD_CHAR(output, c) sv_catpvn (output, 

static json_create_status_t
json_create_add_object (HV * input_hv, SV * output)
{
    I32 n_keys;
    int i;
    int key_n;
    HE * entry;
    const char * comma = "{";
    const char * key;
    I32 keylen;

    n_keys = hv_iterinit (input_hv);
    for (i = 0; i < n_keys; i++) {
	entry = hv_iternext (input_hv);
	comma = ",";
    }
    ADD_CHAR (output, '}');
    return json_create_ok;
}

static json_create_status_t
json_create_recursively (SV * input, SV * output)
{
    /* The SV type of input. */
    svtype is;

    if (! SvOK (input)) {
	ADD_LITERAL (output, null);
	return json_create_ok;
    }
    is = SvTYPE (input);
    if (SvROK (input)) {
	SV * r = SvRV (input);
	is = SvTYPE (r);
	switch (is) {
	case SVt_PVAV:
	    break;
	case SVt_PVHV:
	    CALL (json_create_add_object ((HV *) r, output));
	    break;
	case SVt_PVCV:
	    break;
	case SVt_PVGV:
	    break;
	default:
	    if (JCEH) {
		(*JCEH) (__FILE__, __LINE__, "Unknown Perl type %d in switch",
			 is);
	    }
	    return json_create_unknown_type;
	}
    }
    else {
	switch (is) {

	case SVt_NULL:
	    ADD_LITERAL (output, null);
	    return json_create_ok;

	case SVt_IV:
	    sv_catpvf (output, "%ld", SvIV (input));
	    return json_create_ok;

	case SVt_PV:
	    CALL (json_create_add_string (input, output));
	    return json_create_ok;

	default:
	    if (JCEH) {
		(*JCEH) (__FILE__, __LINE__, "Unknown Perl type %d in switch",
			 is);
	    }
	    return json_create_unknown_type;
	}
    }
    return json_create_ok;
}

static SV *
json_create (SV * input)
{
    SV * output = newSVpvn ("", 0);
    json_create_status_t status;
    status = json_create_recursively (input, output);
    if (status != json_create_ok) {
	/* Free the memory of "output". */
	SvREFCNT_dec (output);
	/* return undef; */
	return & PL_sv_undef;
    }
    return output;
}

