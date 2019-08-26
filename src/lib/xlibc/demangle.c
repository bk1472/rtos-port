#include "demangle.h"

/*----------------------------------------------------------------------------------
 * Global Control Constants
 *----------------------------------------------------------------------------------*/
#define INPUT_STR_SZ	(256)

/*----------------------------------------------------------------------------------
 * Constants Definitions
 *----------------------------------------------------------------------------------*/
#define DMGL_NO_OPTS						0			/* For readability... */
#define DMGL_PARAMS							(1 << 0)	/* Include function args */
#define DMGL_ANSI							(1 << 1)	/* Include const, volatile, etc */
#define DMGL_VERBOSE						(1 << 3)	/* Include implementation details.  */
#define DMGL_TYPES							(1 << 4)	/* Also try to demangle type encodings.  */
#define DMGL_RET_POSTFIX					(1 << 5)	/* Print function return types (when present) after function signature */

/*----------------------------------------------------------------------------------
 * Macro Function Definitions
 *----------------------------------------------------------------------------------*/
#define IS_DIGIT(c)							((c) >= '0' && (c) <= '9')
#define IS_UPPER(c)							((c) >= 'A' && (c) <= 'Z')
#define IS_LOWER(c)							((c) >= 'a' && (c) <= 'z')

#define d_append_string_constant(dpi, s)	d_append_buffer (dpi, (s), sizeof (s) - 1)
#define d_last_char(dpi)					((dpi)->buf == NULL || (dpi)->len == 0 ? '\0' : (dpi)->buf[(dpi)->len - 1])

#define NL(s) 								s, (sizeof s) - 1

/*----------------------------------------------------------------------------------
 * Local Type Definitions
 *----------------------------------------------------------------------------------*/
typedef enum
{
	COMP_OBJ_CTOR = 1,
	BASE_OBJ_CTOR,
	CMP_OBJ_ALLOC_CTOR
} CTOR_KIND_T;

typedef enum
{
	DELETE_DTOR = 1,
	CMP_OBJ_DTOR,
	BASE_OBJ_DTOR
} DTOR_KIND_T;

typedef enum
{
	DMGL_CMPNT_NAME = 0,
	DMGL_CMPNT_QUAL_NAME,
	DMGL_CMPNT_LOCAL_NAME,
	DMGL_CMPNT_TYPED_NAME,
	DMGL_CMPNT_TEMPLATE,
	DMGL_CMPNT_TEMPLATE_PARAM,
	DMGL_CMPNT_CTOR,
	DMGL_CMPNT_DTOR,
	DMGL_CMPNT_VTABLE,
	DMGL_CMPNT_VTT,
	DMGL_CMPNT_CONSTRUCTION_VTABLE,
	DMGL_CMPNT_TYPEINFO,
	DMGL_CMPNT_TYPEINFO_NAME,
	DMGL_CMPNT_TYPEINFO_FN,
	DMGL_CMPNT_THUNK,
	DMGL_CMPNT_VIRTUAL_THUNK,
	DMGL_CMPNT_COVARIANT_THUNK,
	DMGL_CMPNT_GUARD,
	DMGL_CMPNT_REFTEMP,
	DMGL_CMPNT_HIDDEN_ALIAS,
	DMGL_CMPNT_SUB_STD,
	DMGL_CMPNT_RESTRICT,
	DMGL_CMPNT_VOLATILE,
	DMGL_CMPNT_CONST,
	DMGL_CMPNT_RESTRICT_THIS,
	DMGL_CMPNT_VOLATILE_THIS,
	DMGL_CMPNT_CONST_THIS,
	DMGL_CMPNT_VENDOR_TYPE_QUAL,
	DMGL_CMPNT_POINTER,
	DMGL_CMPNT_REFERENCE,
	DMGL_CMPNT_COMPLEX,
	DMGL_CMPNT_IMAGINARY,
	DMGL_CMPNT_BUILTIN_TYPE,
	DMGL_CMPNT_VENDOR_TYPE,
	DMGL_CMPNT_FUNCTION_TYPE,
	DMGL_CMPNT_ARRAY_TYPE,
	DMGL_CMPNT_PTRMEM_TYPE,
	DMGL_CMPNT_ARGLIST,
	DMGL_CMPNT_TEMPLATE_ARGLIST,
	DMGL_CMPNT_OPERATOR,
	DMGL_CMPNT_EXTENDED_OPERATOR,
	DMGL_CMPNT_CAST,
	DMGL_CMPNT_UNARY,
	DMGL_CMPNT_BINARY,
	DMGL_CMPNT_BINARY_ARGS,
	DMGL_CMPNT_TRINARY,
	DMGL_CMPNT_TRINARY_ARG1,
	DMGL_CMPNT_TRINARY_ARG2,
	DMGL_CMPNT_LITERAL,
	DMGL_CMPNT_LITERAL_NEG
} DEMANGLE_COMP_TYPE_T;

typedef enum
{
	PRNT_MODE_DEFAULT,
	PRNT_MODE_INT,
	PRNT_MODE_UNSIGNED,
	PRNT_MODE_LONG,
	PRNT_MODE_UNSIGNED_LONG,
	PRNT_MODE_LONG_LONG,
	PRNT_MODE_UNSIGNED_LONG_LONG,
	PRNT_MODE_BOOL,
	PRNT_MODE_FLOAT,
	PRNT_MODE_VOID
} BUILTIN_TYPE_PRNT_T;

typedef struct
{
	const char *code;
	const char *name;
	int        len;
	int        args;
} op_info_t;

typedef struct
{
	const char          *name;
	int                 len;
	BUILTIN_TYPE_PRNT_T print;
} builtin_type_t;

typedef struct strct_comp
{
  /* The type of this component.  */
	DEMANGLE_COMP_TYPE_T type;

	union
	{
		/* For DMGL_CMPNT_NAME.  */
		struct
		{
			const char *s;
			int len;
		} s_name;

		/* For DMGL_CMPNT_OPERATOR.  */
		struct
		{
			const op_info_t *op;
		} s_op;

		/* For DMGL_CMPNT_EXTENDED_OPERATOR.  */
		struct
		{
			int args;
			struct strct_comp *name;
		} s_ext_op;

		/* For DMGL_CMPNT_CTOR.  */
		struct
		{
			CTOR_KIND_T kind;
			struct strct_comp *name;
		} s_ctor;

		/* For DMGL_CMPNT_DTOR.  */
		struct
		{
			DTOR_KIND_T kind;
			struct strct_comp *name;
		} s_dtor;

		/* For DMGL_CMPNT_BUILTIN_TYPE.  */
		struct
		{
			const builtin_type_t *type;
		} s_builtin;

		/* For DMGL_CMPNT_SUB_STD.  */
		struct
		{
			const char* string;
			int len;
		} s_str;

		/* For DMGL_CMPNT_TEMPLATE_PARAM.  */
		struct
		{
			long number;
		} s_num;

		/* For other types.  */
		struct
		{
			struct strct_comp *left;
			struct strct_comp *right;
		} s_bin;
	} u;
} comp_t;

typedef struct
{
	const char	*s;
	const char	*send;
	int			options;
	const char	*n;
	comp_t		*comps;
	int			next_comp;
	int			num_comps;
	comp_t		**subs;
	int			next_sub;
	int			num_subs;
	int			did_subs;
	comp_t		*last_name;
	int			expansion;
} info_t;


typedef struct
{
	char		code;
	const char	*s_expansion;
	int			s_len;
	const char	*full_expansion;
	int			full_len;
	const char	*set_last_name;
	int			set_last_name_len;
} d_sub_info_t;

typedef struct dp_template
{
	struct dp_template	*next;
	const comp_t		*template_decl;
} dp_template_t;

typedef struct _dp__mod
{
	struct _dp__mod		*next;
	const comp_t		*mod;
	int					printed;
	dp_template_t		*templates;
} dp_mod_t;

typedef struct
{
	int					options;
	char				*buf;
	size_t				len;
	size_t				alc;
	size_t				inSz;
	dp_template_t		*templates;
	dp_mod_t			*modifiers;
	int					allocation_failure;
} dp_info_t;

/*----------------------------------------------------------------------------------
 * Static Functions Declaration
 *----------------------------------------------------------------------------------*/
static comp_t	*_dmgMkEmpty (info_t *);
static comp_t	*_dmgMkComp (info_t *, DEMANGLE_COMP_TYPE_T,comp_t *,comp_t *);
static comp_t	*_dmgMkName (info_t *, const char *, int);
static comp_t	*_dmgMkSub (info_t *, const char *, int);
static comp_t	*_dmgEncoding (info_t *, int);
static comp_t	*_dmgName (info_t *);
static comp_t	*_dmgPrefix (info_t *);
static comp_t	*_dmgUnqualName (info_t *);
static comp_t	*_dmgSrcName (info_t *);
static comp_t	*_dmgOpName (info_t *);
static comp_t	*_dmgSpName (info_t *);
static comp_t	*_dmgDCTRName (info_t *);
static comp_t	**_dmgCVQual (info_t *, comp_t **, int);
static comp_t	*_dmgBareFuncType (info_t *, int);
static comp_t	*_dmgTemplParam (info_t *);
static comp_t	*_dmgTemplArgs (info_t *);
static comp_t	*_dmgTemplArg (info_t *);
static comp_t	*_dmgExpress (info_t *);
static comp_t	*_dmgExpPrim (info_t *);
static comp_t	*_dmgLocalName (info_t *);
static comp_t	*_dmgSubstit (info_t *, int);
static comp_t	*_dmgDecodeType (info_t *);

static int		_dmgCallOffs (info_t *, int);
static int		_dmgAddSubstit (info_t *, comp_t *);
static int		_dmgHasRetType (comp_t *);
static long		_dmgNum (info_t *);
static int		_Is_DCTOR_Conv (comp_t *);
static void		_dpResize (dp_info_t *, size_t);
static void		_dpAppend_char (dp_info_t *, int);
static void		_dpAppend_buffer (dp_info_t *, const char *, size_t);
static void		_dpError (dp_info_t *);
static void		_dpComp (dp_info_t *, const comp_t *);
static void		_dpModList (dp_info_t *, dp_mod_t *, int);
static void		_dpMode (dp_info_t *, const comp_t *);
static void		_dpFunction_type (dp_info_t *, const comp_t *, dp_mod_t *);
static void		_dpArray_type (dp_info_t *, const comp_t *, dp_mod_t *);
static void		_dpExpr_op (dp_info_t *, const comp_t *);
static void		_dpCast (dp_info_t *, const comp_t *);

/*----------------------------------------------------------------------------------
 * Local Functions Declaration
 *----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------
 * Static local inline function definitions
 *----------------------------------------------------------------------------------*/
static inline char d_peek_char(info_t *di)
{
	return *di->n;
}

static inline char d_peek_next_char(info_t *di)
{
	return di->n[1];
}

static inline void d_advance(info_t *di, int i)
{
	di->n += i;
}

static inline int  d_check_char(info_t *di, char c)
{
	return (d_peek_char(di) == c ? (di->n++, 1) : 0);
}

static inline char  d_next_char(info_t *di)
{
	return (d_peek_char(di) == '\0' ? '\0' : *(di->n++));
}

static inline char*  d_str(info_t *di)
{
	return (char*)(di->n);
}

static inline int dp_saw_error(dp_info_t *dpi)
{
	return (dpi->buf == NULL);
}

static void d_append_char(dp_info_t *dpi, char c)
{
	if (dpi->buf != NULL && dpi->len < dpi->alc)
		dpi->buf[dpi->len++] = c;
	else
		_dpAppend_char (dpi, c);
}

static void d_append_buffer(dp_info_t *dpi, const char *s, size_t l)
{
	if (dpi->buf != NULL && dpi->len + l <= dpi->alc)
	{
		memcpy (dpi->buf + dpi->len, s, l);
		dpi->len += l;
	}
	else
		_dpAppend_buffer (dpi, s, l);
}


/*----------------------------------------------------------------------------------
 * Function Bodies
 *----------------------------------------------------------------------------------*/
static comp_t *_dmgMkEmpty (info_t *di)
{
	comp_t *p;

	if (di->next_comp >= di->num_comps)
		return NULL;
	p = &di->comps[di->next_comp];
	++di->next_comp;

	return p;
}

static comp_t *_dmgMkComp (info_t *di, DEMANGLE_COMP_TYPE_T type, comp_t *left, comp_t *right)
{
	#define MODE_N_LINK			0
	#define MODE_L_LINK			1
	#define MODE_R_LINK			2
	#define MODE_D_LINK			3
	#define DMGL_CMPNT_INVALID	0xFF
	typedef struct
	{
		DEMANGLE_COMP_TYPE_T	type;
		int						mode;
	} lnkMode_T;

	int			i;
	comp_t *p;

	lnkMode_T linkMode[] = {
		{ DMGL_CMPNT_QUAL_NAME,           MODE_D_LINK },{ DMGL_CMPNT_LOCAL_NAME,          MODE_D_LINK },
		{ DMGL_CMPNT_TYPED_NAME,          MODE_D_LINK },{ DMGL_CMPNT_TEMPLATE,            MODE_D_LINK },
		{ DMGL_CMPNT_CONSTRUCTION_VTABLE, MODE_D_LINK },{ DMGL_CMPNT_VENDOR_TYPE_QUAL,    MODE_D_LINK },
		{ DMGL_CMPNT_PTRMEM_TYPE,         MODE_D_LINK },{ DMGL_CMPNT_UNARY,               MODE_D_LINK },
		{ DMGL_CMPNT_BINARY,              MODE_D_LINK },{ DMGL_CMPNT_BINARY_ARGS,         MODE_D_LINK },
		{ DMGL_CMPNT_TRINARY,             MODE_D_LINK },{ DMGL_CMPNT_TRINARY_ARG1,        MODE_D_LINK },
		{ DMGL_CMPNT_TRINARY_ARG2,        MODE_D_LINK },{ DMGL_CMPNT_LITERAL,             MODE_D_LINK },
		{ DMGL_CMPNT_LITERAL_NEG,         MODE_D_LINK },{ DMGL_CMPNT_VTABLE,              MODE_L_LINK },
		{ DMGL_CMPNT_VTT,                 MODE_L_LINK },{ DMGL_CMPNT_TYPEINFO,            MODE_L_LINK },
		{ DMGL_CMPNT_TYPEINFO_NAME,       MODE_L_LINK },{ DMGL_CMPNT_TYPEINFO_FN,         MODE_L_LINK },
		{ DMGL_CMPNT_THUNK,               MODE_L_LINK },{ DMGL_CMPNT_VIRTUAL_THUNK,       MODE_L_LINK },
		{ DMGL_CMPNT_COVARIANT_THUNK,     MODE_L_LINK },{ DMGL_CMPNT_GUARD,               MODE_L_LINK },
		{ DMGL_CMPNT_REFTEMP,             MODE_L_LINK },{ DMGL_CMPNT_HIDDEN_ALIAS,        MODE_L_LINK },
		{ DMGL_CMPNT_POINTER,             MODE_L_LINK },{ DMGL_CMPNT_REFERENCE,           MODE_L_LINK },
		{ DMGL_CMPNT_COMPLEX,             MODE_L_LINK },{ DMGL_CMPNT_IMAGINARY,           MODE_L_LINK },
		{ DMGL_CMPNT_VENDOR_TYPE,         MODE_L_LINK },{ DMGL_CMPNT_ARGLIST,             MODE_L_LINK },
		{ DMGL_CMPNT_TEMPLATE_ARGLIST,    MODE_L_LINK },{ DMGL_CMPNT_CAST,                MODE_L_LINK },
		{ DMGL_CMPNT_ARRAY_TYPE,          MODE_R_LINK },{ DMGL_CMPNT_FUNCTION_TYPE,       MODE_N_LINK },
		{ DMGL_CMPNT_RESTRICT,            MODE_N_LINK },{ DMGL_CMPNT_VOLATILE,            MODE_N_LINK },
		{ DMGL_CMPNT_CONST,               MODE_N_LINK },{ DMGL_CMPNT_RESTRICT_THIS,       MODE_N_LINK },
		{ DMGL_CMPNT_VOLATILE_THIS,       MODE_N_LINK },{ DMGL_CMPNT_CONST_THIS,          MODE_N_LINK },
		{ DMGL_CMPNT_INVALID,             -1          }
	};

	for (i = 0; linkMode[i].type != DMGL_CMPNT_INVALID; i++)
	{
		if ( linkMode[i].type != type)
			continue;
		switch (linkMode[i].mode)
		{
			case MODE_D_LINK: if(left == NULL || right == NULL) return NULL;
							  break;
			case MODE_L_LINK: if(left == NULL                 ) return NULL;
							  break;
			case MODE_R_LINK: if(                right == NULL) return NULL;
							  break;
			case MODE_N_LINK: break;

		}
		p = _dmgMkEmpty (di);
		if (p != NULL)
		{
			p->type = type;
			p->u.s_bin.left = left;
			p->u.s_bin.right = right;
		}
		return p;
	}
	return NULL;
}

static comp_t *_dmgMkName (info_t *di, const char *s, int len)
{
	comp_t *p;

	p = _dmgMkEmpty (di);
	if (p == NULL || s == NULL || len == 0)
		return NULL;
	p->type = DMGL_CMPNT_NAME;
	p->u.s_name.s = s;
	p->u.s_name.len = len;

	return p;
}

static comp_t *_dmgMkSub (info_t *di, const char *name, int len)
{
	comp_t *p;

	p = _dmgMkEmpty (di);
	if (p != NULL)
	{
		p->type = DMGL_CMPNT_SUB_STD;
		p->u.s_str.string = name;
		p->u.s_str.len = len;
	}
	return p;
}

static int _dmgHasRetType (comp_t *dc)
{
	if (dc == NULL)
		return 0;
	switch (dc->type)
	{
		default:
			return 0;
		case DMGL_CMPNT_TEMPLATE:
			return ! _Is_DCTOR_Conv (dc->u.s_bin.left);
		case DMGL_CMPNT_RESTRICT_THIS:
		case DMGL_CMPNT_VOLATILE_THIS:
		case DMGL_CMPNT_CONST_THIS:
			return _dmgHasRetType (dc->u.s_bin.left);
	}
}

static int _Is_DCTOR_Conv (comp_t *dc)
{
	if (dc == NULL)
		return 0;
	switch (dc->type)
	{
		default:
			return 0;
		case DMGL_CMPNT_QUAL_NAME:
		case DMGL_CMPNT_LOCAL_NAME:
			return _Is_DCTOR_Conv (dc->u.s_bin.right);
		case DMGL_CMPNT_CTOR:
		case DMGL_CMPNT_DTOR:
		case DMGL_CMPNT_CAST:
			return 1;
	}
}

static comp_t *_dmgEncoding (info_t *di, int top_level)
{
	char	peek = d_peek_char (di);
	comp_t	*dc;

	if (peek == 'G' || peek == 'T')
	{
		return _dmgSpName (di);
	}

	dc = _dmgName (di);

	if (dc != NULL && top_level && (di->options & DMGL_PARAMS) == 0)
	{
		while (dc->type == DMGL_CMPNT_RESTRICT_THIS
			|| dc->type == DMGL_CMPNT_VOLATILE_THIS
			|| dc->type == DMGL_CMPNT_CONST_THIS)
		{
		  dc = dc->u.s_bin.left;
		}
		if (dc->type == DMGL_CMPNT_LOCAL_NAME)
		{
			comp_t *dcr;

			dcr = dc->u.s_bin.right;
			while (dcr->type == DMGL_CMPNT_RESTRICT_THIS
			  	|| dcr->type == DMGL_CMPNT_VOLATILE_THIS
			  	|| dcr->type == DMGL_CMPNT_CONST_THIS)
			{
			    dcr = dcr->u.s_bin.left;
			}
			dc->u.s_bin.right = dcr;
		}

		return dc;
	}

	peek = d_peek_char (di);
	if (peek == '\0' || peek == 'E')
		return dc;
	return _dmgMkComp (di, DMGL_CMPNT_TYPED_NAME, dc, _dmgBareFuncType (di, _dmgHasRetType (dc)));
}

static comp_t *_dmgName (info_t *di)
{
	char peek = d_peek_char (di);
	comp_t *dc;

	switch (peek)
	{
		case 'N':
		{
			comp_t *comp, **pcomp;

			if (!d_check_char(di, 'N'))
				return NULL;
			if((pcomp = _dmgCVQual(di, &comp, 1)) == NULL)
				return NULL;
			if((*pcomp = _dmgPrefix(di)) == NULL)
				return NULL;
			if (!d_check_char(di, 'E'))
				return NULL;
			return comp;
		}
		case 'Z':
			return _dmgLocalName (di);
		case 'S':
		{
			int subst;

			if (d_peek_next_char (di) != 't')
			{
				dc = _dmgSubstit (di, 0);
				subst = 1;
			}
			else
			{
				d_advance (di, 2);
				dc = _dmgMkComp (di, DMGL_CMPNT_QUAL_NAME,
					      _dmgMkName (di, "std", 3),
					      _dmgUnqualName (di));
				di->expansion += 3;
				subst = 0;
			}

			if (d_peek_char (di) != 'I')
			{
				;
			}
			else
			{
				if (! subst)
				{
					if (! _dmgAddSubstit (di, dc))
						return NULL;
				}
				dc = _dmgMkComp (di, DMGL_CMPNT_TEMPLATE, dc,
					      _dmgTemplArgs (di));
			}

			return dc;
		}
		default:
			dc = _dmgUnqualName (di);
			if (d_peek_char (di) == 'I')
			{
				if (! _dmgAddSubstit (di, dc))
					return NULL;
				dc = _dmgMkComp (di, DMGL_CMPNT_TEMPLATE, dc,
				  					_dmgTemplArgs (di));
			}
			return dc;
	}
}

static comp_t *_dmgPrefix (info_t *di)
{
	comp_t *ret = NULL;

	while (1)
	{
		char peek;
		DEMANGLE_COMP_TYPE_T comb_type;
		comp_t *dc;

		peek = d_peek_char (di);
		if (peek == '\0')
			return NULL;

		comb_type = DMGL_CMPNT_QUAL_NAME;

		if (   IS_DIGIT (peek)
			|| IS_LOWER (peek)
			|| peek == 'C'
			|| peek == 'D')
		{
			dc = _dmgUnqualName (di);
		}
		else if (peek == 'S')
		{
			dc = _dmgSubstit (di, 1);
		}
		else if (peek == 'I')
		{
			if (ret == NULL)
				return NULL;
			comb_type = DMGL_CMPNT_TEMPLATE;
			dc = _dmgTemplArgs (di);
		}
		else if (peek == 'T')
			dc = _dmgTemplParam (di);
		else if (peek == 'E')
			return ret;
		else
			return NULL;

		if (ret == NULL)
			ret = dc;
		else
			ret = _dmgMkComp (di, comb_type, ret, dc);

		if (peek != 'S' && d_peek_char (di) != 'E')
		{
			if (! _dmgAddSubstit (di, ret))
		    	return NULL;
		}
	}
}

static comp_t *_dmgUnqualName (info_t *di)
{
	char peek;

	peek = d_peek_char (di);

	if (IS_DIGIT (peek))
		return _dmgSrcName (di);
	else if (IS_LOWER (peek))
	{
		comp_t *ret;

		ret = _dmgOpName (di);
		if (ret != NULL && ret->type == DMGL_CMPNT_OPERATOR)
			di->expansion += sizeof "operator" + ret->u.s_op.op->len - 2;

		return ret;
	}
	else if (peek == 'C' || peek == 'D')
	{
		return _dmgDCTRName (di);
	}
	return NULL;
}

static comp_t *_dmgSrcName (info_t *di)
{
	long		len;
	const char	*name;
	char		*strPrefix = "_GLOBAL_";
	int			nPrefix    = sizeof ("_GLOBAL_") -1;
	char		*anony_str = "(anonymous namespace)";
	int			anony_len  = sizeof ("(anonymous namespace)") -1;
	comp_t		*tmp       = NULL;

	len = _dmgNum (di);

	if (len <= 0)
		return NULL;

	name = d_str (di);
	if (di->send - name < len)
		return NULL;

	d_advance (di, len);

	if (len >= (int) (nPrefix + 2) && memcmp (name, strPrefix,nPrefix) == 0)
	{
		const char *s;

		s = name + nPrefix;
		if ((*s == '.' || *s == '_' || *s == '$') && s[1] == 'N')
		{
			di->expansion -= len - anony_len;
			tmp = _dmgMkName (di, anony_str, anony_len - 1);
		}
	}

	/* tmp 가 setting 이 되지 않았다면 default set */
	if(tmp == NULL)
		tmp = _dmgMkName (di, name, len);
	di->last_name = tmp;

	return di->last_name;
}

static long _dmgNum (info_t *di)
{
	int negative;
	char peek;
	long ret;

	negative = 0;
	peek = d_peek_char (di);
	if (peek == 'n')
	{
		negative = 1;
		d_advance (di, 1);
		peek = d_peek_char (di);
	}

	ret = 0;
	while (1)
	{
		if (! IS_DIGIT (peek))
		{
			if (negative)
				ret = - ret;
			return ret;
		}
		ret = ret * 10 + peek - '0';
		d_advance (di, 1);
		peek = d_peek_char (di);
	}
}

const op_info_t operator_demangle_tbl[] =
{
	{ "aN", NL ("&="),        2 },{ "aS", NL ("="),         2 },
	{ "aa", NL ("&&"),        2 },{ "ad", NL ("&"),         1 },
	{ "an", NL ("&"),         2 },{ "cl", NL ("()"),        0 },
	{ "cm", NL (","),         2 },{ "co", NL ("~"),         1 },
	{ "dV", NL ("/="),        2 },{ "da", NL ("delete[]"),  1 },
	{ "de", NL ("*"),         1 },{ "dl", NL ("delete"),    1 },
	{ "dv", NL ("/"),         2 },{ "eO", NL ("^="),        2 },
	{ "eo", NL ("^"),         2 },{ "eq", NL ("=="),        2 },
	{ "ge", NL (">="),        2 },{ "gt", NL (">"),         2 },
	{ "ix", NL ("[]"),        2 },{ "lS", NL ("<<="),       2 },
	{ "le", NL ("<="),        2 },{ "ls", NL ("<<"),        2 },
	{ "lt", NL ("<"),         2 },{ "mI", NL ("-="),        2 },
	{ "mL", NL ("*="),        2 },{ "mi", NL ("-"),         2 },
	{ "ml", NL ("*"),         2 },{ "mm", NL ("--"),        1 },
	{ "na", NL ("new[]"),     1 },{ "ne", NL ("!="),        2 },
	{ "ng", NL ("-"),         1 },{ "nt", NL ("!"),         1 },
	{ "nw", NL ("new"),       1 },{ "oR", NL ("|="),        2 },
	{ "oo", NL ("||"),        2 },{ "or", NL ("|"),         2 },
	{ "pL", NL ("+="),        2 },{ "pl", NL ("+"),         2 },
	{ "pm", NL ("->*"),       2 },{ "pp", NL ("++"),        1 },
	{ "ps", NL ("+"),         1 },{ "pt", NL ("->"),        2 },
	{ "qu", NL ("?"),         3 },{ "rM", NL ("%="),        2 },
	{ "rS", NL (">>="),       2 },{ "rm", NL ("%"),         2 },
	{ "rs", NL (">>"),        2 },{ "st", NL ("sizeof "),   1 },
	{ "sz", NL ("sizeof "),   1 },{ NULL, NULL, 0,          0 }
};

static comp_t *_dmgOpName (info_t *di)
{
	char c1;
	char c2;

	c1 = d_next_char (di);
	c2 = d_next_char (di);
	if (c1 == 'v' && IS_DIGIT (c2))
	{
		comp_t *comp;
		comp_t *name = _dmgSrcName (di);

		comp = _dmgMkEmpty(di);
		if (comp == NULL || (c2 - '0') < 0 || name == NULL)
			return NULL;
		comp->type                       = DMGL_CMPNT_EXTENDED_OPERATOR;
		comp->u.s_ext_op.args = (c2 - '0');
		comp->u.s_ext_op.name = name;
		return comp;
	}
	else if (c1 == 'c' && c2 == 'v')
	{
		return _dmgMkComp (di, DMGL_CMPNT_CAST, _dmgDecodeType (di), NULL);
	}
	else
	{
		int low = 0;
		int high = ((sizeof (operator_demangle_tbl)/ sizeof (operator_demangle_tbl[0]))- 1);

		while (1)
		{
			int i;
			const op_info_t *p;

			i = low + (high - low) / 2;
			p = operator_demangle_tbl + i;

			if (c1 == p->code[0] && c2 == p->code[1])
			{
				comp_t *comp;
				comp = _dmgMkEmpty(di);
				if(comp != NULL)
				{
					comp->type = DMGL_CMPNT_OPERATOR;
					comp->u.s_op.op = p;
				}
				return comp;
			}
			if (c1 < p->code[0] || (c1 == p->code[0] && c2 < p->code[1]))
				high = i;
			else
				low = i + 1;

			if (low == high)
				return NULL;
		}
	}
}

static comp_t *_dmgSpName (info_t *di)
{
	di->expansion += 20;

	if (d_check_char (di, 'T'))
	{
		switch (d_next_char (di))
		{
			case 'V':
				di->expansion -= 5;
				return _dmgMkComp (di, DMGL_CMPNT_VTABLE, _dmgDecodeType (di), NULL);
			case 'T':
				di->expansion -= 10;
				return _dmgMkComp (di, DMGL_CMPNT_VTT, _dmgDecodeType (di), NULL);
			case 'I':
				return _dmgMkComp (di, DMGL_CMPNT_TYPEINFO, _dmgDecodeType (di), NULL);
			case 'S':
				return _dmgMkComp (di, DMGL_CMPNT_TYPEINFO_NAME, _dmgDecodeType (di), NULL);

			case 'h':
				if (! _dmgCallOffs (di, 'h'))
					return NULL;
				return _dmgMkComp (di, DMGL_CMPNT_THUNK, _dmgEncoding (di, 0/*!top_level*/), NULL);

			case 'v':
				if (! _dmgCallOffs (di, 'v'))
					return NULL;
				return _dmgMkComp (di, DMGL_CMPNT_VIRTUAL_THUNK, _dmgEncoding (di, 0/*!top_level*/), NULL);

			case 'c':
				if (! _dmgCallOffs (di, '\0'))
					return NULL;
				if (! _dmgCallOffs (di, '\0'))
					return NULL;

				return _dmgMkComp (di, DMGL_CMPNT_COVARIANT_THUNK, _dmgEncoding (di, 0/*!top_level*/), NULL);

			case 'C':
			{
				comp_t *derived_type;
				long offset;
				comp_t *base_type;

				derived_type = _dmgDecodeType (di);
				offset = _dmgNum (di);

				if (offset < 0)
					return NULL;
				if (! d_check_char (di, '_'))
					return NULL;
				base_type = _dmgDecodeType (di);
				di->expansion += 5;

				return _dmgMkComp (di, DMGL_CMPNT_CONSTRUCTION_VTABLE, base_type, derived_type);
			}

			case 'F':
				return _dmgMkComp (di, DMGL_CMPNT_TYPEINFO_FN, _dmgDecodeType (di), NULL);
		}
		return NULL;
	}
	else if (d_check_char (di, 'G'))
	{
		switch (d_next_char (di))
		{
			case 'V':
				return _dmgMkComp (di, DMGL_CMPNT_GUARD, _dmgName (di), NULL);
			case 'R':
				return _dmgMkComp (di, DMGL_CMPNT_REFTEMP, _dmgName (di), NULL);
			case 'A':
				return _dmgMkComp (di, DMGL_CMPNT_HIDDEN_ALIAS, _dmgEncoding (di, 0/*!top_level*/), NULL);
		}
		return NULL;
	}

	return NULL;
}

static int _dmgCallOffs (info_t *di, int c)
{
	if (c == '\0')
		c = d_next_char (di);

	if (c == 'h')
		_dmgNum (di);
	else if (c == 'v')
	{
		_dmgNum (di);
		if (! d_check_char (di, '_'))
			return 0;
		_dmgNum (di);
	}
	else
		return 0;

	if (! d_check_char (di, '_'))
		return 0;

	return 1;
}

static comp_t *_dmgDCTRName (info_t *di)
{
	if (di->last_name != NULL)
	{
		if (di->last_name->type == DMGL_CMPNT_NAME)
			di->expansion += di->last_name->u.s_name.len;
		else if (di->last_name->type == DMGL_CMPNT_SUB_STD)
			di->expansion += di->last_name->u.s_str.len;
	}
	switch (d_peek_char (di))
	{
		case 'C':
		{
			CTOR_KIND_T kind;
			comp_t *comp;
			switch (d_peek_next_char (di))
			{
				case '1':
					kind = COMP_OBJ_CTOR;
					break;
				case '2':
					kind = BASE_OBJ_CTOR;
					break;
				case '3':
					kind = CMP_OBJ_ALLOC_CTOR;
					break;
				default:
					return NULL;
			}
			d_advance (di, 2);
			comp = _dmgMkEmpty(di);
			if (      comp       == NULL
				|| di->last_name == NULL
				|| (kind < COMP_OBJ_CTOR
				 && kind > CMP_OBJ_ALLOC_CTOR)
			   )
			{
				return NULL;
			}
			comp->type = DMGL_CMPNT_CTOR;
			comp->u.s_ctor.kind = kind;
			comp->u.s_ctor.name = di->last_name;
			return comp;
		}
		case 'D':
		{
			DTOR_KIND_T kind;
			comp_t *comp;

			switch (d_peek_next_char (di))
			{
				case '0':
					kind = DELETE_DTOR;
					break;
				case '1':
					kind = CMP_OBJ_DTOR;
					break;
				case '2':
					kind = BASE_OBJ_DTOR;
					break;
				default:
					return NULL;
			}
			d_advance (di, 2);
			comp = _dmgMkEmpty (di);
			if (   comp          == NULL
				|| di->last_name == NULL
				|| (kind < DELETE_DTOR
				&& kind > BASE_OBJ_DTOR)
			   )
			{
				return NULL;
			}
			comp->type = DMGL_CMPNT_DTOR;
			comp->u.s_dtor.kind = kind;
			comp->u.s_dtor.name = di->last_name;
			return comp;
		}
		default:
		  return NULL;
	}
}

const builtin_type_t cplus_demangle_builtin_types[] =
{
	/* a */ { NL ("signed char"),        PRNT_MODE_DEFAULT            },
	/* b */ { NL ("bool"),               PRNT_MODE_BOOL               },
	/* c */ { NL ("char"),               PRNT_MODE_DEFAULT            },
	/* d */ { NL ("double"),             PRNT_MODE_FLOAT              },
	/* e */ { NL ("long double"),        PRNT_MODE_FLOAT              },
	/* f */ { NL ("float"),              PRNT_MODE_FLOAT              },
	/* g */ { NL ("__float128"),         PRNT_MODE_FLOAT              },
	/* h */ { NL ("unsigned char"),      PRNT_MODE_DEFAULT            },
	/* i */ { NL ("int"),                PRNT_MODE_INT                },
	/* j */ { NL ("unsigned int"),       PRNT_MODE_UNSIGNED           },
	/* k */ { NULL, 0,                   PRNT_MODE_DEFAULT            },
	/* l */ { NL ("long"),               PRNT_MODE_LONG               },
	/* m */ { NL ("unsigned long"),      PRNT_MODE_UNSIGNED_LONG      },
	/* n */ { NL ("__int128"),           PRNT_MODE_DEFAULT            },
	/* o */ { NL ("unsigned __int128"),  PRNT_MODE_DEFAULT            },
	/* p */ { NULL, 0,                   PRNT_MODE_DEFAULT            },
	/* q */ { NULL, 0,                   PRNT_MODE_DEFAULT            },
	/* r */ { NULL, 0,                   PRNT_MODE_DEFAULT            },
	/* s */ { NL ("short"),              PRNT_MODE_DEFAULT            },
	/* t */ { NL ("unsigned short"),     PRNT_MODE_DEFAULT            },
	/* u */ { NULL, 0,                   PRNT_MODE_DEFAULT            },
	/* v */ { NL ("void"),               PRNT_MODE_VOID               },
	/* w */ { NL ("wchar_t"),            PRNT_MODE_DEFAULT            },
	/* x */ { NL ("long long"),          PRNT_MODE_LONG_LONG          },
	/* y */ { NL ("unsigned long long"), PRNT_MODE_UNSIGNED_LONG_LONG },
	/* z */ { NL ("..."),                PRNT_MODE_DEFAULT            },
};

static comp_t *_dmgDecodeType (info_t *di)
{
	char peek;
	comp_t *ret;
	int can_subst;

	peek = d_peek_char (di);
	if (peek == 'r' || peek == 'V' || peek == 'K')
	{
		comp_t **pret;

		pret = _dmgCVQual (di, &ret, 0);
		if (pret == NULL)
			return NULL;
		*pret = _dmgDecodeType (di);
		if (! _dmgAddSubstit (di, ret))
			return NULL;

		return ret;
	}

	can_subst = 1;

	switch (peek)
	{
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
		case 'h': case 'i': case 'j':           case 'l': case 'm': case 'n':
		case 'o':                               case 's': case 't':
		case 'v': case 'w': case 'x': case 'y': case 'z':
			if((ret = _dmgMkEmpty(di)) == NULL)
				return NULL;
			ret->type = DMGL_CMPNT_BUILTIN_TYPE;
			ret->u.s_builtin.type = &cplus_demangle_builtin_types[peek - 'a'];
			di->expansion += ret->u.s_builtin.type->len;
			can_subst = 0;
			d_advance (di, 1);
			break;
		case 'u':
			d_advance (di, 1);
			ret = _dmgMkComp (di, DMGL_CMPNT_VENDOR_TYPE,_dmgSrcName (di), NULL);
			break;
		case 'F':
		{
			if (! d_check_char (di, 'F'))
			{
				ret = NULL;
				break;
			}
			if (d_peek_char (di) == 'Y')
			{
				d_advance (di, 1);
			}
			ret = _dmgBareFuncType (di, 1);

			if (! d_check_char (di, 'E'))
			{
				ret = NULL;
				break;;
			}
			break;
		}
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case 'N':
		case 'Z':
			ret = _dmgName (di);
			break;
		case 'A':
		{
			char peek_next;
			comp_t *dim;

			if (! d_check_char (di, 'A'))
			{
				ret = NULL;
				break;
			}

			peek_next = d_peek_char (di);
			if (peek_next == '_')
				dim = NULL;
			else if (IS_DIGIT (peek_next))
			{
				const char *s;

				s = d_str (di);
				do
				{
					d_advance (di, 1);
					peek_next = d_peek_char (di);
				}
				while (IS_DIGIT (peek_next));
				dim = _dmgMkName (di, s, d_str (di) - s);
				if (dim == NULL)
				{
					ret = NULL;
					break;
				}
			}
			else
			{
				dim = _dmgExpress (di);
				if (dim == NULL)
				{
					ret = NULL;
					break;
				}
			}

			if (! d_check_char (di, '_'))
			{
				ret = NULL;
				break;
			}

			ret = _dmgMkComp (di, DMGL_CMPNT_ARRAY_TYPE, dim, _dmgDecodeType (di));
			break;
		}
		case 'M':
		{
			comp_t *cl;
			comp_t *mem;
			comp_t **pmem;

			if (! d_check_char (di, 'M'))
			{
				ret = NULL;
				break;
			}

			cl = _dmgDecodeType (di);
			pmem = _dmgCVQual (di, &mem, 1);

			if (pmem == NULL)
			{
				ret = NULL;
				break;
			}
			*pmem = _dmgDecodeType (di);
			if (pmem != &mem && (*pmem)->type != DMGL_CMPNT_FUNCTION_TYPE)
			{
				if (! _dmgAddSubstit (di, mem))
				{
					ret = NULL;
					break;
				}
			}

			ret = _dmgMkComp (di, DMGL_CMPNT_PTRMEM_TYPE, cl, mem);

			break;
		}
		case 'T':
			ret = _dmgTemplParam (di);
			if (d_peek_char (di) == 'I')
			{
				if (! _dmgAddSubstit (di, ret))
					return NULL;
				ret = _dmgMkComp (di, DMGL_CMPNT_TEMPLATE, ret,_dmgTemplArgs (di));
			}
			break;
		case 'S':
		{
			char peek_next;

			peek_next = d_peek_next_char (di);
			if (IS_DIGIT (peek_next)
			    || peek_next == '_'
			    || IS_UPPER (peek_next))
			{
				ret = _dmgSubstit (di, 0);
				if (d_peek_char (di) == 'I')
					ret = _dmgMkComp (di, DMGL_CMPNT_TEMPLATE, ret,_dmgTemplArgs (di));
				else
					can_subst = 0;
			}
			else
			{
				ret = _dmgName (di);
				if (ret != NULL && ret->type == DMGL_CMPNT_SUB_STD)
					can_subst = 0;
			}
		}
			break;

		case 'P':
			d_advance (di, 1);
			ret = _dmgMkComp (di, DMGL_CMPNT_POINTER,
			  	 _dmgDecodeType (di), NULL);
			break;

		case 'R':
			d_advance (di, 1);
			ret = _dmgMkComp (di, DMGL_CMPNT_REFERENCE,
			  	 _dmgDecodeType (di), NULL);
			break;

		case 'C':
			d_advance (di, 1);
			ret = _dmgMkComp (di, DMGL_CMPNT_COMPLEX,
			  	 _dmgDecodeType (di), NULL);
			break;

		case 'G':
			d_advance (di, 1);
			ret = _dmgMkComp (di, DMGL_CMPNT_IMAGINARY,
			  	 _dmgDecodeType (di), NULL);
			break;

		case 'U':
			d_advance (di, 1);
			ret = _dmgSrcName (di);
			ret = _dmgMkComp (di, DMGL_CMPNT_VENDOR_TYPE_QUAL,
			  	 _dmgDecodeType (di), ret);
			break;

		default:
			return NULL;
	  }

	if (can_subst)
	{
		if (! _dmgAddSubstit (di, ret))
			return NULL;
	}

  return ret;
}

static comp_t **_dmgCVQual (info_t *di, comp_t **pret, int member_fn)
{
	char peek;

	peek = d_peek_char (di);
	while (peek == 'r' || peek == 'V' || peek == 'K')
	{
		DEMANGLE_COMP_TYPE_T t;

		d_advance (di, 1);
		if (peek == 'r')
		{
			t = (member_fn
			     ? DMGL_CMPNT_RESTRICT_THIS
			     : DMGL_CMPNT_RESTRICT);
			di->expansion += sizeof "restrict";
		}
		else if (peek == 'V')
		{
			t = (member_fn
			     ? DMGL_CMPNT_VOLATILE_THIS
			     : DMGL_CMPNT_VOLATILE);
			di->expansion += sizeof "volatile";
		}
		else
		{
			t = (member_fn
			     ? DMGL_CMPNT_CONST_THIS
			     : DMGL_CMPNT_CONST);
			di->expansion += sizeof "const";
		}

		*pret = _dmgMkComp (di, t, NULL, NULL);
		if (*pret == NULL)
			return NULL;
		pret = &((*pret)->u.s_bin.left);

		peek = d_peek_char (di);
	}

	return pret;
}

static comp_t *_dmgBareFuncType (info_t *di, int _dmgHasRetType)
{
	comp_t *return_type;
	comp_t *tl;
	comp_t **ptl;
	char peek;

	peek = d_peek_char (di);
	if (peek == 'J')
	{
		d_advance (di, 1);
		_dmgHasRetType = 1;
	}

	return_type = NULL;
	tl = NULL;
	ptl = &tl;
	while (1)
	{
		comp_t *type;

		peek = d_peek_char (di);
		if (peek == '\0' || peek == 'E')
			break;
		type = _dmgDecodeType (di);
		if (type == NULL)
			return NULL;
		if (_dmgHasRetType)
		{
			return_type = type;
			_dmgHasRetType = 0;
		}
		else
		{
			*ptl = _dmgMkComp (di, DMGL_CMPNT_ARGLIST, type, NULL);
			if (*ptl == NULL)
				return NULL;
			ptl = &((*ptl)->u.s_bin.right);
		}
	}

	if (tl == NULL)
		return NULL;

	if (   (tl->u.s_bin.right == NULL)
	    && (tl->u.s_bin.left)->type == DMGL_CMPNT_BUILTIN_TYPE
	    && (tl->u.s_bin.left)->u.s_builtin.type->print == PRNT_MODE_VOID)
	{
		di->expansion -= (tl->u.s_bin.left)->u.s_builtin.type->len;
		tl = NULL;
	}

	return _dmgMkComp (di, DMGL_CMPNT_FUNCTION_TYPE, return_type, tl);
}

static comp_t *_dmgTemplParam (info_t *di)
{
	long		param;
	comp_t	*comp;

	if (! d_check_char (di, 'T'))
		return NULL;

	if (d_peek_char (di) == '_')
		param = 0;
	else
	{
		param = _dmgNum (di);
		if (param < 0)
			return NULL;
		param += 1;
	}

	if (! d_check_char (di, '_'))
		return NULL;

	++di->did_subs;
	comp = _dmgMkEmpty(di);
	if (comp == NULL)
		return NULL;
	comp->type = DMGL_CMPNT_TEMPLATE_PARAM;
	comp->u.s_num.number = param;

	return comp;
}

static comp_t *_dmgTemplArgs (info_t *di)
{
	comp_t *hold_last_name;
	comp_t *al;
	comp_t **pal;

	hold_last_name = di->last_name;

	if (! d_check_char (di, 'I'))
		return NULL;

	al = NULL;
	pal = &al;
	while (1)
	{
		comp_t *a;

		a = _dmgTemplArg (di);
		if (a == NULL)
			return NULL;

		*pal = _dmgMkComp (di, DMGL_CMPNT_TEMPLATE_ARGLIST, a, NULL);
		if (*pal == NULL)
			return NULL;
		pal = &((*pal)->u.s_bin.right);

		if (d_peek_char (di) == 'E')
		{
			d_advance (di, 1);
			break;
		}
	}

	di->last_name = hold_last_name;

	return al;
}

static comp_t *_dmgTemplArg (info_t *di)
{
	comp_t *ret;

	switch (d_peek_char (di))
	{
		case 'X':
			d_advance (di, 1);
			ret = _dmgExpress (di);
			if (! d_check_char (di, 'E'))
				return NULL;
			return ret;

		case 'L':
			return _dmgExpPrim (di);

		default:
			return _dmgDecodeType (di);
	}
}

static comp_t *_dmgExpress (info_t *di)
{
	char peek;

	peek = d_peek_char (di);
	if (peek == 'L')
		return _dmgExpPrim (di);
	else if (peek == 'T')
		return _dmgTemplParam (di);
	else if (peek == 's' && d_peek_next_char (di) == 'r')
	{
		comp_t *type;
		comp_t *name;

		d_advance (di, 2);
		type = _dmgDecodeType (di);
		name = _dmgUnqualName (di);
		if (d_peek_char (di) != 'I')
			return _dmgMkComp (di, DMGL_CMPNT_QUAL_NAME, type, name);
		else
			return _dmgMkComp (di, DMGL_CMPNT_QUAL_NAME, type, _dmgMkComp (di, DMGL_CMPNT_TEMPLATE, name, _dmgTemplArgs (di)));
	}
	else
	{
		 comp_t *op;
		 int args;

		 op = _dmgOpName (di);
		if (op == NULL)
			return NULL;

		if (op->type == DMGL_CMPNT_OPERATOR)
			di->expansion += op->u.s_op.op->len - 2;

		if (op->type == DMGL_CMPNT_OPERATOR
			&& strcmp (op->u.s_op.op->code, "st") == 0)
		{
			return _dmgMkComp (di, DMGL_CMPNT_UNARY, op, _dmgDecodeType (di));
		}

		switch (op->type)
		{
			default:
				return NULL;
			case DMGL_CMPNT_OPERATOR:
				args = op->u.s_op.op->args;
				break;
			case DMGL_CMPNT_EXTENDED_OPERATOR:
				args = op->u.s_ext_op.args;
				break;
			case DMGL_CMPNT_CAST:
				args = 1;
				break;
		}

		switch (args)
		{
			case 1:
				return _dmgMkComp (di, DMGL_CMPNT_UNARY, op,_dmgExpress (di));
			case 2:
			{
				comp_t *left;

				left = _dmgExpress (di);
				return _dmgMkComp (di, DMGL_CMPNT_BINARY, op,
						_dmgMkComp (di,DMGL_CMPNT_BINARY_ARGS,left,_dmgExpress (di)));
			}
			case 3:
			{
				comp_t *first;
				comp_t *second;

				first = _dmgExpress (di);
				second = _dmgExpress (di);
				return _dmgMkComp (di, DMGL_CMPNT_TRINARY, op,
						_dmgMkComp (di,DMGL_CMPNT_TRINARY_ARG1,first,
									_dmgMkComp (di, DMGL_CMPNT_TRINARY_ARG2, second, _dmgExpress (di))));
			}
			default:
			  return NULL;
		}
	}
}

static comp_t *_dmgExpPrim (info_t *di)
{
	comp_t *ret;

	if (! d_check_char (di, 'L'))
		return NULL;
	if (d_peek_char (di) == '_')
	{
		if (! d_check_char (di, '_')) return NULL;
		if (! d_check_char (di, 'Z')) return NULL;
		ret = _dmgEncoding (di, 0/*!top_level*/);
	}
	else
	{
		comp_t *type;
		DEMANGLE_COMP_TYPE_T t;
		const char *s;

		type = _dmgDecodeType (di);
		if (type == NULL)
			return NULL;

		if (     type->type == DMGL_CMPNT_BUILTIN_TYPE
				&& type->u.s_builtin.type->print != PRNT_MODE_DEFAULT)
		{
		  di->expansion -= type->u.s_builtin.type->len;
		}

		t = DMGL_CMPNT_LITERAL;
		if (d_peek_char (di) == 'n')
		{
			t = DMGL_CMPNT_LITERAL_NEG;
			d_advance (di, 1);
		}
		s = d_str (di);
		while (d_peek_char (di) != 'E')
		{
			if (d_peek_char (di) == '\0')
				return NULL;
			d_advance (di, 1);
		}
		ret = _dmgMkComp (di, t, type, _dmgMkName (di, s, d_str (di) - s));
	}

	if (! d_check_char (di, 'E'))
		return NULL;

	return ret;
}

static comp_t *_dmgLocalName (info_t *di)
{
	comp_t *function;

	if (! d_check_char (di, 'Z'))
		return NULL;

	function = _dmgEncoding (di, 0/*!top_level*/);

	if (! d_check_char (di, 'E'))
		return NULL;

	if (d_peek_char (di) == 's')
	{
		d_advance (di, 1);
		if (d_peek_char (di) == '_')
		{
			d_advance (di, 1);
			if( _dmgNum (di) < 0)
				return NULL;
		}

		return _dmgMkComp (di, DMGL_CMPNT_LOCAL_NAME, function,_dmgMkName (di, "string literal", sizeof "string literal" - 1));
	}
	else
	{
		comp_t *name;

		name = _dmgName (di);
		if (d_peek_char (di) == '_')
		{
			d_advance (di, 1);
			if( _dmgNum (di) < 0)
				return NULL;
		}

		return _dmgMkComp (di, DMGL_CMPNT_LOCAL_NAME, function, name);
	}
}

static int _dmgAddSubstit (info_t *di, comp_t *dc)
{
	if (dc == NULL)
		return 0;

	if (di->next_sub >= di->num_subs)
		return 0;

	di->subs[di->next_sub] = dc;
	++di->next_sub;
	return 1;
}

static const d_sub_info_t standard_subs[] =
{
  { 't', NL ("std"),               NL ("std"), NULL, 0 },
  { 'a', NL ("std::allocator"),    NL ("std::allocator"), NL ("allocator") },
  { 'b', NL ("std::basic_string"), NL ("std::basic_string"), NL ("basic_string") },
  { 's', NL ("std::string"),       NL ("std::basic_string<char, std::char_traits<char>, std::allocator<char> >"), NL ("basic_string") },
  { 'i', NL ("std::istream"),      NL ("std::basic_istream<char, std::char_traits<char> >"), NL ("basic_istream") },
  { 'o', NL ("std::ostream"),      NL ("std::basic_ostream<char, std::char_traits<char> >"), NL ("basic_ostream") },
  { 'd', NL ("std::iostream"),     NL ("std::basic_iostream<char, std::char_traits<char> >"), NL ("basic_iostream") }
};

static comp_t *_dmgSubstit (info_t *di, int prefix)
{
	char c;

	if (! d_check_char (di, 'S'))
		return NULL;

	c = d_next_char (di);
	if (c == '_' || IS_DIGIT (c) || IS_UPPER (c))
	{
		int id;

		id = 0;
		if (c != '_')
		{
			do
			{
				if (IS_DIGIT (c))
					id = id * 36 + c - '0';
				else if (IS_UPPER (c))
					id = id * 36 + c - 'A' + 10;
				else
					return NULL;

				if (id < 0)
					return NULL;
				c = d_next_char (di);
			}
			while (c != '_');
			++id;
		}

		if (id >= di->next_sub)
			return NULL;

		++di->did_subs;

		return di->subs[id];
	}
	else
	{
		int verbose;
		const d_sub_info_t *p;
		const d_sub_info_t *pend;

		verbose = (di->options & DMGL_VERBOSE) != 0;
		if (! verbose && prefix)
		{
			char peek;

			peek = d_peek_char (di);
			if (peek == 'C' || peek == 'D')
				verbose = 1;
		}

		pend = (&standard_subs[0] + sizeof standard_subs / sizeof standard_subs[0]);
		for (p = &standard_subs[0]; p < pend; ++p)
		{
			if (c == p->code)
			{
				const char *s;
				int len;

				if (p->set_last_name != NULL)
					di->last_name = _dmgMkSub (di, p->set_last_name, p->set_last_name_len);
				if (verbose)
				{
					s = p->full_expansion;
					len = p->full_len;
				}
				else
				{
					s = p->s_expansion;
					len = p->s_len;
				}
				di->expansion += len;

				return _dmgMkSub (di, s, len);
			}
		}

	  return NULL;
	}
}

static void _dpResize (dp_info_t *dpi, size_t add)
{
	size_t need;

	if (dpi->buf == NULL)
		return;
	need = dpi->len + add;

	if(dpi->inSz != 0 && need > dpi->alc)
	{
		dpi->buf = NULL;
		return;
	}
	while (need > dpi->alc)
	{
		size_t newalc;
		char *newbuf;

		newalc = dpi->alc * 2;
		newbuf = (char *) realloc (dpi->buf, newalc);
		if (newbuf == NULL)
		{
			free (dpi->buf);
			dpi->buf = NULL;
			dpi->allocation_failure = 1;
			return;
		}
		dpi->buf = newbuf;
		dpi->alc = newalc;
	}
}

static void _dpAppend_char (dp_info_t *dpi, int c)
{
	if (dpi->buf != NULL)
	{
		if (dpi->len >= dpi->alc)
		{
			_dpResize (dpi, 1);
			if (dpi->buf == NULL)
				return;
		}

		dpi->buf[dpi->len] = c;
		++dpi->len;
	}
}

static void _dpAppend_buffer (dp_info_t *dpi, const char *s, size_t l)
{
	if (dpi->buf != NULL)
	{
		if (dpi->len + l > dpi->alc)
		{
			_dpResize (dpi, l);

			if (dpi->buf == NULL)
				return;
		}

		memcpy (dpi->buf + dpi->len, s, l);
		dpi->len += l;
	}
}

static void _dpError (dp_info_t *dpi)
{
	free (dpi->buf);
	dpi->buf = NULL;
}

char *make_demangle_print ( int options, const comp_t *dc,
							int estimate, size_t *palc,
							char **ppRetBuf, size_t inSz)
{
	dp_info_t dpi;

	dpi.options = options;

	dpi.alc = estimate + 1;
	if (*ppRetBuf == NULL || inSz == 0)
	{
		dpi.buf  = (char *) malloc (dpi.alc);
	}
	else
	{
		if (dpi.alc >= inSz)
			dpi.alc = inSz;

		dpi.buf  = (char *) *ppRetBuf;
		dpi.inSz = inSz;
	}
	if (dpi.buf == NULL)
	{
		*palc = 1;
		return NULL;
	}

	dpi.len = 0;
	dpi.templates = NULL;
	dpi.modifiers = NULL;

	dpi.allocation_failure = 0;

	_dpComp (&dpi, dc);

	d_append_char (&dpi, '\0');

	if (dpi.buf != NULL)
		*palc = dpi.alc;
	else
		*palc = dpi.allocation_failure;

	return dpi.buf;
}

static void _dpComp (dp_info_t *dpi, const comp_t *dc)
{
	if (dc == NULL)
	{
		_dpError (dpi);
		return;
	}

	if (dp_saw_error (dpi))
    	return;

	switch (dc->type)
	{
		case DMGL_CMPNT_NAME:
			d_append_buffer (dpi, dc->u.s_name.s, dc->u.s_name.len);
			return;

		case DMGL_CMPNT_QUAL_NAME:
		case DMGL_CMPNT_LOCAL_NAME:
			_dpComp (dpi, dc->u.s_bin.left);
			d_append_string_constant (dpi, "::");
    		_dpComp (dpi, dc->u.s_bin.right);
    		return;
		case DMGL_CMPNT_TYPED_NAME:
		{
			dp_mod_t *hold_modifiers;
			comp_t *type_dmgName;
			dp_mod_t adpm[4];
			unsigned int i;
			dp_template_t dpt;

			hold_modifiers = dpi->modifiers;
			i = 0;
			type_dmgName = dc->u.s_bin.left;
			while (type_dmgName != NULL)
			{
				if (i >= sizeof adpm / sizeof adpm[0])
				{
					_dpError (dpi);
					return;
		  		}

				adpm[i].next = dpi->modifiers;
				dpi->modifiers = &adpm[i];
				adpm[i].mod = type_dmgName;
				adpm[i].printed = 0;
				adpm[i].templates = dpi->templates;
				++i;

				if (type_dmgName->type != DMGL_CMPNT_RESTRICT_THIS
					&& type_dmgName->type != DMGL_CMPNT_VOLATILE_THIS
					&& type_dmgName->type != DMGL_CMPNT_CONST_THIS)
					break;
				type_dmgName = type_dmgName->u.s_bin.left;
			}

			if(type_dmgName == NULL)
			{
				_dpError (dpi);
				return;
			}

			if (type_dmgName->type == DMGL_CMPNT_TEMPLATE)
			{
				dpt.next = dpi->templates;
				dpi->templates = &dpt;
				dpt.template_decl = type_dmgName;
			}

			if (type_dmgName->type == DMGL_CMPNT_LOCAL_NAME)
			{
				comp_t *local_name;

				local_name = type_dmgName->u.s_bin.right;
				while (local_name->type == DMGL_CMPNT_RESTRICT_THIS
					|| local_name->type == DMGL_CMPNT_VOLATILE_THIS
					|| local_name->type == DMGL_CMPNT_CONST_THIS)
				{
					if (i >= sizeof adpm / sizeof adpm[0])
					{
						_dpError (dpi);
						return;
					}
					adpm[i] = adpm[i - 1];
					adpm[i].next = &adpm[i - 1];
					dpi->modifiers = &adpm[i];

					adpm[i - 1].mod = local_name;
					adpm[i - 1].printed = 0;
					adpm[i - 1].templates = dpi->templates;
					++i;

					local_name = local_name->u.s_bin.left;
				}
			}

			_dpComp (dpi, dc->u.s_bin.right);

			if (type_dmgName->type == DMGL_CMPNT_TEMPLATE)
				dpi->templates = dpt.next;

			while (i > 0)
			{
				--i;
				if (! adpm[i].printed)
				{
					d_append_char (dpi, ' ');
					_dpMode (dpi, adpm[i].mod);
				}
			}

			dpi->modifiers = hold_modifiers;

			return;
		}

		case DMGL_CMPNT_TEMPLATE:
		{
			dp_mod_t *hold_dpm;

			hold_dpm = dpi->modifiers;
			dpi->modifiers = NULL;

			_dpComp (dpi, dc->u.s_bin.left);

			if (d_last_char (dpi) == '<')
				d_append_char (dpi, ' ');
			d_append_char (dpi, '<');
			_dpComp (dpi, dc->u.s_bin.right);

			if (d_last_char (dpi) == '>')
				d_append_char (dpi, ' ');
			d_append_char (dpi, '>');

			dpi->modifiers = hold_dpm;
			return;
		}

		case DMGL_CMPNT_TEMPLATE_PARAM:
		{
			long i;
			comp_t *a;
			dp_template_t *hold_dpt;

			if (dpi->templates == NULL)
			{
				_dpError (dpi);
				return;
			}
			i = dc->u.s_num.number;
			for (a = (dpi->templates->template_decl)->u.s_bin.right;
			     a != NULL;
			     a = a->u.s_bin.right)
			{
				if (a->type != DMGL_CMPNT_TEMPLATE_ARGLIST)
				{
					_dpError (dpi);
					return;
				}

				if (i <= 0)
					break;
				--i;
			}
			if (i != 0 || a == NULL)
			{
				_dpError (dpi);
				return;
			}

			hold_dpt = dpi->templates;
			dpi->templates = hold_dpt->next;

			_dpComp (dpi, a->u.s_bin.left);

			dpi->templates = hold_dpt;

			return;
      	}

		case DMGL_CMPNT_CTOR:
			_dpComp (dpi, dc->u.s_ctor.name);
			return;

		case DMGL_CMPNT_DTOR:
			d_append_char (dpi, '~');
			_dpComp (dpi, dc->u.s_dtor.name);
			return;

		case DMGL_CMPNT_VTABLE:
			d_append_string_constant (dpi, "vtable for ");
			_dpComp (dpi, dc->u.s_bin.left);
			return;

		case DMGL_CMPNT_VTT:
			d_append_string_constant (dpi, "VTT for ");
			_dpComp (dpi, dc->u.s_bin.left);
			return;

		case DMGL_CMPNT_CONSTRUCTION_VTABLE:
			d_append_string_constant (dpi, "construction vtable for ");
			_dpComp (dpi, dc->u.s_bin.left);
			d_append_string_constant (dpi, "-in-");
			_dpComp (dpi, dc->u.s_bin.right);
			return;

		case DMGL_CMPNT_TYPEINFO:
			d_append_string_constant (dpi, "typeinfo for ");
			_dpComp (dpi, dc->u.s_bin.left);
			return;

		case DMGL_CMPNT_TYPEINFO_NAME:
			d_append_string_constant (dpi, "typeinfo name for ");
			_dpComp (dpi, dc->u.s_bin.left);
			return;

		case DMGL_CMPNT_TYPEINFO_FN:
			d_append_string_constant (dpi, "typeinfo fn for ");
			_dpComp (dpi, dc->u.s_bin.left);
			return;

		case DMGL_CMPNT_THUNK:
			d_append_string_constant (dpi, "non-virtual thunk to ");
			_dpComp (dpi, dc->u.s_bin.left);
			return;

		case DMGL_CMPNT_VIRTUAL_THUNK:
			d_append_string_constant (dpi, "virtual thunk to ");
			_dpComp (dpi, dc->u.s_bin.left);
			return;

		case DMGL_CMPNT_COVARIANT_THUNK:
			d_append_string_constant (dpi, "covariant return thunk to ");
			_dpComp (dpi, dc->u.s_bin.left);
			return;

		case DMGL_CMPNT_GUARD:
			d_append_string_constant (dpi, "guard variable for ");
			_dpComp (dpi, dc->u.s_bin.left);
			return;

		case DMGL_CMPNT_REFTEMP:
			d_append_string_constant (dpi, "reference temporary for ");
			_dpComp (dpi, dc->u.s_bin.left);
			return;

		case DMGL_CMPNT_HIDDEN_ALIAS:
			d_append_string_constant (dpi, "hidden alias for ");
			_dpComp (dpi, dc->u.s_bin.left);
			return;

		case DMGL_CMPNT_SUB_STD:
			d_append_buffer (dpi, dc->u.s_str.string, dc->u.s_str.len);
			return;

		case DMGL_CMPNT_RESTRICT:
		case DMGL_CMPNT_VOLATILE:
		case DMGL_CMPNT_CONST:
		{
			dp_mod_t *pdpm;

			for (pdpm = dpi->modifiers; pdpm != NULL; pdpm = pdpm->next)
			{
				if (! pdpm->printed)
				{
					if (   pdpm->mod->type != DMGL_CMPNT_RESTRICT
					    && pdpm->mod->type != DMGL_CMPNT_VOLATILE
					    && pdpm->mod->type != DMGL_CMPNT_CONST)
						break;
					if (pdpm->mod->type == dc->type)
					{
						_dpComp (dpi, dc->u.s_bin.left);
						return;
					}
				}
			}
		}
		  /* Fall through.  */
		case DMGL_CMPNT_RESTRICT_THIS:
		case DMGL_CMPNT_VOLATILE_THIS:
		case DMGL_CMPNT_CONST_THIS:
		case DMGL_CMPNT_VENDOR_TYPE_QUAL:
		case DMGL_CMPNT_POINTER:
		case DMGL_CMPNT_REFERENCE:
		case DMGL_CMPNT_COMPLEX:
		case DMGL_CMPNT_IMAGINARY:
		{
			/* We keep a list of modifiers on the stack.  */
			dp_mod_t dpm;

			dpm.next = dpi->modifiers;
			dpi->modifiers = &dpm;
			dpm.mod = dc;
			dpm.printed = 0;
			dpm.templates = dpi->templates;

			_dpComp (dpi, dc->u.s_bin.left);

			if (! dpm.printed)
				_dpMode (dpi, dc);

			dpi->modifiers = dpm.next;

			return;
		}

		case DMGL_CMPNT_BUILTIN_TYPE:
			d_append_buffer (dpi, dc->u.s_builtin.type->name, dc->u.s_builtin.type->len);
			return;

		case DMGL_CMPNT_VENDOR_TYPE:
			_dpComp (dpi, dc->u.s_bin.left);
			return;

		case DMGL_CMPNT_FUNCTION_TYPE:
		{
			if ((dpi->options & DMGL_RET_POSTFIX) != 0)
				_dpFunction_type (dpi, dc, dpi->modifiers);

			if (dc->u.s_bin.left != NULL)
			{
				dp_mod_t dpm;

				dpm.next = dpi->modifiers;
				dpi->modifiers = &dpm;
				dpm.mod = dc;
				dpm.printed = 0;
				dpm.templates = dpi->templates;

				_dpComp (dpi, dc->u.s_bin.left);

				dpi->modifiers = dpm.next;

				if (dpm.printed)
					return;

				if ((dpi->options & DMGL_RET_POSTFIX) == 0)
					d_append_char (dpi, ' ');
			}

			if ((dpi->options & DMGL_RET_POSTFIX) == 0)
				_dpFunction_type (dpi, dc, dpi->modifiers);
			return;
		}

		case DMGL_CMPNT_ARRAY_TYPE:
		{
			dp_mod_t *hold_modifiers;
			dp_mod_t adpm[4];
			unsigned int i;
			dp_mod_t *pdpm;

			hold_modifiers = dpi->modifiers;

			adpm[0].next = hold_modifiers;
			dpi->modifiers = &adpm[0];
			adpm[0].mod = dc;
			adpm[0].printed = 0;
			adpm[0].templates = dpi->templates;

			i = 1;
			pdpm = hold_modifiers;
			while (pdpm != NULL
			       &&(pdpm->mod->type == DMGL_CMPNT_RESTRICT
				   || pdpm->mod->type == DMGL_CMPNT_VOLATILE
				   || pdpm->mod->type == DMGL_CMPNT_CONST))
			{
				if (! pdpm->printed)
				{
					if (i >= sizeof adpm / sizeof adpm[0])
					{
						_dpError (dpi);
						return;
					}
					adpm[i] = *pdpm;
					adpm[i].next = dpi->modifiers;
					dpi->modifiers = &adpm[i];
					pdpm->printed = 1;
					++i;
				}

				pdpm = pdpm->next;
			}

			_dpComp (dpi, dc->u.s_bin.right);
			dpi->modifiers = hold_modifiers;

			if (adpm[0].printed)
				return;

			while (i > 1)
			{
				--i;
				_dpMode (dpi, adpm[i].mod);
			}
			_dpArray_type (dpi, dc, dpi->modifiers);
			return;
		}

		case DMGL_CMPNT_PTRMEM_TYPE:
		{
			dp_mod_t dpm;

			dpm.next = dpi->modifiers;
			dpi->modifiers = &dpm;
			dpm.mod = dc;
			dpm.printed = 0;
			dpm.templates = dpi->templates;

			_dpComp (dpi, dc->u.s_bin.right);

			if (! dpm.printed)
			{
				d_append_char (dpi, ' ');
				_dpComp (dpi, dc->u.s_bin.left);
				d_append_string_constant (dpi, "::*");
			}
			dpi->modifiers = dpm.next;
			return;
		}

		case DMGL_CMPNT_ARGLIST:
		case DMGL_CMPNT_TEMPLATE_ARGLIST:
			_dpComp (dpi, dc->u.s_bin.left);
			if (dc->u.s_bin.right != NULL)
			{
				d_append_string_constant (dpi, ", ");
				_dpComp (dpi, dc->u.s_bin.right);
			}
			return;

		case DMGL_CMPNT_OPERATOR:
		{
			char c;

			d_append_string_constant (dpi, "operator");
			c = dc->u.s_op.op->name[0];
			if (IS_LOWER (c))
				d_append_char (dpi, ' ');
			d_append_buffer (dpi, dc->u.s_op.op->name,dc->u.s_op.op->len);
			return;
		}

		case DMGL_CMPNT_EXTENDED_OPERATOR:
			d_append_string_constant (dpi, "operator ");
			_dpComp (dpi, dc->u.s_ext_op.name);
			return;

		case DMGL_CMPNT_CAST:
			d_append_string_constant (dpi, "operator ");
			_dpCast (dpi, dc);
			return;

		case DMGL_CMPNT_UNARY:
			if (dc->u.s_bin.left->type != DMGL_CMPNT_CAST)
				_dpExpr_op (dpi, dc->u.s_bin.left);
			else
			{
				d_append_char (dpi, '(');
				_dpCast (dpi, dc->u.s_bin.left);
				d_append_char (dpi, ')');
			}
			d_append_char (dpi, '(');
			_dpComp (dpi, dc->u.s_bin.right);
			d_append_char (dpi, ')');
			return;

		case DMGL_CMPNT_BINARY:
			if ((dc->u.s_bin.right)->type != DMGL_CMPNT_BINARY_ARGS)
			{
				_dpError (dpi);
				return;
			}
			if (dc->u.s_bin.left->type == DMGL_CMPNT_OPERATOR
			&& dc->u.s_bin.left->u.s_op.op->len == 1
			&& dc->u.s_bin.left->u.s_op.op->name[0] == '>')
			{
				d_append_char (dpi, '(');
			}
			d_append_char (dpi, '(');
			_dpComp (dpi, (dc->u.s_bin.right)->u.s_bin.left);
			d_append_string_constant (dpi, ") ");
			_dpExpr_op (dpi, dc->u.s_bin.left);
			d_append_string_constant (dpi, " (");
			_dpComp (dpi, (dc->u.s_bin.right)->u.s_bin.right);
			d_append_char (dpi, ')');

			if (dc->u.s_bin.left->type == DMGL_CMPNT_OPERATOR
			&& dc->u.s_bin.left->u.s_op.op->len == 1
			&& dc->u.s_bin.left->u.s_op.op->name[0] == '>')
			{
				d_append_char (dpi, ')');
			}

			return;

		case DMGL_CMPNT_BINARY_ARGS:
			_dpError (dpi);
			return;

		case DMGL_CMPNT_TRINARY:
			if (dc->u.s_bin.right->type != DMGL_CMPNT_TRINARY_ARG1
			||((dc->u.s_bin.right)->u.s_bin.right)->type != DMGL_CMPNT_TRINARY_ARG2)
			{
				_dpError (dpi);
				return;
			}
			d_append_char (dpi, '(');
			_dpComp (dpi, (dc->u.s_bin.right)->u.s_bin.left);
			d_append_string_constant (dpi, ") ");
			_dpExpr_op (dpi, dc->u.s_bin.left);
			d_append_string_constant (dpi, " (");
			_dpComp (dpi, ((dc->u.s_bin.right)->u.s_bin.right)->u.s_bin.left);
			d_append_string_constant (dpi, ") : (");
			_dpComp (dpi,((dc->u.s_bin.right)->u.s_bin.right)->u.s_bin.right);
			d_append_char (dpi, ')');
			return;

		case DMGL_CMPNT_TRINARY_ARG1:
		case DMGL_CMPNT_TRINARY_ARG2:
			_dpError (dpi);
			return;

		case DMGL_CMPNT_LITERAL:
		case DMGL_CMPNT_LITERAL_NEG:
		{
			BUILTIN_TYPE_PRNT_T tp;

			/* For some builtin types, produce simpler output.  */
			tp = PRNT_MODE_DEFAULT;
			if (dc->u.s_bin.left->type == DMGL_CMPNT_BUILTIN_TYPE)
			{
				tp = dc->u.s_bin.left->u.s_builtin.type->print;
				switch (tp)
				{
					case PRNT_MODE_INT:
					case PRNT_MODE_UNSIGNED:
					case PRNT_MODE_LONG:
					case PRNT_MODE_UNSIGNED_LONG:
					case PRNT_MODE_LONG_LONG:
					case PRNT_MODE_UNSIGNED_LONG_LONG:
						if (dc->u.s_bin.right->type == DMGL_CMPNT_NAME)
						{
							if (dc->type == DMGL_CMPNT_LITERAL_NEG)
								d_append_char (dpi, '-');
							_dpComp (dpi, dc->u.s_bin.right);
							switch (tp)
							{
								default:break;
								case PRNT_MODE_UNSIGNED:           d_append_char (dpi, 'u');break;
								case PRNT_MODE_LONG:               d_append_char (dpi, 'l');break;
								case PRNT_MODE_UNSIGNED_LONG:      d_append_string_constant (dpi, "ul"); break;
								case PRNT_MODE_LONG_LONG:          d_append_string_constant (dpi, "ll"); break;
								case PRNT_MODE_UNSIGNED_LONG_LONG: d_append_string_constant (dpi, "ull");break;
							}
							return;
						}
						break;

					case PRNT_MODE_BOOL:
						if (   dc->u.s_bin.right->type == DMGL_CMPNT_NAME
							&& dc->u.s_bin.right->u.s_name.len == 1
							&& dc->type == DMGL_CMPNT_LITERAL)
						{
							switch (dc->u.s_bin.right->u.s_name.s[0])
							{
								case '0':
									d_append_string_constant (dpi, "false");
									return;
								case '1':
									d_append_string_constant (dpi, "true");
									return;
								default:
									break;
							}
						}
						break;

	      			default: break;
				}
	  		}

			d_append_char (dpi, '(');
			_dpComp (dpi, dc->u.s_bin.left);
			d_append_char (dpi, ')');

			if (dc->type == DMGL_CMPNT_LITERAL_NEG)
				d_append_char (dpi, '-');

			if (tp == PRNT_MODE_FLOAT)
				d_append_char (dpi, '[');
			_dpComp (dpi, dc->u.s_bin.right);

			if (tp == PRNT_MODE_FLOAT)
	  			d_append_char (dpi, ']');
      	}
      	return;

		default:
			_dpError (dpi);
			return;
    }
}

static void _dpModList (dp_info_t *dpi, dp_mod_t *mods, int suffix)
{
	dp_template_t *hold_dpt;

	if (mods == NULL || dp_saw_error (dpi))
		return;

	if (mods->printed
			|| (! suffix && (mods->mod->type == DMGL_CMPNT_RESTRICT_THIS
	        || mods->mod->type == DMGL_CMPNT_VOLATILE_THIS
	        || mods->mod->type == DMGL_CMPNT_CONST_THIS)))
	{
		_dpModList (dpi, mods->next, suffix);
		return;
	}
	mods->printed = 1;

	hold_dpt = dpi->templates;
	dpi->templates = mods->templates;

	if (mods->mod->type == DMGL_CMPNT_FUNCTION_TYPE)
	{
		_dpFunction_type (dpi, mods->mod, mods->next);
		dpi->templates = hold_dpt;
		return;
	}
	else if (mods->mod->type == DMGL_CMPNT_ARRAY_TYPE)
	{
		_dpArray_type (dpi, mods->mod, mods->next);
		dpi->templates = hold_dpt;
		return;
	}
	else if (mods->mod->type == DMGL_CMPNT_LOCAL_NAME)
	{
		dp_mod_t *hold_modifiers;
		comp_t *dc;

		hold_modifiers = dpi->modifiers;
		dpi->modifiers = NULL;
		_dpComp (dpi, (mods->mod)->u.s_bin.left);
		dpi->modifiers = hold_modifiers;

		d_append_string_constant (dpi, "::");

		dc = (mods->mod)->u.s_bin.right;
		while (dc->type == DMGL_CMPNT_RESTRICT_THIS
		   || dc->type == DMGL_CMPNT_VOLATILE_THIS
		   || dc->type == DMGL_CMPNT_CONST_THIS)
		{
			dc = dc->u.s_bin.left;
		}
		_dpComp (dpi, dc);
		dpi->templates = hold_dpt;
		return;
	}

	_dpMode (dpi, mods->mod);

	dpi->templates = hold_dpt;

	_dpModList (dpi, mods->next, suffix);
}

static void _dpMode (dp_info_t *dpi, const comp_t *mod)
{
	switch (mod->type)
	{
		case DMGL_CMPNT_RESTRICT:
		case DMGL_CMPNT_RESTRICT_THIS:
			d_append_string_constant (dpi, " restrict");
			return;
		case DMGL_CMPNT_VOLATILE:
		case DMGL_CMPNT_VOLATILE_THIS:
			d_append_string_constant (dpi, " volatile");
			return;
		case DMGL_CMPNT_CONST:
		case DMGL_CMPNT_CONST_THIS:
			d_append_string_constant (dpi, " const");
			return;
		case DMGL_CMPNT_VENDOR_TYPE_QUAL:
			d_append_char (dpi, ' ');
			_dpComp (dpi, mod->u.s_bin.right);
			return;
		case DMGL_CMPNT_POINTER:
			d_append_char (dpi, '*');
		 	return;
		case DMGL_CMPNT_REFERENCE:
			d_append_char (dpi, '&');
			return;
		case DMGL_CMPNT_COMPLEX:
			d_append_string_constant (dpi, "complex ");
			return;
		case DMGL_CMPNT_IMAGINARY:
			d_append_string_constant (dpi, "imaginary ");
			return;
		case DMGL_CMPNT_PTRMEM_TYPE:
			if (d_last_char (dpi) != '(')
				d_append_char (dpi, ' ');
			_dpComp (dpi, mod->u.s_bin.left);
			d_append_string_constant (dpi, "::*");
			return;
		case DMGL_CMPNT_TYPED_NAME:
			_dpComp (dpi, mod->u.s_bin.left);
			return;
		default:
			_dpComp (dpi, mod);
			return;
	}
}

static void _dpFunction_type (dp_info_t *dpi, const comp_t *dc, dp_mod_t *mods)
{
	int				need_paren;
	int				saw_mod;
	int				need_space;
	dp_mod_t	*p;
	dp_mod_t	*hold_modifiers;

	need_paren = 0;
	saw_mod = 0;
	need_space = 0;

	for (p = mods; p != NULL; p = p->next)
	{
		if (p->printed)
			break;

		saw_mod = 1;
		switch (p->mod->type)
		{
			case DMGL_CMPNT_POINTER:
			case DMGL_CMPNT_REFERENCE:
				need_paren = 1;
				break;
			case DMGL_CMPNT_RESTRICT:
			case DMGL_CMPNT_VOLATILE:
			case DMGL_CMPNT_CONST:
			case DMGL_CMPNT_VENDOR_TYPE_QUAL:
			case DMGL_CMPNT_COMPLEX:
			case DMGL_CMPNT_IMAGINARY:
			case DMGL_CMPNT_PTRMEM_TYPE:
				need_space = 1;
				need_paren = 1;
				break;
			case DMGL_CMPNT_RESTRICT_THIS:
			case DMGL_CMPNT_VOLATILE_THIS:
			case DMGL_CMPNT_CONST_THIS:
				break;
			default:
				break;
		}
		if (need_paren)
		break;
	}

	if (dc->u.s_bin.left != NULL && ! saw_mod)
		need_paren = 1;

	if (need_paren)
	{
		if (! need_space)
		{
			if (d_last_char (dpi) != '(' && d_last_char (dpi) != '*')
			need_space = 1;
		}

		if (need_space && d_last_char (dpi) != ' ')
			d_append_char (dpi, ' ');
		d_append_char (dpi, '(');
	}

	hold_modifiers = dpi->modifiers;
	dpi->modifiers = NULL;

	_dpModList (dpi, mods, 0);

	if (need_paren)
		d_append_char (dpi, ')');

	d_append_char (dpi, '(');

	if (dc->u.s_bin.right != NULL)
		_dpComp (dpi, dc->u.s_bin.right);

	d_append_char (dpi, ')');

	_dpModList (dpi, mods, 1);

	dpi->modifiers = hold_modifiers;
}

static void _dpArray_type (dp_info_t *dpi, const comp_t *dc, dp_mod_t *mods)
{
	int need_space;

	need_space = 1;
	if (mods != NULL)
	{
		int need_paren;
		dp_mod_t *p;

		need_paren = 0;
		for (p = mods; p != NULL; p = p->next)
		{
			if (p->printed)
				continue;

			if (p->mod->type == DMGL_CMPNT_ARRAY_TYPE)
			{
				need_space = 0;
				break;
			}
			else
			{
				need_paren = 1;
				need_space = 1;
				break;
			}
		}

		if (need_paren)
			d_append_string_constant (dpi, " (");

		_dpModList (dpi, mods, 0);

		if (need_paren)
			d_append_char (dpi, ')');
	}

	if (need_space)
		d_append_char (dpi, ' ');

	d_append_char (dpi, '[');

	if (dc->u.s_bin.left != NULL)
		_dpComp (dpi, dc->u.s_bin.left);

	d_append_char (dpi, ']');
}

static void _dpExpr_op (dp_info_t *dpi, const comp_t *dc)
{
	if (dc->type == DMGL_CMPNT_OPERATOR)
		d_append_buffer (dpi, dc->u.s_op.op->name,
						 dc->u.s_op.op->len);
	else
		_dpComp (dpi, dc);
}

static void _dpCast (dp_info_t *dpi, const comp_t *dc)
{
	dp_mod_t *hold_dpm;
	dp_template_t dpt;

 	if ((dc->u.s_bin.left)->type != DMGL_CMPNT_TEMPLATE)
	{
 		_dpComp (dpi, dc->u.s_bin.left);
		return;
	}

	hold_dpm          = dpi->modifiers;
	dpi->modifiers    = NULL;

	dpt.next          = dpi->templates;
	dpi->templates    = &dpt;
	dpt.template_decl = dc->u.s_bin.left;

	_dpComp (dpi, (dc->u.s_bin.left)->u.s_bin.left);

	dpi->templates = dpt.next;

	if (d_last_char (dpi) == '<')
		d_append_char (dpi, ' ');

	d_append_char (dpi, '<');
	_dpComp (dpi, (dc->u.s_bin.left)->u.s_bin.right);

	if (d_last_char (dpi) == '>')
		d_append_char (dpi, ' ');

	d_append_char (dpi, '>');
	dpi->modifiers = hold_dpm;
}

char *local_demangle (const char* mangled, char *retBuf, size_t input_sz)
{
	int			options = DMGL_PARAMS|DMGL_ANSI|DMGL_TYPES;
	//int			type = 0;
	size_t		len;
	info_t		di;
	comp_t		*dc;
	int			estimate;
	char		globl_ch = 0;
	char		*mangle_seed;
	size_t		alc;
	char		*demangle_buf = NULL;

	alc = 0;

	mangle_seed = (char*)mangled;

	if (strncmp (mangle_seed, "_GLOBAL_", 8) == 0
	     && (mangle_seed[8] == '.' || mangle_seed[8] == '_' || mangle_seed[8] == '$')
	     && (mangle_seed[9] == 'D' || mangle_seed[9] == 'I')
	     && mangle_seed[10] == '_')
	{
		globl_ch    = mangle_seed[9];
		mangle_seed = &mangle_seed[11];
	}

	len = strlen (mangle_seed);

	di.s         = mangle_seed;
	di.send      = mangle_seed + len;
	di.options   = options;
	di.n         = mangle_seed;
	di.num_comps = 2 * len;
	di.next_comp = 0;
	di.num_subs  = len;
	di.next_sub  = 0;
	di.did_subs  = 0;
	di.last_name = NULL;
	di.expansion = 0;
	{
		comp_t comps[di.num_comps];
		comp_t *subs[di.num_subs];

		di.comps = &comps[0];
		di.subs  = &subs[0];

		//if (! type)
		{
			if (! d_check_char (&di, '_')) dc = NULL;
			if (! d_check_char (&di, 'Z')) dc = NULL;

			dc = _dmgEncoding (&di, 1/*top_level*/);
		}
		//else
		//	dc = _dmgDecodeType (&di);

		if (((options & DMGL_PARAMS) != 0) && d_peek_char (&di) != '\0')
			dc = NULL;

		estimate = len + di.expansion + 10 * di.did_subs;
		estimate += estimate / 8;

		if (dc != NULL)
			demangle_buf = make_demangle_print (options, dc, estimate, &alc, &retBuf, input_sz);
	}

  return demangle_buf;
}

char *demangle_symbol (const char* mangled, char *dmgBuf, int size)
{
	char	*demangle = NULL;
	char	inputStr[INPUT_STR_SZ];
	char	*cp;

	if(NULL == mangled || NULL == dmgBuf)
		return NULL;

	if (mangled[0] == '_' && mangled[1] == 'Z')
	{
		;
	}
	else if (strncmp (mangled, "_GLOBAL_", 8) == 0
	     && (mangled[8] == '.' || mangled[8] == '_' || mangled[8] == '$')
	     && (mangled[9] == 'D' || mangled[9] == 'I')
	     &&  mangled[10] == '_')
	{
		;
	}
	else
	{
		return NULL;
	}


	snprintf(&inputStr[0], INPUT_STR_SZ, "%s",mangled);
	cp = strchr (&inputStr[0], '@');
	if(cp != NULL)
		*cp = '\0';

	demangle = local_demangle(&inputStr[0], dmgBuf, size);

	if(demangle == NULL)
	{
		return NULL;
	}

	return demangle;
}
