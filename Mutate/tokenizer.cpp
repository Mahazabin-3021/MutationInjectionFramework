#include "tokenizer.h"
#include <cctype>
#include <stdexcept>
#include <unordered_set>
#include <sstream>

// ---------------------------------------------------------------------------
// Language keyword tables
// ---------------------------------------------------------------------------
static const std::unordered_set<std::string> kw_c = {
    "auto","break","case","char","const","continue","default","do",
    "double","else","enum","extern","float","for","goto","if","inline",
    "int","long","register","restrict","return","short","signed","sizeof",
    "static","struct","switch","typedef","union","unsigned","void",
    "volatile","while"
};
static const std::unordered_set<std::string> kw_cpp = {
    "alignas","alignof","and","and_eq","asm","auto","bitand","bitor","bool",
    "break","case","catch","char","char8_t","char16_t","char32_t","class",
    "compl","concept","const","consteval","constexpr","constinit","const_cast",
    "continue","co_await","co_return","co_yield","decltype","default","delete",
    "do","double","dynamic_cast","else","enum","explicit","export","extern",
    "false","float","for","friend","goto","if","inline","int","long","mutable",
    "namespace","new","noexcept","not","not_eq","nullptr","operator","or",
    "or_eq","private","protected","public","reinterpret_cast","requires",
    "return","short","signed","sizeof","static","static_assert","static_cast",
    "struct","switch","template","this","thread_local","throw","true","try",
    "typedef","typeid","typename","union","unsigned","using","virtual","void",
    "volatile","wchar_t","while","xor","xor_eq","override","final"
};
static const std::unordered_set<std::string> kw_java = {
    "abstract","assert","boolean","break","byte","case","catch","char","class",
    "const","continue","default","do","double","else","enum","extends","final",
    "finally","float","for","goto","if","implements","import","instanceof","int",
    "interface","long","native","new","null","package","private","protected",
    "public","return","short","static","strictfp","super","switch","synchronized",
    "this","throw","throws","transient","true","false","try","void","volatile","while"
};
static const std::unordered_set<std::string> kw_csharp = {
    "abstract","as","base","bool","break","byte","case","catch","char","checked",
    "class","const","continue","decimal","default","delegate","do","double","else",
    "enum","event","explicit","extern","false","finally","fixed","float","for",
    "foreach","goto","if","implicit","in","int","interface","internal","is","lock",
    "long","namespace","new","null","object","operator","out","override","params",
    "private","protected","public","readonly","ref","return","sbyte","sealed",
    "short","sizeof","stackalloc","static","string","struct","switch","this","throw",
    "true","try","typeof","uint","ulong","unchecked","unsafe","ushort","using",
    "virtual","void","volatile","while","async","await","var","dynamic","record",
    "init","with","global","nint","nuint"
};

// ---------------------------------------------------------------------------
// Tokenizer ctor
// ---------------------------------------------------------------------------
Tokenizer::Tokenizer(const std::string& source, Language lang)
    : src_(source), lang_(lang) {}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
char Tokenizer::peek(int offset) const {
    std::size_t idx = pos_ + static_cast<std::size_t>(offset);
    if (idx >= src_.size()) return '\0';
    return src_[idx];
}
char Tokenizer::advance() {
    char c = src_[pos_++];
    if (c == '\n') { ++line_; col_ = 1; }
    else { ++col_; }
    return c;
}
bool Tokenizer::atEnd() const { return pos_ >= src_.size(); }

bool Tokenizer::isKeyword(const std::string& word) const {
    switch (lang_) {
        case Language::C:      return kw_c.count(word)      > 0;
        case Language::CPP:    return kw_cpp.count(word)    > 0;
        case Language::JAVA:   return kw_java.count(word)   > 0;
        case Language::CSHARP: return kw_csharp.count(word) > 0;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Scan helpers
// ---------------------------------------------------------------------------
Token Tokenizer::scanWhitespace() {
    int sl = line_, sc = col_;
    std::string val;
    while (!atEnd() && (peek() == ' ' || peek() == '\t' || peek() == '\r'))
        val += advance();
    return { Token::Type::WHITESPACE, val, sl, sc };
}

Token Tokenizer::scanNewline() {
    int sl = line_, sc = col_;
    std::string val;
    if (peek() == '\r' && peek(1) == '\n') {
        val += advance(); val += advance(); // CRLF
    } else {
        val += advance(); // LF
    }
    return { Token::Type::NEWLINE, val, sl, sc };
}

Token Tokenizer::scanLineComment() {
    int sl = line_, sc = col_;
    std::string val;
    while (!atEnd() && peek() != '\n') val += advance();
    return { Token::Type::COMMENT_LINE, val, sl, sc };
}

Token Tokenizer::scanBlockComment(bool isDoc) {
    int sl = line_, sc = col_;
    std::string val;
    // already consumed opening: /* or /**  or ///
    // For /// (C# doc) it's actually a line comment
    if (isDoc && lang_ == Language::CSHARP) {
        // /// is treated as a line comment style
        while (!atEnd() && peek() != '\n') val += advance();
        return { Token::Type::COMMENT_DOC, val, sl, sc };
    }
    // /* ... */
    while (!atEnd()) {
        if (peek() == '*' && peek(1) == '/') {
            val += advance(); val += advance(); break;
        }
        val += advance();
    }
    return { isDoc ? Token::Type::COMMENT_DOC : Token::Type::COMMENT_BLOCK, val, sl, sc };
}

Token Tokenizer::scanString(char delim) {
    int sl = line_, sc = col_;
    std::string val;
    val += advance(); // opening quote
    while (!atEnd()) {
        char c = peek();
        if (c == '\\') { val += advance(); if (!atEnd()) val += advance(); continue; }
        val += advance();
        if (c == delim) break;
    }
    return { Token::Type::STRING_LIT, val, sl, sc };
}

Token Tokenizer::scanChar() {
    int sl = line_, sc = col_;
    std::string val;
    val += advance(); // opening '
    while (!atEnd()) {
        char c = peek();
        if (c == '\\') { val += advance(); if (!atEnd()) val += advance(); continue; }
        val += advance();
        if (c == '\'') break;
    }
    return { Token::Type::CHAR_LIT, val, sl, sc };
}

Token Tokenizer::scanNumber() {
    int sl = line_, sc = col_;
    std::string val;
    // hex?
    if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'X')) {
        val += advance(); val += advance();
        while (!atEnd() && std::isxdigit((unsigned char)peek())) val += advance();
    } else {
        while (!atEnd() && std::isdigit((unsigned char)peek())) val += advance();
        if (!atEnd() && peek() == '.') {
            val += advance();
            while (!atEnd() && std::isdigit((unsigned char)peek())) val += advance();
        }
        if (!atEnd() && (peek() == 'e' || peek() == 'E')) {
            val += advance();
            if (!atEnd() && (peek() == '+' || peek() == '-')) val += advance();
            while (!atEnd() && std::isdigit((unsigned char)peek())) val += advance();
        }
    }
    // suffix: f, l, u, L, etc.
    while (!atEnd() && (peek() == 'f' || peek() == 'F' || peek() == 'l' ||
                        peek() == 'L' || peek() == 'u' || peek() == 'U'))
        val += advance();
    return { Token::Type::NUMBER, val, sl, sc };
}

Token Tokenizer::scanIdentifierOrKeyword() {
    int sl = line_, sc = col_;
    std::string val;
    while (!atEnd() && (std::isalnum((unsigned char)peek()) || peek() == '_'))
        val += advance();
    Token::Type t = isKeyword(val) ? Token::Type::KEYWORD : Token::Type::IDENTIFIER;
    return { t, val, sl, sc };
}

Token Tokenizer::scanOperatorOrDelimiter() {
    int sl = line_, sc = col_;
    std::string val;
    val += advance();

    // Two-char operators
    if (!atEnd()) {
        char n = peek();
        std::string two = val + n;
        static const std::unordered_set<std::string> two_ops = {
            "->","++","--","<<",">>","<=",">=","==","!=","&&","||",
            "+=","-=","*=","/=","%=","&=","|=","^=","<<=",">>=",
            "::","=>","??","?.",".."
        };
        if (two_ops.count(two)) {
            val += advance();
            // Three-char?
            if (!atEnd()) {
                std::string three = val + peek();
                if (three == "<<=" || three == ">>=") val += advance();
            }
        }
    }

    // Delimiters
    static const std::string delims = "(){}[];,";
    if (val.size() == 1 && delims.find(val[0]) != std::string::npos)
        return { Token::Type::DELIMITER, val, sl, sc };

    return { Token::Type::OPERATOR, val, sl, sc };
}

// ---------------------------------------------------------------------------
// Main tokenize loop
// ---------------------------------------------------------------------------
std::vector<Token> Tokenizer::tokenize() {
    std::vector<Token> tokens;
    while (!atEnd()) {
        char c = peek();

        // Newline
        if (c == '\n' || (c == '\r' && peek(1) == '\n')) {
            tokens.push_back(scanNewline()); continue;
        }
        // CR alone → whitespace
        if (c == '\r') { tokens.push_back(scanWhitespace()); continue; }
        // Whitespace
        if (c == ' ' || c == '\t') { tokens.push_back(scanWhitespace()); continue; }
        // C# verbatim string @"..."
        if (c == '@' && lang_ == Language::CSHARP && peek(1) == '"') {
            int sl = line_, sc = col_;
            std::string val;
            val += advance(); // @
            val += advance(); // "
            while (!atEnd()) {
                if (peek() == '"') {
                    val += advance();
                    if (!atEnd() && peek() == '"') { val += advance(); continue; } // escaped ""
                    break;
                }
                val += advance();
            }
            tokens.push_back({ Token::Type::STRING_LIT, val, sl, sc }); continue;
        }
        // Comments
        if (c == '/') {
            if (peek(1) == '/') {
                int sl = line_, sc = col_;
                std::string pfx;
                pfx += advance(); pfx += advance(); // "//"
                if (lang_ == Language::CSHARP && !atEnd() && peek() == '/') {
                    pfx += advance(); // "///"
                    Token t = scanBlockComment(true);
                    t.value = pfx + t.value; t.line = sl; t.col = sc;
                    tokens.push_back(t);
                } else {
                    Token t = scanLineComment();
                    t.value = pfx + t.value; t.line = sl; t.col = sc;
                    tokens.push_back(t);
                }
                continue;
            }
            if (peek(1) == '*') {
                int sl = line_, sc = col_;
                std::string pfx;
                pfx += advance(); pfx += advance(); // "/*"
                bool isDoc = (!atEnd() && peek() == '*' && peek(1) != '/');
                if (isDoc) pfx += advance(); // "/**"
                Token t = scanBlockComment(isDoc);
                t.value = pfx + t.value; t.line = sl; t.col = sc;
                tokens.push_back(t); continue;
            }
        }
        // Java doc comment /** ... */
        // (handled above via /* path)

        // String literals
        if (c == '"') { tokens.push_back(scanString('"')); continue; }
        if (c == '\'') { tokens.push_back(scanChar()); continue; }

        // Numbers
        if (std::isdigit((unsigned char)c) ||
            (c == '.' && std::isdigit((unsigned char)peek(1)))) {
            tokens.push_back(scanNumber()); continue;
        }

        // Identifiers / keywords
        if (std::isalpha((unsigned char)c) || c == '_') {
            tokens.push_back(scanIdentifierOrKeyword()); continue;
        }

        // Everything else: operator / delimiter
        tokens.push_back(scanOperatorOrDelimiter());
    }
    tokens.push_back({ Token::Type::END_OF_FILE, "", line_, col_ });
    return tokens;
}

// ---------------------------------------------------------------------------
// Reconstruct
// ---------------------------------------------------------------------------
std::string reconstruct(const std::vector<Token>& tokens) {
    std::string out;
    for (const auto& t : tokens) {
        if (t.type == Token::Type::END_OF_FILE) break;
        out += t.value;
    }
    return out;
}
