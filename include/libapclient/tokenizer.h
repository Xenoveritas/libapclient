#ifndef _LIBAPCLIENT_TOKENIZER_H
#define _LIBAPCLIENT_TOKENIZER_H

#include <string>
#include <vector>
#include <locale>

namespace archipelago {

/*! \brief An opaque "quote type" value.
 *
 * 0 if not a quote, any other value to indicate the type of quote. The quote
 * type is solely used to match quotes when tokenizing.
 */
typedef int quote_t;

template<typename CharT> class CharClassifier {
public:
    bool is_whitespace(CharT c) { return c > 0 && c <= 127 && std::isspace(c); }
    bool is_escape(CharT c) { return c == '\\'; }
    /*! \brief Determine if the given character is a quote.
     *
     * This implementation returns 1 if the quote is a "'" and a 2 if the quote
     * is a double quote '"'.
     *
     * \param c the character to check
     * \return the quote classification or 0 if it isn't a quote
     */
    quote_t is_quote(CharT c) {
        if (c == '"') {
            return 2;
        } else if (c == '\'') {
            return 1;
        } else {
            return 0;
        }
    }
};

/*! \brief Tokenize a given input string.
 *
 * \tparam S the string class
 * \tparam CType the classifier to use
 * \param str the string to ready
 * \param tokens the vector to push tokens onto
 */
template<class S = std::string, class CClassifier = CharClassifier<typename S::value_type>> void tokenize(const S& str, std::vector<S>& tokens) {
    enum tokenizer_state {
        // eat any whitespace.
        // on non-whitespace, transition to:
        //   classified as quote -> quoted_token
        //   anything else -> bare_token
        whitespace,
        // in a bare token
        //   whitespace -> store token and transition to whitespace
        bare_token,
        // Inside a quoted string.
        //    escape character -> store everything up to it, transition to quoted_escape
        quoted_token,
        // Always add the character to the current token and then transition
        // back to quoted_token
        // (In the future this might handle things like \n or \000)
        quoted_escape
    };
    enum tokenizer_state state = whitespace;
    // Partial string, used for strings that have escape sequences
    S partial;
    auto classifier = CClassifier();
    size_t token_start = 0;
    size_t current_index = 0;
    size_t length = str.size();
    // Find first whitespace character

    // the quote character that should end the current quoted_token state
    quote_t active_quote = 0;

    for (; current_index < length; current_index++) {
        typename S::value_type c = str[current_index];
        switch (state) {
        case whitespace:
            // Check if this character is whitespace
            if (classifier.is_whitespace(c)) {
                // do nothing
            } else {
                // store this index and change state
                token_start = current_index;
                quote_t q = classifier.is_quote(c);
                if (q != 0) {
                    // move past the quote character
                    token_start++;
                    active_quote = q;
                    state = quoted_token;
                } else {
                    state = bare_token;
                }
            }
            break;
        case bare_token:
            // Check if this character is whitespace
            if (classifier.is_whitespace(c)) {
                // store the token
                tokens.push_back(str.substr(token_start, current_index - token_start));
                // and move to the whitespace state
                state = whitespace;
            }
            // for anything else, just keep going
            break;
        case quoted_token:
            if (c == '\\') {
                state = quoted_escape;
                // token_start can = current_index for when the string is "\""
                if (current_index > token_start) {
                    // append the bit without the escape character to partial
                    partial.append(str, token_start, current_index - token_start);
                }
            } else if (classifier.is_quote(c) == active_quote) {
                if (partial.empty()) {
                    // If there's nothing in partial, can just copy the substring
                    tokens.push_back(str.substr(token_start, current_index - token_start));
                } else {
                    if (current_index > token_start) {
                        // append the next partial part
                        partial.append(str, token_start, current_index - token_start);
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
            token_start = current_index;
            // return to quoted_token.
            state = quoted_token;
            break;
        }
    }
    if (state != whitespace && token_start < length) {
        // In any state other than whitespace, just append whatever's left.
        if (partial.empty()) {
            // If there's nothing in partial, can just copy the substring
            tokens.push_back(str.substr(token_start));
        } else {
            if (current_index - 1 > token_start) {
                // append the next partial part
                partial.append(str, token_start, current_index - token_start - 1);
            }
            // And add the partial
            tokens.push_back(partial);
        }
    }
}

}

#endif // _LIBAPCLIENT_TOKENIZER_H