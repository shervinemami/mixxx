#include "widget/controlwidgetconnection.h"

#include "widget/wbasewidget.h"
#include "control/controlobjectslave.h"
#include "util/debug.h"
#include "util/valuetransformer.h"
#include "util/assert.h"

ControlWidgetConnection::ControlWidgetConnection(WBaseWidget* pBaseWidget,
                                                 const ConfigKey& key,
                                                 ValueTransformer* pTransformer)
        : m_pWidget(pBaseWidget),
          m_pValueTransformer(pTransformer) {
    m_pControl = new ControlObjectSlave(key, this);
    m_pControl->connectValueChanged(SLOT(slotControlValueChanged(double)));
}

void ControlWidgetConnection::setControlParameter(double parameter) {
    if (m_pValueTransformer != nullptr) {
        parameter = m_pValueTransformer->transformInverse(parameter);
    }
    m_pControl->setParameter(parameter);
}

double ControlWidgetConnection::getControlParameter() const {
    double parameter = m_pControl->getParameter();
    if (m_pValueTransformer != nullptr) {
        parameter = m_pValueTransformer->transform(parameter);
    }
    return parameter;
}

double ControlWidgetConnection::getControlParameterForValue(double value) const {
    double parameter = m_pControl->getParameterForValue(value);
    if (m_pValueTransformer != nullptr) {
        parameter = m_pValueTransformer->transform(parameter);
    }
    return parameter;
}

ControlParameterWidgetConnection::ControlParameterWidgetConnection(WBaseWidget* pBaseWidget,
                                                                   const ConfigKey& key,
                                                                   ValueTransformer* pTransformer,
                                                                   DirectionOption directionOption,
                                                                   EmitOption emitOption)
        : ControlWidgetConnection(pBaseWidget, key, pTransformer),
          m_directionOption(directionOption),
          m_emitOption(emitOption) {
}

void ControlParameterWidgetConnection::Init() {
    slotControlValueChanged(m_pControl->get());
}

QString ControlParameterWidgetConnection::toDebugString() const {
    const ConfigKey& key = getKey();
    return QString("%1,%2 Parameter: %3 Direction: %4 Emit: %5")
            .arg(key.group, key.item,
                 QString::number(m_pControl->getParameter()),
                 directionOptionToString(m_directionOption),
                 emitOptionToString(m_emitOption));
}

void ControlParameterWidgetConnection::slotControlValueChanged(double value) {
    if (m_directionOption & DIR_TO_WIDGET) {
        double parameter = getControlParameterForValue(value);
        m_pWidget->onConnectedControlChanged(parameter, value);
    }
}

void ControlParameterWidgetConnection::resetControl() {
    if (m_directionOption & DIR_FROM_WIDGET) {
        m_pControl->reset();
    }
}

void ControlParameterWidgetConnection::setControlParameter(double v) {
    if (m_directionOption & DIR_FROM_WIDGET) {
        ControlWidgetConnection::setControlParameter(v);
    }
}

void ControlParameterWidgetConnection::setControlParameterDown(double v) {
    if ((m_directionOption & DIR_FROM_WIDGET) && (m_emitOption & EMIT_ON_PRESS)) {
        ControlWidgetConnection::setControlParameter(v);
    }
}

void ControlParameterWidgetConnection::setControlParameterUp(double v) {
    if ((m_directionOption & DIR_FROM_WIDGET) && (m_emitOption & EMIT_ON_RELEASE)) {
        ControlWidgetConnection::setControlParameter(v);
    }
}

ControlWidgetPropertyConnection::ControlWidgetPropertyConnection(WBaseWidget* pBaseWidget,
                                                                 const ConfigKey& key,
                                                                 ValueTransformer* pTransformer,
                                                                 const QString& propertyName)
        : ControlWidgetConnection(pBaseWidget, key, pTransformer),
          m_propertyName(propertyName.toAscii()) {
    slotControlValueChanged(m_pControl->get());
}

QString ControlWidgetPropertyConnection::toDebugString() const {
    const ConfigKey& key = getKey();
    return QString("%1,%2 Parameter: %3 Property: %4 Value: %5").arg(
        key.group, key.item, QString::number(m_pControl->getParameter()), m_propertyName,
        m_pWidget->toQWidget()->property(
            m_propertyName.constData()).toString());
}

void ControlWidgetPropertyConnection::slotControlValueChanged(double v) {
    QVariant parameter;
    QWidget* pWidget = m_pWidget->toQWidget();
    QVariant property = pWidget->property(m_propertyName.constData());
    if (property.type() == QVariant::Bool) {
        parameter = getControlParameterForValue(v) > 0;
    } else {
        parameter = getControlParameterForValue(v);
    }

    if (!pWidget->setProperty(m_propertyName.constData(),parameter)) {
        qDebug() << "Setting property" << m_propertyName
                << "to widget failed. Value:" << parameter;
    }
}
