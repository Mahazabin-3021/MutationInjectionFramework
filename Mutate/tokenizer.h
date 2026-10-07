#pragma once
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Supported source languages
// ---------------------------------------------------------------------------
enum class Language { C, CPP, JAVA, CSHARP };

// ---------------------------------------------------------------------------
// Token
// ---------------------------------------------------------------------------
struct Token {
    enum class Type {
        KEYWORD,
        IDENTIFIER,
        NUMBER,
        STRING_LIT,
        CHAR_LIT,
        OPERATOR,
        DELIMITER,
        COMMENT_BLOCK,
        COMMENT_LINE,
        COMMENT_DOC,
        WHITESPACE,   // space / tab (one or more)
        NEWLINE,      // one '\n' (or "\r\n")
        END_OF_FILE
    };

    Type        type;
    std::string value;
    int         line = 0;
    int         col  = 0;
};

// ---------------------------------------------------------------------------
// Tokenizer
// ---------------------------------------------------------------------------
class Tokenizer {
public:
    Tokenizer(const std::string& source, Language lang);
    std::vector<Token> tokenize();

private:
    const std::string& src_;
    Language           lang_;
    std::size_t        pos_  = 0;
    int                line_ = 1;
    int                col_  = 1;

    char peek(int offset = 0) const;
    char advance();
    bool atEnd() const;

    Token scanWhitespace();
    Token scanNewline();
    Token scanLineComment();
    Token scanBlockComment(bool isDoc);
    Token scanString(char delim);
    Token scanChar();
    Token scanNumber();
    Token scanIdentifierOrKeyword();
    Token scanOperatorOrDelimiter();

    bool isKeyword(const std::string& word) const;
};

// ---------------------------------------------------------------------------
// Lossless reconstruction: join all token values in sequence
// ---------------------------------------------------------------------------
std::string reconstruct(const std::vector<Token>& tokens);
