// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include "xml.h"
#include "memory.h"

#ifdef _DEBUG
# include <assert.h>
#endif

#ifdef _DEBUG
# define ASSERT(X) assert((X))
#else
# define ASSERT(X)
#endif

#ifdef _MSC_VER
# define INLINE _inline
#else
# define INLINE inline
#endif

#ifndef TRUE
# define TRUE (!(0))
#endif

#ifndef FALSE
# define FALSE (0)
#endif

#define LINE_FEED				(char)0x0a			// all line endings are normalized to LF
#define LF LINE_FEED
#define CARRIAGE_RETURN			(char)0x0d		// CR gets filtered out
#define CR CARRIAGE_RETURN
#define SINGLE_QUOTE			'\''
#define DOUBLE_QUOTE			'\"'

#define XML_UTF_LEAD_0 0xefU
#define XML_UTF_LEAD_1 0xbbU
#define XML_UTF_LEAD_2 0xbfU

#ifdef __cplusplus
extern "C" {
#endif

static const char* XmlElementGetName(XML_ELEMENT* element);
static void XmlElementSetName(XML_ELEMENT* element, const char* str, int static_mem);

static void XmlDocumentSetError(XML_DOCUMENT* document, eXML_ERROR error, const char* str1, const char* str2);

static int XmlPrinterVisitEnterDocument(XML_PRINTER* printer, const XML_DOCUMENT* document);
static int XmlPrinterVisitEnterElement(XML_PRINTER* printer, const XML_ELEMENT* element, const XML_ATTRIBUTE* attribute);
static int XmlPrinterVisitExitElement(XML_PRINTER* printer, const XML_ELEMENT* element);
static int XmlPrinterVisitText(XML_PRINTER* printer, const XML_TEXT* text);
static int XmlPrinterVisitComment(XML_PRINTER* printer, const XML_COMMENT* comment);
static int XmlPrinterVisitDeclaration(XML_PRINTER* printer, const XML_DECLARATION* declaration);
static int XmlPrinterVisitUnknown(XML_PRINTER* printer, const XML_UNKNOWN* unknown);

typedef struct _ENTITY
{
	const char* pattern;
	int length;
	char value;
} ENTITY;

#define NUM_ENTITIES 5
static const ENTITY g_entities[] =
{
	{"quot", 4,	DOUBLE_QUOTE},
	{"amp", 3,		'&'},
	{"apos", 4,	SINGLE_QUOTE},
	{"lt",	2, 		'<'},
	{"gt",	2,		'>'}
};

static void DummyFuncNoReturn(void* dummy)
{
}

static int DummyFuncReturnTrue(void* dummy1, void* dummy2)
{
	return TRUE;
}

static int DummyFuncReturnTrue2(void* dummy1, void* dummy2, void* dummy3)
{
	return TRUE;
}

static void* DummyFuncReturnNull(void* dummy)
{
	return NULL;
}

static void* ReturnThisFunc(void* data)
{
	return data;
}

static void ConvertUTF32ToUTF8(unsigned long input, char* output, int* length)
{
#define BYTE_MASK 0xBF
#define BYTE_MARK 0x80
	const unsigned long FIRST_BYTE_MARK[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

	if(input < 0x80)
	{
		*length = 1;
	}
	else if(input < 0x800)
	{
		*length = 2;
	}
	else if(input < 0x10000)
	{
		*length = 3;
	}
	else if(input < 0x200000)
	{
		*length = 4;
	}
	else
	{
		*length = 0;    // This code won't covert this correctly anyway.
		return;
	}

	output += *length;

	// Scary scary fall throughs.
	switch(*length)
	{
		case 4:
			--output;
			*output = (char)((input | BYTE_MARK) & BYTE_MASK);
			input >>= 6;
		case 3:
			--output;
			*output = (char)((input | BYTE_MARK) & BYTE_MASK);
			input >>= 6;
		case 2:
			--output;
			*output = (char)((input | BYTE_MARK) & BYTE_MASK);
			input >>= 6;
		case 1:
			--output;
			*output = (char)(input | FIRST_BYTE_MARK[*length]);
		default:
			break;
	}
#undef BYTE_MASK
#undef BYTE_MARK
}

static const char* GetCharacterRef(const char* p, char* value, int* length)
{
	const char *q;

	// Presume an entity, and pull it out.
	*length = 0;

	if(*(p+1) == '#' && *(p+2))
	{
		unsigned long ucs = 0;
		ptrdiff_t delta = 0;
		unsigned int mult = 1;

		if( *(p+2) == 'x')
		{
			// 16進数
			if(*(p+3) == 0)
			{
				return 0;
			}

			q = p+3;
			q = strchr(q, ';');

			if(q == NULL || *q == 0)
			{
				return 0;
			}

			delta = q-p;
			--q;

			while(*q != 'x')
			{
				if(*q >= '0' && *q <= '9')
				{
					ucs += mult * (*q - '0');
				}
				else if(*q >= 'a' && *q <= 'f')
				{
					ucs += mult * (*q - 'a' + 10);
				}
				else if(*q >= 'A' && *q <= 'F')
				{
					ucs += mult * (*q - 'A' + 10 );
				}
				else
				{
					return 0;
				}
				mult *= 16;
				--q;
			}
		}
		else
		{
			// 10進数
			if(*(p+2) == 0)
			{
				return 0;
			}

			q = p+2;
			q = strchr( q, ';' );

			if( q == NULL || *q == 0)
			{
				return 0;
			}

			delta = q-p;
			--q;

			while(*q != '#')
			{
				if(*q >= '0' && *q <= '9')
				{
					ucs += mult * (*q - '0');
				}
				else
				{
					return 0;
				}
				mult *= 10;
				--q;
			}
		}
		// convert the UCS to UTF-8
		ConvertUTF32ToUTF8(ucs, value, length);
		return p + delta + 1;
	}
	return p+1;
}

static INLINE int IsUTF8Continuation(const char p)
{
	return (p & 0x80) != 0;
}

static const char* SkipWhiteSpace( const char* p)
{
	while(IsUTF8Continuation(*p) == FALSE && isspace(*(unsigned char*)(p)))
	{
		++p;
	}
	return p;
}

static int IsWhiteSpace(char p)
{
	return (IsUTF8Continuation(p) == 0) && isspace((unsigned char)(p));
}

static INLINE int StringEqual(const char* p, const char* q, int n_char)
{
	int n = 0;
	if(p == q)
	{
		return TRUE;
	}

	if(n_char < 0)
	{
		n_char = INT_MAX;
	}

	while( *p && *q && *p == *q && n<n_char)
	{
		++p;
		++q;
		++n;
	}
	if((n == n_char) || (*p == 0 && *q == 0))
	{
		return TRUE;
	}
	return FALSE;
}

static INLINE int IsNameStartChar(unsigned char ch)
{
	return ((ch < 128) ? isalpha(ch) : 1)
		|| ch == ':'
		|| ch == '_';
}

static INLINE int IsNameChar(unsigned char ch)
{
	return IsNameStartChar( ch )
		|| isdigit( ch )
		|| ch == '.'
		|| ch == '-';
}

static const char* ReadBOM(const char* p, int* bom)
{
	const unsigned char *pu;
	*bom = FALSE;
	pu = (const unsigned char*)(p);
	// Check for BOM:
	if(*(pu+0) == XML_UTF_LEAD_0
			&& *(pu+1) == XML_UTF_LEAD_1
			&& *(pu+2) == XML_UTF_LEAD_2)
	{
		*bom = TRUE;
		p += 3;
	}
	return p;
}

void SignedToString(int v, char* buffer)
{
	(void)sprintf(buffer, "%d", v);
}

void UnsignedToString(unsigned v, char* buffer)
{
	(void)sprintf(buffer, "%u", v);
}

void BooleanToString(int v, char* buffer)
{
	(void)sprintf(buffer, "%d", v != 0 ? 1 : 0);
}

void FloatToString(float v, char* buffer)
{
	(void)sprintf(buffer, "%.8g", v);
}

void DoubleToString(double v, char* buffer)
{
	(void)sprintf(buffer, "%.17g", v);
}

int StringToSigned(const char* str, int* value)
{
	if(sscanf(str, "%d", value) == 1)
	{
		return TRUE;
	}
	return FALSE;
}

int StringToUnsigned(const char* str, unsigned *value)
{
	if(sscanf(str, "%u", value) == 1)
	{
		return TRUE;
	}
	return FALSE;
}

int StringToBool(const char* str, int* value )
{
	int ival = 0;
	if(StringToSigned( str, &ival) != 0)
	{
		*value = (ival == 0) ? FALSE : TRUE;
		return TRUE;
	}
	if(StringEqual( str, "true", INT_MAX) != 0)
	{
		*value = TRUE;
		return TRUE;
	}
	else if(StringEqual(str, "false", INT_MAX) != 0)
	{
		*value = FALSE;
		return TRUE;
	}
	return FALSE;
}

int StringToFloat(const char* str, float* value)
{
	if(sscanf(str, "%f", value ) == 1)
	{
		return TRUE;
	}
	return FALSE;
}

int StringToDouble(const char* str, double* value)
{
	if(sscanf( str, "%lf", value ) == 1 )
	{
		return TRUE;
	}
	return FALSE;
}

void XmlStringPairSet(XML_STRING_PAIR* str, char* start, char* end, int flags)
{
	XmlStringPairReset(str);
	str->start = start;
	str->end = end;
	str->flags = flags;
}

void XmlStringPairReset(XML_STRING_PAIR* str)
{
	if((str->flags & XML_STRING_NEEDS_DELETE) != 0)
	{
		MEM_FREE_FUNC(str->start);
	}
	(void)memset(str, 0, sizeof(*str));
}

static void XmlStringPairCollapseWhiteSpace(XML_STRING_PAIR* str)
{
	// Adjusting _start would cause undefined behavior on delete[]
	ASSERT((str->flags & XML_STRING_NEEDS_DELETE ) == 0);
	// Trim leading space.
	str->start = (char*)SkipWhiteSpace(str->start);

	if(str->start != NULL && *str->start != 0)
	{
		char* p = str->start;	// the read pointer
		char* q = str->start;	// the write pointer

		while(*p)
		{
			if(IsWhiteSpace(*p) != 0)
			{
				p = (char*)SkipWhiteSpace(p);
				if(*p == 0)
				{
					break;    // don't write to q; this trims the trailing space.
				}
				*q = ' ';
				++q;
			}
			*q = *p;
			++q;
			++p;
		}
		*q = 0;
	}
}

const char* XmlStringPairGetString(XML_STRING_PAIR* str)
{
	if((str->flags & XML_STRING_NEEDS_FLUSH) != 0)
	{
		*str->end = 0;
		str->flags ^= XML_STRING_NEEDS_FLUSH;

		if(str->flags != 0)
		{
			char* p = str->start;	// the read pointer
			char* q = str->start;	// the write pointer

			while(p < str->end)
			{
				if(((str->flags & XML_STRING_NEEDS_NEW_LINE_NORMALIZATION) != 0) && *p == CR)
				{
					// CR-LF pair becomes LF
					// CR alone becomes LF
					// LF-CR becomes LF
					if(*(p+1) == LF)
					{
						p += 2;
					}
					else
					{
						++p;
					}
					*q++ = LF;
				}
				else if(((str->flags & XML_STRING_NEEDS_NEW_LINE_NORMALIZATION) != 0) && *p == LF)
				{
					if(*(p+1) == CR)
					{
						p += 2;
					}
					else
					{
						++p;
					}
					*q++ = LF;
				}
				else if((str->flags & XML_STRING_NEEDS_ENTITY_PROCESSING) && *p == '&')
				{
					// Entities handled by tinyXML2:
					// - special entities in the entity table [in/out]
					// - numeric character reference [in]
					//   &#20013; or &#x4e2d;

					if(*(p+1) == '#')
					{
#define BUFFER_LENGTH 10
						char buf[BUFFER_LENGTH] = {0};
						int len = 0;
						p = (char*)(GetCharacterRef( p, buf, &len ) );
						ASSERT(0 <= len && len <= BUFFER_LENGTH);
						ASSERT(q + len <= p);
						memcpy(q, buf, len);
						q += len;
#undef BUFFER_LENGTH
					}
					else
					{
						int i=0;
						for(; i<NUM_ENTITIES; ++i )
						{
							const ENTITY *entity = &g_entities[i];
							if(strncmp(p + 1, entity->pattern, entity->length) == 0
									&& *(p + entity->length + 1) == ';')
							{
								// Found an entity - convert.
								*q = entity->value;
								++q;
								p += entity->length + 2;
								break;
							}
						}
						if(i == NUM_ENTITIES)
						{
							// fixme: treat as error?
							++p;
							++q;
						}
					}
				}
				else
				{
					*q = *p;
					++p;
					++q;
				}
			}
			*q = 0;
		}
		// The loop below has plenty going on, and this
		// is a less useful mode. Break it out.
		if((str->flags & XML_STRING_COLLAPSE_WHITE_SPACE) != 0)
		{
			XmlStringPairCollapseWhiteSpace(str);
		}
		str->flags = (str->flags & XML_STRING_NEEDS_DELETE);
	}
	return str->start;
}

void XmlStringPairSetString(XML_STRING_PAIR* str_pair, const char* str, int flags)
{
	size_t len = strlen(str);
	XmlStringPairReset(str_pair);
	str_pair->start = (char*)MEM_ALLOC_FUNC(len+1);
	(void)memcpy(str_pair->start, str, len+1 );
	str_pair->end = str_pair->start + len;
	str_pair->flags = flags | XML_STRING_NEEDS_DELETE;
}

static int XmlStringPairEmpty(XML_STRING_PAIR* str)
{
	return str->start == str->end;
}

static void XmlStringPairSetInternedString(XML_STRING_PAIR* str_pair, const char* str)
{
	XmlStringPairReset(str_pair);
	str_pair->start = (char*)(str);
}


char* XmlStringPairParseText(XML_STRING_PAIR* str, char* p, const char* end_tag, int str_flags)
{
	char *start;
	char end_char;
	size_t length;

	ASSERT(end_tag != NULL && *end_tag != 0);

	start = p;
	end_char = *end_tag;
	length = strlen(end_tag);

	// Inner loop of text parsing.
	while(*p != '\0')
	{
		if(*p == end_char && strncmp(p, end_tag, length ) == 0)
		{
			XmlStringPairSet(str, start, p, str_flags);
			return p + length;
		}
		++p;
	}
	return 0;
}


char* XmlStringPairParseName(XML_STRING_PAIR* str, char* p)
{
	const char *start = p;

	if(p == NULL || (*p) == 0)
	{
		return 0;
	}

	while(*p && (p == start ? IsNameStartChar(*p) : IsNameChar( *p ) ))
	{
		++p;
	}

	if(p > start)
	{
		XmlStringPairSet(str, (char*)start, p, 0);
		return p;
	}
	return 0;
}

void InitializeXmlVisitor(XML_VISITOR* visitor)
{
	visitor->visit_enter_document = (int (*)(XML_VISITOR*, const XML_DOCUMENT*))DummyFuncReturnTrue;
	visitor->visit_exit_document = (int (*)(XML_VISITOR*, const XML_DOCUMENT*))DummyFuncReturnTrue;
	visitor->visit_enter_element = (int (*)(XML_VISITOR*, const XML_ELEMENT*, const XML_ATTRIBUTE*))DummyFuncReturnTrue2;
	visitor->visit_exit_element = (int (*)(XML_VISITOR*, const XML_ELEMENT*))DummyFuncReturnTrue;
	visitor->visit_declaration = (int (*)(XML_VISITOR*, const XML_DECLARATION*))DummyFuncReturnTrue;
	visitor->visit_text = (int (*)(XML_VISITOR*, const XML_TEXT*))DummyFuncReturnTrue;
	visitor->visit_comment = (int (*)(XML_VISITOR*, const XML_COMMENT*))DummyFuncReturnTrue;
	visitor->visit_unknown = (int (*)(XML_VISITOR*, const XML_UNKNOWN*))DummyFuncReturnTrue;
}

void InitializeXmlNode(XML_NODE* node, XML_DOCUMENT* document)
{
	(void)memset(node, 0, sizeof(*node));
	node->to_element = (XML_ELEMENT* (*)(XML_NODE*))DummyFuncReturnNull;
	node->to_text = (XML_TEXT* (*)(XML_NODE*))DummyFuncReturnNull;
	node->to_comment = (XML_COMMENT* (*)(XML_NODE*))DummyFuncReturnNull;
	node->to_document = (XML_DOCUMENT* (*)(XML_NODE*))DummyFuncReturnNull;
	node->to_unknown = (XML_UNKNOWN* (*)(XML_NODE*))DummyFuncReturnNull;
	node->parse_deep = (char* (*)(XML_NODE*, char*, XML_STRING_PAIR*))XmlNodeParseDeep;
	node->delete_func = (void (*)(XML_NODE*))ReleaseXmlNode;
	node->document = document;
}

static void XmlNodeDeleteNode(XML_NODE* this_node, XML_NODE* node)
{
	XML_MEMORY_POOL *pool;

	if(node == 0)
	{
		return;
	}
	pool = node->memory_pool;
	node->delete_func(node);
	XmlMemoryPoolFree(pool, node);
}

static void XmlNodeUnlink(XML_NODE* node, XML_NODE* child)
{
	if(child == node->first_child)
	{
		node->first_child = node->first_child->next;
	}
	if(child == node->last_child)
	{
		node->last_child = node->last_child->prev;
	}

	if(child->prev != NULL) 
	{
		child->prev->next = child->next;
	}
	if(child->next != NULL)
	{
		child->next->prev = child->prev;
	}
	child->parent = NULL;
}

static void XmlNodeDeleteChildren(XML_NODE* this_node)
{
	while(this_node->first_child != NULL)
	{
		XML_NODE *node = this_node->first_child;
		XmlNodeUnlink(this_node, node);

		XmlNodeDeleteNode(this_node, node);
	}
	this_node->first_child = this_node->last_child = NULL;
}

void ReleaseXmlNode(XML_NODE* node)
{
	XmlNodeDeleteChildren(node);
	if(node->parent != NULL)
	{
		XmlNodeUnlink(node, node->parent);
	}
}

static void XmlNodeDeleteChild(XML_NODE* this_node, XML_NODE* node)
{
	ASSERT(node->parent == this_node);
	XmlNodeDeleteNode(this_node, node);
}

const char* XmlNodeGetValue(XML_NODE* node)
{
	return XmlStringPairGetString(&node->value);
}

void XmlNodeSetValue(XML_NODE* node, const char* str, int static_mem)
{
	if(static_mem != 0)
	{
		XmlStringPairSetInternedString(&node->value, str);
	}
	else
	{
		XmlStringPairSetString(&node->value, str, 0);
	}
}

XML_NODE* XmlNodeInsertEndChild(XML_NODE* node, XML_NODE* add)
{
	if(add->document != node->document)
	{
		return 0;
	}

	if(add->parent != NULL)
	{
		XmlNodeUnlink(add->parent, add);
	}
	else
	{
		XmlMemoryPoolSetTracked(add->memory_pool);
	}

	if(node->last_child != NULL)
	{
		ASSERT(node->first_child != NULL);
		ASSERT(node->last_child == NULL);
		node->last_child->next = add;
		add->prev = node->last_child;
		node->last_child = add;

		add->next = NULL;
	}
	else
	{
		ASSERT(node->first_child == NULL);
		node->first_child = node->last_child = add;

		add->prev = NULL;
		add->next = NULL;
	}
	add->parent = node;
	return add;
}

XML_NODE* XmlNodeInsertFirstChild(XML_NODE* node, XML_NODE* add)
{
	if(add->document != node->document)
	{
		return 0;
	}

	if(add->parent != NULL)
	{
		XmlNodeUnlink(add->parent, add);
	}
	else
	{
		XmlMemoryPoolSetTracked(add->memory_pool);
	}

	if(node->first_child != NULL)
	{
		ASSERT(node->last_child != NULL);
		ASSERT(node->first_child->prev == NULL);

		node->first_child->prev = add;
		add->next = node->first_child;
		node->first_child = add;

		add->prev = NULL;
	}
	else
	{
		ASSERT(node->last_child == NULL);
		node->first_child = node->last_child = add;

		add->prev = NULL;
		add->next = NULL;
	}
	add->parent = node;
	return add;
}

XML_NODE* XmlNodeInsertAfterChild(XML_NODE* node, XML_NODE* after, XML_NODE* add)
{
	if(add->document != node->document)
	{
		return 0;
	}

	ASSERT(after->parent == node);

	if(after->parent != node)
	{
		return 0;
	}

	if(after->next == NULL)
	{
		// The last node or the only node.
		return XmlNodeInsertEndChild(node, add);
	}
	if(add->parent != NULL)
	{
		XmlNodeUnlink(add->parent, add);
	}
	else
	{
		XmlMemoryPoolSetTracked(add->memory_pool);
	}

	add->prev = after;
	add->next = after->next;
	after->next->prev = add;
	after->next = add;
	add->parent = node;
	return add;
}

XML_ELEMENT* XmlNodeFirstChildElement(XML_NODE* this_node, const char* value)
{
	XML_NODE *node;
	for(node=this_node->first_child; node != NULL; node=node->next)
	{
		XML_ELEMENT *element = node->to_element(node);
		if(element != NULL)
		{
			if(value == NULL || StringEqual(XmlElementGetName(element), value, INT_MAX) != FALSE)
			{
				return element;
			}
		}
	}
	return NULL;
}

const XML_ELEMENT* XmlNodeLastChildElement(XML_NODE* this_node, const char* value)
{
	XML_NODE *node;
	for(node=this_node->last_child; node != NULL; node = node->prev)
	{
		XML_ELEMENT *element = node->to_element(node);
		if(element != NULL)
		{
			if(value == NULL || StringEqual(XmlElementGetName(element), value, INT_MAX) != FALSE)
			{
				return element;
			}
		}
	}
	return 0;
}

XML_ELEMENT* XmlNodeNextSiblingElement(XML_NODE* this_node, const char* value)
{
	XML_NODE *node;
	for(node=this_node->next; node != NULL; node = node->next)
	{
		XML_ELEMENT *element = node->to_element(node);
		if(element != NULL
				&& (value == NULL || StringEqual(value, XmlNodeGetValue(node), INT_MAX) != FALSE))
		{
			return element;
		}
	}
	return 0;
}

XML_ELEMENT* XmlNodePreviousSiblingElement(XML_NODE* this_node, const char* value)
{
	XML_NODE *node;
	for(node=this_node->prev; node != NULL; node = node->prev)
	{
		XML_ELEMENT *element = node->to_element(node);
		if(element != NULL
				&& (value == NULL || StringEqual(value, XmlNodeGetValue(node), INT_MAX) != FALSE))
		{
			return element;
		}
	}
	return 0;
}

char* XmlNodeParseDeep(XML_NODE* this_node, char* p, XML_STRING_PAIR* parent_end)
{
	while(p != NULL && *p != '\0')
	{
		XML_NODE *node = NULL;
		XML_STRING_PAIR end_tag;
		XML_ELEMENT *element;

		p = XmlDocumentIdentify(this_node->document, p, &node);
		if(p == NULL || node == NULL)
		{
			break;
		}

		p = node->parse_deep(node, p, &end_tag);
		if(p == NULL)
		{
			XmlNodeDeleteNode(this_node, node);
			node = NULL;
			if(this_node->document->error_id != XML_NO_ERROR)
			{
				XmlDocumentSetError(this_node->document, XML_ERROR_PARSING, 0, 0);
			}
			break;
		}

		element = node->to_element(node);
		// We read the end tag. Return it to the parent.
		if(element != NULL && element->closing_type == XML_ELEMENT_CLOSING_TYPE_CLOSING)
		{
			if(parent_end != NULL)
			{
				*parent_end = element->node.value;
			}
			XmlMemoryPoolSetTracked(node->memory_pool);
			XmlNodeDeleteNode(this_node, node);
			return p;
		}

		// Handle an end tag returned to this level.
		// And handle a bunch of annoying errors.
		if(element != NULL)
		{
			int mismatch = FALSE;
			if(XmlStringPairEmpty(&end_tag) != FALSE && element->closing_type == XML_ELEMENT_CLOSING_TYPE_OPEN)
			{
				mismatch = TRUE;
			}
			else if(XmlStringPairEmpty(&end_tag) == FALSE && element->closing_type != XML_ELEMENT_CLOSING_TYPE_OPEN)
			{
				mismatch = TRUE;
			}
			else if(XmlStringPairEmpty(&end_tag) == FALSE)
			{
				if(StringEqual(XmlStringPairGetString(&end_tag), XmlNodeGetValue(node), INT_MAX) == FALSE)
				{
					mismatch = TRUE;
				}
			}
			if(mismatch != FALSE)
			{
				XmlDocumentSetError(this_node->document, XML_ERROR_MISMATCHED_ELEMENT, XmlNodeGetValue(node), 0);
				p = 0;
			}
		}
		if(p == NULL)
		{
			XmlNodeDeleteNode(this_node, node);
			node = NULL;
		}
		if(node != NULL)
		{
			XmlNodeInsertEndChild(this_node, node);
		}
	}
	return 0;
}

void InitializeXmlMemoryPool(XML_MEMORY_POOL* pool)
{
	(void)memset(pool, 0, sizeof(*pool));
	InitializeDynPointerArray(&pool->block_pointers, 10);
}

void ReleaseXmlMemoryPool(XML_MEMORY_POOL* pool)
{
	int i;

	for(i=0; i<pool->block_pointers.size; i++)
	{
		MEM_FREE_FUNC(pool->block_pointers.memory[i]);
	}

	ReleaseDynPointerArray(&pool->block_pointers, NULL);
}

void* XmlMemoryPoolAllocation(XML_MEMORY_POOL* pool)
{
	void *ret;

	if(pool->root == NULL)
	{
		XML_MEMORY_POOL_BLOCK *block =
			(XML_MEMORY_POOL_BLOCK*)MEM_ALLOC_FUNC(sizeof(*block));
		int i;

		(void)memset(block, 0, sizeof(*block));
		DynPointerArrayPush(&pool->block_pointers, block);

		for(i=0; i<XML_MEMORY_POOL_BLOCK_SIZE-1; i++)
		{
			block->chunk[i].next = &block->chunk[i+1];
		}
		block->chunk[XML_MEMORY_POOL_BLOCK_SIZE-1].next = NULL;
		pool->root = block->chunk;
	}

	ret = (void*)pool->root->mem;
	pool->root = pool->root->next;

	pool->current_allocations++;
	if(pool->current_allocations > pool->max_allocations)
	{
		pool->max_allocations = pool->current_allocations;
	}
	pool->num_allocations++;
	pool->num_untracked++;

	return ret;
}

void XmlMemoryPoolFree(XML_MEMORY_POOL* pool, void* mem)
{
	XML_MEMORY_POOL_CHUNK *chunk;
	if(mem == NULL)
	{
		return;
	}

	pool->current_allocations--;
	chunk = (XML_MEMORY_POOL_CHUNK*)((unsigned char*)mem
		- offsetof(XML_MEMORY_POOL_CHUNK, mem));
	chunk->next = pool->root;
	pool->root = chunk;
}

void XmlMemoryPoolSetTracked(XML_MEMORY_POOL* pool)
{
	pool->num_untracked--;
}

void InitializeXmlDocument(XML_DOCUMENT* document)
{
	(void)memset(document, 0, sizeof(*document));
	InitializeXmlNode(&document->node, document);
	document->node.to_document = (XML_DOCUMENT* (*)(XML_NODE*))ReturnThisFunc;
	document->node.accept = (int (*)(XML_NODE*, XML_VISITOR*))XmlDocumentAccept;
	document->node.delete_func = (void (*)(XML_NODE*))ReleaseXmlDocument;
	document->process_entities = TRUE;
	document->white_space = XML_PRESERVE_WHITE_SPACE;
	InitializeXmlMemoryPool(&document->element_pool);
	InitializeXmlMemoryPool(&document->attribute_pool);
	InitializeXmlMemoryPool(&document->text_pool);
	InitializeXmlMemoryPool(&document->comment_pool);
}

void InitializeXmlDocumentDetail(XML_DOCUMENT* document, int process_entities, eXML_WHITE_SPACE white_space)
{
	(void)memset(document, 0, sizeof(*document));
	InitializeXmlNode(&document->node, document);
	document->node.to_document = (XML_DOCUMENT* (*)(XML_NODE*))ReturnThisFunc;
	document->node.accept = (int (*)(XML_NODE*, XML_VISITOR*))XmlDocumentAccept;
	document->node.delete_func = (void (*)(XML_NODE*))ReleaseXmlDocument;
	document->process_entities = process_entities;
	document->white_space = white_space;
	InitializeXmlMemoryPool(&document->element_pool);
	InitializeXmlMemoryPool(&document->attribute_pool);
	InitializeXmlMemoryPool(&document->text_pool);
	InitializeXmlMemoryPool(&document->comment_pool);
	document->error_print_func = (int (*)(const char*, ...))printf;
}

void ReleaseXmlDocument(XML_DOCUMENT* document)
{
	ReleaseXmlNode(&document->node);
	ReleaseXmlMemoryPool(&document->element_pool);
	ReleaseXmlMemoryPool(&document->attribute_pool);
	ReleaseXmlMemoryPool(&document->text_pool);
	ReleaseXmlMemoryPool(&document->comment_pool);
	MEM_FREE_FUNC(document->buffer);
}

static const char* GetXmlErrorName(int index)
{
	// Warning: List must match 'enum XMLError'
	const char *error_names[XML_ERROR_COUNT] =
	{
		"XML_SUCCESS",
		"XML_NO_ATTRIBUTE",
		"XML_WRONG_ATTRIBUTE_TYPE",
		"XML_ERROR_FILE_NOT_FOUND",
		"XML_ERROR_FILE_COULD_NOT_BE_OPENED",
		"XML_ERROR_FILE_READ_ERROR",
		"XML_ERROR_ELEMENT_MISMATCH",
		"XML_ERROR_PARSING_ELEMENT",
		"XML_ERROR_PARSING_ATTRIBUTE",
		"XML_ERROR_IDENTIFYING_TAG",
		"XML_ERROR_PARSING_TEXT",
		"XML_ERROR_PARSING_CDATA",
		"XML_ERROR_PARSING_COMMENT",
		"XML_ERROR_PARSING_DECLARATION",
		"XML_ERROR_PARSING_UNKNOWN",
		"XML_ERROR_EMPTY_DOCUMENT",
		"XML_ERROR_MISMATCHED_ELEMENT",
		"XML_ERROR_PARSING",
		"XML_CAN_NOT_CONVERT_TEXT",
		"XML_NO_TEXT_NODE"
	};

	return error_names[index];
}

eXML_ERROR XmlDocumentLoadFile(XML_DOCUMENT* document, const char* file_name)
{
	FILE *fp;
	size_t data_size;

	XmlDocumentClear(document);
	fp = fopen(file_name, "rb");
	if(fp == NULL)
	{
		XmlDocumentSetError(document, XML_ERROR_FILE_NOT_FOUND, file_name, NULL);
		return document->error_id;
	}

	(void)fseek(fp, 0, SEEK_END);
	data_size = ftell(fp);
	rewind(fp);

	(void)XmlDocumentLoad(document, fp, (size_t (*)(void*, size_t, size_t, void*))fread, data_size);

	(void)fclose( fp );
	return document->error_id;
}

eXML_ERROR XmlDocumentLoad(
	XML_DOCUMENT* document,
	void* src,
	size_t (*read_func)(void*, size_t, size_t, void*),
	size_t data_size
)
{
	const char *p;
	size_t read_size;
	int write_bom;

	XmlDocumentClear(document);
	if(data_size == 0)
	{
		XmlDocumentSetError(document, XML_ERROR_EMPTY_DOCUMENT, NULL, NULL);
		return (eXML_ERROR)document->error_id;
	}

	document->buffer = (char*)MEM_ALLOC_FUNC(data_size+1);
	read_size = read_func(document->buffer, 1, data_size, src);
	document->buffer[data_size] = '\0';

	if(read_size != data_size)
	{
		XmlDocumentSetError(document, XML_ERROR_FILE_READ_ERROR, NULL, NULL);
		return (eXML_ERROR)document->error_id;
	}

	p = document->buffer;
	p = SkipWhiteSpace(p);
	p = ReadBOM(p, &write_bom);
	document->write_bom = (char)write_bom;
	if(p == NULL || *p == '\0')
	{
		XmlDocumentSetError(document, XML_ERROR_EMPTY_DOCUMENT, NULL, NULL);
		return (eXML_ERROR)document->error_id;
	}

	(void)document->node.parse_deep(&document->node, document->buffer + (p - document->buffer), NULL);
	return (eXML_ERROR)document->error_id;
}

eXML_ERROR XmlDocumentParse(XML_DOCUMENT* document, const char* p, long length)
{
	ptrdiff_t delta;
	const char* start = p;
	int write_bom;

	XmlDocumentClear(document);

	if(length == 0 || p == NULL || *p == '\0')
	{
		XmlDocumentSetError(document, XML_ERROR_EMPTY_DOCUMENT, NULL, NULL);
		return (eXML_ERROR)document->error_id;
	}

	if(length == -1)
	{
		length = (long)strlen(p);
	}

	document->buffer = (char*)MEM_ALLOC_FUNC(length+1);
	(void)memcpy(document->buffer, p, length);
	document->buffer[length] = '\0';

	p = (char*)SkipWhiteSpace((char*)p);
	p = (char*)ReadBOM((char*)p, &write_bom);
	document->write_bom = (char)write_bom;
	if(p == NULL || *p == '\0')
	{
		XmlDocumentSetError(document, XML_ERROR_EMPTY_DOCUMENT, NULL, NULL);
		return (eXML_ERROR)document->error_id;
	}

	delta = p - start;	// skip initial whitespace, BOM, etc.
	(void)document->node.parse_deep(&document->node, document->buffer + delta, NULL);
	return (eXML_ERROR)document->error_id;
}


eXML_ERROR XmlDocumentSaveFile(XML_DOCUMENT* document, const char* file_name, int compact)
{
	FILE *fp = fopen(file_name, "w");
	if(fp == NULL)
	{
		XmlDocumentSetError(document, XML_ERROR_FILE_COULD_NOT_BE_OPENED, file_name, NULL);
		return (eXML_ERROR)document->error_id;
	}
	XmlDocumentSave(document, (void*)fp,
		(int (*)(void*, const char*, va_list))vfprintf, compact);
	(void)fclose(fp);
	return (eXML_ERROR)document->error_id;
}


eXML_ERROR XmlDocumentSave(
	XML_DOCUMENT* document,
	void* target,
	int (*vprint_func)(void*, const char*, va_list),
	int compact
)
{
	XML_PRINTER printer;
	InitializeXmlPrinter(&printer, target, vprint_func, compact, 0);
	XmlDocumentPrint(document, &printer);
	ReleaseXmlPrinter(&printer);
	return document->error_id;
}

void XmlDocumentPrint(XML_DOCUMENT* document, XML_PRINTER* streamer)
{
	if(streamer == NULL)
	{
		XML_PRINTER std_streamer;
		InitializeXmlPrinter(&std_streamer, (void*)stdout,
			(int (*)(void*, const char*, ...))sprintf, FALSE, 0);
		(void)XmlDocumentAccept(document, &std_streamer.visitor);
	}
	else
	{
		(void)XmlDocumentAccept(document, &streamer->visitor);
	}
}

static void XmlDocumentSetError(XML_DOCUMENT* document, eXML_ERROR error, const char* str1, const char* str2)
{
	document->error_id = (char)error;
	document->error_string1 = (char*)str1;
	document->error_string2 = (char*)str2;
}

void XmlDocumentPrintError(XML_DOCUMENT* document)
{
#define LENGTH 20
	if(document->error_id != 0)
	{
		char buff1[LENGTH] = {0};
		char buff2[LENGTH] = {0};

		if(document->error_string1 != NULL)
		{
			(void)strcpy(buff1, document->error_string1);
		}
		if(document->error_string2 != NULL)
		{
			(void)strcpy(buff2, document->error_string2);
		}

		document->error_print_func("XMLDocument error id=%d '%s' str1=%s str2=%s\n",
			document->error_id, GetXmlErrorName(document->error_id), buff1, buff2);
	}
#undef LENGTH
}

char* XmlDocumentIdentify(XML_DOCUMENT* document, char* p, XML_NODE** node)
{
#define XML_HEADER "<?"
#define COMMENT_HEADER "<!--"
#define DTD_HEADER "<!"
#define CDATA_HEADER "<![CDATA["
#define ELEMENT_HEADER "<"

#define XML_HEADER_LENGTH 2
#define COMMENT_HEADER_LENGTH 4
#define DTD_HEADER_LENGTH 2
#define CDATA_HEADER_LENGTH 9
#define ELEMENT_HEADER_LENGTH 1

	const char *start = p;

	XML_NODE *return_node = NULL;

	p = (char*)SkipWhiteSpace(p);
	if(p == NULL || *p == '\0')
	{
		return p;
	}

	if(StringEqual(p, XML_HEADER, XML_HEADER_LENGTH) != FALSE)
	{
		return_node = (XML_NODE*)XmlMemoryPoolAllocation(&document->comment_pool);
		InitializeXmlDeclaration((XML_DECLARATION*)return_node, document);

		return_node->memory_pool = &document->comment_pool;
		p += XML_HEADER_LENGTH;
	}
	else if(StringEqual(p, COMMENT_HEADER, COMMENT_HEADER_LENGTH) != FALSE)
	{
		return_node = (XML_NODE*)XmlMemoryPoolAllocation(&document->comment_pool);
		InitializeXmlComment((XML_COMMENT*)return_node, document);

		return_node->memory_pool = &document->comment_pool;
		p += COMMENT_HEADER_LENGTH;
	}
	else if(StringEqual(p, CDATA_HEADER, CDATA_HEADER_LENGTH) != FALSE)
	{
		XML_TEXT *text = (XML_TEXT*)XmlMemoryPoolAllocation(&document->text_pool);
		InitializeXmlText(text, document);

		return_node = (XML_NODE*)text;
		return_node->memory_pool = &document->text_pool;
		p += CDATA_HEADER_LENGTH;
		text->is_c_data = TRUE;
	}
	else if(StringEqual(p, DTD_HEADER, DTD_HEADER_LENGTH) != FALSE)
	{
		return_node = (XML_NODE*)XmlMemoryPoolAllocation(&document->comment_pool);
		InitializeXmlUnknown((XML_UNKNOWN*)return_node, document);

		return_node->memory_pool = &document->comment_pool;
		p += DTD_HEADER_LENGTH;
	}
	else if(StringEqual(p, ELEMENT_HEADER, ELEMENT_HEADER_LENGTH) != FALSE)
	{
		return_node = (XML_NODE*)XmlMemoryPoolAllocation(&document->element_pool);
		InitializeXmlElement((XML_ELEMENT*)return_node, document);

		return_node->memory_pool = &document->element_pool;
		p += ELEMENT_HEADER_LENGTH;
	}
	else
	{
		return_node = (XML_NODE*)XmlMemoryPoolAllocation(&document->text_pool);
		InitializeXmlText(return_node, document);
		return_node->memory_pool = &document->text_pool;
		p = (char*)start;	// Back it up, all the text counts.
	}

	*node = return_node;
	return p;
}

int XmlDocumentAccept(XML_DOCUMENT* document, XML_VISITOR* visitor)
{
	if(visitor->visit_enter_document(visitor, document) != FALSE)
	{
		XML_NODE *node;
		for (node=document->node.first_child; node != NULL; node=node->next)
		{
			if(node->accept(node, visitor) == FALSE)
			{
				break;
			}
		}
	}
	return visitor->visit_exit_document(visitor, document);
}

void XmlDocumentClear(XML_DOCUMENT* document)
{
	XmlNodeDeleteChildren(&document->node);

	document->error_id = XML_NO_ERROR;
	document->error_string1 = NULL;
	document->error_string2 = NULL;

	MEM_FREE_FUNC(document->buffer);
	document->buffer = NULL;
}

XML_ELEMENT* XmlDocumentNewElement(XML_DOCUMENT* document, const char* name)
{
	XML_ELEMENT *element = (XML_ELEMENT*)XmlMemoryPoolAllocation(&document->element_pool);
	InitializeXmlElement(element, document);
	element->node.memory_pool = &document->element_pool;
	XmlElementSetName(element, name, FALSE);

	return element;
}

XML_COMMENT* XmlDocumentNewComment(XML_DOCUMENT* document, const char* str)
{
	XML_COMMENT *comment = (XML_COMMENT*)XmlMemoryPoolAllocation(&document->comment_pool);
	InitializeXmlComment(comment, document);
	comment->node.memory_pool = &document->comment_pool;
	XmlNodeSetValue(&comment->node, str, FALSE);
	return comment;
}

XML_TEXT* XmlDocumentNewText(XML_DOCUMENT* document, const char* str)
{
	XML_TEXT *text = (XML_TEXT*)XmlMemoryPoolAllocation(&document->text_pool);
	InitializeXmlText(text, document);
	text->node.memory_pool = &document->text_pool;
	XmlNodeSetValue(&text->node, str, FALSE);
	return text;
}

XML_DECLARATION* XmlDocumentNewDeclaration(XML_DOCUMENT* document, const char* str)
{
	XML_DECLARATION *declaration = (XML_DECLARATION*)XmlMemoryPoolAllocation(&document->comment_pool);
	InitializeXmlDeclaration(declaration, document);
	declaration->node.memory_pool = &document->comment_pool;
	XmlNodeSetValue(&declaration->node, str != NULL ? str : "xml version=\"1.0\" encoding=\"UTF-8\"", FALSE);
	return declaration;
}

XML_UNKNOWN* XmlDocumentNewUnknown(XML_DOCUMENT* document, const char* str)
{
	XML_UNKNOWN *unknown = (XML_UNKNOWN*)XmlMemoryPoolAllocation(&document->comment_pool);
	InitializeXmlUnknown(unknown, document);
	unknown->node.memory_pool = &document->comment_pool;
	XmlNodeSetValue(&unknown->node, str, FALSE);
	return unknown;
}

static void XmlDocumentDeleteNode(XML_NODE* node)
{
	XmlNodeDeleteChild(node->parent, node);
}

// --------- XMLText ---------- //

void InitializeXmlText(XML_TEXT* text, XML_DOCUMENT* document)
{
	(void)memset(text, 0, sizeof(*text));
	InitializeXmlNode(&text->node, document);
	text->node.to_text = (XML_TEXT* (*)(XML_NODE*))ReturnThisFunc;
	text->node.shallow_clone = (XML_NODE* (*)(XML_NODE*, XML_DOCUMENT*))XmlTextShallowClone;
	text->node.shallow_equal = (int (*)(XML_NODE*, const XML_NODE*))XmlTextShallowEqual;
	text->node.accept = (int (*)(XML_NODE*, XML_VISITOR*))XmlTextAccept;
	text->node.parse_deep = (char* (*)(struct _XML_NODE*, char*, XML_STRING_PAIR*))XmlTextParseDeep;
}

char* XmlTextParseDeep(XML_TEXT* text, char* p, XML_STRING_PAIR* pair)
{
	const char* start = p;
	if(text->is_c_data != FALSE)
	{
		p = XmlStringPairParseText(&text->node.value, p, "]]>", XML_STRING_NEEDS_NEW_LINE_NORMALIZATION);
		if(p == NULL)
		{
			XmlDocumentSetError(text->node.document, XML_ERROR_PARSING_CDATA, start, 0);
		}
		return p;
	}
	else
	{
		int flags = (text->node.document->process_entities != FALSE) ? XML_STRING_TEXT_ELEMENT : XML_STRING_TEXT_ELEMENT_LEAVE_ENTITIES;

		if(text->node.document->white_space == XML_COLLAPSE_WHITE_SPACE)
		{
			flags |= XML_STRING_COLLAPSE_WHITE_SPACE;
		}

		p = XmlStringPairParseText(&text->node.value, p, "<", flags);
		if(p == NULL)
		{
			XmlDocumentSetError(text->node.document, XML_ERROR_PARSING_TEXT, start, 0);
		}
		if(p != NULL && *p != '\0')
		{
			return p-1;
		}
	}
	return 0;
}

XML_NODE* XmlTextShallowClone(XML_TEXT* this_text, XML_DOCUMENT* document)
{
	XML_TEXT *text;

	if(document == NULL)
	{
		document = this_text->node.document;
	}
	text = XmlDocumentNewText(document, XmlNodeGetValue(&this_text->node));
	text->is_c_data = this_text->is_c_data;
	return (XML_NODE*)text;
}

int XmlTextShallowEqual(XML_TEXT* this_text, const XML_NODE* compare)
{
	XML_TEXT *text = compare->to_text((XML_NODE*)compare);
	return (text != NULL && StringEqual(XmlNodeGetValue(&text->node), XmlNodeGetValue(&this_text->node), INT_MAX) != FALSE);
}

int XmlTextAccept(XML_TEXT* text, XML_VISITOR* visitor)
{
	return visitor->visit_text(visitor, text);
}

// --------- XMLComment ---------- //

void InitializeXmlComment(XML_COMMENT* comment, XML_DOCUMENT* document)
{
	InitializeXmlNode(&comment->node, document);
	comment->node.to_comment = (XML_COMMENT* (*)(XML_NODE*))ReturnThisFunc;
	comment->node.shallow_clone = (XML_NODE* (*)(XML_NODE*, XML_DOCUMENT*))XmlCommentShallowClone;
	comment->node.shallow_equal = (int (*)(XML_NODE*, const XML_NODE*))XmlCommentShallowEqual;
	comment->node.accept = (int (*)(XML_NODE*, XML_VISITOR*))XmlCommentAccept;
	comment->node.parse_deep = (char* (*)(struct _XML_NODE*, char*, XML_STRING_PAIR*))XmlCommentParseDeep;
}

char* XmlCommentParseDeep(XML_COMMENT* comment, char* p, XML_STRING_PAIR* str)
{
	// Comment parses as text.
	const char* start = p;
	p = XmlStringPairParseText(&comment->node.value, p, "-->", XML_STRING_COMMENT);
	if(p == NULL)
	{
		XmlDocumentSetError(comment->node.document, XML_ERROR_PARSING_COMMENT, start, NULL);
	}
	return p;
}

XML_NODE* XmlCommentShallowClone(XML_COMMENT* this_comment, XML_DOCUMENT* document)
{
	XML_COMMENT *comment;
	if(document == NULL)
	{
		document = this_comment->node.document;
	}
	comment = XmlDocumentNewComment(document, XmlNodeGetValue(&this_comment->node));
	return (XML_NODE*)comment;
}

int XmlCommentShallowEqual(XML_COMMENT* this_comment, const XML_NODE* compare)
{
	XML_COMMENT *comment = compare->to_comment((XML_NODE*)compare);
	return (comment != NULL && StringEqual(XmlNodeGetValue(&comment->node), XmlNodeGetValue(&this_comment->node), INT_MAX) != FALSE);
}

int XmlCommentAccept(XML_COMMENT* comment, XML_VISITOR* visitor)
{
	return visitor->visit_comment(visitor, comment);
}


// --------- XMLDeclaration ---------- //

void InitializeXmlDeclaration(XML_DECLARATION* declaration, XML_DOCUMENT* document)
{
	InitializeXmlNode(&declaration->node, document);
	declaration->node.to_declaration = (XML_DECLARATION* (*)(XML_NODE*))ReturnThisFunc;
	declaration->node.parse_deep = (char* (*)(XML_NODE*, char*, XML_STRING_PAIR*))XmlDeclarationParseDeep;
	declaration->node.shallow_clone = (XML_NODE* (*)(XML_NODE*, XML_DOCUMENT*))XmlDeclarationShallowClone;
	declaration->node.shallow_equal = (int (*)(XML_NODE*, const XML_NODE*))XmlDeclarationShallowEqual;
	declaration->node.accept = (int (*)(XML_NODE*, XML_VISITOR*))XmlDeclarationAccept;
}

char* XmlDeclarationParseDeep(XML_DECLARATION* declaration, char* p, XML_STRING_PAIR* str)
{
    // Declaration parses as text.
    const char* start = p;
	p = XmlStringPairParseText(&declaration->node.value, p, "?>", XML_STRING_NEEDS_NEW_LINE_NORMALIZATION);
	if(p == NULL)
	{
		XmlDocumentSetError(declaration->node.document, XML_ERROR_PARSING_DECLARATION, start, NULL);
	}
	return p;
}

XML_NODE* XmlDeclarationShallowClone(XML_DECLARATION* declaration, XML_DOCUMENT* document)
{
	XML_DECLARATION *dec;
	if(document == NULL)
	{
		document = declaration->node.document;
	}
	dec = XmlDocumentNewDeclaration(document, XmlNodeGetValue(&declaration->node));
	return (XML_NODE*)dec;
}

int XmlDeclarationShallowEqual(XML_DECLARATION* this_declaration, const XML_NODE* compare)
{
	XML_DECLARATION* declaration = compare->to_declaration((XML_NODE*)compare);
	return (declaration != NULL && StringEqual(XmlNodeGetValue(&declaration->node),
		XmlNodeGetValue(&this_declaration->node), INT_MAX) != FALSE);
}

int XmlDeclarationAccept(XML_DECLARATION* declaration, XML_VISITOR* visitor)
{
	return visitor->visit_declaration(visitor, declaration);
}

// --------- XMLUnknown ---------- //

void InitializeXmlUnknown(XML_UNKNOWN* unknown, XML_DOCUMENT* document)
{
	InitializeXmlNode(&unknown->node, document);
	unknown->node.to_unknown = (XML_UNKNOWN* (*)(XML_NODE*))ReturnThisFunc;
	unknown->node.parse_deep = (char* (*)(XML_NODE*, char*, XML_STRING_PAIR*))XmlUnknownParseDeep;
	unknown->node.shallow_clone = (XML_NODE* (*)(XML_NODE*, XML_DOCUMENT*))XmlUnknownShallowClone;
	unknown->node.shallow_equal = (int (*)(XML_NODE*, const XML_NODE*))XmlUnknownShallowEqual;
	unknown->node.accept = (int (*)(XML_NODE*, XML_VISITOR*))XmlUnknownAccept;
}

char* XmlUnknownParseDeep(XML_UNKNOWN* unknown, char* p, XML_STRING_PAIR* pair)
{
	// Unknown parses as text.
	const char* start = p;

	p = XmlStringPairParseText(&unknown->node.value, p, ">", XML_STRING_NEEDS_NEW_LINE_NORMALIZATION);
	if(p == NULL)
	{
		XmlDocumentSetError(unknown->node.document, XML_ERROR_PARSING_UNKNOWN, start, NULL);
	}
	return p;
}

XML_NODE* XmlUnknownShallowClone(XML_UNKNOWN* unknown, XML_DOCUMENT* document)
{
	XML_UNKNOWN *text;
	if(document == NULL)
	{
		document = unknown->node.document;
	}
	text = XmlDocumentNewUnknown(document, XmlNodeGetValue(&unknown->node));
	return (XML_NODE*)text;
}

int XmlUnknownShallowEqual(XML_UNKNOWN* this_unknown, const XML_NODE* compare)
{
	XML_UNKNOWN* unknown = compare->to_unknown((XML_NODE*)compare);
	return (unknown != NULL && StringEqual(XmlNodeGetValue(&unknown->node),
		XmlNodeGetValue(&this_unknown->node), INT_MAX) != FALSE);
}

int XmlUnknownAccept(XML_UNKNOWN* unknown, XML_VISITOR* visitor)
{
	return visitor->visit_unknown(visitor, unknown);
}

// --------- XMLAttribute ---------- //
void InitializeXmlAttribute(XML_ATTRIBUTE* attribute)
{
	(void)memset(attribute, 0, sizeof(*attribute));
	attribute->delete_func = (void (*)(XML_ATTRIBUTE*))DummyFuncNoReturn;
}

const char* XmlAttributeGetName(XML_ATTRIBUTE* attribute) 
{
	return XmlStringPairGetString(&attribute->name);
}

const char* XmlAttributeGetValue(XML_ATTRIBUTE* attribute) 
{
	return XmlStringPairGetString(&attribute->value);
}

char* XmlAttributeParseDeep(XML_ATTRIBUTE* attribute, char* p, int process_entities)
{
	char end_tag[2] = {0};

	// Parse using the name rules: bug fix, was using ParseText before
	p = XmlStringPairParseName(&attribute->name, p);
	if(p == NULL || *p == '\0')
	{
		return NULL;
	}

	// Skip white space before =
	p = (char*)SkipWhiteSpace(p);
	if(p == NULL || *p != '=')
	{
		return NULL;
	}

	++p;	// move up to opening quote
	p = (char*)SkipWhiteSpace(p);
	if (*p != '\"' && *p != '\'')
	{
		return NULL;
	}

	end_tag[0] = *p;
	++p;	// move past opening quote

	p = XmlStringPairParseText(&attribute->value, p, end_tag,
		process_entities != FALSE ? XML_STRING_VALUE : XML_STRING_VALUE_LEAVE_ENTITIES);
	return p;
}

void XmlAttributeSetName(XML_ATTRIBUTE* attribute, const char* n)
{
	XmlStringPairSetString(&attribute->name, n, 0);
}

eXML_ERROR XmolAttributeQuerySignedValue(XML_ATTRIBUTE* attribute, int* value)
{
	if(StringToSigned(XmlAttributeGetValue(attribute), value) != FALSE)
	{
		return XML_NO_ERROR;
	}
	return XML_WRONG_ATTRIBUTE_TYPE;
}

eXML_ERROR XmlAttributeQueryUnsignedValue(XML_ATTRIBUTE* attribute, unsigned int* value)
{
	if (StringToUnsigned(XmlAttributeGetValue(attribute), value) != FALSE)
	{
		return XML_NO_ERROR;
	}
	return XML_WRONG_ATTRIBUTE_TYPE;
}

eXML_ERROR XmlAttributeQueryBooleanValue(XML_ATTRIBUTE* attribute, int* value)
{
	if(StringToBool(XmlAttributeGetValue(attribute), value) != FALSE)
	{
		return XML_NO_ERROR;
	}
	return XML_WRONG_ATTRIBUTE_TYPE;
}

eXML_ERROR XmlAttributeQueryFloatValue(XML_ATTRIBUTE* attribute, float* value)
{
	if(StringToFloat(XmlAttributeGetValue(attribute), value) != FALSE)
	{
		return XML_NO_ERROR;
	}
	return XML_WRONG_ATTRIBUTE_TYPE;
}

eXML_ERROR XmlAttributeQueryDoubleValue(XML_ATTRIBUTE* attribute, double* value)
{
	if(StringToDouble(XmlAttributeGetValue(attribute), value) != FALSE)
	{
		return XML_NO_ERROR;
	}
	return XML_WRONG_ATTRIBUTE_TYPE;
}

#define BUFFER_SIZE 200

void XmlAttributeSetStringAttribute(XML_ATTRIBUTE* attribute, const char* v)
{
	XmlStringPairSetString(&attribute->value, v, 0);
}

void XmlAttributeSetSignedAttribute(XML_ATTRIBUTE* attribute, int v )
{
	char buff[BUFFER_SIZE];
	SignedToString(v, buff);
	XmlStringPairSetString(&attribute->value, buff, 0);
}

void XmlAttributeSetUnsigndAttribute(XML_ATTRIBUTE* attribute, unsigned int v)
{
	char buff[BUFFER_SIZE];
	UnsignedToString(v, buff);
	XmlStringPairSetString(&attribute->value, buff, 0);
}

void XmlAttributeSetBooleanAttribute(XML_ATTRIBUTE* attribute, int v)
{
	char buff[BUFFER_SIZE];
	BooleanToString(v, buff);
	XmlStringPairSetString(&attribute->value, buff, 0);
}

void XmlAttributeSetDoubleAttribute(XML_ATTRIBUTE* attribute, double v)
{
	char buff[BUFFER_SIZE];
	DoubleToString(v, buff);
	XmlStringPairSetString(&attribute->value, buff, 0);
}

void XmlAttributeSetFloatAttribute(XML_ATTRIBUTE* attribute, float v)
{
	char buff[BUFFER_SIZE];
	FloatToString(v, buff);
	XmlStringPairSetString(&attribute->value, buff, 0);
}

// --------- XMLElement ---------- //
void InitializeXmlElement(XML_ELEMENT* element, XML_DOCUMENT* document)
{
	(void)memset(element, 0, sizeof(*element));
	InitializeXmlNode(&element->node, document);
	element->node.to_element = (XML_ELEMENT* (*)(XML_NODE*))ReturnThisFunc;
	element->node.shallow_clone = (XML_NODE* (*)(XML_NODE*, XML_DOCUMENT*))XmlElementShallowClone;
	element->node.shallow_equal = (int (*)(XML_NODE*, const XML_NODE*))XmlElementShallowEqual;
	element->node.parse_deep = (char* (*)(XML_NODE*, char*, XML_STRING_PAIR*))XmlElementParseDeep;
	element->node.accept = (int (*)(XML_NODE*, XML_VISITOR*))XmlElementAccept;
	element->node.delete_func = (void (*)(XML_NODE*))ReleaseXmlElement;
}

static void DeleteAttribute(XML_ATTRIBUTE* attribute )
{
	XML_MEMORY_POOL *pool;
	if(attribute == NULL)
	{
		return;
	}
	pool = attribute->memory_pool;
	if(attribute->delete_func != NULL)
	{
		attribute->delete_func(attribute);
	}
	XmlMemoryPoolFree(pool, attribute);
}

void XmlElementDeleteAttributeByName(XML_ELEMENT* element, const char* name)
{
	XML_ATTRIBUTE *prev = NULL;
	XML_ATTRIBUTE *attribute;

	for(attribute=element->root_attribute; attribute != NULL; attribute=attribute->next)
	{
		if(StringEqual(name, XmlAttributeGetName(attribute), INT_MAX) != TRUE)
		{
			if(prev != NULL)
			{
				prev->next = attribute->next;
			}
			else
			{
				element->root_attribute = attribute->next;
			}
			DeleteAttribute(attribute);
			break;
		}
		prev = attribute;
	}
}

void ReleaseXmlElement(XML_ELEMENT* element)
{
	while(element->root_attribute != NULL)
	{
		XML_ATTRIBUTE *next = element->root_attribute->next;
		DeleteAttribute(element->root_attribute);
		element->root_attribute = next;
	}
}

static const char* XmlElementGetName(XML_ELEMENT* element)
{
	return XmlNodeGetValue(&element->node);
}

static void XmlElementSetName(XML_ELEMENT* element, const char* str, int static_mem)
{
	XmlNodeSetValue(&element->node, str, static_mem);
}

XML_ATTRIBUTE* XmlElementFindAttribute(XML_ELEMENT* element, const char* name)
{
	XML_ATTRIBUTE *attribute;
	for(attribute = element->root_attribute; attribute != NULL; attribute = attribute->next)
	{
		if(StringEqual(XmlAttributeGetName(attribute), name, INT_MAX) != FALSE)
		{
			return attribute;
		}
	}
	return NULL;
}

const char* XmlElementAttribute(XML_ELEMENT* element, const char* name, const char* value)
{
	XML_ATTRIBUTE *attribute = XmlElementFindAttribute(element, name);
	if(attribute == NULL)
	{
		return NULL;
	}
	if(value == NULL || StringEqual(XmlAttributeGetValue(attribute), value, INT_MAX) != FALSE)
	{
		return XmlAttributeGetValue(attribute);
	}
	return NULL;
}

const char* XmlElementGetText(XML_ELEMENT* element)
{
	if(element->node.first_child != NULL
		&& element->node.first_child->to_text(element->node.first_child) != NULL)
	{
		return XmlNodeGetValue(element->node.first_child);
	}
	return NULL;
}

void XmlElementSetText(XML_ELEMENT* element, const char* in_text)
{
	if(element->node.first_child != NULL
		&& element->node.first_child->to_text(element->node.first_child) != NULL)
	{
		XmlNodeSetValue(element->node.first_child, in_text, FALSE);
	}
	else
	{
		XML_TEXT* text = XmlDocumentNewText(element->node.document, in_text);
		XmlNodeInsertFirstChild(&element->node, &text->node);
	}
}

void XmlElementSetSignedText(XML_ELEMENT* element, int v) 
{
	char buff[BUFFER_SIZE];
	SignedToString(v, buff);
	XmlElementSetText(element, buff);
}

void XmlElementSetUnsignedText(XML_ELEMENT* element, unsigned v) 
{
	char buff[BUFFER_SIZE];
	UnsignedToString(v, buff);
	XmlElementSetText(element, buff);
}

void XmlElementSetBooleanText(XML_ELEMENT* element, int v) 
{
    char buff[BUFFER_SIZE];
	BooleanToString(v, buff);
	XmlElementSetText(element, buff);
}

void XmlElementSetFloatText(XML_ELEMENT* element, float v) 
{
	char buff[BUFFER_SIZE];
	FloatToString(v, buff);
	XmlElementSetText(element, buff);
}


void XmlElementSetDoubleText(XML_ELEMENT* element, double v) 
{
	char buff[BUFFER_SIZE];
	DoubleToString(v, buff);
	XmlElementSetText(element, buff);
}

eXML_ERROR XmlElementQuerySignedText(XML_ELEMENT* element, int* value)
{
	if(element->node.first_child != NULL
		&& element->node.first_child->to_text(element->node.first_child) != NULL)
	{
		const char *t = XmlNodeGetValue(element->node.first_child);
		if(StringToSigned(t, value) != FALSE)
		{
			return XML_SUCCESS;
		}
		return XML_CAN_NOT_CONVERT_TEXT;
	}
	return XML_NO_TEXT_NODE;
}

eXML_ERROR XmlElementQueryUnsignedText(XML_ELEMENT* element, unsigned int* value)
{
	if(element->node.first_child != NULL
		&& element->node.first_child->to_text(element->node.first_child) != NULL)
	{
		const char *t = XmlNodeGetValue(element->node.first_child);
		if(StringToUnsigned(t, value) != FALSE)
		{
			return XML_SUCCESS;
		}
		return XML_CAN_NOT_CONVERT_TEXT;
	}
	return XML_NO_TEXT_NODE;
}

eXML_ERROR XmlElementQueryBooleanText(XML_ELEMENT* element, int* value)
{
	if(element->node.first_child != NULL
		&& element->node.first_child->to_text(element->node.first_child) != NULL)
	{
		const char *t = XmlNodeGetValue(element->node.first_child);
		if(StringToBool(t, value) != FALSE)
		{
			return XML_SUCCESS;
		}
		return XML_CAN_NOT_CONVERT_TEXT;
	}
	return XML_NO_TEXT_NODE;
}

eXML_ERROR XmlElementQueryDoubleText(XML_ELEMENT* element, double* value)
{
	if(element->node.first_child != NULL
		&& element->node.first_child->to_text(element->node.first_child) != NULL)
	{
		const char *t = XmlNodeGetValue(element->node.first_child);
		if(StringToDouble(t, value) != FALSE)
		{
			return XML_SUCCESS;
		}
		return XML_CAN_NOT_CONVERT_TEXT;
	}
	return XML_NO_TEXT_NODE;
}

eXML_ERROR XmlElementQueryFloatText(XML_ELEMENT* element, float* value)
{
	if(element->node.first_child != NULL
		&& element->node.first_child->to_text(element->node.first_child) != NULL)
	{
		const char *t = XmlNodeGetValue(element->node.first_child);
		if(StringToFloat(t, value) != FALSE)
		{
			return XML_SUCCESS;
		}
		return XML_CAN_NOT_CONVERT_TEXT;
	}
	return XML_NO_TEXT_NODE;
}

XML_ATTRIBUTE* XmlElementFindOrCreateAttribute(XML_ELEMENT* element, const char* name)
{
	XML_ATTRIBUTE *last = NULL;
	XML_ATTRIBUTE *attrib = NULL;
	for( attrib = element->root_attribute; attrib != NULL; last = attrib, attrib = attrib->next)
	{
		if(StringEqual(XmlAttributeGetName(attrib), name, INT_MAX) != FALSE)
		{
			break;
		}
	}
	if(attrib == NULL)
	{
		attrib = (XML_ATTRIBUTE*)XmlMemoryPoolAllocation(&element->node.document->attribute_pool);
		InitializeXmlAttribute(attrib);
		attrib->memory_pool = &element->node.document->attribute_pool;

		if(last != NULL)
		{
			last->next = attrib;
		}
		else
		{
			element->root_attribute = attrib;
		}
		XmlAttributeSetName(attrib, name);
		XmlMemoryPoolSetTracked(attrib->memory_pool);
	}
	return attrib;
}

char* XmlElementParseAttributes(XML_ELEMENT* element, char* p)
{
	const char *start = p;
	XML_ATTRIBUTE *prev_attribute = NULL;

	// Read the attributes.
	while(p != NULL)
	{
		p = (char*)SkipWhiteSpace(p);
		if(p == NULL || (*p) == '\0')
		{
			XmlDocumentSetError(element->node.document, XML_ERROR_PARSING_ELEMENT,
				start, XmlElementGetName(element));
			return NULL;
		}

		// attribute.
		if(IsNameStartChar(*p) != FALSE)
		{
			XML_ATTRIBUTE *attrib = XmlMemoryPoolAllocation(&element->node.document->attribute_pool);
			InitializeXmlAttribute(attrib);
			attrib->memory_pool = &element->node.document->attribute_pool;
			XmlMemoryPoolSetTracked(attrib->memory_pool);

			p = XmlAttributeParseDeep(attrib, p, element->node.document->process_entities);
			if(p == NULL || XmlElementAttribute(element, XmlAttributeGetName(attrib), NULL) != NULL)
			{
				DeleteAttribute(attrib);
				XmlDocumentSetError(element->node.document, XML_ERROR_PARSING_ATTRIBUTE, start, p);
				return NULL;
			}
			// There is a minor bug here: if the attribute in the source xml
			// document is duplicated, it will not be detected and the
			// attribute will be doubly added. However, tracking the 'prevAttribute'
			// avoids re-scanning the attribute list. Preferring performance for
			// now, may reconsider in the future.
			if(prev_attribute != NULL)
			{
				prev_attribute->next = attrib;
			}
			else
			{
				element->root_attribute = attrib;
			}
			prev_attribute = attrib;
		}
		// end of the tag
		else if(*p == '/' && *(p+1) == '>')
		{
			element->closing_type = XML_ELEMENT_CLOSING_TYPE_CLOSED;
			return p+2;	// done; sealed element.
		}
		// end of the tag
		else if( *p == '>')
		{
			++p;
			break;
		}
		else
		{
			XmlDocumentSetError(element->node.document, XML_ERROR_PARSING_ELEMENT, start, p);
			return NULL;
		}
	}
	return p;
}

char* XmlElementParseDeep(XML_ELEMENT* element, char* p, XML_STRING_PAIR* str)
{
	// Read the element name.
	p = (char*)SkipWhiteSpace(p);
	if(p == NULL)
	{
		return NULL;
	}

	// The closing element is the </element> form. It is
	// parsed just like a regular element then deleted from
	// the DOM.
	if(*p == '/')
	{
		element->closing_type = XML_ELEMENT_CLOSING_TYPE_CLOSING;
		++p;
	}

	p = XmlStringPairParseName(&element->node.value, p);
	if(XmlStringPairEmpty(&element->node.value) != FALSE)
	{
		return NULL;
	}

	p = XmlElementParseAttributes(element, p);
	if(p == NULL || *p != '\0' || element->closing_type != 0)
	{
		return p;
	}

	p = XmlNodeParseDeep(&element->node, p, str);
	return p;
}

void XmlElementSetStringAttribute(XML_ELEMENT* element, const char* name, const char* value)
{
	XML_ATTRIBUTE* a = XmlElementFindOrCreateAttribute(element, name);
	XmlAttributeSetStringAttribute(a, value);
}

void XmlElementSetSignedAttribute(XML_ELEMENT* element, const char* name, int value)
{
	XML_ATTRIBUTE* a = XmlElementFindOrCreateAttribute(element, name);
	XmlAttributeSetSignedAttribute(a, value);
}

void XmlElementSetUnsignedAttribute(XML_ELEMENT* element, const char* name, unsigned int value)
{
	XML_ATTRIBUTE* a = XmlElementFindOrCreateAttribute(element, name);
	XmlAttributeSetUnsigndAttribute(a, value);
}

void XmlElementSetBooleanAttribute(XML_ELEMENT* element, const char* name, int value)
{
	XML_ATTRIBUTE* a = XmlElementFindOrCreateAttribute(element, name);
	XmlAttributeSetBooleanAttribute(a, value);
}

void XmlElementSetDoubleAttribute(XML_ELEMENT* element, const char* name, double value)
{
	XML_ATTRIBUTE* a = XmlElementFindOrCreateAttribute(element, name);
	XmlAttributeSetDoubleAttribute(a, value);
}

void XmlElementSetFloatAttribute(XML_ELEMENT* element, const char* name, float value)
{
	XML_ATTRIBUTE* a = XmlElementFindOrCreateAttribute(element, name);
	XmlAttributeSetFloatAttribute(a, value);
}

XML_NODE* XmlElementShallowClone(XML_ELEMENT* this_element, XML_DOCUMENT* document)
{
	XML_ELEMENT *element;
	XML_ATTRIBUTE *attribute;

	if(document == NULL)
	{
		document = this_element->node.document;
	}
	element = XmlDocumentNewElement(document, XmlNodeGetValue(&this_element->node));
	for(attribute=element->root_attribute; attribute != NULL; attribute=attribute->next)
	{
		XmlElementSetStringAttribute(element, XmlAttributeGetName(attribute), XmlAttributeGetValue(attribute));
	}
	return &element->node;
}

int XmlElementShallowEqual(XML_ELEMENT* element, const XML_NODE* compare)
{
	XML_ELEMENT* other = compare->to_element((XML_NODE*)compare);
	if(other != NULL && StringEqual(XmlNodeGetValue(&other->node), XmlNodeGetValue(&element->node), INT_MAX) != FALSE)
	{
		XML_ATTRIBUTE* a=element->root_attribute;
		XML_ATTRIBUTE* b=other->root_attribute;

		while(a != NULL && b != NULL)
		{
			if(StringEqual(XmlAttributeGetValue(a), XmlAttributeGetValue(b), INT_MAX) == FALSE)
			{
				return FALSE;
			}
			a = a->next;
			b = b->next;
		}
		if(a != NULL || b != NULL)
		{
			// different count
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

int XmlElementAccept(XML_ELEMENT* element, XML_VISITOR* visitor)
{
	if(visitor->visit_enter_element(visitor, element, element->root_attribute) != FALSE)
	{
		XML_NODE *node;
		for(node=element->node.first_child; node != NULL; node = node->next)
		{
			if(node->accept(node, visitor) == FALSE)
			{
				break;
			}
		}
	}
	return visitor->visit_exit_element(visitor, element);
}

static int XmlPrinterGetCompactMode(XML_PRINTER* printer, XML_ELEMENT* element)
{
	return printer->compact_mode;
}

void InitializeXmlPrinter(
	XML_PRINTER* printer,
	void* target,
	int (*vprint_func)(void*, const char*, va_list),
	int compact,
	int depth
)
{
	int i;

	(void)memset(printer, 0, sizeof(printer));
	InitializeXmlVisitor(&printer->visitor);
	printer->print_space = (void (*)(XML_PRINTER*, int))XmlPrinterPrintSpace;
	printer->get_compact_mode = (int (*)(XML_PRINTER*, XML_ELEMENT*))XmlPrinterGetCompactMode;
	printer->first_element = TRUE;
	printer->target = target;
	printer->vprint_func = vprint_func;
	printer->depth = depth;
	printer->text_depth = -1;
	printer->process_entities = TRUE;
	printer->compact_mode = (char)compact;

	printer->visitor.visit_enter_document = (int (*)(XML_VISITOR*, const XML_DOCUMENT*))XmlPrinterVisitEnterDocument;
	printer->visitor.visit_enter_element = (int (*)(XML_VISITOR*, const XML_ELEMENT*, const XML_ATTRIBUTE*))XmlPrinterVisitEnterElement;
	printer->visitor.visit_exit_element = (int (*)(XML_VISITOR*, const XML_ELEMENT*))XmlPrinterVisitExitElement;
	printer->visitor.visit_text = (int (*)(XML_VISITOR*, const XML_TEXT*))XmlPrinterVisitText;
	printer->visitor.visit_comment = (int (*)(XML_VISITOR*, const XML_COMMENT*))XmlPrinterVisitComment;
	printer->visitor.visit_declaration = (int (*)(XML_VISITOR*, const XML_DECLARATION*))XmlPrinterVisitDeclaration;
	printer->visitor.visit_unknown = (int (*)(XML_VISITOR*, const XML_UNKNOWN*))XmlPrinterVisitUnknown;

	for(i=0; i<NUM_ENTITIES; i++)
	{
		ASSERT(g_entities[i].value < XML_PRINTER_ENTITY_RANGE);
		if(g_entities[i].value < XML_PRINTER_ENTITY_RANGE)
		{
			printer->entity_flag[g_entities[i].value] = TRUE;
		}
	}

	printer->restricted_entity_flag['&'] = TRUE;
	printer->restricted_entity_flag['<'] = TRUE;
	printer->restricted_entity_flag['>'] = TRUE;

	InitializeDynCharacterArray(&printer->buffer, 20);
	InitializeDynPointerArray(&printer->stack, 10);
}

void ReleaseXmlPrinter(XML_PRINTER* printer)
{
	ReleaseDynPointerArray(&printer->stack, NULL);
	ReleaseDynCharacterArray(&printer->buffer);
}

void XmlPrinterPrint(XML_PRINTER* printer, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	printer->vprint_func(printer->target, format, args);
}

void XmlPrinterPrintSpace(XML_PRINTER* printer, int depth)
{
	int i;
	for(i=0; i<depth; i++)
	{
		XmlPrinterPrint(printer, "    ");
	}
}

void XmlPrinterPrintString(XML_PRINTER* printer, const char* p, int restricted)
{
	// Look for runs of bytes between entities to print.
	const char *q = p;
	const char *flag = (restricted != FALSE) ?
		printer->restricted_entity_flag : printer->entity_flag;
	int i;

	if(printer->process_entities != FALSE)
	{
		while(*q != '\0')
		{
			// Remember, char is sometimes signed. (How many times has that bitten me?)
			if(*q > 0 && *q < XML_PRINTER_ENTITY_RANGE)
			{
				// Check for entities. If one is found, flush
				// the stream up until the entity, write the
				// entity, and keep looking.
				if(flag[(unsigned int)(*q)])
				{
					while(p < q)
					{
						XmlPrinterPrint(printer, "%c", *p);
						++p;
					}
					for(i=0; i<NUM_ENTITIES; ++i)
					{
						if(g_entities[i].value == *q)
						{
							XmlPrinterPrint(printer, "&%s;", g_entities[i].pattern);
							break;
						}
					}
					++p;
				}
			}
			++q;
		}
	}
	// Flush the remaining string. This will be the entire
	// string if an entity wasn't found.
	if(printer->process_entities == FALSE || (q-p > 0))
	{
		XmlPrinterPrint(printer, "%s", p );
	}
}

void XmlPrinterSealElement(XML_PRINTER* printer)
{
	printer->element_jsut_opened = FALSE;
	XmlPrinterPrint(printer, ">");
}

void XmlPrinterPushDeclaration(XML_PRINTER* printer, const char* value)
{
	if(printer->element_jsut_opened != FALSE)
	{
		XmlPrinterSealElement(printer);
	}
	if(printer->text_depth < 0 && printer->first_element == FALSE && printer->compact_mode == FALSE)
	{
		XmlPrinterPrint(printer, "\n");
		printer->print_space(printer, printer->depth);
	}
	printer->first_element = FALSE;
	XmlPrinterPrint(printer, "<?%s?>", value);
}

void XmlPrinterPushHeader(XML_PRINTER* printer, int write_bom, int write_dec)
{
	if(write_bom != FALSE)
	{
		const unsigned char bom[] = {XML_UTF_LEAD_0, XML_UTF_LEAD_1, XML_UTF_LEAD_2, '\0'};
		XmlPrinterPrint(printer, "%s", bom);
	}
	if(write_dec != FALSE)
	{
		XmlPrinterPushDeclaration(printer, "xml version=\"1.0\"");
	}
}

void XmlPrinterOpenElement(XML_PRINTER* printer, const char* name, int compact_mode)
{
	if(printer->element_jsut_opened != FALSE)
	{
		XmlPrinterSealElement(printer);
	}
	DynPointerArrayPush(&printer->stack, name);

	if(printer->text_depth < 0 && printer->first_element == FALSE && printer->compact_mode == FALSE)
	{
		XmlPrinterPrint(printer, "\n");
	}
	if(printer->compact_mode == FALSE)
	{
		printer->print_space(printer, printer->depth);
	}

	XmlPrinterPrint(printer, "<%s", name);
	printer->element_jsut_opened = TRUE;
	printer->first_element = FALSE;
	printer->depth++;
}

void XmlPrinterPushTextAttribute(XML_PRINTER* printer, const char* name, const char* value)
{
	ASSERT(printer->element_jsut_opened != FALSE);
	XmlPrinterPrint(printer, " %s=\"", name);
	XmlPrinterPrintString(printer, value, FALSE);
	XmlPrinterPrint(printer, "\"");
}

void XmlPrinterPushSignedAttribute(XML_PRINTER* printer, const char* name, int v)
{
	char buff[BUFFER_SIZE];
	SignedToString(v, buff);
	XmlPrinterPushTextAttribute(printer, name, buff);
}

void XmlPrinterPushUnsignedAttribute(XML_PRINTER* printer, const char* name, unsigned int v)
{
	char buff[BUFFER_SIZE];
	UnsignedToString(v, buff);
	XmlPrinterPushTextAttribute(printer, name, buff);
}

void XmlPrinterPushBooleanAttribute(XML_PRINTER* printer, const char* name, int v)
{
	char buff[BUFFER_SIZE];
	BooleanToString(v, buff);
	XmlPrinterPushTextAttribute(printer, name, buff);
}

void XmlPrinterPushDoubleAttribute(XML_PRINTER* printer, const char* name, double v)
{
	char buff[BUFFER_SIZE];
	DoubleToString(v, buff);
	XmlPrinterPushTextAttribute(printer, name, buff);
}

void XmlPrinterCloseElement(XML_PRINTER* printer,  int compact_mode)
{
	const char *name;
	printer->depth--;

	name = DynPointerArrayPop(&printer->stack);

	if(printer->element_jsut_opened != FALSE)
	{
		XmlPrinterPrint(printer, "/>" );
	}
	else
	{
		if(printer->text_depth < 0 && printer->compact_mode == FALSE)
		{
			XmlPrinterPrint(printer, "\n");
			printer->print_space(printer, printer->depth);
		}
		XmlPrinterPrint(printer, "</%s>", name);
	}

	if(printer->text_depth == printer->depth)
	{
		printer->text_depth = -1;
	}
	if(printer->depth == 0 && printer->compact_mode == FALSE)
	{
		XmlPrinterPrint(printer,  "\n");
	}
	printer->element_jsut_opened = FALSE;
}

void XmlPrinterPushText(XML_PRINTER* printer, const char* text, int cdata)
{
	printer->text_depth = printer->depth-1;

	if(printer->element_jsut_opened != FALSE)
	{
		XmlPrinterSealElement(printer);
	}
	if(cdata != FALSE)
	{
		XmlPrinterPrint(printer, "<![CDATA[");
		XmlPrinterPrint(printer, "%s", text);
		XmlPrinterPrint(printer, "]]>");
	}
	else
	{
		XmlPrinterPrintString(printer, text, TRUE);
	}
}

void XmlPrinterPushSignedText(XML_PRINTER* printer, int value)
{
	char buff[BUFFER_SIZE];
	SignedToString(value, buff);
	XmlPrinterPushText(printer, buff, FALSE);
}


void XmlPrinterPushUnsignedText(XML_PRINTER* printer, unsigned int value)
{
	char buff[BUFFER_SIZE];
	UnsignedToString(value, buff);
	XmlPrinterPushText(printer, buff, FALSE);
}

void XmlPrinterPushBooleanText(XML_PRINTER* printer, int value)
{
	char buff[BUFFER_SIZE];
	BooleanToString(value, buff);
	XmlPrinterPushText(printer, buff, FALSE);
}

void XmlPrinterPushFloatText(XML_PRINTER* printer, float value)
{
	char buff[BUFFER_SIZE];
	FloatToString(value, buff);
	XmlPrinterPushText(printer, buff, FALSE);
}

void XmlPrinterPushDoubleText(XML_PRINTER* printer, double value)
{
	char buff[BUFFER_SIZE];
	DoubleToString(value, buff);
	XmlPrinterPushText(printer, buff, FALSE);
}

void XmlPrinterPushComment(XML_PRINTER* printer, const char* comment)
{
	if(printer->element_jsut_opened != FALSE)
	{
		XmlPrinterSealElement(printer);
	}
	if(printer->text_depth < 0 && printer->first_element == FALSE && printer->compact_mode == FALSE)
	{
		XmlPrinterPrint(printer, "\n");
		printer->print_space(printer, printer->depth);
	}
	printer->first_element = FALSE;
	XmlPrinterPrint(printer, "<!--%s-->", comment);
}

void XmlPrinterPushUnknown(XML_PRINTER* printer, const char* value)
{
	if(printer->element_jsut_opened != FALSE)
	{
		XmlPrinterSealElement(printer);
	}
	if(printer->text_depth < 0 && printer->first_element == FALSE && printer->compact_mode == FALSE)
	{
		XmlPrinterPrint(printer, "\n");
		printer->print_space(printer, printer->depth);
	}
	printer->first_element = FALSE;
	XmlPrinterPrint(printer, "<!%s>", value);
}

static int XmlPrinterVisitEnterDocument(XML_PRINTER* printer, const XML_DOCUMENT* document)
{
	printer->process_entities = document->process_entities;

	if(document->write_bom != FALSE)
	{
		XmlPrinterPushHeader(printer, TRUE, FALSE);
	}
	return TRUE;
}

static int XmlPrinterVisitEnterElement(XML_PRINTER* printer, const XML_ELEMENT* element, const XML_ATTRIBUTE* attribute)
{
	const XML_ELEMENT *parent_element = element->node.parent->to_element(element->node.parent);
	int compact_mode = (parent_element != NULL) ?
		printer->get_compact_mode(printer, (XML_ELEMENT*)parent_element) : printer->compact_mode;
	XmlPrinterOpenElement(printer, XmlElementGetName((XML_ELEMENT*)element), compact_mode);
	while(attribute != NULL)
	{
		XmlPrinterPushTextAttribute(printer,
			XmlAttributeGetName((XML_ATTRIBUTE*)attribute), XmlAttributeGetValue((XML_ATTRIBUTE*)attribute));
		attribute = attribute->next;
	}
	return TRUE;
}

static int XmlPrinterVisitExitElement(XML_PRINTER* printer, const XML_ELEMENT* element)
{
	XmlPrinterCloseElement(printer, printer->get_compact_mode(printer, (XML_ELEMENT*)element));
	return TRUE;
}

static int XmlPrinterVisitText(XML_PRINTER* printer, const XML_TEXT* text)
{
	XmlPrinterPushText(printer, XmlNodeGetValue((XML_NODE*)&text->node), text->is_c_data);
	return TRUE;
}

static int XmlPrinterVisitComment(XML_PRINTER* printer, const XML_COMMENT* comment)
{
	XmlPrinterPushComment(printer, XmlNodeGetValue((XML_NODE*)&comment->node));
	return TRUE;
}

static int XmlPrinterVisitDeclaration(XML_PRINTER* printer, const XML_DECLARATION* declaration)
{
	XmlPrinterPushDeclaration(printer, XmlNodeGetValue((XML_NODE*)&declaration->node));
	return TRUE;
}

static int XmlPrinterVisitUnknown(XML_PRINTER* printer, const XML_UNKNOWN* unknown)
{
	XmlPrinterPushUnknown(printer, XmlNodeGetValue((XML_NODE*)&unknown->node));
	return TRUE;
}

#ifdef __cplusplus
}
#endif
