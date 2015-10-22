#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE
#endif /* __GNUC__ */

typedef enum {
    json_create_ok,
    /* Unknown Perl svtype within the structure. */
    json_create_unknown_type,
    /* An error from the unicode.c library. */
    json_create_unicode_error,
    /* A printed number turned out to be longer than MARGIN bytes. */
    json_create_number_too_long,
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

/* MARGIN is the size of the "spillover" area where we can print
   numbers or Unicode UTF-8 whole characters (runes) into the buffer
   without having to check the printed length after each byte. */

#define MARGIN 0x40

typedef struct json_create {
    /* The length of the input string. */
    int length;
    unsigned char buffer[BUFSIZE];
    /* Place to write the buffer to. */
    SV * output;
    /* Do any of the SVs have a Unicode flag? */
    unsigned int unicode : 1;
    /* Does the SV we're currently looking at have a Unicode flag? */
    unsigned int unicode_now : 1;
    /* Did we see non-Unicode and non-ASCII bytes? */
    unsigned int non_unicode : 1;
}
json_create_t;

/* Check the length of the buffer, and if we don't have more than
   MARGIN bytes left to write into, then we put "jc->buffer" into the
   Perl scalar "jc->output" via "json_create_buffer_fill". We always
   want to be at least MARGIN bytes from the end of "jc->buffer" after
   every write operation so that we always have room to put a number
   or a UTF-8 "rune" in the buffer without checking the length
   excessively. */

#define CHECKLENGTH				\
    if (jc->length >= BUFSIZE - MARGIN) {	\
	CALL (json_create_buffer_fill (jc));	\
    }

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
    CHECKLENGTH;
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
    /* Hopefully, the compiler optimizes the following "if" statement
       away to a true value for almost all cases. */
    if (slen < MARGIN) {
	/* Gonna take you right into the DANGER ZONE. */
	for (i = 0; i < slen; i++) {
	    /* DANGER ZONE! */
	    jc->buffer[jc->length + i] = s[i];
	}
	/* Phew. We survived the DANGER ZONE. */
	jc->length += slen;
	CHECKLENGTH;
    }
    else {
	/* A very long string which may overflow the buffer, so use
	   checking routines. */
	for (i = 0; i < slen; i++) {
	    CALL (add_char (jc, (unsigned char) s[i]));
	}
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
    /* In the case that we want to Unicode-escape everything, this
       would be a good place to soup-up. The below code is
       inefficient. */
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
		/* We know c is less than 0x10000, so we can use
		   "add_one_u" not "add_u" here. */
		CALL (add_one_u (jc, (unsigned int) c));
	    }
	}
	else if (c < 0x80) {
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
	else {
	    if (! jc->unicode_now) {
		jc->non_unicode = 1;
	    }
	    CALL (add_char (jc, c));
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
    char * istring;
    STRLEN ilength;
    /*
      "jc->unicode_now" is true (equals 1) if Perl says that "input"
      contains a "SvUTF8" scalar, and false (equals 0) if Perl does
      not say that "input" contains a "SvUTF8" scalar.

      "jc->unicode" is true if Perl says that anything in the whole of
      the input to "json_create" is a "SvUTF8" scalar.
    */
    jc->unicode_now = 0;
    istring = SvPV (input, ilength);
    if (SvUTF8 (input)) {
	/* We have to force everything in the whole output to
	   Unicode. */
	jc->unicode = 1;
	/* Flag that this string is supposed to be Unicode to the
	   upstream. */
	jc->unicode_now = 1;
    }
    /* Backtrace fall through, remember to check the caller's line. */
    return json_create_add_key_len (jc, (unsigned char *) istring,
				    (STRLEN) ilength);
}

static INLINE json_create_status_t
json_create_add_integer (json_create_t * jc, SV * sv)
{
    long int iv;
    int ivlen;
    iv = SvIV (sv);

    /* Souped-up integer printing for small integers. The following is
       all just souped up versions of snprintf ("%d", iv);. */

    if (iv < 0) {
	CALL (add_char (jc, '-'));
	iv = -iv;
    }
    if (iv < 10) {
	CALL (add_char (jc, iv + '0'));
    }
    else if (iv < 100) {
	char ivc[2];
	ivc[0] = iv / 10 + '0';
	ivc[1] = iv % 10 + '0';
	CALL (add_str_len (jc, ivc, 2));
    }
    else if (iv < 1000) {
	char ivc[3];
	ivc[0] = iv / 100 + '0';
	ivc[1] = (iv / 10) % 10 + '0';
	ivc[2] = iv % 10 + '0';
	CALL (add_str_len (jc, ivc, 3));
    }
    else if (iv < 10000) {
	char ivc[4];
	ivc[0] = iv / 1000 + '0';
	ivc[1] = (iv / 100) % 10 + '0';
	ivc[2] = (iv / 10) % 10 + '0';
	ivc[3] = iv % 10 + '0';
	CALL (add_str_len (jc, ivc, 4));
    }
    else if (iv < 100000) {
	char ivc[5];
	ivc[0] = iv / 10000 + '0';
	ivc[1] = (iv / 1000) % 10 + '0';
	ivc[2] = (iv / 100) % 10 + '0';
	ivc[3] = (iv / 10) % 10 + '0';
	ivc[4] = iv % 10 + '0';
	CALL (add_str_len (jc, ivc, 5));
    }
    else {
	/* The number is 100,000 or more, so we're just going to print
	   it into "jc->buffer" with snprintf. */
	ivlen = snprintf ((char *) jc->buffer + jc->length, MARGIN, "%ld", iv);
	if (ivlen >= MARGIN) {
	    return json_create_number_too_long;
	}
	jc->length += ivlen;
	CHECKLENGTH;
    }
    return json_create_ok;
}

static INLINE json_create_status_t
json_create_add_float (json_create_t * jc, SV * sv)
{
    double fv;
    STRLEN fvlen;
    char fvbuf[MARGIN];
    fv = SvNV (sv);
    fvlen = snprintf ((char *) jc->buffer + jc->length, MARGIN, "%g", fv);
    if (fvlen >= MARGIN) {
	return json_create_number_too_long;
    }
    jc->length += fvlen;
    CHECKLENGTH;
    return json_create_ok;
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
	HE * he;
	/* The key is not unicode unless Perl tells us it's
	   unicode. */
	jc->unicode_now = 0;
	COMMA;
	/* We have to do all this rigamarole because hv_iternextsv
	   doesn't tell us whether the key is "SvUTF8" or not. */
	he = hv_iternext (input_hv);
	if (HeUTF8 (he)) {
	    jc->unicode = 1;
	    jc->unicode_now = 1;
	}
	key = hv_iterkey (he, & keylen);
	value = hv_iterval (input_hv, he);
	/* Back to the future. */
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
    /* Not Unicode. */
    jc.unicode = 0;
    /* So far we have not seen any non-Unicode bytes over 0x80. */
    jc.non_unicode = 0;

    /* "jc.buffer" is dirty here, we have not initialized it, we are
       just writing to uninitialized stack memory. "jc.length" is the
       only thing we know is OK at this point. */

    /* Unleash the dogs. */
    FINALCALL (json_create_recursively (& jc, input));
    /* Copy the remaining text in jc's buffer into input. */
    FINALCALL (json_create_buffer_fill (& jc));

    /*
      At least one of the SVs was Unicoded, so switch on Unicode here.

      We also checked that there was nothing with a non-ASCII byte in
      an SV not marked as Unicode, so we are now going to trust that
      the user did not send insane inputs. If there was something with
      a non-ASCII byte not marked as Unicode, we're going to just
      refuse to encode it.
    */

    if (jc.unicode) {
	if (jc.non_unicode) {
	    warn ("Mixed multibyte and binary inputs, refusing to encode to JSON");
	    SvREFCNT_dec (jc.output);
	    return & PL_sv_undef;
	}
	SvUTF8_on (jc.output);
    }
    /* We didn't allocate any memory except for the SV, all our memory
       is on the stack, so there is nothing to free here. */
    return jc.output;
}

