#include "projectsettingswidget.h"

#include <QTabWidget>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>
#include <QPushButton>
#include <QStandardPaths>

ProjectSettingsWidget::ProjectSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    auto mainLayout = new QVBoxLayout(this);

    m_tabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_tabWidget);

    // Load schema.json from config folder (same as AppConfig)
    QString configFolder = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString schemaPath = QDir(configFolder).filePath("schema.json");
    QFile schemaFile(schemaPath);
    if (!schemaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "ProjectSettingsWidget: Failed to open schema.json at" << schemaPath;
        return;
    }
    QByteArray schemaData = schemaFile.readAll();
    schemaFile.close();

    QJsonParseError parseError;
    QJsonDocument schemaDoc = QJsonDocument::fromJson(schemaData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "ProjectSettingsWidget: Failed to parse schema.json:" << parseError.errorString();
        return;
    }
    if (!schemaDoc.isObject()) {
        qWarning() << "ProjectSettingsWidget: schema.json root is not an object";
        return;
    }
    QJsonObject rootObj = schemaDoc.object();

    if (!rootObj.contains("default_project_settings") || !rootObj.value("default_project_settings").isObject()) {
        qWarning() << "ProjectSettingsWidget: schema.json missing 'default_project_settings' object";
        return;
    }

    m_schemaDefaultProjectSettings = rootObj.value("default_project_settings").toVariant().toMap();

    // Extract the 'properties' map inside default_project_settings
    QVariantMap defaultProjectSettingsProperties;
    if (m_schemaDefaultProjectSettings.contains("properties") &&
        m_schemaDefaultProjectSettings.value("properties").canConvert<QVariantMap>()) {
        defaultProjectSettingsProperties = m_schemaDefaultProjectSettings.value("properties").toMap();
    } else {
        qWarning() << "default_project_settings schema missing 'properties' key or not a map";
        return;
    }

    // Build tabs dynamically for each top-level property in default_project_settings
    for (auto it = defaultProjectSettingsProperties.constBegin(); it != defaultProjectSettingsProperties.constEnd(); ++it) {
        QString tabKey = it.key();
        QVariantMap tabSchema = it.value().toMap();

        // Special case: command_pipes tab will be hard-coded
        if (tabKey == "command_pipes") {
            QWidget *cmdTab = createCommandPipesTab();
            m_tabWidget->addTab(cmdTab, "Command Pipes");
            continue;
        }

        QWidget *tabWidget = buildFormForSchema(tabSchema, tabKey);
        m_tabWidget->addTab(tabWidget, prettifyKey(tabKey));
    }
}

QWidget* ProjectSettingsWidget::buildFormForSchema(const QVariantMap &schemaObj, const QString &parentKey)
{
    QWidget *container = new QWidget(this);
    QFormLayout *formLayout = new QFormLayout(container);

    // schemaObj should have "properties" key with object
    if (!schemaObj.contains("properties") || !schemaObj.value("properties").canConvert<QVariantMap>()) {
        qDebug() << "Schema object missing 'properties' or not a map";
        return container;
    }

    QVariantMap properties = schemaObj.value("properties").toMap();

    populateFormLayout(formLayout, properties, parentKey);

    return container;
}

void ProjectSettingsWidget::populateFormLayout(QFormLayout *formLayout, const QVariantMap &properties, const QString &parentKey)
{
    for (auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
        QString key = it.key();
        QVariantMap propSchema = it.value().toMap();

        QString fullKey = parentKey.isEmpty() ? key : parentKey + "." + key;

        QString type = propSchema.value("type").toString();

        if (type == "object") {
            // Recursively create a group box or nested form
            QWidget *subForm = buildFormForSchema(propSchema, fullKey);
            formLayout->addRow(prettifyKey(key), subForm);
        } else if (type == "array") {
            // For arrays of strings, use QLineEdit with comma separated values
            if (propSchema.contains("items") && propSchema.value("items").toMap().value("type").toString() == "string") {
                QLineEdit *lineEdit = new QLineEdit(this);
                lineEdit->setPlaceholderText("Comma separated values");
                formLayout->addRow(prettifyKey(key), lineEdit);

                PropertyInfo info;
                info.key = fullKey;
                info.type = type;
                info.defaultValue = propSchema.value("default");
                info.widget = lineEdit;
                m_properties.insert(fullKey, info);
            } else {
                // Unsupported array type - skip or implement later
                QLabel *label = new QLabel("<unsupported array type>", this);
                formLayout->addRow(prettifyKey(key), label);
            }
        } else {
            QWidget *w = createWidgetForProperty(fullKey, propSchema);
            if (w) {
                formLayout->addRow(prettifyKey(key), w);

                PropertyInfo info;
                info.key = fullKey;
                info.type = type;
                info.defaultValue = propSchema.value("default");
                info.widget = w;
                m_properties.insert(fullKey, info);
            }
        }
    }
}

QWidget* ProjectSettingsWidget::createWidgetForProperty(const QString &key, const QVariantMap &propSchema)
{
    QString type = propSchema.value("type").toString();

    if (type == "string") {
        QLineEdit *lineEdit = new QLineEdit(this);
        if (propSchema.contains("default")) {
            lineEdit->setPlaceholderText(QString("Default: %1").arg(propSchema.value("default").toString()));
        }
        return lineEdit;
    } else if (type == "integer") {
        QLineEdit *lineEdit = new QLineEdit(this);
        lineEdit->setValidator(new QIntValidator(this));
        if (propSchema.contains("default")) {
            lineEdit->setPlaceholderText(QString("Default: %1").arg(propSchema.value("default").toInt()));
        }
        return lineEdit;
    } else if (type == "number") {
        QLineEdit *lineEdit = new QLineEdit(this);
        lineEdit->setValidator(new QDoubleValidator(this));
        if (propSchema.contains("default")) {
            lineEdit->setPlaceholderText(QString("Default: %1").arg(propSchema.value("default").toDouble()));
        }
        return lineEdit;
    } else if (type == "boolean") {
        QCheckBox *checkBox = new QCheckBox(this);
        // For booleans, default is tri-state: unchecked means inherit
        checkBox->setTristate(false);
        return checkBox;
    }

    // Unsupported type
    return nullptr;
}

QString ProjectSettingsWidget::prettifyKey(const QString &key) const
{
    QString s = key;
    s.replace('_', ' ');
    s[0] = s[0].toUpper();
    return s;
}

void ProjectSettingsWidget::loadSettings(const QVariantMap &settings)
{
    // Clear all widgets first
    for (auto &info : m_properties) {
        if (!info.widget)
            continue;

        if (QLineEdit *le = qobject_cast<QLineEdit*>(info.widget)) {
            le->clear();
        } else if (QCheckBox *cb = qobject_cast<QCheckBox*>(info.widget)) {
            cb->setCheckState(Qt::Unchecked);
        }
    }

    // Load values recursively
    std::function<void(const QVariantMap &, const QString &)> loadRec;
    loadRec = [&](const QVariantMap &map, const QString &parentKey) {
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            QString key = it.key();
            QVariant val = it.value();

            QString fullKey = parentKey.isEmpty() ? key : parentKey + "." + key;

            if (m_properties.contains(fullKey)) {
                PropertyInfo &info = m_properties[fullKey];
                if (!info.widget)
                    continue;

                if (QLineEdit *le = qobject_cast<QLineEdit*>(info.widget)) {
                    if (info.type == "array") {
                        // array of strings stored as QStringList or QVariantList
                        QStringList list;
                        if (val.type() == QVariant::StringList) {
                            list = val.toStringList();
                        } else if (val.canConvert<QVariantList>()) {
                            QVariantList vlist = val.toList();
                            for (const QVariant &v : vlist)
                                list.append(v.toString());
                        } else if (val.type() == QVariant::String) {
                            list = val.toString().split(',', Qt::SkipEmptyParts);
                        }
                        le->setText(list.join(", "));
                    } else {
                        le->setText(val.toString());
                    }
                } else if (QCheckBox *cb = qobject_cast<QCheckBox*>(info.widget)) {
                    if (val.type() == QVariant::Bool) {
                        cb->setChecked(val.toBool());
                    } else {
                        cb->setCheckState(Qt::Unchecked);
                    }
                }
            } else {
                // Possibly nested object, recurse if val is map
                if (val.type() == QVariant::Map) {
                    loadRec(val.toMap(), fullKey);
                }
            }
        }
    };

    loadRec(settings, QString());

    // Load command pipes separately
    if (settings.contains("command_pipes")) {
        QVariantMap cpMap = settings.value("command_pipes").toMap();
        loadCommandPipes(cpMap);
    }
}

QVariantMap ProjectSettingsWidget::getSettings() const
{
    QVariantMap result = extractValues(QString());

    // Insert command pipes separately
    QVariantMap cpMap = getCommandPipes();
    if (!cpMap.isEmpty())
        result.insert("command_pipes", cpMap);

    return result;
}

QVariantMap ProjectSettingsWidget::extractValues(const QString &parentKey) const
{
    QVariantMap map;

    for (auto it = m_properties.constBegin(); it != m_properties.constEnd(); ++it) {
        const PropertyInfo &info = it.value();

        if (!info.widget)
            continue;

        if (!info.key.startsWith(parentKey) && !parentKey.isEmpty())
            continue;

        QString relativeKey = info.key;
        if (!parentKey.isEmpty()) {
            if (!info.key.startsWith(parentKey + "."))
                continue;
            relativeKey = info.key.mid(parentKey.length() + 1);
        }

        if (relativeKey.contains('.'))
            continue; // only top-level keys in this call

        QVariant value;

        if (QLineEdit *le = qobject_cast<QLineEdit*>(info.widget)) {
            QString text = le->text().trimmed();
            if (text.isEmpty())
                continue; // omit empty to inherit

            if (info.type == "array") {
                QStringList list = text.split(',', Qt::SkipEmptyParts);
                for (QString &s : list)
                    s = s.trimmed();
                value = QVariant::fromValue(list);
            } else if (info.type == "integer") {
                bool ok = false;
                int v = text.toInt(&ok);
                if (!ok)
                    continue;
                value = v;
            } else if (info.type == "number") {
                bool ok = false;
                double v = text.toDouble(&ok);
                if (!ok)
                    continue;
                value = v;
            } else {
                value = text;
            }
        } else if (QCheckBox *cb = qobject_cast<QCheckBox*>(info.widget)) {
            if (cb->checkState() == Qt::Unchecked)
                continue; // unchecked means inherit
            value = cb->isChecked();
        }

        if (!value.isValid())
            continue;

        map.insert(relativeKey, value);
    }

    // Recursively build nested objects
    // Find keys with dots and build nested maps
    QVariantMap nestedMap;

    for (auto it = map.begin(); it != map.end();) {
        QString key = it.key();
        if (key.contains('.')) {
            QStringList parts = key.split('.');
            QString first = parts.takeFirst();
            QString rest = parts.join('.');

            QVariantMap subMap = nestedMap.value(first).toMap();
            subMap.insert(rest, it.value());
            nestedMap.insert(first, subMap);

            it = map.erase(it);
        } else {
            ++it;
        }
    }

    // Merge nestedMap into map
    for (auto it = nestedMap.constBegin(); it != nestedMap.constEnd(); ++it) {
        map.insert(it.key(), it.value());
    }

    return map;
}

QWidget* ProjectSettingsWidget::createCommandPipesTab()
{
    QWidget *tab = new QWidget(this);
    auto layout = new QVBoxLayout(tab);

    m_commandPipesTable = new QTableWidget(tab);
    m_commandPipesTable->setColumnCount(2);
    m_commandPipesTable->setHorizontalHeaderLabels(QStringList() << "Name" << "Command(s)");
    m_commandPipesTable->horizontalHeader()->setStretchLastSection(true);
    m_commandPipesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_commandPipesTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    layout->addWidget(m_commandPipesTable);

    // Buttons for add/remove
    auto btnLayout = new QHBoxLayout();
    QPushButton *addBtn = new QPushButton("Add", tab);
    QPushButton *removeBtn = new QPushButton("Remove", tab);
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(removeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(addBtn, &QPushButton::clicked, this, [this]() {
        int newRow = m_commandPipesTable->rowCount();
        m_commandPipesTable->insertRow(newRow);
        m_commandPipesTable->setItem(newRow, 0, new QTableWidgetItem("new_command"));
        m_commandPipesTable->setItem(newRow, 1, new QTableWidgetItem("command args"));
        m_commandPipesTable->editItem(m_commandPipesTable->item(newRow, 0));
    });

    connect(removeBtn, &QPushButton::clicked, this, [this]() {
        auto selectedRanges = m_commandPipesTable->selectedRanges();
        if (selectedRanges.isEmpty())
            return;
        for (int i = selectedRanges.size() - 1; i >= 0; --i) {
            int topRow = selectedRanges.at(i).topRow();
            int bottomRow = selectedRanges.at(i).bottomRow();
            for (int row = bottomRow; row >= topRow; --row) {
                m_commandPipesTable->removeRow(row);
            }
        }
    });

    return tab;
}

void ProjectSettingsWidget::loadCommandPipes(const QVariantMap &commandPipes)
{
    if (!m_commandPipesTable)
        return;

    m_commandPipesTable->clearContents();
    m_commandPipesTable->setRowCount(commandPipes.size());

    int row = 0;
    for (auto it = commandPipes.constBegin(); it != commandPipes.constEnd(); ++it, ++row) {
        QString name = it.key();
        QStringList cmds;
        if (it.value().type() == QVariant::StringList) {
            cmds = it.value().toStringList();
        } else if (it.value().type() == QVariant::List) {
            QVariantList vlist = it.value().toList();
            for (const QVariant &v : vlist)
                cmds.append(v.toString());
        }

        QTableWidgetItem *nameItem = new QTableWidgetItem(name);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable); // name not editable for now
        m_commandPipesTable->setItem(row, 0, nameItem);

        QString cmdStr = cmds.join(" | ");
        QTableWidgetItem *cmdItem = new QTableWidgetItem(cmdStr);
        m_commandPipesTable->setItem(row, 1, cmdItem);
    }
}

QVariantMap ProjectSettingsWidget::getCommandPipes() const
{
    QVariantMap result;
    if (!m_commandPipesTable)
        return result;

    int rowCount = m_commandPipesTable->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        QTableWidgetItem *nameItem = m_commandPipesTable->item(row, 0);
        QTableWidgetItem *cmdItem = m_commandPipesTable->item(row, 1);
        if (!nameItem || !cmdItem)
            continue;

        QString name = nameItem->text().trimmed();
        QString cmdStr = cmdItem->text().trimmed();
        QStringList cmds = cmdStr.split('|', Qt::SkipEmptyParts);
        for (QString &s : cmds)
            s = s.trimmed();

        if (!name.isEmpty() && !cmds.isEmpty())
            result.insert(name, cmds);
    }
    return result;
}
