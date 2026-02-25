#include "jsmn.h"
#include <string.h>

static jsmntok_t *jsmn_alloc_token(jsmn_parser *parser,
        jsmntok_t *tokens, size_t num_tokens) {
    jsmntok_t *tok;
    if (parser->toknext >= num_tokens) return NULL;
    tok = &tokens[parser->toknext++];
    tok->start = tok->end = -1;
    tok->size = 0;
    return tok;
}

static void jsmn_fill_token(jsmntok_t *token, jsmntype_t type, int start, int end) {
    token->type  = type;
    token->start = start;
    token->end   = end;
    token->size  = 0;
}

static int jsmn_parse_primitive(jsmn_parser *parser, const char *js,
        size_t len, jsmntok_t *tokens, size_t num_tokens) {
    jsmntok_t *token;
    int start = parser->pos;
    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
        switch (js[parser->pos]) {
        case ':': case ',': case ']': case '}': case ' ': case '\t':
        case '\r': case '\n':
            goto found;
        }
        if (js[parser->pos] < 32 || js[parser->pos] >= 127) return JSMN_ERROR_INVAL;
    }
found:
    if (tokens == NULL) { parser->pos--; return 0; }
    token = jsmn_alloc_token(parser, tokens, num_tokens);
    if (token == NULL) { parser->pos = start; return JSMN_ERROR_NOMEM; }
    jsmn_fill_token(token, JSMN_PRIMITIVE, start, parser->pos);
    parser->pos--;
    return 0;
}

static int jsmn_parse_string(jsmn_parser *parser, const char *js,
        size_t len, jsmntok_t *tokens, size_t num_tokens) {
    jsmntok_t *token;
    int start = parser->pos++;
    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
        char c = js[parser->pos];
        if (c == '\"') {
            if (tokens == NULL) return 0;
            token = jsmn_alloc_token(parser, tokens, num_tokens);
            if (token == NULL) { parser->pos = start; return JSMN_ERROR_NOMEM; }
            jsmn_fill_token(token, JSMN_STRING, start+1, parser->pos);
            return 0;
        }
        if (c == '\\' && parser->pos + 1 < len) parser->pos++;
    }
    parser->pos = start;
    return JSMN_ERROR_PART;
}

void jsmn_init(jsmn_parser *parser) {
    parser->pos      = 0;
    parser->toknext  = 0;
    parser->toksuper = -1;
}

int jsmn_parse(jsmn_parser *parser, const char *js, size_t len,
               jsmntok_t *tokens, unsigned int num_tokens) {
    int r;
    int i;
    jsmntok_t *token;
    int count = parser->toknext;

    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
        char c = js[parser->pos];
        switch (c) {
        case '{': case '[':
            count++;
            if (tokens == NULL) break;
            token = jsmn_alloc_token(parser, tokens, num_tokens);
            if (token == NULL) return JSMN_ERROR_NOMEM;
            if (parser->toksuper != -1) tokens[parser->toksuper].size++;
            token->type  = (c == '{') ? JSMN_OBJECT : JSMN_ARRAY;
            token->start = parser->pos;
            parser->toksuper = parser->toknext - 1;
            break;
        case '}': case ']':
            if (tokens == NULL) break;
            for (i = parser->toknext - 1; i >= 0; i--) {
                token = &tokens[i];
                if (token->start != -1 && token->end == -1) {
                    parser->toksuper = -1;
                    token->end = parser->pos + 1;
                    break;
                }
            }
            for (; i > 0; i--) {
                token = &tokens[i];
                if (token->start != -1 && token->end == -1) {
                    parser->toksuper = i;
                    break;
                }
            }
            break;
        case '\"':
            r = jsmn_parse_string(parser, js, len, tokens, num_tokens);
            if (r < 0) return r;
            count++;
            if (parser->toksuper != -1 && tokens != NULL)
                tokens[parser->toksuper].size++;
            break;
        case '\t': case '\r': case '\n': case ' ': break;
        case ':':
            parser->toksuper = parser->toknext - 1;
            break;
        case ',':
            if (tokens != NULL && parser->toksuper != -1 &&
                tokens[parser->toksuper].type != JSMN_ARRAY &&
                tokens[parser->toksuper].type != JSMN_OBJECT) {
                for (i = parser->toknext - 1; i >= 0; i--) {
                    if (tokens[i].type == JSMN_ARRAY || tokens[i].type == JSMN_OBJECT) {
                        if (tokens[i].start != -1 && tokens[i].end == -1) {
                            parser->toksuper = i;
                            break;
                        }
                    }
                }
            }
            break;
        default:
            r = jsmn_parse_primitive(parser, js, len, tokens, num_tokens);
            if (r < 0) return r;
            count++;
            if (parser->toksuper != -1 && tokens != NULL)
                tokens[parser->toksuper].size++;
            break;
        }
    }
    if (tokens != NULL) {
        for (i = parser->toknext - 1; i >= 0; i--) {
            if (tokens[i].start != -1 && tokens[i].end == -1)
                return JSMN_ERROR_PART;
        }
    }
    return count;
}