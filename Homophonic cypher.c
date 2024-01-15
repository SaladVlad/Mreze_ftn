#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* homophone(char* message, int* key);

int main() {
    int key[] = {3302, 5, 4, 10, 5812, 21, 99, 83, 7101, 6, 47, 91, 12, 22, 1416, 31, 56, 42, 8, 77, 6652, 51, 39, 46, 24, 29};

    char message[] = "HELLO";
    printf("Original Message: %s\n", message);

    homophone(message, key);

    printf("Encrypted Message: ");
    for (int i = 0; i < strlen(message); i++) {
        printf("%d ", message[i]);
    }

    return 0;
}
