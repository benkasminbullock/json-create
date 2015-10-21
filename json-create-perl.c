#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE
#endif /* __GNUC__ */

typedef enum {
    json_create_ok,
    /* Unknown Perl svtype within the structure. */
    json_create_unknown_type,
    json_create_bad_char,
    /* An error from the unicode.c library. */
    json_create_unicode_error,
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
/* Add a margin of 16 bytes in case some stupid code reads after the
   end of the buffer. */
#define MARGIN 0x10

typedef struct json_create {
    /* The length of the input string. */
    int length;
    unsigned char buffer[BUFSIZE];
    /* Place to write the buffer to. */
    SV * output;
}
json_create_t;

/* Everything else in this file is ordered from callee at the top to
   caller at the bottom, but because of the recursion as we look at
   JSON values within arrays or hashes, we need to forward-declare
   "json_create_recursively". */

static json_create_status_t
json_create_recursively (json_create_t * jc, SV * input);

/* Copy the jc buffer into its SV. */

static INLINE json_create_status_t
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
    /* "Empty" the buffer, we don't bother cleaning out the old
       values, so "jc->length" is our only clue as to the clean/dirty
       state of the buffer. */
    jc->length = 0;
    return json_create_ok;
}

/* Add one character to the end of jc. */

static INLINE json_create_status_t
add_char (json_create_t * jc, unsigned char c)
{
    jc->buffer[jc->length] = c;
    jc->length++;
    /* The size we have to use before we write the buffer out. */
    if (jc->length >= BUFSIZE - MARGIN) {
	CALL (json_create_buffer_fill (jc));
    }
    return json_create_ok;
}

/* Add a nul-terminated string to "jc", up to the nul byte. This
   should not be used unless it's strictly necessary, prefer to use
   "add_str_len" instead. This is not intended to be Unicode-safe, it
   is only to be used for strings which we know are not Unicode (for
   example sprintf'd numbers or something). */

static INLINE json_create_status_t
add_str (json_create_t * jc, const char * s)
{
    int i;
    for (i = 0; s[i]; i++) {
	unsigned char c;
	c = (unsigned char) s[i];
	CALL (add_char (jc, c));
    }
    return json_create_ok;
}

/* Add a string "s" with length "slen" to "jc". This does not test for
   nul bytes, but just copies "slen" bytes of the string.  This is not
   intended to be Unicode-safe, it is only to be used for strings we
   know are not Unicode. */

static INLINE json_create_status_t
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

/* "Add a string" macro, this just saves cut and pasting a string and
   typing "strlen" over and over again. For ASCII values only, not
   Unicode safe. */

#define ADD(x) CALL (add_str_len (jc, x, strlen (x)));


static INLINE json_create_status_t
add_one_u (json_create_t * jc, unsigned int u)
{
    char hex[5];
    ADD ("\\u");
    snprintf (hex, 4, "%04u", u);
    CALL (add_str_len (jc, hex, 4));
    return json_create_ok;
}

/* Add a "\u3000" or surrogate pair if necessary. */

static INLINE json_create_status_t
add_u (json_create_t * jc, unsigned int u)
{
    if (u > 0xffff) {
	unsigned hi;
	unsigned lo;
	int status = unicode_to_surrogates (u, & hi, & lo);
	if (status != UNICODE_OK) {
	    if (JCEH) {
		(*JCEH) (__FILE__, __LINE__,
			 "Error %d making surrogate pairs from %X",
			 status, u);
	    }
	    return json_create_unicode_error;
	}
	CALL (add_one_u (jc, hi));
	/* Backtrace fallthrough. */
	return add_one_u (jc, lo);
    }
    else {
	/* Backtrace fallthrough. */
	return add_one_u (jc, u);
    }
}

/* Add a string to the buffer with quotes around it and escapes for
   the escapables. When Unicode verification is added to the module,
   it will be added here. */

static INLINE json_create_status_t
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
	/* Unicode verification switch statements copied from
	   JSON::Parse will go here. */
    }
    CALL (add_char (jc, '"'));
    return json_create_ok;
}

static INLINE json_create_status_t
json_create_add_string (json_create_t * jc, SV * input)
{
    int i;
    char * istring;
    STRLEN ilength;
    istring = SvPV (input, ilength);
    /* Backtrace fall through, remember to check the caller's line. */
    return json_create_add_key_len (jc, (unsigned char *) istring, (STRLEN) ilength);
}

static INLINE json_create_status_t
json_create_add_integer (json_create_t * jc, SV * sv)
{
    long int iv;
    STRLEN ivlen;
    char ivbuf[0x40];
    iv = SvIV (sv);
    ivlen = snprintf (ivbuf, 0x40, "%ld", iv);
    /* Backtrace fall through, remember to check the caller's line. */
    return add_str_len (jc, ivbuf, ivlen);
}

static INLINE json_create_status_t
json_create_add_float (json_create_t * jc, SV * sv)
{
    double fv;
    STRLEN fvlen;
    char fvbuf[0x40];
    fv = SvNV (sv);
    fvlen = snprintf (fvbuf, 0x40, "%g", fv);
    /* Backtrace fall through, remember to check the caller's line. */
    return add_str_len (jc, fvbuf, fvlen);
}

/* Add a number which is already stringified. This bypasses snprintf
   and just copies the Perl string straight into the buffer. */

static INLINE json_create_status_t
json_create_add_stringified (json_create_t * jc, SV *r)
{
    /* Stringified number. */
    char * s;
    /* Length of "r". */
    STRLEN rlen;
    s = SvPV (r, rlen);
    /* This doesn't backtrace correctly, but the calling routine
       should print out that it was calling "add_stringified", so as
       long as we're careful not to ignore the caller line, it
       shouldn't matter. */
    return add_str_len (jc, s, (unsigned int) rlen);
}

/* Add the comma where necessary. */

#define COMMA					\
    if (i > 0) {				\
	CALL (add_char (jc, ','));		\
    }

/* Given a reference to a hash in "input_hv", recursively process it
   into JSON. */

static INLINE json_create_status_t
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
	COMMA;
	value = hv_iternextsv (input_hv, & key, & keylen);
	CALL (json_create_add_key_len (jc, (const unsigned char *) key,
				       (STRLEN) keylen));
	CALL (add_char (jc, ':'));
	CALL (json_create_recursively (jc, value));
    }
    CALL (add_char (jc, '}'));
    return json_create_ok;
}

/* Given an array reference in "av", recursively process it into
   JSON. */

static INLINE json_create_status_t
json_create_add_array (json_create_t * jc, AV * av)
{
    I32 n_keys;
    int i;
    SV * value;

    CALL (add_char (jc, '['));
    n_keys = av_len (av) + 1;
    /* This deals correctly with empty arrays, since av_len is -1 if
       the array is empty, so we do not test for a valid n_keys value
       before entering the loop. */
    for (i = 0; i < n_keys; i++) {
	COMMA;
	value = * (av_fetch (av, i, 0 /* don't delete the array value */));
	CALL (json_create_recursively (jc, value));
    }
    CALL (add_char (jc, ']'));
    return json_create_ok;
}

/*

Copied from

https://metacpan.org/source/TOBYINK/match-simple-XS-0.001/XS.xs#L11

via

http://grep.cpan.me/?q=SvRX

*/

#ifndef SvRXOK
 
#define SvRXOK(sv) is_regexp(aTHX_ sv)
 
static INLINE int
is_regexp (pTHX_ SV* sv) {
        SV* tmpsv;
         
        if (SvMAGICAL(sv))
        {
                mg_get(sv);
        }
         
        if (SvROK(sv)
        && (tmpsv = (SV*) SvRV(sv))
        && SvTYPE(tmpsv) == SVt_PVMG 
        && (mg_find(tmpsv, PERL_MAGIC_qr)))
        {
                return TRUE;
        }
         
        return FALSE;
}
 
#endif

/* <-- End of Toby Inkster contribution. Thank you. */

#define UNKNOWN_TYPE_FAIL(t)				\
    if (JCEH) {						\
	(*JCEH) (__FILE__, __LINE__,			\
		 "Unknown Perl type %d", t);		\
    }							\
    return json_create_unknown_type

/* This is the core routine, it is called recursively as hash values
   and array values containing array or hash references are
   handled. */

static json_create_status_t
json_create_recursively (json_create_t * jc, SV * input)
{
    if (! SvOK (input)) {
	/* We were told to add an undefined value, so put the literal
	   'null' (without quotes) at the end of "jc" then return. */
	ADD ("null");
	return json_create_ok;
    }
    if (SvROK (input)) {
	/* We have a reference, so decide what to do with it. */
	svtype t;
	SV * r;
	r = SvRV (input);
	t = SvTYPE (r);
	/* First try a switch for the types which have been in Perl
	   for a while. We can't add the case of SVt_REGEXP here since
	   it's not present in some older Perls, so we test for
	   regexes in the default: case at the bottom. */
	switch (t) {
	case SVt_PVAV:
	    CALL (json_create_add_array (jc, (AV *) r));
	    break;

	case SVt_PVHV:
	    CALL (json_create_add_object (jc, (HV *) r));
	    break;

	case SVt_PVMG:
	    CALL (json_create_add_string (jc, r));
	    break;

	case SVt_PVGV:
	    /* Completely untested. */
	    CALL (json_create_add_string (jc, r));
	    break;

	default:
	    /* Test for regex, possibly using the Toby Inkster code
	       above. */
	    if (SvRXOK (r)) {
		/* Use it as a string. */
		CALL (json_create_add_string (jc, r));
	    }
	    else {
		UNKNOWN_TYPE_FAIL (t);
	    }
	}
    }
    else {
	/* Not a reference, think about what to do. */
	SV * r = input;
	svtype t;
	t = SvTYPE (r);
	switch (t) {

	case SVt_NULL:
	    ADD ("null");
	    break;

	case SVt_PV:
	case SVt_PVMG:
	    CALL (json_create_add_string (jc, r));
	    break;

	case SVt_IV:
	    CALL (json_create_add_integer (jc, r));
	    break;

	case SVt_NV:
	    CALL (json_create_add_float (jc, r));
	    break;

	case SVt_PVNV:
	case SVt_PVIV:
	    /* Experimentally, add these as stringified. This code
	       path is untested. */
	    CALL (json_create_add_stringified (jc, r));
	    break;
	    
	default:
	    UNKNOWN_TYPE_FAIL(t);
	}
    }
    return json_create_ok;
}

/* Master-caller macro. Calls to subsystems from "json_create" cannot
   be handled using the CALL macro above, because we need to return a
   non-status value from json_create. If things go wrong somewhere, we
   return "undef". */

#define FINALCALL(x) {						\
	json_create_status_t status;				\
	status = x;						\
	if (status != json_create_ok) {				\
	    if (JCEH) {						\
		(*JCEH) (__FILE__, __LINE__,			\
			 "%s failed with status %d\n",		\
			 #x, status);				\
	    }							\
	    /* Free the memory of "output". */			\
	    if (jc.output) {					\
		SvREFCNT_dec (jc.output);			\
	    }							\
	    /* return undef; */					\
	    return & PL_sv_undef;				\
	}							\
    }

/* Master routine, callers should only ever use this. Everything above
   is only for the sake of "json_create" to use. */

static SV *
json_create (SV * input)
{
    json_create_t jc;

    jc.length = 0;
    /* Tell json_create_buffer_fill that it needs to allocate an
       SV. */
    jc.output = 0;

    /* "jc.buffer" is dirty here, we have not initialized it, we are
       just writing to uninitialized stack memory. "jc.length" is the
       only thing we know is OK at this point. */

    /* Unleash the dogs. */
    FINALCALL (json_create_recursively (& jc, input));
    /* Copy the remaining text in jc's buffer into input. */
    FINALCALL (json_create_buffer_fill (& jc));
    /* We didn't allocate any memory except for the SV, all our memory
       is on the stack, so there is nothing to free here. */
    return jc.output;
}

