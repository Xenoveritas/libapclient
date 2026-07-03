#include "tokenizer.h"

#include <string>
#include <vector>

enum TokenizerState {
    // eat any whitespace.
    // on non-whitespace, transition to:
    //   " -> double_quote_token
    //   ' -> single_quote_token
    //   anything else -> bare_token
    whitespace,
    bare_token,
    quoted_token,
    quoted_escape
};

bool is_whitespace(unsigned char c) {
    return std::isspace(c);
}

// Tokenize a given input string.
void tokenize(const std::string& str, std::vector<std::string>& tokens) {
    // If there is nothing in the string, abort.
    if (str.empty()) {
        return;
    }
    TokenizerState state = whitespace;
    size_t start_index = 0;
    size_t current_index = 0;
    size_t length = str.size();
    // Partial string, used for strings that have escape sequences
    std::string partial;
    // the quote character that should end the current quoted_token state
    char quote_char = '"';
    for (; current_index < length; current_index++) {
        char c = str[current_index];
        switch (state) {
        case whitespace:
            // Check if this character is whitespace
            if (is_whitespace(c)) {
                // do nothing
            } else {
                // store this index and change state
                start_index = current_index;
                if (c == '"' || c == '\'') {
                    // move past the quote character
                    start_index++;
                    quote_char = c;
                    state = quoted_token;
                } else {
                    state = bare_token;
                }
            }
            break;
        case bare_token:
            // Check if this character is whitespace
            if (is_whitespace(c)) {
                // store the token
                tokens.push_back(str.substr(start_index, current_index - start_index));
                // and move to the whitespace state
                state = whitespace;
            }
            // for anything else, just keep going
            break;
        case quoted_token:
            if (c == '\\') {
                state = quoted_escape;
                // start_index can = current_index for when the string is "\""
                if (current_index > start_index) {
                    // append the bit without the escape character to partial
                    partial.append(str, start_index, current_index - start_index);
                }
            } else if (c == quote_char) {
                if (partial.empty()) {
                    // If there's nothing in partial, can just copy the substring
                    tokens.push_back(str.substr(start_index, current_index - start_index));
                } else {
                    if (current_index > start_index) {
                        // append the next partial part
                        partial.append(str, start_index, current_index - start_index);
                    }
                    // And add the partial
                    tokens.push_back(partial);
                    partial.clear();
                }
                state = whitespace;
            }
            break;
        case quoted_escape:
            // store this index as the start of the current token
            // (TODO: actual escape sequences like \r and \n?)
            start_index = current_index;
            // return to quoted_token.
            state = quoted_token;
            break;
        }
    }
    if (state != whitespace && start_index < length) {
        // In any state other than whitespace, just append whatever's left.
        if (partial.empty()) {
            // If there's nothing in partial, can just copy the substring
            tokens.push_back(str.substr(start_index));
        } else {
            if (current_index - 1 > start_index) {
                // append the next partial part
                partial.append(str, start_index, current_index - start_index - 1);
            }
            // And add the partial
            tokens.push_back(partial);
        }
    }
}