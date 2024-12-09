#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define WINDOWS
#include <stdio.h>
#define read _read
#define ftruncate _chsize
#define fileno _fileno
#else
#define _XOPEN_SOURCE_EXTENDED 1
#include <unistd.h>
#endif

#define DEFAULT_ALLOC_SIZE 4
#define BUFFER_SIZE 8192

// Скопировать source в destination, перезаписывая destination при необходимости
void copyFile(char* source, char* destination) {
#ifdef WINDOWS
    CopyFile(source, destination, FALSE)
#else
    FILE* src = fopen(source, "r");
    if (src == NULL) {
        printf("Возникла ошибка при чтении файла %s\n", source);
        exit(1);
    }
    FILE* dst = fopen(destination, "w");
    if (dst == NULL) {
        printf("Возникла ошибка при записи в файл %s\n", destination);
        exit(2);
    }
    int src_fd = fileno(src);
    int dst_fd = fileno(dst);

    char buffer[BUFFER_SIZE];
    ssize_t count = 0;
    while ((count = read(src_fd, buffer, BUFFER_SIZE)) > 0)
        write(dst_fd, buffer, count);

    fclose(src);
    fclose(dst);
    if (count < 0) {
        printf("Возникла ошибка при копировании файла %s в файл %s", source,
               destination);
        exit(10);
    }
#endif
}

// Сортировка естественным слиянием
void extMergeSort(char* input_file, char* result_file) {
    size_t sequence_count; // Количество отсортированных последовательностей
    sequence_count = 2; // Бессмысленное значение для запуска цикла

    // Копируем входные данные в файл, который будет меняться (в конце - файл с
    // результатом)
    copyFile(input_file, result_file);
    // Сортировка будет происходить до тех пор, пока массив не будет
    // отсортирован, то есть не останется одной отсортированной
    // последовательности
    while (sequence_count > 1) {
        sequence_count = 1;
        size_t numbers_count = 1; // Количество чисел

        // Количество чисел, под которое выделена память
        size_t numbers_alloc = DEFAULT_ALLOC_SIZE;
        // Количество длин последовательностей, под которое выделена память
        size_t sequence_len_alloc = DEFAULT_ALLOC_SIZE;

        // Числа массива
        double* numbers = (double*)malloc(sizeof(double) * numbers_alloc);
        // Длины последовательностей
        size_t* sequence_lengths =
            (size_t*)calloc(sequence_len_alloc, sizeof(size_t));

        double num; // Считанное число
        size_t* num_size =
            (size_t*)malloc(sizeof(size_t)); // Длина считанной строки
        char* num_str = malloc(0);           // Считанная строка

        // Открываем исходный файл
        FILE* file = fopen(result_file, "r"); // Поток исходного файла
        if (file == NULL) {
            printf("Невозможно открыть исходный файл\n");
            exit(1);
        }

        // Задаём первый элемент первой последовательности
        if (!getdelim(&num_str, num_size, ' ', file)) {
            printf("Возникла ошибка при чтении исходного файла\n");
            exit(10);
        }
        num = atof(num_str);
        numbers[0] = num;
        sequence_lengths[0] = 1;

        // Читаем до конца файла
        size_t seq = 0;
        size_t i = 0;
        while (getdelim(&num_str, num_size, ' ', file)) {
            num = atof(num_str);
            // Увеличиваем размер массива последовательностей по мере
            // необходимости
            if (++numbers_count >= numbers_alloc) {
                numbers_alloc *= 2;
                numbers =
                    (double*)realloc(numbers, sizeof(double) * numbers_alloc);
            }
            // Если число >= предыдущего, оно входит в последовательность,
            if (num >= numbers[i++]) {
                sequence_lengths[seq]++;
            } else { // иначе оно начинает новую последовательность
                // Увеличиваем размер массива длин последовательностей по мере
                // необходимости
                if (++sequence_count >= sequence_len_alloc) {
                    sequence_len_alloc *= 2;
                    sequence_lengths = (size_t*)realloc(
                        sequence_lengths, sizeof(size_t) * sequence_len_alloc);
                }
                sequence_lengths[++seq] = 1;
            }
            numbers[i] = num;

            // Выход из цикла при окончании файла
            if (feof(file))
                break;
        }
        fclose(file);

        // Если есть всего одна последовательность, то массив уже отсортирован
        if (sequence_count <= 1)
            break;

        // Распределяем последовательности
        FILE* tmp_A = tmpfile(); // Поток первого файла
        FILE* tmp_B = tmpfile(); // Поток второго файла
        if (tmp_A == NULL || tmp_B == NULL) {
            printf("Возникла ошибка при открытии вспомогательного файла\n");
            exit(3);
        }

        // Заполняем промежуточные файлы
        FILE* current = tmp_A; // Текущий файл
        size_t length_A = 0;   // Длина записи в A
        size_t length_B = 0;   // Длина записи в B
        size_t* current_len = &length_A; // Длина записи в текущем файле
        // Позиция, где в массиве начинается последовательность
        size_t seq_i = 0;
        for (seq = 0; seq < sequence_count; seq++) {
            for (i = 0; i < sequence_lengths[seq]; i++)
                *current_len += fprintf(current, "%f ", numbers[i + seq_i]);
            seq_i += sequence_lengths[seq];

            fprintf(current, "| ");
            // Меняем файл для записи
            if (current == tmp_A) {
                current = tmp_B;
                current_len = &length_B;
            } else {
                current = tmp_A;
                current_len = &length_A;
            }
        }
        // Обрезаем "| " в конце файла A
        fflush(tmp_A);
        ftruncate(fileno(tmp_A), length_A - 2);
        // Обрезаем "| " в конце файла B
        fflush(tmp_B);
        ftruncate(fileno(tmp_B), length_B - 2);
        // Заканчиваем запись в файлы
        // (используется fflush вместо fclose, так как файлы будут необходимы
        // позже)
        fflush(tmp_A);
        fflush(tmp_B);

        // Освобождаем память, выделенную под числа
        free(numbers);
        free(sequence_lengths);

        // Сливаем последовательности
        file = fopen(result_file, "w");
        current = tmp_A;
        if (file == NULL) {
            printf("Невозможно открыть исходный файл\n");
            exit(1);
        }

        // Ставим указатели во временных файлах в начало
        fseek(tmp_A, 0, SEEK_SET);
        fseek(tmp_B, 0, SEEK_SET);

        // Считываем первые числа (для сравнения)
        double num_A; // Последнее прочитанное число из файла A
        double num_B; // Последнее прочитанное число из файла B
        getdelim(&num_str, num_size, ' ', tmp_A);
        num_A = atof(num_str);
        getdelim(&num_str, num_size, ' ', tmp_B);
        num_B = atof(num_str);
        // Считываем числа из файла, пока не дойдём до конца файлов, и
        // записываем меньшее из чисел
        int eof_A;
        int eof_B;
        while (!((eof_A = feof(tmp_A)) & (eof_B = feof(tmp_B)))) {
            if (num_A <= num_B) {
                // Записываем число в файл и читаем следующее число
                fprintf(file, "%f ", num_A);
                getdelim(&num_str, num_size, ' ', tmp_A);
                // Если мы дошли до разделителя в файле A, то дальше можно
                // сразу записать отсортированную последовательность из
                // файла B
                if (strcmp(num_str, "| ") == 0) {
                    // Читаем до разделителя "| " (или до EOF) в файле B
                    while (getdelim(&num_str, num_size, ' ', tmp_B) &&
                           !feof(tmp_B)) {
                        // Записываем число
                        fprintf(file, "%f ", num_B);
                        // Если прочитали разделитель, то берём следующее за
                        // ним число и выходим из цикла (последовательность
                        // закончилась)
                        if (strcmp(num_str, "| ") == 0) {
                            getdelim(&num_str, num_size, ' ', tmp_B);
                            num_B = atof(num_str);
                            break;
                        }
                        // Иначе продолжаем читать файл B
                        num_B = atof(num_str);
                    }
                    // Считываем следующее за разделителем число из файла B
                    if (getdelim(&num_str, num_size, ' ', tmp_B))
                        num_B = atof(num_str);
                }
                // Если мы достигли конца файла, считаем числа из файла B меньше
                // числа из файла A (для этого приравниваем это число
                // бесконечности)
                else if (eof_A)
                    num_A = INFINITY;
                // Иначе мы берём следующее число и продолжаем слияние
                else
                    num_A = atof(num_str);

            } else {
                // Записываем число в файл и читаем следующее число
                fprintf(file, "%f ", num_B);
                getdelim(&num_str, num_size, ' ', tmp_B);
                // Если мы дошли до разделителя в файле B, то дальше можно
                // сразу записать отсортированную последовательность из
                // файла A
                if (strcmp(num_str, "| ") == 0) {
                    // Читаем до разделителя "| " (или до EOF) в файле A
                    while (getdelim(&num_str, num_size, ' ', tmp_A) &&
                           !feof(tmp_A)) {
                        // Записываем число
                        fprintf(file, "%f ", num_A);
                        // Если прочитали разделитель, то берём следующее за
                        // ним число и выходим из цикла (последовательность
                        // закончилась)
                        if (strcmp(num_str, "| ") == 0) {
                            getdelim(&num_str, num_size, ' ', tmp_A);
                            num_A = atof(num_str);
                            break;
                        }
                        // Иначе продолжаем читать файл A
                        num_A = atof(num_str);
                    }
                    // Считываем следующее за разделителем число из файла B
                    if (getdelim(&num_str, num_size, ' ', tmp_B))
                        num_B = atof(num_str);
                }
                // Если мы достигли конца файла, считаем числа из файла A меньше
                // числа из файла B (для этого приравниваем это число
                // бесконечности)
                else if (eof_B)
                    num_B = INFINITY;
                // Иначе мы продолжаем слияние
                else
                    num_B = atof(num_str);
            }
            fflush(file);
        }
        // При необходимости записываем 2 последних числа
        if (!isinf(num_A) && !isinf(num_B)) {
            if (num_A <= num_B)
                fprintf(file, "%f %f", num_A, num_B);
            else
                fprintf(file, "%f %f", num_B, num_A);
        } else if (!isinf(num_A))
            fprintf(file, "%f", num_A);
        else if (!isinf(num_A))
            fprintf(file, "%f", num_B);

        // Прекращаем работу с файлами и освобождаем память
        fclose(file);
        fclose(tmp_A);
        fclose(tmp_B);
    }
}

int main(int argc, char* argv[]) {
    // Имена файлов
    char* input_file = malloc(0); // Имя начального файла
    char* result_file = malloc(sizeof(char) * 7); // Имя файла с результатом
    memcpy(result_file, "result", 7);

    // Работа с аргументами
    if (argc > 1) {
        input_file = argv[1];
        if (argc > 2) {
            result_file = argv[2];
        }
    }
    // Работа с клавиатурой
    else {
        size_t* line_size = (size_t*)malloc(sizeof(size_t));
        *line_size = DEFAULT_ALLOC_SIZE;
        char* line = (char*)malloc(sizeof(char) * *line_size);
        size_t line_len;

        // Ввод имени входного файла
        printf("Введите путь к файлу:\n> ");
        getline(&line, line_size, stdin);
        line_len = strlen(line);
        while (line_len <= 1) {
            printf("Имя файла: > ");
            getline(&line, line_size, stdin);
            line_len = strlen(line);
        }
        line[line_len - 1] = '\0';
        strcpy(input_file, line);
        // Ввод имени результирующего файла
        printf("Введите имя файла с результатом (или нажмите Enter, чтобы "
               "оставить имя по умолчанию)\n> ");
        getline(&line, line_size, stdin);
        line_len = strlen(line);
        if (line_len > 1) {
            line[line_len - 1] = '\0';
            strcpy(result_file, line);
        }
    }

    extMergeSort(input_file, result_file);

    free(input_file);
    free(result_file);
}
