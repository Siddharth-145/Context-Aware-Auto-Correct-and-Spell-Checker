#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// --- Constants ---
#define ALPHABET_SIZE 26
#define MAX_WORD_SIZE 100
#define MAX_SUGGESTIONS 200
#define MAX_DIST1_CANDIDATES 2000

#define CHAR_TO_INDEX(c) ((int)c - (int)'a')

// --- Data Structures ---

typedef struct TrieNode {
    struct TrieNode* children[ALPHABET_SIZE];
    bool is_end_of_word;
    int frequency;
} TrieNode;

typedef struct {
    char* word;
    int frequency;
} Suggestion;

// --- Trie Functions ---

TrieNode* create_node() {
    TrieNode* new_node = (TrieNode*)calloc(1, sizeof(TrieNode));
    return new_node;
}

void insert(TrieNode* trie_root, const char* word) {
    TrieNode* crawler = trie_root;
    int word_length = strlen(word);

    for (int i = 0; i < word_length; i++) {
        int char_index = CHAR_TO_INDEX(word[i]);
        if (char_index < 0 || char_index >= ALPHABET_SIZE) continue;

        if (crawler->children[char_index] == NULL) {
            crawler->children[char_index] = create_node();
        }
        crawler = crawler->children[char_index];
    }
    crawler->is_end_of_word = true;
    crawler->frequency++;
}

TrieNode* find_prefix_node(TrieNode* trie_root, const char* prefix_text) {
    TrieNode* crawler = trie_root;
    int word_length = strlen(prefix_text);

    for (int i = 0; i < word_length; i++) {
        int char_index = CHAR_TO_INDEX(prefix_text[i]);
        if (char_index < 0 || char_index >= ALPHABET_SIZE) return NULL;
        if (crawler->children[char_index] == NULL) return NULL;
        crawler = crawler->children[char_index];
    }
    return crawler;
}

bool is_word(TrieNode* trie_root, const char* word) {
    TrieNode* found_node = find_prefix_node(trie_root, word);
    return (found_node != NULL && found_node->is_end_of_word);
}

void free_trie(TrieNode* current_node) {
    if (current_node == NULL) return;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        free_trie(current_node->children[i]);
    }
    free(current_node);
}

// --- Sorting Function ---

int compare_suggestions(const void* a, const void* b) {
    Suggestion* s1 = (Suggestion*)a;
    Suggestion* s2 = (Suggestion*)b;
    return s2->frequency - s1->frequency;
}

// --- Autocomplete Functions ---

void dfs_collect(TrieNode* current_node, char* buffer, int len, Suggestion* suggestions, int* suggestion_count) {
    if (current_node->is_end_of_word) {
        if (*suggestion_count < MAX_SUGGESTIONS) {
            buffer[len] = '\0';
            suggestions[*suggestion_count].word = strdup(buffer);
            suggestions[*suggestion_count].frequency = current_node->frequency;
            (*suggestion_count)++;
        }
    }
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (current_node->children[i] != NULL) {
            buffer[len] = (char)('a' + i);
            dfs_collect(current_node->children[i], buffer, len + 1, suggestions, suggestion_count);
        }
    }
}

void get_autocomplete_suggestions(TrieNode* trie_root, const char* prefix_text) {
    TrieNode* prefix_end_node = find_prefix_node(trie_root, prefix_text);
    if (prefix_end_node == NULL) {
        printf("Autocomplete: No suggestions found.\n");
        return;
    }
    Suggestion suggestion_list[MAX_SUGGESTIONS];
    int suggestion_count = 0;
    char word_buffer[MAX_WORD_SIZE];
    strcpy(word_buffer, prefix_text);

    printf("Autocomplete Suggestions (sorted by frequency):\n");
    dfs_collect(prefix_end_node, word_buffer, strlen(prefix_text), suggestion_list, &suggestion_count);

    if (suggestion_count == 0) {
        printf("  (Prefix is valid, but no complete words match.)\n");
        return;
    }
    qsort(suggestion_list, suggestion_count, sizeof(Suggestion), compare_suggestions);
    for (int i = 0; i < suggestion_count; i++) {
        printf("  - %s (freq: %d)\n", suggestion_list[i].word, suggestion_list[i].frequency);
        free(suggestion_list[i].word);
    }
}

// --- Spell-Checker Functions ---

void add_suggestion(Suggestion* suggestions, int* suggestion_count, const char* word, TrieNode* trie_root) {
    if (*suggestion_count >= MAX_SUGGESTIONS) return;
    for (int i = 0; i < *suggestion_count; i++) {
        if (strcmp(suggestions[i].word, word) == 0) {
            return;
        }
    }
    TrieNode* word_node = find_prefix_node(trie_root, word);
    if (word_node == NULL || !word_node->is_end_of_word) return;

    suggestions[*suggestion_count].word = strdup(word);
    suggestions[*suggestion_count].frequency = word_node->frequency;
    (*suggestion_count)++;
}

// Generates all d1 candidate strings from 'word' (does not check Trie).
void generate_candidates(const char* word, char* candidate_list[], int* candidate_count) {
    int word_length = strlen(word);
    if (word_length >= MAX_WORD_SIZE - 2) return;
    char temp_candidate[MAX_WORD_SIZE + 1];

    // Deletions
    for (int i = 0; i < word_length; i++) {
        strncpy(temp_candidate, word, i);
        strcpy(temp_candidate + i, word + i + 1);
        if (*candidate_count < MAX_DIST1_CANDIDATES) {
            candidate_list[(*candidate_count)++] = strdup(temp_candidate);
        }
    }
    // Transpositions
    for (int i = 0; i < word_length - 1; i++) {
        strcpy(temp_candidate, word);
        char temp_char = temp_candidate[i];
        temp_candidate[i] = temp_candidate[i+1];
        temp_candidate[i+1] = temp_char;
        if (*candidate_count < MAX_DIST1_CANDIDATES) {
            candidate_list[(*candidate_count)++] = strdup(temp_candidate);
        }
    }
    // Substitutions
    for (int i = 0; i < word_length; i++) {
        strcpy(temp_candidate, word);
        char original_char = temp_candidate[i];
        for (char c = 'a'; c <= 'z'; c++) {
            if (c == original_char) continue;
            temp_candidate[i] = c;
            if (*candidate_count < MAX_DIST1_CANDIDATES) {
                candidate_list[(*candidate_count)++] = strdup(temp_candidate);
            }
        }
    }
    // Insertions
    for (int i = 0; i <= word_length; i++) {
        for (char c = 'a'; c <= 'z'; c++) {
            strncpy(temp_candidate, word, i);
            temp_candidate[i] = c;
            strcpy(temp_candidate + i + 1, word + i);
            if (*candidate_count < MAX_DIST1_CANDIDATES) {
                candidate_list[(*candidate_count)++] = strdup(temp_candidate);
            }
        }
    }
}

// Generates d1 candidates from 'word', CHECKS them, and adds valid ones.
void generate_edits_from_candidate(const char* word, TrieNode* trie_root, Suggestion* suggestions, int* suggestion_count) {
    int word_length = strlen(word);
    if (word_length >= MAX_WORD_SIZE - 2) return;
    char temp_candidate[MAX_WORD_SIZE + 1];

    // Deletions
    for (int i = 0; i < word_length; i++) {
        strncpy(temp_candidate, word, i);
        strcpy(temp_candidate + i, word + i + 1);
        if (is_word(trie_root, temp_candidate)) {
            add_suggestion(suggestions, suggestion_count, temp_candidate, trie_root);
        }
    }
    // Transpositions
    for (int i = 0; i < word_length - 1; i++) {
        strcpy(temp_candidate, word);
        char temp_char = temp_candidate[i];
        temp_candidate[i] = temp_candidate[i+1];
        temp_candidate[i+1] = temp_char;
        if (is_word(trie_root, temp_candidate)) {
            add_suggestion(suggestions, suggestion_count, temp_candidate, trie_root);
        }
    }
    // Substitutions
    for (int i = 0; i < word_length; i++) {
        strcpy(temp_candidate, word);
        char original_char = temp_candidate[i];
        for (char c = 'a'; c <= 'z'; c++) {
            if (c == original_char) continue;
            temp_candidate[i] = c;
            if (is_word(trie_root, temp_candidate)) {
                add_suggestion(suggestions, suggestion_count, temp_candidate, trie_root);
            }
        }
    }
    // Insertions
    for (int i = 0; i <= word_length; i++) {
        strncpy(temp_candidate, word, i);
        temp_candidate[i] = ' '; // placeholder
        strcpy(temp_candidate + i + 1, word + i);
        for (char c = 'a'; c <= 'z'; c++) {
            temp_candidate[i] = c;
            if (is_word(trie_root, temp_candidate)) {
                add_suggestion(suggestions, suggestion_count, temp_candidate, trie_root);
            }
        }
    }
}

// Main spell-check function (finds d1 and d2).
void get_spell_check_suggestions(TrieNode* trie_root, const char* misspelled_word) {
    Suggestion suggestion_list[MAX_SUGGESTIONS];
    int suggestion_count = 0;

    char* dist1_candidate_list[MAX_DIST1_CANDIDATES];
    int dist1_candidate_count = 0;

    printf("Spell-Check Suggestions:\n");

    generate_candidates(misspelled_word, dist1_candidate_list, &dist1_candidate_count);

    for (int i = 0; i < dist1_candidate_count; i++) {
        char* current_candidate = dist1_candidate_list[i];

        if (is_word(trie_root, current_candidate)) {
            add_suggestion(suggestion_list, &suggestion_count, current_candidate, trie_root);
        }

        generate_edits_from_candidate(current_candidate, trie_root, suggestion_list, &suggestion_count);

        free(current_candidate);
    }

    if (suggestion_count == 0) {
        printf("  No spell-check suggestions found.\n");
    } else {
        qsort(suggestion_list, suggestion_count, sizeof(Suggestion), compare_suggestions);
        for (int i = 0; i < suggestion_count; i++) {
            printf("  - %s (freq: %d)\n", suggestion_list[i].word, suggestion_list[i].frequency);
            free(suggestion_list[i].word);
        }
    }
}


// --- File Parsing ---

bool parse_and_insert(const char* filename, TrieNode* trie_root) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open file '%s'.\n", filename);
        return false;
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* file_content = (char*)malloc(file_size + 1);
    fread(file_content, 1, file_size, file);
    file_content[file_size] = '\0';
    fclose(file);

    char parsing_buffer[MAX_WORD_SIZE];
    int buffer_pos = 0;
    int total_words_found = 0;

    for (long i = 0; i <= file_size; i++) {
        char current_char = file_content[i];
        if (isalpha(current_char)) {
            if (buffer_pos < MAX_WORD_SIZE - 1) {
                parsing_buffer[buffer_pos++] = tolower(current_char);
            }
        } else {
            if (buffer_pos > 0) {
                parsing_buffer[buffer_pos] = '\0';
                insert(trie_root, parsing_buffer);
                total_words_found++;
                buffer_pos = 0;
            }
        }
    }
    printf("Trie built successfully with %d total word.\n", total_words_found);
    printf("\n");
    free(file_content);
    return true;
}

// --- Main Program ---

int main() {
    const char* filename = "Context.txt";
    TrieNode* trie_root = create_node();

    if (!parse_and_insert(filename, trie_root)) {
        free_trie(trie_root);
        return 1;
    }

    char input_buffer[MAX_WORD_SIZE];
    printf("Enter a prefix/word, or '!add <word>' or 'exit' - to quit:\n");

    while (true) {
        printf("> ");
        if (fgets(input_buffer, MAX_WORD_SIZE, stdin) == NULL) break;

        input_buffer[strcspn(input_buffer, "\n")] = 0;
        if (strcmp(input_buffer, "exit") == 0) break;
        if (strlen(input_buffer) == 0) continue;

        if (strncmp(input_buffer, "!add ", 5) == 0) {
            char* new_word_to_add = input_buffer + 5;
            for(int i = 0; new_word_to_add[i]; i++){ new_word_to_add[i] = tolower(new_word_to_add[i]); }

            if (strlen(new_word_to_add) > 0) {
                insert(trie_root, new_word_to_add);
                printf("Added/updated '%s' in the dictionary.\n", new_word_to_add);
            } else {
                printf("Usage: !add <word_to_add>\n");
            }
            printf("\n");
            continue;
        }

        char lowercase_input[MAX_WORD_SIZE];
        strcpy(lowercase_input, input_buffer);
        for(int i = 0; lowercase_input[i]; i++){
            lowercase_input[i] = tolower(lowercase_input[i]);
        }

        get_autocomplete_suggestions(trie_root, lowercase_input);
        printf("---\n");

        if (!is_word(trie_root, lowercase_input)) {
            get_spell_check_suggestions(trie_root, lowercase_input);
        } else {
            TrieNode* correct_word_node = find_prefix_node(trie_root, lowercase_input);
            printf("'%s' is spelled correctly\n", lowercase_input);
        }
        printf("\n");
    }

    printf("Thank You....Tire Memory has been Freed\n");
    free_trie(trie_root);
    return 0;
}

