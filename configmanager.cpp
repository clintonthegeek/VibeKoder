#include "configmanager.h"

ConfigManager::ConfigManager(const QString &configFilePath,
                             const QString &schemaFilePath,
                             QObject *parent)
    : QObject(parent)
    , m_configFilePath(configFilePath)
    , m_schemaFilePath(schemaFilePath)
{
}

bool ConfigManager::load()
{
    // Load schema file
    QFile schemaFile(m_schemaFilePath);
    if (!schemaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open schema file:" << m_schemaFilePath;
        return false;
    }
    QByteArray schemaData = schemaFile.readAll();
    schemaFile.close();

    QJsonParseError schemaParseError;
    QJsonDocument schemaDoc = QJsonDocument::fromJson(schemaData, &schemaParseError);
    if (schemaParseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse schema JSON:" << schemaParseError.errorString();
        return false;
    }
    if (!schemaDoc.isObject()) {
        qWarning() << "Schema JSON root is not an object";
        return false;
    }
    m_schemaObject = schemaDoc.object();

    // Load config file
    QFile configFile(m_configFilePath);
    if (!configFile.exists()) {
        // Config file does not exist, create empty config object
        m_configObject = QJsonObject();
    } else {
        if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Failed to open config file:" << m_configFilePath;
            return false;
        }
        QByteArray configData = configFile.readAll();
        configFile.close();

        QJsonParseError configParseError;
        QJsonDocument configDoc = QJsonDocument::fromJson(configData, &configParseError);
        if (configParseError.error != QJsonParseError::NoError) {
            qWarning() << "Failed to parse config JSON:" << configParseError.errorString();
            return false;
        }
        if (!configDoc.isObject()) {
            qWarning() << "Config JSON root is not an object";
            return false;
        }
        m_configObject = configDoc.object();
    }

    // Apply defaults from schema to config
    applyDefaults(m_configObject, m_schemaObject);

    emit configChanged();
    return true;
}

bool ConfigManager::save() const
{
    QFile configFile(m_configFilePath);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open config file for writing:" << m_configFilePath;
        return false;
    }

    QJsonDocument doc(m_configObject);
    configFile.write(doc.toJson(QJsonDocument::Indented));
    configFile.close();

    return true;
}

QVariant ConfigManager::getValue(const QString &keyPath, const QVariant &defaultValue) const
{
    QStringList keys = keyPath.split('.', Qt::SkipEmptyParts);
    if (keys.isEmpty())
        return defaultValue;

    QVariant val = getValueRecursive(m_configObject, keys, 0);
    if (!val.isValid())
        return defaultValue;
    return val;
}

void ConfigManager::setValue(const QString &keyPath, const QVariant &value)
{
    QStringList keys = keyPath.split('.', Qt::SkipEmptyParts);
    if (keys.isEmpty())
        return;

    setValueRecursive(m_configObject, keys, 0, value);
    emit configChanged();
}

QVariant ConfigManager::getValueRecursive(const QJsonObject &obj, const QStringList &keys, int index) const
{
    if (index >= keys.size())
        return QVariant();

    QString key = keys[index];
    if (!obj.contains(key))
        return QVariant();

    QJsonValue val = obj.value(key);
    if (index == keys.size() - 1) {
        return jsonValueToVariant(val);
    }

    if (!val.isObject())
        return QVariant();

    return getValueRecursive(val.toObject(), keys, index + 1);
}

void ConfigManager::setValueRecursive(QJsonObject &obj, const QStringList &keys, int index, const QVariant &value)
{
    if (index >= keys.size())
        return;

    QString key = keys[index];
    if (index == keys.size() - 1) {
        obj[key] = variantToJsonValue(value);
        return;
    }

    QJsonObject childObj;
    if (obj.contains(key) && obj.value(key).isObject()) {
        childObj = obj.value(key).toObject();
    }

    setValueRecursive(childObj, keys, index + 1, value);
    obj[key] = childObj;
}

void ConfigManager::applyDefaults(QJsonObject &obj, const QJsonObject &schema)
{
    // Schema must have "properties" object for this level
    if (!schema.contains("properties") || !schema.value("properties").isObject())
        return;

    QJsonObject props = schema.value("properties").toObject();

    for (const QString &key : props.keys()) {
        QJsonObject propSchema = props.value(key).toObject();

        if (!obj.contains(key)) {
            // If default exists in schema, set it
            if (propSchema.contains("default")) {
                obj[key] = propSchema.value("default");
            } else if (propSchema.value("type") == "object") {
                // Create empty object and recurse
                QJsonObject childObj;
                applyDefaults(childObj, propSchema);
                obj[key] = childObj;
            }
            // else leave missing
        } else {
            // Key exists, if object type recurse
            if (propSchema.value("type") == "object" && obj.value(key).isObject()) {
                QJsonObject childObj = obj.value(key).toObject();
                applyDefaults(childObj, propSchema);
                obj[key] = childObj;
            }
        }
    }
}

bool ConfigManager::validate() const
{
    QString errorMsg;
    return validateRecursive(m_configObject, m_schemaObject, &errorMsg);
}

bool ConfigManager::validateRecursive(const QJsonObject &obj, const QJsonObject &schema, QString *errorMsg) const
{
    if (!schema.contains("properties") || !schema.value("properties").isObject())
        return true; // nothing to validate here

    QJsonObject props = schema.value("properties").toObject();

    for (const QString &key : props.keys()) {
        QJsonObject propSchema = props.value(key).toObject();

        if (!obj.contains(key)) {
            if (propSchema.contains("default")) {
                // missing key but default exists, OK
                continue;
            } else {
                if (errorMsg)
                    *errorMsg = QString("Missing required key: %1").arg(key);
                return false;
            }
        }

        QJsonValue val = obj.value(key);

        QString type = propSchema.value("type").toString();

        if (type == "object") {
            if (!val.isObject()) {
                if (errorMsg)
                    *errorMsg = QString("Key '%1' expected to be object").arg(key);
                return false;
            }
            if (!validateRecursive(val.toObject(), propSchema, errorMsg))
                return false;
        } else if (type == "array") {
            if (!val.isArray()) {
                if (errorMsg)
                    *errorMsg = QString("Key '%1' expected to be array").arg(key);
                return false;
            }
            // Could add item type validation here if desired
        } else if (type == "string") {
            if (!val.isString()) {
                if (errorMsg)
                    *errorMsg = QString("Key '%1' expected to be string").arg(key);
                return false;
            }
        } else if (type == "integer") {
            if (!val.isDouble() || val.toDouble() != val.toInt()) {
                if (errorMsg)
                    *errorMsg = QString("Key '%1' expected to be integer").arg(key);
                return false;
            }
        } else if (type == "number") {
            if (!val.isDouble()) {
                if (errorMsg)
                    *errorMsg = QString("Key '%1' expected to be number").arg(key);
                return false;
            }
        } else if (type == "boolean") {
            if (!val.isBool()) {
                if (errorMsg)
                    *errorMsg = QString("Key '%1' expected to be boolean").arg(key);
                return false;
            }
        }
        // else unknown type: skip validation
    }

    return true;
}

QJsonValue ConfigManager::variantToJsonValue(const QVariant &var) const
{
    switch (var.type()) {
    case QMetaType::Bool:
        return QJsonValue(var.toBool());
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Double:
        return QJsonValue(var.toDouble());
    case QMetaType::QString:
        return QJsonValue(var.toString());
    case QMetaType::QVariantList: {
        QJsonArray arr;
        for (const QVariant &v : var.toList()) {
            arr.append(variantToJsonValue(v));
        }
        return arr;
    }
    case QMetaType::QVariantMap: {
        QJsonObject obj;
        QVariantMap map = var.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            obj.insert(it.key(), variantToJsonValue(it.value()));
        }
        return obj;
    }
    default:
        // fallback to string
        return QJsonValue(var.toString());
    }
}

QVariant ConfigManager::jsonValueToVariant(const QJsonValue &val) const
{
    if (val.isBool())
        return QVariant(val.toBool());
    if (val.isDouble())
        return QVariant(val.toDouble());
    if (val.isString())
        return QVariant(val.toString());
    if (val.isArray()) {
        QVariantList list;
        for (const QJsonValue &v : val.toArray()) {
            list.append(jsonValueToVariant(v));
        }
        return list;
    }
    if (val.isObject()) {
        QVariantMap map;
        QJsonObject obj = val.toObject();
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            map.insert(it.key(), jsonValueToVariant(it.value()));
        }
        return map;
    }
    return QVariant();
}
