Программа на C, которая вычисляет значения ячеек в CSV-таблице с формулами

CSV-файл содержит:
1) Заголовок
2) Формулы вида "=A1+B2", и так далее

Требуется gcc/mingw для Windows и gcc для Linux и MacOS

Команда сборки gcc -o csveval test_task.c

Запуск ./csveval file.csv