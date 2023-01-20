enum class Token_Type {
	EMPTY,
	//room for ASCII chars
	IDENTIFIER = 256,
	INTEGER,
	DECIMAL,
	SINGLE_QUOTES,
	DOUBLE_QUOTES,
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
};

Lexer createLexer(char *filePath) {
	Lexer lexer;

#if(XE_PLAT_WIN)
	char fullFilePathBuff[1024];
	u32 len = GetFullPathNameA(filePath, 1024, fullFilePathBuff, NULL);
	lexer.fileName = (char*)mem::salloc(len);
	memcpy(lexer.fileName, fullFilePathBuff, len + 1);
#endif
	
	FILE *fp = fopen(fullFilePathBuff, "r");
	if (fp == nullptr) {
		lexer.fileContent = nullptr;
		return lexer;
	};
	fseek(fp, 0, SEEK_END);
	u64 size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	lexer.fileContent = (char*)mem::alloc(size + 1);
	lexer.fileContent[0] = '\n'; //padding for getLineAndOff
	lexer.fileContent += 1;
	size = fread(lexer.fileContent, sizeof(char), size, fp);
	lexer.fileContent[size-1] = '\0';

	//50% of the file size. @foodforthought: change percentage?
	u32 tokenCount = (u32)((50 * size) / 100);
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
			case '\t':
				x += 1;
				break;
			case '\0':
			default:
				return;
		};
	};
};
LineOff getLineAndOff(char *mem, u64 offset) {
	LineOff lo = { 1, 1 };
	for (u64 i = 0; i < offset; i += 1) {
		if (mem[i] == '\n') { lo.line += 1; };
	};
	while (mem[offset-lo.off] != '\n') {
		lo.off += 1;
	};
	return lo;
};
b32 isAlpha(char x) { return (x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z'); };
b32 isNum(char x) { return (x >= '0' && x <= '9'); };
b32 genTokens(Lexer &lex) {
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
						emitErr(lex.fileName,
						        getLineAndOff(lex.fileContent, off),
						        "Expected ending %s quotes", (c == '"')?"double":"single");
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
					while (isAlpha(src[x]) || src[x] == '_') { x += 1; };
					lex.tokenTypes.push(Token_Type::IDENTIFIER);
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
							emitErr(lex.fileName,
							        getLineAndOff(lex.fileContent, start),
							        "Decimal cannot have 2 decimals"
							        );
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
					TokenOffset offset;
					offset.off = x;
					lex.tokenOffsets.push(offset);
					lex.tokenTypes.push((Token_Type)src[x]);
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
void dumpLexerStat(Lexer &lexer) {
	printf("\n[LEXER_STAT]\nfileName: %s\ntokenCount: %d\n----\n", lexer.fileName, lexer.tokenTypes.count);
};
void dumpLexerTokens(Lexer &lexer) {
	printf("\n[LEXER_TOKENS]");
	for (u32 x = 0; x < lexer.tokenTypes.count; x += 1) {
		printf("\n[TOKEN]\n");
		LineOff lo = getLineAndOff(lexer.fileContent, lexer.tokenOffsets[x].off);
		printf("line: %d off: %d\n", lo.line, lo.off);
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
			case Token_Type::END_OF_FILE: {
				printf("end_of_file");
			} break;
			case (Token_Type)'\n':
				printf("new_line"); break;
			default:
				printf("%c", lexer.tokenTypes[x]); break;
		};
	};
	printf("\n----\n");
};
#endif
