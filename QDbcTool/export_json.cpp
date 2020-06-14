#include <QFile>
#include <QTextStream>
#include <QJsonArray>
#include <QJsonObject>

#include "DTObject.h"

void DTObject::exportAsJSON(bool withHiden)
{
    if (!_model)
        return;

    QFile exportFile(_saveFileName);
    exportFile.open(QIODevice::WriteOnly | QIODevice::Truncate);

    QTextStream stream(&exportFile);

    quint32 step = 0;

    QList<QStringList> dbcList = _model->getDbcList();
    emit loadingStart(dbcList.size());

    auto isNeedCloseArray = [&](quint32 pos) {
        quint32 nextIndex = pos + 1;
        return (nextIndex >= _format->getFieldCount() ||
                (_format->getFieldName(nextIndex).chopped(1) !=
                 _format->getFieldName(pos).chopped(1)));
    };

    auto hasNextVisibleField = [&](quint32 pos) {
      for (quint32 i = pos + 1; i < _format->getFieldCount(); ++i)
          if (withHiden || !_format->isHiden(i))
              return true;
      return false;
    };

    auto hasNextVisibleFieldRef = [&](DBCFormat* fmt, quint32 size, quint32 pos) {
      for (quint32 i = pos + 1; i < size; ++i)
          if (withHiden || !fmt->isHiden(i))
              return true;
      return false;
    };

    auto writeToStream = [&](quint32 pos, QStringList dataList, bool arrayExport = false) {
        QString endl = hasNextVisibleField(pos) && !arrayExport ? ", " : "";

        switch (_format->getFieldType(pos))
        {
            case 's':
                if (_format->isCustom(pos))
                    stream << "\"" + _format->getValue(pos) + "\"" + endl;
                else
                    stream << "\"" << escapedString(dataList.at(pos)) << "\"" << endl;
                break;
            case '!':
                break;
            case 'u':
            case 'i':
            case 'f':
            default:
                if (_format->isCustom(pos))
                    stream << _format->getValue(pos) + endl;
                else
                    stream << dataList.at(pos) + endl;
                break;
        }
    };

    QHash<QString, QPair<DTObject*, DBCFormat*>> refs;
    stream << "[ ";
    for (quint32 i = 0; i < _dbc->m_recordCount; i++)
    {
        QStringList dataList = dbcList.at(i);

        bool arrayExport = false;
        QString arrayField = "";
        stream << "{ ";
        for (quint32 j = 0; j < _format->getFieldCount(); j++)
        {
            if (!withHiden && _format->isHiden(j))
                continue;

            if (!arrayExport)
            {
                if (_format->isArrayExport(j))
                {
                    arrayExport = true;
                    arrayField = _format->getFieldName(j).chopped(1);
                    stream << "\"" << arrayField << "\": [ ";
                }
                else
                {
                    stream << "\"" << _format->getFieldName(j) << "\": ";
                }
            }

            QString ref = _format->getFieldRef(j);
            if (!ref.isEmpty())
            {
                if (!refs.contains(ref))
                {
                    QStringList versions = _format->getVersionList(ref);
                    if (versions.contains(_version))
                    {
                        DBCFormat* fmtRef = new DBCFormat(_format->getJson());
                        fmtRef->loadFormat(ref, _version);
                        DTObject* dtRef = new DTObject(nullptr, fmtRef);
                        dtRef->set(_fileName.section('/', 0, 3) + "/" + ref + ".dbc", _version);
                        dtRef->load();
                        refs[ref] = QPair<DTObject*, DBCFormat*>(dtRef, fmtRef);
                    }
                    else if (versions.contains("all"))
                    {
                        DBCFormat* fmtRef = new DBCFormat(_format->getJson());
                        fmtRef->loadFormat(ref, "all");
                        DTObject* dtRef = new DTObject(nullptr, fmtRef);
                        dtRef->set(_fileName.section('/', 0, 3) + "/" + ref + ".dbc", "all");
                        dtRef->load();
                        refs[ref] = QPair<DTObject*, DBCFormat*>(dtRef, fmtRef);
                    }
                }

                DTObject* dtRef = refs[ref].first;
                DBCFormat* fmtRef = refs[ref].second;
                DBCTableModel* modelRef = dtRef->getModel();
                if (modelRef)
                {
                    QString index = dataList.at(j);
                    auto tableRef = modelRef->getDbcList();
                    auto result = std::find_if(tableRef.begin(), tableRef.end(), [index](QStringList l) {
                        return l.at(0) == index;
                    });

                    QStringList dataListRef;
                    if (result != tableRef.end())
                    {
                        stream << "{ ";
                        dataListRef = *result;
                    }
                    else
                    {
                        writeToStream(j, dataList);
                        continue;
                    }

                    for (qint32 k = 0; k < dataListRef.size(); ++k)
                    {
                        if (!withHiden && fmtRef->isHiden(k))
                            continue;

                        stream << "\"" << fmtRef->getFieldName(k) << "\": ";

                        QString endl = hasNextVisibleFieldRef(fmtRef, dataListRef.size(), k) ? ", " : "";
                        switch (fmtRef->getFieldType(k))
                        {
                            case 'u':
                            case 'i':
                            case 'f':
                                stream << dataListRef.at(k) + endl;
                                break;
                            case 's':
                                stream << "\"" + escapedString(dataListRef.at(k)) + "\"" + endl;
                                break;
                            case '!':
                                break;
                            default:
                                stream << dataListRef.at(k) + endl;
                                break;
                        }
                    }
                    stream << " }" << (hasNextVisibleField(j) ? ", " : "");
                }
            }
            else
            {
                writeToStream(j, dataList, arrayExport);
                if (arrayExport) {
                    QString endl = hasNextVisibleField(j) ? ", " : "";
                    if (isNeedCloseArray(j)) {
                        stream << " ]" << endl;
                        arrayExport = false;
                    } else {
                        stream << endl;
                    }
                }
            }
        }

        if (i + 1 == _dbc->m_recordCount) {
            stream << " }\n";
        } else {
            stream << " },\n";
        }

        step++;
        emit loadingStep(step);
    }
    stream << "] ";

    exportFile.close();

    emit loadingNote(QString("Done!"));
}
