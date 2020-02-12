
#include <avrio.c>
#include <textmode.c>

int i;

// Необходимо для forward-доступа из expr3
int expr1(char e[], int l);

// Удаление пробелов
void avoid_spaces(char e[], int l) {
    while (i < l && e[i] == ' ') i++;
}

// Принадлежит множеству [A-Za-z_]
int is_alpha(char ch) {

    return (ch >= 'A' && ch <= 'Z') ||
           (ch >= 'a' && ch <= 'z') ||
            ch == '_' ? 1 : 0;
}

// Выражение третьего уровня (числа, символы)
int expr3(char e[], int l) {

    avoid_spaces(e, l);

    int v = 0;

    // -------------------
    // Скобки?
    // -------------------
    if (e[i] == '(') {

        i++; v = expr1(e, l);
        if (e[i] == ')') i++; // else error()
    }
    // -------------------
    // Это переменная?
    // -------------------
    else if (is_alpha(e[i])) {

        // @todo Найти переменную
        while (is_alpha(e[i])) { i++; }
        v = 0;
    }
    // -------------------
    // Получение числа
    // -------------------
    else {

        while (i < l) {

            if (e[i] >= '0' && e[i] <= '9')
                 v = 10*v + (e[i] - '0');
            else break;

            i++;
        }
    }

    avoid_spaces(e, l);
    return v;
}

// Выражения второго уровня (div, mul)
int expr2(char e[], int l) {

    int a, b;

    // Левая часть
    a = expr3(e, l);

    // Поиск по цепочке
    while (i < l) {

        if (e[i] == '*') {
            i++; b = expr3(e, l); a *= b;
        } else if (e[i] == '/') {
            i++; b = expr3(e, l); a /= b;
        } else {
            break;
        }
    }

    return a;
}

// Выражения первого уровня (plus, minus)
int expr1(char e[],  int l) {

    int a, b;

    // Левая часть
    a = expr2(e, l);

    // Поиск по цепочке
    while (i < l) {

        if (e[i] == '+') {
            i++; b = expr2(e, l); a += b;
        } else if (e[i] == '-') {
            i++; b = expr2(e, l); a -= b;
        } else {
            break;
        }
    }

    return a;
}

// Функция верхнего уровня
int expr(char e[]) {

    i = 0;
    int l = 0;
    while (e[l]) l++;
    return expr1(e, l);
}

int main() {

    cls(0x07);

    char ein[] = "2 + (-3)";
    locate(0, 0);
    printint(expr(ein));
}
