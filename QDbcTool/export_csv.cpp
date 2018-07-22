#include <QFile>
#include <QTextStream>

#include "DTObject.h"

void DTObject::exportAsCSV()
{
    if (!_model)
        return;

    QFile exportFile(_saveFileName);
    exportFile.open(QIODevice::WriteOnly | QIODevice::Truncate);

    QTextStream stream(&exportFile);

    quint32 step = 0;

    QStringList fieldNames = _format->getFieldNames();

    for (quint32 f = 0; f < _dbc->m_fieldCount; f++)
        stream << fieldNames.at(f) + ";";

    stream << "\n";

    QList<QStringList> dbcList = _model->getDbcList();
    emit loadingStart(dbcList.size());

    for (quint32 i = 0; i < _dbc->m_recordCount; i++)
    {
        QStringList dataList = dbcList.at(i);

        for (quint32 j = 0; j < _dbc->m_fieldCount; j++)
        {
            if (_format->isHiden(j))
                continue;

            switch (_format->getFieldType(j))
            {
                case 'u':
                case 'i':
                case 'f':
                    stream << dataList.at(j) + ";";
                    break;
                case 's':
                    stream << "\"" + dataList.at(j) + "\";";
                    break;
                default:
                    stream << dataList.at(j) + ";";
                    break;
            }
        }

        stream << "\n";

        step++;
        emit loadingStep(step);
    }

    exportFile.close();

    emit loadingNote(QString("Done!"));
}
