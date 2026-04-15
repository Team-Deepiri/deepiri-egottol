#include "main_window.h"
#include "../io/simulation_data.h"

namespace deepiri {

class MainWindow::Impl {
public:
    std::shared_ptr<SimulationData> simData;
};

MainWindow::MainWindow() : pImpl(std::make_unique<Impl>()) {}
MainWindow::~MainWindow() = default;

void MainWindow::show() {}

void MainWindow::setSimulationData(std::shared_ptr<SimulationData> data) {
    pImpl->simData = std::move(data);
}

}