#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <cstddef>
#include <fstream>
#include <string_view>

using namespace std::literals;

namespace img_lib {

//
//      заголовок файла
//   {"БМ" - Подпись}
//   {Суммарный размер файла = offset_of_header + BMPStride * BMPHeight}
//   {Зарезервированное пространство — 4 байта, заполненные нулями.}
//   {Отступ данных от начала файла = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader)}
//       конец заголовка файла
//       заголовок инфо файла
//   {Размер заголовка — 4 байта, беззнаковое целое. Учитывается только размер второй части заголовка.}
//   {Ширина изображения в пикселях — 4 байта, знаковое целое.}
//   {Высота изображения в пикселях — 4 байта, знаковое целое.}
//   {Количество плоскостей — 2 байта, беззнаковое целое. В нашем случае всегда 1 — одна RGB плоскость.}
//   {Количество бит на пиксель — 2 байта, беззнаковое целое. В нашем случае всегда 24.}
//   {Тип сжатия — 4 байта, беззнаковое целое. В нашем случае всегда 0 — отсутствие сжатия.}
//   {Количество байт в данных — 4 байта, беззнаковое целое. Произведение отступа на высоту.}
//   {Горизонтальное разрешение, пикселей на метр — 4 байта, знаковое целое. Нужно записать 11811, что примерно соответствует 300 DPI.}
//   {Вертикальное разрешение, пикселей на метр — 4 байта, знаковое целое. Нужно записать 11811, что примерно соответствует 300 DPI.}
//   {Количество использованных цветов — 4 байта, знаковое целое. Нужно записать 0 — значение не определено.}
//   {Количество значимых цветов — 4 байта, знаковое целое. Нужно записать 0x1000000.}
//       конец заголовка инфо файла
//   дддддддддддддддддддддддддддддддддддддддддддддддддддддддддддддддд000
//   дддддддддддддддддддддддддддддддддддддддддддддддддддддддддддддддд000
//   дддддддддддддддддддддддддддддддддддддддддддддддддддддддддддддддд000
//   дддддддддддддддддддддддддддддддддддддддддддддддддддддддддддддддд000
//   дддддддддддддддддддддддддддддддддддддддддддддддддддддддддддддддд000
//   дддддддддддддддддддддддддддддддддддддддддддддддддддддддддддддддд000
//                              данные                             padding




PACKED_STRUCT_BEGIN BitmapFileHeader { // поля заголовка Bitmap File Header
    BitmapFileHeader() = default;
    explicit BitmapFileHeader(const uint32_t data_size) : data_size(data_size) {}
    
    const char signature[2] = {'B','M'};
    const uint32_t data_size = 54;
    const uint32_t reserved = 0;
    const uint32_t offset_of_header = 54;
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader { // поля заголовка Bitmap Info Header
    BitmapInfoHeader() = default;
    explicit BitmapInfoHeader(const uint32_t width, const uint32_t height, const uint32_t data_size) : width(width), height(height), data_size(data_size) {}
    const uint32_t info_header_size = 40;
    const uint32_t width = 0;
    const uint32_t height = 0;
    const uint16_t planes = 1;
    const uint16_t bits_per_pixel = 24;
    const uint32_t compression = 0;
    const uint32_t data_size = 0;
    const uint32_t horizontal_resolution = 11811;
    const uint32_t vertical_resolution = 11811;
    const uint32_t number_of_color_used = 0;
    const uint32_t number_of_color_important = 0x1000000; }
PACKED_STRUCT_END

// функция вычисления отступа по ширине
static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}

// напишите эту функцию
bool SaveBMP(const Path& file, const Image& image) {
    std::ofstream ofs(file, std::ios::binary);
    if (!ofs.is_open()) { return false; }
    
    uint32_t bmp_stride = GetBMPStride(image.GetWidth());
    uint32_t offset_of_header = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);
    uint32_t data_size = bmp_stride * image.GetHeight();
    uint32_t file_size = offset_of_header + data_size;
    
    BitmapFileHeader file_header {file_size};
    BitmapInfoHeader info_header {static_cast<uint32_t>(image.GetWidth()), static_cast<uint32_t>(image.GetHeight()), data_size};
    ofs.write(reinterpret_cast<char*>(&file_header), sizeof(BitmapFileHeader));
    ofs.write(reinterpret_cast<char*>(&info_header), sizeof(BitmapInfoHeader));
    
    const int w = image.GetWidth();
    const int h = image.GetHeight();
    uint32_t stride_offset = bmp_stride - w * 3;
    
    std::vector<char> buff(w * 3);
    std::vector<char> stride(stride_offset);
    
    for (int y = h - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < w; ++x) {
            buff[x * 3 + 0] = static_cast<char>(line[x].b);
            buff[x * 3 + 1] = static_cast<char>(line[x].g);
            buff[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        ofs.write(buff.data(), w * 3);
        ofs.write(stride.data(), stride_offset);
    }
    
    return ofs.good();
}

// напишите эту функцию
Image LoadBMP(const Path& file) {// открываем поток с флагом ios::binary
    std::ifstream ifs(file, std::ios::binary);
    
    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;
    
    ifs.read(reinterpret_cast<char*>(&file_header), sizeof(BitmapFileHeader));
    ifs.read(reinterpret_cast<char*>(&info_header), sizeof(BitmapInfoHeader));
    
    Image result(info_header.width, info_header.height, Color::Black());
    
    std::vector<char> buff(info_header.width * 3);
    
    for (int y = info_header.height - 1; y >= 0; --y) {
        Color* line = result.GetLine(y);
        ifs.read(buff.data(), info_header.width * 3);
        ifs.ignore(GetBMPStride(info_header.width) - info_header.width * 3);
        
        for (int x = 0; x < info_header.width; ++x) {
            line[x].b = static_cast<std::byte>(buff[x * 3 + 0]);
            line[x].g = static_cast<std::byte>(buff[x * 3 + 1]);
            line[x].r = static_cast<std::byte>(buff[x * 3 + 2]);
        }
    }
    
    return result;
}
}  // namespace img_lib