#include "tasker.hpp"

#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

struct Startup {};
struct Grayscale {};
struct Blur {};
struct Sobel {};
struct Shutdown {};

// ------------------------------------------------------------
// Resources
// ------------------------------------------------------------

struct RawImage {
    int width = 0;
    int height = 0;
    int channels = 0; // loaded as 3 (RGB)
    std::vector<uint8_t> pixels; // RGB
};

struct ProcessedImages {
    std::vector<uint8_t> gray;   // 1 channel
    std::vector<uint8_t> blur;   // 1 channel
    std::vector<uint8_t> sobel;  // 1 channel
    int width = 0;
    int height = 0;
};

struct Settings {
    std::string input_path = "D:/Projects/Hustle/cpp/tasker/examples/me.png";
    std::string gray_path = "output_gray.png";
    std::string blur_path = "output_blur.png";
    std::string sobel_path = "output_sobel.png";
};

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

static void check(bool cond, const char* msg) {
    if (!cond) throw std::runtime_error(msg);
}

static inline uint8_t clamp_u8(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return static_cast<uint8_t>(v);
}

// ------------------------------------------------------------
// Tasks
// ------------------------------------------------------------

static void load_image(tskr::Resource<Settings> settings,
    tskr::Resource<RawImage> img)
{
    int w, h, c;
    stbi_uc* data = stbi_load(settings->input_path.c_str(), &w, &h, &c, 3);
    check(data != nullptr, "Failed to load image");

    img->width = w;
    img->height = h;
    img->channels = 3;
    img->pixels.assign(data, data + w * h * 3);

    stbi_image_free(data);

    std::cout << "Loaded image: " << w << "x" << h << "\n";
}

static void prepare_outputs(tskr::Resource<RawImage> img,
    tskr::Resource<ProcessedImages> proc)
{
    proc->width = img->width;
    proc->height = img->height;

    size_t count = static_cast<size_t>(img->width) * img->height;
    proc->gray.assign(count, 0);
    proc->blur.assign(count, 0);
    proc->sobel.assign(count, 0);

    std::cout << "Prepared output buffers\n";
}

static void grayscale(tskr::Resource<RawImage> img,
    tskr::Resource<ProcessedImages> proc)
{
    check(img->channels == 3, "Expected RGB image");

    const int w = img->width;
    const int h = img->height;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx = (y * w + x) * 3;
            uint8_t r = img->pixels[idx + 0];
            uint8_t g = img->pixels[idx + 1];
            uint8_t b = img->pixels[idx + 2];

            // simple luminance
            uint8_t gray = static_cast<uint8_t>(
                0.299f * r + 0.587f * g + 0.114f * b
                );

            proc->gray[y * w + x] = gray;
        }
    }

    std::cout << "Grayscale done\n";
}

static void blur_3x3(tskr::Resource<ProcessedImages> proc)
{
    const int w = proc->width;
    const int h = proc->height;

    std::vector<uint8_t> out(proc->gray.size(), 0);

    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            int sum = 0;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    sum += proc->gray[(y + ky) * w + (x + kx)];
                }
            }
            out[y * w + x] = static_cast<uint8_t>(sum / 9);
        }
    }

    proc->blur.swap(out);
    std::cout << "Blur done\n";
}

static void sobel_edge(tskr::Resource<ProcessedImages> proc)
{
    const int w = proc->width;
    const int h = proc->height;

    std::vector<uint8_t> out(proc->gray.size(), 0);

    const int gx[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };
    const int gy[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };

    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            int sx = 0, sy = 0;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int v = proc->gray[(y + ky) * w + (x + kx)];
                    sx += gx[ky + 1][kx + 1] * v;
                    sy += gy[ky + 1][kx + 1] * v;
                }
            }
            int mag = static_cast<int>(std::sqrt(float(sx * sx + sy * sy)));
            out[y * w + x] = clamp_u8(mag);
        }
    }

    proc->sobel.swap(out);
    std::cout << "Sobel done\n";
}

static void save_outputs(tskr::Resource<Settings> settings,
    tskr::Resource<ProcessedImages> proc)
{
    const int w = proc->width;
    const int h = proc->height;

    auto write_gray = [&](const std::string& path,
        const std::vector<uint8_t>& data)
        {
            int ok = stbi_write_png(path.c_str(), w, h, 1,
                data.data(), w);
            check(ok != 0, "Failed to write image");
            std::cout << "Saved: " << path << "\n";
        };

    write_gray(settings->gray_path, proc->gray);
    write_gray(settings->blur_path, proc->blur);
    write_gray(settings->sobel_path, proc->sobel);
}

int main()
{
    tskr::Tasker tasker;

    tasker
        .add_schedules<Startup, Grayscale, tskr::Parallel<Blur, Sobel>, Shutdown>(tskr::ExecutionPolicy::Single)
        .add_tasks<Startup>(tskr::TaskFn<prepare_outputs>{}.after(tskr::TaskFn<load_image>{}))
        .add_tasks<Grayscale>(tskr::TaskFn<grayscale>{})
        .add_tasks<Blur>(tskr::TaskFn<blur_3x3>{})
        .add_tasks<Sobel>(tskr::TaskFn<sobel_edge>{})
        .add_tasks<Shutdown>(tskr::TaskFn<save_outputs>{})

        .register_resource(Settings{})
        .register_resource(RawImage{})
        .register_resource(ProcessedImages{})

        .run();
    return 0;
}