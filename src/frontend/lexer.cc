enum class Token_Type {
    EMPTY,
    //room for ASCII chars
    IDENTIFIER = 256,
    INTEGER,
    DECIMAL,
    SINGLE_QUOTES,
    DOUBLE_QUOTES,
    K_START,  //keywords start
    K_U8,
    K_F16,
    K_F32,
    K_F64,
    K_U16,
    K_U64,
    K_S64,
    K_U32,
    K_S8,
    K_S16,
    K_S32,
    K_PROC,
    K_IF,
    K_STRUCT,
    K_FOR,
    K_CONSTANT,
    K_COMPTIME,
    K_END,    //keywords end
    ARROW,
    END_OF_FILE,
};

struct TokenOffset {
    u64 off;
    u16 len;
};
struct Lexer {
    DynamicArray<TokenOffset> tokenOffsets;
    DynamicArray<Token_Type> tokenTypes;
    char *fileName;
    char *fileContent;

    void emitErr(u64 off, char *fmt, ...) {
	if (report::errorOff == MAX_REPORTS) { return; };
	report::Error &rep = report::errors[report::errorOff];
	report::errorOff += 1;
	rep.fileName = fileName;
	rep.off = off;
	rep.fileContent = fileContent;
	if (fmt != nullptr) { rep.msg = report::reportBuff + report::reportBuffTop; };
	va_list args;
	va_start(args, fmt);
	report::reportBuffTop += vsprintf(report::reportBuff, fmt, args);
	va_end(args);
    };
};

Map keywords;

void initKeywords() {
    const u8 keywordCount = (u32)Token_Type::K_END - (u32)Token_Type::K_START - 1;
    struct KeywordData {
	const char *str;
	const Token_Type type;
    };
    KeywordData data[keywordCount] = {
	{"u8", Token_Type::K_U8},
	{"f16", Token_Type::K_F16},
	{"f32", Token_Type::K_F32},
	{"f64", Token_Type::K_F64},
	{"u16", Token_Type::K_U16},
	{"u32", Token_Type::K_U32},
	{"u64", Token_Type::K_U64},
	{"s64", Token_Type::K_S64},
	{"s8", Token_Type::K_S8},
	{"s16", Token_Type::K_S16},
	{"s32", Token_Type::K_S32 },
	{"proc", Token_Type::K_PROC},
	{"if", Token_Type::K_IF},
	{"struct", Token_Type::K_STRUCT},
	{"for", Token_Type::K_FOR},
	{"const", Token_Type::K_CONSTANT},
	{"comptime", Token_Type::K_COMPTIME},
    };
    keywords.init(keywordCount);
    
    for (u8 i = 0; i < keywordCount; i += 1) {
	s32 k = keywords.insertValue({(char*)data[i].str, (u32)strlen(data[i].str) }, (u16)data[i].type);
#if(XE_DBG)
	if (k == -1) {
	    printf("\n[LEXER] Could not register keyword\n");
	    return;
	};
#endif
    };
};
void uninitKeywords() { keywords.uninit(); };

bool isKeyword(Token_Type type) { return type > Token_Type::K_START && type < Token_Type::K_END; };
bool isType(Token_Type type) {
    return (type>Token_Type::K_START && type<Token_Type::K_PROC) || type == Token_Type::IDENTIFIER;
};

Lexer createLexer(char *filePath) {
    Lexer lexer;
	
    lexer.fileName = os::getFileFullName(filePath);
	
    FILE *fp = fopen(lexer.fileName, "r");
    if (fp == nullptr) {
	lexer.fileContent = nullptr;
	return lexer;
    };
    fseek(fp, 0, SEEK_END);
    u64 size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    lexer.fileContent = (char*)mem::alloc(size + 17); //one for newline in the start, and 16 for SIMD padding(comments,etc...)
    lexer.fileContent[0] = '\n'; //padding for getLineAndOff
    lexer.fileContent += 1;
    size = fread(lexer.fileContent, sizeof(char), size, fp);
    memset(lexer.fileContent + size, '\0', 16);

    //50% of the file size. @foodforthought: change percentage?
    u32 tokenCount = (u32)((50 * size) / 100) + 1;
    lexer.tokenTypes.init(tokenCount);
    lexer.tokenOffsets.init(tokenCount);

    return lexer;
};
void destroyLexer(Lexer &lexer) {
    mem::free(lexer.fileContent-1);
    lexer.tokenTypes.uninit();
    lexer.tokenOffsets.uninit();
};
void eatUnwantedChars(char *mem, u64& x) {
    while (true) {
	switch (mem[x]) {
	case ' ':
	case '\r':
	case '\t':
	    x += 1;
	    break;
	case '\0':
	default:
	    return;
	};
    };
};
void eatNewlines(DynamicArray<Token_Type> &tokTypes, u32 &x){
    while (tokTypes[x] == (Token_Type)'\n') { x += 1; };
};
b32 isAlpha(char x) { return (x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z'); };
b32 isNum(char x) { return (x >= '0' && x <= '9'); };
b32 genTokens(Lexer &lex) {
    TIME_BLOCK;
    u64 x = 0;
    char *src = lex.fileContent;
    Token_Type stringType = Token_Type::EMPTY;
    char c;
    eatUnwantedChars(src, x);
    while (src[x] != '\0') {
	switch (src[x]) {
	case '\'':
	    stringType = Token_Type::SINGLE_QUOTES;
	    c = '\'';
	case '"': {
	    if (stringType == Token_Type::EMPTY) {
		stringType = Token_Type::DOUBLE_QUOTES;
		c = '\"';
	    };
	    u64 off = x;
	    DOUBLE_QUOTE_TOP:
	    x += 1;
	    while (src[x] != c) {
		x += 1;
		if (src[x] == '\0' || src[x] == '\n') {
		    lex.emitErr(off, "Expected ending %s quotes", (c == '"')?"double":"single");
		    return false;
		};
	    };
	    if (src[x-1] == '\\') { goto DOUBLE_QUOTE_TOP; };
	    TokenOffset offset;
	    offset.off = off;  //offset of "
	    offset.len = (u16)(x-off-1);
	    lex.tokenOffsets.push(offset);
	    x += 1;
	    lex.tokenTypes.push(stringType);
	    stringType = Token_Type::EMPTY;
	} break;
	default: {
	    if (isAlpha(src[x]) || src[x] == '_') {
		u64 start = x;
		x += 1;
		while (isAlpha(src[x]) || src[x] == '_' || isNum(src[x])) { x += 1; };
		s32 type = keywords.getValue({src+start, (u32)(x-start)});
		if (type != -1) { lex.tokenTypes.push((Token_Type)type); }
		else { lex.tokenTypes.push(Token_Type::IDENTIFIER); };
		TokenOffset offset;
		offset.off = start;
		offset.len = (u16)(x-start);
		lex.tokenOffsets.push(offset);
	    } else if (isNum(src[x])) {
		u64 start = x;
		Token_Type numType = Token_Type::INTEGER;
	    CHECK_NUM_DEC:
		x += 1;
		while (isNum(src[x]) || src[x] == '_') { x += 1; };
		if (src[x] == '.') {
		    if (numType == Token_Type::DECIMAL) {
			lex.emitErr(start, "Decimal cannot have 2 decimals");
			return false;
		    };
		    numType = Token_Type::DECIMAL;
		    goto CHECK_NUM_DEC;
		};
		TokenOffset offset;
		offset.off = start;
		offset.len = (u16)(x-start);
		lex.tokenOffsets.push(offset);
		lex.tokenTypes.push(numType);
	    } else {
		Token_Type type = (Token_Type)src[x];
		if (src[x] == '-' && src[x+1] == '>') {
		    type = Token_Type::ARROW;
		    x += 1;
		} else if (src[x] == '/' && src[x + 1] == '/') {
		    x += 2;
		    //Since the src buffer is padded we do not have to worry
		    u32 times = 0;
		    char *mem = src+x;
		    s32 mask;
		    while (true) {
			__m128i tocmp = _mm_set1_epi8('\n');
			__m128i chunk = _mm_loadu_si128((const __m128i*)mem);
			__m128i results =  _mm_cmpeq_epi8(chunk, tocmp);
			mask = _mm_movemask_epi8(results);
			if (mask != 0) {break;};
			tocmp = _mm_set1_epi8('\0');
			results =  _mm_cmpeq_epi8(chunk, tocmp);
			mask = _mm_movemask_epi8(results);
			if (mask == 0) {
			    mem += 16;
			    times += 16;
			    continue;
			};
			return true;
		    };
		    u32 xy = 0;
		    while (IS_BIT(mask, xy) == 0) { xy += 1; };
		    x += times + xy + 1;
		    eatUnwantedChars(src, x);
		    continue;
		} else if (src[x] == '/' && src[x+1] == '*') {
		    u8 level = 1;
		    u64 beg = x;
		    x += 3;
		    while (level != 0) {
			__m128i tocmp = _mm_set1_epi8('/');
			__m128i chunk = _mm_loadu_si128((const __m128i*)(src+x));
			__m128i results =  _mm_cmpeq_epi8(chunk, tocmp);
			s32 frontslashMask = _mm_movemask_epi8(results);
			tocmp = _mm_set1_epi8('*');
			results =  _mm_cmpeq_epi8(chunk, tocmp);
			s32 startMask = _mm_movemask_epi8(results);
			tocmp = _mm_set1_epi8('\0');
			results =  _mm_cmpeq_epi8(chunk, tocmp);
			s32 nullbyteMask = _mm_movemask_epi8(results);
			s32 mask = frontslashMask | startMask | nullbyteMask;
			if (mask == 0){
			    x += 16;
			    continue;
			};
			u32 y = 0;
			while (mask != 0) {
			    while (IS_BIT(mask, y) == 0) {
				x += 1;
				y += 1; };
			    CLEAR_BIT(mask, y);
			    y += 1;
			    switch (src[x]) {
			    case '\0': {
				lex.emitErr(beg, "%d multi line comment%snot terminated", level, (level==1)?" ":"s ");
				return false;
			    } break;
			    case '*': {
				x += 1;
				if (src[x] == '/') {
				    level -= 1;
				    CLEAR_BIT(mask, y);
				};
			    } break;
			    case '/': {
				x += 1;
				if (src[x] == '*') {
				    level += 1;
				    CLEAR_BIT(mask, y);
				};
			    } break;
			    };
			    x += 1;
			    y += 1;
			};
		    };
		    eatUnwantedChars(src, x);
		    continue;
		};
		TokenOffset offset;
		offset.off = x;
		lex.tokenOffsets.push(offset);
		lex.tokenTypes.push(type);
		x += 1;
	    };
	} break;
	};
	eatUnwantedChars(src, x);
    };
    lex.tokenTypes.push(Token_Type::END_OF_FILE);
    lex.tokenOffsets.push( { x, 0 });
    return true;
};

#if(XE_DBG)
namespace dbg {
    void dumpLexerStat(Lexer &lexer) {
	printf("\n[LEXER_STAT]\nfileName: %s\ntokenCount: %d\n----\n", lexer.fileName, lexer.tokenTypes.count);
    };
    void dumpLexerTokens(Lexer &lexer) {
	printf("\n[LEXER_TOKENS]");
	for (u32 x = 0; x < lexer.tokenTypes.count; x += 1) {
	    printf("\n[TOKEN]\n");
	    u32 line = 1;
	    u32 off = 1;
	    report::getLineAndOff(lexer.fileContent, lexer.tokenOffsets[x].off, line, off);
	    printf("line: %d off: %d\n", line, off);
	    switch (lexer.tokenTypes[x]) {
	    case Token_Type::DOUBLE_QUOTES: {
		printf("double_quotes\n%.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off + 1);
	    } break;
	    case Token_Type::SINGLE_QUOTES: {
		printf("single_quotes\n%.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off + 1);
	    } break;
	    case Token_Type::IDENTIFIER: {
		printf("identifier\n%.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off);
	    } break;
	    case Token_Type::INTEGER: {
		printf("integer\n%.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off);
	    } break;
	    case Token_Type::DECIMAL: {
		printf("decimal\n%.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off);
	    } break;
	    case Token_Type::END_OF_FILE: printf("end_of_file"); break;
	    case (Token_Type)'\n': printf("new_line"); break;
	    case Token_Type::ARROW: printf("->"); break;
	    default:
		if (lexer.tokenTypes[x] >= Token_Type::K_START && lexer.tokenTypes[x] <= Token_Type::K_END) {
		    printf("keyword\n%.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off);
		}
		else { printf("%c", lexer.tokenTypes[x]); };
		break;
	    };
	};
	printf("\n----\n");
    };
};
#endif
