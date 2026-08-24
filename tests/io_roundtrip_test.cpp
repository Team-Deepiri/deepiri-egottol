// ProjectLoader round-trip + WaveformWriter smoke tests.
#include "../io/project_loader.h"
#include "../io/waveform_writer.h"
#include "../io/netlist_parser.h"
#include "../io/netlist_builder.h"
#include "../core/mna_solver.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <string>

using namespace deepiri;

namespace {

int failures = 0;

void expect(bool cond, const char* what) {
    if (!cond) {
        std::fprintf(stderr, "FAILED: %s\n", what);
        ++failures;
    }
}

void test_project_roundtrip() {
    Project p;
    p.name = "divider";
    p.version = "1.0";
    p.author = "test";
    p.format = "egottol-project";

    SchematicComponentData v;
    v.type = 29;  // SOURCE-ish; exact enum value doesn't matter for IO test
    v.label = "V1";
    v.x = 10;
    v.y = 20;
    v.properties["value"] = "5";
    p.components.push_back(v);

    SchematicComponentData r;
    r.type = 32;
    r.label = "R1";
    r.x = 100;
    r.y = 20;
    r.properties["value"] = "1k";
    p.components.push_back(r);

    SchematicWireData w;
    w.points = {{10, 40}, {100, 40}};
    p.wires.push_back(w);

    ProjectLoader loader;
    loader.setProject(p);
    const std::string path = "/tmp/egottol_test_project.egt";
    expect(loader.save(path), "save .egt");

    ProjectLoader loader2;
    expect(loader2.load(path), "load .egt");
    Project got = loader2.getProject();
    expect(got.name == "divider", "name");
    expect(got.components.size() == 2, "2 components");
    expect(got.components[0].label == "V1", "V1 label");
    expect(got.components[0].properties["value"] == "5", "V1 value");
    expect(got.components[1].properties["value"] == "1k", "R1 value");
    expect(got.wires.size() == 1 && got.wires[0].points.size() == 2, "wire points");
}

void test_waveform_csv() {
    WaveformData a;
    a.name = "V(out)";
    a.time_points = {0.0, 1e-6, 2e-6};
    a.values = {0.0, 1.0, 2.0};

    WaveformWriter writer;
    const std::string path = "/tmp/egottol_test_wave.csv";
    expect(writer.writeCSV(path, {a}), "write CSV");

    std::ifstream in(path);
    expect(in.good(), "csv readable");
    std::string header;
    std::getline(in, header);
    expect(header.find("time") != std::string::npos && header.find("V(out)") != std::string::npos,
           "csv header");
}

void test_end_to_end_netlist_dc() {
    const char* nl =
        "* product loop smoke\n"
        "V1 in 0 12\n"
        "R1 in out 2.2k\n"
        "R2 out 0 4.7k\n"
        ".op\n"
        ".end\n";
    NetlistParser parser;
    expect(parser.parse(nl), "parse");
    BuiltCircuit c = buildCircuitFromNetlist(parser);
    expect(c.ok, "build");
    MNASolver solver;
    auto sol = solver.solve(c.devices, c.nodeMap, {});
    expect(sol.success, "solve");
    // Voltage divider: 12 * 4.7/(2.2+4.7) ≈ 8.17 V
    size_t out = c.nodeMap["out"];
    expect(out >= 1 && out - 1 < sol.voltages.size(), "out index");
    if (out >= 1 && out - 1 < sol.voltages.size()) {
        double v = sol.voltages[out - 1];
        if (std::fabs(v - 8.1739) > 0.2) {
            std::fprintf(stderr, "FAILED: out voltage %g (want ~8.17)\n", v);
            ++failures;
        }
    }
}

}  // namespace

int main() {
    test_project_roundtrip();
    test_waveform_csv();
    test_end_to_end_netlist_dc();
    if (failures > 0) {
        std::fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    std::printf("io_roundtrip_test: all passed\n");
    return 0;
}
