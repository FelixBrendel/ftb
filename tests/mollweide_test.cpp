#define _CRT_SECURE_NO_WARNINGS
#define FTB_INTERNAL_DEBUG
#define FTB_ALLOCATION_STATS_IMPL
#define FTB_MOLLWEIDE_IMPL
#define FTB_MATH_IMPL
#define FTB_TRACK_MALLOCS

#include <stdlib.h>
#include <random>

#include "../allocation_stats.hpp"
#include "../mollweide.hpp"

std::random_device rd;
std::mt19937 e2(rd());
std::uniform_real_distribution<> dist(-1.0f, 1.0f);

V3 rand_dir() {
    return noz(V3{
            (f32)dist(e2),
            (f32)dist(e2),
            (f32)dist(e2),
    });

    // f32 phi   = dist(e2) * pi / 2;
    // f32 theta = dist(e2) * pi;

    // return (m4x4_from_axis_angle({0,0,1}, theta) *
    //         m4x4_from_axis_angle({1,0,0}, phi) *
    //         V4{0, 1, 0, 1}).xyz;

    // return noz(V3{
            // (f32)(rand() % 1000 - 500),
            // (f32)(rand() % 1000 - 500),
            // (f32)(rand() % 1000 - 500),
    // });
}

void write_image(u8* pixels, s32 width, s32 height, const char* file_name) {
    // NOTE(Felix) swizzle from RGBA tgo BGRA for bmp convention
    {
        #pragma pack(push, 1)
        struct Pixel {
            u8 r; u8 g; u8 b; u8 a;
        };
        #pragma pack(pop)


        for (u32 i = 0; i < width * height; ++i) {
            Pixel* px = (Pixel*)(pixels+(i*sizeof(u32)));
            u8 t = px->r;
            px->r = px->b;
            px->b = t;
        }
    }

#pragma pack(push, 1)
    struct bitmap_header
    {
        u16 FileType;
        u32 FileSize;
        u16 Reserved1;
        u16 Reserved2;
        u32 BitmapOffset;
        u32 Size;
        s32 Width;
        s32 Height;
        u16 Planes;
        u16 BitsPerPixel;
        u32 Compression;
        u32 SizeOfBitmap;
        s32 HorzResolution;
        s32 VertResolution;
        u32 ColorsUsed;
        u32 ColorsImportant;
    };
#pragma pack(pop)

    u32 output_px_size = sizeof(u32) * width * height;

    bitmap_header header = {};
    header.FileType = 0x4D42;
    header.FileSize = sizeof(header) + output_px_size;
    header.BitmapOffset = sizeof(header);
    header.Size = sizeof(header) - 14;
    header.Width = width;
    header.Height = height;
    header.Planes = 1;
    header.BitsPerPixel = 32;
    header.Compression = 0;
    header.SizeOfBitmap = output_px_size;
    header.HorzResolution = 0;
    header.VertResolution = 0;
    header.ColorsUsed = 0;
    header.ColorsImportant = 0;

    FILE *out_file = fopen(file_name, "wb");
    defer { fclose(out_file); };

    fwrite(&header, sizeof(header), 1, out_file);
    fwrite(pixels, output_px_size, 1, out_file);
}

// V3 dirs[100000];
V3 dirs[1000000];

int main(int argc, char* argv[]) {
    for (u32 i = 0; i < array_length(dirs); ++i) {
        dirs[i] = rand_dir();
    }

    Mollweide_Grid<s32> m_grid;

    Array_List<s32> retrieval;
    retrieval.init(array_length(dirs));

    profile_mallocs {
        m_grid.init(7);

        srand(11);

        for (int i = 0; i < array_length(dirs); ++i) {
            m_grid.insert(dirs[i], i);
        }

        u8* pixels = m_grid.render_heatmap(512);
        write_image(pixels, 1024, 512, "heatmap.bmp");

        m_grid.range_query(V2{-0.5, -0.25}, V2{0.0, 0}, &retrieval);


        m_grid.deinit();
    };

    FILE* partial = fopen("obj/partial_sphere.obj", "wb");
    FILE* full    = fopen("obj/full_sphere.obj", "wb");
    defer {
        fclose(partial);
        fclose(full);
    };
    for (V3 point : dirs) {
        fprintf(full, "v %f %f %f\n", point.x, point.y, point.z);
    }
    for (s32 idx : retrieval) {
        fprintf(partial, "v %f %f %f\n", dirs[idx].x, dirs[idx].y, dirs[idx].z);
    }
}
