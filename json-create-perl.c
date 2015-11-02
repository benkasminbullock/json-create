/* 
   This is the main part of JSON::Create.

   It's kept in a separate file but #included into the main file,
   Create.xs.
*/

#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE
#endif /* __GNUC__ */

/* These are return statuses for the types of failures which can
   occur. */

typedef enum {
    json_create_ok,
    /* Unknown Perl svtype within the structure. */
    json_create_unknown_type,
    /* An error from the unicode.c library. */
    json_create_unicode_error,
    /* A printed number turned out to be longer than MARGIN bytes. */
    json_create_number_too_long,
    /* Unknown type of floating point number. */
    json_create_unknown_floating_point,
    /* Bad format for floating point. */
    json_create_bad_floating_format,
    /* */
    json_create_unicode_bad_utf8,
    /* User's routine returned invalid stuff. */
    json_create_invalid_user_json,
}
json_create_status_t;

#define BUFSIZE 0x4000

/* MARGIN is the size of the "spillover" area where we can print
   numbers or Unicode UTF-8 whole characters (runes) into the buffer
   without having to check the printed length after each byte. */

#define MARGIN 0x40

typedef struct json_create {
    /* The length of the input string. */
    int length;
    unsigned char * buffer;
    /* Place to write the buffer to. */
    SV * output;
    /* Format for floating point numbers. */
    char * fformat;
    /* Memory leak counter. */
    int n_mallocs;
    /* Handlers for objects and booleans. If there are no handlers,
       this is zero (a NULL pointer). */
    HV * handlers;
    /* User reference handler. */
    SV * type_handler;
    /* Do any of the SVs have a Unicode flag? */
    unsigned int unicode : 1;
    /* Should we convert / into \/? */
    unsigned int escape_slash : 1;
    /* Should Unicode be upper case? */
    unsigned int unicode_upper : 1;
    /* Should we escape all non-ascii? */
    unsigned int unicode_escape_all : 1;
    /* Should we validate user-defined JSON? */
    unsigned int validate : 1;
    /* Do not escape U+2028 and U+2029. */
    unsigned int no_javascript_safe : 1;
    /* Make errors fatal. */
    unsigned int fatal_errors : 1;
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

#define HANDLE_STATUS(x,status) {				\
	switch (status) {					\
	    /* These exceptions indicate a user error. */	\
	case json_create_unknown_type:				\
	case json_create_unicode_bad_utf8:			\
	case json_create_invalid_user_json:			\
	    break;						\
	    							\
	    /* All other exceptions are bugs. */		\
	default:						\
	    if (JCEH) {						\
		(*JCEH) (__FILE__, __LINE__,			\
			 "call to %s failed with status %d",	\
			 #x, status);				\
	    }							\
	}							\
    }

#define CALL(x) {							\
	json_create_status_t status;					\
	status = x;							\
	if (status != json_create_ok) {					\
	    HANDLE_STATUS (x,status);					\
	    return status;						\
	}								\
    }

static void
json_create_user_message (json_create_t * jc, json_create_status_t status, const char * format, ...)
{
    va_list a;
    /* Check the status. */
    va_start (a, format);
    if (jc->fatal_errors) {
	vcroak (format, & a);
    }
    else {
	vwarn (format, & a);
    }
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

#define add_char_unsafe(jc, c) jc->buffer[jc->length++] = c

/* Add a nul-terminated string to "jc", up to the nul byte. This
   should not be used unless it's strictly necessary, prefer to use
   "add_str_len" instead. This is not intended to be Unicode-safe, it
   is only to be used for strings which we know do not need to be
   checked for Unicode validity (for example sprintf'd numbers or
   something). Basically, don't use this. */

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
   know do not need to be checked for Unicode validity. */

static INLINE json_create_status_t
add_str_len (json_create_t * jc, const char * s, unsigned int slen)
{
    int i;
    /* We know that (BUFSIZE - jc->length) is always bigger than
       MARGIN going into this, but the compiler doesn't. Hopefully,
       the compiler optimizes the following "if" statement away to a
       true value for almost all cases when this is inlined and slen
       is known to be smaller than MARGIN. */
    if (slen < MARGIN || slen < BUFSIZE - jc->length) {
	for (i = 0; i < slen; i++) {
	    jc->buffer[jc->length + i] = s[i];
	}
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

static const char *uc_hex = "0123456789ABCDEF";
static const char *lc_hex = "0123456789abcdef";

static INLINE json_create_status_t
add_one_u (json_create_t * jc, unsigned int u)
{
    char * spillover;
    const char * hex;
    hex = lc_hex;
    if (jc->unicode_upper) {
	hex = uc_hex;
    }
    spillover = (char *) (jc->buffer) + jc->length;
    spillover[0] = '\\';
    spillover[1] = 'u';
    // Method poached from https://metacpan.org/source/CHANSEN/Unicode-UTF8-0.60/UTF8.xs#L196
    spillover[5] = hex[u & 0xf];
    u >>= 4;
    spillover[4] = hex[u & 0xf];
    u >>= 4;
    spillover[3] = hex[u & 0xf];
    u >>= 4;
    spillover[2] = hex[u & 0xf];
    jc->length += 6;
    CHECKLENGTH;
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

#define BADUTF8							\
    json_create_user_message (jc, json_create_unicode_bad_utf8,	\
			      "Invalid UTF-8");			\
    return json_create_unicode_bad_utf8;


/* Add a string to the buffer with quotes around it and escapes for
   the escapables. */

static INLINE json_create_status_t
json_create_add_key_len (json_create_t * jc, const unsigned char * key, STRLEN keylen)
{
    int i;
    int l;
    l = 0;
    add_char_unsafe (jc, '"');
    for (i = 0; i < keylen; ) {
	unsigned char c;
	c = key[i];
	if (c < 0x20) {
	    if (c =='\b') {
		ADD ("\\b");
	    }
	    else if (c == '\t') {
		ADD ("\\t");
	    }
	    else if (c == '\n') {
		ADD ("\\n");
	    }
	    else if (c == '\f') {
		ADD ("\\f");
	    }
	    else if (c == '\r') {
		ADD ("\\r");
	    }
	    else {
		/* We know c is less than 0x10000, so we can use
		   "add_one_u" not "add_u" here. */
		CALL (add_one_u (jc, (unsigned int) c));
	    }
	    i++;
	}
	else if (c < 0x80) {
	    if (c == '"') {
		ADD ("\\\"");
	    }
	    else if (c == '\\') {
		ADD ("\\\\");
	    }
	    else if (c == '/' && jc->escape_slash) {
		ADD ("\\/");
	    }
	    else {
		CALL (add_char (jc, c));
	    }
	    i++;
	}
	else {
	    unsigned char d, e, f;
	    switch (c) {
	    case 0x80:
	    case 0x81:
	    case 0x82:
	    case 0x83:
	    case 0x84:
	    case 0x85:
	    case 0x86:
	    case 0x87:
	    case 0x88:
	    case 0x89:
	    case 0x8a:
	    case 0x8b:
	    case 0x8c:
	    case 0x8d:
	    case 0x8e:
	    case 0x8f:
	    case 0x90:
	    case 0x91:
	    case 0x92:
	    case 0x93:
	    case 0x94:
	    case 0x95:
	    case 0x96:
	    case 0x97:
	    case 0x98:
	    case 0x99:
	    case 0x9a:
	    case 0x9b:
	    case 0x9c:
	    case 0x9d:
	    case 0x9e:
	    case 0x9f:
	    case 0xa0:
	    case 0xa1:
	    case 0xa2:
	    case 0xa3:
	    case 0xa4:
	    case 0xa5:
	    case 0xa6:
	    case 0xa7:
	    case 0xa8:
	    case 0xa9:
	    case 0xaa:
	    case 0xab:
	    case 0xac:
	    case 0xad:
	    case 0xae:
	    case 0xaf:
	    case 0xb0:
	    case 0xb1:
	    case 0xb2:
	    case 0xb3:
	    case 0xb4:
	    case 0xb5:
	    case 0xb6:
	    case 0xb7:
	    case 0xb8:
	    case 0xb9:
	    case 0xba:
	    case 0xbb:
	    case 0xbc:
	    case 0xbd:
	    case 0xbe:
	    case 0xbf:
	    case 0xc0:
	    case 0xc1:
		BADUTF8;

	    case 0xc2:
	    case 0xc3:
	    case 0xc4:
	    case 0xc5:
	    case 0xc6:
	    case 0xc7:
	    case 0xc8:
	    case 0xc9:
	    case 0xca:
	    case 0xcb:
	    case 0xcc:
	    case 0xcd:
	    case 0xce:
	    case 0xcf:
	    case 0xd0:
	    case 0xd1:
	    case 0xd2:
	    case 0xd3:
	    case 0xd4:
	    case 0xd5:
	    case 0xd6:
	    case 0xd7:
	    case 0xd8:
	    case 0xd9:
	    case 0xda:
	    case 0xdb:
	    case 0xdc:
	    case 0xdd:
	    case 0xde:
	    case 0xdf:
		d = key[i + 1];
		if (d < 0x80 || d > 0xBF) {
		    BADUTF8;
		}
		if (jc->unicode_escape_all) {
		    unsigned int u;
		    u = (c & 0x1F)<<6
			| (d & 0x3F);
		    CALL (add_u (jc, u));
		}
		else {
		    CALL (add_str_len (jc, (const char *) key + i, 2));
		}
		i += 2;
		break;

	    case 0xe0:
	    case 0xe1:
	    case 0xe2:
	    case 0xe3:
	    case 0xe4:
	    case 0xe5:
	    case 0xe6:
	    case 0xe7:
	    case 0xe8:
	    case 0xe9:
	    case 0xea:
	    case 0xeb:
	    case 0xec:
	    case 0xed:
	    case 0xee:
	    case 0xef:
		d = key[i + 1];
		e = key[i + 2];
		if (d < 0x80 || d > 0xBF ||
		    e < 0x80 || e > 0xBF) {
		    BADUTF8;
		}
		if (! jc->no_javascript_safe &&
		    c == 0xe2 && d == 0x80 && 
		    (e == 0xa8 || e == 0xa9)) {
		    CALL (add_one_u (jc, 0x2028 + e - 0xa8));
		}
		else {
		    if (jc->unicode_escape_all) {
			unsigned int u;
			u = (c & 0x0F)<<12
			    | (d & 0x3F)<<6
			    | (e & 0x3F);
			CALL (add_u (jc, u));
		    }
		    else {
			CALL (add_str_len (jc, (const char *) key + i, 3));
		    }
		}
		i += 3;
		break;

	    case 0xf0:
	    case 0xf1:
	    case 0xf2:
	    case 0xf3:
	    case 0xf4:
		if (jc->unicode_escape_all) {
		    unsigned int u;
		    const unsigned char * input;
		    input = key + i;
		    u = (input[0] & 0x07) << 18
			| (input[1] & 0x3F) << 12
			| (input[2] & 0x3F) <<  6
			| (input[3] & 0x3F);
		    add_u (jc, u);
		}
		else {
		    CALL (add_str_len (jc, (const char *) key + i, 2));
		}
		i += 4;
		break;

	    case 0xf5:
	    case 0xf6:
	    case 0xf7:
	    case 0xf8:
	    case 0xf9:
	    case 0xfa:
	    case 0xfb:
	    case 0xfc:
	    case 0xfd:
	    case 0xfe:
	    case 0xff:
		BADUTF8;
	    }
	}
    }
    add_char_unsafe (jc, '"');
    return json_create_ok;
}

static INLINE json_create_status_t
json_create_add_string (json_create_t * jc, SV * input)
{
    char * istring;
    STRLEN ilength;
    /*
      "jc->unicode" is true if Perl says that anything in the whole of
      the input to "json_create" is a "SvUTF8" scalar.
    */
    istring = SvPV (input, ilength);
    if (SvUTF8 (input)) {
	/* We have to force everything in the whole output to
	   Unicode. */
	jc->unicode = 1;
    }
    /* Backtrace fall through, remember to check the caller's line. */
    return json_create_add_key_len (jc, (unsigned char *) istring,
				    (STRLEN) ilength);
}

/* Extract the remainder of x when divided by ten and then turn it
   into the equivalent ASCII digit. '0' in ASCII is 0x30, and (x)%10
   is guaranteed not to have any of the high bits set. */

#define DIGIT(x) (((x)%10)|0x30)

static INLINE json_create_status_t
json_create_add_integer (json_create_t * jc, SV * sv)
{
    long int iv;
    int ivlen;
    char * spillover;

    iv = SvIV (sv);
    ivlen = 0;

    /* Pointer arithmetic. */

    spillover = ((char *) jc->buffer) + jc->length;

    /* Souped-up integer printing for small integers. The following is
       all just souped up versions of snprintf ("%d", iv);. */

    if (iv < 0) {
	spillover[ivlen] = '-';
	ivlen++;
	iv = -iv;
    }
    if (iv < 10) {
	/* iv has exactly one digit. The first digit may be zero. */
	spillover[ivlen] = DIGIT (iv);
	ivlen++;
    }
    else if (iv < 100) {
	/* iv has exactly two digits. The first digit is not zero. */
	spillover[ivlen] = DIGIT (iv/10);
	ivlen++;
	spillover[ivlen] = DIGIT (iv);
	ivlen++;
    }
    else if (iv < 1000) {
	/* iv has exactly three digits. The first digit is not
	   zero. */
	spillover[ivlen] = DIGIT (iv/100);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/10);
	ivlen++;
	spillover[ivlen] = DIGIT (iv);
	ivlen++;
    }
    else if (iv < 10000) {
	/* etc. */
	spillover[ivlen] = DIGIT (iv/1000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/100);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/10);
	ivlen++;
	spillover[ivlen] = DIGIT (iv);
	ivlen++;
    }
    else if (iv < 100000) {
	spillover[ivlen] = DIGIT (iv/10000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/1000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/100);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/10);
	ivlen++;
	spillover[ivlen] = DIGIT (iv);
	ivlen++;
    }
    else if (iv < 1000000) {
	spillover[ivlen] = DIGIT (iv/100000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/10000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/1000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/100);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/10);
	ivlen++;
	spillover[ivlen] = DIGIT (iv);
	ivlen++;
    }
    else if (iv < 10000000) {
	spillover[ivlen] = DIGIT (iv/1000000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/100000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/10000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/1000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/100);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/10);
	ivlen++;
	spillover[ivlen] = DIGIT (iv);
	ivlen++;
    }
    else if (iv < 100000000) {
	spillover[ivlen] = DIGIT (iv/10000000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/1000000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/100000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/10000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/1000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/100);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/10);
	ivlen++;
	spillover[ivlen] = DIGIT (iv);
	ivlen++;
    }
    else if (iv < 1000000000) {
	spillover[ivlen] = DIGIT (iv/100000000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/10000000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/1000000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/100000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/10000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/1000);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/100);
	ivlen++;
	spillover[ivlen] = DIGIT (iv/10);
	ivlen++;
	spillover[ivlen] = DIGIT (iv);
	ivlen++;
    }
    else {
	/* The number is one billion (1000,000,000) or more, so we're
	   just going to print it into "jc->buffer" with snprintf. */
	ivlen += snprintf (spillover + ivlen, MARGIN - ivlen, "%ld", iv);
	if (ivlen >= MARGIN) {
	    if (JCEH) {
		(*JCEH) (__FILE__, __LINE__,
			 "A printed integer number %ld was "
			 "longer than MARGIN=%d bytes",
			 SvIV (sv), MARGIN);
	    }
	    return json_create_number_too_long;
	}
    }
    jc->length += ivlen;
    CHECKLENGTH;
    return json_create_ok;
}

static INLINE json_create_status_t
json_create_add_float (json_create_t * jc, SV * sv)
{
    double fv;
    STRLEN fvlen;
    fv = SvNV (sv);
    if (isfinite (fv)) {
	if (jc->fformat) {
	    fvlen = snprintf ((char *) jc->buffer + jc->length, MARGIN, jc->fformat, fv);
	}
	else {
	    fvlen = snprintf ((char *) jc->buffer + jc->length, MARGIN, "%g", fv);
	}
	if (fvlen >= MARGIN) {
	    return json_create_number_too_long;
	}
	jc->length += fvlen;
	CHECKLENGTH;
    }
    else {
	if (isnan (fv)) {
	    ADD ("\"nan\"");
	}
	else if (isinf (fv)) {
	    if (fv < 0.0) {
		ADD ("\"-inf\"");
	    }
	    else {
		ADD ("\"inf\"");
	    }
	}
	else {
	    return json_create_unknown_floating_point;
	}
    }
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

/* Add a comma where necessary. This is shared between objects and
   arrays. */

#define COMMA					\
    if (i > 0) {				\
	add_char_unsafe (jc, ',');		\
    }

/* Given a reference to a hash in "input_hv", recursively process it
   into JSON. "object" here means "JSON object", not "Perl object". */

static INLINE json_create_status_t
json_create_add_object (json_create_t * jc, HV * input_hv)
{
    I32 n_keys;
    int i;
    SV * value;
    char * key;
    I32 keylen;

    add_char_unsafe (jc, '{');
    n_keys = hv_iterinit (input_hv);
    for (i = 0; i < n_keys; i++) {
	HE * he;

	/* Get the information from the hash. */
	/* The following is necessary because "hv_iternextsv" doesn't
	   tell us whether the key is "SvUTF8" or not. */
	he = hv_iternext (input_hv);
	if (HeUTF8 (he)) {
	    jc->unicode = 1;
	}
	key = hv_iterkey (he, & keylen);
	value = hv_iterval (input_hv, he);

	/* Write the information into the buffer. */

	COMMA;
	CALL (json_create_add_key_len (jc, (const unsigned char *) key,
				       (STRLEN) keylen));
	add_char_unsafe (jc, ':');
	CALL (json_create_recursively (jc, value));
    }
    add_char_unsafe (jc, '}');
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

    add_char_unsafe (jc, '[');
    n_keys = av_len (av) + 1;
    /* This deals correctly with empty arrays, since av_len is -1 if
       the array is empty, so we do not test for a valid n_keys value
       before entering the loop. */
    for (i = 0; i < n_keys; i++) {
	COMMA;
	value = * (av_fetch (av, i, 0 /* don't delete the array value */));
	CALL (json_create_recursively (jc, value));
    }
    add_char_unsafe (jc, ']');
    return json_create_ok;
}

#define UNKNOWN_TYPE_FAIL(t)				\
    if (JCEH) {						\
	(*JCEH) (__FILE__, __LINE__,			\
		 "Unknown Perl type %d", t);		\
    }							\
    return json_create_unknown_type

//#define DEBUGOBJ

static json_create_status_t
json_create_validate_user_json (json_create_t * jc, SV * json)
{
    SV * error;
    dSP;
    ENTER;
    SAVETMPS;
    PUSHMARK (SP);
    XPUSHs (sv_2mortal (newSVsv (json)));
    PUTBACK;
    call_pv ("JSON::Parse::assert_valid_json",
	     G_EVAL|G_DISCARD);
    FREETMPS;
    LEAVE;  
    error = get_sv ("@", 0);
    if (! error) {
	return json_create_ok;
    }
    if (SvOK (error) && SvCUR (error) > 0) {
	json_create_user_message (jc, json_create_invalid_user_json,
				  "JSON::Parse::assert_valid_json failed for '%s': %s",
				  SvPV_nolen (json), SvPV_nolen (error));
	return json_create_invalid_user_json;
    }
    return json_create_ok;
}


static json_create_status_t
json_create_call_to_json (json_create_t * jc, SV * cv, SV * r)
{
    I32 count;
    SV * json;
    char * jsonc;
    STRLEN jsonl;
    // https://metacpan.org/source/AMBS/Math-GSL-0.35/swig/gsl_typemaps.i#L438
    dSP;
    
    ENTER;
    SAVETMPS;
    
    PUSHMARK (SP);
    //https://metacpan.org/source/AMBS/Math-GSL-0.35/swig/gsl_typemaps.i#L482
    XPUSHs (sv_2mortal(newRV(r)));
    PUTBACK;
    call_sv (cv, 0);
    json = POPs;
    SvREFCNT_inc (json);
    FREETMPS;
    LEAVE;  
    jsonc = SvPV (json, jsonl);
    if (jc->validate) {
	CALL (json_create_validate_user_json (jc, json));
    }
    CALL (add_str_len (jc, jsonc, jsonl));
    SvREFCNT_dec (json);
    return json_create_ok;
}

static INLINE json_create_status_t
json_create_handle_unknown_type (json_create_t * jc, SV * r)
{
    if (jc->type_handler) {
	CALL (json_create_call_to_json (jc, jc->type_handler, r));
    }
    else {
	json_create_user_message (jc, json_create_unknown_type,
				  "Input's type cannot be serialized to JSON");
	return json_create_unknown_type;
    }
    return json_create_ok;
}

static INLINE json_create_status_t
json_create_handle_ref (json_create_t * jc, SV * input)
{
    svtype t;
    SV * r;
    r = SvRV (input);
    t = SvTYPE (r);
    switch (t) {
    case SVt_PVAV:
	CALL (json_create_add_array (jc, (AV *) r));
	break;

    case SVt_PVHV:
	CALL (json_create_add_object (jc, (HV *) r));
	break;

    case SVt_PVMG:
	/* There are some edge cases with blessed references
	   containing numbers which we need to handle correctly. */
	if (SvIOK (r)) {
	    CALL (json_create_add_integer (jc, r));
	}
	else if (SvNOK (r)) {
	    CALL (json_create_add_float (jc, r));
	}
	else {
	    CALL (json_create_add_string (jc, r));
	}
	break;

    case SVt_PVGV:
	/* Completely untested. */
	CALL (json_create_add_string (jc, r));
	break;

    case SVt_PV:
	CALL (json_create_add_string (jc, r));
	break;

    default:
	CALL (json_create_handle_unknown_type (jc, r));
    }
    return json_create_ok;
}

static INLINE json_create_status_t
json_create_handle_object (json_create_t * jc, SV * input)
{
    const char * objtype;
    SV * r;
    r = SvRV (input);
    /* The second argument to sv_reftype is true if we
       look it up in the object table, false
       otherwise. Undocumented, reported as
       https://rt.perl.org/Ticket/Display.html?id=126469. */
    objtype = sv_reftype (r, 1);
    if (objtype) {
	SV ** sv_ptr;
	I32 olen;
#ifdef DEBUGOBJ
	fprintf (stderr, "Have found an object of type %s.\n", objtype);
#endif
	olen = strlen (objtype);
	sv_ptr = hv_fetch (jc->handlers, objtype, olen, 0);
	if (sv_ptr) {
	    char * pv;
	    STRLEN pvlen;
	    pv = SvPV (*sv_ptr, pvlen);
#ifdef DEBUGOBJ
	    fprintf (stderr, "Have found a handler %s for %s.\n", pv, objtype);
#endif
	    if (pvlen == strlen ("bool") &&
		strncmp (pv, "bool", 4) == 0) {
		if (SvTRUE (r)) {
		    ADD ("true");
		}
		else {
		    ADD ("false");
		}
	    }
	    else if (SvROK (*sv_ptr)) {
		SV * what;
		what = SvRV (*sv_ptr);
		if (SvROK (what)) {
		    what = SvRV(what);
		}
		switch (SvTYPE (what)) {
		case SVt_PVCV:
		    CALL (json_create_call_to_json (jc, what, r));
		    break;
		default:
		    /* Weird handler, not a code reference. */
		    goto nothandled;
		}
	    }
	    else {
		/* It's an object, it's in our handlers, but we don't
		   have any code to deal with it, so we'll print an
		   error and then stringify it. */
		if (JCEH) {
		    (*JCEH) (__FILE__, __LINE__, "Unhandled handler %s.\n",
			     pv);
		    goto nothandled;
		}
	    }
	}
	else {
#ifdef DEBUGOBJ
	    /* Leaving this debugging code here since this is liable
	       to change a lot. */
	    I32 hvnum;
	    SV * s;
	    char * key;
	    I32 retlen;
	    fprintf (stderr, "Nothing in handlers for %s.\n", objtype);
	    hvnum = hv_iterinit (jc->handlers);

	    fprintf (stderr, "There are %ld keys in handlers.\n", hvnum);
	    while (1) {
		s = hv_iternextsv (jc->handlers, & key, & retlen);
		if (! s) {
		    break;
		}
		fprintf (stderr, "%s: %s\n", key, SvPV_nolen (s));
	    }
#endif /* 0 */
	nothandled:
	    CALL (json_create_handle_ref (jc, input));
	}
    }
    return json_create_ok;
}

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
    /* JSON::Parse inserts pointers to &PL_sv_yes and no as literal
       "true" and "false" markers. */
    if (input == &PL_sv_yes) {
	ADD ("true");
	return json_create_ok;
    }
    if (input == &PL_sv_no) {
	ADD ("false");
	return json_create_ok;
    }
    if (SvROK (input)) {
	/* We have a reference, so decide what to do with it. */
	if (sv_isobject (input) && jc->handlers) {
	    CALL (json_create_handle_object (jc, input));
	}
	else {
	    CALL (json_create_handle_ref (jc, input));
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

	case SVt_PVMG:
	case SVt_PV:
	    CALL (json_create_add_string (jc, r));
	    break;

	case SVt_IV:
	    CALL (json_create_add_integer (jc, r));
	    break;

	case SVt_NV:
	    CALL (json_create_add_float (jc, r));
	    break;

	case SVt_PVNV:
	    /* We need to handle non-finite numbers without using
	       Perl's stringified forms, because we need to put quotes
	       around them, whereas Perl will just print 'nan' the
	       same way it will print '0.01'. 'nan' is not valid JSON,
	       so we have to convert to '"nan"'. */
	    CALL (json_create_add_float (jc, r));
	    break;

	case SVt_PVIV:
	    /* Experimentally, add these as stringified. This code
	       path is untested. */
	    CALL (json_create_add_stringified (jc, r));
	    break;
	    
	default:
	    CALL (json_create_handle_unknown_type (jc, r));
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
	    HANDLE_STATUS (x, status);				\
	    /* Free the memory of "output". */			\
	    if (jc->output) {					\
		SvREFCNT_dec (jc->output);			\
		jc->output = 0;					\
	    }							\
	    /* return undef; */					\
	    return & PL_sv_undef;				\
	}							\
    }

/* Dog run. */

static INLINE SV *
json_create_run (json_create_t * jc, SV * input)
{
    unsigned char buffer[BUFSIZE];

    /* Set up all the transient variables for reading. */

    /* "jc.buffer" is dirty here, we have not initialized it, we are
       just writing to uninitialized stack memory. "jc.length" is the
       only thing we know is OK at this point. */
    jc->buffer = buffer;

    jc->length = 0;
    /* Tell json_create_buffer_fill that it needs to allocate an
       SV. */
    jc->output = 0;
    /* Not Unicode. */
    jc->unicode = 0;

    /* Unleash the dogs. */
    FINALCALL (json_create_recursively (jc, input));
    /* Copy the remaining text in jc's buffer into "jc->output". */
    FINALCALL (json_create_buffer_fill (jc));

    if (jc->unicode) {
	SvUTF8_on (jc->output);
    }

    /* We didn't allocate any memory except for the SV, all our memory
       is on the stack, so there is nothing to free here. */

    return jc->output;
}

/* Master routine, callers should only ever use this. Everything above
   is only for the sake of "json_create" to use. */

static INLINE SV *
json_create (SV * input)
{
    /* With all the options, this really needs to be blanked out. Thus
       "buffer" is moved from being inside "jc" to being inside
       "json_create_run" above. */
    json_create_t jc = {0};
    /* Set up the default options. */

    /* Floating point number format. */
    jc.fformat = 0;
    /* Escape slash. */
    jc.escape_slash = 0;

    jc.unicode_escape_all = 0;
    jc.handlers = 0;
    jc.type_handler = 0;
    return json_create_run (& jc, input);
}

/*  __  __      _   _               _     
   |  \/  | ___| |_| |__   ___   __| |___ 
   | |\/| |/ _ \ __| '_ \ / _ \ / _` / __|
   | |  | |  __/ |_| | | | (_) | (_| \__ \
   |_|  |_|\___|\__|_| |_|\___/ \__,_|___/ */
                                       

static json_create_status_t
json_create_new (json_create_t ** jc_ptr)
{
    json_create_t * jc;
    Newxz (jc, 1, json_create_t);
    jc->n_mallocs = 0;
    jc->n_mallocs++;
    jc->fformat = 0;
    jc->type_handler = 0;
    jc->handlers = 0;
    * jc_ptr = jc;
    return json_create_ok;
}

static json_create_status_t
json_create_free_fformat (json_create_t * jc)
{
    if (jc->fformat) {
	Safefree (jc->fformat);
	jc->fformat = 0;
	jc->n_mallocs--;
    }
    return json_create_ok;
}

static json_create_status_t
json_create_set_fformat (json_create_t * jc, SV * fformat)
{
    char * ff;
    STRLEN fflen;
    int i;

    CALL (json_create_free_fformat (jc));
    if (! SvTRUE (fformat)) {
	jc->fformat = 0;
	return json_create_ok;
    }

    ff = SvPV (fformat, fflen);
    if (! strchr (ff, '%')) {
	return json_create_bad_floating_format;
    }
    Newx (jc->fformat, fflen + 1, char);
    jc->n_mallocs++;
    for (i = 0; i < fflen; i++) {
	/* We could also check the format in this loop. */
	jc->fformat[i] = ff[i];
    }
    jc->fformat[fflen] = '\0';
    return json_create_ok;
}

static json_create_status_t
json_create_remove_handlers (json_create_t * jc)
{
    if (jc->handlers) {
	SvREFCNT_dec ((SV *) jc->handlers);
	jc->handlers = 0;
	jc->n_mallocs--;
    }
    return json_create_ok;
}

static json_create_status_t
json_create_remove_type_handler (json_create_t * jc)
{
    if (jc->type_handler) {
	SvREFCNT_dec (jc->type_handler);
	jc->type_handler = 0;
	jc->n_mallocs--;
    }
    return json_create_ok;
}

static json_create_status_t
json_create_free (json_create_t * jc)
{
    CALL (json_create_free_fformat (jc));
    CALL (json_create_remove_handlers (jc));
    CALL (json_create_remove_type_handler (jc));

    /* Finished, check we have no leaks before freeing. */

    jc->n_mallocs--;
    if (jc->n_mallocs != 0) {
	fprintf (stderr, "%s:%d: n_mallocs = %d\n",
		 __FILE__, __LINE__, jc->n_mallocs);
    }
    Safefree (jc);
    return json_create_ok;
}
