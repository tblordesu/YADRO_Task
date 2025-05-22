#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Максимальное количество строк, столбцов и длина содержимого ячейки
#define MAX_ROWS 1024
#define MAX_COLS 64
#define MAX_CELL_LEN 128

// Заголовки столбцов
char headers[MAX_COLS][MAX_CELL_LEN];

// Таблица: каждая ячейка содержит строку (либо число, либо формулу)
char *cells[MAX_ROWS][MAX_COLS];

// Номера строк (например, 1, 2, 30)
int row_nums[MAX_ROWS];

// Общее количество строк и столбцов
int n_rows = 0, n_cols = 0;

// Кэш значений после вычисления
int values[MAX_ROWS][MAX_COLS];
// Флаги, чтобы не вычислять ячейки повторно
char computed[MAX_ROWS][MAX_COLS]; // 0 — не вычислено, 1 — уже вычислено

// Удаляет пробелы в начале и в конце строки
char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = 0;
    return s;
}

// Ищем индекс столбца по названию (например, "A")
int get_col_index(const char *name) {
    for (int i = 0; i < n_cols; ++i)
        if (strcmp(headers[i], name) == 0)
            return i;
    return -1;
}

// Ищем индекс строки по номеру (например, 1, 2, 30)
int get_row_index(int row_num) {
    for (int i = 0; i < n_rows; ++i)
        if (row_nums[i] == row_num)
            return i;
    return -1;
}

// Декларация функции (определена ниже)
int eval_cell(int row, int col, int *result);

// Вычисление выражения вида =ARG1 OP ARG2
int eval_expr(char *expr, int *out) {
    char arg1[MAX_CELL_LEN], arg2[MAX_CELL_LEN];
    char op;

    if (expr[0] != '=') return -1;

    // Создаем копию выражения, чтобы не портить оригинал
    char tmp[MAX_CELL_LEN];
    strncpy(tmp, expr + 1, MAX_CELL_LEN - 1);
    tmp[MAX_CELL_LEN - 1] = '\0';

    // Ищем оператор (+, -, * или /)
    char *op_ptr = strpbrk(tmp, "+-*/");
    if (!op_ptr) return -1;

    op = *op_ptr;
    *op_ptr = '\0';

    // Разделяем аргументы
    strncpy(arg1, tmp, MAX_CELL_LEN - 1);
    arg1[MAX_CELL_LEN - 1] = '\0';

    strncpy(arg2, op_ptr + 1, MAX_CELL_LEN - 1);
    arg2[MAX_CELL_LEN - 1] = '\0';

    int val1, val2;

    // Обрабатываем ARG1
    if (isalpha(arg1[0])) {
        char colname[MAX_CELL_LEN];
        int rownum;
        // Разбиваем на имя столбца и номер строки
        if (sscanf(arg1, "%[A-Za-z]%d", colname, &rownum) != 2) return -1;
        int c = get_col_index(colname);
        int r = get_row_index(rownum);
        if (c == -1 || r == -1 || eval_cell(r, c, &val1) != 0) return -1;
    } else {
        val1 = atoi(arg1); // Просто число
    }

    // Обрабатываем ARG2
    if (isalpha(arg2[0])) {
        char colname[MAX_CELL_LEN];
        int rownum;
        if (sscanf(arg2, "%[A-Za-z]%d", colname, &rownum) != 2) return -1;
        int c = get_col_index(colname);
        int r = get_row_index(rownum);
        if (c == -1 || r == -1 || eval_cell(r, c, &val2) != 0) return -1;
    } else {
        val2 = atoi(arg2);
    }

    // Вычисляем значение по оператору
    switch (op) {
        case '+': *out = val1 + val2; break;
        case '-': *out = val1 - val2; break;
        case '*': *out = val1 * val2; break;
        case '/':
            if (val2 == 0) return -1; // Деление на 0 — ошибка
            *out = val1 / val2; break;
        default: return -1;
    }
    return 0;
}

// Вычисляет значение конкретной ячейки
int eval_cell(int row, int col, int *result) {
    if (computed[row][col]) { // если уже считали, просто возвращаем
        *result = values[row][col];
        return 0;
    }

    char *content = cells[row][col];
    if (!content || !*content) return -1;

    if (content[0] == '=') {
        int val;
        if (eval_expr(content, &val) == 0) {
            values[row][col] = val;
            computed[row][col] = 1;
            *result = val;
            return 0;
        } else {
            return -1;
        }
    } else {
        int val = atoi(content); // просто число
        values[row][col] = val;
        computed[row][col] = 1;
        *result = val;
        return 0;
    }
}

// Печатает таблицу в stdout
void print_table() {
    printf(","); // верхняя левая пустая ячейка
    for (int i = 0; i < n_cols; ++i)
        printf("%s%s", headers[i], i == n_cols - 1 ? "\n" : ",");

    for (int i = 0; i < n_rows; ++i) {
        printf("%d", row_nums[i]);
        for (int j = 0; j < n_cols; ++j) {
            int val;
            if (eval_cell(i, j, &val) == 0)
                printf(",%d", val);
            else
                printf(",ERR");
        }
        printf("\n");
    }
}

// Основная функция — точка входа
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file.csv\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    char line[1024];

    // Чтение первой строки — заголовков
    if (fgets(line, sizeof(line), fp)) {
        char *tok = strtok(line, ",\r\n");
        while ((tok = strtok(NULL, ",\r\n"))) {
            strncpy(headers[n_cols++], trim(tok), MAX_CELL_LEN);
        }
    }

    // Чтение остальных строк (данные)
    while (fgets(line, sizeof(line), fp)) {
        int col = 0;
        char *tok = strtok(line, ",\r\n");
        if (!tok) continue;
        row_nums[n_rows] = atoi(tok); // номер строки

        while ((tok = strtok(NULL, ",\r\n")) && col < MAX_COLS) {
            cells[n_rows][col] = strdup(trim(tok)); // сохраняем строку
            col++;
        }
        n_rows++;
    }

    fclose(fp);
    print_table(); // печатаем результат
    return 0;
}
