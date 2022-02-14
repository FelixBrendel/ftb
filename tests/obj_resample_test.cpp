#define  _CRT_SECURE_NO_WARNINGS
#define FTB_MESH_IMPL
#define FTB_MATH_IMPL
#define FTB_STACKTRACE_IMPL
#define FTB_IO_IMPL
#define FTB_PRINT_IMPL
#define FTB_HASHMAP_IMPL

#include "../mesh.hpp"
#include "../print.hpp"

int main() {
    init_printer();

    Mesh_Data m = load_obj("obj/test.obj");
    defer {
        m.faces.deinit();
        m.vertices.deinit();
    };

    println("veritces: %u\n"
            "-------------", m.vertices.count);
    for (auto v : m.vertices) {
        println("  %{->v3}", &v);
    }

    println("faces:    %u\n"
            "-------------", m.faces.count);

    for (auto f : m.faces) {
        println("  %{u32[3]}", &f);
    }

    Array_List<V3> resamples;
    u32 num_samples = 10000;
    resamples.init(num_samples);

    resample_mesh(m, num_samples, &resamples);

    FILE* out = fopen("obj/samples.obj", "wb");
    defer {
        fclose(out);
    };

    for (auto v : resamples) {
        print_to_file(out, "v %f %f %f\n", v.x, v.y, v.z);
    }

    println("Done!");
}
