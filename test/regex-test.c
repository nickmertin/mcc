#include <stdio.h>
#include <stdbool.h>
#include <memory.h>
#include "../util/regex.h"

int main() {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while (true) {
        char regex[64], text[256];
        printf("Regex: ");
        fgets(regex, 64, stdin);
        printf("Text: ");
        fgets(text, 256, stdin);
        size_t len = strlen(regex);
        if (len)
            regex[len - 1] = 0;
        len = strlen(text);
        if (len)
            text[len - 1] = 0;
        struct __regex_result_t *result = __regex_match(regex, text);
        if (result) {
            text[result->length] = 0;
            printf("Match: %s\n", text);
            for (size_t i = 0; i < result->se_count; ++i)
                printf("%s\n", result->se[i]);
        }
        else
            printf("Not a match\n");
    }
#pragma clang diagnostic pop
}
