#include <img_lib.h>
#include <jpeg_image.h>
#include <ppm_image.h>

#include <filesystem>
#include <string_view>
#include <iostream>

using namespace std::literals;

class ImageFormatInterface {
public:
    virtual bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const = 0;
    virtual img_lib::Image LoadImage(const img_lib::Path& file) const = 0;
};

class PpmImageFormat : public ImageFormatInterface {
public:
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SaveJPEG(file, image);
    }
    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadPPM(file);
    }
};

class JpegImageFormat : public ImageFormatInterface {
public:
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SavePPM(file, image);
    }
    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadJPEG(file);
    }
};

enum class Format {
    PPM,
    JPEG,
    UNKNOWN
};

Format GetFormatByExtension(const img_lib::Path& input_file) {
    const std::string ext = input_file.extension().string();
    if (ext == ".jpg"sv || ext == ".jpeg"sv) {
        return Format::JPEG;
    }
    
    if (ext == ".ppm"sv) {
        return Format::PPM;
    }
    
    return Format::UNKNOWN;
}


ImageFormatInterface* GetFormatInterface(const img_lib::Path& path) {
    if (GetFormatByExtension(path) == Format::JPEG) {
        return new JpegImageFormat();
    }
    if (GetFormatByExtension(path) == Format::PPM) {
        return new PpmImageFormat();
    }
    return nullptr;
}


int main(int argc, const char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: "sv << argv[0] << " <in_file> <out_file>"sv << std::endl;
        return 1;
    }

    img_lib::Path in_path = argv[1];
    img_lib::Path out_path = argv[2];
    
    if (GetFormatByExtension(in_path) == Format::UNKNOWN) {
        std::cerr << "Unknown format of the input file"sv << std::endl;
        return 2;
    }
    if (GetFormatByExtension(out_path) == Format::UNKNOWN) {
        std::cerr << "Unknown format of the output file"sv << std::endl;
        return 3;
    }
    
    ImageFormatInterface* image_format_ptr = GetFormatInterface(in_path);
 
    if (!image_format_ptr) {
        std::cerr << "Loading failed"sv << std::endl;
        return 4;
    }
    
    img_lib::Image image = image_format_ptr->LoadImage(in_path);
    
    if (!image) {
        std::cerr << "Loading failed"sv << std::endl;
        return 4;
    }

    if (!image_format_ptr->SaveImage(out_path, image)) {
        std::cerr << "Saving failed"sv << std::endl;
        return 5;
    }
    
    std::cout << "Successfully converted"sv << std::endl;
}