typedef enum {
    json_create_ok,
    /* Unknown Perl svtype within the structure. */
    json_create_unknown_type,
    json_create_bad_char,
    json_create_unicode_too_big,
}
json_create_status_t;

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

#define BUFSIZE 0x4000

typedef struct json_create {
    /* The size we have to use before we write the buffer out. */
    int size;
    /* The length of the input string. */
    int length;
    unsigned char buffer[BUFSIZE];
    /* Place to write the buffer to. */
    SV * output;
}
json_create_t;

static json_create_status_t
json_create_recursively (json_create_t * jc, SV * input);

/* Copy the jc buffer into its SV. */

static inline json_create_status_t
json_create_buffer_fill (json_create_t * jc)
{
    /* There is nothing to put in the output. */
    if (jc->length == 0) {
	if (jc->output == 0) {
	    /* And there was not anything before either. */
	    jc->output = & PL_sv_undef;
	}
	/* Either way, we don't need to do anything more. */
	return json_create_ok;
    }
    if (! jc->output) {
	jc->output = newSVpvn ((char *) jc->buffer, (STRLEN) jc->length);
    }
    else {
	sv_catpvn (jc->output, (char *) jc->buffer, (STRLEN) jc->length);
    }
    jc->length = 0;
    return json_create_ok;
}

/* Add one character to the end of jc. */

static inline json_create_status_t
add_char (json_create_t * jc, unsigned char c)
{
//    fprintf (stderr, "Adding %c\n", c);
    jc->buffer[jc->length] = c;
    jc->length++;
    if (jc->length >= jc->size) {
	CALL (json_create_buffer_fill (jc));
    }
    return json_create_ok;
}

static inline json_create_status_t
add_str (json_create_t * jc, const char * s)
{
    int i;
    i = 0;
    while (1) {
	unsigned char c;
	if (c == 0) {
	    return json_create_ok;
	}
	c = (unsigned char) s[i];
	CALL (add_char (jc, c));
	i++;
    }
    return json_create_ok;
}

static inline json_create_status_t
add_str_len (json_create_t * jc, const char * s, unsigned int slen)
{
    int i;
    for (i = 0; i < slen; i++) {
	unsigned char c;
	c = (unsigned char) s[i];
	CALL (add_char (jc, c));
    }
    return json_create_ok;
}

#define ADD(x) CALL (add_str_len (jc, x, strlen (x)));

static inline json_create_status_t
add_u (json_create_t * jc, unsigned int u)
{
    char hex[5];
    ADD ("\\u");
    if (u > 0xffff) {
	return json_create_unicode_too_big;
    }
    snprintf (hex, 4, "%04u", u);
    CALL (add_str_len (jc, hex, 4));
    return json_create_ok;
}

static inline json_create_status_t
json_create_add_key_len (json_create_t * jc, const unsigned char * key, STRLEN keylen)
{
    int i;
    char * istring;
    CALL (add_char (jc, '"'));
    for (i = 0; i < keylen; i++) {
	unsigned char c;
	c = key[i];
	if (c < 0x20) {
	    if (c == '\t') {
		ADD ("\\t");
	    }
	    else if (c == '\n') {
		ADD ("\\n");
	    }
	    else if (c == '\r') {
		ADD ("\\r");
	    }
	    else if (c == '\b') {
		ADD ("\\b");
	    }
	    else if (c == '\f') {
		ADD ("\\f");
	    }
	    else {
		CALL (add_u (jc, c));
	    }
	}
	else {
	    if (c == '"') {
		ADD ("\\\"");
	    }
	    else if (c == '\\') {
		ADD ("\\\\");
	    }
	    else {
		CALL (add_char (jc, c));
	    }
	}
    }
    CALL (add_char (jc, '"'));
    return json_create_ok;
}

static json_create_status_t
json_create_add_string (json_create_t * jc, SV * input)
{
    int i;
    char * istring;
    STRLEN ilength;
    istring = SvPV (input, ilength);
    return json_create_add_key_len (jc, (unsigned char *) istring, (STRLEN) ilength);
}

static inline json_create_status_t
json_create_add_integer (json_create_t * jc, SV * sv)
{
    long int iv;
    STRLEN ivlen;
    char ivbuf[0x40];
    iv = SvIV (sv);
    ivlen = snprintf (ivbuf, 0x40, "%ld", iv);
    CALL (add_str_len (jc, ivbuf, ivlen));
    return json_create_ok;
}

static inline json_create_status_t
json_create_add_float (json_create_t * jc, SV * sv)
{
    double fv;
    STRLEN fvlen;
    char fvbuf[0x40];
    fv = SvNV (sv);
    fvlen = snprintf (fvbuf, 0x40, "%g", fv);
    CALL (add_str_len (jc, fvbuf, fvlen));
    return json_create_ok;
}

static inline json_create_status_t
json_create_add_object (json_create_t * jc, HV * input_hv)
{
    I32 n_keys;
    int i;
    SV * value;
    char * key;
    I32 keylen;

    CALL (add_char (jc, '{'));
    n_keys = hv_iterinit (input_hv);
    for (i = 0; i < n_keys; i++) {
	if (i > 0) {
	    CALL (add_char (jc, ','));
	}
	value = hv_iternextsv (input_hv, & key, & keylen);
	CALL (json_create_add_key_len (jc, (const unsigned char *) key,
				       (STRLEN) keylen));
	CALL (add_char (jc, ':'));
	CALL (json_create_recursively (jc, value));
    }
    CALL (add_char (jc, '}'));
    return json_create_ok;
}

static inline json_create_status_t
json_create_add_array (json_create_t * jc, AV * av)
{
    I32 n_keys;
    int i;
    SV * value;

    CALL (add_char (jc, '['));
    n_keys = av_top_index (av) + 1;
    for (i = 0; i < n_keys; i++) {
	if (i > 0) {
	    CALL (add_char (jc, ','));
	}
	value = * (av_fetch (av, i, 0 /* don't delete the array value */));
	CALL (json_create_recursively (jc, value));
    }
    CALL (add_char (jc, ']'));
    return json_create_ok;
}

static json_create_status_t
json_create_recursively (json_create_t * jc, SV * input)
{
    /* The SV type of input. */
    svtype is;

    if (! SvOK (input)) {
	CALL (add_str (jc, "null"));
	return json_create_ok;
    }
    is = SvTYPE (input);
    if (SvROK (input)) {
	SV * r = SvRV (input);
	is = SvTYPE (r);

	switch (is) {

	case SVt_PVAV:
	    CALL (json_create_add_array (jc, (AV *) r));
	    break;

	case SVt_PVHV:
	    CALL (json_create_add_object (jc, (HV *) r));
	    break;

	case SVt_PV:
	    CALL (json_create_add_string (jc, r));
	    break;

	case SVt_IV:
	    CALL (json_create_add_integer (jc, r));
	    break;

	case SVt_NV:
	    CALL (json_create_add_float (jc, r));
	    break;

	case SVt_PVGV:
	case SVt_PVCV:
	case SVt_REGEXP:
	    /* Use it as a string. */
	    CALL (json_create_add_string (jc, r));
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
	    CALL (add_str (jc, "null"));
	    return json_create_ok;

	case SVt_PV:
	    CALL (json_create_add_string (jc, input));
	    return json_create_ok;

	case SVt_IV:
	    CALL (json_create_add_integer (jc, input));
	    break;

	case SVt_NV:
	    CALL (json_create_add_float (jc, input));
	    break;

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

#define FINALCALL(x) {						\
	json_create_status_t status;				\
	status = x;						\
	if (status != json_create_ok) {				\
	    fprintf (stderr,					\
		     "%s:%d: %s failed with status %d\n",	\
		     __FILE__, __LINE__, #x, status);		\
	    /* Free the memory of "output". */			\
	    if (jc.output) {					\
		SvREFCNT_dec (jc.output);			\
	    }							\
	    /* return undef; */					\
	    return & PL_sv_undef;				\
	}							\
    }

static SV *
json_create (SV * input)
{
    json_create_t jc;

    jc.length = 0;
    jc.size = BUFSIZE - 0x10;
    jc.output = 0;

    FINALCALL (json_create_recursively (& jc, input));
    FINALCALL (json_create_buffer_fill (& jc));
    return jc.output;
}

