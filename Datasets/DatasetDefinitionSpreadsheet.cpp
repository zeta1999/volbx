#include "DatasetDefinitionSpreadsheet.h"

#include <ImportSpreadsheet.h>
#include <QDebug>

#include "Shared/Logger.h"

DatasetDefinitionSpreadsheet::DatasetDefinitionSpreadsheet(
    const QString& name, const QString& zipFileName)
    : DatasetDefinition(name), zipFile_(zipFileName)
{
}

bool DatasetDefinitionSpreadsheet::init()
{
    importer_->setNameForEmptyColumn(QObject::tr("no name"));
    QObject::connect(importer_.get(),
                     &ImportSpreadsheet::progressPercentChanged, this,
                     &DatasetDefinition::loadingPercentChanged);

    if (!getSheetList() || !loadSpecificData() ||
        !getColumnList(getSheetName()))
        return false;

    columnsCount_ = headerColumnNames_.size();

    if (!getColumnTypes(getSheetName()))
        return false;

    // Sample data.
    sampleData_.resize(SAMPLE_SIZE < rowsCount_ ? SAMPLE_SIZE : rowsCount_);
    if (!getDataFromZip(getSheetName(), &sampleData_, true))
        return false;

    // Set proper strings for sample data.
    updateSampleDataStrings();
    valid_ = true;

    return true;
}

void DatasetDefinitionSpreadsheet::updateSampleDataStrings()
{
    if (stringsMap_.isEmpty())
        return;

    for (int i = 0; i < columnsCount_; ++i)
    {
        if (ColumnType::STRING == columnTypes_.at(i))
        {
            for (auto& sampleDataRow : sampleData_)
            {
                sampleDataRow[i] = stringsMap_.key(sampleDataRow[i].toInt());
            }
        }
    }
}

const QString& DatasetDefinitionSpreadsheet::getSheetName()
{
    return sheetNames_.constFirst();
}

bool DatasetDefinitionSpreadsheet::getSheetList()
{
    bool success{false};
    std::tie(success, sheetNames_) = importer_->getSheetNames();
    if (!success)
        LOG(LogTypes::IMPORT_EXPORT, importXlsx_.getLastError());
    return success;
}

bool DatasetDefinitionSpreadsheet::getColumnList(const QString& sheetName)
{
    bool success{false};
    std::tie(success, headerColumnNames_) =
        importer_->getColumnNames(sheetName);
    if (!success)
        LOG(LogTypes::IMPORT_EXPORT, importXlsx_.getLastError());
    return success;
}

bool DatasetDefinitionSpreadsheet::getColumnTypes(const QString& sheetName)
{
    bool success{false};
    std::tie(success, columnTypes_) = importer_->getColumnTypes(sheetName);
    if (!success)
    {
        LOG(LogTypes::IMPORT_EXPORT, importOds_.getLastError());
        return false;
    }
    rowsCount_ = static_cast<int>(importer_->getRowCount(sheetName).second);
    return true;
}

bool DatasetDefinitionSpreadsheet::isValid() const { return valid_; }

std::unique_ptr<QVariant[]> DatasetDefinitionSpreadsheet::getSharedStringTable()
{
    auto stringsTable = std::make_unique<QVariant[]>(
        static_cast<size_t>(nextSharedStringIndex_));
    QHash<QString, int>::const_iterator i = stringsMap_.constBegin();
    while (i != stringsMap_.constEnd())
    {
        stringsTable[static_cast<size_t>(i.value())] = QVariant(i.key());
        ++i;
    }

    stringsMap_.clear();

    return stringsTable;
}

bool DatasetDefinitionSpreadsheet::getData(
    QVector<QVector<QVariant> >* dataContainer)
{
    const bool result{getDataFromZip(getSheetName(), dataContainer, false)};
    rebuildDefinitonUsingActiveColumnsOnly();
    return result;
}
