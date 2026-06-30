#include "netlist_parser.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace deepiri {

class NetlistParser::Impl {
public:
  std::vector<NetlistElement> elements_;
  std::map<std::string, std::vector<std::string>> nets_;
  std::vector<std::string> controls_;

  static std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
      return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
  }

  static NetlistElementType parseElementType(const std::string &type_str) {
    if (type_str == "R" || type_str == "res")
      return NetlistElementType::Resistor;
    if (type_str == "C" || type_str == "cap")
      return NetlistElementType::Capacitor;
    if (type_str == "L" || type_str == "ind")
      return NetlistElementType::Inductor;
    if (type_str == "V" || type_str == "vsrc")
      return NetlistElementType::VoltageSource;
    if (type_str == "I" || type_str == "isrc")
      return NetlistElementType::CurrentSource;
    if (type_str == "M" || type_str == "mos")
      return NetlistElementType::MOSFET;
    if (type_str == "Q" || type_str == "bjt")
      return NetlistElementType::BJT;
    if (type_str == "D" || type_str == "diode")
      return NetlistElementType::Diode;
    if (type_str == "X")
      return NetlistElementType::Instance;
    return NetlistElementType::Instance;
  }

  static bool isControlLine(const std::string &line) {
    return line.find(".end") != std::string::npos ||
           line.find(".include") != std::string::npos ||
           line.find(".lib") != std::string::npos ||
           line.find(".param") != std::string::npos ||
           line.find(".option") != std::string::npos;
  }
};

NetlistParser::NetlistParser() : pImpl(std::make_unique<Impl>()) {}
NetlistParser::~NetlistParser() = default;

bool NetlistParser::parse(const std::string &netlist_content) {
  pImpl->elements_.clear();
  pImpl->nets_.clear();
  pImpl->controls_.clear();

  std::istringstream iss(netlist_content);
  std::string line;

  while (std::getline(iss, line)) {
    line = Impl::trim(line);
    if (line.empty() || line[0] == '*' || line[0] == ';') {
      continue;
    }

    if (Impl::isControlLine(line)) {
      pImpl->controls_.push_back(line);
      continue;
    }

    std::istringstream line_iss(line);
    std::string type_str, name;
    line_iss >> type_str >> name;

    if (name.empty())
      continue;

    NetlistElement elem;
    elem.type = Impl::parseElementType(type_str);
    elem.name = name;

    if (elem.type == NetlistElementType::Instance) {
      elem.subckt_name = type_str;
    }

    std::string node;
    while (line_iss >> node) {
      if (!node.empty()) {
        NetlistNode n;
        n.name = node;
        n.net = node;
        elem.nodes.push_back(n);
        pImpl->nets_[node].push_back(name);
      }
    }

    double param;
    while (line_iss >> param) {
      elem.parameters.push_back(param);
    }

    pImpl->elements_.push_back(elem);
  }

  return true;
}

bool NetlistParser::loadFromFile(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    return false;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return parse(buffer.str());
}

std::vector<NetlistElement> NetlistParser::getElements() const {
  return pImpl->elements_;
}

std::map<std::string, std::vector<std::string>> NetlistParser::getNets() const {
  return pImpl->nets_;
}

std::vector<std::string> NetlistParser::getControls() const {
  return pImpl->controls_;
}

std::string NetlistParser::toNetlist() const {
  std::ostringstream oss;

  for (const auto &ctrl : pImpl->controls_) {
    oss << ctrl << "\n";
  }

  for (const auto &elem : pImpl->elements_) {
    switch (elem.type) {
    case NetlistElementType::Resistor:
      oss << "R";
      break;
    case NetlistElementType::Capacitor:
      oss << "C";
      break;
    case NetlistElementType::Inductor:
      oss << "L";
      break;
    case NetlistElementType::VoltageSource:
      oss << "V";
      break;
    case NetlistElementType::CurrentSource:
      oss << "I";
      break;
    case NetlistElementType::MOSFET:
      oss << "M";
      break;
    case NetlistElementType::BJT:
      oss << "Q";
      break;
    case NetlistElementType::Diode:
      oss << "D";
      break;
    default:
      oss << "X";
      break;
    }

    oss << " " << elem.name;

    for (const auto &node : elem.nodes) {
      oss << " " << node.net;
    }

    for (double p : elem.parameters) {
      oss << " " << p;
    }

    oss << "\n";
  }

  return oss.str();
}

} // namespace deepiri