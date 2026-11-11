const vscode = require('vscode');

const KEYWORDS = [
    'fn', 'val', 'var', 'struct', 'union', 'contract', 'impl', 'import',
    'pub', 'return', 'if', 'else', 'match', 'while', 'for', 'in', 'mut', 'ref', 'as'
];

const STDLIB_FUNCS = [
    'vec_new_i32', 'vec_push_i32', 'vec_pop_i32', 'vec_len_i32', 'vec_get_i32', 'vec_clear_i32',
    'map_new', 'map_insert', 'map_len', 'map_clear',
    'json_reader_new', 'json_next_token', 'json_find_field', 'json_token_as_i32',
    'fmt_new', 'fmt_write_str', 'fmt_write_i32', 'fmt_finish',
    'http_parse_request', 'http_response_ok', 'http_response_not_found',
    'base64_encode', 'base64_encoded_len',
    'path_join', 'path_has_extension', 'path_file_name',
    'cli_has_flag', 'cli_parse_port_or_default',
    'vec_dot_product', 'vec_cosine_similarity_scaled',
    'match_single_order', 'ecs_update_single_position',
    'instant_now', 'duration_between', 'sleep_ms',
    'heap_alloc', 'heap_free', 'arena_new', 'arena_alloc',
    'slice_is_empty', 'slice_get', 'slice_equals', 'slice_fill_u8',
    'str_starts_with', 'str_ends_with', 'str_find_char', 'str_parse_i32'
];

function activate(context) {
    // 1. Completion Item Provider (Autocomplete)
    const completionProvider = vscode.languages.registerCompletionItemProvider('sonlang', {
        provideCompletionItems(document, position) {
            const items = [];

            // Add language keywords
            for (const kw of KEYWORDS) {
                const item = new vscode.CompletionItem(kw, vscode.CompletionItemKind.Keyword);
                items.push(item);
            }

            // Add standard library functions
            for (const fnName of STDLIB_FUNCS) {
                const item = new vscode.CompletionItem(fnName, vscode.CompletionItemKind.Function);
                item.detail = 'SonLang Standard Library';
                items.push(item);
            }

            return items;
        }
    });

    // 2. Hover Provider
    const hoverProvider = vscode.languages.registerHoverProvider('sonlang', {
        provideHover(document, position) {
            const range = document.getWordRangeAtPosition(position);
            const word = document.getText(range);

            if (STDLIB_FUNCS.includes(word)) {
                return new vscode.Hover(new vscode.MarkdownString(`**SonLang Core API**: \`${word}\`\n\n*Standard library verified zero-cost primitive.*`));
            }

            if (KEYWORDS.includes(word)) {
                return new vscode.Hover(new vscode.MarkdownString(`**SonLang Keyword**: \`${word}\``));
            }

            return null;
        }
    });

    context.subscriptions.push(completionProvider, hoverProvider);
}

function deactivate() {}

module.exports = {
    activate,
    deactivate
};
