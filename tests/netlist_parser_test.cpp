// NetlistParser + NetlistBuilder regression tests with real-world SPICE forms.
#include "../io/netlist_parser.h"
#include "../io/netlist_builder.h"
#include "../core/mna_solver.h"
#include "../core/ac_analysis.h"
#include "../core/spice_engine.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
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

void expectNear(double got, double want, double tol, const char* what) {
    if (std::fabs(got - want) > tol) {
        std::fprintf(stderr, "FAILED: %s (got %g, want %g ± %g)\n", what, got, want, tol);
        ++failures;
    }
}

void test_parse_value_suffixes() {
    double v = 0.0;
    expect(NetlistParser::parseValue("1k", v) && std::fabs(v - 1000.0) < 1e-9, "1k → 1000");
    expect(NetlistParser::parseValue("4.7u", v) && std::fabs(v - 4.7e-6) < 1e-15, "4.7u");
    expect(NetlistParser::parseValue("100m", v) && std::fabs(v - 0.1) < 1e-12, "100m");
    expect(NetlistParser::parseValue("2Meg", v) && std::fabs(v - 2e6) < 1e-3, "2Meg");
    expect(NetlistParser::parseValue("1G", v) && std::fabs(v - 1e9) < 1.0, "1G");
    expect(NetlistParser::parseValue("10n", v) && std::fabs(v - 1e-8) < 1e-18, "10n");
    expect(NetlistParser::parseValue("5p", v) && std::fabs(v - 5e-12) < 1e-20, "5p");
    expect(NetlistParser::parseValue("1.5e3", v) && std::fabs(v - 1500.0) < 1e-9, "1.5e3");
}

void test_standard_spice_form() {
    const char* nl =
        "* voltage divider\n"
        "V1 in 0 5\n"
        "R1 in mid 1k\n"
        "R2 mid 0 1k\n"
        ".op\n"
        ".end\n";

    NetlistParser parser;
    expect(parser.parse(nl), "parse divider");

    auto elems = parser.getElements();
    expect(elems.size() == 3, "3 elements");
    expect(elems[0].type == NetlistElementType::VoltageSource, "V1 type");
    expect(elems[0].name == "V1", "V1 name");
    expect(elems[0].nodes.size() == 2, "V1 nodes");
    expect(elems[0].parameters.size() == 1 && elems[0].parameters[0] == 5.0, "V1=5");
    expect(elems[1].type == NetlistElementType::Resistor, "R1 type");
    expect(elems[1].parameters.size() == 1 && std::fabs(elems[1].parameters[0] - 1000.0) < 1e-9,
           "R1=1k");
    expect(elems[2].parameters.size() == 1 && std::fabs(elems[2].parameters[0] - 1000.0) < 1e-9,
           "R2=1k");

    auto ctrls = parser.getControlDirectives();
    bool hasOp = false, hasEnd = false;
    for (const auto& c : ctrls) {
        if (c.kind == "op") hasOp = true;
        if (c.kind == "end") hasEnd = true;
    }
    expect(hasOp && hasEnd, ".op and .end recognized as controls");
}

void test_named_type_form_still_works() {
    const char* nl = "R load n1 n2 2.2k\n";
    NetlistParser parser;
    expect(parser.parse(nl), "parse named form");
    auto elems = parser.getElements();
    expect(elems.size() == 1, "one element");
    expect(elems[0].type == NetlistElementType::Resistor, "resistor");
    expect(elems[0].name == "load", "name=load");
    expect(elems[0].nodes.size() == 2, "2 nodes");
    expectNear(elems[0].parameters[0], 2200.0, 1e-6, "2.2k");
}

void test_control_cards_not_elements() {
    const char* nl =
        "R1 1 0 1k\n"
        ".tran 1u 1m\n"
        ".ac dec 10 1 1Meg\n"
        ".dc V1 0 5 0.1\n"
        ".step param R 1k 10k 1k\n"
        ".op\n";

    NetlistParser parser;
    expect(parser.parse(nl), "parse controls");
    expect(parser.getElements().size() == 1, "only R1 is an element");

    auto dirs = parser.getControlDirectives();
    expect(dirs.size() == 5, "5 control directives");

    bool sawTran = false, sawAc = false, sawDc = false, sawStep = false, sawOp = false;
    for (const auto& d : dirs) {
        if (d.kind == "tran") {
            sawTran = true;
            expect(d.numbers.size() >= 2, ".tran has numbers");
            expectNear(d.numbers[0], 1e-6, 1e-15, "tran step 1u");
            expectNear(d.numbers[1], 1e-3, 1e-12, "tran stop 1m");
        }
        if (d.kind == "ac") {
            sawAc = true;
            expect(d.numbers.size() >= 3, ".ac has numbers");
            expectNear(d.numbers.back(), 1e6, 1.0, "ac fstop 1Meg");
        }
        if (d.kind == "dc") sawDc = true;
        if (d.kind == "step") sawStep = true;
        if (d.kind == "op") sawOp = true;
    }
    expect(sawTran && sawAc && sawDc && sawStep && sawOp, "all analysis cards found");
}

void test_builder_solves_divider() {
    const char* nl =
        "V1 in 0 5\n"
        "R1 in mid 1k\n"
        "R2 mid 0 1k\n";

    NetlistParser parser;
    parser.parse(nl);
    BuiltCircuit circuit = buildCircuitFromNetlist(parser);
    expect(circuit.ok, "builder ok");
    expect(circuit.devices.size() == 3, "3 devices");
    expect(circuit.nodeMap.count("in") == 1 && circuit.nodeMap.count("mid") == 1, "node map");

    MNASolver solver;
    auto sol = solver.solve(circuit.devices, circuit.nodeMap, {});
    expect(sol.success, "MNA solve success");
    if (sol.success && sol.voltages.size() >= 2) {
        size_t midIdx = circuit.nodeMap["mid"];
        // voltages[] is 0-based for nodes 1..N
        expectNear(sol.voltages[midIdx - 1], 2.5, 0.05, "mid ≈ 2.5 V");
    }
}

void test_rc_netlist_ac() {
    const char* nl =
        "V1 in 0 1 1\n"
        "R1 in out 1k\n"
        "C1 out 0 1u\n"
        ".ac dec 5 1 1Meg\n";

    NetlistParser parser;
    parser.parse(nl);
    BuiltCircuit circuit = buildCircuitFromNetlist(parser);
    expect(circuit.ok, "RC builder ok");

    ACAnalysis ac;
    auto sweep = ac.sweep(circuit.devices, circuit.nodeMap, 1.0, 1e6, 20);
    expect(sweep.success, "AC sweep success");
    expect(!sweep.frequenciesHz.empty(), "has frequencies");
}

void test_mosfet_bjt_diode_lines() {
    const char* nl =
        "D1 a 0 DMOD\n"
        "Q1 c b e NPN\n"
        "M1 d g s b NMOS W=10u L=1u\n"
        "R1 a 0 1k\n";

    NetlistParser parser;
    expect(parser.parse(nl), "parse active devices");
    auto elems = parser.getElements();
    expect(elems.size() == 4, "4 elements");
    expect(elems[0].type == NetlistElementType::Diode && elems[0].model_name == "DMOD", "diode model");
    expect(elems[1].type == NetlistElementType::BJT && elems[1].nodes.size() == 3, "bjt 3 nodes");
    expect(elems[2].type == NetlistElementType::MOSFET && elems[2].nodes.size() == 4, "mos 4 nodes");
}

void test_model_card() {
    const char* nl =
        ".model mynmos NMOS (VTO=0.5 KP=50u LAMBDA=0.02)\n"
        "M1 d g 0 0 mynmos W=20u L=2u\n"
        "Vdd d 0 3\n"
        "Vg g 0 1\n";
    NetlistParser p;
    expect(p.parse(nl), "parse .model");
    auto models = p.getModels();
    expect(models.count("mynmos") == 1, "mynmos present");
    if (models.count("mynmos")) {
        expect(std::fabs(models["mynmos"].params["vto"] - 0.5) < 1e-9, "VTO");
        expect(std::fabs(models["mynmos"].params["kp"] - 50e-6) < 1e-12, "KP");
    }
    auto c = buildCircuitFromNetlist(p);
    expect(c.ok, "build with model");
}

void test_subckt_expand() {
    const char* nl =
        ".subckt DIV in mid\n"
        "R1 in mid 3k\n"
        "R2 mid 0 1k\n"
        ".ends\n"
        "V1 in 0 10\n"
        "X1 in mid DIV\n"
        ".op\n";
    NetlistParser p;
    expect(p.parse(nl), "parse subckt");
    expect(p.getSubckts().count("div") == 1, "subckt stored");
    expect(p.getElements().size() == 2, "top-level V+X only");
    auto flat = p.expandedElements();
    expect(flat.size() == 3, "expanded to V+R+R");  // V + 2R
    auto c = buildCircuitFromNetlist(p);
    expect(c.ok, "subckt build");
    auto sol = MNASolver().solve(c.devices, c.nodeMap, {});
    if (!sol.success) sol = DcOperatingPoint().solve(c.devices, c.nodeMap);
    expect(sol.success, "subckt dc");
    if (sol.success && c.nodeMap.count("mid")) {
        expect(std::fabs(sol.voltages[c.nodeMap["mid"] - 1] - 2.5) < 1e-3, "subckt mid=2.5");
    }
}

void test_nested_subckt() {
    const char* nl =
        ".subckt RDIV in out\n"
        "R1 in out 3k\n"
        "R2 out 0 1k\n"
        ".ends\n"
        ".subckt OUTER in out\n"
        "Xinner in out RDIV\n"
        ".ends\n"
        "V1 in 0 10\n"
        "X1 in mid OUTER\n";
    NetlistParser p;
    expect(p.parse(nl), "parse nested");
    auto flat = p.expandedElements();
    // V + 2R (inner fully flattened)
    expect(flat.size() == 3, "nested flattened to 3");
    auto c = buildCircuitFromNetlist(p);
    auto sol = DcOperatingPoint().solve(c.devices, c.nodeMap);
    if (!sol.success) sol = MNASolver().solve(c.devices, c.nodeMap, {});
    expect(sol.success, "nested dc");
    if (c.nodeMap.count("mid")) {
        expect(std::fabs(sol.voltages[c.nodeMap["mid"] - 1] - 2.5) < 1e-3, "nested mid=2.5");
    }
}

}  // namespace

int main() {
    test_parse_value_suffixes();
    test_standard_spice_form();
    test_named_type_form_still_works();
    test_control_cards_not_elements();
    test_builder_solves_divider();
    test_rc_netlist_ac();
    test_mosfet_bjt_diode_lines();
    test_model_card();
    test_subckt_expand();
    test_nested_subckt();

    if (failures > 0) {
        std::fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    std::printf("netlist_parser_test: all passed\n");
    return 0;
}
