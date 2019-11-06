/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include <sys/types.h>
#include "regex.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

#include "m.h"

#define MAX_STRING_LENGTH 256
#define MAX_ARGS 20
#define STACK_SIZE 20

static void
usage( void )
{
    fprintf( stderr,
"\n"
"usage: m [options] [arg] [arg] [op] [....]\n"
"\n"
"if no options given, all output formats are done simultaneously.\n"
"\n"
"    -s       assembly format output\n"
"    -b       binary format output\n"
"    -B base  output in arbitrary base\n"
"    -d       decimal format output\n"
"    -u       unsigned decimal output\n"
"    -h       hex output\n"
"    -o       octal output\n"
"    -x       print this help message\n"
"    -4       restrict output to 32 bits (4 bytes)\n"
"\n"
"when inputting hex numbers, can use either '0x' prefix or 'h' suffix.\n"
"if hex number contains letters, no prefix or suffix required. \n"
"ppc assembly format also supported, enter in form '@ha,@l' with comma.\n"
"\n"
"calculations are stack-based -- push args then call operation. operation\n"
"pops required arguments then pushes back result. stack can be up to 20 deep\n"
"but must be only one deep after final calculation. operations when there\n"
"are not enough stack args for calculation results in error. more than one\n"
"stack item left at the end is also an error.\n"
"\n"
"valid ops are: + - * times / %% and or xor not swap32 swap16 \n"
"               rshift >> lshift << lt < lteq <= gt > gteq >= eq == \n"
"\n"
"examples:\n"
"\n"
"    m -d 4 1 +          gives '5'\n"
"    m -h 4f29e 814f -   gives '0004714f'\n"
"    m -d 4 4            gives 'stack not reduced to one item' error\n"
"    m -d 4 +            gives 'not enough args for +' error\n"
"    m -s 0x294f81       gives '41,20353'\n"
"    m -h 41,0x4f81      gives '00414f81'\n"
"\n"
"note on the last example that even though @ha looks like decimal it is\n"
"interpreted as hex because the @l is given in hex.  @ha and @l can be in\n"
"hex or dec, but they must both be the same radix.\n"
"\n"
"input numbers can be of the following forms: \n"
"                       -?[0-7]+o : octal\n"
"                         -?[0-9] : decimal\n"
"                       [0-9a-f]+ : hex (implicit)\n"
"                   -?0x[0-9a-f]+ : hex (prefix format)\n"
"                      [0-9a-f]+h : hex (emon format)\n"
"               -?[0-9]+,-?[0-9]+ : assembly decimal implicit\n"
"     -?0x[0-9a-f]+,-?0x[0-9a-f]+ : assembly hex explicit\n"
"           -?[0-9A-Za-z]+@[0-9]+ : arbitrary base (number@base)\n"
"                        [01]+bin : binary explicit\n"
"\n"
        );
    exit( 0 );
}


/*
 * a regular expression which locates whitespace.
 * the inputs to this file can be multiple args
 * or a single string in one arg. it will parse either
 * way.
 */

static const char * whtsp_expr_string = "[ \t\r\n]+";

#define MAX_WHITESPACE_MATCHES 2

/*
 * a regular expression which matches all of the other
 * inputs, including arguments, operators, and input numeric values
 * in various representations. the parenthetical expressions correspond
 * to fields in the regmatch_t when regexec is performed, so to figure
 * out which kind of arg a particular arg is, just find the first element
 * in the regmatch_t which is non-minus-one, and its index corresponds to
 * the entry in this table.
 */

static const char * arg_expr_string = "\
^(-[sbBduho4x]+)$|\
^(\\+)$|\
^(-)$|\
^(\\*|times)$|\
^(/)$|\
^(%)$|\
^(and)$|\
^(or)$|\
^(xor)$|\
^(not)$|\
^(swap32)$|\
^(swap16)$|\
^(rshift|>>)$|\
^(lshift|<<)$|\
^(lt|<)$|\
^(lteq|<=)$|\
^(gt|>)$|\
^(gteq|>=)$|\
^(eq|==)$|\
^(-?[0-7]+o)$|\
^(-?[0-9]+)$|\
^([0-9a-f]+)$|\
^(-?0x[0-9a-f]+)$|\
^([0-9a-f]+h)$|\
^(-?[0-9]+,-?[0-9]+)$|\
^(-?0x[0-9a-f]+,-?0x[0-9a-f]+)$|\
^(-?[0-9A-Za-z]+@[0-9]+)$|\
^([01]+bin)$|\
^(.*)$\
";

/*
 * the order of the above expressions must match the 
 * order of the enums below, as the numeric values of the
 * enums below are used as an index. ".*" must always
 * be the last entry above so that ARG_ERR is returned on 
 * anything which matches none of the other patterns.
 */

enum arg_type {
    ARG_DUMMY = 0, /* the zeroth entry of a regmatch_t */
    ARG_FLAG,
    ARG_PLUS, ARG_MINUS, ARG_TIMES, ARG_DIVIDE, ARG_MOD,
    ARG_AND, ARG_OR, ARG_XOR, ARG_NOT,
    ARG_SWAP32, ARG_SWAP16, ARG_RSHFT, ARG_LSHFT,
    ARG_LT, ARG_LTEQ, ARG_GT, ARG_GTEQ, ARG_EQ,
    ARG_OCTAL, ARG_DECIMAL, ARG_HEX_IMPL, ARG_HEX_P, ARG_HEX_H,
    ARG_HEX_ASSEM1, ARG_HEX_ASSEM2, ARG_ARBITRARY, ARG_BINARY, ARG_ERR,
    ARG_MAX
};

#define MAX_MATCHES ARG_MAX

#if 0
/*
 * Only used in debugging prints; the index into this array
 * must match the above array.
 */
static char * arg_type_names[] = {
    "dummy", "flag",
    "plus", "minus", "times", "divide", "modulo",
    "and", "or", "xor", "not",
    "swap32", "swap16", "rshift", "lshift", "less",
    "lesseq", "greater", "greatereq", "equal",
    "octal with 'o'", "decimal",
    "hex implied", "hex w/prefix", "hex with 'h'",
    "hex in @ha,@l", "hex in (0x)@ha,@l", "arbitrary base", "binary", 
    "parse error"
};
#endif

static char errbuf[80];
static int output_base_B = 10;

#define FLAGS_HEX_ASSM   0x01
#define FLAGS_BINARY     0x02
#define FLAGS_BASE       0x04
#define FLAGS_DECIMAL    0x08
#define FLAGS_UDECIMAL   0x10
#define FLAGS_HEX        0x20
#define FLAGS_OCTAL      0x40
#define FLAGS_32BIT      0x80

static int
parse_flags( char * str, int flags )
{
    str++;
    while ( *str )
    {
        switch ( *str )
        {
#define     DOFLG(letter,val) case letter : flags |= val ; break
            DOFLG('s',FLAGS_HEX_ASSM);
            DOFLG('b',FLAGS_BINARY  );
            DOFLG('d',FLAGS_DECIMAL );
            DOFLG('u',FLAGS_UDECIMAL);
            DOFLG('h',FLAGS_HEX     );
            DOFLG('o',FLAGS_OCTAL   );
            DOFLG('4',FLAGS_32BIT   );
#undef      DOFLG
        case 'B':
            /* flag to the main parse loop that it
               needs to update the base global. */
            output_base_B = -1;
            flags |= FLAGS_BASE;
            break;
        case 'x':  usage(); break;
        }
        str++;
    }
    return flags;
}

/*
 * note that the following number-format conversion
 * functions have very little error checking -- this is
 * because the strings passed into them have already been
 * checked for conformance to the format by the primary
 * parser regular expression.
 */

static M_INT64
m_cvt_dec( char * s )
{
    M_INT64 ret;
    int neg = 0;

    if ( *s == '-' )
    {
        neg = 1;
        s++;
    }

    for ( ret = 0; *s >= 0x30 && *s <= 0x39; s++ )
    {
        int d = *s - 0x30;
        ret = (ret * 10) + d;
    }

    return neg ? -ret : ret;
}

static M_INT64
m_cvt_hex( char * s )
{
    static const signed char cvt_matrix[] = {
/* 0x30 */    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
/* 0x40 */   -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
/* 0x50 */   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
/* 0x60 */   -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1
    };
    M_INT64 ret;
    int neg = 0;

    if ( *s == '-' )
    {
        neg = 1;
        s++;
    }

    if ( s[0] == '0' && s[1] == 'x' )
        s += 2;

    for ( ret = 0; *s >= 0x30 && *s <= 0x6f; s++ )
    {
        int d;
        d = cvt_matrix[ *s - 0x30 ];
        if ( d == -1 )
            break;
        ret = (ret << 4) + d;
    }

    return neg ? -ret : ret;
}

static M_INT64
m_cvt_octal( char * s )
{
    M_INT64 ret;
    int neg = 0;

    if ( *s == '-' )
    {
        neg = 1;
        s++;
    }

    for ( ret = 0; *s >= 0x30 && *s <= 0x37; s++ )
    {
        int d = *s - 0x30;
        ret = (ret << 3) + d;
    }

    return neg ? -ret : ret;
}

static M_INT64
m_cvt_arbitrary( char * str )
{
    M_INT64 ret;
    char *baseptr;
    int base;

    baseptr = strchr( str, '@' );
    base = atoi( baseptr+1 );
    if ( m_parse_number( &ret, str, baseptr - str, base ) != 0 )
    {
        // error parsing arbitrary base number
        return 0;
    }
    return ret;
}

static M_INT64
m_cvt_binary( char * s )
{
    M_INT64 ret;
    for ( ret = 0; *s >= 0x30 && *s <= 0x31; s++ )
    {
        int d = *s - 0x30;
        ret = (ret << 1) + d;
    }
    return ret;
}

static M_INT64
parse_assem1( char * str )
{
    int highval, lowval;
    highval = atoi( str );
    str = strchr( str, ',' );

    if ( str )
        lowval = atoi( str+1 );
    else
    {
        // unparsable arg
        return 0;
    }

    return (M_INT64)((highval << 16) + lowval);
}

static M_INT64
parse_assem2( char * str )
{
    int highval, lowval;
    highval = m_cvt_hex( str );
    str = strchr( str, ',' );

    if ( str )
        lowval = m_cvt_hex( str+1 );
    else
    {
        // unparsable arg
        return 0;
    }

    return (M_INT64)((highval << 16) + lowval);
}

static M_INT64
parse_value( int val_type, char * str )
{
    switch ( val_type )
    {
#define DO_TYPE( type, func ) case type : return func ( str )
        DO_TYPE( ARG_DECIMAL,     m_cvt_dec       );
        DO_TYPE( ARG_HEX_IMPL,    m_cvt_hex       );
        DO_TYPE( ARG_HEX_H,       m_cvt_hex       );
        DO_TYPE( ARG_HEX_P,       m_cvt_hex       );
        DO_TYPE( ARG_OCTAL,       m_cvt_octal     );
        DO_TYPE( ARG_ARBITRARY,   m_cvt_arbitrary );
        DO_TYPE( ARG_BINARY,      m_cvt_binary    );
        DO_TYPE( ARG_HEX_ASSEM1,  parse_assem1    );
        DO_TYPE( ARG_HEX_ASSEM2,  parse_assem2    );
#undef  DO_TYPE
    }

    // parse_value internal error
    return 0;
}

static M_INT64
operation1( int operation, M_INT64 arg )
{
    switch ( operation )
    {
    case ARG_NOT:
        return arg ^ 0xffffffffffffffffULL;
    case ARG_SWAP32:
        return
            (( arg >> 24 ) & 0x000000ff ) |
            (( arg >>  8 ) & 0x0000ff00 ) |
            (( arg <<  8 ) & 0x00ff0000 ) |
            (( arg << 24 ) & 0xff000000 );
    case ARG_SWAP16:
        return
            (( arg >>  8 ) & 0x000000ff ) |
            (( arg <<  8 ) & 0x0000ff00 );
    }
    // operation1 internal error
    return 0;
}

static M_INT64
operation2( int operation, M_INT64 arg1, M_INT64 arg2 )
{
    switch ( operation )
    {
#define DO_OP(op,how)   case op : return arg1 how arg2
        DO_OP( ARG_PLUS,    +   );
        DO_OP( ARG_MINUS,   -   );
        DO_OP( ARG_TIMES,   *   );
        DO_OP( ARG_DIVIDE,  /   );
        DO_OP( ARG_MOD,     %   );
        DO_OP( ARG_AND,     &   );
        DO_OP( ARG_OR,      |   );
        DO_OP( ARG_XOR,     ^   );
        DO_OP( ARG_RSHFT,   >>  );
        DO_OP( ARG_LSHFT,   <<  );
        DO_OP( ARG_LT,      <   );
        DO_OP( ARG_LTEQ,    <=  );
        DO_OP( ARG_GT,      >   );
        DO_OP( ARG_GTEQ,    >=  );
        DO_OP( ARG_EQ,      ==  );
#undef  DO_OP
    }
    // operation2: internal error
    return 0;
}

enum m_math_retvals
m_do_math( int argc, char ** argv, M_INT64 *result, int *flags )
{
    static regex_t expr;

    regmatch_t matches[ MAX_MATCHES ];
    char _instring[MAX_STRING_LENGTH + 1];
    char * instring;
    char * _args[MAX_ARGS];
    char ** args;
    M_INT64 stack[ STACK_SIZE ], top = 0, regerr;
    M_INT64 ret = 0;
    int i, inlength, numargs;
    enum m_math_retvals err;

    memset(stack, 0, sizeof(stack));

    instring = _instring;
    *instring = 0;
    inlength = 1;

    /*
     * concatenate the args into a single
     * string with whitespace at end
     */

    while ( argc > 0 )
    {
        int arglen = strlen( *argv );

        if (( arglen + inlength + 1 ) > MAX_STRING_LENGTH )
        {
            // input string overflow
            return M_MATH_OVERFLOW;
        }

        strcpy( instring, *argv );
        inlength += arglen + 1;
        instring += arglen;
        *instring++ = ' ';
        *instring = 0;
        argc--;
        argv++;
    }

    instring = _instring;
    numargs = 0;
    /* now parse string into args. */

    regerr = regcomp( &expr, whtsp_expr_string, REG_EXTENDED );

    if ( regerr != 0 )
    {
        regerror( regerr, &expr, errbuf, 80 );
        return M_MATH_REGERR;
    }

    while ( *instring )
    {
        int so,eo;
        regerr = regexec( &expr, instring,
                          MAX_WHITESPACE_MATCHES, matches, 0 );
        if ( regerr != 0 )
        {
            regerror( regerr, &expr, errbuf, 80 );
            return M_MATH_REGERR;
        }
        so = (int)matches[0].rm_so;
        eo = (int)matches[0].rm_eo;
        if ( so != 0 )
        {
            _args[numargs] = instring;
            instring[so] = 0;
            numargs++;
            if ( numargs == MAX_ARGS )
            {
                // too many args
                return M_MATH_TOOMANYARGS;
            }
        }
        instring += (int)matches[0].rm_eo;
    }

    regfree( &expr );

    regerr = regcomp( &expr, arg_expr_string,
                      REG_EXTENDED | REG_ICASE );

    if ( regerr != 0 )
    {
        regerror( regerr, &expr, errbuf, 80 );
        return M_MATH_REGERR;
    }

    err = M_MATH_OK;

    /*
     * now run regex over each arg to figure out what it is
     * and then apply that arg to the stack. the arg is either:
     * 1. a value to add to the stack
     * 2. a 1-operand operation to perform on the top of stack
     * 3. a 2-operand operation to perform on the top of stack
     * 4. flags to modify operation of output
     * and the regex expr can tell the difference for us.
     */

    for ( args = _args; numargs > 0; args++, numargs-- )
    {
        int arg_type = 0;

        regerr = regexec( &expr, *args, MAX_MATCHES, matches, 0 );
        if ( regerr != 0 )
        {
            regerror( regerr, &expr, errbuf, 80 );
            err = M_MATH_REGERR;
            goto out;
        }

        /* find the first subexpression of the regex 
           which matched */
        for ( i = 1; i < ARG_MAX; i++ )
        {
            if ( (int)matches[i].rm_so != -1 )
            {
                arg_type = i;
                break;
            }
        }

        switch ( arg_type )
        {
        case ARG_FLAG:

            if ( flags == NULL )
            {
                // flag args not allowed
                return M_MATH_NOFLAGS;
            }
            *flags = parse_flags( *args, *flags );
            if ( output_base_B == -1 )
            {
                args++; numargs--;
                output_base_B = atoi(*args);
            }
            break;

        case ARG_BINARY:       case ARG_OCTAL:        
        case ARG_DECIMAL:      case ARG_HEX_IMPL:
        case ARG_HEX_P:        case ARG_HEX_H:
        case ARG_HEX_ASSEM1:   case ARG_HEX_ASSEM2:
        case ARG_ARBITRARY:

            if ( top == STACK_SIZE )
            {
                err = M_MATH_STACKFULL;
                goto out;
            }
            stack[ top ] = parse_value( arg_type, *args );
            if ( errbuf[0] != 0 )
            {
                err = M_MATH_PARSEVALUEERR;
                goto out;
            }
            top++;
            break;

        case ARG_PLUS:    case ARG_MINUS:    case ARG_TIMES:
        case ARG_DIVIDE:  case ARG_RSHFT:    case ARG_LSHFT:
        case ARG_LT:      case ARG_LTEQ:     case ARG_GT:
        case ARG_GTEQ:    case ARG_EQ:       case ARG_MOD:
        case ARG_AND:     case ARG_OR:       case ARG_XOR:

            if ( top < 2 )
            {
                // not enough args for %s
                err = M_MATH_TOOFEWARGS;
                goto out;
            }
            if ( arg_type == ARG_DIVIDE && 
                 stack[ top-1 ] == 0 )
            {
                err = M_MATH_DIVIDE_BY_ZERO;
                goto out;
            }

            stack[ top-2 ] = operation2( arg_type,
                                         stack[ top-2 ],
                                         stack[ top-1 ] );
            if ( errbuf[0] != 0 )
            {
                err = M_MATH_OP2ERR;
                goto out;
            }
            top--;
            break;

        case ARG_NOT:
        case ARG_SWAP32:
        case ARG_SWAP16:

            if ( top == 0 )
            {
                // not enough args for %s
                err = M_MATH_TOOFEWARGS;
                goto out;
            }
            stack[ top-1 ] = operation1( arg_type,
                                         stack[ top-1 ] );
            if ( errbuf[0] != 0 )
            {
                err = M_MATH_OP1ERR;
                goto out;
            }
            break;

        case ARG_ERR:

            // syntax error(ARG_ERR) at '%s'
            err = M_MATH_SYNTAX;
            goto out;

        default:

            // syntax error(default)
            err = M_MATH_SYNTAX;
            goto out;
        }
    }

 out:
    regfree( &expr );

    if ( err == M_MATH_OK )
    {
        ret = stack[0];
        if ( top == 0 )
        {
            err = M_MATH_EMPTYSTACK;
        }
        else if ( top != 1 )
        {
            err = M_MATH_STACKNOTONE;
        }
        if ( flags != NULL  && *flags & FLAGS_32BIT )
            ret &= 0xffffffff;
    }

    *result = ret;
    return err;
}

int
m_parse_number( M_INT64 * result, char * string, int len, int base )
{
    M_INT64 ret = 0;
    int neg = 0;

    if ( len > 0 && *string == '-' )
    {
        neg = 1;
        string++;
        len--;
    }

    while ( len-- > 0 )
    {
        char c;
        int val = 0;

        c = *string++;
        if ( base <= 36 && islower((int)c) )
            c = toupper(c);
        if ( c < '0' )
            return -1;
        if ( c <= '9' )
            val = c - '0';
        else if ( c <= 'Z' )
        {
            if ( c < 'A' )
                return -1;
            val = c - 'A' + 10;
        }
        else if ( c < 'z' )
        {
            if ( c < 'a' )
                return -1;
            val = c - 'a' + 36;
        }
        else
            return -1;

        if ( val >= base )
            return -1;

        ret *= base;
        ret += val;
    }

    if ( neg )
        *result = -ret;
    else
        *result = ret;

    return 0;
}

char *
m_dump_number( M_INT64 num, int base )
{
    static char ret[ 80 ];
    char intret[ 80 ];
    int retpos = 0;
    int intretpos = 0;

    if ( base < 0 )
    {
        M_INT64 bit63 = 1;
        bit63 <<= 63;
        base = -base;
        if ( num & bit63 )
        {
            num ^= 0xffffffffffffffffULL;
            num += 1;
            ret[ retpos++ ] = '-';
        }
    }

    if ( num == 0 )
    {
        ret[ retpos++ ] = '0';
    }
    else
    {
        while ( num != 0 )
        {
            M_INT64 quot = num / base;
            int dig = num % base;
            intret[ intretpos++ ] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz"[ dig ];
            num = quot;
        }
        do {
            ret[ retpos++ ] = intret[ --intretpos ];
        } while ( intretpos != 0 );
    }
    ret[ retpos++ ] = 0;
    return ret;
}

int
m_main( int argc, char ** argv )
{
    enum m_math_retvals err;
    int flags = 0;
    M_INT64 result = 0;

    if ( argc == 1 )
        usage();

    err = m_do_math( argc-1, argv+1, &result, &flags );

    if ( err != M_MATH_OK )
    {
        fprintf( stderr, "error %d\n", err );
        goto out;
    }

    /* establish default for outputs */

    if ( flags == 0 )
        flags = FLAGS_HEX;

    /* dump output */

    if ( flags & FLAGS_HEX_ASSM )
    {
        short upper, lower;
        upper = (result >> 16) & 0xffff;
        lower = result & 0xffff;
        if ( lower < 0 )
            upper += 1;   /* perform the @ha adjustment */
        printf( "0x%x,0x%x\n", upper, lower );
    }
    if ( flags & FLAGS_BASE )
    {
        printf( "%s\n", m_dump_number( result, output_base_B ));
    }
    if ( flags & FLAGS_BINARY )
    {
        printf( "%s\n", m_dump_number( result, 2 ));
    }
    if ( flags & FLAGS_DECIMAL )
    {
        printf( "%s\n", m_dump_number( result, -10 ));
    }
    if ( flags & FLAGS_UDECIMAL )
    {
        printf( "%s\n", m_dump_number( result, 10 ));
    }
    if ( flags & FLAGS_HEX )
    {
        printf( "%s\n", m_dump_number( result, 16 ));
    }
    if ( flags & FLAGS_OCTAL )
    {
        printf( "%s\n", m_dump_number( result, 8 ));
    }

 out:
    return 0;
}
