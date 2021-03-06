#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <array>

#define STB_IMAGE_IMPLEMENTATION
#include "../dependencies/stb_image.h"

bool load_image(std::filesystem::path & img_path, int & x, int & y, std::vector<unsigned char> & dest) {
    int n;
    unsigned char * data = stbi_load(img_path.string().c_str(), &x, &y, &n, 4);
    bool loaded = (data != nullptr);

    if(loaded) {
        dest = std::vector<unsigned char>(data, data + x * y * 4);
        stbi_image_free(data);
    }

    return loaded;
}

struct Pixel {
    int r, g, b, a;

    float greyscale() {
        return (.299F * (float)(r) + .587F * (float)(g) + .114F * (float)(b)) / 255.0F * ((float)(a) / 255.0F);
    }
};

void bytes_to_pixels(const int & width, const int & height, const int & value, std::vector<unsigned char> & source, std::vector<Pixel> & dest) {
    for(int y = 0; y < height; y++)
        for(int x = 0; x < width; x++) {
            auto index = value * (y * width + x);
            dest.push_back({
                    static_cast<int>(source[index + 0]),
                    static_cast<int>(source[index + 1]),
                    static_cast<int>(source[index + 2]),
                    static_cast<int>(source[index + 3])
            });
        }
}

constexpr size_t SIZE = 24;

struct {
    private:
        float _i = 0.0F;
    public:
        constexpr float operator () (bool reset = false) {
            if(reset)
                _i = 0.0F;
            else
                _i += 1.0F / (float)(SIZE);
            return _i;
        }
} iota;

std::array<std::pair<float, char>, SIZE> Brightness;

std::array<std::pair<float, char>, SIZE> B1 = {
    std::make_pair(iota(), ' '), std::make_pair(iota(), '`'), std::make_pair(iota(), '\''), std::make_pair(iota(), '.'),
    std::make_pair(iota(), ':'), std::make_pair(iota(), ';'), std::make_pair(iota(), '-'), std::make_pair(iota(), '~'),
    std::make_pair(iota(), '"'), std::make_pair(iota(), '<'), std::make_pair(iota(), '+'), std::make_pair(iota(), '*'),
    std::make_pair(iota(), 'u'), std::make_pair(iota(), 'o'), std::make_pair(iota(), 'x'), std::make_pair(iota(), 'a'),
    std::make_pair(iota(), 'N'), std::make_pair(iota(), '&'), std::make_pair(iota(), '8'), std::make_pair(iota(), '$'),
    std::make_pair(iota(), '%'), std::make_pair(iota(), '#'), std::make_pair(iota(), 'W'), std::make_pair(iota(), '@')
};

std::array<std::pair<float, char>, SIZE> B2 = {
    std::make_pair(iota(true) + iota(), ' '), std::make_pair(iota(), '`'), std::make_pair(iota(), '\''), std::make_pair(iota(), '.'),
    std::make_pair(iota(), '-'), std::make_pair(iota(), '~'), std::make_pair(iota(), '"'), std::make_pair(iota(), ':'),
    std::make_pair(iota(), ';'), std::make_pair(iota(), '<'), std::make_pair(iota(), '+'), std::make_pair(iota(), '*'),
    std::make_pair(iota(), 'u'), std::make_pair(iota(), 'o'), std::make_pair(iota(), 'x'), std::make_pair(iota(), 'a'),
    std::make_pair(iota(), 'N'), std::make_pair(iota(), '&'), std::make_pair(iota(), '8'), std::make_pair(iota(), '$'),
    std::make_pair(iota(), '%'), std::make_pair(iota(), '#'), std::make_pair(iota(), 'W'), std::make_pair(iota(), '@')
};

struct Block {
    int pixels = 0;
    float grayscales = .0F;
};

char block_converter(Block & block) {
    float brightness = block.grayscales / (float)(block.pixels);

    auto cmp = [](float br, std::pair<float, char> & el) -> bool {
        return br < el.first;
    };

    auto it = std::upper_bound(Brightness.begin(), Brightness.end(), brightness, cmp);

    if(it == Brightness.end()) {
        return '@';
    }
    return it->second;
}

Block create_block(const int & width, const std::pair<int, int> & block_size, const int & y1, const int & x1, std::vector<Pixel> & img_pixels) {
    Block b;
    for(int y = y1; y < y1 + block_size.first; y++)
        for(int x = x1; x < x1 + block_size.second; x++) {
            auto greyscale = img_pixels.at(y * width + x).greyscale();
            b.grayscales += greyscale;
            ++b.pixels;
        }
    return b;
}

void img_to_ascii(const int & width, const int & height, const std::pair<int, int> & block_size, std::vector<Pixel> & img_pixels, std::vector<std::string> & ascii) {
    ascii.resize(1);
    for(int y = 0; y < height; y += block_size.first) {
        for(int x = 0; x < width; x += block_size.second) {
            try {
                auto b = create_block(width, block_size, y, x, img_pixels);
                *ascii.rbegin() += block_converter(b);
            } catch(...) {}
        }
        ascii.push_back("");
    }
}

void load_args(std::filesystem::path & filename, int & font_pt, std::pair<int, int> & ratio) {
    std::cout << "Path: "; std::cin >> filename;

    std::cout << "Font Size (Points): "; std::cin >> font_pt;

    std::string tr;

    std::cout << "Ratio (1:2 or 3:5): "; std::cin >> tr;

    if(tr == "3:5") ratio = {3, 5};
    else ratio = {1, 2};

    char inv;

    int scheme;

    std::cout << "Character Scheme (Default = 0; 0-1): "; std::cin >> scheme;

    if(scheme == 0)
        Brightness = B1;
    else
        Brightness = B2;

    std::cout << "Inverted (y/n): "; std::cin >> inv;

    if(inv == 'y') {
        iota(true);
        std::reverse(Brightness.begin(), Brightness.end());
        for(auto & br : Brightness)
            br.first = iota();
    }
}

inline int process_images(std::filesystem::path path, int font_height, std::pair<int, int> ratio) {
    int width, height;

    std::vector<unsigned char> img;

    if(!load_image(path, width, height, img)) {
        std::cerr << "Invalid Path\n";
        return 42;
    }

    std::vector<Pixel> img_pixels;

    bytes_to_pixels(width, height, 4, img, img_pixels);

    std::vector<std::string> ascii;

    std::pair<int, int> blck_size;

    blck_size.first = font_height;
    blck_size.second = ratio.first * font_height / ratio.second;

    img_to_ascii(width, height, blck_size, img_pixels, ascii);

    path.replace_extension(".txt");

    std::ofstream out(path);

    for(auto & line : ascii) {
        if(line != "")
            out << line << '\n';
    }

    return 0;
}

int main(int argc, char ** argv) {

    std::filesystem::path path;
    int font_pt;
    std::pair<int, int> ratio;

    load_args(path, font_pt, ratio);

    int font_height = std::ceil(font_pt * 1.333333F);

    if(path.has_extension()) {
        return process_images(path, font_height, ratio);
    } else {
        int errc = 0;
        for(auto f : std::filesystem::directory_iterator(path))
            if(!f.is_directory()) {
                if(f.path().extension() == ".png")
                    errc += process_images(f.path(), font_height, ratio);
            }
        return errc;
    }
}
