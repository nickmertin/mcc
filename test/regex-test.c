#include <stdio.h>
#include <stdbool.h>
#include <memory.h>
#include <stdlib.h>
#include "../util/regex.h"

int main() {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while (true) {
        char regex[256], text[256];
        printf("Regex: ");
        fgets(regex, 256, stdin);
        printf("Text: ");
        fgets(text, 256, stdin);
        size_t len = strlen(regex);
        if (len)
            regex[len - 1] = 0;
        len = strlen(text);
        if (len)
            text[len - 1] = 0;
        char *replaced = __regex_replace(regex, text, "REGEX");
        struct __regex_find_result_t *result = __regex_find(regex, text);
        if (result) {
            text[(result->string - text) + result->info.length] = 0;
            printf("Match: %s\n", result->string);
            for (size_t i = 0; i < result->info.se_count; ++i)
                printf("%s\n", result->info.se[i]);
        }
        else
            printf("Not a match\n");
        printf("%s\n", replaced);
        free(replaced);
    }
#pragma clang diagnostic pop
}
