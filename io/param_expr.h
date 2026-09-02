#pragma once

#include <cctype>
#include <cmath>
#include <functional>
#include <map>
#include <string>

namespace deepiri {

// Evaluate a simple arithmetic expression with SPICE units and {param} / bare params.
// Supports + - * / ( ) and engineering suffixes on literals.
inline bool evalSpiceExpr(
    const std::string& exprIn,
    const std::map<std::string, double>& params,
    double& out
) {
    std::string expr;
    expr.reserve(exprIn.size());
    for (char c : exprIn) {
        if (!std::isspace(static_cast<unsigned char>(c))) expr.push_back(c);
    }
    if (expr.empty()) return false;

    size_t pos = 0;
    auto peek = [&]() -> char { return pos < expr.size() ? expr[pos] : '\0'; };
    auto get = [&]() -> char { return pos < expr.size() ? expr[pos++] : '\0'; };

    std::function<bool(double&)> parseExpr, parseTerm, parseFactor;

    auto parseNumberOrParam = [&](double& v) -> bool {
        if (peek() == '{') {
            get();
            std::string key;
            while (peek() && peek() != '}') key.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(get()))));
            if (peek() != '}') return false;
            get();
            auto it = params.find(key);
            if (it == params.end()) return false;
            v = it->second;
            return true;
        }
        if (std::isalpha(static_cast<unsigned char>(peek())) || peek() == '_') {
            std::string key;
            while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
                key.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(get()))));
            }
            auto it = params.find(key);
            if (it == params.end()) return false;
            v = it->second;
            return true;
        }
        // Numeric with optional SPICE suffix
        size_t start = pos;
        if (peek() == '+' || peek() == '-') get();
        bool any = false;
        while (std::isdigit(static_cast<unsigned char>(peek())) || peek() == '.') {
            get();
            any = true;
        }
        if (peek() == 'e' || peek() == 'E') {
            get();
            if (peek() == '+' || peek() == '-') get();
            while (std::isdigit(static_cast<unsigned char>(peek()))) get();
        }
        std::string num = expr.substr(start, pos - start);
        // suffix
        size_t sufStart = pos;
        while (std::isalpha(static_cast<unsigned char>(peek()))) get();
        std::string suf = expr.substr(sufStart, pos - sufStart);
        if (!any && num.empty()) return false;
        try {
            size_t idx = 0;
            v = std::stod(num, &idx);
        } catch (...) {
            return false;
        }
        std::string s = suf;
        for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        double scale = 1.0;
        if (s.rfind("meg", 0) == 0) scale = 1e6;
        else if (!s.empty()) {
            switch (s[0]) {
                case 't': scale = 1e12; break;
                case 'g': scale = 1e9; break;
                case 'k': scale = 1e3; break;
                case 'm': scale = 1e-3; break;
                case 'u': scale = 1e-6; break;
                case 'n': scale = 1e-9; break;
                case 'p': scale = 1e-12; break;
                case 'f': scale = 1e-15; break;
                default: break;
            }
        }
        v *= scale;
        return true;
    };

    parseFactor = [&](double& v) -> bool {
        if (peek() == '+') { get(); return parseFactor(v); }
        if (peek() == '-') {
            get();
            double t = 0;
            if (!parseFactor(t)) return false;
            v = -t;
            return true;
        }
        if (peek() == '(') {
            get();
            if (!parseExpr(v)) return false;
            if (peek() != ')') return false;
            get();
            return true;
        }
        return parseNumberOrParam(v);
    };

    parseTerm = [&](double& v) -> bool {
        if (!parseFactor(v)) return false;
        while (peek() == '*' || peek() == '/') {
            char op = get();
            double rhs = 0;
            if (!parseFactor(rhs)) return false;
            if (op == '*') v *= rhs;
            else {
                if (std::abs(rhs) < 1e-30) return false;
                v /= rhs;
            }
        }
        return true;
    };

    parseExpr = [&](double& v) -> bool {
        if (!parseTerm(v)) return false;
        while (peek() == '+' || peek() == '-') {
            char op = get();
            double rhs = 0;
            if (!parseTerm(rhs)) return false;
            if (op == '+') v += rhs;
            else v -= rhs;
        }
        return true;
    };

    double v = 0;
    if (!parseExpr(v)) return false;
    if (pos != expr.size()) return false;
    out = v;
    return true;
}

}
