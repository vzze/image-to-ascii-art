#include <filesystem>
#include <algorithm>
#include <cstdint>
#include <cstddef>

#include <iostream>
#include <fstream>

#include <string_view>
#include <string>
#include <vector>
#include <array>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct Pixel {
    std::uint8_t r, g, b, a;

    float greyscale() const noexcept {
        return (
            .299F * static_cast<float>(r) +
            .587F * static_cast<float>(g) +
            .114F * static_cast<float>(b)
        ) / 255.0F * (static_cast<float>(a) / 255.0F);
    }
};

struct Image {
    int width, height;
    std::vector<Pixel> pixels;
};

Image load_image(const std::filesystem::path & image_path) {
    int n;
    Image image;

    unsigned char * bytes = stbi_load(image_path.string().c_str(), &image.width, &image.height, &n, 4);

    if(bytes == nullptr)
        return image;

    for(int y = 0; y < image.height; ++y)
        for(int x = 0; x < image.width; ++x) {
            //           v forced stbi_load to generate byte data with 4 channels
            auto index = 4 * (y * image.width + x);

            image.pixels.push_back({
                static_cast<std::uint8_t>(bytes[index + 0]),
                static_cast<std::uint8_t>(bytes[index + 1]),
                static_cast<std::uint8_t>(bytes[index + 2]),
                static_cast<std::uint8_t>(bytes[index + 3])
            });
        }

    stbi_image_free(bytes);

    return image;
}

constexpr std::size_t SIZE = 24;

consteval std::array<float, SIZE> division() {
    std::array<float, SIZE> divs;
    float d = 0.0F;

    for(auto & div : divs) {
        d += 1.0F / static_cast<float>(SIZE);
        div = d;
    }

    return divs;
}

int OFFSET = 0;
std::string_view ASCII = " `'.:;-~\"<+*uoxaN&8$%#W@ `'.-~\":;<+*uoxaN&8$%#W@";

auto BRIGHTNESS = division();

struct Pixel_Block {
    std::size_t pixels = 0;
    float grayscales = 0.0F;
};

char block_converter(const Pixel_Block block) {
    const float brightness = block.grayscales / static_cast<float>(block.pixels);

    const auto cmp = [](float brness, float el) -> bool {
        return brness <= el;
    };

    const auto it = std::upper_bound(BRIGHTNESS.begin(), BRIGHTNESS.end(), brightness, cmp);

    return ASCII[it - BRIGHTNESS.begin() + OFFSET];
}

Pixel_Block create_block(const Image & image, const std::pair<int, int> block_size, const int curr_x, const int curr_y) {
    Pixel_Block block;

    for(int y = curr_y; y < curr_y + block_size.first; ++y)
        for(int x = curr_x; x < curr_x + block_size.second; ++x) {
            try {
                block.grayscales += image.pixels.at(y * image.width + x).greyscale();
                ++block.pixels;
            } catch(...) {}
        }

    return block;
}

std::vector<std::string> image_to_ascii(const Image & image, const std::pair<int, int> block_size) {
    std::vector<std::string> ascii; ascii.resize(1);

    for(int y = 0; y < image.height; y += block_size.first) {
        for(int x = 0; x < image.width; x += block_size.second)
            ascii.rbegin()->append(1, block_converter(create_block(image, block_size, x, y)));
        ascii.push_back("");
    }

    return ascii;
}

struct Arguments {
    std::filesystem::path path;
    std::pair<int, int> block_size;
};

Arguments load_args() {
    Arguments args;

    std::cout << "Path: ";
    std::cin >> args.path;

    int font_height;
    std::cout << "Font Size (Points): ";

    std::cin >> font_height;

    font_height = std::ceil(static_cast<float>(font_height) * 1.333333F);

    std::string tr;
    std::cout << "Ratio (1:2 or 3:5): "; std::cin >> tr;

    std::pair<int, int> ratio;

    if(tr == "3:5") ratio = {3, 5};
    else ratio = {1, 2};

    args.block_size.first = font_height;
    args.block_size.second = ratio.first * font_height / ratio.second;

    char inverted;

    std::cout << "Inverted (y/n): ";
    std::cin >> inverted;

    if(inverted == 'y')
        std::reverse(BRIGHTNESS.begin(), BRIGHTNESS.end());

    int scheme;

    std::cout << "Character Scheme (Default = 0; 0-1): ";
    std::cin >> scheme;

    OFFSET = scheme * 24;

    return args;
}

int process_image(const std::filesystem::path & image_path, const std::pair<int, int> block_size) {
    Image image = load_image(image_path);

    if(image.pixels.empty())
        return 1602;

    auto ascii = image_to_ascii(image, block_size);

    std::filesystem::path ascii_path = image_path;
    ascii_path.replace_extension(".txt");

    std::ofstream out(ascii_path);

    if(!out.is_open())
        return 16;

    for(const auto & line : ascii)
        if(line != "")
            out << line << '\n';

    return 0;
}

int main() {
    const Arguments args = load_args();

    if(args.path.has_extension()) {
        return process_image(args.path, args.block_size);
    }

    int err_code = 0;

    for(const auto file : std::filesystem::directory_iterator(args.path))
        if(!file.is_directory())
            err_code += process_image(file, args.block_size);

    return err_code;
}
