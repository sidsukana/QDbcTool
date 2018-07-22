#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include "DTObject.h"

void DTObject::exportAsSQL()
{
    if (!_model)
        return;

    QFileInfo finfo(_fileName);

    QFile exportFile(_saveFileName);
    exportFile.open(QIODevice::WriteOnly | QIODevice::Truncate);

    QTextStream stream(&exportFile);

    emit loadingStart(_dbc->m_fieldCount + _dbc->m_recordCount);
    quint32 step = 0;

    QStringList fieldNames = _format->getFieldNames();

    auto hasNextVisibleField = [this](quint32 pos) {
      for (quint32 i = pos + 1; i < _dbc->m_fieldCount; ++i)
          if (!_format->isHiden(i))
              return true;
      return false;
    };

    stream << "CREATE TABLE `" + finfo.baseName() + "_dbc` (\n";
    for (quint32 i = 0; i < _dbc->m_fieldCount; i++)
    {
        if (_format->isHiden(i))
            continue;

        QString endl = hasNextVisibleField(i) ? ",\n" : "\n";
        switch (_format->getFieldType(i))
        {
            case 'u':
            case 'i':
            case 'l':
                stream << "\t`" + fieldNames.at(i) + "` bigint(20) NOT NULL default '0'" + endl;
                break;
            case 'f':
                stream << "\t`" + fieldNames.at(i) + "` float NOT NULL default '0'" + endl;
                break;
            case 's':
                stream << "\t`" + fieldNames.at(i) + "` text NOT NULL" + endl;
                break;
            case '!':
                break;
            default:
                stream << "\t`" + fieldNames.at(i) + "` bigint(20) NOT NULL default '0'" + endl;
                break;
        }
        step++;
        emit loadingStep(step);
    }
    stream << ") ENGINE = MyISAM DEFAULT CHARSET = utf8 COMMENT = 'Data from " + finfo.fileName() + "';\n\n";

    QList<QStringList> dbcList = _model->getDbcList();
    for (quint32 i = 0; i < _dbc->m_recordCount; i++)
    {
        stream << "INSERT INTO `" + finfo.baseName() + "_dbc` (";
        for (quint32 f = 0; f < _dbc->m_fieldCount; f++)
        {
            if (_format->isHiden(f))
                continue;

            QString endl = hasNextVisibleField(f) ? "`, " : "`) VALUES (";
            stream << "`" + fieldNames.at(f) + endl;
        }
        QStringList dataList = dbcList.at(i);

        for (quint32 d = 0; d < _dbc->m_fieldCount; d++)
        {
            if (_format->isHiden(d))
                continue;

            if (dataList.at(d).contains("'"))
            {
                QString data = dataList.at(d);
                data.replace("'", "\\'");
                dataList.replace(d, data);
            }
        }

        for (quint32 j = 0; j < _dbc->m_fieldCount; j++)
        {
            if (_format->isHiden(j))
                continue;

            QString endl = hasNextVisibleField(j) ? "', " : "');\n";
            stream << "'" + dataList.at(j) + endl;
        }
        step++;
        emit loadingStep(step);
    }

    exportFile.close();

    emit loadingNote(QString("Done!"));
}
