#include "../core/mna_solver.h"
#include "../core/matrix.h"
#include "../gui/component_factory.h"
#include "../gui/component_item.h"
#include "../gui/main_window.h"
#include "../gui/port_layout.h"
#include "../gui/property_editor.h"
#include "../gui/scene.h"
#include "../gui/schematic_document.h"
#include "../gui/schematic_to_circuit.h"
#include "../gui/symbol_renderer.h"
#include "../gui/wire_item.h"
#include "../io/symbol_library.h"

#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QPainterPath>

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace deepiri;

namespace {

void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

SchematicComponent component(const QString &id, const QString &key,
                             std::initializer_list<QString> ports,
                             QMap<QString, QVariant> parameters = {}) {
  SchematicComponent result;
  result.id = id;
  result.registryKey = key;
  result.symbolKey = PortLayout::symbolKeyForRegistry(key);
  result.parameters = std::move(parameters);
  for (const QString &port : ports)
    result.ports.push_back({port, QStringLiteral("inout")});
  return result;
}

void connect(SchematicDocument &document, const QString &id,
             const QString &fromComponent, const QString &fromPort,
             const QString &toComponent, const QString &toPort) {
  document.addWire(
      {id, fromComponent, fromPort, toComponent, toPort});
}

void testCatalogAndRendering() {
  SymbolLibrary library;
  const auto resistor = library.getSymbol("RES");
  require(resistor.has_value(), "RES is absent from the native catalog");
  require(resistor->symbol_key == "R", "RES symbol key is incorrect");
  require(resistor->properties.at("R") == "1000",
          "RES default value is incorrect");
  require(PortLayout::portsForSymbol("GATE_AND").size() == 3,
          "AND gate port layout is incomplete");

  QImage image(100, 100, QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::transparent);
  QPainter painter(&image);
  painter.translate(50, 50);
  require(SymbolRenderer::draw(&painter, "VSRC", false),
          "VSRC renderer is unavailable");
}

void testGaussianSolver() {
  Matrix matrix(2, 2);
  matrix.at(0, 0) = 2.0;
  matrix.at(0, 1) = 1.0;
  matrix.at(1, 0) = 1.0;
  matrix.at(1, 1) = 3.0;
  const auto solution = matrix.solveGaussian({5.0, 6.0});
  require(std::abs(solution[0] - 1.8) < 1e-12 &&
              std::abs(solution[1] - 1.4) < 1e-12,
          "Gaussian back-substitution is incorrect");
}

void testDocumentAndScene() {
  SchematicDocument document;
  SchematicScene scene;
  scene.set_document(&document);
  ComponentItem *first = ComponentFactory::placeComponent(
      &scene, &document, "RES", QPointF(0, 0));
  ComponentItem *second = ComponentFactory::placeComponent(
      &scene, &document, "RES", QPointF(100, 0));
  require(first && second, "component placement failed");
  require(scene.start_wire(first->label(), "2", first->pin_position("2")),
          "wire start failed");
  require(scene.finish_wire(second->label(), "1", second->pin_position("1")),
          "wire finish failed");
  require(document.wires().size() == 1,
          "completed wire was not added to the document");

  const QPointF oldEnd = scene.wires().front()->end_point();
  second->setPos(140, 20);
  require(scene.wires().front()->end_point() != oldEnd,
          "wire endpoint did not follow a moved component");

  first->setSelected(true);
  scene.delete_selection();
  require(document.components().size() == 1,
          "component deletion did not update the document");
  require(document.wires().empty(),
          "component deletion left an orphan document wire");
}

void testOrthogonalPathAndProperties() {
  const QPainterPath path =
      WireItem::orthogonal_path(QPointF(0, 0), QPointF(50, 30));
  require(path.elementCount() == 4, "orthogonal path has wrong point count");
  require(path.elementAt(1).x == 0 && path.elementAt(2).x == 50,
          "wire path is not orthogonal");

  PropertyEditor editor("RES_test", {{"R", 1000.0}});
  require(std::abs(editor.parameters().value("R").toDouble() - 1000.0) <
              1e-12,
          "property editor did not retain numeric values");
}

void testDivider() {
  SchematicDocument document;
  document.addComponent(component("V1", "VSRC", {"+", "-"}, {{"V", 5.0}}));
  document.addComponent(component("R1", "RES", {"1", "2"}, {{"R", 1000.0}}));
  document.addComponent(component("R2", "RES", {"1", "2"}, {{"R", 1000.0}}));
  document.addComponent(component("G1", "GND", {"G"}));
  connect(document, "W1", "V1", "+", "R1", "1");
  connect(document, "W2", "R1", "2", "R2", "1");
  connect(document, "W3", "R2", "2", "V1", "-");
  connect(document, "W4", "V1", "-", "G1", "G");

  SchematicCircuitExport exported = buildCircuitFromSchematic(document);
  require(exported.isValid(), "divider netlist export failed");
  MNASolver solver;
  const auto solution =
      solver.solve(exported.circuit->getDevices(), exported.solverNodeMap, {});
  require(solution.success, "divider solve failed");

  const size_t midpoint = exported.portNodes.value("R1:2");
  require(midpoint > 0 && midpoint <= solution.voltages.size(),
          "divider midpoint node is invalid");
  require(std::abs(solution.voltages[midpoint - 1] - 2.5) < 1e-6,
          "divider midpoint is not 2.5 V");
  require(exported.portNodes.value("G1:G") == 0,
          "ground net was not assigned node zero");
}

} // namespace

int main(int argc, char **argv) {
  qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
  QApplication app(argc, argv);
  try {
    testCatalogAndRendering();
    testGaussianSolver();
    testDocumentAndScene();
    testOrthogonalPathAndProperties();
    testDivider();
    MainWindow window;
    require(window.document() != nullptr && window.schematicScene() != nullptr,
            "main window smoke test failed");
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
  return 0;
}
