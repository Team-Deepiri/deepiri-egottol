#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace deepiri {

enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    INTEGER,
    STRING,
    OPERATOR,
    DELIMITER,
    COMMENT,
    EOF_TOKEN
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
};

class VHDLLexer {
public:
    VHDLLexer(const std::string& source);
    
    std::vector<Token> tokenize();
    Token next_token();
    void reset();
    
    bool is_keyword(const std::string& text) const;
    static std::string token_type_name(TokenType type);

private:
    char peek() const;
    char peek_ahead(int offset) const;
    char advance();
    bool is_at_end() const;
    void skip_whitespace();
    void skip_comment();
    
    Token scan_identifier();
    Token scan_number();
    Token scan_string();
    Token scan_operator();
    
    std::string source_;
    size_t current_;
    int line_;
    int column_;
    
    static const std::unordered_map<std::string, TokenType> keywords_;
};

}