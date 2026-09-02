#pragma once

#include <QGraphicsItem>
#include <QString>
#include <QList>
#include <QPointF>
#include <QSizeF>
#include <QRectF>
#include <QColor>
#include <QMap>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

namespace deepiri {

enum class ComponentType {
    AND_GATE,
    OR_GATE,
    NOT_GATE,
    NAND_GATE,
    NOR_GATE,
    XOR_GATE,
    XNOR_GATE,
    D_FLIPFLOP,
    JK_FLIPFLOP,
    T_FLIPFLOP,
    SOURCE,
    GROUND,
    LED,
    RESISTOR,
    CAPACITOR,
    INDUCTOR,
    VCC,
    BUFFER,
    CUSTOM,
    // Batch 1 — Passive & Discrete
    VARISTOR,
    THERMISTOR_NTC,
    THERMISTOR_PTC,
    TRIMMER,
    CAP_ELEC,
    CAP_CER,
    CAP_FILM,
    CAP_TANT,
    CAP_TRIM,
    IND_FERRITE,
    IND_VAR,
    FUSE,
    FUSE_PTC,
    CRYSTAL,
    RESONATOR,
    BUZZER,
    RELAY_SPST,
    RELAY_DPDT,
    PHOTODIODE,
    PHOTOTRANS,
    OPTOCOUPLER,
    BRIDGE_RECT,
    TVS,
    SCHOTTKY,
    DIODE_TUNNEL,
    // Batch 2 — Active
    DARLINGTON,
    JFET_N,
    JFET_P,
    SCR,
    TRIAC,
    DIAC,
    IGBT,
    PHOTOTRIAC,
    VARACTOR,
    LED_RGB,
    LASER_DIODE,
    DIODE_SHOCKLEY,
    HBT,
    HEMT,
    MESFET,
    SOLAR_CELL,
    LED_7SEG,
    LED_MATRIX,
    DIODE_BRIDGE_MOD,
    THERMOCOUPLE,
    PELTIER,
    MEMS_ACCEL,
    MEMS_GYRO,
    PHOTO_VOLTAIC,
    CURRENT_MIRROR,
    // Batch 3 — IC Blocks
    LM393,
    LM311,
    LM324,
    NE5532,
    OPA2134,
    AD823,
    INA128,
    LM386,
    TDA7297,
    MAX232,
    MAX485,
    MAX31855,
    ADS1115,
    MCP3008,
    DAC0808,
    MCP4725,
    DS1307,
    DS18B20,
    DHT22,
    BMP280,
    MPU6050,
    HC_SR04,
    TCRT5000,
    LM35,
    TMP36,
    // Batch 4 — Logic
    BUFFER_GATE,
    BUF_TRI,
    SCHMITT_INV,
    SCHMITT_AND,
    DECODER_2TO4,
    DECODER_4TO16,
    ENCODER_4TO2,
    DEMUX_1TO4,
    DEMUX_1TO16,
    MUX_4TO1,
    MUX_8TO1,
    MUX_16TO1,
    HALF_ADDER,
    FULL_ADDER,
    RIPPLE_ADDER,
    ALU_4BIT,
    COMP_4BIT,
    JK_FF,
    T_FF,
    SR_FF,
    COUNTER_BIN,
    COUNTER_BCD,
    SHIFT_REG,
    REG_8BIT,
    LATCH_8BIT,
    // Batch 5 — Power & RF
    LM2596,
    LM2577,
    XL6009,
    TPS7A47,
    AMS1117,
    LM337,
    TL431,
    MCP1700,
    ICL7660,
    MAX660,
    IRF520,
    IRF9530,
    IRLZ44,
    TRANSFO_CT,
    TRANSFO_3PH,
    CHOKE_CM,
    COUPLER_DIR,
    SPLITTER_RF,
    MIXER_RF,
    ATTENUATOR_RF,
    BALUN,
    ANT_LOOP,
    ANT_PATCH,
    ANT_DIPOLE,
    ANT_YAGI,
    // Batch 6 — Quantum
    QUBIT,
    HADAMARD,
    PAULI_X,
    PAULI_Y,
    PAULI_Z,
    S_GATE,
    T_GATE,
    CNOT,
    CZ,
    SWAP,
    CSWAP,
    TOFFOLI,
    PHASE_SHIFT,
    U1,
    U2,
    U3,
    RX,
    RY,
    RZ,
    MEASURE,
    RESET,
    BARRIER,
    QFT_BLOCK,
    QUBIT_REG,
    Q_IC,
    // Batch 7 — Sensors, Connectors, Electromechanical
    CONN_2PIN,
    CONN_3PIN,
    CONN_4PIN,
    CONN_8PIN,
    CONN_DB9,
    CONN_USB,
    CONN_HDMI,
    CONN_RCA,
    CONN_BNC,
    CONN_JACK,
    HEADER_2X5,
    HEADER_2X8,
    JUMPER,
    TEST_POINT,
    MOTOR_DC,
    MOTOR_STEPPER,
    MOTOR_SERVO,
    SOLENOID,
    FAN,
    SPEAKER,
    MICROPHONE,
    TRANSFO_AUDIO,
    HALL_SENSOR,
    CURRENT_SENSE,
    SHUNT
};

struct Pin {
    QString name;
    QPointF position;
    bool is_input;
    int bit_width;
};

class ComponentItem : public QGraphicsItem {
public:
    explicit ComponentItem(ComponentType type, const QString& label = QString(), QGraphicsItem* parent = nullptr);
    ~ComponentItem();

    ComponentType component_type() const;
    QString label() const;
    void set_label(const QString& label);

    // Electrical / display properties (e.g. "value" → "1k").
    QString property(const QString& key, const QString& fallback = QString()) const;
    void set_property(const QString& key, const QString& value);
    QMap<QString, QString> properties() const;
    void set_properties(const QMap<QString, QString>& props);

    QList<Pin> pins() const;
    void add_pin(const Pin& pin);
    QPointF pin_position(const QString& name) const;
    QString pin_at(const QPointF& pos) const;

    void set_selection_color(const QColor& color);
    QColor selection_color() const;

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    int type() const override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    void update_pins();

    class ComponentItemImpl;
    ComponentItemImpl* d;
};

}