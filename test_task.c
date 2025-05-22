#include <stdio.h>              // Стандартный ввод/вывод
#include <stdlib.h>             // Для функций atoi, atof, malloc
#include <string.h>             // Для работы со строками
#include <ctype.h>              // Для проверки символов

#define MAX_ROWS 100            // Максимальное количество строк в таблице
#define MAX_COLS 26             // Максимальное количество столбцов
#define MAX_CELL_LEN 256        // Максимальная длина содержимого одной ячейки
#define MAX_LINE_LEN 1024

// Хранение заголовков столбцов
char headers[MAX_COLS][MAX_CELL_LEN];
// Сырые строки таблицы
char table[MAX_ROWS][MAX_COLS][MAX_CELL_LEN];
// Числовые значения после вычисления
double values[MAX_ROWS][MAX_COLS];
// Отметка вычисления ячейки
int evaluated[MAX_ROWS][MAX_COLS];
int evaluating[MAX_ROWS][MAX_COLS]; // цикл

int row_indices[MAX_ROWS]; // csv_row_number -> internal index
int row_numbers[MAX_ROWS]; // internal index -> csv_row_number
int row_count = 0, col_count = 0; // Счётчики строк и столбцов

// Получение индекса столбца по имени
int get_col_index(const char *col_name) 
{
    for (int i = 0; i < col_count; ++i) 
    {
        if (strcmp(headers[i], col_name) == 0) 
        {
            return i;
        }
    }
    return -1; // Не найдено
}

int get_row_index(int csv_row_number) {
    for (int i = 0; i < row_count; ++i) {
        if (row_numbers[i] == csv_row_number) return i;
    }
    return -1;
}

// парс адреса ячейки
int parse_cell_address(const char *expr, int *col, int *row) 
{
    char col_name[MAX_CELL_LEN];
    int i = 0;
    while (isalpha(expr[i]) && i < MAX_CELL_LEN - 1) 
    {
        col_name[i] = expr[i];
        i++;
    }
    col_name[i] = '\0';
    *col = get_col_index(col_name);
    if (*col == -1) return -1;
    int csv_row = atoi(expr + i);
    *row = get_row_index(csv_row);
    if (*row == -1) return -1;
    return 0;
}

// Предварительное объявление, чтобы использовать в eval_expr
double eval_cell(int row, int col);

char *trim_whitespace(char *str) {
    while (isspace(*str)) str++;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) *end-- = '\0';
    return str;
}

char *find_first_operator(char *expr) {
    while (*expr) {
        if (strchr("+-*/", *expr)) return expr;
        expr++;
    }
    return NULL;
}

// Вычисление выражения из строки
double eval_expr(char *expr) 
{
    expr = trim_whitespace(expr);
    char *op_pos = find_first_operator(expr);
    // Поиск первого оператора
    if (op_pos) {
        char left[MAX_CELL_LEN], right[MAX_CELL_LEN];
        strncpy(left, expr, op_pos - expr);
        left[op_pos - expr] = '\0';
        strcpy(right, op_pos + 1);

        int col1, row1, col2, row2;
        if (parse_cell_address(trim_whitespace(left), &col1, &row1) != 0 ||
            parse_cell_address(trim_whitespace(right), &col2, &row2) != 0) return 0;

        double val1 = eval_cell(row1, col1);
        double val2 = eval_cell(row2, col2);

        switch (*op_pos) 
        {
            case '+': return val1 + val2;
            case '-': return val1 - val2;
            case '*': return val1 * val2;
            case '/': return val2 != 0 ? val1 / val2 : 0; // Защита от деления на 0
        }
    } else 
    {
        return atof(expr); // число
    }
    return 0; // Ошибка по умолчанию
}

// Вычисление значения конкретной ячейки
double eval_cell(int row, int col) {
    if (evaluated[row][col]) return values[row][col];
    if (evaluating[row][col]) {
        fprintf(stderr, "Cyclic dependency at cell (%d, %s)\n", row_numbers[row], headers[col]);
        exit(1);
    }
    evaluating[row][col] = 1;
    char *content = table[row][col];
    if (content[0] == '=') {
        values[row][col] = eval_expr(content + 1);
    } else {
        values[row][col] = atof(content);
    }
    evaluated[row][col] = 1;
    evaluating[row][col] = 0;
    return values[row][col];
}

// Печать таблицы с вычисленными значениями в CSV
void print_table() 
{
    printf(",");
    for (int i = 0; i < col_count; i++)
    {
        printf("%s%s", headers[i], i < col_count - 1 ? "," : "\n");
    }
    for (int i = 0; i < row_count; i++) 
    {
        printf("%d,", i);
        for (int j = 0; j < col_count; j++) 
        {
            printf("%.6g%s", eval_cell(i, j), j < col_count - 1 ? "," : "\n");
        }
    }
}

int main(int argc, char *argv[]) 
{
    if (argc < 2) 
    {
        fprintf(stderr, "Usage: %s file.csv\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) 
    {
        perror("Failed to open file");
        return 1;
    }

    char line[MAX_LINE_LEN];
    fgets(line, sizeof(line), f); // Считываем строку заголовка

    char *token = strtok(line, ",\n"); // Пропускаем пустую ячейку
    while ((token = strtok(NULL, ",\n")) != NULL && col_count < MAX_COLS) 
    {
        strcpy(headers[col_count++], token); // Сохраняем имена столбцов
    }

    // Чтение строк таблицы
     while (fgets(line, sizeof(line), f)) {
        if (row_count >= MAX_ROWS) break;
        token = strtok(line, ",\n");
        if (!token) continue;
        int csv_row_number = atoi(token);
        row_numbers[row_count] = csv_row_number;
        row_indices[csv_row_number] = row_count;

        int col = 0;
        while ((token = strtok(NULL, ",\n")) != NULL && col < col_count) {
            strcpy(table[row_count][col++], token);
        }
        row_count++;
    }
    fclose(f);

    print_table(); // Печать результата
    return 0;
}
