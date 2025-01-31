#include <stdio.h>
#include <math.h>
// to use it it has to be linked
// gcc main.c -lm

#include <stdbool.h>
// need to use bool type

#include <string.h>
#include <ctype.h>

enum Operators{
    ADDITION = '+',
    SUBSTRACTION = '-',
    MULTIPLICATION = '*',
    DIVISION = '/'
};

char operator;
float x;
float y;

float addition(int x, int y){return x + y;}
float substraction(int x, int y){return x - y;}
float multiplication(int x, int y){return x * y;}
float division(int x, int y){return x / y;}

void calculate(){
    char error_message[] = "Unknown operator!";
    for (char *p = error_message ; *p; ++p) *p = toupper(*p);  

    printf("\n");
    printf("Enter X: ");
    scanf("%f", &x);

    printf("Enter Y: ");
    scanf("%f", &y);

    printf("Enter operator (+-*/): ");
    scanf("%s", &operator);

    float result;
    float (*fn)(int, int);

    switch (operator)
    {
        case ADDITION: fn = addition; break;
        case SUBSTRACTION: fn = substraction; break;
        case MULTIPLICATION: fn = multiplication; break;
        case DIVISION: fn = division; break;
        default:
            printf("%s\n", error_message);
            result = -1;
            break;
    }

    printf("%.2f %c %.2f = %.2f\n", x, operator, y, (fn)(x, y));

    printf("\n");
}

int main(int argc, char const *argv[]){
    bool more = false;
    char message;

    do{
        calculate();
        printf("Proceed (y/n)? ");
        scanf("%s", &message);
        if(message == 'y') more = true;
        else more = false;
    } while(more);

    return 0;
}
