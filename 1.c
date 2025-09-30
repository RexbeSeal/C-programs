```c
#include <stdio.h>  // Подключаю стандартную библиотеку ввода-вывода для работы с файлами и консолью
#include <stdlib.h> // Нужна для функций malloc и free при работе с динамической памятью
#include <stdint.h> // Подключаю чтобы использовать типы с фиксированной размерностью, как uint16_t
#include <string.h> // Нужна для работы с строками, хотя в этой программе используется мало

#pragma pack(push, 1) // Сохраняю текущее выравнивание и устанавливаю 1 байт, чтобы структуры точно соответствовали формату BMP
// Определяю структуру заголовка BMP-файла (14 байт)
typedef struct {
    uint16_t bfType;        // "BM" (0x4D42) - идентификатор BMP файла
    uint32_t bfSize;        // Общий размер файла в байтах
    uint16_t bfReserved1;   // Зарезервировано, должно быть 0
    uint16_t bfReserved2;   // Зарезервировано, должно быть 0
    uint32_t bfOffBits;     // Смещение до данных изображения
} BITMAPFILEHEADER;

// Определяю структуру информационного заголовка BMP (40 байт)
typedef struct {
    uint32_t biSize;            // Размер этого заголовка (40 байт)
    int32_t biWidth;            // Ширина изображения в пикселях
    int32_t biHeight;           // Высота изображения в пикселях (может быть отрицательной)
    uint16_t biPlanes;          // Должно быть 1
    uint16_t biBitCount;        // Бит на пиксель (24 для RGB)
    uint32_t biCompression;     // Тип сжатия (0 = без сжатия)
    uint32_t biSizeImage;       // Размер данных изображения
    int32_t biXPelsPerMeter;    // Горизонтальное разрешение
    int32_t biYPelsPerMeter;    // Вертикальное разрешение
    uint32_t biClrUsed;         // Количество используемых цветов
    uint32_t biClrImportant;    // Количество важных цветов
} BITMAPINFOHEADER;
#pragma pack(pop) // Возвращаю предыдущее выравнивание структур

// Эта функция рисует линию между двумя точками с использованием алгоритма Брезенхэма
// Я реализовал её, чтобы инвертировать цвет пикселей на пути линии
void draw_line(uint8_t* data, int width, int height, int row_size, 
               int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1); // Вычисляю разницу по X
    int dy = abs(y2 - y1); // Вычисляю разницу по Y
    int sx = (x1 < x2) ? 1 : -1; // Определяю направление движения по X
    int sy = (y1 < y2) ? 1 : -1; // Определяю направление движения по Y
    int err = dx - dy; // Ошибка для алгоритма Брезенхэма
    
    while (1) {
        // Проверяю, что координаты в пределах изображения
        if (x1 >= 0 && x1 < width && y1 >= 0 && y1 < height) {
            int offset = y1 * row_size + x1 * 3; // Вычисляю смещение в буфере данных
            
            // Инвертирую цвет пикселя: если белый (255,255,255) -> черный, иначе -> белый
            if (data[offset] == 255 && data[offset + 1] == 255 && data[offset + 2] == 255) {
                data[offset] = 0;
                data[offset + 1] = 0; 
                data[offset + 2] = 0;
            } else {
                data[offset] = 255;
                data[offset + 1] = 255;
                data[offset + 2] = 255;
            }
        }
        // Если достигли конечной точки, выходим из цикла
        if (x1 == x2 && y1 == y2) break;
        
        int e2 = 2 * err; // Удваиваю ошибку для сравнения
        // Корректируем ошибку и двигаемся по X
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        // Корректируем ошибку и двигаемся по Y
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

int main(int argc, char* argv[]) {
    // Проверяю, что передан ровно один аргумент (путь к файлу)
    if (argc != 2) {
        printf("Ошибка: требуется указать путь к BMP файлу.\n");
        printf("Использование: %s <путь_к_файлу.bmp>\n", argv[0]);
        return 1;
    }
    
    // Открываю файл в режиме чтения/записи бинарных данных
    FILE* file = fopen(argv[1], "rb+");
    if (!file) {
        perror("Ошибка открытия файла");
        return 1;
    }
    
    // Читаю заголовок BMP-файла
    BITMAPFILEHEADER file_header;
    if (fread(&file_header, sizeof(BITMAPFILEHEADER), 1, file) != 1) {
        printf("Ошибка чтения заголовка файла\n");
        fclose(file);
        return 1;
    }
    
    // Проверяю, что это действительно BMP файл (сигнатура "BM")
    if (file_header.bfType != 0x4D42) {
        printf("Ошибка: файл не является BMP изображением\n");
        fclose(file);
        return 1;
    }
    
    // Читаю информационный заголовок
    BITMAPINFOHEADER info_header;
    if (fread(&info_header, sizeof(BITMAPINFOHEADER), 1, file) != 1) {
        printf("Ошибка чтения информационного заголовка\n");
        fclose(file);
        return 1;
    }
    
    // Проверяю поддерживаемый формат: только 24-битные BMP без сжатия
    if (info_header.biBitCount != 24 || info_header.biCompression != 0) {
        printf("Ошибка: поддерживается только 24-битный BMP без сжатия\n");
        fclose(file);
        return 1;
    }
    
    // Вычисляю padding (выравнивание строк до кратности 4 байт)
    int padding = (4 - (info_header.biWidth * 3) % 4) % 4;
    // Вычисляю реальный размер строки в файле (с учетом padding)
    int row_size = info_header.biWidth * 3 + padding; 
    // Вычисляю общий размер данных изображения
    int image_size = row_size * abs(info_header.biHeight); 
    
    // Проверяю, соответствует ли указанный размер данных действительности
    if (info_header.biSizeImage != 0 && info_header.biSizeImage != image_size) {
        printf("Ошибка: неожиданный размер данных изображения\n");
        fclose(file);
        return 1;
    }
    
    // Перемещаюсь к началу данных изображения
    if (fseek(file, file_header.bfOffBits, SEEK_SET) != 0) {
        printf("Ошибка позиционирования в файле\n");
        fclose(file);
        return 1;
    }
    
    // Выделяю память под данные изображения
    uint8_t* data = (uint8_t*)malloc(image_size);
    if (!data) {
        printf("Ошибка выделения памяти\n");
        fclose(file);
        return 1;
    }
    
    // Читаю данные изображения в буфер
    if (fread(data, 1, image_size, file) != (size_t)image_size) {
        printf("Ошибка чтения данных изображения\n");
        free(data);
        fclose(file);
        return 1;
    }
    
    // Вывожу черно-белое текстовое представление изображения в консоль
    printf("\nЧёрно-белое представление изображения:\n");
    // Обхожу изображение снизу вверх, так как BMP хранит строки снизу вверх
    for (int y = abs(info_header.biHeight) - 1; y >= 0; y--) {
        for (int x = 0; x < info_header.biWidth; x++) {
            int offset = y * row_size + x * 3;
            uint8_t b = data[offset];      // Синий канал
            uint8_t g = data[offset + 1];  // Зеленый канал
            uint8_t r = data[offset + 2];  // Красный канал
            // Средняя яркость для преобразования в черно-белое
            int brightness = (r + g + b) / 3;
            // Вывожу '.' для ярких пикселей и '#' для темных
            putchar(brightness > 127 ? '.' : '#');
        }
        putchar('\n'); // Переход на новую строку после каждой строки изображения
    }
    printf("\n"); // Дополнительный отступ после вывода
    
    // Рисую диагональ от нижнего левого угла к верхнему правому
    draw_line(data, info_header.biWidth, abs(info_header.biHeight), 
              row_size, 0, abs(info_header.biHeight) - 1, 
              info_header.biWidth - 1, 0);
    
    // Рисую диагональ от верхнего левого угла к нижнему правому
    draw_line(data, info_header.biWidth, abs(info_header.biHeight), 
              row_size, 0, 0, 
              info_header.biWidth - 1, abs(info_header.biHeight) - 1);
    
    // Перемещаюсь обратно к началу данных изображения для записи
    if (fseek(file, file_header.bfOffBits, SEEK_SET) != 0) {
        printf("Ошибка при записи данных\n");
        free(data);
        fclose(file);
        return 1;
    }
    
    // Записываю измененные данные обратно в файл
    if (fwrite(data, 1, image_size, file) != (size_t)image_size) {
        printf("Ошибка записи данных в файл\n");
        free(data);
        fclose(file);
        return 1;
    }
    
    // Освобождаю память и закрываю файл
    free(data);
    fclose(file);
    
    // Вывожу сообщение об успехе
    printf("Изображение успешно модифицировано и сохранено!\n");
    return 0;
}

```
