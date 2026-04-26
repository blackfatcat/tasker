#include "tasker.hpp"

#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

struct Startup {};
struct Grayscale {};
struct PostProcess {};
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
    // 0 gray;
    // 1 blur;
    // 2 sobel;
    // 3 sharpen;
    // 4 emboss;
    // 5 threshold;
    // 6 invert;
    // 7 gamma_correct;
    // 8 box_blur;
    // 9 median;
    // 10 bilateral;
    // 11 laplacian;
    // 12 unsharp;
    std::vector<std::vector<uint8_t>> inputs;

    int width = 0;
    int height = 0;
};

struct Settings {
    std::string input_path = "D:/Projects/Hustle/cpp/tasker/examples/me.png";
    std::vector<std::string> outputs;
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

static void prepare_outputs(
    tskr::Resource<RawImage> img,
    tskr::Resource<ProcessedImages> proc,
    tskr::Resource<Settings> settings
)
{
    proc->width = img->width;
    proc->height = img->height;

    size_t count = static_cast<size_t>(img->width) * img->height;

    proc->inputs.resize(13);
    for (auto& input : proc->inputs)
        input.assign(count, 0);

    settings->outputs.push_back("output_gray.png");
    settings->outputs.push_back("output_blur.png");
    settings->outputs.push_back("output_sobel.png");
    settings->outputs.push_back("output_sharpen.png");
    settings->outputs.push_back("output_emboss.png");
    settings->outputs.push_back("output_threshold.png");
    settings->outputs.push_back("output_invert.png");
    settings->outputs.push_back("output_gamma_correct.png");
    settings->outputs.push_back("output_box_blur.png");
    settings->outputs.push_back("output_median.png");
    settings->outputs.push_back("output_bilateral.png");
    settings->outputs.push_back("output_laplacian.png");
    settings->outputs.push_back("output_unsharp.png");

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

            proc->inputs[0][y * w + x] = gray;
        }
    }

    std::cout << "Grayscale done\n";
}

static void blur_3x3(tskr::Resource<ProcessedImages> proc)
{
    const int w = proc->width;
    const int h = proc->height;

    std::vector<uint8_t> out(proc->inputs[0].size(), 0);

    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            int sum = 0;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    sum += proc->inputs[0][(y + ky) * w + (x + kx)];
                }
            }
            out[y * w + x] = static_cast<uint8_t>(sum / 9);
        }
    }

    proc->inputs[1].swap(out);
    std::cout << "Blur done\n";
}

static void sobel_edge(tskr::Resource<ProcessedImages> proc)
{
    const int w = proc->width;
    const int h = proc->height;

    std::vector<uint8_t> out(proc->inputs[0].size(), 0);

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
                    int v = proc->inputs[0][(y + ky) * w + (x + kx)];
                    sx += gx[ky + 1][kx + 1] * v;
                    sy += gy[ky + 1][kx + 1] * v;
                }
            }
            int mag = static_cast<int>(std::sqrt(float(sx * sx + sy * sy)));
            out[y * w + x] = clamp_u8(mag);
        }
    }

    proc->inputs[2].swap(out);
    std::cout << "Sobel done\n";
}

static void sharpen(tskr::Resource<ProcessedImages> proc)
{
    int w = proc->width, h = proc->height;
    std::vector<uint8_t> out(proc->inputs[0].size());

    int k[3][3] = {
        { 0, -1,  0},
        {-1,  5, -1},
        { 0, -1,  0}
    };

    for (int y = 1; y < h - 1; ++y)
        for (int x = 1; x < w - 1; ++x) {
            int sum = 0;
            for (int ky = -1; ky <= 1; ++ky)
                for (int kx = -1; kx <= 1; ++kx)
                    sum += proc->inputs[0][(y + ky) * w + (x + kx)] * k[ky + 1][kx + 1];
            out[y * w + x] = clamp_u8(sum);
        }

    proc->inputs[3].swap(out);
    std::cout << "Sharpen done\n";
}

static void emboss(tskr::Resource<ProcessedImages> proc)
{
    int w = proc->width, h = proc->height;
    std::vector<uint8_t> out(proc->inputs[0].size());

    int k[3][3] = {
        {-2, -1, 0},
        {-1,  1, 1},
        { 0,  1, 2}
    };

    for (int y = 1; y < h - 1; ++y)
        for (int x = 1; x < w - 1; ++x) {
            int sum = 0;
            for (int ky = -1; ky <= 1; ++ky)
                for (int kx = -1; kx <= 1; ++kx)
                    sum += proc->inputs[0][(y + ky) * w + (x + kx)] * k[ky + 1][kx + 1];
            out[y * w + x] = clamp_u8(sum + 128);
        }

    proc->inputs[4].swap(out);
    std::cout << "Emboss done\n";
}

static void threshold(tskr::Resource<ProcessedImages> proc)
{
    for (size_t i = 0; i < proc->inputs[0].size(); ++i)
        proc->inputs[5][i] = (proc->inputs[0][i] > 128) ? 255 : 0;
    std::cout << "Threshold done\n";
}

static void invert(tskr::Resource<ProcessedImages> proc)
{
    for (size_t i = 0; i < proc->inputs[0].size(); ++i)
        proc->inputs[6][i] = 255 - proc->inputs[0][i];
    std::cout << "Invert done\n";
}

static void gamma_correct(tskr::Resource<ProcessedImages> proc)
{
    const float gamma = 2.2f;

    for (size_t i = 0; i < proc->inputs[0].size(); ++i)
    {
        float norm = proc->inputs[0][i] / 255.0f;
        float corrected = std::pow(norm, 1.0f / gamma);
        proc->inputs[7][i] = static_cast<uint8_t>(corrected * 255.0f);
    }
    std::cout << "Gamma done\n";
}

static void box_blur(tskr::Resource<ProcessedImages> proc)
{
    int w = proc->width, h = proc->height;

    std::vector<uint8_t> out(proc->inputs[0].size(), 0);

    for (int y = 1; y < h - 1; ++y)
    {
        for (int x = 1; x < w - 1; ++x)
        {
            int sum = 0;
            sum += proc->inputs[0][(y - 1) * w + (x - 1)];
            sum += proc->inputs[0][(y - 1) * w + (x)];
            sum += proc->inputs[0][(y - 1) * w + (x + 1)];
            sum += proc->inputs[0][(y)*w + (x - 1)];
            sum += proc->inputs[0][(y)*w + (x)];
            sum += proc->inputs[0][(y)*w + (x + 1)];
            sum += proc->inputs[0][(y + 1) * w + (x - 1)];
            sum += proc->inputs[0][(y + 1) * w + (x)];
            sum += proc->inputs[0][(y + 1) * w + (x + 1)];

            out[y * w + x] = static_cast<uint8_t>(sum / 9);
        }
    }

    proc->inputs[8].swap(out);
    std::cout << "Box Blur done\n";
}

static void median(tskr::Resource<ProcessedImages> proc)
{
    int w = proc->width, h = proc->height;

    std::vector<uint8_t> out(proc->inputs[0].size(), 0);
    uint8_t window[9];

    for (int y = 1; y < h - 1; ++y)
    {
        for (int x = 1; x < w - 1; ++x)
        {
            int idx = 0;
            for (int ky = -1; ky <= 1; ++ky)
                for (int kx = -1; kx <= 1; ++kx)
                    window[idx++] = proc->inputs[0][(y + ky) * w + (x + kx)];

            std::sort(window, window + 9);
            out[y * w + x] = window[4]; // median
        }
    }

    proc->inputs[9].swap(out);
    std::cout << "Median done\n";
}

static void bilateral(tskr::Resource<ProcessedImages> proc)
{
    int w = proc->width, h = proc->height;

    std::vector<uint8_t> out(proc->inputs[0].size(), 0);

    const float sigma_spatial = 1.0f;
    const float sigma_range = 25.0f;

    auto gauss = [](float x, float sigma) {
        return std::exp(-(x * x) / (2.0f * sigma * sigma));
        };

    for (int y = 1; y < h - 1; ++y)
    {
        for (int x = 1; x < w - 1; ++x)
        {
            float sum = 0.0f;
            float wsum = 0.0f;
            float center = proc->inputs[0][y * w + x];

            for (int ky = -1; ky <= 1; ++ky)
            {
                for (int kx = -1; kx <= 1; ++kx)
                {
                    float sample = proc->inputs[0][(y + ky) * w + (x + kx)];

                    float w_spatial = gauss(std::sqrt(float(kx * kx + ky * ky)), sigma_spatial);
                    float w_range = gauss(sample - center, sigma_range);

                    float weight = w_spatial * w_range;

                    sum += sample * weight;
                    wsum += weight;
                }
            }

            out[y * w + x] = static_cast<uint8_t>(sum / wsum);
        }
    }

    proc->inputs[10].swap(out);
    std::cout << "Bilateral filter done\n";
}

static void laplacian(tskr::Resource<ProcessedImages> proc)
{
    int w = proc->width, h = proc->height;

    std::vector<uint8_t> out(proc->inputs[0].size(), 0);

    int k[3][3] = {
        { 0, -1,  0},
        {-1,  4, -1},
        { 0, -1,  0}
    };

    for (int y = 1; y < h - 1; ++y)
    {
        for (int x = 1; x < w - 1; ++x)
        {
            int sum = 0;
            for (int ky = -1; ky <= 1; ++ky)
                for (int kx = -1; kx <= 1; ++kx)
                    sum += proc->inputs[0][(y + ky) * w + (x + kx)] * k[ky + 1][kx + 1];

            out[y * w + x] = clamp_u8(sum + 128); // center around mid-gray
        }
    }

    proc->inputs[11].swap(out);
    std::cout << "Laplacian done\n";
}

void unsharp(tskr::Resource<ProcessedImages> proc)
{
    int w = proc->width, h = proc->height;

    std::vector<uint8_t> out(proc->inputs[0].size(), 0);

    // Use the existing blur buffer as the "blurred" version
    const float amount = 1.5f;

    for (int i = 0; i < w * h; ++i)
    {
        int orig = proc->inputs[0][i];
        int blur = proc->inputs[0][i];
        int val = int(orig + amount * (orig - blur));
        out[i] = clamp_u8(val);
    }

    proc->inputs[12].swap(out);
    std::cout << "Unsharp done\n";
}

static void save_single(
    int index,
    tskr::Resource<Settings> settings,
    tskr::Resource<ProcessedImages> proc
)
{
    auto& path = settings->outputs[index];
    const int w = proc->width;
    const int h = proc->height;

    int ok = stbi_write_png(path.c_str(), w, h, 1, proc->inputs[index].data(), w);
    check(ok != 0, "Failed to write image");
    std::cout << "Saved: " << path << "\n";
}

template<int... Ns>
static void save_outputs_n_inner(tskr::Commands& cmds, std::integer_sequence<int, Ns...> const&)
{
    ((cmds.spawn(tskr::TaskFn<save_single, tskr::TaskSpawnType::Scheduled, Ns>{})), ...);
}

template<int N>
static void save_outputs_n(tskr::Commands& cmds)
{
    save_outputs_n_inner(cmds, std::make_integer_sequence<int, N>{});
}

static void save_outputs(
    tskr::Commands cmds,
    tskr::Resource<Settings> settings
)
{
    save_outputs_n<13>(cmds);
}

int main()
{
    tskr::Tasker tasker;

    tasker
        .add_schedules<Startup, Grayscale, PostProcess, Shutdown>(tskr::ExecutionPolicy::Single)
        // Setup data vectors and load the images into them
        .add_tasks<Startup>(
            tskr::TaskFn<prepare_outputs>{}.after(tskr::TaskFn<load_image>{})
        )

        // Apply grayscale
        .add_tasks<Grayscale>(tskr::TaskFn<grayscale>{})

        // Apply the prostprocess effects on the grayscaled image in parallel
        .add_tasks<PostProcess>(
            tskr::TaskFn<blur_3x3>{},
            tskr::TaskFn<sobel_edge>{},
            tskr::TaskFn<sharpen>{},
            tskr::TaskFn<emboss>{},
            tskr::TaskFn<threshold>{},
            tskr::TaskFn<invert>{},
            tskr::TaskFn<gamma_correct>{},
            tskr::TaskFn<box_blur>{},
            tskr::TaskFn<median>{},
            tskr::TaskFn<bilateral>{},
            tskr::TaskFn<laplacian>{},
            tskr::TaskFn<unsharp>{}
        )

        // Save all in parallel by spawning child tasks
        .add_tasks<Shutdown>(tskr::TaskFn<save_outputs>{})

        .register_resource(Settings{})
        .register_resource(RawImage{})
        .register_resource(ProcessedImages{})

        .run();
    return 0;
}