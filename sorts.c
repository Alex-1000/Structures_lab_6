#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <stdio.h>
#define ftruncate _chsize
#define fileno _fileno
#else
#define _XOPEN_SOURCE_EXTENDED 1
#include <unistd.h>
#endif

#define DEFAULT_ALLOC_SIZE 4

// Сортировка естественным слиянием
void extMergeSort(char* array_file, char* tmp_file_A, char* tmp_file_B) {
    size_t sequence_count; // Количество отсортированных последовательностей
    sequence_count = 2; // Бессмысленное значение для запуска цикла

    // Сортировка будет происходить до тех пор, пока массив не будет
    // отсортирован, то есть не останется одной отсортированной
    // последовательности
    size_t iteration = 1;
    for (;; iteration++) {
        sequence_count = 1;
        size_t numbers_count = 1; // Количество чисел

        // Количество чисел, под которое выделена память
        size_t numbers_alloc = DEFAULT_ALLOC_SIZE;
        // Количество длин последовательностей, под которое выделена память
        size_t sequence_len_alloc = DEFAULT_ALLOC_SIZE;

        // Отсортированные последовательности чисел
        double* sequences = (double*)malloc(sizeof(double) * numbers_alloc);
        // Длины последовательностей
        size_t* sequence_lengths =
            (size_t*)calloc(sequence_len_alloc, sizeof(size_t));

        double num; // Считанное число
        size_t* num_size =
            (size_t*)malloc(sizeof(size_t)); // Длина считанной строки
        char* num_str = malloc(0);           // Считанная строка

        // Открываем исходный файл
        // Название файла с предыдущей последовательностью
        char* prev_array_file;
        if (iteration == 1) {
            size_t len = strlen(array_file) + 1;
            prev_array_file = (char*)malloc(sizeof(char) * len);
            memcpy(prev_array_file, array_file, len);
        } else {
            prev_array_file = (char*)malloc(
                sizeof(char) * (strlen(array_file) + 2 + (iteration - 1) % 10));
            sprintf(prev_array_file, "%zu_%s", iteration - 1, array_file);
        }
        FILE* file = fopen(prev_array_file, "r"); // Поток исходного файла
        if (file == NULL) {
            printf("Невозможно открыть исходный файл\n");
            exit(1);
        }

        // Задаём первый элемент первой последовательности
        if (!getdelim(&num_str, num_size, ' ', file)) {
            printf("Возникла ошибка при чтении исходного файла\n");
            exit(2);
        }
        num = atof(num_str);
        sequences[0] = num;
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
                sequences =
                    (double*)realloc(sequences, sizeof(double) * numbers_alloc);
            }
            // Если число >= предыдущего, оно входит в последовательность,
            if (num >= sequences[i++]) {
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
            sequences[i] = num;

            // Выход из цикла при окончании файла
            if (feof(file))
                break;
        }

        // Закрываем файл
        fclose(file);
        free(prev_array_file);

        // Если есть всего одна последовательность, то массив уже отсортирован
        if (sequence_count <= 1)
            break;

        // Распределяем последовательности
        // Имя первого файла
        char* tmp_file_A_name = (char*)malloc(
            sizeof(char) * (strlen(tmp_file_A) + 2 + iteration % 10));
        sprintf(tmp_file_A_name, "%zu_%s", iteration, tmp_file_A);
        // Имя второго файла
        char* tmp_file_B_name = (char*)malloc(
            sizeof(char) * (strlen(tmp_file_B) + 2 + iteration % 10));
        sprintf(tmp_file_B_name, "%zu_%s", iteration, tmp_file_B);

        FILE* tmp_A = fopen(tmp_file_A_name, "w"); // Поток первого файла
        FILE* tmp_B = fopen(tmp_file_B_name, "w"); // Поток второго файла
        if (tmp_A == NULL || tmp_B == NULL) {
            printf("Возникла ошибка при открытии вспомогательного файла\n");
            exit(10);
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
                *current_len += fprintf(current, "%f ", sequences[i + seq_i]);
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
        fclose(tmp_A);
        fclose(tmp_B);

        // Освобождаем память, выделенную под числа
        free(sequences);
        free(sequence_lengths);

        // Сливаем последовательности
        // Название файла с промежуточной последовательностью
        char* new_array_file = (char*)malloc(
            sizeof(char) * (strlen(array_file) + 2 + iteration % 10));
        sprintf(new_array_file, "%zu_%s", iteration, array_file);
        file = fopen(new_array_file, "w");
        tmp_A = fopen(tmp_file_A_name, "r");
        tmp_B = fopen(tmp_file_B_name, "r");
        current = tmp_A;
        if (file == NULL) {
            printf("Невозможно открыть исходный файл\n");
            exit(1);
        }
        if (tmp_A == NULL || tmp_B == NULL) {
            printf("Возникла ошибка при открытии вспомогательного файла\n");
            exit(10);
        }

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
            if (!eof_A && num_A <= num_B) {
                fprintf(file, "%f ", num_A);
                if (getdelim(&num_str, num_size, ' ', tmp_A)) {
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

                    // Иначе мы продолжаем слияние
                    else
                        num_A = atof(num_str);
                }
            } else if (!eof_B) {
                fprintf(file, "%f ", num_B);
                if (getdelim(&num_str, num_size, ' ', tmp_B)) {
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
                    // Иначе мы продолжаем слияние
                    else
                        num_B = atof(num_str);
                }
            }
            fflush(file);
        }
        // Записываем 2 последних числа (цикл закончился из-за EOF)
        if (num_A <= num_B)
            fprintf(file, "%f %f", num_A, num_B);
        else
            fprintf(file, "%f %f", num_B, num_A);
        // Прекращаем работу с файлами и освобождаем память
        fclose(file);
        fclose(tmp_A);
        fclose(tmp_B);
        free(new_array_file);
        free(tmp_file_A_name);
        free(tmp_file_B_name);
    }

    // Переименование результирующего файла
    if (iteration > 1) {
        char* old_name = (char*)malloc(
            sizeof(char) * (strlen(array_file) + 2 + (iteration - 1) % 10));
        sprintf(old_name, "%zu_%s", iteration - 1, array_file);
        rename(old_name, "result");
    }
}

int main(int argc, char* argv[]) {
    // Имена файлов
    char* file_path = malloc(0); // Имя начального файла
    char* tmp_file_A = malloc(sizeof(char) * 5); // Имя первого временного файла
    char* tmp_file_B = malloc(sizeof(char) * 5); // Имя второго временного файла
    // Стандартные пути для временных файлов
    memcpy(tmp_file_A, "tmp_A", 5);
    memcpy(tmp_file_B, "tmp_B", 5);

    // Работа с аргументами
    if (argc > 1) {
        file_path = argv[1];
        if (argc > 3) {
            tmp_file_A = argv[2];
            tmp_file_B = argv[3];
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
            printf("Укажите путь > ");
            getline(&line, line_size, stdin);
            line_len = strlen(line);
        }
        line[line_len - 1] = '\0';
        strcpy(file_path, line);
        // Ввод имён временных файлов
        printf("Введите имена промежуточных файлов (или нажмите Enter, чтобы "
               "оставить имя по умолчанию)\n");
        printf("Имя первого промежуточного файла:\n> ");
        getline(&line, line_size, stdin);
        line_len = strlen(line);
        if (line_len > 1) {
            line[line_len - 1] = '\0';
            strcpy(tmp_file_A, line);
        }
        printf("Имя второго промежуточного файла:\n> ");
        getline(&line, line_size, stdin);
        line_len = strlen(line);
        if (line_len > 1) {
            line[line_len - 1] = '\0';
            strcpy(tmp_file_B, line);
        }
    }

    extMergeSort(file_path, tmp_file_A, tmp_file_B);

    free(file_path);
    free(tmp_file_A);
    free(tmp_file_B);
}