#include <iostream>
#include <cstring>
#include <qrencode.h>
#include <png.h>

// Функция для сохранения QR-кода в формате PNG
void save_png(const char* filename, QRcode* qr, int size) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        std::cerr << "Error: could not open file for writing: " << filename << std::endl;
        return;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        std::cerr << "Error: could not create PNG write structure" << std::endl;
        fclose(fp);
        return;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        std::cerr << "Error: could not create PNG info structure" << std::endl;
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp);
        return;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        std::cerr << "Error: an error occurred while writing the PNG file" << std::endl;
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return;
    }

    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, size, size,
                 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_byte** row_pointers = new png_byte*[size];
    for (int y = 0; y < size; ++y) {
        row_pointers[y] = new png_byte[size * 4];
        memset(row_pointers[y], 255, size * 4); // Устанавливаем белый цвет по умолчанию
    }

    int qr_width = qr->width;
    int scale = size / qr_width;

    // Цвет фона жёлтый (RGBA: 255, 255, 0, 255)
    png_byte yellow[4] = {255, 255, 0, 255};

    // Цвет QR-кода лиловый (RGBA: 128, 0, 128, 255)
    png_byte purple[4] = {128, 0, 128, 255};

    for (int y = 0; y < qr_width; ++y) {
        for (int x = 0; x < qr_width; ++x) {
            if (qr->data[y * qr_width + x] & 1) {
                for (int yy = 0; yy < scale; ++yy) {
                    for (int xx = 0; xx < scale; ++xx) {
                        int pixel_x = x * scale + xx;
                        int pixel_y = y * scale + yy;

                        row_pointers[pixel_y][pixel_x * 4] = purple[0];
                        row_pointers[pixel_y][pixel_x * 4 + 1] = purple[1];
                        row_pointers[pixel_y][pixel_x * 4 + 2] = purple[2];
                        row_pointers[pixel_y][pixel_x * 4 + 3] = purple[3];
                    }
                }
            } else {
                for (int yy = 0; yy < scale; ++yy) {
                    for (int xx = 0; xx < scale; ++xx) {
                        int pixel_x = x * scale + xx;
                        int pixel_y = y * scale + yy;

                        row_pointers[pixel_y][pixel_x * 4] = yellow[0];
                        row_pointers[pixel_y][pixel_x * 4 + 1] = yellow[1];
                        row_pointers[pixel_y][pixel_x * 4 + 2] = yellow[2];
                        row_pointers[pixel_y][pixel_x * 4 + 3] = yellow[3];
                    }
                }
            }
        }
    }

    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    for (int y = 0; y < size; ++y) {
        delete[] row_pointers[y];
    }
    delete[] row_pointers;

    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
}

int main() {
    // URL-адрес, на который вы хотите перенаправить пользователя
    const char* url = "natalyaligay.incoral.com";

    // Создание QR-кода из URL-адреса
    QRcode* qr = QRcode_encodeString(url, 0, QR_ECLEVEL_L, QR_MODE_8, 1);

    // Размер изображения QR-кода (в пикселях)
    int size = 400;

    // Сохранение QR-кода в формате PNG
    save_png("qrcode.png", qr, size);

    // Освобождение памяти, выделенной для QR-кода
    QRcode_free(qr);

    return 0;
}
