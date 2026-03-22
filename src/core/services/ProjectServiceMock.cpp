#include "ProjectServiceMock.h"

#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QVariant>
#include <QSet>
#include <QRegularExpression>
#include <QImage>
#include <QPainter>
#include <QBuffer>
#include <algorithm>
#include <QMetaObject>
#include <QPointer>
#include <QtConcurrent/QtConcurrentRun>
#include <limits>
#include <cstdint>
#include <unordered_set>
#include <QtMath>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Model.hpp>
#include <libslic3r/ModelObject.hpp>
#include <libslic3r/ModelVolume.hpp>
#include <libslic3r/ModelInstance.hpp>
#include <libslic3r/TriangleMesh.hpp>
#include <libslic3r/QuadricEdgeCollapse.hpp>
#include <libslic3r/Format/3mf.hpp>
#include <libslic3r/Format/bbs_3mf.hpp>
#include <libslic3r/PrintConfig.hpp>
#include <libslic3r/Emboss.hpp>
#include <libslic3r/TextConfiguration.hpp>
#include <libslic3r/Orient.hpp>
#include <libslic3r/ModelArrange.hpp>
#include <libslic3r/TriangleMeshSlicer.hpp>
#include <libslic3r/MeshBoolean.hpp>
#include <libslic3r/Geometry.hpp>
#include <libslic3r/CutUtils.hpp>
#include <cstring> // memcpy

namespace
{
  using ScopedConfig = Slic3r::ModelConfig;

  QString importStageToText(int stage)
  {
    switch (stage)
    {
    case 0:
      return QObject::tr("恢复项目");
    case 1:
      return QObject::tr("打开文件");
    case 2:
      return QObject::tr("读取数据");
    case 3:
      return QObject::tr("解压内容");
    case 4:
      return QObject::tr("加载模型");
    case 5:
      return QObject::tr("加载平台");
    case 6:
      return QObject::tr("完成导入");
    case 7:
      return QObject::tr("创建实例");
    case 8:
      return QObject::tr("更新 G-code");
    case 9:
      return QObject::tr("检查模式");
    default:
      return QObject::tr("处理文件");
    }
  }

  QString volumeTypeToLabel(const Slic3r::ModelVolume *volume)
  {
    if (!volume)
      return {};
    if (volume->is_model_part())
      return QObject::tr("模型部件");
    if (volume->is_negative_volume())
      return QObject::tr("负体积");
    if (volume->is_modifier())
      return QObject::tr("参数修改器");
    if (volume->is_support_enforcer())
      return QObject::tr("支撑增强");
    if (volume->is_support_blocker())
      return QObject::tr("支撑屏蔽");
    return QObject::tr("体积");
  }

  std::unique_ptr<Slic3r::DynamicPrintConfig> defaultConfigForKey(const std::string &key)
  {
    return std::unique_ptr<Slic3r::DynamicPrintConfig>(Slic3r::DynamicPrintConfig::new_from_defaults_keys({key}));
  }

  const ScopedConfig *scopedConfigForRead(const Slic3r::Model *model, int objectIndex, int volumeIndex)
  {
    if (!model || objectIndex < 0 || size_t(objectIndex) >= model->objects.size())
      return nullptr;

    const auto *object = model->objects[size_t(objectIndex)];
    if (!object)
      return nullptr;

    if (volumeIndex >= 0)
    {
      if (size_t(volumeIndex) >= object->volumes.size() || !object->volumes[size_t(volumeIndex)])
        return nullptr;
      return &object->volumes[size_t(volumeIndex)]->config;
    }

    return &object->config;
  }

  ScopedConfig *scopedConfigForWrite(Slic3r::Model *model, int objectIndex, int volumeIndex)
  {
    if (!model || objectIndex < 0 || size_t(objectIndex) >= model->objects.size())
      return nullptr;

    auto *object = model->objects[size_t(objectIndex)];
    if (!object)
      return nullptr;

    if (volumeIndex >= 0)
    {
      if (size_t(volumeIndex) >= object->volumes.size() || !object->volumes[size_t(volumeIndex)])
        return nullptr;
      return &object->volumes[size_t(volumeIndex)]->config;
    }

    return &object->config;
  }

  QString resolvedBedTempKey(const ScopedConfig *config)
  {
    Slic3r::BedType bedType = Slic3r::btPEI;
    if (config && config->option("curr_bed_type"))
      bedType = config->get().opt_enum<Slic3r::BedType>("curr_bed_type");
    return QString::fromStdString(Slic3r::get_bed_temp_key(bedType));
  }

  QVariant readConfigValue(const ScopedConfig *config, const QString &key, const QVariant &fallbackValue)
  {
    if (!config)
      return fallbackValue;

    if (key == QStringLiteral("brim_enable"))
    {
      const auto *brimType = config->option("brim_type");
      const auto *brimWidth = config->option("brim_width");
      if (!brimType || !brimWidth)
        return fallbackValue;
      return brimType->getInt() != int(Slic3r::BrimType::btNoBrim) && brimWidth->getFloat() > 0.0;
    }

    if (key == QStringLiteral("nozzle_temp"))
    {
      const auto *option = config->get().option<Slic3r::ConfigOptionInts>("nozzle_temperature");
      return option && !option->values.empty() ? QVariant(option->get_at(0)) : fallbackValue;
    }

    if (key == QStringLiteral("bed_temp"))
    {
      const QString bedKey = resolvedBedTempKey(config);
      const auto *option = config->get().option<Slic3r::ConfigOptionInts>(bedKey.toStdString());
      return option && !option->values.empty() ? QVariant(option->get_at(0)) : fallbackValue;
    }

    if (key == QStringLiteral("chamber_temperature"))
    {
      const auto *option = config->get().option<Slic3r::ConfigOptionInts>("chamber_temperature");
      return option && !option->values.empty() ? QVariant(option->get_at(0)) : fallbackValue;
    }

    const std::string stdKey = key.toStdString();
    const auto *option = config->option(stdKey);
    if (!option)
      return fallbackValue;

    // Use the option's actual type to determine how to read (对齐上游 ConfigOptionType)
    switch (option->type())
    {
    case Slic3r::coFloat:
    case Slic3r::coPercent:
    case Slic3r::coFloatOrPercent:
      return option->getFloat();
    case Slic3r::coInt:
      return option->getInt();
    case Slic3r::coBool:
      return option->getBool();
    case Slic3r::coString:
      return QString::fromStdString(static_cast<const Slic3r::ConfigOptionString *>(option)->value);
    case Slic3r::coEnum:
    case Slic3r::coEnums:
    {
      const int intVal = option->getInt();
      // Try to find the enum label from print_config_def
      const auto *def = Slic3r::print_config_def.get(stdKey);
      if (def && intVal >= 0 && size_t(intVal) < def->enum_labels.size())
        return QString::fromStdString(def->enum_labels[intVal]);
      return intVal;
    }
    default:
      return option->getInt();
    }
  }

  bool writeConfigValue(ScopedConfig *config, const QString &key, const QVariant &value)
  {
    if (!config)
      return false;

    if (key == QStringLiteral("brim_enable"))
    {
      if (value.toBool())
      {
        config->set_key_value("brim_type", new Slic3r::ConfigOptionEnum<Slic3r::BrimType>(Slic3r::BrimType::btOuterOnly));
        if (!config->option("brim_width"))
          config->set_key_value("brim_width", new Slic3r::ConfigOptionFloat(5.0));
      }
      else
      {
        config->set_key_value("brim_type", new Slic3r::ConfigOptionEnum<Slic3r::BrimType>(Slic3r::BrimType::btNoBrim));
      }
      return true;
    }

    if (key == QStringLiteral("nozzle_temp"))
    {
      config->set_key_value("nozzle_temperature", new Slic3r::ConfigOptionInts(1, value.toInt()));
      return true;
    }

    if (key == QStringLiteral("bed_temp"))
    {
      const QString bedKey = resolvedBedTempKey(config);
      if (bedKey.isEmpty())
        return false;
      config->set_key_value(bedKey.toStdString(), new Slic3r::ConfigOptionInts(1, value.toInt()));
      return true;
    }

    if (key == QStringLiteral("chamber_temperature"))
    {
      config->set_key_value("chamber_temperature", new Slic3r::ConfigOptionInts(1, value.toInt()));
      return true;
    }

    const std::string stdKey = key.toStdString();
    auto defaults = defaultConfigForKey(stdKey);
    if (!defaults)
      return false;

    const auto *defaultOption = defaults->option(stdKey);
    if (!defaultOption)
      return false;

    std::unique_ptr<Slic3r::ConfigOption> next(defaultOption->clone());

    // Handle string values (enum labels from QML combobox, or string options)
    if (value.typeId() == QMetaType::QString && !value.toString().isEmpty())
    {
      const std::string strVal = value.toString().toStdString();
      // Check if this is an enum — convert label back to int index
      const auto *def = Slic3r::print_config_def.get(stdKey);
      if (def && !def->enum_labels.empty())
      {
        for (size_t ei = 0; ei < def->enum_labels.size(); ++ei)
        {
          if (strVal == def->enum_labels[ei])
          {
            next->setInt(static_cast<int>(ei));
            config->set_key_value(stdKey, next.release());
            return true;
          }
        }
      }
      // String option
      if (auto *strOption = dynamic_cast<Slic3r::ConfigOptionString *>(next.get()))
      {
        strOption->value = strVal;
        config->set_key_value(stdKey, next.release());
        return true;
      }
    }

    // Handle numeric/bool values
    if (auto *boolOption = dynamic_cast<Slic3r::ConfigOptionBool *>(next.get()))
      boolOption->value = value.toBool();
    else if (auto *floatOption = dynamic_cast<Slic3r::ConfigOptionFloat *>(next.get()))
      floatOption->value = value.toDouble();
    else
      next->setInt(value.toInt());

    config->set_key_value(stdKey, next.release());
    return true;
  }

} // namespace
#endif

ProjectServiceMock::ProjectServiceMock(QObject *parent)
    : QObject(parent)
{
#ifdef HAS_LIBSLIC3R
  model_ = new Slic3r::Model();
#endif
}

ProjectServiceMock::~ProjectServiceMock()
{
#ifdef HAS_LIBSLIC3R
  delete model_;
#endif
}

QString ProjectServiceMock::projectName() const
{
  return projectName_;
}

int ProjectServiceMock::modelCount() const
{
  return modelCount_;
}

int ProjectServiceMock::plateCount() const
{
  return plateCount_;
}

int ProjectServiceMock::currentPlateIndex() const
{
  return currentPlateIndex_;
}

QString ProjectServiceMock::lastError() const
{
  return lastError_;
}

int ProjectServiceMock::loadProgress() const
{
  return loadProgress_;
}

bool ProjectServiceMock::loading() const
{
  return loading_;
}

QString ProjectServiceMock::sourceFilePath() const
{
  return sourceFilePath_;
}

#ifdef HAS_LIBSLIC3R
std::unique_ptr<Slic3r::Model> ProjectServiceMock::cloneCurrentPlateModel() const
{
  if (!model_ || currentPlateIndex_ < 0 || currentPlateIndex_ >= plateObjectIndices_.size())
    return {};

  auto clonedModel = std::make_unique<Slic3r::Model>(*model_);
  const QList<int> plateObjectIndices = currentPlateObjectIndices();
  QSet<int> allowedObjectIndices;
  allowedObjectIndices.reserve(plateObjectIndices.size());
  for (int objectIndex : plateObjectIndices)
  {
    if (objectIndex >= 0 && objectIndex < objectPrintableStates_.size() && objectPrintableStates_[objectIndex])
      allowedObjectIndices.insert(objectIndex);
  }

  for (int objectIndex = int(clonedModel->objects.size()) - 1; objectIndex >= 0; --objectIndex)
  {
    if (!allowedObjectIndices.contains(objectIndex))
      clonedModel->delete_object(size_t(objectIndex));
  }

  return clonedModel;
}
#endif

bool ProjectServiceMock::loadFile(const QString &filePath)
{
  if (loading_)
  {
    lastError_ = tr("已有任务正在加载");
    emit projectChanged();
    return false;
  }

  QFileInfo fi(filePath);
  if (!fi.exists())
  {
    lastError_ = tr("文件不存在: %1").arg(filePath);
    qWarning() << lastError_;
    emit projectChanged();
    return false;
  }

  loading_ = true;
  loadProgress_ = 0;
  activeCancelFlag_ = std::make_shared<std::atomic_bool>(false);
  emit loadingChanged();
  emit loadProgressChanged();

#ifdef HAS_LIBSLIC3R
  const QString localPath = fi.absoluteFilePath();
  const QPointer<ProjectServiceMock> receiver(this);
  const auto cancelFlag = activeCancelFlag_;

  QtConcurrent::run([receiver, localPath, cancelFlag]()
                    {
    auto *loadedModel = new Slic3r::Model();
    std::string err;
    int loadedPlateCount = 0;
    Slic3r::PlateDataPtrs plateDataList;
    QStringList loadedPlateNames;
    QList<QList<int>> loadedPlateObjectIndices;

    Slic3r::Import3mfProgressFn progressFn = [receiver, cancelFlag](int import_stage, int current, int total, bool &cancel) {
      cancel = cancelFlag && cancelFlag->load();

      int progress = 0;
      if (total > 0)
      {
        progress = int((double(current) * 100.0) / double(total));
      }
      progress = qBound(0, progress, 100);

      if (!receiver)
        return;
      const QString stageText = importStageToText(import_stage);
      QMetaObject::invokeMethod(receiver, [receiver, progress, stageText]() {
        if (!receiver)
          return;
        if (receiver->loadProgress_ != progress)
        {
          receiver->loadProgress_ = progress;
          emit receiver->loadProgressChanged();
        }
        emit receiver->loadProgressUpdated(progress, stageText);
      }, Qt::QueuedConnection);
    };

    bool ok = false;
    try
    {
      if (receiver)
      {
        QMetaObject::invokeMethod(receiver, [receiver]() {
          if (!receiver)
            return;
          receiver->loadProgress_ = 10;
          emit receiver->loadProgressChanged();
          emit receiver->loadProgressUpdated(10, QObject::tr("读取数据"));
        }, Qt::QueuedConnection);
      }

      Slic3r::Model loaded = Slic3r::Model::read_from_file(localPath.toStdString(),
                                                            nullptr,
                                                            nullptr,
                                                            Slic3r::LoadStrategy::AddDefaultInstances | Slic3r::LoadStrategy::LoadModel,
                                                            &plateDataList,
                                                            nullptr,
                                                            nullptr,
                                                            nullptr,
                                                            progressFn);
      *loadedModel = std::move(loaded);

      if (!loadedModel->objects.empty())
      {
        bool hasGeometry = false;
        for (const auto *obj : loadedModel->objects)
        {
          if (!obj)
            continue;
          for (const auto *vol : obj->volumes)
          {
            if (!vol)
              continue;
            const auto &its = vol->mesh().its;
            if (!its.vertices.empty() && !its.indices.empty())
            {
              hasGeometry = true;
              break;
            }
          }
          if (hasGeometry)
            break;
        }

        if (!hasGeometry)
        {
          err = "model has no renderable mesh data";
          ok = false;
        }
        else
        {
          ok = true;
          loadedPlateCount = int(plateDataList.size());

          if (!plateDataList.empty())
          {
            loadedPlateNames.reserve(loadedPlateCount);
            loadedPlateObjectIndices.reserve(loadedPlateCount);

            const int modelObjCount = int(loadedModel->objects.size());
            for (int plateIdx = 0; plateIdx < loadedPlateCount; ++plateIdx)
            {
              const auto *plate = plateDataList[size_t(plateIdx)];
              const QString plateName = (plate && !plate->plate_name.empty())
                                            ? QString::fromStdString(plate->plate_name)
                                            : QObject::tr("平板 %1").arg(plateIdx + 1);
              loadedPlateNames << plateName;

              QSet<int> uniq;
              QList<int> objList;
              if (plate)
              {
                for (const auto &pair : plate->objects_and_instances)
                {
                  const int objIndex = pair.first;
                  if (objIndex >= 0 && objIndex < modelObjCount && !uniq.contains(objIndex))
                  {
                    uniq.insert(objIndex);
                    objList.append(objIndex);
                  }
                }
              }
              std::sort(objList.begin(), objList.end());
              loadedPlateObjectIndices.append(objList);
            }
          }
          else
          {
            loadedPlateCount = 1;
            loadedPlateNames << QObject::tr("平板 1");

            QList<int> allObjects;
            allObjects.reserve(int(loadedModel->objects.size()));
            for (int i = 0; i < int(loadedModel->objects.size()); ++i)
              allObjects.append(i);
            loadedPlateObjectIndices.append(allObjects);
          }

          if (receiver)
          {
            QMetaObject::invokeMethod(receiver, [receiver]() {
              if (!receiver)
                return;
              receiver->loadProgress_ = 90;
              emit receiver->loadProgressChanged();
              emit receiver->loadProgressUpdated(90, QObject::tr("加载模型"));
            }, Qt::QueuedConnection);
          }
        }
      }
      else
      {
        err = "no model objects parsed";
      }
    }
    catch (const std::exception &ex)
    {
      err = ex.what();
      ok = false;
    }
    catch (...)
    {
      err = "unknown error";
      ok = false;
    }

    if (!plateDataList.empty())
      Slic3r::release_PlateData_list(plateDataList);

    const bool canceled = cancelFlag && cancelFlag->load();

    QStringList names;
    QStringList moduleNames;
    QList<bool> printableStates;
    QList<bool> visibleStates;
    QString errorText;
    QString loadedProjectName;
    if (ok && !canceled)
    {
      loadedProjectName = QFileInfo(localPath).completeBaseName();
      const auto &objs = loadedModel->objects;
      names.reserve(int(objs.size()));
      moduleNames.reserve(int(objs.size()));
      printableStates.reserve(int(objs.size()));
      visibleStates.reserve(int(objs.size()));
      for (size_t i = 0; i < objs.size(); ++i)
      {
        const auto *obj = objs[i];
        if (obj && !obj->name.empty())
          names << QString::fromStdString(obj->name);
        else
          names << QObject::tr("对象 %1").arg(int(i + 1));

        if (obj && !obj->module_name.empty())
          moduleNames << QString::fromStdString(obj->module_name);
        else
          moduleNames << QObject::tr("默认模块");

        const bool printable = obj && !obj->instances.empty() ? obj->instances.front()->printable : true;
        printableStates.append(printable);

        const bool visible = obj && !obj->instances.empty() ? obj->instances.front()->is_printable() : true;
        visibleStates.append(visible);
      }
    }
    else
    {
      errorText = canceled ? QObject::tr("已取消加载") : (err.empty() ? QObject::tr("加载失败") : QString::fromStdString(err));
      qWarning() << "[ProjectService] load failed:" << localPath << errorText;
    }

    if (!receiver)
    {
      delete loadedModel;
      return;
    }

    QMetaObject::invokeMethod(receiver, [receiver, loadedModel, ok, canceled, names, moduleNames, printableStates, visibleStates, errorText, loadedProjectName, loadedPlateCount, localPath, loadedPlateNames, loadedPlateObjectIndices]() {
      if (!receiver)
      {
        delete loadedModel;
        return;
      }

      receiver->loading_ = false;
      receiver->activeCancelFlag_.reset();
      emit receiver->loadingChanged();

      receiver->loadProgress_ = (ok && !canceled) ? 100 : 0;
      emit receiver->loadProgressChanged();
      if (ok && !canceled)
      {
        emit receiver->loadProgressUpdated(100, QObject::tr("完成导入"));
      }

      if (ok && !canceled)
      {
        if (receiver->model_)
          delete receiver->model_;
        receiver->model_ = loadedModel;
        receiver->projectName_ = loadedProjectName;
        receiver->sourceFilePath_ = localPath;
        receiver->objectNames_ = names;
        receiver->objectModuleNames_ = moduleNames;
        receiver->objectPrintableStates_ = printableStates;
        receiver->objectVisibleStates_ = visibleStates;
        receiver->modelCount_ = names.size();

        // P0.5.1: 同步 libslic3r 真实变换到 Mock 平行数组（slic3r → GL 坐标转换）
        {
          receiver->objectPositions_.clear();
          receiver->objectRotations_.clear();
          receiver->objectScales_.clear();
          const auto &objs = loadedModel->objects;
          receiver->objectPositions_.reserve(int(objs.size()));
          receiver->objectRotations_.reserve(int(objs.size()));
          receiver->objectScales_.reserve(int(objs.size()));
          for (const auto *obj : objs)
          {
            if (!obj || obj->instances.empty() || !obj->instances.front())
            {
              receiver->objectPositions_.append(QVector3D(0, 0, 0));
              receiver->objectRotations_.append(QVector3D(0, 0, 0));
              receiver->objectScales_.append(QVector3D(1, 1, 1));
              continue;
            }
            const auto *inst = obj->instances.front();
            const auto off = inst->get_offset();
            // slic3r(X,Y,Z) → GL(X,Z,Y): GL.x=slic3r.x, GL.y=slic3r.z(高度), GL.z=slic3r.y
            receiver->objectPositions_.append(QVector3D(
              static_cast<float>(off.x()),
              static_cast<float>(off.z()),
              static_cast<float>(off.y())));
            const auto rot = inst->get_rotation();
            // 旋转：弧度 → 角度，不做 Y/Z 互换（分量级操作）
            receiver->objectRotations_.append(QVector3D(
              qRadiansToDegrees(static_cast<float>(rot.x())),
              qRadiansToDegrees(static_cast<float>(rot.y())),
              qRadiansToDegrees(static_cast<float>(rot.z()))));
            const auto sc = inst->get_scaling_factor();
            receiver->objectScales_.append(QVector3D(
              static_cast<float>(sc.x()),
              static_cast<float>(sc.y()),
              static_cast<float>(sc.z())));
          }
        }

        receiver->plateNames_ = loadedPlateNames;
        receiver->plateObjectIndices_ = loadedPlateObjectIndices;

        if (receiver->plateObjectIndices_.isEmpty())
        {
          receiver->plateNames_.clear();
          receiver->plateNames_ << QObject::tr("平板 1");
          QList<int> all;
          all.reserve(receiver->modelCount_);
          for (int i = 0; i < receiver->modelCount_; ++i)
            all.append(i);
          receiver->plateObjectIndices_.append(all);
        }

        receiver->plateCount_ = receiver->plateObjectIndices_.size();
        if (receiver->plateCount_ != loadedPlateCount)
          receiver->plateCount_ = std::max(receiver->plateCount_, loadedPlateCount);
        receiver->currentPlateIndex_ = receiver->plateCount_ > 0 ? 0 : -1;
        receiver->lastError_.clear();

        emit receiver->projectChanged();
        emit receiver->plateDataLoaded(receiver->plateCount_);
        emit receiver->plateSelectionChanged();
        emit receiver->loadFinished(true, QObject::tr("加载完成"));
      }
      else
      {
        delete loadedModel;
        receiver->modelCount_ = 0;
        receiver->plateCount_ = 0;
        receiver->currentPlateIndex_ = -1;
        receiver->sourceFilePath_.clear();
        receiver->objectNames_.clear();
        receiver->objectModuleNames_.clear();
        receiver->objectPrintableStates_.clear();
        receiver->objectVisibleStates_.clear();
        receiver->plateNames_.clear();
        receiver->plateObjectIndices_.clear();
        receiver->plateLockedStates_.clear();
        receiver->plateFirstLayerSeqChoices_.clear();
        receiver->plateFirstLayerSeqOrders_.clear();
        receiver->plateOtherLayersSeqChoices_.clear();
        receiver->plateOtherLayersSeqEntries_.clear();
        receiver->lastError_ = errorText;
        emit receiver->projectChanged();
        emit receiver->plateDataLoaded(0);
        emit receiver->plateSelectionChanged();
        emit receiver->loadFinished(false, errorText);
      }
    }, Qt::QueuedConnection); });

  return true;

#else
  Q_UNUSED(fi);
  lastError_ = tr("当前构建未启用 libslic3r，无法导入模型");
  loading_ = false;
  loadProgress_ = 0;
  activeCancelFlag_.reset();
  emit loadingChanged();
  emit loadProgressChanged();
  emit projectChanged();
  emit plateDataLoaded(0);
  emit plateSelectionChanged();
  emit loadFinished(false, lastError_);
  return false;
#endif
}

void ProjectServiceMock::cancelLoad()
{
  if (!loading_ || !activeCancelFlag_)
    return;
  activeCancelFlag_->store(true);
}

QStringList ProjectServiceMock::objectNames() const
{
  return objectNames_;
}

QStringList ProjectServiceMock::plateNames() const
{
  return plateNames_;
}

QString ProjectServiceMock::objectModuleName(int index) const
{
  if (index < 0 || index >= objectModuleNames_.size())
    return {};
  return objectModuleNames_[index];
}

bool ProjectServiceMock::setCurrentPlateIndex(int index)
{
  if (index < 0 || index >= plateObjectIndices_.size())
    return false;
  if (currentPlateIndex_ == index)
    return true;
  currentPlateIndex_ = index;
  emit plateSelectionChanged();
  return true;
}

bool ProjectServiceMock::addPlate()
{
  if (loading_)
    return false;

  const int newPlateIndex = plateCount_;
  plateNames_ << tr("平板 %1").arg(newPlateIndex + 1);
  plateObjectIndices_ << QList<int>();
  plateLockedStates_ << false;
  plateFirstLayerSeqChoices_ << 0;   // Auto
  plateFirstLayerSeqOrders_ << QList<int>{};
  plateOtherLayersSeqChoices_ << 0;  // Auto
  plateOtherLayersSeqEntries_ << QList<MockLayerSeqEntry>{};
  plateCount_ = plateObjectIndices_.size();

  emit projectChanged();
  emit plateDataLoaded(plateCount_);
  return true;
}

bool ProjectServiceMock::deletePlate(int plateIndex)
{
  if (loading_)
    return false;

  if (plateObjectIndices_.size() <= 1)
    return false; // 至少保留 1 个平板

  if (plateIndex < 0 || plateIndex >= plateObjectIndices_.size())
    return false;

  plateNames_.removeAt(plateIndex);
  plateObjectIndices_.removeAt(plateIndex);
  if (plateIndex < plateLockedStates_.size())
    plateLockedStates_.removeAt(plateIndex);
  if (plateIndex < plateFirstLayerSeqChoices_.size()) plateFirstLayerSeqChoices_.removeAt(plateIndex);
  if (plateIndex < plateFirstLayerSeqOrders_.size()) plateFirstLayerSeqOrders_.removeAt(plateIndex);
  if (plateIndex < plateOtherLayersSeqChoices_.size()) plateOtherLayersSeqChoices_.removeAt(plateIndex);
  if (plateIndex < plateOtherLayersSeqEntries_.size()) plateOtherLayersSeqEntries_.removeAt(plateIndex);
  plateCount_ = plateObjectIndices_.size();

  if (currentPlateIndex_ >= plateCount_)
    currentPlateIndex_ = plateCount_ - 1;
  if (currentPlateIndex_ < 0)
    currentPlateIndex_ = 0;

  emit projectChanged();
  emit plateDataLoaded(plateCount_);
  emit plateSelectionChanged();
  return true;
}

bool ProjectServiceMock::renamePlate(int plateIndex, const QString &newName)
{
  if (loading_)
    return false;
  if (plateIndex < 0 || plateIndex >= plateNames_.size())
    return false;
  plateNames_[plateIndex] = newName;
  emit projectChanged();
  return true;
}

bool ProjectServiceMock::removeAllOnPlate(int plateIndex)
{
  if (loading_)
    return false;
  if (plateIndex < 0 || plateIndex >= plateObjectIndices_.size())
    return false;

  const QList<int> objs = plateObjectIndices_[plateIndex];
  if (objs.isEmpty())
    return true;

  // Delete objects in reverse order to keep indices stable
  QList<int> sorted = objs;
  std::sort(sorted.begin(), sorted.end(), std::greater<int>());
  for (int objIdx : sorted)
    deleteObject(objIdx);

  return true;
}

bool ProjectServiceMock::isPlateLocked(int plateIndex) const
{
  if (plateIndex < 0 || plateIndex >= plateLockedStates_.size())
    return false;
  return plateLockedStates_[plateIndex];
}

bool ProjectServiceMock::setPlateLocked(int plateIndex, bool locked)
{
  if (plateIndex < 0 || plateIndex >= plateObjectIndices_.size())
    return false;
  while (plateLockedStates_.size() < plateObjectIndices_.size())
    plateLockedStates_.append(false);
  plateLockedStates_[plateIndex] = locked;
  emit projectChanged();
  return true;
}

QList<int> ProjectServiceMock::plateObjectIndices(int plateIndex) const
{
  if (plateIndex < 0 || plateIndex >= plateObjectIndices_.size())
    return {};
  return plateObjectIndices_[plateIndex];
}

QList<int> ProjectServiceMock::currentPlateObjectIndices() const
{
  if (currentPlateIndex_ < 0 || currentPlateIndex_ >= plateObjectIndices_.size())
    return {};
  return plateObjectIndices_[currentPlateIndex_];
}

int ProjectServiceMock::plateObjectCount(int index) const
{
  if (index < 0 || index >= plateObjectIndices_.size())
    return 0;
  return plateObjectIndices_[index].size();
}

int ProjectServiceMock::plateIndexForObject(int objectIndex) const
{
  if (objectIndex < 0 || objectIndex >= objectNames_.size())
    return -1;

  for (int plateIndex = 0; plateIndex < plateObjectIndices_.size(); ++plateIndex)
  {
    if (plateObjectIndices_[plateIndex].contains(objectIndex))
      return plateIndex;
  }

  if (plateObjectIndices_.size() == 1)
    return 0;
  return -1;
}

void ProjectServiceMock::setObjectPlateForIndex(int objectIndex, int plateIndex)
{
  if (objectIndex < 0 || objectIndex >= objectNames_.size())
    return;
  if (plateIndex < 0 || plateIndex >= plateObjectIndices_.size())
    return;

  // 从源平板移除
  for (auto &indices : plateObjectIndices_)
    indices.removeAll(objectIndex);

  // 添加到目标平板
  plateObjectIndices_[plateIndex].append(objectIndex);
  emit projectChanged();
  emit plateDataLoaded(plateObjectIndices_.size());
}

bool ProjectServiceMock::objectPrintable(int index) const
{
  if (index < 0 || index >= objectPrintableStates_.size())
    return false;
  return objectPrintableStates_[index];
}

bool ProjectServiceMock::setObjectPrintable(int index, bool printable)
{
  if (loading_)
  {
    lastError_ = tr("加载中，无法更新对象打印状态");
    emit projectChanged();
    return false;
  }

  if (index < 0 || index >= objectPrintableStates_.size())
  {
    lastError_ = tr("更新失败：对象索引无效");
    emit projectChanged();
    return false;
  }

#ifdef HAS_LIBSLIC3R
  if (!model_ || size_t(index) >= model_->objects.size() || !model_->objects[size_t(index)])
  {
    lastError_ = tr("更新失败：模型对象无效");
    emit projectChanged();
    return false;
  }

  auto *obj = model_->objects[size_t(index)];
  for (auto *inst : obj->instances)
  {
    if (inst)
      inst->printable = printable;
  }
#endif

  objectPrintableStates_[index] = printable;
  lastError_.clear();
  emit projectChanged();
  return true;
}

bool ProjectServiceMock::objectVisible(int index) const
{
  if (index < 0 || index >= objectVisibleStates_.size())
    return true;
  return objectVisibleStates_[index];
}

bool ProjectServiceMock::setObjectVisible(int index, bool visible)
{
  if (loading_)
    return false;

  if (index < 0 || index >= objectVisibleStates_.size())
    return false;

#ifdef HAS_LIBSLIC3R
  if (!model_ || size_t(index) >= model_->objects.size() || !model_->objects[size_t(index)])
    return false;

  for (auto *inst : model_->objects[size_t(index)]->instances)
  {
    if (inst)
      inst->printable = visible;
  }
#endif

  objectVisibleStates_[index] = visible;
  emit projectChanged();
  return true;
}

int ProjectServiceMock::objectVolumeCount(int index) const
{
#ifdef HAS_LIBSLIC3R
  if (!model_ || index < 0 || size_t(index) >= model_->objects.size() || !model_->objects[size_t(index)])
    return 0;
  return int(model_->objects[size_t(index)]->volumes.size());
#else
  if (index < 0)
    return 0;
  const auto it = m_mockVolumes.constFind(index);
  return (it != m_mockVolumes.constEnd()) ? it->size() : 0;
#endif
}

int ProjectServiceMock::objectInstanceCount(int index) const
{
#ifdef HAS_LIBSLIC3R
  if (!model_ || index < 0 || size_t(index) >= model_->objects.size() || !model_->objects[size_t(index)])
    return 0;
  return int(model_->objects[size_t(index)]->instances.size());
#else
  Q_UNUSED(index)
  return 1; // Mock 模式: 每个 object 有 1 个 instance
#endif
}

QString ProjectServiceMock::objectVolumeName(int objectIndex, int volumeIndex) const
{
#ifdef HAS_LIBSLIC3R
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size())
    return {};

  const auto *obj = model_->objects[size_t(objectIndex)];
  if (!obj || volumeIndex < 0 || size_t(volumeIndex) >= obj->volumes.size() || !obj->volumes[size_t(volumeIndex)])
    return {};

  const auto *volume = obj->volumes[size_t(volumeIndex)];
  if (!volume->name.empty())
    return QString::fromStdString(volume->name);
  return tr("体积 %1").arg(volumeIndex + 1);
#else
  if (objectIndex < 0)
    return {};
  const auto it = m_mockVolumes.constFind(objectIndex);
  if (it == m_mockVolumes.constEnd() || volumeIndex < 0 || volumeIndex >= it->size())
    return {};
  return it->at(volumeIndex).name;
#endif
}

/// Mock-mode volume type label (对齐上游 volumeTypeToLabel)
static QString mockVolumeTypeLabel(MockVolumeType type)
{
  switch (type) {
  case MockVolumeType::ModelPart:        return QObject::tr("模型部件");
  case MockVolumeType::NegativeVolume:   return QObject::tr("负体积");
  case MockVolumeType::ParameterModifier:return QObject::tr("参数修改器");
  case MockVolumeType::SupportBlocker:   return QObject::tr("支撑屏蔽");
  case MockVolumeType::SupportEnforcer:  return QObject::tr("支撑增强");
  case MockVolumeType::TextEmboss:       return QObject::tr("文字浮雕");
  case MockVolumeType::SvgEmboss:        return QObject::tr("SVG浮雕");
  }
  return QObject::tr("体积");
}

QString ProjectServiceMock::objectVolumeTypeLabel(int objectIndex, int volumeIndex) const
{
#ifdef HAS_LIBSLIC3R
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size())
    return {};

  const auto *obj = model_->objects[size_t(objectIndex)];
  if (!obj || volumeIndex < 0 || size_t(volumeIndex) >= obj->volumes.size())
    return {};

  return volumeTypeToLabel(obj->volumes[size_t(volumeIndex)]);
#else
  if (objectIndex < 0)
    return {};
  const auto it = m_mockVolumes.constFind(objectIndex);
  if (it == m_mockVolumes.constEnd() || volumeIndex < 0 || volumeIndex >= it->size())
    return {};
  const auto &vol = it->at(volumeIndex);
  // Only show type label for non-model-part volumes (对齐上游 GUI_ObjectList 行为)
  if (vol.type == MockVolumeType::ModelPart)
    return {};
  return mockVolumeTypeLabel(vol.type);
#endif
}

QVariant ProjectServiceMock::scopedOptionValue(int objectIndex, int volumeIndex, const QString &key, const QVariant &fallbackValue) const
{
#ifdef HAS_LIBSLIC3R
  return readConfigValue(scopedConfigForRead(model_, objectIndex, volumeIndex), key, fallbackValue);
#else
  // Mock mode: check volume overrides first, then object overrides
  if (volumeIndex >= 0)
  {
    const int volKey = (objectIndex << 16) | volumeIndex;
    const auto vit = m_mockVolumeOverrides.constFind(volKey);
    if (vit != m_mockVolumeOverrides.constEnd())
    {
      const auto kv = vit->constFind(key);
      if (kv != vit->constEnd())
        return kv.value();
    }
  }
  if (objectIndex >= 0)
  {
    const auto oit = m_mockObjectOverrides.constFind(objectIndex);
    if (oit != m_mockObjectOverrides.constEnd())
    {
      const auto kv = oit->constFind(key);
      if (kv != oit->constEnd())
        return kv.value();
    }
  }
  return fallbackValue;
#endif
}

bool ProjectServiceMock::setScopedOptionValue(int objectIndex, int volumeIndex, const QString &key, const QVariant &value)
{
  if (loading_)
  {
    lastError_ = tr("加载中，无法更新参数");
    emit projectChanged();
    return false;
  }

#ifdef HAS_LIBSLIC3R
  auto *config = scopedConfigForWrite(model_, objectIndex, volumeIndex);
  if (!config)
  {
    lastError_ = tr("更新失败：配置目标无效");
    emit projectChanged();
    return false;
  }

  if (!writeConfigValue(config, key, value))
  {
    lastError_ = tr("更新失败：参数 %1 当前未支持写入").arg(key);
    emit projectChanged();
    return false;
  }

  lastError_.clear();
  emit projectChanged();
  return true;
#else
  // Mock mode: store per-volume or per-object overrides
  if (volumeIndex >= 0)
  {
    const int volKey = (objectIndex << 16) | volumeIndex;
    m_mockVolumeOverrides[volKey][key] = value;
  }
  else if (objectIndex >= 0)
  {
    m_mockObjectOverrides[objectIndex][key] = value;
  }
  else
  {
    lastError_ = tr("更新失败：配置目标无效（未指定对象或部件）");
    return false;
  }
  lastError_.clear();
  emit projectChanged();
  return true;
#endif
}

QVariant ProjectServiceMock::plateScopedOptionValue(int plateIndex, const QString &key, const QVariant &fallbackValue) const
{
#ifdef HAS_LIBSLIC3R
  Q_UNUSED(plateIndex);
  Q_UNUSED(key);
  return fallbackValue; // TODO: upstream PartPlate config access
#else
  const auto it = m_mockPlateOverrides.constFind(plateIndex);
  if (it != m_mockPlateOverrides.constEnd())
  {
    const auto kv = it->constFind(key);
    if (kv != it->constEnd())
      return kv.value();
  }
  return fallbackValue;
#endif
}

bool ProjectServiceMock::setPlateScopedOptionValue(int plateIndex, const QString &key, const QVariant &value)
{
#ifdef HAS_LIBSLIC3R
  Q_UNUSED(plateIndex);
  Q_UNUSED(key);
  Q_UNUSED(value);
  return false; // TODO: upstream PartPlate config access
#else
  m_mockPlateOverrides[plateIndex][key] = value;
  emit projectChanged();
  return true;
#endif
}

int ProjectServiceMock::scopedOverrideCount(int objectIndex, int volumeIndex) const
{
  if (volumeIndex >= 0) {
    const int volKey = (objectIndex << 16) | volumeIndex;
    auto it = m_mockVolumeOverrides.constFind(volKey);
    return it != m_mockVolumeOverrides.constEnd() ? it.value().size() : 0;
  }
  auto it = m_mockObjectOverrides.constFind(objectIndex);
  return it != m_mockObjectOverrides.constEnd() ? it.value().size() : 0;
}

QString ProjectServiceMock::scopedOverriddenKey(int objectIndex, int volumeIndex, int index) const
{
  const QHash<QString, QVariant> *map = nullptr;
  if (volumeIndex >= 0) {
    const int volKey = (objectIndex << 16) | volumeIndex;
    auto it = m_mockVolumeOverrides.constFind(volKey);
    if (it != m_mockVolumeOverrides.constEnd()) map = &it.value();
  } else {
    auto it = m_mockObjectOverrides.constFind(objectIndex);
    if (it != m_mockObjectOverrides.constEnd()) map = &it.value();
  }
  if (!map || index < 0 || index >= map->size()) return {};
  return map->keys().value(index);
}

bool ProjectServiceMock::resetScopedOptionValue(int objectIndex, int volumeIndex, const QString &key)
{
  if (volumeIndex >= 0) {
    const int volKey = (objectIndex << 16) | volumeIndex;
    auto it = m_mockVolumeOverrides.find(volKey);
    if (it != m_mockVolumeOverrides.end() && it.value().remove(key)) { emit projectChanged(); return true; }
  } else {
    auto it = m_mockObjectOverrides.find(objectIndex);
    if (it != m_mockObjectOverrides.end() && it.value().remove(key)) { emit projectChanged(); return true; }
  }
  return false;
}

int ProjectServiceMock::plateScopedOverrideCount(int plateIndex) const
{
  auto it = m_mockPlateOverrides.find(plateIndex);
  return it != m_mockPlateOverrides.end() ? it.value().size() : 0;
}

QString ProjectServiceMock::plateScopedOverriddenKey(int plateIndex, int index) const
{
  auto it = m_mockPlateOverrides.find(plateIndex);
  if (it == m_mockPlateOverrides.end() || index < 0 || index >= it.value().size()) return {};
  return it.value().keys().value(index);
}

bool ProjectServiceMock::resetPlateScopedOptionValue(int plateIndex, const QString &key)
{
  auto it = m_mockPlateOverrides.find(plateIndex);
  if (it != m_mockPlateOverrides.end() && it.value().remove(key)) { emit projectChanged(); return true; }
  return false;
}

bool ProjectServiceMock::deleteObjectVolume(int objectIndex, int volumeIndex)
{
  if (loading_)
  {
    lastError_ = tr("加载中，无法删除部件");
    emit projectChanged();
    return false;
  }

#ifdef HAS_LIBSLIC3R
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size())
  {
    lastError_ = tr("删除失败：对象索引无效");
    emit projectChanged();
    return false;
  }

  auto *obj = model_->objects[size_t(objectIndex)];
  if (!obj || volumeIndex < 0 || size_t(volumeIndex) >= obj->volumes.size())
  {
    lastError_ = tr("删除失败：部件索引无效");
    emit projectChanged();
    return false;
  }

  auto *volume = obj->volumes[size_t(volumeIndex)];
  if (!volume)
  {
    lastError_ = tr("删除失败：部件无效");
    emit projectChanged();
    return false;
  }

  int solidPartCount = 0;
  for (const auto *candidate : obj->volumes)
  {
    if (candidate && candidate->is_model_part())
      ++solidPartCount;
  }

  if (volume->is_model_part() && solidPartCount <= 1)
  {
    lastError_ = tr("不能删除最后一个实体部件");
    emit projectChanged();
    return false;
  }

  if (obj->is_cut() && (volume->is_model_part() || volume->is_negative_volume()))
  {
    lastError_ = tr("当前切割对象暂不支持删除该部件");
    emit projectChanged();
    return false;
  }

  obj->delete_volume(size_t(volumeIndex));
  lastError_.clear();
  emit projectChanged();
  return true;
#else
  if (objectIndex < 0)
  {
    lastError_ = tr("删除失败：对象索引无效");
    emit projectChanged();
    return false;
  }
  auto it = m_mockVolumes.find(objectIndex);
  if (it == m_mockVolumes.end() || volumeIndex < 0 || volumeIndex >= it->size())
  {
    lastError_ = tr("删除失败：部件索引无效");
    emit projectChanged();
    return false;
  }
  // Don't allow deleting the last model part (对齐上游约束)
  if (it->at(volumeIndex).type == MockVolumeType::ModelPart)
  {
    int solidPartCount = 0;
    for (const auto &v : *it)
      if (v.type == MockVolumeType::ModelPart)
        ++solidPartCount;
    if (solidPartCount <= 1)
    {
      lastError_ = tr("不能删除最后一个实体部件");
      emit projectChanged();
      return false;
    }
  }
  it->removeAt(volumeIndex);
  if (it->isEmpty())
    m_mockVolumes.erase(it);
  lastError_.clear();
  emit projectChanged();
  return true;
#endif
}

bool ProjectServiceMock::addVolume(int objectIndex, int volumeType)
{
  if (loading_)
  {
    lastError_ = tr("加载中，无法添加部件");
    emit projectChanged();
    return false;
  }

  if (objectIndex < 0 || objectIndex >= objectNames_.size())
  {
    lastError_ = tr("添加失败：对象索引无效");
    emit projectChanged();
    return false;
  }

  if (volumeType < 0 || volumeType > 4)
  {
    lastError_ = tr("添加失败：部件类型无效");
    emit projectChanged();
    return false;
  }

#ifdef HAS_LIBSLIC3R
  // P0.5.4: 对齐上游 GUI_ObjectList::load_generic_subobject
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size() || !model_->objects[size_t(objectIndex)])
  {
    lastError_ = tr("添加失败：模型对象无效");
    emit projectChanged();
    return false;
  }

  auto *obj = model_->objects[size_t(objectIndex)];

  try
  {
    // 从第一个实体部件复制网格（对齐上游：modifier/blocker 继承父对象网格）
    Slic3r::TriangleMesh mesh;
    for (const auto *vol : obj->volumes)
    {
      if (vol && vol->is_model_part())
      {
        mesh = vol->mesh();
        break;
      }
    }

    // 计算同类部件数量（对齐上游命名规则）
    static const char *typeNames[] = {
      "Part", "Negative Volume", "Modifier", "Support Blocker", "Support Enforcer"};
    int count = 0;
    for (const auto *v : obj->volumes)
    {
      if (v && v->type() == static_cast<Slic3r::ModelVolumeType>(volumeType))
        ++count;
    }

    auto *newVol = obj->add_volume(std::move(mesh), static_cast<Slic3r::ModelVolumeType>(volumeType));
    newVol->name = std::string(typeNames[volumeType]) + " " + std::to_string(count + 1);

    lastError_.clear();
    emit projectChanged();
    return true;
  }
  catch (const std::exception &ex)
  {
    lastError_ = tr("添加失败：%1").arg(QString::fromStdString(ex.what()));
    emit projectChanged();
    return false;
  }
#else
  auto type = static_cast<MockVolumeType>(volumeType);
  MockVolumeEntry entry;
  entry.type = type;

  // Generate name (对齐上游 GUI_ObjectList::load_generic_subobject 命名)
  static const char *typeNames[] = {
    QT_TRANSLATE_NOOP("ProjectServiceMock", "部件"),
    QT_TRANSLATE_NOOP("ProjectServiceMock", "负体积"),
    QT_TRANSLATE_NOOP("ProjectServiceMock", "修改器"),
    QT_TRANSLATE_NOOP("ProjectServiceMock", "支撑屏蔽"),
    QT_TRANSLATE_NOOP("ProjectServiceMock", "支撑增强"),
  };
  int count = 0;
  auto it = m_mockVolumes.find(objectIndex);
  if (it != m_mockVolumes.end())
  {
    for (const auto &v : *it)
      if (v.type == type)
        ++count;
  }
  entry.name = tr(typeNames[volumeType]) + QString(" %1").arg(count + 1);

  if (it == m_mockVolumes.end())
    it = m_mockVolumes.insert(objectIndex, {});
  it->append(entry);

  lastError_.clear();
  emit projectChanged();
  return true;
#endif
}

bool ProjectServiceMock::changeVolumeType(int objectIndex, int volumeIndex, int newVolumeType)
{
  if (objectIndex < 0 || objectIndex >= objectNames_.size())
  {
    lastError_ = tr("类型转换失败：对象索引无效");
    return false;
  }
  if (newVolumeType < 0 || newVolumeType > 6)
  {
    lastError_ = tr("类型转换失败：目标类型无效");
    return false;
  }

  auto it = m_mockVolumes.find(objectIndex);
  if (it == m_mockVolumes.end() || volumeIndex < 0 || volumeIndex >= it->size())
  {
    lastError_ = tr("类型转换失败：部件索引无效");
    return false;
  }

  auto oldType = it->at(volumeIndex).type;
  auto newType = static_cast<MockVolumeType>(newVolumeType);
  if (oldType == newType)
    return true; // no change

  // P0.5.4: 同步真实 ModelVolume 类型（仅 0-4 映射到上游 ModelVolumeType）
#ifdef HAS_LIBSLIC3R
  if (model_ && newVolumeType <= 4 &&
      size_t(objectIndex) < model_->objects.size() &&
      model_->objects[size_t(objectIndex)] &&
      size_t(volumeIndex) < model_->objects[size_t(objectIndex)]->volumes.size())
  {
    auto *vol = model_->objects[size_t(objectIndex)]->volumes[size_t(volumeIndex)];
    if (vol)
      vol->set_type(static_cast<Slic3r::ModelVolumeType>(newVolumeType));
  }
#endif

  // 对齐上游约束：转换时保留部件名，但更新类型
  it->operator[](volumeIndex).type = newType;

  // 更新部件名称前缀
  static const char *typeNames[] = {
      QT_TRANSLATE_NOOP("ProjectServiceMock", "部件"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "负体积"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "修改器"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "支撑屏蔽"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "支撑增强"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "文字浮雕"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "SVG浮雕"),
  };
  // 保留编号，更新类型名
  QString oldName = it->at(volumeIndex).name;
  QRegularExpression re(QStringLiteral("^(?:部件|负体积|修改器|支撑屏蔽|支撑增强|文字浮雕|SVG浮雕)\\s+(\\d+)$"));
  auto match = re.match(oldName);
  if (match.hasMatch())
    it->operator[](volumeIndex).name = tr(typeNames[newVolumeType]) + QStringLiteral(" ") + match.captured(1);

  lastError_.clear();
  emit projectChanged();
  return true;
}

// ── Volume 级 extruder 分配（对齐上游 ModelVolume::extruder_id）──

int ProjectServiceMock::volumeExtruderId(int objectIndex, int volumeIndex) const
{
#ifdef HAS_LIBSLIC3R
  if (model_ && objectIndex >= 0 && size_t(objectIndex) < model_->objects.size() &&
      model_->objects[size_t(objectIndex)] &&
      volumeIndex >= 0 && size_t(volumeIndex) < model_->objects[size_t(objectIndex)]->volumes.size())
  {
    const auto *vol = model_->objects[size_t(objectIndex)]->volumes[size_t(volumeIndex)];
    if (vol)
      return vol->extruder_id();
  }
#endif
  auto it = m_mockVolumes.find(objectIndex);
  if (it == m_mockVolumes.end() || volumeIndex < 0 || volumeIndex >= it->size())
    return -1; // inherit from object
  return it->at(volumeIndex).extruderId;
}

bool ProjectServiceMock::setVolumeExtruderId(int objectIndex, int volumeIndex, int extruderId)
{
  if (objectIndex < 0 || objectIndex >= objectNames_.size())
  {
    lastError_ = tr("设置耗材失败：对象索引无效");
    return false;
  }

  auto it = m_mockVolumes.find(objectIndex);
  if (it == m_mockVolumes.end() || volumeIndex < 0 || volumeIndex >= it->size())
  {
    lastError_ = tr("设置耗材失败：部件索引无效");
    return false;
  }

  // -1 means inherit from object, 0+ means specific extruder
  if (extruderId < -1)
  {
    lastError_ = tr("设置耗材失败：耗材索引无效");
    return false;
  }

  // P0.5.4: 同步真实 ModelVolume extruder config
#ifdef HAS_LIBSLIC3R
  if (model_ && size_t(objectIndex) < model_->objects.size() &&
      model_->objects[size_t(objectIndex)] &&
      size_t(volumeIndex) < model_->objects[size_t(objectIndex)]->volumes.size())
  {
    auto *vol = model_->objects[size_t(objectIndex)]->volumes[size_t(volumeIndex)];
    if (vol && extruderId >= 0)
    {
      // 对齐上游 GUI_ObjectList::set_extruder_for_selected_items
      if (vol->config.has("extruder"))
        vol->config.set("extruder", extruderId);
      else
        vol->config.set_key_value("extruder", new Slic3r::ConfigOptionInt(extruderId));
    }
  }
#endif

  it->operator[](volumeIndex).extruderId = extruderId;
  lastError_.clear();
  emit projectChanged();
  return true;
}

// ── Volume 外部文件导入（对齐上游 GUI_ObjectList::load_generic_subobject 文件加载）──

bool ProjectServiceMock::addVolumeFromFile(int objectIndex, const QString &filePath, int volumeType)
{
  if (objectIndex < 0 || objectIndex >= objectNames_.size())
  {
    lastError_ = tr("导入失败：对象索引无效");
    return false;
  }
  if (volumeType < 0 || volumeType > 6)
  {
    lastError_ = tr("导入失败：部件类型无效");
    return false;
  }

  // 从文件名生成部件名
  QFileInfo fi(filePath);
  const QString baseName = fi.completeBaseName();

#ifdef HAS_LIBSLIC3R
  // P0.5.4: 对齐上游 GUI_ObjectList::load_generic_subobject 文件加载
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size() || !model_->objects[size_t(objectIndex)])
  {
    lastError_ = tr("导入失败：模型对象无效");
    return false;
  }

  auto *obj = model_->objects[size_t(objectIndex)];

  try
  {
    Slic3r::Model loaded = Slic3r::Model::read_from_file(filePath.toStdString());
    Slic3r::TriangleMesh mesh;

    for (const auto *loadedObj : loaded.objects)
    {
      if (!loadedObj) continue;
      for (const auto *vol : loadedObj->volumes)
      {
        if (vol && !vol->mesh().its.vertices.empty())
        {
          mesh = vol->mesh();
          break;
        }
      }
      if (!mesh.its.vertices.empty()) break;
    }

    if (mesh.its.vertices.empty())
    {
      lastError_ = tr("导入失败：文件中未找到有效网格");
      return false;
    }

    auto *newVol = obj->add_volume(std::move(mesh), static_cast<Slic3r::ModelVolumeType>(volumeType));
    newVol->name = baseName.toStdString();

    lastError_.clear();
    emit projectChanged();
    return true;
  }
  catch (const std::exception &ex)
  {
    lastError_ = tr("导入失败：%1").arg(QString::fromStdString(ex.what()));
    return false;
  }
#else
  auto &vols = m_mockVolumes[objectIndex];
  static const char *typeNames[] = {
      QT_TRANSLATE_NOOP("ProjectServiceMock", "部件"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "负体积"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "修改器"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "支撑屏蔽"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "支撑增强"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "文字浮雕"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "SVG浮雕"),
  };

  MockVolumeEntry entry;
  entry.name = baseName;
  entry.type = static_cast<MockVolumeType>(volumeType);
  vols.append(entry);

  lastError_.clear();
  emit projectChanged();
  return true;
#endif
}

// ── 原始体创建（对齐上游 create_mesh + add_volume）──

bool ProjectServiceMock::addPrimitive(int objectIndex, int primitiveType)
{
  if (objectIndex < 0 || objectIndex >= objectNames_.size())
  {
    lastError_ = tr("创建原始体失败：对象索引无效");
    return false;
  }

  static const char *primNames[] = {
      QT_TRANSLATE_NOOP("ProjectServiceMock", "立方体"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "球体"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "圆柱体"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "圆环"),
  };
  if (primitiveType < 0 || primitiveType > 3)
  {
    lastError_ = tr("创建原始体失败：原始体类型无效");
    return false;
  }

#ifdef HAS_LIBSLIC3R
  if (model_ && objectIndex >= 0 && size_t(objectIndex) < model_->objects.size() && model_->objects[size_t(objectIndex)])
  {
    auto *obj = model_->objects[size_t(objectIndex)];
    try
    {
      static const char *typeNames[] = {"Cube", "Sphere", "Cylinder", "Torus"};
      indexed_triangle_set its;

      switch (primitiveType)
      {
      case 0: its = Slic3r::its_make_cube(20, 20, 20); break;
      case 1: its = Slic3r::its_make_sphere(10, 2.0 * M_PI / 360.0); break;
      case 2: its = Slic3r::its_make_cylinder(10, 20, 2.0 * M_PI / 360.0); break;
      default: its = Slic3r::its_make_sphere(10, 2.0 * M_PI / 360.0); break;
      }

      auto *newVol = obj->add_volume(Slic3r::TriangleMesh(std::move(its)));
      newVol->name = std::string(typeNames[primitiveType]);

      lastError_.clear();
      emit projectChanged();
      return true;
    }
    catch (const std::exception &ex)
    {
      lastError_ = tr("创建原始体失败：%1").arg(QString::fromStdString(ex.what()));
      emit projectChanged();
      return false;
    }
  }
#endif

  // Mock fallback
  auto &vols = m_mockVolumes[objectIndex];
  MockVolumeEntry entry;
  entry.name = tr(primNames[primitiveType]);
  entry.type = MockVolumeType::ModelPart;
  vols.append(entry);

  lastError_.clear();
  emit projectChanged();
  return true;
}

// ── 文字浮雕 volume（对齐上游 GLGizmoText）──

bool ProjectServiceMock::addTextVolume(int objectIndex, const QString &text)
{
  if (objectIndex < 0 || objectIndex >= objectNames_.size())
  {
    lastError_ = tr("添加文字失败：对象索引无效");
    return false;
  }
  if (text.isEmpty())
  {
    lastError_ = tr("添加文字失败：文字内容为空");
    return false;
  }

#ifdef HAS_LIBSLIC3R
  // 对齐上游 GLGizmoEmboss → Emboss::text2shapes + polygons2model
  if (model_ && size_t(objectIndex) < model_->objects.size() &&
      model_->objects[size_t(objectIndex)])
  {
    try
    {
      // 1. 加载系统字体（对齐上游 GLGizmoEmboss 使用 wxFont → Emboss::create_font_file）
      auto font_file = Slic3r::Emboss::create_font_file(
#ifdef _WIN32
          "C:/Windows/Fonts/arial.ttf"
#else
          "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#endif
      );
      if (!font_file)
      {
        lastError_ = tr("添加文字失败：无法加载字体");
        return false;
      }

      Slic3r::Emboss::FontFileWithCache font_cache(std::move(font_file));

      // 2. 配置字体属性（对齐上游 FontProp，定义在 TextConfiguration.hpp）
      Slic3r::FontProp font_prop;
      font_prop.size_in_mm = 10.0;   // 10mm 字高
      font_prop.boldness = 0.0f;     // 正常字重

      // 3. 文字 → 2D 多边形
      auto shapes = Slic3r::Emboss::text2shapes(font_cache, text.toLocal8Bit().constData(), font_prop);
      if (shapes.expolygons.empty())
      {
        lastError_ = tr("添加文字失败：字体渲染返回空结果");
        return false;
      }

      // 4. 2D 多边形 → 3D 网格（对齐上游 polygons2model + ProjectZ）
      const double depth = 2.0; // 2mm 挤出深度
      Slic3r::Emboss::ProjectZ projection(depth);
      auto its = Slic3r::Emboss::polygons2model(shapes.expolygons, projection);

      if (its.vertices.empty())
      {
        lastError_ = tr("添加文字失败：网格生成失败");
        return false;
      }

      // 5. 添加为真实 Volume
      Slic3r::TriangleMesh mesh(std::move(its));
      auto *newVol = model_->objects[size_t(objectIndex)]->add_volume(std::move(mesh), Slic3r::ModelVolumeType::MODEL_PART);
      if (newVol)
      {
        const QString volName = text.length() > 12 ? text.left(12) + "..." : text;
        newVol->name = volName.toStdString();
        newVol->set_type(Slic3r::ModelVolumeType::MODEL_PART);
        // 同步 Mock 数据
        auto &vols = m_mockVolumes[objectIndex];
        MockVolumeEntry entry;
        entry.name = volName;
        entry.type = MockVolumeType::TextEmboss;
        vols.append(entry);
        lastError_.clear();
        emit projectChanged();
        return true;
      }
    }
    catch (const std::exception &e)
    {
      lastError_ = QString::fromUtf8(e.what());
      return false;
    }
  }
#endif

  // Fallback: Mock 模式
  auto &vols = m_mockVolumes[objectIndex];
  MockVolumeEntry entry;
  entry.name = text.length() > 12 ? text.left(12) + "..." : text;
  entry.type = MockVolumeType::TextEmboss;
  vols.append(entry);

  lastError_.clear();
  emit projectChanged();
  return true;
}

// ── SVG 浮雕 volume（对齐上游 GLGizmoSVG → Model::read_from_file）──

bool ProjectServiceMock::addSvgVolume(int objectIndex, const QString &svgFilePath)
{
  if (objectIndex < 0 || objectIndex >= objectNames_.size())
  {
    lastError_ = tr("添加 SVG 失败：对象索引无效");
    return false;
  }

  QFileInfo fi(svgFilePath);
  if (!fi.exists())
  {
    lastError_ = tr("添加 SVG 失败：文件不存在");
    return false;
  }

  const QString baseName = fi.completeBaseName();

#ifdef HAS_LIBSLIC3R
  // 对齐上游：通过 Model::read_from_file 加载 SVG 为 3D 几何体
  if (model_ && size_t(objectIndex) < model_->objects.size() &&
      model_->objects[size_t(objectIndex)])
  {
    try
    {
      Slic3r::Model svgModel;
      svgModel.read_from_file(svgFilePath.toLocal8Bit().constData());
      if (!svgModel.objects.empty() && !svgModel.objects.front()->volumes.empty())
      {
        auto *srcVol = svgModel.objects.front()->volumes.front();
        if (srcVol && !srcVol->mesh().empty())
        {
          auto *newVol = model_->objects[size_t(objectIndex)]->add_volume(*srcVol);
          if (newVol)
          {
            newVol->name = baseName.toStdString();
            newVol->set_type(Slic3r::ModelVolumeType::MODEL_PART);
            // 同步 Mock 数据
            auto &vols = m_mockVolumes[objectIndex];
            MockVolumeEntry entry;
            entry.name = baseName.length() > 12 ? baseName.left(12) + "..." : baseName;
            entry.type = MockVolumeType::SvgEmboss;
            vols.append(entry);
            lastError_.clear();
            emit projectChanged();
            return true;
          }
        }
      }
    }
    catch (const std::exception &e)
    {
      lastError_ = QString::fromUtf8(e.what());
      return false;
    }
  }
#endif

  // Fallback: Mock 模式
  auto &vols = m_mockVolumes[objectIndex];
  MockVolumeEntry entry;
  entry.name = baseName.length() > 12 ? baseName.left(12) + "..." : baseName;
  entry.type = MockVolumeType::SvgEmboss;
  vols.append(entry);

  lastError_.clear();
  emit projectChanged();
  return true;
}

// ── 网格简化（对齐上游 GLGizmoSimplify → its_quadric_edge_collapse）──

bool ProjectServiceMock::simplifyObject(int objectIndex, int wantedCount, float maxError)
{
  if (loading_)
  {
    lastError_ = tr("加载中，无法简化");
    emit projectChanged();
    return false;
  }

  if (objectIndex < 0 || objectIndex >= objectNames_.size())
  {
    lastError_ = tr("简化失败：对象索引无效");
    emit projectChanged();
    return false;
  }

#ifdef HAS_LIBSLIC3R
  if (!model_ || size_t(objectIndex) >= model_->objects.size() || !model_->objects[size_t(objectIndex)])
  {
    lastError_ = tr("简化失败：模型对象无效");
    emit projectChanged();
    return false;
  }

  auto *obj = model_->objects[size_t(objectIndex)];

  try
  {
    for (auto *vol : obj->volumes)
    {
      if (!vol || vol->mesh().its.vertices.empty())
        continue;

      // Copy its (vol->mesh() returns const ref, need mutable copy)
      auto itsCopy = vol->mesh().its;
      const uint32_t originalCount = uint32_t(itsCopy.indices.size());

      // 对齐上游 GLGizmoSimplify 参数传递
      uint32_t targetCount = (wantedCount > 0) ? uint32_t(wantedCount) : 0;
      float errorVal = (maxError > 0.0f) ? maxError : std::numeric_limits<float>::max();

      Slic3r::its_quadric_edge_collapse(itsCopy, targetCount, &errorVal);

      const uint32_t newCount = uint32_t(itsCopy.indices.size());
      qInfo("[ProjectService] simplifyObject(%d): %u -> %u triangles",
            objectIndex, originalCount, newCount);

      // Replace volume mesh with simplified version (对齐上游 GLGizmoSimplify 结果回写)
      vol->set_mesh(std::move(itsCopy));
    }

    lastError_.clear();
    emit projectChanged();
    return true;
  }
  catch (const std::exception &ex)
  {
    lastError_ = tr("简化失败：%1").arg(QString::fromStdString(ex.what()));
    emit projectChanged();
    return false;
  }
#else
  Q_UNUSED(wantedCount);
  Q_UNUSED(maxError);
  lastError_ = tr("简化功能需要 libslic3r 支持");
  emit projectChanged();
  return false;
#endif
}

bool ProjectServiceMock::orientObject(int objectIndex)
{
#ifdef HAS_LIBSLIC3R
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size())
    return false;
  auto *obj = model_->objects[size_t(objectIndex)];
  if (!obj)
    return false;

  try
  {
    Slic3r::orientation::orient(obj);
    return true;
  }
  catch (const std::exception &e)
  {
    qDebug() << "orient failed for object" << objectIndex << ":" << e.what();
    return false;
  }
#else
  Q_UNUSED(objectIndex);
  return false;
#endif
}

bool ProjectServiceMock::arrangeObjects(float spacing, bool allowRotation, bool alignY)
{
#ifdef HAS_LIBSLIC3R
  if (!model_ || model_->objects.empty())
    return false;

  try
  {
    Slic3r::ArrangeParams params;
    params.min_obj_distance = static_cast<coord_t>(spacing * 1000.0); // mm → μm
    params.allow_rotations = allowRotation;
    params.align_to_y_axis = alignY;
    params.parallel = false; // Run synchronously in Qt thread
    params.progressind = [](unsigned, std::string) {}; // Suppress console output

    Slic3r::InfiniteBed bed;
    Slic3r::arrange_objects(*model_, bed, params);
    return true;
  }
  catch (const std::exception &e)
  {
    qDebug() << "arrange failed:" << e.what();
    return false;
  }
#else
  Q_UNUSED(spacing);
  Q_UNUSED(allowRotation);
  Q_UNUSED(alignY);
  return false;
#endif
}

int ProjectServiceMock::cutObject(int objectIndex, int axis, double position, int keepMode)
{
#ifdef HAS_LIBSLIC3R
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size())
    return -1;
  auto *obj = model_->objects[size_t(objectIndex)];
  if (!obj || obj->volumes.empty())
    return -1;

  try
  {
    // Collect all model_part volumes into a single mesh
    indexed_triangle_set combined;
    for (auto *vol : obj->volumes)
    {
      if (vol->is_model_part())
        Slic3r::its_merge(combined, vol->mesh().its);
    }
    if (combined.indices.empty())
      return -1;

    // Transform mesh to align cut axis with Z:
    // Z axis: no rotation, cut at z=position
    // X axis: rotate 90° around Y (X→Z), cut at z=position
    // Y axis: rotate -90° around X (Y→Z), cut at z=position
    Eigen::Matrix3f rot;
    float cutZ = float(position);
    if (axis == 1)
    {
      // Y→Z: rotate -90° around X
      rot = Eigen::AngleAxisf(float(-M_PI / 2.0), Eigen::Vector3f::UnitX()).toRotationMatrix();
    }
    else if (axis == 0)
    {
      // X→Z: rotate 90° around Y
      rot = Eigen::AngleAxisf(float(M_PI / 2.0), Eigen::Vector3f::UnitY()).toRotationMatrix();
    }
    else
    {
      rot = Eigen::Matrix3f::Identity();
    }

    // Apply rotation to all vertices
    for (auto &v : combined.vertices)
    {
      Eigen::Vector3f p(v.x(), v.y(), v.z());
      p = rot * p;
      v.x() = p.x(); v.y() = p.y(); v.z() = p.z();
    }

    // Perform the cut at the Z height
    indexed_triangle_set upper, lower;
    Slic3r::cut_mesh(combined, cutZ, &upper, &lower, true);

    // Rotate results back
    auto rotInv = rot.transpose();
    auto rotateBack = [&rotInv](indexed_triangle_set &its)
    {
      for (auto &v : its.vertices)
      {
        Eigen::Vector3f p(v.x(), v.y(), v.z());
        p = rotInv * p;
        v.x() = p.x(); v.y() = p.y(); v.z() = p.z();
      }
    };

    // Determine what to keep based on keepMode: 0=all, 1=upper, 2=lower
    bool keepUpper = (keepMode == 0 || keepMode == 1);
    bool keepLower = (keepMode == 0 || keepMode == 2);

    if (!keepUpper && !keepLower)
      return -1;

    // Replace original object's mesh with the lower part
    if (keepLower && !lower.indices.empty())
    {
      rotateBack(lower);
      // Replace first model_part volume mesh
      for (auto *vol : obj->volumes)
      {
        if (vol->is_model_part())
        {
          vol->set_mesh(Slic3r::TriangleMesh(std::move(lower)));
          break;
        }
      }
    }
    else if (!keepLower)
    {
      // Only keeping upper — delete the original object's mesh (will be replaced by upper)
    }

    // Add upper part as a new object if keeping it
    if (keepUpper && !upper.indices.empty())
    {
      rotateBack(upper);
      auto *newObj = model_->add_object();
      newObj->name = obj->name + "_upper";
      newObj->add_volume(Slic3r::TriangleMesh(std::move(upper)));
      // Copy instance transforms from original object
      for (auto *inst : obj->instances)
        newObj->add_instance(*inst);

      // Rebuild metadata arrays for the new object
      objectNames_.append(QString::fromStdString(newObj->name));
      objectPositions_.append({0, 0, 0});
      objectRotations_.append({0, 0, 0});
      objectScales_.append({1, 1, 1});
      objectPrintableStates_.append(true);
      objectVisibleStates_.append(true);

      return objectNames_.size() - 1;
    }

    return -1; // No new object created (only original modified)
  }
  catch (const std::exception &e)
  {
    qDebug() << "cut failed for object" << objectIndex << ":" << e.what();
    return -1;
  }
#else
  Q_UNUSED(objectIndex);
  Q_UNUSED(axis);
  Q_UNUSED(position);
  Q_UNUSED(keepMode);
  return -1;
#endif
}

int ProjectServiceMock::cutObjectWithGroove(int objectIndex, int axis, double position, int keepMode,
                                            float grooveDepth, float grooveWidth, float grooveFlapsAngle, float grooveAngle)
{
#ifdef HAS_LIBSLIC3R
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size())
    return -1;
  auto *obj = model_->objects[size_t(objectIndex)];
  if (!obj || obj->volumes.empty())
    return -1;

  try
  {
    // Build the cut_matrix: translation to cut plane position + rotation aligning cut axis with Z
    // The Cut class expects a transform that maps the model into a space where the cut
    // happens at z=0 in the XY plane.
    // For groove cut: cut_matrix = translation(position along cut axis) * rotation(axis->Z)
    // rotation_m (second param to perform_with_groove) is just the rotation part.

    using Vec3d = Eigen::Matrix<double, 3, 1, Eigen::DontAlign>;

    // Build rotation: align the cut axis with Z
    Slic3r::Transform3d rotation_m = Slic3r::Transform3d::Identity();
    Vec3d cut_translation = Vec3d::Zero();

    if (axis == 0)
    {
      // X axis: rotate Y by -90 degrees so X->Z
      rotation_m = Eigen::AngleAxisd(-M_PI / 2.0, Vec3d::UnitY()) * Slic3r::Transform3d::Identity();
      cut_translation = Vec3d(position, 0.0, 0.0);
    }
    else if (axis == 1)
    {
      // Y axis: rotate X by +90 degrees so Y->Z
      rotation_m = Eigen::AngleAxisd(M_PI / 2.0, Vec3d::UnitX()) * Slic3r::Transform3d::Identity();
      cut_translation = Vec3d(0.0, position, 0.0);
    }
    else
    {
      // Z axis: no rotation needed, cut at z=position
      rotation_m = Slic3r::Transform3d::Identity();
      cut_translation = Vec3d(0.0, 0.0, position);
    }

    // cut_matrix = translation to cut position * rotation (aligns cut axis with Z)
    // This follows upstream: get_cut_matrix() = translation_transform(cut_center_offset) * m_rotation_m
    Slic3r::Transform3d cut_matrix;
    cut_matrix = Eigen::Translation3d(cut_translation) * rotation_m;

    // Build attributes based on keepMode: 0=all, 1=upper, 2=lower
    // Follow upstream pattern: only_if(condition, enum) to build bitmask
    using MCA = Slic3r::ModelObjectCutAttribute;
    Slic3r::ModelObjectCutAttributes attributes =
        Slic3r::only_if(keepMode == 0 || keepMode == 1, MCA::KeepUpper) |
        Slic3r::only_if(keepMode == 0 || keepMode == 2, MCA::KeepLower) |
        MCA::KeepAsParts;

    // Build Groove struct (对齐上游 Cut::Groove)
    Slic3r::Cut::Groove groove;
    groove.depth = grooveDepth;
    groove.width = grooveWidth;
    groove.flaps_angle = grooveFlapsAngle;
    groove.angle = grooveAngle;
    groove.depth_init = grooveDepth;
    groove.width_init = grooveWidth;
    groove.flaps_angle_init = grooveFlapsAngle;
    groove.angle_init = grooveAngle;
    groove.depth_tolerance = 0.1f;
    groove.width_tolerance = 0.1f;

    // Validate groove: flaps_width must not exceed groove.width
    // (对齐上游 has_valid_groove check)
    const float flaps_width = (grooveFlapsAngle > 0.001f)
        ? -2.0f * grooveDepth / std::tan(grooveFlapsAngle)
        : 0.0f;
    if (flaps_width > grooveWidth)
    {
      qDebug() << "cutObjectWithGroove: invalid groove - flaps_width" << flaps_width
               << "> groove.width" << grooveWidth;
      return -1;
    }

    // Use instance 0 (single instance model)
    const int instance_idx = 0;

    // Perform the groove cut (对齐上游 perform_cut → cut.perform_with_groove)
    Slic3r::Cut cut(obj, instance_idx, cut_matrix, attributes);
    const Slic3r::ModelObjectPtrs &new_objects = cut.perform_with_groove(groove, rotation_m, false);

    if (new_objects.empty())
      return -1;

    // Replace original object with the first cut result, add subsequent results as new objects
    bool firstResult = true;
    int newObjectIdx = -1;

    for (auto *newObj : new_objects)
    {
      if (!newObj || newObj->volumes.empty())
        continue;

      if (firstResult)
      {
        // Replace original object's volumes with the first result
        // (upper part replaces original, or lower part depending on keep mode)
        obj->clear_volumes();
        for (auto *vol : newObj->volumes)
        {
          auto *newVol = obj->add_volume(*vol);
          Q_UNUSED(newVol);
        }
        obj->name = newObj->name;
        firstResult = false;
        newObjectIdx = objectIndex;
      }
      else
      {
        // Add subsequent results as new objects
        auto *addedObj = model_->add_object(*newObj);
        // Rebuild metadata arrays for the new object
        objectNames_.append(QString::fromStdString(addedObj->name));
        objectPositions_.append({0, 0, 0});
        objectRotations_.append({0, 0, 0});
        objectScales_.append({1, 1, 1});
        objectPrintableStates_.append(true);
        objectVisibleStates_.append(true);
        newObjectIdx = objectNames_.size() - 1;
      }
    }

    return newObjectIdx;
  }
  catch (const std::exception &e)
  {
    qDebug() << "cutObjectWithGroove failed for object" << objectIndex << ":" << e.what();
    return -1;
  }
#else
  Q_UNUSED(objectIndex);
  Q_UNUSED(axis);
  Q_UNUSED(position);
  Q_UNUSED(keepMode);
  Q_UNUSED(grooveDepth);
  Q_UNUSED(grooveWidth);
  Q_UNUSED(grooveFlapsAngle);
  Q_UNUSED(grooveAngle);
  return -1;
#endif
}

bool ProjectServiceMock::mirrorObject(int objectIndex, int axis)
{
#ifdef HAS_LIBSLIC3R
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size())
    return false;
  auto *obj = model_->objects[size_t(objectIndex)];
  if (!obj || obj->instances.empty())
    return false;

  try
  {
    // Toggle mirror on the specified axis (0=X, 1=Y, 2=Z)
    const Slic3r::Axis axes[] = {Slic3r::X, Slic3r::Y, Slic3r::Z};
    if (axis < 0 || axis > 2)
      return false;
    auto *inst = obj->instances.front();
    const double current = inst->get_mirror(axes[axis]);
    inst->set_mirror(axes[axis], -current); // Toggle: 1→-1 or -1→1
    return true;
  }
  catch (const std::exception &e)
  {
    qDebug() << "mirror failed for object" << objectIndex << ":" << e.what();
    return false;
  }
#else
  Q_UNUSED(objectIndex);
  Q_UNUSED(axis);
  return false;
#endif
}

bool ProjectServiceMock::meshBoolean(int srcObjectIndex, int toolObjectIndex, int operation)
{
#ifdef HAS_LIBSLIC3R
  if (!model_)
    return false;
  if (srcObjectIndex < 0 || size_t(srcObjectIndex) >= model_->objects.size())
    return false;
  if (toolObjectIndex < 0 || size_t(toolObjectIndex) >= model_->objects.size())
    return false;

  auto *srcObj = model_->objects[size_t(srcObjectIndex)];
  auto *toolObj = model_->objects[size_t(toolObjectIndex)];
  if (!srcObj || !toolObj || srcObj->volumes.empty() || toolObj->volumes.empty())
    return false;

  try
  {
    // Collect source mesh (all model_part volumes, with transform)
    Slic3r::TriangleMesh srcMesh;
    for (auto *vol : srcObj->volumes)
    {
      if (vol->is_model_part())
      {
        auto m = vol->mesh();
        m.transform(vol->get_matrix());
        Slic3r::its_merge(srcMesh.its, m.its);
      }
    }
    if (srcMesh.its.indices.empty())
      return false;

    // Collect tool mesh (all model_part volumes, with transform)
    Slic3r::TriangleMesh toolMesh;
    for (auto *vol : toolObj->volumes)
    {
      if (vol->is_model_part())
      {
        auto m = vol->mesh();
        m.transform(vol->get_matrix());
        Slic3r::its_merge(toolMesh.its, m.its);
      }
    }
    if (toolMesh.its.indices.empty())
      return false;

    // Perform boolean operation (对齐上游 GLGizmoMeshBoolean::execute_mesh_boolean)
    // operation: 0=union, 1=difference(A-B), 2=intersection
    // Use CGAL backend directly (对齐上游 Slic3r::MeshBoolean::cgal::plus/minus/intersect)
    if (operation == 0)
    {
      Slic3r::MeshBoolean::cgal::plus(srcMesh, toolMesh);
    }
    else if (operation == 1)
    {
      Slic3r::MeshBoolean::cgal::minus(srcMesh, toolMesh);
    }
    else if (operation == 2)
    {
      Slic3r::MeshBoolean::cgal::intersect(srcMesh, toolMesh);
    }
    else
    {
      return false;
    }

    // Replace source object's first model_part volume with result mesh
    // (对齐上游 generate_new_volume behavior: replace src volume, optionally delete tool)
    bool replaced = false;
    for (auto *vol : srcObj->volumes)
    {
      if (vol->is_model_part())
      {
        vol->set_mesh(std::move(srcMesh));
        vol->set_new_unique_id();
        replaced = true;
        break;
      }
    }

    // Delete tool object from model (对齐上游: delete_input = true for union)
    if (replaced)
    {
      // Remove tool object from model_ internal list
      model_->objects.erase(model_->objects.begin() + toolObjectIndex);

      // Sync mock arrays: remove tool object entry
      if (toolObjectIndex >= 0 && toolObjectIndex < objectNames_.size())
      {
        objectNames_.removeAt(toolObjectIndex);
        objectModuleNames_.removeAt(toolObjectIndex);
        objectPrintableStates_.removeAt(toolObjectIndex);
        objectVisibleStates_.removeAt(toolObjectIndex);
        objectPositions_.removeAt(toolObjectIndex);
        objectRotations_.removeAt(toolObjectIndex);
        objectScales_.removeAt(toolObjectIndex);
        m_mockVolumes.remove(toolObjectIndex);
        m_mockObjectOverrides.remove(toolObjectIndex);
        m_mockLayerRanges.remove(toolObjectIndex);
        modelCount_ = objectNames_.size();

        // Update plate object indices (shift down any index > toolObjectIndex)
        for (auto &plateObjs : plateObjectIndices_)
        {
          for (int i = 0; i < plateObjs.size(); ++i)
          {
            if (plateObjs[i] == toolObjectIndex)
              plateObjs.removeAt(i--);
            else if (plateObjs[i] > toolObjectIndex)
              --plateObjs[i];
          }
        }

        // Update source index if it shifted
        if (srcObjectIndex > toolObjectIndex)
          --srcObjectIndex;

        // Refresh source object name to indicate boolean result
        if (srcObjectIndex >= 0 && srcObjectIndex < objectNames_.size())
        {
          const char *suffix[] = {"_union", "_diff", "_inter"};
          objectNames_[srcObjectIndex] += QString(suffix[operation]);
        }
      }

      emit projectChanged();
      emit plateDataLoaded(plateCount_);
    }

    return replaced;
  }
  catch (const std::exception &e)
  {
    qWarning() << "meshBoolean failed:" << e.what();
    return false;
  }
#else
  Q_UNUSED(srcObjectIndex);
  Q_UNUSED(toolObjectIndex);
  Q_UNUSED(operation);
  return false;
#endif
}

bool ProjectServiceMock::drillObject(int objectIndex, float radius, float depth, int shape, int direction, bool oneLayerOnly)
{
#ifdef HAS_LIBSLIC3R
  if (!model_)
    return false;
  if (objectIndex < 0 || size_t(objectIndex) >= model_->objects.size())
    return false;

  auto *obj = model_->objects[size_t(objectIndex)];
  if (!obj || obj->volumes.empty())
    return false;

  try
  {
    // Collect source mesh from first model_part volume (对齐上游 GLGizmoDrill)
    Slic3r::ModelVolume *srcVol = nullptr;
    for (auto *vol : obj->volumes)
    {
      if (vol->is_model_part())
      {
        srcVol = vol;
        break;
      }
    }
    if (!srcVol)
      return false;

    // Get the volume's mesh (with its transform applied)
    Slic3r::TriangleMesh srcMesh(srcVol->mesh().its);

    // Clean up source mesh (对齐上游 its_merge_vertices + its_remove_degenerate_faces)
    Slic3r::its_merge_vertices(srcMesh.its);
    Slic3r::its_remove_degenerate_faces(srcMesh.its);
    Slic3r::its_compactify_vertices(srcMesh.its);

    if (srcMesh.its.indices.empty())
      return false;

    // Create drill tool mesh (对齐上游 GLGizmoDrill constructor: its_make_cylinder for all shapes)
    // Upstream uses the same its_make_cylinder with different angular resolution:
    //   Circle: 2*PI/16, Square: 2*PI/4, Triangle: 2*PI/3
    double angle = 2.0 * M_PI / 16.0;
    switch (shape)
    {
    case 1: angle = 2.0 * M_PI / 3.0; break;  // Triangle
    case 2: angle = 2.0 * M_PI / 4.0; break;  // Square
    default: break; // Circle (16 segments)
    }

    indexed_triangle_set drill_its = Slic3r::its_make_cylinder(1.0, 1.0, angle);
    Slic3r::TriangleMesh drillMesh(drill_its);

    // Compute bounding box to determine drill position
    const auto bb = srcMesh.bounding_box();
    const double topZ = bb.max.z();

    // Position the drill at top-center of the object, drilling downward (-Z)
    // (对齐上游 feature_matrix_world construction, simplified without raycasting hit point)
    const double guard = 100.0; // 对齐上游 depth_scale_factor
    const double drillDepth = oneLayerOnly ? 2.0 : static_cast<double>(depth);

    // Build feature matrix in local volume space (对齐上游 GLGizmoDrill::gizmo_event)
    // The drill mesh unit cylinder is 1x1, so we scale by (radius, radius, drillDepth + guard)
    // and position at (center_x, center_y, topZ + guard) pointing down (-Z)
    const double cx = (bb.min.x() + bb.max.x()) * 0.5;
    const double cy = (bb.min.y() + bb.max.y()) * 0.5;

    // Direction handling (对齐上游 GLGizmoDrill::getDirection):
    // direction 0 = Normal (surface normal, simplified to -Z here)
    // direction 1 = Parallel to platform (horizontal, simplified to -Y here)
    // direction 2 = Perpendicular to screen (not applicable without camera, use -Z)
    Slic3r::Vec3d drillNormal = Slic3r::Vec3d::UnitZ(); // Drill direction (+Z = down into object)
    Slic3r::Vec3d drillPos(cx, cy, topZ);

    if (direction == 1)
    {
      // Parallel to platform: horizontal drill from the side
      drillNormal = Slic3r::Vec3d::UnitY();
      drillPos = Slic3r::Vec3d(bb.max.x(), (bb.min.y() + bb.max.y()) * 0.5, (bb.min.z() + bb.max.z()) * 0.5);
    }
    // direction 0 and 2 both drill vertically (simplified)

    // The drill mesh is along Z-axis; we rotate it to align with drill direction
    // (对齐上游 Eigen::Quaternion::FromTwoVectors(Vec3d::UnitZ(), -n_world))
    Eigen::Quaterniond rot = Eigen::Quaterniond::FromTwoVectors(Eigen::Vector3d::UnitZ(), -drillNormal);

    // Transform: translate to drill position + guard offset along drill direction,
    // then rotate to align with drill normal, then scale to desired radius/depth
    // (对齐上游 feature_matrix_world construction)
    const Slic3r::Transform3d featureMatrix =
      Slic3r::Geometry::translation_transform(drillPos + drillNormal * guard) *
      rot *
      Slic3r::Geometry::scale_transform(Slic3r::Vec3d(static_cast<double>(radius), static_cast<double>(radius), drillDepth + guard));

    // Transform drill mesh by feature matrix (对齐 upstream temp_tool_mesh.transform(feature_matrix_local))
    drillMesh.transform(featureMatrix);

    // Perform CGAL boolean subtraction (对齐上游 sla::hollow_mesh_and_drill → MeshBoolean::cgal::minus)
    // Note: hollow_mesh_and_drill internally calls MeshBoolean::cgal::minus too,
    // but also pulls in OpenVDB (mesh_to_grid/grid_to_mesh/redistance_grid).
    // We use the CGAL minus directly to avoid the OpenVDB linker dependency.
    try
    {
      Slic3r::MeshBoolean::cgal::minus(srcMesh, drillMesh);

      // Update source volume with drilled mesh (对齐上游 generate_new_volume)
      srcVol->set_mesh(std::move(srcMesh));
      srcVol->set_new_unique_id();

      syncTransformsFromModel();
      emit projectChanged();
      return true;
    }
    catch (const std::exception &e)
    {
      qWarning() << "drillObject: CGAL minus failed:" << e.what();
      return false;
    }
  }
  catch (const std::exception &e)
  {
    qWarning() << "drillObject failed for object" << objectIndex << ":" << e.what();
    return false;
  }
#else
  Q_UNUSED(objectIndex);
  Q_UNUSED(radius);
  Q_UNUSED(depth);
  Q_UNUSED(shape);
  Q_UNUSED(direction);
  Q_UNUSED(oneLayerOnly);
  return false;
#endif
}

void ProjectServiceMock::syncTransformsFromModel()
{
#ifdef HAS_LIBSLIC3R
  if (!model_)
    return;
  for (int i = 0; i < objectNames_.size() && i < objectPositions_.size(); ++i)
  {
    if (size_t(i) < model_->objects.size() && model_->objects[size_t(i)])
    {
      objectPositions_[i] = objectPosition(i);
      objectRotations_[i] = objectRotation(i);
      objectScales_[i] = objectScale(i);
    }
  }
#endif
}

// ── Mesh snapshot for undo/redo ──

QByteArray ProjectServiceMock::captureObjectMeshSnapshot(int objectIndex) const
{
#ifdef HAS_LIBSLIC3R
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size())
    return {};
  auto *obj = model_->objects[size_t(objectIndex)];
  if (!obj)
    return {};
  // Find the first model_part volume
  Slic3r::ModelVolume *srcVol = nullptr;
  for (auto *vol : obj->volumes)
  {
    if (vol && vol->is_model_part())
    {
      srcVol = vol;
      break;
    }
  }
  if (!srcVol)
    return {};
  const auto &its = srcVol->mesh().its;
  if (its.indices.empty())
    return {};

  // Serialize: vertex_count(int32) + index_count(int32) + vertices(float*3) + indices(int32*3)
  const int32_t vCount = int32_t(its.vertices.size());
  const int32_t iCount = int32_t(its.indices.size());
  const size_t dataBytes = 2 * sizeof(int32_t)
      + vCount * 3 * sizeof(float)
      + iCount * 3 * sizeof(int32_t);
  QByteArray ba;
  ba.resize(int(dataBytes));
  int offset = 0;
  std::memcpy(ba.data() + offset, &vCount, sizeof(int32_t)); offset += sizeof(int32_t);
  std::memcpy(ba.data() + offset, &iCount, sizeof(int32_t)); offset += sizeof(int32_t);
  if (vCount > 0)
  {
    std::memcpy(ba.data() + offset, its.vertices.data(), vCount * 3 * sizeof(float));
    offset += vCount * 3 * sizeof(float);
  }
  if (iCount > 0)
  {
    std::memcpy(ba.data() + offset, its.indices.data(), iCount * 3 * sizeof(int32_t));
  }
  return ba;
#else
  Q_UNUSED(objectIndex);
  return {};
#endif
}

bool ProjectServiceMock::restoreObjectMeshSnapshot(int objectIndex, const QByteArray &snapshot)
{
#ifdef HAS_LIBSLIC3R
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size())
    return false;
  if (snapshot.size() < int(2 * sizeof(int32_t)))
    return false;
  auto *obj = model_->objects[size_t(objectIndex)];
  if (!obj)
    return false;
  // Find the first model_part volume
  Slic3r::ModelVolume *srcVol = nullptr;
  for (auto *vol : obj->volumes)
  {
    if (vol && vol->is_model_part())
    {
      srcVol = vol;
      break;
    }
  }
  if (!srcVol)
    return false;

  int offset = 0;
  int32_t vCount = 0, iCount = 0;
  std::memcpy(&vCount, snapshot.constData() + offset, sizeof(int32_t)); offset += sizeof(int32_t);
  std::memcpy(&iCount, snapshot.constData() + offset, sizeof(int32_t)); offset += sizeof(int32_t);

  indexed_triangle_set restored;
  restored.vertices.resize(size_t(vCount));
  restored.indices.resize(size_t(iCount));
  if (vCount > 0)
  {
    std::memcpy(restored.vertices.data(), snapshot.constData() + offset, vCount * 3 * sizeof(float));
    offset += vCount * 3 * sizeof(float);
  }
  if (iCount > 0)
  {
    std::memcpy(restored.indices.data(), snapshot.constData() + offset, iCount * 3 * sizeof(int32_t));
  }

  srcVol->set_mesh(Slic3r::TriangleMesh(std::move(restored)));
  srcVol->set_new_unique_id();
  syncTransformsFromModel();
  emit projectChanged();
  return true;
#else
  Q_UNUSED(objectIndex);
  Q_UNUSED(snapshot);
  return false;
#endif
}

#ifdef HAS_LIBSLIC3R
int ProjectServiceMock::objectTriangleCount(int objectIndex) const
{
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size() || !model_->objects[size_t(objectIndex)])
    return 0;

  int total = 0;
  for (const auto *vol : model_->objects[size_t(objectIndex)]->volumes)
  {
    if (vol)
      total += int(vol->mesh().its.indices.size());
  }
  return total;
}

int ProjectServiceMock::objectOpenEdges(int objectIndex) const
{
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size() || !model_->objects[size_t(objectIndex)])
    return 0;
  return model_->objects[size_t(objectIndex)]->get_object_stl_stats().open_edges;
}

int ProjectServiceMock::objectRepairedErrors(int objectIndex) const
{
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size() || !model_->objects[size_t(objectIndex)])
    return 0;
  return model_->objects[size_t(objectIndex)]->get_repaired_errors_count();
}

float ProjectServiceMock::objectVolume(int objectIndex) const
{
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size() || !model_->objects[size_t(objectIndex)])
    return 0.f;
  return model_->objects[size_t(objectIndex)]->get_object_stl_stats().volume;
}
#endif

// ── Layer range support (对齐上游 ModelObject::layer_config_ranges) ──

QList<MockLayerRange> ProjectServiceMock::objectLayerRanges(int objectIndex) const
{
  return m_mockLayerRanges.value(objectIndex);
}

bool ProjectServiceMock::addObjectLayerRange(int objectIndex, double minZ, double maxZ)
{
  if (objectIndex < 0 || objectIndex >= objectNames_.size())
    return false;
  if (minZ >= maxZ)
    return false;

  MockLayerRange range;
  range.minZ = minZ;
  range.maxZ = maxZ;
  m_mockLayerRanges[objectIndex].append(range);
  return true;
}

bool ProjectServiceMock::removeObjectLayerRange(int objectIndex, int rangeIndex)
{
  auto it = m_mockLayerRanges.find(objectIndex);
  if (it == m_mockLayerRanges.end())
    return false;
  if (rangeIndex < 0 || rangeIndex >= it->size())
    return false;
  it->removeAt(rangeIndex);
  return true;
}

bool ProjectServiceMock::setLayerRangeValue(int objectIndex, int rangeIndex, const QString &key, const QVariant &value)
{
  auto it = m_mockLayerRanges.find(objectIndex);
  if (it == m_mockLayerRanges.end())
    return false;
  if (rangeIndex < 0 || rangeIndex >= it->size())
    return false;
  it->operator[](rangeIndex).overrides[key] = value;
  return true;
}

QVariant ProjectServiceMock::layerRangeValue(int objectIndex, int rangeIndex, const QString &key, const QVariant &fallback) const
{
  auto it = m_mockLayerRanges.constFind(objectIndex);
  if (it == m_mockLayerRanges.cend())
    return fallback;
  if (rangeIndex < 0 || rangeIndex >= it->size())
    return fallback;
  return it->at(rangeIndex).overrides.value(key, fallback);
}

bool ProjectServiceMock::deleteObject(int index)
{
  if (loading_)
  {
    lastError_ = tr("加载中，无法删除对象");
    emit projectChanged();
    return false;
  }

  if (index < 0 || index >= objectNames_.size())
  {
    lastError_ = tr("删除失败：对象索引无效");
    emit projectChanged();
    return false;
  }

  try
  {
#ifdef HAS_LIBSLIC3R
    if (!model_)
    {
      lastError_ = tr("删除失败：模型未初始化");
      emit projectChanged();
      return false;
    }
    if (size_t(index) >= model_->objects.size())
    {
      lastError_ = tr("删除失败：模型索引无效");
      emit projectChanged();
      return false;
    }

    model_->delete_object(size_t(index));

    objectNames_.clear();
    objectNames_.reserve(int(model_->objects.size()));
    objectModuleNames_.clear();
    objectModuleNames_.reserve(int(model_->objects.size()));
    objectPrintableStates_.clear();
    objectVisibleStates_.clear();
    objectPositions_.clear();
    objectRotations_.clear();
    objectScales_.clear();
    objectPrintableStates_.reserve(int(model_->objects.size()));
    objectVisibleStates_.reserve(int(model_->objects.size()));
    for (size_t i = 0; i < model_->objects.size(); ++i)
    {
      const auto *obj = model_->objects[i];
      if (obj && !obj->name.empty())
        objectNames_ << QString::fromStdString(obj->name);
      else
        objectNames_ << tr("对象 %1").arg(int(i + 1));

      if (obj && !obj->module_name.empty())
        objectModuleNames_ << QString::fromStdString(obj->module_name);
      else
        objectModuleNames_ << tr("默认模块");

      const bool printable = obj && !obj->instances.empty() ? obj->instances.front()->printable : true;
      objectPrintableStates_.append(printable);

      const bool vis = obj && !obj->instances.empty() ? obj->instances.front()->is_printable() : true;
      objectVisibleStates_.append(vis);

      // P0.5.1: 从真实实例同步变换（slic3r → GL）
      if (obj && !obj->instances.empty() && obj->instances.front())
      {
        const auto *inst = obj->instances.front();
        const auto off = inst->get_offset();
        objectPositions_.append(QVector3D(
          static_cast<float>(off.x()), static_cast<float>(off.z()), static_cast<float>(off.y())));
        const auto rot = inst->get_rotation();
        objectRotations_.append(QVector3D(
          qRadiansToDegrees(static_cast<float>(rot.x())),
          qRadiansToDegrees(static_cast<float>(rot.y())),
          qRadiansToDegrees(static_cast<float>(rot.z()))));
        const auto sc = inst->get_scaling_factor();
        objectScales_.append(QVector3D(
          static_cast<float>(sc.x()), static_cast<float>(sc.y()), static_cast<float>(sc.z())));
      }
      else
      {
        objectPositions_.append(QVector3D(0, 0, 0));
        objectRotations_.append(QVector3D(0, 0, 0));
        objectScales_.append(QVector3D(1, 1, 1));
      }
    }
#else
    // Clean up mock volumes and layer ranges for this object
    m_mockVolumes.remove(index);
    m_mockLayerRanges.remove(index);
    // Shift volume keys for objects after deleted one
    QHash<int, QList<MockVolumeEntry>> shiftedVolumes;
    for (auto it = m_mockVolumes.constBegin(); it != m_mockVolumes.constEnd(); ++it)
    {
      int newObjIndex = (it.key() > index) ? it.key() - 1 : it.key();
      shiftedVolumes[newObjIndex] = it.value();
    }
    m_mockVolumes = shiftedVolumes;

    objectNames_.removeAt(index);
    objectModuleNames_.removeAt(index);
    objectPrintableStates_.removeAt(index);
    if (index < objectVisibleStates_.size())
      objectVisibleStates_.removeAt(index);
#endif

    modelCount_ = objectNames_.size();
    if (modelCount_ <= 0)
    {
      plateCount_ = 0;
      currentPlateIndex_ = -1;
      plateNames_.clear();
      plateObjectIndices_.clear();
      plateLockedStates_.clear();
      plateFirstLayerSeqChoices_.clear();
      plateFirstLayerSeqOrders_.clear();
      plateOtherLayersSeqChoices_.clear();
      plateOtherLayersSeqEntries_.clear();
    }
    else
    {
      if (plateObjectIndices_.isEmpty())
      {
        QList<int> all;
        all.reserve(modelCount_);
        for (int i = 0; i < modelCount_; ++i)
          all.append(i);
        plateObjectIndices_.append(all);
        if (plateNames_.isEmpty())
          plateNames_ << tr("平板 1");
      }
      else
      {
        for (auto &plateObjList : plateObjectIndices_)
        {
          QSet<int> uniq;
          QList<int> next;
          for (int objIndex : plateObjList)
          {
            if (objIndex == index)
              continue;
            const int adjusted = objIndex > index ? objIndex - 1 : objIndex;
            if (adjusted >= 0 && adjusted < modelCount_ && !uniq.contains(adjusted))
            {
              uniq.insert(adjusted);
              next.append(adjusted);
            }
          }
          std::sort(next.begin(), next.end());
          plateObjList = next;
        }
      }

      for (int p = plateObjectIndices_.size() - 1; p >= 0; --p)
      {
        if (plateObjectIndices_[p].isEmpty() && plateObjectIndices_.size() > 1)
        {
          plateObjectIndices_.removeAt(p);
          if (p < plateNames_.size())
            plateNames_.removeAt(p);
          if (currentPlateIndex_ >= p)
            --currentPlateIndex_;
        }
      }

      plateCount_ = plateObjectIndices_.size();
      if (currentPlateIndex_ < 0)
        currentPlateIndex_ = 0;
      if (currentPlateIndex_ >= plateCount_)
        currentPlateIndex_ = plateCount_ - 1;
    }

    lastError_.clear();
    emit projectChanged();
    emit plateDataLoaded(plateCount_);
    emit plateSelectionChanged();
    return true;
  }
  catch (const std::exception &ex)
  {
    lastError_ = QString::fromStdString(ex.what());
    emit projectChanged();
    return false;
  }
  catch (...)
  {
    lastError_ = tr("删除失败：未知错误");
    emit projectChanged();
    return false;
  }
}

bool ProjectServiceMock::renameObject(int index, const QString &newName)
{
  if (index < 0 || index >= objectNames_.size())
    return false;
#ifdef HAS_LIBSLIC3R
  if (model_ && size_t(index) < model_->objects.size() && model_->objects[size_t(index)])
    model_->objects[size_t(index)]->name = newName.toStdString();
#endif
  objectNames_[index] = newName;
  emit projectChanged();
  return true;
}

bool ProjectServiceMock::moveObject(int fromIndex, int toIndex)
{
  if (fromIndex < 0 || fromIndex >= objectNames_.size())
    return false;
  if (toIndex < 0 || toIndex >= objectNames_.size())
    return false;
  if (fromIndex == toIndex)
    return true;

#ifdef HAS_LIBSLIC3R
  // P0.5.3: 对齐上游 GUI_ObjectList::OnDrop 迭代 std::swap 实现对象重排序
  if (model_ && size_t(fromIndex) < model_->objects.size() && size_t(toIndex) < model_->objects.size())
  {
    const int delta = fromIndex < toIndex ? 1 : -1;
    for (int id = fromIndex; id != toIndex; id += delta)
      std::swap(model_->objects[size_t(id)], model_->objects[size_t(id + delta)]);
  }
#endif

  // Swap all parallel arrays at fromIndex ↔ toIndex
  std::swap(objectNames_[fromIndex], objectNames_[toIndex]);
  std::swap(objectModuleNames_[fromIndex], objectModuleNames_[toIndex]);
  std::swap(objectPrintableStates_[fromIndex], objectPrintableStates_[toIndex]);
  std::swap(objectVisibleStates_[fromIndex], objectVisibleStates_[toIndex]);
  if (fromIndex < objectPositions_.size() && toIndex < objectPositions_.size()) {
    std::swap(objectPositions_[fromIndex], objectPositions_[toIndex]);
    std::swap(objectRotations_[fromIndex], objectRotations_[toIndex]);
    std::swap(objectScales_[fromIndex], objectScales_[toIndex]);
  }

  // Update plateObjectIndices_ references
  for (auto &indices : plateObjectIndices_) {
    for (int &idx : indices) {
      if (idx == fromIndex) idx = toIndex;
      else if (idx == toIndex) idx = fromIndex;
    }
  }

  // Swap per-object scoped overrides
  if (m_mockObjectOverrides.contains(fromIndex) || m_mockObjectOverrides.contains(toIndex)) {
    auto fromOverrides = m_mockObjectOverrides.take(fromIndex);
    auto toOverrides = m_mockObjectOverrides.take(toIndex);
    if (!fromOverrides.isEmpty()) m_mockObjectOverrides[toIndex] = fromOverrides;
    if (!toOverrides.isEmpty()) m_mockObjectOverrides[fromIndex] = toOverrides;
  }

  emit projectChanged();
  return true;
}

int ProjectServiceMock::plateBedType(int plateIndex) const
{
  return (plateIndex >= 0 && plateIndex < plateBedTypes_.size()) ? plateBedTypes_[plateIndex] : 0;
}

bool ProjectServiceMock::setPlateBedType(int plateIndex, int bedType)
{
  if (plateIndex < 0 || plateIndex >= plateNames_.size()) return false;
  while (plateBedTypes_.size() <= plateIndex) plateBedTypes_.append(0);
  plateBedTypes_[plateIndex] = bedType;
  emit projectChanged();
  return true;
}

int ProjectServiceMock::platePrintSequence(int plateIndex) const
{
  return (plateIndex >= 0 && plateIndex < platePrintSequences_.size()) ? platePrintSequences_[plateIndex] : 0;
}

bool ProjectServiceMock::setPlatePrintSequence(int plateIndex, int seq)
{
  if (plateIndex < 0 || plateIndex >= plateNames_.size()) return false;
  while (platePrintSequences_.size() <= plateIndex) platePrintSequences_.append(0);
  platePrintSequences_[plateIndex] = seq;
  emit projectChanged();
  return true;
}

int ProjectServiceMock::plateSpiralMode(int plateIndex) const
{
  return (plateIndex >= 0 && plateIndex < plateSpiralModes_.size()) ? plateSpiralModes_[plateIndex] : 0;
}

bool ProjectServiceMock::setPlateSpiralMode(int plateIndex, int mode)
{
  if (plateIndex < 0 || plateIndex >= plateNames_.size()) return false;
  while (plateSpiralModes_.size() <= plateIndex) plateSpiralModes_.append(0);
  plateSpiralModes_[plateIndex] = mode;
  emit projectChanged();
  return true;
}

// ── 首层耗材顺序（对齐上游 first_layer_print_sequence）──

int ProjectServiceMock::plateFirstLayerSeqChoice(int plateIndex) const
{
  return (plateIndex >= 0 && plateIndex < plateFirstLayerSeqChoices_.size()) ? plateFirstLayerSeqChoices_[plateIndex] : 0;
}

bool ProjectServiceMock::setPlateFirstLayerSeqChoice(int plateIndex, int choice)
{
  if (plateIndex < 0 || plateIndex >= plateNames_.size()) return false;
  while (plateFirstLayerSeqChoices_.size() <= plateIndex) plateFirstLayerSeqChoices_.append(0);
  plateFirstLayerSeqChoices_[plateIndex] = choice;
  emit projectChanged();
  return true;
}

QVariantList ProjectServiceMock::plateFirstLayerSeqOrder(int plateIndex) const
{
  QVariantList result;
  if (plateIndex >= 0 && plateIndex < plateFirstLayerSeqOrders_.size())
    for (int v : plateFirstLayerSeqOrders_[plateIndex])
      result.append(v);
  return result;
}

bool ProjectServiceMock::setPlateFirstLayerSeqOrder(int plateIndex, const QVariantList &order)
{
  if (plateIndex < 0 || plateIndex >= plateNames_.size()) return false;
  while (plateFirstLayerSeqOrders_.size() <= plateIndex) plateFirstLayerSeqOrders_.append(QList<int>{});
  QList<int> intOrder;
  for (const auto &v : order)
    intOrder.append(v.toInt());
  plateFirstLayerSeqOrders_[plateIndex] = intOrder;
  emit projectChanged();
  return true;
}

// ── 其他层耗材顺序（对齐上游 other_layers_print_sequence + LayerPrintSequence）──

int ProjectServiceMock::plateOtherLayersSeqChoice(int plateIndex) const
{
  return (plateIndex >= 0 && plateIndex < plateOtherLayersSeqChoices_.size()) ? plateOtherLayersSeqChoices_[plateIndex] : 0;
}

bool ProjectServiceMock::setPlateOtherLayersSeqChoice(int plateIndex, int choice)
{
  if (plateIndex < 0 || plateIndex >= plateNames_.size()) return false;
  while (plateOtherLayersSeqChoices_.size() <= plateIndex) plateOtherLayersSeqChoices_.append(0);
  plateOtherLayersSeqChoices_[plateIndex] = choice;
  emit projectChanged();
  return true;
}

int ProjectServiceMock::plateOtherLayersSeqCount(int plateIndex) const
{
  return (plateIndex >= 0 && plateIndex < plateOtherLayersSeqEntries_.size()) ? plateOtherLayersSeqEntries_[plateIndex].size() : 0;
}

int ProjectServiceMock::plateOtherLayersSeqBegin(int plateIndex, int entryIndex) const
{
  if (plateIndex < 0 || plateIndex >= plateOtherLayersSeqEntries_.size()) return 2;
  const auto &entries = plateOtherLayersSeqEntries_[plateIndex];
  if (entryIndex < 0 || entryIndex >= entries.size()) return 2;
  return entries[entryIndex].beginLayer;
}

int ProjectServiceMock::plateOtherLayersSeqEnd(int plateIndex, int entryIndex) const
{
  if (plateIndex < 0 || plateIndex >= plateOtherLayersSeqEntries_.size()) return 100;
  const auto &entries = plateOtherLayersSeqEntries_[plateIndex];
  if (entryIndex < 0 || entryIndex >= entries.size()) return 100;
  return entries[entryIndex].endLayer;
}

QVariantList ProjectServiceMock::plateOtherLayersSeqOrder(int plateIndex, int entryIndex) const
{
  QVariantList result;
  if (plateIndex >= 0 && plateIndex < plateOtherLayersSeqEntries_.size())
  {
    const auto &entries = plateOtherLayersSeqEntries_[plateIndex];
    if (entryIndex >= 0 && entryIndex < entries.size())
      for (int v : entries[entryIndex].extruderOrder)
        result.append(v);
  }
  return result;
}

bool ProjectServiceMock::addPlateOtherLayersSeqEntry(int plateIndex, int beginLayer, int endLayer)
{
  if (plateIndex < 0 || plateIndex >= plateNames_.size()) return false;
  if (beginLayer < 2) beginLayer = 2; // 对齐上游：begin_layer 必须 >= 2
  if (endLayer < beginLayer) endLayer = beginLayer;
  while (plateOtherLayersSeqEntries_.size() <= plateIndex) plateOtherLayersSeqEntries_.append(QList<MockLayerSeqEntry>{});
  MockLayerSeqEntry entry;
  entry.beginLayer = beginLayer;
  entry.endLayer = endLayer;
  // 默认顺序：按挤出机编号
  const int extCount = qMax(1, plateObjectCount(plateIndex));
  for (int i = 0; i < extCount; ++i)
    entry.extruderOrder.append(i);
  plateOtherLayersSeqEntries_[plateIndex].append(entry);
  // 自动排序（对齐上游 auto-sort）
  std::sort(plateOtherLayersSeqEntries_[plateIndex].begin(),
            plateOtherLayersSeqEntries_[plateIndex].end(),
            [](const MockLayerSeqEntry &a, const MockLayerSeqEntry &b) {
              return a.beginLayer < b.beginLayer;
            });
  emit projectChanged();
  return true;
}

bool ProjectServiceMock::removePlateOtherLayersSeqEntry(int plateIndex, int entryIndex)
{
  if (plateIndex < 0 || plateIndex >= plateOtherLayersSeqEntries_.size()) return false;
  auto &entries = plateOtherLayersSeqEntries_[plateIndex];
  if (entryIndex < 0 || entryIndex >= entries.size()) return false;
  entries.removeAt(entryIndex);
  emit projectChanged();
  return true;
}

bool ProjectServiceMock::setPlateOtherLayersSeqRange(int plateIndex, int entryIndex, int beginLayer, int endLayer)
{
  if (plateIndex < 0 || plateIndex >= plateOtherLayersSeqEntries_.size()) return false;
  auto &entries = plateOtherLayersSeqEntries_[plateIndex];
  if (entryIndex < 0 || entryIndex >= entries.size()) return false;
  if (beginLayer < 2) beginLayer = 2;
  if (endLayer < beginLayer) endLayer = beginLayer;
  entries[entryIndex].beginLayer = beginLayer;
  entries[entryIndex].endLayer = endLayer;
  // 自动排序
  std::sort(entries.begin(), entries.end(),
            [](const MockLayerSeqEntry &a, const MockLayerSeqEntry &b) {
              return a.beginLayer < b.beginLayer;
            });
  emit projectChanged();
  return true;
}

bool ProjectServiceMock::setPlateOtherLayersSeqOrder(int plateIndex, int entryIndex, const QVariantList &order)
{
  if (plateIndex < 0 || plateIndex >= plateOtherLayersSeqEntries_.size()) return false;
  auto &entries = plateOtherLayersSeqEntries_[plateIndex];
  if (entryIndex < 0 || entryIndex >= entries.size()) return false;
  QList<int> intOrder;
  for (const auto &v : order)
    intOrder.append(v.toInt());
  entries[entryIndex].extruderOrder = intOrder;
  emit projectChanged();
  return true;
}

int ProjectServiceMock::plateExtruderCount(int plateIndex) const
{
  // Mock 模式：基于平板上的对象数推断耗材数（上限 4）
  return qMin(qMax(1, plateObjectCount(plateIndex)), 4);
}

// ── 平板缩略图生成（对齐上游 PartPlate::thumbnail_data，Mock 模式使用 QPainter 合成）──

namespace
{
  // 确定性伪随机（基于种子，避免缩略图每帧抖动）
  static int seededRand(int &seed)
  {
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    return seed;
  }
}

QString ProjectServiceMock::generatePlateThumbnail(int plateIndex, int size)
{
  const int count = plateObjectCount(plateIndex);
  QImage img(size, size, QImage::Format_ARGB32_Premultiplied);
  img.fill(QColor(42, 42, 46)); // 深色背景

  QPainter p(&img);
  p.setRenderHint(QPainter::Antialiasing);

  // 绘制热床网格（对齐上游 plate 网格渲染）
  p.setPen(QPen(QColor(60, 60, 65), 1));
  const int gridSize = size / 8;
  for (int i = 1; i < 8; ++i)
  {
    p.drawLine(i * gridSize, 0, i * gridSize, size);
    p.drawLine(0, i * gridSize, size, i * gridSize);
  }

  if (count == 0)
  {
    // 空平板：绘制虚线边框
    p.setPen(QPen(QColor(100, 100, 100), 1, Qt::DashLine));
    p.setBrush(Qt::NoBrush);
    p.drawRect(4, 4, size - 8, size - 8);
    p.end();
    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    return QString::fromLatin1(ba.toBase64());
  }

  // 为每个对象绘制简化几何体
  const QColor objColors[] = {
    QColor(80, 180, 255),   // 蓝色
    QColor(255, 160, 60),   // 橙色
    QColor(120, 220, 120),  // 绿色
    QColor(220, 120, 220),  // 紫色
    QColor(255, 220, 80),   // 黄色
    QColor(255, 100, 100),  // 红色
    QColor(100, 220, 220),  // 青色
    QColor(200, 180, 160),  // 棕色
  };

  int randSeed = plateIndex * 1000 + 42;
  const int margin = 6;
  const int area = size - margin * 2;

  for (int i = 0; i < count && i < 8; ++i)
  {
    const QColor color = objColors[i % 8];
    // 基于对象位置生成确定性位置
    QVector3D pos = (plateIndex >= 0 && plateIndex < plateObjectIndices_.size())
      ? objectPositions_.value(plateObjectIndices_[plateIndex].value(i, 0), QVector3D(0, 0, 0))
      : QVector3D(0, 0, 0);

    // 简化布局：网格排列
    const int cols = qMin(count, 3);
    const int rows = (count + cols - 1) / cols;
    const int cellW = area / cols;
    const int cellH = area / rows;
    const int col = i % cols;
    const int row = i / cols;

    const int cx = margin + col * cellW + cellW / 2;
    const int cy = margin + row * cellH + cellH / 2;
    const int objSize = qMin(cellW, cellH) * 0.6;

    // 绘制阴影
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 40));
    p.drawRoundedRect(cx - objSize / 2 + 2, cy - objSize / 2 + 2, objSize, objSize, 4, 4);

    // 绘制对象轮廓（3D 立方体效果）
    p.setBrush(color.darker(120));
    p.drawRoundedRect(cx - objSize / 2, cy - objSize / 2, objSize, objSize, 4, 4);

    // 顶面高光
    p.setBrush(color);
    p.drawRoundedRect(cx - objSize / 2 + 2, cy - objSize / 2 + 2, objSize - 4, objSize * 0.5 - 2, 3, 3);

    // 对象序号
    p.setPen(QColor(255, 255, 255, 180));
    p.setFont(QFont("Arial", 8, QFont::Bold));
    p.drawText(QRect(cx - objSize / 2, cy - objSize / 2, objSize, objSize),
               Qt::AlignCenter, QString::number(i + 1));
  }

  // 已切片标记：绿色角标
  if (plateIndex >= 0 && plateIndex < plateObjectIndices_.size())
  {
    // Slice status is tracked by EditorViewModel; here we use a mock check
    bool sliced = false;
    // We can check if plate has settings that indicate slicing was done
    if (plateBedTypes_.size() > plateIndex) sliced = true; // simple heuristic for demo
    if (sliced)
    {
      p.setPen(Qt::NoPen);
      p.setBrush(QColor(40, 200, 80));
      p.drawEllipse(size - 14, 2, 12, 12);
      p.setPen(Qt::white);
      p.setFont(QFont("Arial", 7, QFont::Bold));
      p.drawText(QRect(size - 14, 2, 12, 12), Qt::AlignCenter, "✓");
    }
  }

  p.end();

  QByteArray ba;
  QBuffer buf(&ba);
  buf.open(QIODevice::WriteOnly);
  img.save(&buf, "PNG");
  return QString::fromLatin1(ba.toBase64());
}

int ProjectServiceMock::duplicateObject(int sourceIndex)
{
  if (loading_)
  {
    lastError_ = tr("加载中，无法复制对象");
    emit projectChanged();
    return -1;
  }

  if (sourceIndex < 0 || sourceIndex >= objectNames_.size())
  {
    lastError_ = tr("复制失败：对象索引无效");
    emit projectChanged();
    return -1;
  }

  try
  {
#ifdef HAS_LIBSLIC3R
    if (!model_ || size_t(sourceIndex) >= model_->objects.size() || !model_->objects[size_t(sourceIndex)])
    {
      lastError_ = tr("复制失败：模型对象无效");
      emit projectChanged();
      return -1;
    }

    auto *srcObj = model_->objects[size_t(sourceIndex)];

    // 对齐上游 Plater::clone_selection → Selection::paste_objects_from_clipboard
    // 深拷贝 ModelObject（含所有 volumes、config），并偏移新实例位置
    auto *newObj = model_->add_object(*srcObj);
    newObj->name = srcObj->name + " " + QObject::tr("(副本)").toStdString();

    // 给新对象的所有实例增加 X 偏移（匹配上游 find_displacements 的 20mm 间距）
    const double cloneOffset = 20.0;
    for (auto *inst : newObj->instances)
    {
      if (!inst)
        continue;
      inst->set_offset(Slic3r::Vec3d(
          inst->get_offset(Slic3r::X) + cloneOffset,
          inst->get_offset(Slic3r::Y),
          inst->get_offset(Slic3r::Z)));
    }

    // 重建元数据
    objectNames_.clear();
    objectNames_.reserve(int(model_->objects.size()));
    objectModuleNames_.clear();
    objectModuleNames_.reserve(int(model_->objects.size()));
    objectPrintableStates_.clear();
    objectVisibleStates_.clear();
    objectPositions_.clear();
    objectRotations_.clear();
    objectScales_.clear();
    objectPrintableStates_.reserve(int(model_->objects.size()));
    objectVisibleStates_.reserve(int(model_->objects.size()));
    for (size_t i = 0; i < model_->objects.size(); ++i)
    {
      const auto *obj = model_->objects[i];
      if (obj && !obj->name.empty())
        objectNames_ << QString::fromStdString(obj->name);
      else
        objectNames_ << tr("对象 %1").arg(int(i + 1));

      if (obj && !obj->module_name.empty())
        objectModuleNames_ << QString::fromStdString(obj->module_name);
      else
        objectModuleNames_ << tr("默认模块");

      const bool printable = obj && !obj->instances.empty() ? obj->instances.front()->printable : true;
      objectPrintableStates_.append(printable);

      const bool vis = obj && !obj->instances.empty() ? obj->instances.front()->is_printable() : true;
      objectVisibleStates_.append(vis);

      // P0.5.1: 从真实实例同步变换（slic3r → GL）
      if (obj && !obj->instances.empty() && obj->instances.front())
      {
        const auto *inst = obj->instances.front();
        const auto off = inst->get_offset();
        objectPositions_.append(QVector3D(
          static_cast<float>(off.x()), static_cast<float>(off.z()), static_cast<float>(off.y())));
        const auto rot = inst->get_rotation();
        objectRotations_.append(QVector3D(
          qRadiansToDegrees(static_cast<float>(rot.x())),
          qRadiansToDegrees(static_cast<float>(rot.y())),
          qRadiansToDegrees(static_cast<float>(rot.z()))));
        const auto sc = inst->get_scaling_factor();
        objectScales_.append(QVector3D(
          static_cast<float>(sc.x()), static_cast<float>(sc.y()), static_cast<float>(sc.z())));
      }
      else
      {
        objectPositions_.append(QVector3D(0, 0, 0));
        objectRotations_.append(QVector3D(0, 0, 0));
        objectScales_.append(QVector3D(1, 1, 1));
      }
    }

    const int newIndex = int(model_->objects.size()) - 1;
#else
    Q_UNUSED(sourceIndex);

    // Mock 模式：复制元数据
    const QString newName = objectNames_[sourceIndex] + " " + tr("(副本)");
    const QString moduleName = objectModuleNames_[sourceIndex];
    const bool printable = objectPrintableStates_[sourceIndex];
    const bool vis = sourceIndex < objectVisibleStates_.size() ? objectVisibleStates_[sourceIndex] : true;

    // 在 sourceIndex+1 处插入
    const int insertPos = sourceIndex + 1;
    objectNames_.insert(insertPos, newName);
    objectModuleNames_.insert(insertPos, moduleName);
    objectPrintableStates_.insert(insertPos, printable);
    objectVisibleStates_.insert(insertPos, vis);

    // 更新 plate 对象索引（所有 > sourceIndex 的索引 +1）
    for (auto &plateObjList : plateObjectIndices_)
    {
      for (int j = 0; j < plateObjList.size(); ++j)
      {
        if (plateObjList[j] > sourceIndex)
          ++plateObjList[j];
      }
    }

    // 将新对象加入当前 plate
    if (currentPlateIndex_ >= 0 && currentPlateIndex_ < plateObjectIndices_.size())
    {
      plateObjectIndices_[currentPlateIndex_].append(insertPos);
      std::sort(plateObjectIndices_[currentPlateIndex_].begin(),
                plateObjectIndices_[currentPlateIndex_].end());
    }

    const int newIndex = insertPos;
#endif

    modelCount_ = objectNames_.size();
    lastError_.clear();
    emit projectChanged();
    emit plateDataLoaded(plateCount_);
    return newIndex;
  }
  catch (const std::exception &ex)
  {
    lastError_ = QString::fromStdString(ex.what());
    emit projectChanged();
    return -1;
  }
  catch (...)
  {
    lastError_ = tr("复制失败：未知错误");
    emit projectChanged();
    return -1;
  }
}

QList<int> ProjectServiceMock::splitObject(int objectIndex)
{
  QList<int> newIndices;

  if (loading_)
  {
    lastError_ = tr("加载中，无法拆分对象");
    emit projectChanged();
    return newIndices;
  }

  if (objectIndex < 0 || objectIndex >= objectNames_.size())
  {
    lastError_ = tr("拆分失败：对象索引无效");
    emit projectChanged();
    return newIndices;
  }

  try
  {
#ifdef HAS_LIBSLIC3R
    if (!model_ || size_t(objectIndex) >= model_->objects.size() || !model_->objects[size_t(objectIndex)])
    {
      lastError_ = tr("拆分失败：模型对象无效");
      emit projectChanged();
      return newIndices;
    }

    auto *obj = model_->objects[size_t(objectIndex)];

    // 对齐上游 Plater::priv::split_object()
    // 在模型克隆上执行 split（对齐上游：clone model to avoid duplicate volumes）
    Slic3r::Model tempModel = *model_;
    auto *tempObj = tempModel.objects[size_t(objectIndex)];

    Slic3r::ModelObjectPtrs newObjects;
    tempObj->split(&newObjects);

    // 对齐上游：如果只有一个结果，表示无法拆分
    if (newObjects.size() <= 1)
    {
      lastError_.clear();
      emit projectChanged();
      return newIndices;
    }

    // 对齐上游：移除原对象
    model_->delete_object(size_t(objectIndex));

    // 将拆分后的新对象添加到真实 model_
    for (auto *newObj : newObjects)
    {
      if (!newObj)
        continue;
      model_->add_object(*newObj);

      // 将新对象加入当前 plate
      if (currentPlateIndex_ >= 0 && currentPlateIndex_ < plateObjectIndices_.size())
      {
        plateObjectIndices_[currentPlateIndex_].append(int(model_->objects.size()) - 1);
      }
    }

    // 重建元数据
    objectNames_.clear();
    objectNames_.reserve(int(model_->objects.size()));
    objectModuleNames_.clear();
    objectModuleNames_.reserve(int(model_->objects.size()));
    objectPrintableStates_.clear();
    objectPrintableStates_.reserve(int(model_->objects.size()));
    objectVisibleStates_.clear();
    objectVisibleStates_.reserve(int(model_->objects.size()));
    for (size_t i = 0; i < model_->objects.size(); ++i)
    {
      const auto *o = model_->objects[i];
      if (o && !o->name.empty())
        objectNames_ << QString::fromStdString(o->name);
      else
        objectNames_ << tr("对象 %1").arg(int(i + 1));

      if (o && !o->module_name.empty())
        objectModuleNames_ << QString::fromStdString(o->module_name);
      else
        objectModuleNames_ << tr("默认模块");

      const bool printable = o && !o->instances.empty() ? o->instances.front()->printable : true;
      objectPrintableStates_.append(printable);

      const bool vis = o && !o->instances.empty() ? o->instances.front()->is_printable() : true;
      objectVisibleStates_.append(vis);
    }

    // 新对象从 objectIndex 位置开始
    for (size_t i = size_t(objectIndex); i < model_->objects.size(); ++i)
      newIndices.append(int(i));

    // 同步变换数组
    syncTransformsFromModel();

    modelCount_ = objectNames_.size();
#else
    Q_UNUSED(objectIndex);
#endif

    lastError_.clear();
    emit projectChanged();
    return newIndices;
  }
  catch (const std::exception &ex)
  {
    lastError_ = QString::fromStdString(ex.what());
    emit projectChanged();
    return newIndices;
  }
  catch (...)
  {
    lastError_ = tr("拆分失败：未知错误");
    emit projectChanged();
    return newIndices;
  }
}

int ProjectServiceMock::addObject(const QString &name)
{
  if (loading_)
  {
    lastError_ = tr("加载中，无法添加对象");
    emit projectChanged();
    return -1;
  }

#ifdef HAS_LIBSLIC3R
  // 真实模式：通过 libslic3r Model 添加空对象
  try
  {
    if (!model_)
    {
      model_ = new Slic3r::Model();
      model_->add_default_instances();
    }
    auto *obj = model_->add_object();
    obj->name = name.toStdString();
    obj->module_name = "default";
    auto *inst = obj->add_instance();
    inst->set_offset(Slic3r::Vec3d(0, 0, 0));

    // 重建元数据
    objectNames_.clear();
    objectModuleNames_.clear();
    objectPrintableStates_.clear();
    objectVisibleStates_.clear();
    objectPositions_.clear();
    objectRotations_.clear();
    objectScales_.clear();
    for (size_t i = 0; i < model_->objects.size(); ++i)
    {
      const auto *o = model_->objects[i];
      objectNames_ << (o ? QString::fromStdString(o->name) : tr("对象 %1").arg(int(i + 1)));
      objectModuleNames_ << (o ? QString::fromStdString(o->module_name) : tr("默认模块"));
      const bool pr = o && !o->instances.empty() ? o->instances.front()->printable : true;
      objectPrintableStates_.append(pr);
      objectVisibleStates_.append(true);
      // P0.5.1: 从真实实例同步变换（slic3r → GL）
      if (o && !o->instances.empty() && o->instances.front())
      {
        const auto *inst = o->instances.front();
        const auto off = inst->get_offset();
        objectPositions_.append(QVector3D(
          static_cast<float>(off.x()), static_cast<float>(off.z()), static_cast<float>(off.y())));
        const auto rot = inst->get_rotation();
        objectRotations_.append(QVector3D(
          qRadiansToDegrees(static_cast<float>(rot.x())),
          qRadiansToDegrees(static_cast<float>(rot.y())),
          qRadiansToDegrees(static_cast<float>(rot.z()))));
        const auto sc = inst->get_scaling_factor();
        objectScales_.append(QVector3D(
          static_cast<float>(sc.x()), static_cast<float>(sc.y()), static_cast<float>(sc.z())));
      }
      else
      {
        objectPositions_.append(QVector3D(0, 0, 0));
        objectRotations_.append(QVector3D(0, 0, 0));
        objectScales_.append(QVector3D(1, 1, 1));
      }
    }

    const int newIndex = int(model_->objects.size()) - 1;
    modelCount_ = objectNames_.size();
    emit projectChanged();
    return newIndex;
  }
  catch (const std::exception &ex)
  {
    lastError_ = QString::fromStdString(ex.what());
    emit projectChanged();
    return -1;
  }
#else
  Q_UNUSED(name);
  // Mock 模式：添加一个空对象到当前平板末尾
  const QString newName = tr("新对象");
  const int insertPos = objectNames_.size();
  objectNames_.append(newName);
  objectModuleNames_.append(tr("默认模块"));
  objectPrintableStates_.append(true);
  objectVisibleStates_.append(true);
  objectPositions_.append(QVector3D(0, 0, 0));
  objectRotations_.append(QVector3D(0, 0, 0));
  objectScales_.append(QVector3D(1, 1, 1));

  if (currentPlateIndex_ >= 0 && currentPlateIndex_ < plateObjectIndices_.size())
  {
    plateObjectIndices_[currentPlateIndex_].append(insertPos);
  }

  modelCount_ = objectNames_.size();
  lastError_.clear();
  emit projectChanged();
  emit plateDataLoaded(plateCount_);
  return insertPos;
#endif
}

QVector3D ProjectServiceMock::objectPosition(int index) const
{
#ifdef HAS_LIBSLIC3R
  if (model_ && index >= 0 && size_t(index) < model_->objects.size() &&
      model_->objects[size_t(index)] && !model_->objects[size_t(index)]->instances.empty())
  {
    const auto *inst = model_->objects[size_t(index)]->instances.front();
    if (inst)
    {
      const auto off = inst->get_offset();
      // slic3r(X,Y,Z) → GL(X,Z,Y): GL.x=slic3r.x, GL.y=slic3r.z, GL.z=slic3r.y
      return QVector3D(static_cast<float>(off.x()), static_cast<float>(off.z()), static_cast<float>(off.y()));
    }
  }
#endif
  return (index >= 0 && index < objectPositions_.size()) ? objectPositions_[index] : QVector3D(0, 0, 0);
}

QVector3D ProjectServiceMock::objectRotation(int index) const
{
#ifdef HAS_LIBSLIC3R
  if (model_ && index >= 0 && size_t(index) < model_->objects.size() &&
      model_->objects[size_t(index)] && !model_->objects[size_t(index)]->instances.empty())
  {
    const auto *inst = model_->objects[size_t(index)]->instances.front();
    if (inst)
    {
      const auto rot = inst->get_rotation(); // radians in slic3r space
      return QVector3D(
        qRadiansToDegrees(static_cast<float>(rot.x())),
        qRadiansToDegrees(static_cast<float>(rot.y())),
        qRadiansToDegrees(static_cast<float>(rot.z())));
    }
  }
#endif
  return (index >= 0 && index < objectRotations_.size()) ? objectRotations_[index] : QVector3D(0, 0, 0);
}

QVector3D ProjectServiceMock::objectScale(int index) const
{
#ifdef HAS_LIBSLIC3R
  if (model_ && index >= 0 && size_t(index) < model_->objects.size() &&
      model_->objects[size_t(index)] && !model_->objects[size_t(index)]->instances.empty())
  {
    const auto *inst = model_->objects[size_t(index)]->instances.front();
    if (inst)
    {
      const auto sc = inst->get_scaling_factor();
      return QVector3D(static_cast<float>(sc.x()), static_cast<float>(sc.y()), static_cast<float>(sc.z()));
    }
  }
#endif
  return (index >= 0 && index < objectScales_.size()) ? objectScales_[index] : QVector3D(1, 1, 1);
}

bool ProjectServiceMock::setObjectPosition(int index, float x, float y, float z)
{
  if (index < 0 || index >= objectNames_.size())
    return false;
  // P0.5.1: 写入真实 ModelInstance 并同步 Mock 数组
#ifdef HAS_LIBSLIC3R
  if (model_ && size_t(index) < model_->objects.size() &&
      model_->objects[size_t(index)] && !model_->objects[size_t(index)]->instances.empty())
  {
    auto *inst = model_->objects[size_t(index)]->instances.front();
    if (inst)
    {
      // GL(X,Z,Y) → slic3r(X,Y,Z): slic3r.x=GL.x, slic3r.y=GL.z, slic3r.z=GL.y
      inst->set_offset(Slic3r::Vec3d(x, z, y));
    }
  }
#endif
  while (objectPositions_.size() <= index)
    objectPositions_.append(QVector3D(0, 0, 0));
  objectPositions_[index] = QVector3D(x, y, z);
  emit projectChanged();
  return true;
}

bool ProjectServiceMock::setObjectRotation(int index, float x, float y, float z)
{
  if (index < 0 || index >= objectNames_.size())
    return false;
#ifdef HAS_LIBSLIC3R
  if (model_ && size_t(index) < model_->objects.size() &&
      model_->objects[size_t(index)] && !model_->objects[size_t(index)]->instances.empty())
  {
    auto *inst = model_->objects[size_t(index)]->instances.front();
    if (inst)
    {
      // 角度 → 弧度
      inst->set_rotation(Slic3r::Vec3d(
        qDegreesToRadians(x), qDegreesToRadians(y), qDegreesToRadians(z)));
    }
  }
#endif
  while (objectRotations_.size() <= index)
    objectRotations_.append(QVector3D(0, 0, 0));
  objectRotations_[index] = QVector3D(x, y, z);
  emit projectChanged();
  return true;
}

bool ProjectServiceMock::setObjectScale(int index, float x, float y, float z)
{
  if (index < 0 || index >= objectNames_.size())
    return false;
#ifdef HAS_LIBSLIC3R
  if (model_ && size_t(index) < model_->objects.size() &&
      model_->objects[size_t(index)] && !model_->objects[size_t(index)]->instances.empty())
  {
    auto *inst = model_->objects[size_t(index)]->instances.front();
    if (inst)
    {
      inst->set_scaling_factor(Slic3r::Vec3d(x, y, z));
    }
  }
#endif
  while (objectScales_.size() <= index)
    objectScales_.append(QVector3D(1, 1, 1));
  objectScales_[index] = QVector3D(x, y, z);
  emit projectChanged();
  return true;
}

bool ProjectServiceMock::setObjectScaleUniform(int index, float s)
{
  return setObjectScale(index, s, s, s);
}

void ProjectServiceMock::clearProject()
{
  if (loading_)
    cancelLoad();

  projectName_ = tr("未命名项目");
  modelCount_ = 0;
  plateCount_ = 0;
  currentPlateIndex_ = -1;
  sourceFilePath_.clear();
  objectNames_.clear();
  objectModuleNames_.clear();
  objectPrintableStates_.clear();
  objectVisibleStates_.clear();
  plateNames_.clear();
  plateObjectIndices_.clear();
  plateLockedStates_.clear();
  plateFirstLayerSeqChoices_.clear();
  plateFirstLayerSeqOrders_.clear();
  plateOtherLayersSeqChoices_.clear();
  plateOtherLayersSeqEntries_.clear();
  objectPositions_.clear();
  objectRotations_.clear();
  objectScales_.clear();
  m_mockVolumes.clear();
  lastError_.clear();
  loadProgress_ = 0;
  loading_ = false;
  activeCancelFlag_.reset();

#ifdef HAS_LIBSLIC3R
  if (model_)
  {
    delete model_;
    model_ = new Slic3r::Model();
  }
#endif

  emit loadingChanged();
  emit loadProgressChanged();
  emit projectChanged();
  emit plateDataLoaded(0);
  emit plateSelectionChanged();
}

bool ProjectServiceMock::saveProject(const QString &filePath)
{
  if (loading_)
  {
    lastError_ = tr("加载中，无法保存");
    emit projectChanged();
    return false;
  }

#ifdef HAS_LIBSLIC3R
  // TODO: 调用真实 3MF writer (bbs_3mf export)
  // wxGetApp().plater()->save_project(filePath) equivalent
  lastError_ = tr("3MF 导出尚未实现");
  emit projectChanged();
  return false;
#else
  // Mock mode: save project metadata as JSON
  if (filePath.isEmpty())
  {
    lastError_ = tr("保存路径为空");
    emit projectChanged();
    return false;
  }

  QJsonDocument doc;
  QJsonObject root;

  root[QStringLiteral("name")] = projectName_;
  root[QStringLiteral("source")] = sourceFilePath_;
  root[QStringLiteral("currentPlate")] = currentPlateIndex_;

  // Save plates
  QJsonArray platesArr;
  for (int p = 0; p < plateCount_; ++p)
  {
    QJsonObject plateObj;
    plateObj[QStringLiteral("name")] = plateNames_.value(p, QStringLiteral("Plate %1").arg(p + 1));
    plateObj[QStringLiteral("locked")] = plateLockedStates_.value(p, false);
    plateObj[QStringLiteral("bedType")] = plateBedTypes_.value(p, 0);
    plateObj[QStringLiteral("printSequence")] = platePrintSequences_.value(p, 0);
    plateObj[QStringLiteral("spiralMode")] = plateSpiralModes_.value(p, 0);
    plateObj[QStringLiteral("firstLayerSeqChoice")] = plateFirstLayerSeqChoices_.value(p, 0);
    // Save first layer extruder order
    {
      QJsonArray seqArr;
      if (p < plateFirstLayerSeqOrders_.size())
        for (int v : plateFirstLayerSeqOrders_[p]) seqArr.append(v);
      plateObj[QStringLiteral("firstLayerSeqOrder")] = seqArr;
    }
    plateObj[QStringLiteral("otherLayersSeqChoice")] = plateOtherLayersSeqChoices_.value(p, 0);
    // Save other layers sequence entries
    {
      QJsonArray seqEntriesArr;
      if (p < plateOtherLayersSeqEntries_.size())
      {
        for (const auto &entry : plateOtherLayersSeqEntries_[p])
        {
          QJsonObject entryObj;
          entryObj[QStringLiteral("beginLayer")] = entry.beginLayer;
          entryObj[QStringLiteral("endLayer")] = entry.endLayer;
          QJsonArray orderArr;
          for (int v : entry.extruderOrder) orderArr.append(v);
          entryObj[QStringLiteral("order")] = orderArr;
          seqEntriesArr.append(entryObj);
        }
      }
      plateObj[QStringLiteral("otherLayersSeqEntries")] = seqEntriesArr;
    }

    QJsonArray objsArr;
    for (int oi : plateObjectIndices_.value(p, QList<int>{}))
    {
      QJsonObject objObj;
      objObj[QStringLiteral("name")] = objectNames_.value(oi, QString());
      objObj[QStringLiteral("module")] = objectModuleNames_.value(oi, QString());
      objObj[QStringLiteral("printable")] = objectPrintableStates_.value(oi, true);
      objObj[QStringLiteral("visible")] = objectVisibleStates_.value(oi, true);

      // Save transforms
      const QVector3D pos = objectPositions_.value(oi, QVector3D(0, 0, 0));
      const QVector3D rot = objectRotations_.value(oi, QVector3D(0, 0, 0));
      const QVector3D scl = objectScales_.value(oi, QVector3D(1, 1, 1));
      objObj[QStringLiteral("position")] = QJsonArray{pos.x(), pos.y(), pos.z()};
      objObj[QStringLiteral("rotation")] = QJsonArray{rot.x(), rot.y(), rot.z()};
      objObj[QStringLiteral("scale")] = QJsonArray{scl.x(), scl.y(), scl.z()};

      // Save per-object overrides
      const auto oit = m_mockObjectOverrides.constFind(oi);
      if (oit != m_mockObjectOverrides.constEnd() && !oit->isEmpty())
      {
        QJsonObject overrides;
        for (auto kvIt = oit->constBegin(); kvIt != oit->constEnd(); ++kvIt)
          overrides[kvIt.key()] = QJsonValue::fromVariant(kvIt.value());
        objObj[QStringLiteral("overrides")] = overrides;
      }

      objsArr.append(objObj);
    }
    plateObj[QStringLiteral("objects")] = objsArr;
    platesArr.append(plateObj);
  }
  root[QStringLiteral("plates")] = platesArr;

  doc.setObject(root);

  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
  {
    lastError_ = tr("无法打开文件: %1").arg(filePath);
    emit projectChanged();
    return false;
  }

  const QByteArray data = doc.toJson(QJsonDocument::Indented);
  if (file.write(data) != data.size())
  {
    lastError_ = tr("写入文件失败");
    file.close();
    emit projectChanged();
    return false;
  }

  file.close();
  sourceFilePath_ = filePath;
  // Derive project name from path
  QFileInfo fi(filePath);
  projectName_ = fi.completeBaseName();
  lastError_.clear();
  emit projectChanged();
  return true;
#endif
}

bool ProjectServiceMock::loadProject(const QString &filePath)
{
  if (filePath.isEmpty())
  {
    lastError_ = tr("加载路径为空");
    return false;
  }

  QFileInfo fi(filePath);
  if (!fi.exists())
  {
    lastError_ = tr("文件不存在: %1").arg(filePath);
    return false;
  }

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly))
  {
    lastError_ = tr("无法打开文件: %1").arg(filePath);
    return false;
  }

  const QByteArray raw = file.readAll();
  file.close();

  QJsonParseError parseErr;
  const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseErr);
  if (doc.isNull())
  {
    lastError_ = tr("JSON 解析失败: %1").arg(parseErr.errorString());
    return false;
  }

  const QJsonObject root = doc.object();

  // Clear existing state
  objectNames_.clear();
  objectModuleNames_.clear();
  objectPrintableStates_.clear();
  objectVisibleStates_.clear();
  objectPositions_.clear();
  objectRotations_.clear();
  objectScales_.clear();
  plateNames_.clear();
  plateObjectIndices_.clear();
  plateLockedStates_.clear();
  plateFirstLayerSeqChoices_.clear();
  plateFirstLayerSeqOrders_.clear();
  plateOtherLayersSeqChoices_.clear();
  plateOtherLayersSeqEntries_.clear();
  m_mockObjectOverrides.clear();
  m_mockVolumeOverrides.clear();
  m_mockPlateOverrides.clear();

#ifdef HAS_LIBSLIC3R
  // JSON 项目加载不包含真实 3D 数据，重置 model_ 避免读取过期对象
  if (model_)
  {
    delete model_;
    model_ = nullptr;
  }
#endif

  // Restore project name
  projectName_ = root.value(QStringLiteral("name")).toString(fi.completeBaseName());
  sourceFilePath_ = root.value(QStringLiteral("source")).toString(filePath);

  // Restore plates and objects
  const QJsonArray platesArr = root.value(QStringLiteral("plates")).toArray();
  int globalObjectIdx = 0;
  for (int p = 0; p < platesArr.size(); ++p)
  {
    const QJsonObject plateObj = platesArr[p].toObject();
    plateNames_.append(plateObj.value(QStringLiteral("name")).toString(
                         QStringLiteral("Plate %1").arg(p + 1)));
    plateLockedStates_.append(plateObj.value(QStringLiteral("locked")).toBool(false));
    plateBedTypes_.append(plateObj.value(QStringLiteral("bedType")).toInt(0));
    platePrintSequences_.append(plateObj.value(QStringLiteral("printSequence")).toInt(0));
    plateSpiralModes_.append(plateObj.value(QStringLiteral("spiralMode")).toInt(0));
    plateFirstLayerSeqChoices_.append(plateObj.value(QStringLiteral("firstLayerSeqChoice")).toInt(0));
    // Restore first layer extruder order
    {
      QList<int> order;
      const QJsonArray orderArr = plateObj.value(QStringLiteral("firstLayerSeqOrder")).toArray();
      for (const auto &v : orderArr) order.append(v.toInt());
      plateFirstLayerSeqOrders_.append(order);
    }
    plateOtherLayersSeqChoices_.append(plateObj.value(QStringLiteral("otherLayersSeqChoice")).toInt(0));
    // Restore other layers sequence entries
    {
      QList<MockLayerSeqEntry> entries;
      const QJsonArray entriesArr = plateObj.value(QStringLiteral("otherLayersSeqEntries")).toArray();
      for (const auto &entryVal : entriesArr)
      {
        const QJsonObject entryObj = entryVal.toObject();
        MockLayerSeqEntry entry;
        entry.beginLayer = entryObj.value(QStringLiteral("beginLayer")).toInt(2);
        entry.endLayer = entryObj.value(QStringLiteral("endLayer")).toInt(100);
        const QJsonArray orderArr = entryObj.value(QStringLiteral("order")).toArray();
        for (const auto &v : orderArr) entry.extruderOrder.append(v.toInt());
        entries.append(entry);
      }
      plateOtherLayersSeqEntries_.append(entries);
    }

    QList<int> objIndices;
    const QJsonArray objsArr = plateObj.value(QStringLiteral("objects")).toArray();
    for (int oi = 0; oi < objsArr.size(); ++oi)
    {
      const QJsonObject objObj = objsArr[oi].toObject();
      const int idx = globalObjectIdx++;

      objectNames_.append(objObj.value(QStringLiteral("name")).toString(
                            QStringLiteral("Object %1").arg(idx + 1)));
      objectModuleNames_.append(objObj.value(QStringLiteral("module")).toString());
      objectPrintableStates_.append(objObj.value(QStringLiteral("printable")).toBool(true));
      objectVisibleStates_.append(objObj.value(QStringLiteral("visible")).toBool(true));

      // Restore transforms
      const QJsonArray posArr = objObj.value(QStringLiteral("position")).toArray();
      objectPositions_.append(posArr.size() == 3
        ? QVector3D(float(posArr[0].toDouble()), float(posArr[1].toDouble()), float(posArr[2].toDouble()))
        : QVector3D(0, 0, 0));
      const QJsonArray rotArr = objObj.value(QStringLiteral("rotation")).toArray();
      objectRotations_.append(rotArr.size() == 3
        ? QVector3D(float(rotArr[0].toDouble()), float(rotArr[1].toDouble()), float(rotArr[2].toDouble()))
        : QVector3D(0, 0, 0));
      const QJsonArray sclArr = objObj.value(QStringLiteral("scale")).toArray();
      objectScales_.append(sclArr.size() == 3
        ? QVector3D(float(sclArr[0].toDouble()), float(sclArr[1].toDouble()), float(sclArr[2].toDouble()))
        : QVector3D(1, 1, 1));

      // Restore per-object overrides
      const QJsonObject overrides = objObj.value(QStringLiteral("overrides")).toObject();
      if (!overrides.isEmpty())
      {
        QHash<QString, QVariant> overrideMap;
        for (auto it = overrides.constBegin(); it != overrides.constEnd(); ++it)
          overrideMap[it.key()] = it.value().toVariant();
        m_mockObjectOverrides[idx] = overrideMap;
      }

      objIndices.append(idx);
    }
    plateObjectIndices_.append(objIndices);
  }

  plateCount_ = plateNames_.size();
  modelCount_ = objectNames_.size();
  currentPlateIndex_ = qBound(0, root.value(QStringLiteral("currentPlate")).toInt(0),
                              qMax(0, plateCount_ - 1));
  loadProgress_ = 100;
  loading_ = false;
  lastError_.clear();

  emit projectChanged();
  emit plateDataLoaded(plateCount_);
  emit plateSelectionChanged();
  emit loadProgressChanged();
  emit loadingChanged();
  emit loadFinished(true, filePath);
  return true;
}

QByteArray ProjectServiceMock::meshData() const
{
#ifdef HAS_LIBSLIC3R
  if (!model_ || model_->objects.empty())
    return {};

  try
  {

    // ── TLV 格式 ─────────────────────────────────────────────────────────────
    // [int32 objectCount]
    // per object: [int32 objectId][int32 triangleCount][float * triangleCount*9]
    // trailer:    [float * 6] bbox (minGLx, minGLy, minGLz, maxGLx, maxGLy, maxGLz)
    //
    // 坐标转换: slic3r(X=right, Y=forward, Z=up) → GL(X=right, Y=up, Z=toward)
    //   GL.x = slic3r.x,  GL.y = slic3r.z,  GL.z = slic3r.y
    // ──────────────────────────────────────────────────────────────────────────

    struct ObjBatch
    {
      int32_t objectId;
      std::vector<float> verts; // 9 floats per triangle (3 verts × xyz)
    };

    std::vector<ObjBatch> batches;
    batches.reserve(8);

    float bminX = 1e30f, bminY = 1e30f, bminZ = 1e30f;
    float bmaxX = -1e30f, bmaxY = -1e30f, bmaxZ = -1e30f;

    std::unordered_set<int32_t> usedObjectIds;
    usedObjectIds.reserve(model_->objects.size() * 2 + 1);

    auto makeStableObjectId = [&usedObjectIds](const Slic3r::ModelObject *obj,
                                               const Slic3r::ModelInstance *inst) -> int32_t
    {
      const auto objAddr = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(obj));
      const auto instAddr = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(inst));
      uint64_t mixed = objAddr ^ (instAddr + 0x9e3779b97f4a7c15ULL + (objAddr << 6) + (objAddr >> 2));

      int32_t id = int32_t(mixed & 0x7fffffffULL);
      if (id == 0)
        id = 1;

      while (usedObjectIds.find(id) != usedObjectIds.end())
        id = (id == std::numeric_limits<int32_t>::max()) ? 1 : (id + 1);

      usedObjectIds.insert(id);
      return id;
    };

    for (const auto *obj : model_->objects)
    {
      if (!obj)
        continue;

      auto appendObjectByInstance = [&](const Slic3r::Transform3d &instMat,
                                        const Slic3r::ModelInstance *inst)
      {
        ObjBatch batch;
        batch.objectId = makeStableObjectId(obj, inst);

        for (const auto *vol : obj->volumes)
        {
          if (!vol)
            continue;

          // volume 自身的局部变换
          const Slic3r::Transform3d &volMat = vol->get_transformation().get_matrix();
          const Slic3r::Transform3d combined = instMat * volMat;

          const auto &its = vol->mesh().its; // const ref — 零拷贝
          if (its.vertices.empty() || its.indices.empty())
            continue;

          for (const auto &face : its.indices)
          {
            const int idx0 = face(0);
            const int idx1 = face(1);
            const int idx2 = face(2);
            const int vcount = int(its.vertices.size());
            if (idx0 < 0 || idx0 >= vcount ||
                idx1 < 0 || idx1 >= vcount ||
                idx2 < 0 || idx2 >= vcount)
            {
              continue;
            }

            for (int k = 0; k < 3; ++k)
            {
              const Slic3r::Vec3f &lv = its.vertices[face(k)];
              // 应用实例+Volume 变换到世界坐标 (slic3r)
              // Transform3d * Vec3d 直接应用仿射变换（含平移）
              const Slic3r::Vec3d hw = combined * Slic3r::Vec3d(
                                                      (double)lv.x(), (double)lv.y(), (double)lv.z());

              // slic3r → GL 坐标系
              const float gx = (float)hw.x();
              const float gy = (float)hw.z(); // slic3r Z → GL Y (高度)
              const float gz = (float)hw.y(); // slic3r Y → GL Z

              batch.verts.push_back(gx);
              batch.verts.push_back(gy);
              batch.verts.push_back(gz);

              bminX = std::min(bminX, gx);
              bmaxX = std::max(bmaxX, gx);
              bminY = std::min(bminY, gy);
              bmaxY = std::max(bmaxY, gy);
              bminZ = std::min(bminZ, gz);
              bmaxZ = std::max(bmaxZ, gz);
            }
          }
        }

        if (!batch.verts.empty())
          batches.push_back(std::move(batch));
      };

      if (!obj->instances.empty())
      {
        for (const auto *inst : obj->instances)
        {
          if (!inst)
            continue;
          appendObjectByInstance(inst->get_transformation().get_matrix(), inst);
        }
      }
      else
      {
        appendObjectByInstance(Slic3r::Transform3d::Identity(), nullptr);
      }
    }

    if (batches.empty())
      return {};

    // Auto-normalize model placement onto build plate:
    // keep relative arrangement, move global center to (110, *, 110)
    // and make lowest Y rest on the plate (Y=0).
    const float centerX = (bminX + bmaxX) * 0.5f;
    const float centerZ = (bminZ + bmaxZ) * 0.5f;
    const float offsetX = 110.0f - centerX;
    const float offsetZ = 110.0f - centerZ;
    const float offsetY = (bminY < 0.0f) ? (-bminY) : 0.0f;

    if (offsetX != 0.0f || offsetY != 0.0f || offsetZ != 0.0f)
    {
      for (auto &batch : batches)
      {
        for (size_t vi = 0; vi + 2 < batch.verts.size(); vi += 3)
        {
          batch.verts[vi + 0] += offsetX;
          batch.verts[vi + 1] += offsetY;
          batch.verts[vi + 2] += offsetZ;
        }
      }

      bminX += offsetX;
      bmaxX += offsetX;
      bminY += offsetY;
      bmaxY += offsetY;
      bminZ += offsetZ;
      bmaxZ += offsetZ;
    }

    // ── 计算缓冲区大小并写入 ──────────────────────────────────────────────────
    if (batches.size() > size_t(std::numeric_limits<int32_t>::max()))
    {
      qWarning("[ProjectService] meshData aborted: too many batches (%zu)", batches.size());
      return {};
    }

    int32_t objCount = static_cast<int32_t>(batches.size());
    qsizetype totalBytes = static_cast<qsizetype>(sizeof(int32_t)); // objectCount header
    for (const auto &b : batches)
    {
      const qsizetype dataBytes = static_cast<qsizetype>(b.verts.size()) * static_cast<qsizetype>(sizeof(float));
      if (dataBytes < 0 || dataBytes > (std::numeric_limits<qsizetype>::max)() - totalBytes)
      {
        qWarning("[ProjectService] meshData aborted: size overflow while packing vertices");
        return {};
      }
      const qsizetype perObjBytes = static_cast<qsizetype>(2 * sizeof(int32_t));
      if (perObjBytes > (std::numeric_limits<qsizetype>::max)() - totalBytes - dataBytes)
      {
        qWarning("[ProjectService] meshData aborted: size overflow while packing object header");
        return {};
      }
      totalBytes += perObjBytes + dataBytes;
    }
    const qsizetype trailerBytes = static_cast<qsizetype>(6 * sizeof(float));
    if (trailerBytes > (std::numeric_limits<qsizetype>::max)() - totalBytes)
    {
      qWarning("[ProjectService] meshData aborted: size overflow while packing bbox");
      return {};
    }
    totalBytes += trailerBytes;

    if (totalBytes <= 0 || totalBytes > (std::numeric_limits<int>::max)())
    {
      qWarning("[ProjectService] meshData aborted: invalid total buffer size (%lld)", static_cast<long long>(totalBytes));
      return {};
    }

    QByteArray buf;
    buf.resize(static_cast<int>(totalBytes));
    char *p = buf.data();

    // objectCount
    memcpy(p, &objCount, sizeof(int32_t));
    p += sizeof(int32_t);

    for (const auto &b : batches)
    {
      int32_t triCount = static_cast<int32_t>(b.verts.size() / 9);
      memcpy(p, &b.objectId, sizeof(int32_t));
      p += sizeof(int32_t);
      memcpy(p, &triCount, sizeof(int32_t));
      p += sizeof(int32_t);
      memcpy(p, b.verts.data(), b.verts.size() * sizeof(float));
      p += static_cast<ptrdiff_t>(b.verts.size() * sizeof(float));
    }

    // bbox trailer
    const float bbox[6] = {bminX, bminY, bminZ, bmaxX, bmaxY, bmaxZ};
    memcpy(p, bbox, 6 * sizeof(float));

    qInfo("[ProjectService] meshData: %d batches, bbox=[%.1f..%.1f, %.1f..%.1f, %.1f..%.1f]",
          objCount, bminX, bmaxX, bminY, bmaxY, bminZ, bmaxZ);
    return buf;
  }
  catch (const std::exception &ex)
  {
    qWarning("[ProjectService] meshData exception: %s", ex.what());
    return {};
  }
  catch (...)
  {
    qWarning("[ProjectService] meshData exception: unknown");
    return {};
  }
#else
  return {};
#endif
}
