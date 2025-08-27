/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AM32EepromFactGroup.h"
#include "Vehicle.h"
#include <QtCore/QDebug>

AM32EepromFactGroup::AM32EepromFactGroup(QObject* parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/AM32EepromFact.json"), parent)
{
    // Add all facts to the group
    _addFact(&_firmwareMajorFact);
    _addFact(&_firmwareMinorFact);
    _addFact(&_bootloaderVersionFact);
    _addFact(&_eepromVersionFact);

    _addFact(&_directionReversedFact);
    _addFact(&_bidirectionalModeFact);
    _addFact(&_sineStartupFact);
    _addFact(&_complementaryPwmFact);
    _addFact(&_variablePwmFreqFact);
    _addFact(&_stuckRotorProtectionFact);
    _addFact(&_brakeOnStopFact);
    _addFact(&_antiStallFact);
    _addFact(&_telemetry30msFact);
    _addFact(&_lowVoltageCutoffFact);
    _addFact(&_rcCarReversingFact);
    _addFact(&_hallSensorsFact);
    _addFact(&_autoTimingFact);

    _addFact(&_maxRampSpeedFact);
    _addFact(&_minDutyCycleFact);
    _addFact(&_timingAdvanceFact);
    _addFact(&_pwmFrequencyFact);
    _addFact(&_startupPowerFact);
    _addFact(&_motorKvFact);
    _addFact(&_motorPolesFact);
    _addFact(&_beepVolumeFact);
    _addFact(&_activeBrakePowerFact);
    _addFact(&_dragBrakeStrengthFact);
    _addFact(&_runningBrakeAmountFact);
    _addFact(&_temperatureLimitFact);
    _addFact(&_currentLimitFact);
    _addFact(&_lowVoltageThresholdFact);
    _addFact(&_sineModeRangeFact);
    _addFact(&_sineModeStrengthFact);
    _addFact(&_inputTypeFact);
    _addFact(&_currentPidPFact);
    _addFact(&_currentPidIFact);
    _addFact(&_currentPidDFact);

    // Map facts to their EEPROM byte indices
    _factToByteIndex[&_maxRampSpeedFact] = BYTE_MAX_RAMP_SPEED;
    _factToByteIndex[&_minDutyCycleFact] = BYTE_MIN_DUTY_CYCLE;
    _factToByteIndex[&_currentPidPFact] = BYTE_CURRENT_PID_P;
    _factToByteIndex[&_currentPidIFact] = BYTE_CURRENT_PID_I;
    _factToByteIndex[&_currentPidDFact] = BYTE_CURRENT_PID_D;
    _factToByteIndex[&_activeBrakePowerFact] = BYTE_ACTIVE_BRAKE_POWER;
    _factToByteIndex[&_directionReversedFact] = BYTE_DIR_REVERSED;
    _factToByteIndex[&_bidirectionalModeFact] = BYTE_BI_DIRECTION;
    _factToByteIndex[&_sineStartupFact] = BYTE_USE_SINE_START;
    _factToByteIndex[&_complementaryPwmFact] = BYTE_COMP_PWM;
    _factToByteIndex[&_variablePwmFreqFact] = BYTE_VARIABLE_PWM;
    _factToByteIndex[&_stuckRotorProtectionFact] = BYTE_STUCK_ROTOR;
    _factToByteIndex[&_timingAdvanceFact] = BYTE_TIMING_ADVANCE;
    _factToByteIndex[&_pwmFrequencyFact] = BYTE_PWM_FREQUENCY;
    _factToByteIndex[&_startupPowerFact] = BYTE_STARTUP_POWER;
    _factToByteIndex[&_motorKvFact] = BYTE_MOTOR_KV;
    _factToByteIndex[&_motorPolesFact] = BYTE_MOTOR_POLES;
    _factToByteIndex[&_brakeOnStopFact] = BYTE_BRAKE_ON_STOP;
    _factToByteIndex[&_antiStallFact] = BYTE_ANTI_STALL;
    _factToByteIndex[&_beepVolumeFact] = BYTE_BEEP_VOLUME;
    _factToByteIndex[&_telemetry30msFact] = BYTE_TELEMETRY_30MS;
    _factToByteIndex[&_lowVoltageCutoffFact] = BYTE_LOW_VOLTAGE_CUTOFF;
    _factToByteIndex[&_lowVoltageThresholdFact] = BYTE_LOW_VOLTAGE_THRESHOLD;
    _factToByteIndex[&_rcCarReversingFact] = BYTE_RC_CAR_REVERSING;
    _factToByteIndex[&_hallSensorsFact] = BYTE_HALL_SENSORS;
    _factToByteIndex[&_sineModeRangeFact] = BYTE_SINE_MODE_RANGE;
    _factToByteIndex[&_dragBrakeStrengthFact] = BYTE_DRAG_BRAKE;
    _factToByteIndex[&_runningBrakeAmountFact] = BYTE_RUNNING_BRAKE;
    _factToByteIndex[&_temperatureLimitFact] = BYTE_TEMP_LIMIT;
    _factToByteIndex[&_currentLimitFact] = BYTE_CURRENT_LIMIT;
    _factToByteIndex[&_sineModeStrengthFact] = BYTE_SINE_MODE_STRENGTH;
    _factToByteIndex[&_inputTypeFact] = BYTE_INPUT_TYPE;
    _factToByteIndex[&_autoTimingFact] = BYTE_AUTO_TIMING;

    // Connect change signals for all editable facts
    _connectFactChangeSignals();
}

void AM32EepromFactGroup::_connectFactChangeSignals()
{
    // Connect all editable facts to change tracking
    for (auto it = _factToByteIndex.begin(); it != _factToByteIndex.end(); ++it) {
        connect(it.key(), &Fact::rawValueChanged, this, [this, fact = it.key()](QVariant value) {
            _factValueChanged(value);

            // Mark the byte(s) this fact affects as modified
            int byteIndex = _factToByteIndex[fact];
            if (byteIndex >= 0) {
                _modifiedBytes.insert(byteIndex);

                // Some facts affect multiple bytes or require special handling
                if (fact == &_lowVoltageThresholdFact && _lowVoltageCutoffFact.rawValue().toBool()) {
                    _modifiedBytes.insert(BYTE_LOW_VOLTAGE_CUTOFF);
                }

                emit hasUnsavedChangesChanged();
            }
        });
    }
}

void AM32EepromFactGroup::_factValueChanged(QVariant value)
{
    Q_UNUSED(value)
    // Additional logic if needed
}

void AM32EepromFactGroup::setEscIndex(int index)
{
    if (_escIndex != index) {
        _escIndex = index;
        emit escIndexChanged();
    }
}

void AM32EepromFactGroup::handleEepromData(const uint8_t* data, int length)
{
    if (length < 48) {
        qWarning() << "AM32 EEPROM data too short:" << length;
        return;
    }

    // Store original data for comparison
    _originalEepromData = QByteArray(reinterpret_cast<const char*>(data), length);

    // Clear modified bytes since we're loading fresh data
    _modifiedBytes.clear();

    // Parse read-only info
    _eepromVersionFact.setRawValue(data[1]);
    _bootloaderVersionFact.setRawValue(data[2]);
    _firmwareMajorFact.setRawValue(data[3]);
    _firmwareMinorFact.setRawValue(data[4]);

    // Parse configurable settings with proper conversions
    _maxRampSpeedFact.setRawValue(data[5] / 10.0);  // value/10 percent per ms
    _minDutyCycleFact.setRawValue(data[6] / 2.0);   // value/2 percent

    // Skip stick calibration (index 7)

    // Voltage and PID settings
    // Note: voltage_cutoff at index 8 is absolute voltage, not used in UI
    _currentPidPFact.setRawValue(data[9] * 2.0);    // P value x2
    _currentPidIFact.setRawValue(data[10]);         // I value
    _currentPidDFact.setRawValue(data[11] * 10.0);  // D value x10
    _activeBrakePowerFact.setRawValue(data[12]);

    // Boolean flags
    _directionReversedFact.setRawValue(data[17] != 0);
    _bidirectionalModeFact.setRawValue(data[18] != 0);
    _sineStartupFact.setRawValue(data[19] != 0);
    _complementaryPwmFact.setRawValue(data[20] != 0);
    _variablePwmFreqFact.setRawValue(data[21] != 0);
    _stuckRotorProtectionFact.setRawValue(data[22] != 0);

    // Timing and power
    if (data[23] < 10) {
        // Old format: 0-3 mapped to 0-24 degrees
        _timingAdvanceFact.setRawValue(data[23] * 8 * 0.9375);
    } else if (data[23] <= 42) {
        // New format: subtract 10 and multiply by 0.9375
        _timingAdvanceFact.setRawValue((data[23] - 10) * 0.9375);
    } else {
        // Default to 15 degrees
        _timingAdvanceFact.setRawValue(15.0);
    }

    _pwmFrequencyFact.setRawValue(data[24]);
    _startupPowerFact.setRawValue(data[25]);
    _motorKvFact.setRawValue(data[26] * 40 + 20);  // KV in increments of 40
    _motorPolesFact.setRawValue(data[27]);
    _brakeOnStopFact.setRawValue(data[28] != 0);
    _antiStallFact.setRawValue(data[29] != 0);
    _beepVolumeFact.setRawValue(data[30]);
    _telemetry30msFact.setRawValue(data[31] != 0);

    // Skip servo settings (32-35)

    // Protection settings
    _lowVoltageCutoffFact.setRawValue(data[36] != 0);
    _lowVoltageThresholdFact.setRawValue((data[37] + 250) / 100.0);  // (value+250)/100 V/cell
    _rcCarReversingFact.setRawValue(data[38] != 0);
    _hallSensorsFact.setRawValue(data[39] != 0);

    // Sine mode and brake settings
    _sineModeRangeFact.setRawValue(data[40]);
    _dragBrakeStrengthFact.setRawValue(data[41]);
    _runningBrakeAmountFact.setRawValue(data[42]);

    // Temperature and current limits
    _temperatureLimitFact.setRawValue(data[43]);  // 141 = disabled
    _currentLimitFact.setRawValue(data[44] == 102 ? 0.0 : data[44] * 2.0);  // value x2, 102 = disabled

    _sineModeStrengthFact.setRawValue(data[45]);
    _inputTypeFact.setRawValue(data[46]);
    _autoTimingFact.setRawValue(data[47] != 0);

    _dataLoaded = true;
    emit dataLoadedChanged();
    emit hasUnsavedChangesChanged();
    emit readComplete(true);
}

void AM32EepromFactGroup::calculateWriteMask(uint32_t writeMask[6]) const
{
    // Initialize mask to all zeros
    memset(writeMask, 0, 6 * sizeof(uint32_t));

    // Set bits only for modified bytes
    for (int byteIndex : _modifiedBytes) {
        if (byteIndex < 192) {
            int maskIndex = byteIndex / 32;
            int bitIndex = byteIndex % 32;
            writeMask[maskIndex] |= (1U << bitIndex);
        }
    }

    // Never write to read-only bytes 0-4
    writeMask[0] &= 0xFFFFFFE0;  // Clear bits 0-4
}

QByteArray AM32EepromFactGroup::packEepromData() const
{
    QByteArray packed(192, 0);  // Initialize with zeros

    // Start byte must be 1
    packed[0] = 1;

    // Read-only values (keep existing)
    packed[1] = _eepromVersionFact.rawValue().toUInt();
    packed[2] = _bootloaderVersionFact.rawValue().toUInt();
    packed[3] = _firmwareMajorFact.rawValue().toUInt();
    packed[4] = _firmwareMinorFact.rawValue().toUInt();

    // Configurable settings with proper conversions
    packed[5] = static_cast<uint8_t>(_maxRampSpeedFact.rawValue().toDouble() * 10);  // %/ms to value/10
    packed[6] = static_cast<uint8_t>(_minDutyCycleFact.rawValue().toDouble() * 2);   // % to value/2
    packed[7] = 0;  // stick_calibration (keep default)
    packed[8] = 10; // voltage_cutoff (keep default)
    packed[9] = static_cast<uint8_t>(_currentPidPFact.rawValue().toDouble() / 2);    // P value / 2
    packed[10] = _currentPidIFact.rawValue().toUInt();
    packed[11] = static_cast<uint8_t>(_currentPidDFact.rawValue().toDouble() / 10);  // D value / 10
    packed[12] = _activeBrakePowerFact.rawValue().toUInt();

    // Reserved bytes 13-16

    // Boolean flags
    packed[17] = _directionReversedFact.rawValue().toBool() ? 1 : 0;
    packed[18] = _bidirectionalModeFact.rawValue().toBool() ? 1 : 0;
    packed[19] = _sineStartupFact.rawValue().toBool() ? 1 : 0;
    packed[20] = _complementaryPwmFact.rawValue().toBool() ? 1 : 0;
    packed[21] = _variablePwmFreqFact.rawValue().toBool() ? 1 : 0;
    packed[22] = _stuckRotorProtectionFact.rawValue().toBool() ? 1 : 0;

    // Timing advance - convert degrees to AM32 format
    double timingDegrees = _timingAdvanceFact.rawValue().toDouble();
    packed[23] = static_cast<uint8_t>((timingDegrees / 0.9375) + 10);  // New format

    packed[24] = _pwmFrequencyFact.rawValue().toUInt();
    packed[25] = _startupPowerFact.rawValue().toUInt();
    packed[26] = static_cast<uint8_t>((_motorKvFact.rawValue().toUInt() - 20) / 40);  // KV to increments
    packed[27] = _motorPolesFact.rawValue().toUInt();
    packed[28] = _brakeOnStopFact.rawValue().toBool() ? 1 : 0;
    packed[29] = _antiStallFact.rawValue().toBool() ? 1 : 0;
    packed[30] = _beepVolumeFact.rawValue().toUInt();
    packed[31] = _telemetry30msFact.rawValue().toBool() ? 1 : 0;

    // Servo settings (keep defaults)
    packed[32] = 0;  // servo_low
    packed[33] = 0;  // servo_high
    packed[34] = 128;  // servo_neutral
    packed[35] = 0;  // servo_deadband

    // Protection settings
    packed[36] = _lowVoltageCutoffFact.rawValue().toBool() ? 1 : 0;
    packed[37] = static_cast<uint8_t>(_lowVoltageThresholdFact.rawValue().toDouble() * 100 - 250);  // V/cell to value
    packed[38] = _rcCarReversingFact.rawValue().toBool() ? 1 : 0;
    packed[39] = _hallSensorsFact.rawValue().toBool() ? 1 : 0;

    // Sine mode and brake settings
    packed[40] = _sineModeRangeFact.rawValue().toUInt();
    packed[41] = _dragBrakeStrengthFact.rawValue().toUInt();
    packed[42] = _runningBrakeAmountFact.rawValue().toUInt();

    // Temperature and current limits
    packed[43] = _temperatureLimitFact.rawValue().toUInt();
    double currentLimit = _currentLimitFact.rawValue().toDouble();
    packed[44] = (currentLimit == 0) ? 102 : static_cast<uint8_t>(currentLimit / 2);  // 0 means disabled (102)

    packed[45] = _sineModeStrengthFact.rawValue().toUInt();
    packed[46] = _inputTypeFact.rawValue().toUInt();
    packed[47] = _autoTimingFact.rawValue().toBool() ? 1 : 0;

    return packed;
}

void AM32EepromFactGroup::requestRead(Vehicle* vehicle)
{
    if (!vehicle) {
        return;
    }

    // Send MAV_CMD_AM32_REQUEST_EEPROM
    vehicle->sendMavCommand(
        vehicle->defaultComponentId(),
        MAV_CMD_AM32_REQUEST_EEPROM,
        false,  // showError
        _escIndex,  // param1: ESC index
        0, 0, 0, 0, 0, 0  // unused params
    );
}

void AM32EepromFactGroup::requestWrite(Vehicle* vehicle)
{
    if (!vehicle || _modifiedBytes.isEmpty()) {
        return;
    }

    // Pack current settings
    QByteArray packedData = packEepromData();

    // Calculate write mask based on modified bytes
    uint32_t writeMask[6];
    calculateWriteMask(writeMask);

    // Log which bytes we're writing
    qDebug() << "Writing AM32 EEPROM bytes:" << _modifiedBytes;
    qDebug() << "Write mask:" << Qt::hex
             << writeMask[0] << writeMask[1] << writeMask[2]
             << writeMask[3] << writeMask[4] << writeMask[5];

    // Send AM32_EEPROM message with write mode
    mavlink_message_t msg;
    mavlink_am32_eeprom_t eeprom;

    eeprom.target_system = vehicle->id();
    eeprom.target_component = vehicle->defaultComponentId();
    eeprom.index = _escIndex;
    eeprom.mode = 1;  // Write mode
    memcpy(eeprom.write_mask, writeMask, sizeof(writeMask));
    eeprom.length = qMin(packedData.size(), (int)sizeof(eeprom.data));
    memcpy(eeprom.data, packedData.data(), eeprom.length);

    SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();

    if (sharedLink) {
        mavlink_msg_am32_eeprom_encode_chan(
            vehicle->id(),
            vehicle->defaultComponentId(),
            sharedLink->mavlinkChannel(),
            &msg,
            &eeprom
        );

        vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}

void AM32EepromFactGroup::markChangesSaved()
{
    _modifiedBytes.clear();
    _originalEepromData = packEepromData();
    emit hasUnsavedChangesChanged();
}

void AM32EepromFactGroup::discardChanges()
{
    if (_originalEepromData.isEmpty() || !_dataLoaded) {
        return;
    }

    // Restore facts from original EEPROM data
    handleEepromData(reinterpret_cast<const uint8_t*>(_originalEepromData.data()),
                     _originalEepromData.size());
}
