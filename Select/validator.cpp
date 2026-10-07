#include "validator.h"
#include "../Mutate/tokenizer.h"
#include <vector>
#include <string>
#include <iostream>

static Language getLanguageEnum(const std::string& langStr) {
    if (langStr == "c") return Language::C;
    if (langStr == "cpp" || langStr == "c++") return Language::CPP;
    if (langStr == "java") return Language::JAVA;
    if (langStr == "csharp" || langStr == "cs") return Language::CSHARP;
    return Language::CPP;
}

static bool check_mCW_A(const std::vector<Token>& tokens) {
    for (size_t i = 0; i + 1 < tokens.size(); ++i) {
        if (tokens[i].type == Token::Type::END_OF_FILE || tokens[i+1].type == Token::Type::END_OF_FILE) break;
        if (tokens[i].type == Token::Type::WHITESPACE || tokens[i].type == Token::Type::NEWLINE) continue;
        if (tokens[i+1].type == Token::Type::WHITESPACE || tokens[i+1].type == Token::Type::NEWLINE) continue;
        return true;
    }
    return false;
}

static bool check_mCW_R(const std::vector<Token>& tokens) {
    auto needsSep = [](Token::Type a, Token::Type b) {
        auto isWord = [](Token::Type t) { return t == Token::Type::KEYWORD || t == Token::Type::IDENTIFIER || t == Token::Type::NUMBER; };
        return isWord(a) && isWord(b);
    };
    auto prevCode = [&](size_t i) -> int {
        for (int j = (int)i - 1; j >= 0; --j) {
            auto t = tokens[j].type;
            if (t != Token::Type::WHITESPACE && t != Token::Type::NEWLINE) return j;
        }
        return -1;
    };
    auto nextCode = [&](size_t i) -> int {
        for (size_t j = i + 1; j < tokens.size(); ++j) {
            auto t = tokens[j].type;
            if (t != Token::Type::WHITESPACE && t != Token::Type::NEWLINE && t != Token::Type::END_OF_FILE) return j;
        }
        return -1;
    };
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].type != Token::Type::WHITESPACE) continue;
        int prev = prevCode(i);
        int next = nextCode(i);
        if (prev >= 0 && next >= 0 && !needsSep(tokens[prev].type, tokens[next].type)) return true;
    }
    return false;
}

static bool check_mCC_BT(const std::vector<Token>& tokens) {
    for (size_t i = 0; i + 1 < tokens.size(); ++i) {
        if (tokens[i].type == Token::Type::END_OF_FILE || tokens[i+1].type == Token::Type::END_OF_FILE) break;
        if (tokens[i].type == Token::Type::WHITESPACE || tokens[i].type == Token::Type::NEWLINE || tokens[i+1].type == Token::Type::NEWLINE || tokens[i+1].type == Token::Type::WHITESPACE) continue;
        if (tokens[i].type == Token::Type::STRING_LIT || tokens[i].type == Token::Type::CHAR_LIT) continue;
        return true;
    }
    return false;
}

static bool check_mCC_EOL(const std::vector<Token>& tokens) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].type != Token::Type::NEWLINE) continue;
        bool hasCode = false;
        for (int j = (int)i - 1; j >= 0; --j) {
            auto t = tokens[j].type;
            if (t == Token::Type::NEWLINE) break;
            if (t != Token::Type::WHITESPACE && t != Token::Type::COMMENT_LINE && t != Token::Type::COMMENT_DOC && t != Token::Type::END_OF_FILE) {
                hasCode = true; break;
            }
        }
        if (!hasCode) continue;
        bool alreadyComment = false;
        for (int j = (int)i - 1; j >= 0; --j) {
            auto t = tokens[j].type;
            if (t == Token::Type::NEWLINE) break;
            if (t == Token::Type::COMMENT_LINE || t == Token::Type::COMMENT_DOC) { alreadyComment = true; break; }
        }
        if (!alreadyComment) return true;
    }
    return false;
}

static bool check_mCF_A(const std::vector<Token>& tokens) {
    for (size_t i = 0; i + 1 < tokens.size(); ++i) {
        if (tokens[i].type == Token::Type::END_OF_FILE || tokens[i+1].type == Token::Type::END_OF_FILE) break;
        if (tokens[i].type == Token::Type::NEWLINE || tokens[i+1].type == Token::Type::NEWLINE) continue;
        if (tokens[i].type == Token::Type::STRING_LIT || tokens[i].type == Token::Type::COMMENT_BLOCK) continue;
        return true;
    }
    return false;
}

static bool check_mCF_R(const std::vector<Token>& tokens) {
    for (const auto& t : tokens) {
        if (t.type == Token::Type::NEWLINE) return true;
    }
    return false;
}

static bool check_mCF_SL(const std::vector<Token>& tokens) {
    for (const auto& t : tokens) {
        if (t.type == Token::Type::NEWLINE) return true;
    }
    return false;
}

static bool check_mSRI(const std::vector<Token>& tokens) {
    for (const auto& t : tokens) {
        if (t.type == Token::Type::IDENTIFIER) return true;
    }
    return false;
}

static bool check_mARI(const std::vector<Token>& tokens) {
    for (const auto& t : tokens) {
        if (t.type == Token::Type::IDENTIFIER) return true;
    }
    return false;
}

static bool check_mRL_N(const std::vector<Token>& tokens) {
    for (const auto& t : tokens) {
        if (t.type == Token::Type::NUMBER) return true;
    }
    return false;
}

static bool check_mRL_S(const std::vector<Token>& tokens) {
    for (const auto& t : tokens) {
        if (t.type == Token::Type::STRING_LIT || t.type == Token::Type::CHAR_LIT) return true;
    }
    return false;
}

static bool check_mSIL(const std::vector<Token>& tokens) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].value == "(" && tokens[i].type == Token::Type::DELIMITER) {
            int prev = -1;
            for (int j = (int)i - 1; j >= 0; --j) {
                if (tokens[j].type != Token::Type::WHITESPACE && tokens[j].type != Token::Type::NEWLINE) { prev = j; break; }
            }
            if (prev >= 0 && (tokens[prev].type == Token::Type::IDENTIFIER || tokens[prev].type == Token::Type::KEYWORD)) {
                bool isControlFlow = false;
                std::string kw[] = {"if", "while", "for", "switch", "catch"};
                for (const auto& k : kw) if (tokens[prev].value == k) { isControlFlow = true; break; }
                if (!isControlFlow) return true;
            }
        }
    }
    return false;
}

static bool check_mSDL(const std::vector<Token>& tokens) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].value == "(" && tokens[i].type == Token::Type::DELIMITER) {
            int prev = -1;
            for (int j = (int)i - 1; j >= 0; --j) {
                if (tokens[j].type != Token::Type::WHITESPACE && tokens[j].type != Token::Type::NEWLINE) { prev = j; break; }
            }
            if (prev >= 0 && (tokens[prev].type == Token::Type::IDENTIFIER || tokens[prev].type == Token::Type::KEYWORD)) {
                bool isControlFlow = false;
                std::string kw[] = {"if", "while", "for", "switch", "catch"};
                for (const auto& k : kw) if (tokens[prev].value == k) { isControlFlow = true; break; }
                if (isControlFlow) continue;
                
                int depth = 1;
                bool hasComma = false;
                for (size_t j = i + 1; j < tokens.size(); ++j) {
                    if (tokens[j].type == Token::Type::END_OF_FILE) break;
                    if (tokens[j].value == "(" || tokens[j].value == "[") ++depth;
                    else if (tokens[j].value == ")" || tokens[j].value == "]") --depth;
                    else if (tokens[j].value == "," && depth == 1) { hasComma = true; break; }
                    if (depth == 0) break;
                }
                if (hasComma) return true;
            }
        }
    }
    return false;
}

static std::vector<std::pair<size_t, size_t>> getLines(const std::vector<Token>& tokens) {
    std::vector<std::pair<size_t, size_t>> lines;
    size_t start = 0;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].type == Token::Type::NEWLINE || tokens[i].type == Token::Type::END_OF_FILE) {
            if (start < i) lines.push_back({start, i});
            start = i + 1;
        }
    }
    if (start < tokens.size()) lines.push_back({start, tokens.size()});
    return lines;
}

static bool check_mIL(const std::vector<Token>& tokens) {
    return getLines(tokens).size() >= 5;
}

static bool check_mDL(const std::vector<Token>& tokens) {
    auto lines = getLines(tokens);
    if (lines.size() < 5) return false;
    for (size_t i = 2; i < lines.size() - 2; ++i) {
        int lastT = -1;
        for (int j = (int)lines[i].second - 1; j >= (int)lines[i].first; --j) {
            auto t = tokens[j].type;
            if (t != Token::Type::WHITESPACE && t != Token::Type::NEWLINE && t != Token::Type::COMMENT_LINE && t != Token::Type::COMMENT_DOC && t != Token::Type::COMMENT_BLOCK) {
                lastT = j; break;
            }
        }
        if (lastT >= 0) {
            std::string val = tokens[lastT].value;
            if (val == ";" || val == "}") return true;
        }
    }
    return false;
}

static bool check_mML(const std::vector<Token>& tokens) {
    auto lines = getLines(tokens);
    if (lines.size() < 5) return false;
    for (size_t i = 2; i < lines.size() - 2; ++i) {
        int lastT = -1;
        for (int j = (int)lines[i].second - 1; j >= (int)lines[i].first; --j) {
            auto t = tokens[j].type;
            if (t != Token::Type::WHITESPACE && t != Token::Type::NEWLINE && t != Token::Type::COMMENT_LINE && t != Token::Type::COMMENT_DOC && t != Token::Type::COMMENT_BLOCK) {
                lastT = j; break;
            }
        }
        if (lastT >= 0 && tokens[lastT].value == ";") return true;
    }
    return false;
}

bool validateFunctionCandidate(const std::string& funcText, const std::string& langStr) {
    Tokenizer tokenizer(funcText, getLanguageEnum(langStr));
    auto tokens = tokenizer.tokenize();

    return check_mCW_A(tokens) &&
           check_mCW_R(tokens) &&
           check_mCC_BT(tokens) &&
           check_mCC_EOL(tokens) &&
           check_mCF_A(tokens) &&
           check_mCF_R(tokens) &&
           check_mCF_SL(tokens) &&
           check_mSRI(tokens) &&
           check_mARI(tokens) &&
           check_mRL_N(tokens) &&
           check_mRL_S(tokens) &&
           check_mSIL(tokens) &&
           check_mSDL(tokens) &&
           check_mIL(tokens) &&
           check_mDL(tokens) &&
           check_mML(tokens);
}
