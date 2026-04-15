#ifndef DEEPIRI_BJT_H
#define DEEPIRI_BJT_H

#include "device.h"
#include <string>
#include <cmath>

namespace deepiri {

enum class BJTType { NPN, PNP };

class BJT : public Device {
public:
    BJT(BJTType type = BJTType::NPN, double is = 1e-16, double bf = 100.0);
    BJT(const std::string& name, BJTType type = BJTType::NPN, double is = 1e-16, double bf = 100.0);

    void setType(BJTType bjtType) { type_ = bjtType; }
    BJTType getBJTType() const { return type_; }
    void setIS(double is) { Is_ = is; }
    void setBF(double bf) { BF_ = bf; }
    void setBR(double br) { BR_ = br; }
    void setVAF(double vaf) { VAF_ = vaf; }
    void setVAR(double var) { VAR_ = var; }
    void setCJE(double cje) { Cje_ = cje; }
    void setCJC(double cjc) { Cjc_ = cjc; }
    void setTF(double tf) { Tf_ = tf; }
    void setTR(double tr) { Tr_ = tr; }

    void initializeDC() override;
    std::vector<double> getCurrent() const override;
    std::vector<std::vector<double>> getConductanceMatrix() const override;

    std::string name() const override { return name_; }
    std::string type() const override { return "BJT"; }

    void getInitialGuess(std::vector<double>& guess) const override;
    void updateState(const std::vector<double>& state) override;

private:
    BJTType type_;
    std::string name_;
    double Is_, BF_, BR_, VAF_, VAR_;
    double Cje_, Cjc_, Tf_, Tr_;
    double vbe_, vbc_, ie_, ib_, ic_, gmu_, gpi_, gm_;
    double vt_;
    double area_;
};

}

#endif