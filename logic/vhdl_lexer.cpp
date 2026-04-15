#include "vhdl_lexer.h"

namespace deepiri {

const std::unordered_map<std::string, TokenType> VHDLLexer::keywords_ = {
    {"entity", TokenType::KEYWORD},
    {"architecture", TokenType::KEYWORD},
    {"signal", TokenType::KEYWORD},
    {"variable", TokenType::KEYWORD},
    {"constant", TokenType::KEYWORD},
    {"process", TokenType::KEYWORD},
    {"begin", TokenType::KEYWORD},
    {"end", TokenType::KEYWORD},
    {"if", TokenType::KEYWORD},
    {"then", TokenType::KEYWORD},
    {"else", TokenType::KEYWORD},
    {"elsif", TokenType::KEYWORD},
    {"case", TokenType::KEYWORD},
    {"when", TokenType::KEYWORD},
    {"for", TokenType::KEYWORD},
    {"loop", TokenType::KEYWORD},
    {"and", TokenType::KEYWORD},
    {"or", TokenType::KEYWORD},
    {"not", TokenType::KEYWORD},
    {"nand", TokenType::KEYWORD},
    {"nor", TokenType::KEYWORD},
    {"xor", TokenType::KEYWORD},
    {"xnor", TokenType::KEYWORD},
    {"in", TokenType::KEYWORD},
    {"out", TokenType::KEYWORD},
    {"inout", TokenType::KEYWORD},
    {"buffer", TokenType::KEYWORD},
    {"library", TokenType::KEYWORD},
    {"use", TokenType::KEYWORD},
    {"port", TokenType::KEYWORD},
    {"generic", TokenType::KEYWORD},
    {"map", TokenType::KEYWORD},
    {"component", TokenType::KEYWORD},
    {"port map", TokenType::KEYWORD},
    {"generic map", TokenType::KEYWORD},
    {"上升沿", TokenType::KEYWORD},
    {"rising_edge", TokenType::KEYWORD},
    {"falling_edge", TokenType::KEYWORD},
    {"after", TokenType::KEYWORD},
    {"wait", TokenType::KEYWORD},
    {"null", TokenType::KEYWORD},
    {"true", TokenType::KEYWORD},
    {"false", TokenType::KEYWORD},
    {"std_logic", TokenType::KEYWORD},
    {"std_logic_vector", TokenType::KEYWORD},
    {"bit", TokenType::KEYWORD},
    {"bit_vector", TokenType::KEYWORD},
    {"integer", TokenType::KEYWORD},
    {"natural", TokenType::KEYWORD},
    {"positive", TokenType::KEYWORD},
    {"real", TokenType::KEYWORD},
    {"time", TokenType::KEYWORD}
};

VHDLLexer::VHDLLexer(const std::string& source)
    : source_(source), current_(0), line_(1), column_(1) {}

std::vector<Token> VHDLLexer::tokenize() {
    std::vector<Token> tokens;
    while (!is_at_end()) {
        Token token = next_token();
        if (token.type != TokenType::COMMENT) {
            tokens.push_back(token);
        }
    }
    tokens.push_back({TokenType::EOF_TOKEN, "", line_, column_});
    return tokens;
}

Token VHDLLexer::next_token() {
    skip_whitespace();
    
    if (is_at_end()) {
        return {TokenType::EOF_TOKEN, "", line_, column_};
    }
    
    char c = peek();
    
    if (c == '-' && peek_ahead(1) == '-') {
        skip_comment();
        return {TokenType::COMMENT, "", line_, column_};
    }
    
    if (c == '"') {
        return scan_string();
    }
    
    if (c == '\'') {
        return scan_string();
    }
    
    if (std::isalpha(c) || c == '_') {
        return scan_identifier();
    }
    
    if (std::isdigit(c)) {
        return scan_number();
    }
    
    return scan_operator();
}

void VHDLLexer::reset() {
    current_ = 0;
    line_ = 1;
    column_ = 1;
}

bool VHDLLexer::is_keyword(const std::string& text) const {
    return keywords_.find(text) != keywords_.end();
}

std::string VHDLLexer::token_type_name(TokenType type) {
    switch (type) {
        case TokenType::KEYWORD: return "KEYWORD";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::INTEGER: return "INTEGER";
        case TokenType::STRING: return "STRING";
        case TokenType::OPERATOR: return "OPERATOR";
        case TokenType::DELIMITER: return "DELIMITER";
        case TokenType::COMMENT: return "COMMENT";
        case TokenType::EOF_TOKEN: return "EOF";
    }
    return "UNKNOWN";
}

char VHDLLexer::peek() const {
    if (is_at_end()) return '\0';
    return source_[current_];
}

char VHDLLexer::peek_ahead(int offset) const {
    if (current_ + offset >= source_.size()) return '\0';
    return source_[current_ + offset];
}

char VHDLLexer::advance() {
    if (is_at_end()) return '\0';
    char c = source_[current_++];
    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    return c;
}

bool VHDLLexer::is_at_end() const {
    return current_ >= source_.size();
}

void VHDLLexer::skip_whitespace() {
    while (!is_at_end()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            advance();
        } else {
            break;
        }
    }
}

void VHDLLexer::skip_comment() {
    while (!is_at_end() && peek() != '\n') {
        advance();
    }
}

Token VHDLLexer::scan_identifier() {
    int start_line = line_;
    int start_column = column_;
    std::string text;
    
    while (std::isalnum(peek()) || peek() == '_') {
        text += advance();
    }
    
    TokenType type = is_keyword(text) ? TokenType::KEYWORD : TokenType::IDENTIFIER;
    return {type, text, start_line, start_column};
}

Token VHDLLexer::scan_number() {
    int start_line = line_;
    int start_column = column_;
    std::string text;
    
    while (std::isdigit(peek())) {
        text += advance();
    }
    
    return {TokenType::INTEGER, text, start_line, start_column};
}

Token VHDLLexer::scan_string() {
    int start_line = line_;
    int start_column = column_;
    std::string text;
    char quote = advance();
    
    while (!is_at_end() && peek() != quote) {
        text += advance();
    }
    
    if (!is_at_end()) {
        advance();
    }
    
    return {TokenType::STRING, text, start_line, start_column};
}

Token VHDLLexer::scan_operator() {
    int start_line = line_;
    int start_column = column_;
    char c = advance();
    
    std::string op(1, c);
    
    if (c == '(' || c == ')' || c == '[' || c == ']' ||
        c == ',' || c == ';' || c == ':' || c == '=' ||
        c == '+' || c == '-' || c == '*' || c == '/') {
        return {TokenType::DELIMITER, op, start_line, start_column};
    }
    
    return {TokenType::OPERATOR, op, start_line, start_column};
}

}