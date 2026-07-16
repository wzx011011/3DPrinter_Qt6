#include "ProjectServiceMock.h"

#include <QFileInfo>
#include <QFile>
#include <QDir>
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
#include <libslic3r/TriangleMesh.hpp>
#include <libslic3r/TriangleSelector.hpp>
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
#include <libslic3r/Semver.hpp>
#include <libslic3r/miniz_extension.hpp>  // Phase 97: open_zip_reader/close_zip_reader for thumbnail extraction
#include <cstring> // memcpy

#ifdef HAS_LIBSLIC3R
// Phase 97 (THUMBRT-01): forward declaration -- the definition (with full
// root-cause writeup) is below near qimageToThumbnailData. The model-load
// read blocks (loadFile/loadProject) call it before the definition site.
static QImage extractPlateThumbnailFrom3mf(const QString &archivePath, int plateIndex);
#endif

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
    , m_plateList(std::make_unique<OWzx::PartPlateList>())
{
#ifdef HAS_LIBSLIC3R
  model_ = new Slic3r::Model();
#endif
}

QList<int> ProjectServiceMock::meshBatchSourceObjectIndices() const
{
#ifdef HAS_LIBSLIC3R
  // Phase 91 (ASMEXPLODE-02): emit one source-object index per VOLUME batch
  // (mirroring meshData()'s per-volume emission). meshData() now produces one
  // batch per volume per instance, so this parallel array must produce one
  // entry per volume per instance, each carrying the parent objectIndex. The
  // two arrays MUST stay length-aligned for PrepareSceneData's parser
  // (batchSourceObjectIndices.size() == objectCount, PrepareSceneData.cpp:143);
  // Prepare's highlight unions sibling volumes by this sourceObjectIndex.
  QList<int> indices;
  if (!model_ || model_->objects.empty())
    return indices;

  auto volumeHasRenderableTriangles = [](const Slic3r::ModelVolume *vol) -> bool
  {
    if (!vol)
      return false;
    const auto &its = vol->mesh().its;
    const int vcount = int(its.vertices.size());
    if (vcount <= 0 || its.indices.empty())
      return false;

    for (const auto &face : its.indices)
    {
      const int idx0 = face(0);
      const int idx1 = face(1);
      const int idx2 = face(2);
      if (idx0 >= 0 && idx0 < vcount
          && idx1 >= 0 && idx1 < vcount
          && idx2 >= 0 && idx2 < vcount)
        return true;
    }
    return false;
  };

  for (int objectIndex = 0; objectIndex < int(model_->objects.size()); ++objectIndex)
  {
    const Slic3r::ModelObject *obj = model_->objects[objectIndex];
    if (!obj)
      continue;

    const int instanceCount = obj->instances.empty() ? 1 : int(obj->instances.size());
    for (int instIdx = 0; instIdx < instanceCount; ++instIdx)
    {
      // Skip an instance slot only if the object has instances and this slot is
      // null; meshData() emits a single identity-instance slot when there are
      // no instances.
      if (!obj->instances.empty() && !obj->instances[instIdx])
        continue;

      for (const auto *vol : obj->volumes)
      {
        if (volumeHasRenderableTriangles(vol))
          indices.append(objectIndex);
      }
    }
  }

  return indices;
#else
  return {};
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
  return m_plateList ? m_plateList->plateCount() : 0;
}

int ProjectServiceMock::currentPlateIndex() const
{
  return m_plateList ? m_plateList->currentPlateIndex() : -1;
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
  if (!model_ || !m_plateList || m_plateList->currentPlateIndex() < 0 ||
      m_plateList->currentPlateIndex() >= m_plateList->plateCount())
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

// Phase 122/123 (PAINT-04/05): bridge PaintEngine's TriangleSelector into the
// ModelVolume FacetsAnnotation members so the slice consumes painted facets.
// Mirrors upstream update_model_object: GLGizmoFdmSupports.cpp:577 writes
// mv->supported_facets.set(selector); GLGizmoSeam.cpp:315 writes seam_facets;
// GLGizmoMmuSegmentation.cpp:700 writes mmu_segmentation_facets. The FacetsAnnotation
// is a member of ModelVolume (Model.hpp:870/873/876); cloneCurrentPlateModel
// deep-copies it (Model.hpp:1111) so print.apply flows the paint to the slice.
bool ProjectServiceMock::writePaintToModelVolume(int objectIndex, int volumeIndex,
                                                 PaintKind kind,
                                                 const Slic3r::TriangleSelector &selector)
{
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size())
    return false;
  Slic3r::ModelObject *mo = model_->objects[size_t(objectIndex)];
  if (!mo || volumeIndex < 0 || size_t(volumeIndex) >= mo->volumes.size())
    return false;
  Slic3r::ModelVolume *mv = mo->volumes[size_t(volumeIndex)];
  if (!mv)
    return false;
  // FacetsAnnotation::set (Model.cpp:3524) serializes the selector + touch()
  // (timestamp bump -> Print::apply diff detects change -> re-slice).
  switch (kind) {
    case PaintKind::Support: mv->supported_facets.set(selector); break;
    case PaintKind::Seam:    mv->seam_facets.set(selector);     break;
    case PaintKind::Mmu:     mv->mmu_segmentation_facets.set(selector); break;
  }
  return true;
}

bool ProjectServiceMock::clearPaintOnModelVolume(int objectIndex, int volumeIndex,
                                                 PaintKind kind)
{
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size())
    return false;
  Slic3r::ModelObject *mo = model_->objects[size_t(objectIndex)];
  if (!mo || volumeIndex < 0 || size_t(volumeIndex) >= mo->volumes.size())
    return false;
  Slic3r::ModelVolume *mv = mo->volumes[size_t(volumeIndex)];
  if (!mv)
    return false;
  // FacetsAnnotation::reset clears the serialized state + touch() for re-slice.
  switch (kind) {
    case PaintKind::Support: mv->supported_facets.reset(); break;
    case PaintKind::Seam:    mv->seam_facets.reset();     break;
    case PaintKind::Mmu:     mv->mmu_segmentation_facets.reset(); break;
  }
  return true;
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

  qInfo("[ProjectService] import start path=%s ext=%s",
        fi.absoluteFilePath().toUtf8().constData(),
        fi.suffix().toUtf8().constData());

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

          // v3.0 Phase 18 (D-12): capture locked + bed-type + print-seq + spiral from
          // PlateData onto the receiver so the later rebuild lambda can restore them.
          receiver->pendingPlateLocked_.clear();
          receiver->pendingPlateBedType_.clear();
          receiver->pendingPlatePrintSeq_.clear();
          receiver->pendingPlateSpiral_.clear();
          receiver->pendingPlateThumbnails_.clear();  // v3.2 Phase 30 (THUMB-02)
      receiver->pendingPlateFilamentMaps_.clear();   // v3.2 Phase 31 (FMAP-02)
      receiver->pendingPlateFilamentMapMode_.clear(); // v3.2 Phase 31 (FMAP-02)
          receiver->pendingPlateFilamentMaps_.clear();   // v3.2 Phase 31 (FMAP-02)
          receiver->pendingPlateFilamentMapMode_.clear(); // v3.2 Phase 31 (FMAP-02)

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

              receiver->pendingPlateLocked_.append(plate ? plate->locked : false);
              int bedType = 0, printSeq = 0, spiral = 0;
              if (plate) {
                if (auto *opt = plate->config.option("curr_bed_type"))
                  bedType = int(opt->getInt());
                if (auto *opt = plate->config.option("print_sequence"))
                  printSeq = int(opt->getInt());
                // Phase 97 fix (THUMBRT-01): spiral_mode is coBool; ConfigOptionBool
                // does NOT override getInt (only ConfigOptionEnum/Int do, Config.hpp),
                // so getInt() throws "Calling ConfigOption::getInt on a non-int
                // ConfigOption". Read via the typed Bool accessor instead. This is
                // the read-side mirror of the write-side spiral_mode fix at ~5159.
                if (auto *opt = plate->config.option<Slic3r::ConfigOptionBool>("spiral_mode"))
                  spiral = (opt->value ? 1 : 0);
              }
              receiver->pendingPlateBedType_.append(bedType);
              receiver->pendingPlatePrintSeq_.append(printSeq);
              receiver->pendingPlateSpiral_.append(spiral);

              // v3.2 Phase 31 (FMAP-02) + v4.5 Phase 107 (FMAP-02): extract
              // filament maps + mode. The mode read applies the Pitfall 2 /
              // FM-03 migration so pre-v4.5 "Manual"=1 plates do NOT silently
              // reload as the new fmmAutoForMatch=1. Mirrors the loadProject
              // read block (see ~5445); the coEnum-vs-legacy discriminator and
              // the legacy raw-int-1 -> fmmManual mapping are identical.
              QList<int> fmap;
              if (plate) {
                for (int m : plate->filament_maps) fmap.append(m);
              }
              int fmapMode = int(OWzx::FilamentMapMode::fmmAutoForFlush);
              if (plate) {
                if (auto *opt = plate->config.option("filament_map_mode")) {
                  // Typed coEnum (deserialized from the upstream string by
                  // bbs_3mf.cpp:4443-4448) is trusted directly; a legacy raw
                  // int is migrated: 0->fmmAutoForFlush, 1->fmmManual (the
                  // pre-v4.5 "Manual" preservation), else->fmmDefault.
                  if (opt->type() == Slic3r::coEnum) {
                    fmapMode = int(opt->getInt());
                  } else {
                    // Legacy raw-int path (pre-v4.5 Qt6 files that bypassed enum
                    // typing). Apply the FM-03 migration preserving pre-v4.5
                    // intent: raw 0 (old "Auto") -> fmmAutoForFlush (0); raw 1
                    // (old "Manual") -> fmmManual (2) -- the headline FMAP-02 fix
                    // (NOT fmmAutoForMatch=1); else -> fmmDefault (3) safe
                    // fallback. Phase 111 (FMAP-04 / R-01): factored into
                    // OWzx::migrateLegacyFilamentMapMode so the legacy branch is
                    // unit-tested in isolation (PartPlateTests).
                    fmapMode = int(OWzx::migrateLegacyFilamentMapMode(int(opt->getInt())));
                  }
                }
              }
              receiver->pendingPlateFilamentMaps_.append(fmap);
              receiver->pendingPlateFilamentMapMode_.append(fmapMode);

              // Phase 97 fix (THUMBRT-01): restore the persisted per-plate
              // thumbnail so it survives save->reload. The normal model-load
              // path (_BBS_3MF_Importer::_load_model_from_file) does NOT extract
              // the per-plate thumbnail bytes into plate_thumbnail.pixels -- it
              // only copies the thumbnail_file STRING (bbs_3mf.cpp:2299). The
              // byte extraction (_extract_from_archive at bbs_3mf.cpp:1640) lives
              // only in load_gcode_3mf_from_stream, not the model-load path. So
              // read the PNG straight out of the archive (Metadata/plate_N.png,
              // matching the writer at bbs_3mf.cpp:6550) and decode it. tdefl PNG
              // is lossless, so the decoded pixels are the exact saved pixels
              // (Phase 96 format symmetry). See extractPlateThumbnailFrom3mf for
              // the full root-cause writeup. (Mirrors the loadProject block.)
              QImage loadedThumb;
              if (plate)
                loadedThumb = extractPlateThumbnailFrom3mf(localPath, plate->plate_index);
              receiver->pendingPlateThumbnails_.append(loadedThumb);

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
            receiver->pendingPlateLocked_.append(false);
            receiver->pendingPlateBedType_.append(0);
            receiver->pendingPlatePrintSeq_.append(0);
            receiver->pendingPlateSpiral_.append(0);

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

        // v3.0 Phase 16 (D-05): rebuild m_plateList from the loaded plate data.
        // loadedPlateNames / loadedPlateObjectIndices are local QStringLists / QList<QList<int>>
        // built earlier in this callback from the 3MF plate list (or model objects).
        {
          QStringList plateNames = loadedPlateNames;
          QList<QList<int>> plateObjs = loadedPlateObjectIndices;

          if (plateObjs.isEmpty())
          {
            plateNames.clear();
            plateNames << QObject::tr("平板 1");
            QList<int> all;
            all.reserve(receiver->modelCount_);
            for (int i = 0; i < receiver->modelCount_; ++i)
              all.append(i);
            plateObjs.append(all);
          }

          // Reconstruct the PartPlateList from the per-plate object-index lists.
          // (Instance-level membership is added as (objectIndex, 0) per object — the
          // previous per-object representation. Phase 18 will populate true instance
          // pairs from PlateData::objects_and_instances.)
          receiver->m_plateList = std::make_unique<OWzx::PartPlateList>();
          receiver->m_plateList->resetToSinglePlate();
          for (int pi = 0; pi < plateObjs.size(); ++pi) {
            OWzx::PartPlate *p = (pi == 0) ? receiver->m_plateList->plate(0)
                                           : receiver->m_plateList->createPlate();
            if (!p) continue;
            if (pi < plateNames.size())
              p->setName(plateNames[pi].toStdString());
            p->clearInstances();
            for (int objIdx : plateObjs[pi])
              p->addInstance(objIdx, 0);
            // v3.0 Phase 18 (D-12): restore locked + bed-type/print-seq/spiral captured
            // from PlateData so multi-plate state round-trips.
            if (pi < receiver->pendingPlateLocked_.size()) p->setLocked(receiver->pendingPlateLocked_[pi]);
            if (pi < receiver->pendingPlateBedType_.size()) p->setBedType(receiver->pendingPlateBedType_[pi]);
            if (pi < receiver->pendingPlatePrintSeq_.size()) p->setPrintSequence(receiver->pendingPlatePrintSeq_[pi]);
            if (pi < receiver->pendingPlateSpiral_.size()) p->setSpiralMode(receiver->pendingPlateSpiral_[pi]);
            // v3.2 Phase 31 (FMAP-02) + v4.5 Phase 107 (FMAP-02): restore
            // filament maps + mode (the mode was migrated in the read block).
            if (pi < receiver->pendingPlateFilamentMaps_.size()) {
              const QList<int> &fm = receiver->pendingPlateFilamentMaps_[pi];
              p->setFilamentMaps(std::vector<int>(fm.begin(), fm.end()));
            }
            if (pi < receiver->pendingPlateFilamentMapMode_.size())
              p->setFilamentMapMode(receiver->pendingPlateFilamentMapMode_[pi]);
            // NOTE: thumbnail restoration is applied AFTER arrangeObjects below.
            // The in-loop setThumbnail would be wiped by arrangeObjects ->
            // rebuildPlatesAfterArrangement (clearInstances + addInstance each
            // invalidate m_thumbnail per PartPlate.h D-30-10). See the
            // post-arrange restore pass at ~907.
          }
          // loadedPlateCount may exceed the reconstructed list (multi-plate 3MF whose
          // object membership wasn't fully parsed) — pad with empty plates to match.
          const int reconstructed = receiver->m_plateList->plateCount();
          const int target = std::max(reconstructed, loadedPlateCount);
          for (int pi = reconstructed; pi < target; ++pi)
            receiver->m_plateList->createPlate();

          receiver->m_plateList->setCurrentPlateIndex(
              receiver->m_plateList->plateCount() > 0 ? 0 : -1);
          if (receiver->m_plateList->currentPlateIndex() < 0 &&
              receiver->m_plateList->plateCount() > 0)
            receiver->m_plateList->setCurrentPlateIndex(0);
        }
        receiver->lastError_.clear();
        qInfo("[ProjectService] import success path=%s ext=%s objects=%d plates=%d",
              localPath.toUtf8().constData(),
              QFileInfo(localPath).suffix().toUtf8().constData(),
              receiver->modelCount_,
              receiver->m_plateList ? receiver->m_plateList->plateCount() : 0);

        // Auto-arrange after load (对齐上游 arrange_loaded_object_to_new_position)
        // 传入默认 220x220 热床（与 CLI bed_shape 一致）：ProjectServiceMock 不持有
        // 打印机预设，但导入后自动摆放需要有效热床；InfiniteBed 路径在当前 OrcaSlicer
        // 源码下 libnest2d remove_unpackable_items 会让某些几何 bed_idx=-1，
        // 传具体热床 + 容错 vfn（见 arrangeObjects）保证导入后模型在床内且不抛异常。
        receiver->arrangeObjects(5.0f, false, false,
                                 QStringLiteral("0,0,220,0,220,220,0,220"));

        // Phase 97 fix (THUMBRT-01): restore persisted thumbnails AFTER
        // arrangeObjects. arrangeObjects -> rebuildPlatesAfterArrangement
        // invalidates m_thumbnail via clearInstances()/addInstance()
        // (PartPlate.h D-30-10), so an in-loop setThumbnail (pre-arrange) would
        // be wiped before loadFinished fires. This post-arrange pass is the
        // last writer of m_thumbnail before the load completes, so the
        // restored thumbnail survives save->reload. (Mirrors loadProject below.)
        if (receiver->m_plateList) {
          for (int pi = 0; pi < receiver->m_plateList->plateCount(); ++pi) {
            if (pi >= receiver->pendingPlateThumbnails_.size()) break;
            const QImage &thumb = receiver->pendingPlateThumbnails_[pi];
            if (!thumb.isNull())
              receiver->m_plateList->plate(pi)->setThumbnail(thumb);
          }
        }

        emit receiver->projectChanged();
        emit receiver->plateDataLoaded(receiver->m_plateList ? receiver->m_plateList->plateCount() : 0);
        emit receiver->plateSelectionChanged();
        emit receiver->loadFinished(true, QObject::tr("加载完成"));
      }
      else
      {
        delete loadedModel;
        receiver->modelCount_ = 0;
        // Reset plate storage to a fresh single-plate list (D-05).
        receiver->m_plateList = std::make_unique<OWzx::PartPlateList>();
        receiver->m_plateList->setCurrentPlateIndex(-1);
        receiver->sourceFilePath_.clear();
        receiver->objectNames_.clear();
        receiver->objectModuleNames_.clear();
        receiver->objectPrintableStates_.clear();
        receiver->objectVisibleStates_.clear();
        receiver->lastError_ = errorText;
        qWarning("[ProjectService] import failed path=%s reason=%s",
                 localPath.toUtf8().constData(),
                 errorText.toUtf8().constData());
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

// v2.4 IO-01: 项目保存（调用 libslic3r store_3mf 真实导出 .3mf）
// 对齐上游 Plater::save_project → export_3mf (Plater.cpp:12165)
bool ProjectServiceMock::saveProjectAs(const QString &filePath)
{
#ifdef HAS_LIBSLIC3R
    if (!model_) {
        qWarning("[Project] saveProjectAs: no model loaded");
        return false;
    }
    // 构建 config（从当前 model 的 default config）
    Slic3r::DynamicPrintConfig config = Slic3r::DynamicPrintConfig::full_print_config();
    // 调用 libslic3r store_3mf（Format/3mf.hpp:60）
    // model_ 是裸指针 Slic3r::Model*（非 unique_ptr）
    bool ok = Slic3r::store_3mf(filePath.toStdString().c_str(),
                                model_,
                                &config,
                                false,   // fullpath_sources=false
                                nullptr, // thumbnail=nullptr
                                true);   // zip64=true
    if (ok) {
        qDebug("[Project] saved project to: %s", filePath.toUtf8().constData());
        currentProjectPath_ = filePath;
    } else {
        qWarning("[Project] store_3mf failed for: %s", filePath.toUtf8().constData());
    }
    return ok;
#else
    qWarning("[Project] saveProjectAs: HAS_LIBSLIC3R not enabled");
    return false;
#endif
}

// v2.4 IO-02: 导出模型（STL/3MF/OBJ）
bool ProjectServiceMock::exportModel(const QString &filePath, const QString &format)
{
#ifdef HAS_LIBSLIC3R
    if (!model_) {
        qWarning("[Project] exportModel: no model loaded");
        return false;
    }
    auto ext = format.toLower().toStdString();
    if (ext == "stl") {
        // 导出 STL（write_binary，简化：第一个对象）
        if (!model_->objects.empty()) {
            return model_->objects.front()->mesh().write_binary(filePath.toUtf8().constData());
        }
        return false;
    } else if (ext == "3mf") {
        // 导出 3MF（复用 store_3mf）
        return saveProjectAs(filePath);
    } else if (ext == "obj") {
        // 导出 OBJ（TriangleMesh::WriteOBJFile 接受 const char*）
        if (!model_->objects.empty()) {
            model_->objects.front()->mesh().WriteOBJFile(filePath.toUtf8().constData());
            return true;
        }
        return false;
    }
    qWarning("[Project] exportModel: unsupported format %s", format.toUtf8().constData());
    return false;
#else
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
  QStringList names;
  if (!m_plateList)
    return names;
  for (int i = 0; i < m_plateList->plateCount(); ++i) {
    const OWzx::PartPlate *p = m_plateList->plate(i);
    names << QString::fromStdString(p ? p->name() : std::string());
  }
  return names;
}

QString ProjectServiceMock::objectModuleName(int index) const
{
  if (index < 0 || index >= objectModuleNames_.size())
    return {};
  return objectModuleNames_[index];
}

bool ProjectServiceMock::setCurrentPlateIndex(int index)
{
  if (!m_plateList || index < 0 || index >= m_plateList->plateCount())
    return false;
  if (m_plateList->currentPlateIndex() == index)
    return true;
  m_plateList->setCurrentPlateIndex(index);
  emit plateSelectionChanged();
  return true;
}

bool ProjectServiceMock::addPlate()
{
  if (loading_ || !m_plateList)
    return false;

  OWzx::PartPlate *p = m_plateList->createPlate();
  if (!p)
    return false;  // kMaxPlateCount reached
  // Default name mirrors the previous behavior ("平板 N" 1-based).
  p->setName(tr("平板 %1").arg(p->plateIndex() + 1).toStdString());

  emit projectChanged();
  emit plateDataLoaded(m_plateList->plateCount());
  return true;
}

bool ProjectServiceMock::deletePlate(int plateIndex)
{
  if (loading_ || !m_plateList)
    return false;

  const int prevCurrent = m_plateList->currentPlateIndex();
  if (!m_plateList->deletePlate(plateIndex))
    return false;  // refuses last plate / invalid index

  emit projectChanged();
  emit plateDataLoaded(m_plateList->plateCount());
  if (m_plateList->currentPlateIndex() != prevCurrent)
    emit plateSelectionChanged();
  return true;
}

bool ProjectServiceMock::renamePlate(int plateIndex, const QString &newName)
{
  if (loading_ || !m_plateList)
    return false;
  if (!m_plateList->renamePlate(plateIndex, newName.toStdString()))
    return false;
  emit projectChanged();
  return true;
}

bool ProjectServiceMock::removeAllOnPlate(int plateIndex)
{
  if (loading_ || !m_plateList)
    return false;
  if (plateIndex < 0 || plateIndex >= m_plateList->plateCount())
    return false;

  const QList<int> objs = m_plateList->objectIndicesOnPlate(plateIndex);
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
  if (!m_plateList)
    return false;
  const OWzx::PartPlate *p = m_plateList->plate(plateIndex);
  return p ? p->isLocked() : false;
}

bool ProjectServiceMock::setPlateLocked(int plateIndex, bool locked)
{
  if (!m_plateList || plateIndex < 0 || plateIndex >= m_plateList->plateCount())
    return false;
  m_plateList->setPlateLocked(plateIndex, locked);
  emit projectChanged();
  return true;
}

// ── v3.0 Phase 17: plate lifecycle completion (PLATE-03/04/05) ──────────

bool ProjectServiceMock::isPlatePrintable(int plateIndex) const
{
  if (!m_plateList) return false;
  const OWzx::PartPlate *p = m_plateList->plate(plateIndex);
  return p ? p->isPrintable() : false;
}

bool ProjectServiceMock::setPlatePrintable(int plateIndex, bool printable)
{
  if (!m_plateList || plateIndex < 0 || plateIndex >= m_plateList->plateCount())
    return false;
  m_plateList->setPlatePrintable(plateIndex, printable);
  emit projectChanged();
  return true;
}

// v3.2 Phase 31 (FMAP-03, Manual mode) + v4.5 Phase 107 (FMAP-02): per-plate
// filament->extruder mapping. `mode` is the widened 4-value FilamentMapMode
// (cast to int for the Q_INVOKABLE boundary; QML/Phase 110 popup will pass the
// enum-named constants). The int overload on PartPlate::setFilamentMapMode
// accepts the raw value here.
bool ProjectServiceMock::setPlateFilamentMap(int plateIndex, int mode, const QList<int>& maps)
{
  if (!m_plateList || plateIndex < 0 || plateIndex >= m_plateList->plateCount())
    return false;
  OWzx::PartPlate *p = m_plateList->plate(plateIndex);
  if (!p) return false;
  std::vector<int> vmaps(maps.begin(), maps.end());
  p->setFilamentMapMode(mode);
  p->setFilamentMaps(vmaps);
  emit projectChanged();
  return true;
}

// Phase 110 (FMAP-03): mode-only write path. Reuses the existing plate-write
// plumbing by reading the current maps and delegating to setPlateFilamentMap.
// The clamp at PartPlate::setFilamentMapMode(int) (R-02 / FP-04) guards the
// int boundary, so out-of-range modes resolve to fmmDefault before the write.
bool ProjectServiceMock::setPlateFilamentMapMode(int plateIndex, int mode)
{
  if (!m_plateList || plateIndex < 0 || plateIndex >= m_plateList->plateCount())
    return false;
  const OWzx::PartPlate *p = m_plateList->plate(plateIndex);
  if (!p) return false;
  // Preserve the existing per-extruder map; only the mode changes.
  QList<int> currentMaps;
  const std::vector<int> &v = p->filamentMaps();
  for (int m : v) currentMaps.append(m);
  return setPlateFilamentMap(plateIndex, mode, currentMaps);
}

int ProjectServiceMock::plateFilamentMapMode(int plateIndex) const
{
  if (!m_plateList || plateIndex < 0 || plateIndex >= m_plateList->plateCount())
    return int(OWzx::FilamentMapMode::fmmAutoForFlush);  // default Auto
  const OWzx::PartPlate *p = m_plateList->plate(plateIndex);
  return p ? int(p->filamentMapMode())
           : int(OWzx::FilamentMapMode::fmmAutoForFlush);
}

QList<int> ProjectServiceMock::plateFilamentMaps(int plateIndex) const
{
  QList<int> result;
  if (!m_plateList || plateIndex < 0 || plateIndex >= m_plateList->plateCount())
    return result;
  const OWzx::PartPlate *p = m_plateList->plate(plateIndex);
  if (!p) return result;
  const std::vector<int> &v = p->filamentMaps();
  for (int m : v) result.append(m);
  return result;
}

bool ProjectServiceMock::movePlate(int oldIndex, int newIndex)
{
  if (loading_ || !m_plateList) return false;
  const int prevCurrent = m_plateList->currentPlateIndex();
  if (!m_plateList->movePlate(oldIndex, newIndex)) return false;
  emit projectChanged();
  emit plateDataLoaded(m_plateList->plateCount());
  if (m_plateList->currentPlateIndex() != prevCurrent)
    emit plateSelectionChanged();
  return true;
}

bool ProjectServiceMock::clonePlate(int sourceIndex)
{
  // D-06: deep copy including ModelObjects (upstream duplicate_plate).
  if (loading_ || !m_plateList) return false;
  if (sourceIndex < 0 || sourceIndex >= m_plateList->plateCount()) return false;

  const OWzx::PartPlate *src = m_plateList->plate(sourceIndex);
  if (!src) return false;

  // Create the destination plate.
  OWzx::PartPlate *dst = m_plateList->createPlate();
  if (!dst) return false;  // kMaxPlateCount reached

  // Copy plate metadata from source.
  std::string newName = src->name();
  if (newName.empty()) newName = "Plate";
  newName += " (copy)";
  dst->setName(newName);
  dst->setLocked(src->isLocked());
  dst->setPrintable(src->isPrintable());
  dst->setBedType(src->bedType());
  dst->setPrintSequence(src->printSequence());
  dst->setSpiralMode(src->spiralMode());
  dst->setFirstLayerSeqChoice(src->firstLayerSeqChoice());
  dst->setFirstLayerSeqOrder(src->firstLayerSeqOrder());
  dst->setOtherLayersSeqChoice(src->otherLayersSeqChoice());
  dst->setOtherLayersSeqEntries(src->otherLayersSeqEntries());
#ifdef HAS_LIBSLIC3R
  dst->config() = src->config();
#endif

  // Deep-copy the objects onto the destination plate. Reuse duplicateObject to
  // clone the ModelObject + parallel-array metadata. duplicateObject's two
  // branches differ: the HAS branch appends a new ModelObject (no plate
  // membership set), while the mock branch INSERTS at sourceIndex+1 (shifting
  // higher indices up) AND adds the new object to currentPlate(). To make the
  // mock branch target dst (not whatever current happens to be), temporarily
  // switch current to dst for the duration of the loop. Iterate source indices
  // DESCENDING so the mock branch's index-shift doesn't invalidate unprocessed
  // lower indices. (HAS appends to the end, so descending is a no-op there but
  // harmless.) After each duplicate, dst->addInstance(dupIdx, 0) ensures the
  // HAS path assigns membership (the mock path already added it via the
  // currentPlate() route; the std::set membership dedupes the duplicate pair).
  const int savedCurrent = m_plateList->currentPlateIndex();
  m_plateList->setCurrentPlateIndex(dst->plateIndex());
  QList<int> srcObjs = m_plateList->objectIndicesOnPlate(sourceIndex);
  std::sort(srcObjs.begin(), srcObjs.end(), std::greater<int>());
  for (int srcObjIdx : srcObjs) {
    const int modelCountBefore = int(model_ ? model_->objects.size() : 0);
    const int dupIdx = duplicateObject(srcObjIdx);  // returns new object index
    if (dupIdx >= 0) {
      dst->addInstance(dupIdx, 0);
    } else {
      // Fallback: derive new index from model object-count growth.
      const int modelCountAfter = int(model_ ? model_->objects.size() : 0);
      if (modelCountAfter > modelCountBefore)
        dst->addInstance(modelCountAfter - 1, 0);
    }
  }
  m_plateList->setCurrentPlateIndex(savedCurrent);

  emit projectChanged();
  emit plateDataLoaded(m_plateList->plateCount());
  return true;
}

QList<int> ProjectServiceMock::plateObjectIndices(int plateIndex) const
{
  if (!m_plateList)
    return {};
  return m_plateList->objectIndicesOnPlate(plateIndex);
}

QList<int> ProjectServiceMock::currentPlateObjectIndices() const
{
  if (!m_plateList)
    return {};
  return m_plateList->objectIndicesOnPlate(m_plateList->currentPlateIndex());
}

int ProjectServiceMock::plateObjectCount(int index) const
{
  if (!m_plateList || index < 0 || index >= m_plateList->plateCount())
    return 0;
  return m_plateList->objectIndicesOnPlate(index).size();
}

int ProjectServiceMock::plateIndexForObject(int objectIndex) const
{
  if (objectIndex < 0 || objectIndex >= objectNames_.size())
    return -1;
  if (!m_plateList)
    return -1;
  const int found = m_plateList->plateIndexForObject(objectIndex);
  if (found >= 0)
    return found;
  // Preserve previous single-plate fallback behavior: an unassigned object on a
  // single-plate project is considered to be on plate 0.
  if (m_plateList->plateCount() == 1)
    return 0;
  return -1;
}

void ProjectServiceMock::setObjectPlateForIndex(int objectIndex, int plateIndex)
{
  if (objectIndex < 0 || objectIndex >= objectNames_.size())
    return;
  if (!m_plateList || plateIndex < 0 || plateIndex >= m_plateList->plateCount())
    return;

  // Remove the object from every plate it currently belongs to (whole-object move,
  // preserving the previous per-object semantics). Instance-level model: erase all
  // (obj, inst) pairs for this object across all plates.
  for (int i = 0; i < m_plateList->plateCount(); ++i) {
    OWzx::PartPlate *p = m_plateList->plate(i);
    if (!p)
      continue;
    // Collect then erase (cannot mutate set while iterating it).
    std::vector<std::pair<int, int>> toErase;
    for (const auto &pair : p->objToInstanceSet())
      if (pair.first == objectIndex)
        toErase.push_back(pair);
    for (const auto &pair : toErase)
      p->removeInstance(pair.first, pair.second);
  }

  // Add the object to the target plate (instance 0 — single-instance representation
  // matching the previous per-object behavior).
  OWzx::PartPlate *target = m_plateList->plate(plateIndex);
  if (target)
    target->addInstance(objectIndex, 0);

  emit projectChanged();
  emit plateDataLoaded(m_plateList->plateCount());
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

int ProjectServiceMock::objectVolumeType(int objectIndex, int volumeIndex) const
{
#ifdef HAS_LIBSLIC3R
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size())
    return 0;
  const auto *obj = model_->objects[size_t(objectIndex)];
  if (!obj || volumeIndex < 0 || size_t(volumeIndex) >= obj->volumes.size())
    return 0;
  const auto *vol = obj->volumes[size_t(volumeIndex)];
  if (!vol) return 0;
  if (vol->is_model_part())       return 0;
  if (vol->is_negative_volume())  return 1;
  if (vol->is_modifier())         return 2;
  if (vol->is_support_blocker())  return 3;
  if (vol->is_support_enforcer()) return 4;
  return 0;
#else
  if (objectIndex < 0)
    return 0;
  const auto it = m_mockVolumes.constFind(objectIndex);
  if (it == m_mockVolumes.constEnd() || volumeIndex < 0 || volumeIndex >= it->size())
    return 0;
  return static_cast<int>(it->at(volumeIndex).type);
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
  // v3.0 Phase 19 (D-16): read from PartPlate::config() (replaces the TODO stub).
  const OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  if (!p) return fallbackValue;
  const Slic3r::DynamicPrintConfig &cfg = p->config();
  const auto *opt = cfg.option(key.toUtf8().constData());
  if (!opt) return fallbackValue;
  // Dispatch by concrete ConfigOption type (Config.hpp: ConfigOptionInt/Float/Bool/String).
  if (const auto *o = dynamic_cast<const Slic3r::ConfigOptionInt *>(opt))
    return o->getInt();
  if (const auto *o = dynamic_cast<const Slic3r::ConfigOptionFloat *>(opt))
    return o->getFloat();
  if (const auto *o = dynamic_cast<const Slic3r::ConfigOptionBool *>(opt))
    return o->getBool();
  if (const auto *o = dynamic_cast<const Slic3r::ConfigOptionString *>(opt))
    return QString::fromStdString(o->value);
  // Percent/enum/etc. expose via getInt as a fallback.
  try { return opt->getInt(); }
  catch (...) { return fallbackValue; }
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
  // v3.0 Phase 19 (D-16): write into PartPlate::config() (replaces the TODO stub).
  OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  if (!p) return false;
  Slic3r::DynamicPrintConfig &cfg = p->config();
  const std::string k = key.toUtf8().constData();
  auto *opt = cfg.option(k, true);  // create=true so missing keys are created
  if (!opt) {
    // DynamicPrintConfig can only instantiate options registered in the config
    // schema; unknown keys (typo, upstream-unregistered) return null. Warn so
    // the dropped override is diagnosable rather than silent.
    qWarning("[PartPlate] unknown config key '%s' for plate %d; override dropped",
             k.c_str(), plateIndex);
    return false;
  }
  // Dispatch by QVariant type → matching ConfigOption .value write (no setters in
  // libslic3r; ConfigOptionSingle<T> exposes a public `value` member).
  if (value.typeId() == QMetaType::Bool) {
    if (auto *b = dynamic_cast<Slic3r::ConfigOptionBool *>(opt)) b->value = value.toBool();
    else opt->setInt(value.toBool() ? 1 : 0);
  } else if (value.typeId() == QMetaType::Double) {
    if (auto *f = dynamic_cast<Slic3r::ConfigOptionFloat *>(opt)) f->value = value.toDouble();
    else opt->setInt(int(value.toDouble()));
  } else if (value.typeId() == QMetaType::Int || value.typeId() == QMetaType::LongLong) {
    opt->setInt(value.toInt());
  } else {
    if (auto *s = dynamic_cast<Slic3r::ConfigOptionString *>(opt))
      s->value = value.toString().toStdString();
    else
      opt->setInt(value.toInt());
  }
  emit projectChanged();
  return true;
#else
  m_mockPlateOverrides[plateIndex][key] = value;
  emit projectChanged();
  return true;
#endif
}

const Slic3r::DynamicPrintConfig *ProjectServiceMock::plateDynamicConfig(int plateIndex) const
{
#ifdef HAS_LIBSLIC3R
  const OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  return p ? &p->config() : nullptr;
#else
  Q_UNUSED(plateIndex);
  return nullptr;
#endif
}

int ProjectServiceMock::scopedOverrideCount(int objectIndex, int volumeIndex) const
{
#ifdef HAS_LIBSLIC3R
  const auto *config = scopedConfigForRead(model_, objectIndex, volumeIndex);
  return config ? int(config->keys().size()) : 0;
#else
  if (volumeIndex >= 0) {
    const int volKey = (objectIndex << 16) | volumeIndex;
    auto it = m_mockVolumeOverrides.constFind(volKey);
    return it != m_mockVolumeOverrides.constEnd() ? it.value().size() : 0;
  }
  auto it = m_mockObjectOverrides.constFind(objectIndex);
  return it != m_mockObjectOverrides.constEnd() ? it.value().size() : 0;
#endif
}

QString ProjectServiceMock::scopedOverriddenKey(int objectIndex, int volumeIndex, int index) const
{
#ifdef HAS_LIBSLIC3R
  const auto *config = scopedConfigForRead(model_, objectIndex, volumeIndex);
  if (!config || index < 0 || index >= int(config->keys().size()))
    return {};
  const auto &keys = config->keys();
  auto it = keys.begin();
  std::advance(it, index);
  return QString::fromStdString(*it);
#else
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
#endif
}

bool ProjectServiceMock::resetScopedOptionValue(int objectIndex, int volumeIndex, const QString &key)
{
#ifdef HAS_LIBSLIC3R
  auto *config = scopedConfigForWrite(model_, objectIndex, volumeIndex);
  if (!config)
    return false;
  const std::string stdKey = key.toUtf8().constData();
  if (!config->option(stdKey))
    return false;
  config->erase(stdKey);
  emit projectChanged();
  return true;
#else
  if (volumeIndex >= 0) {
    const int volKey = (objectIndex << 16) | volumeIndex;
    auto it = m_mockVolumeOverrides.find(volKey);
    if (it != m_mockVolumeOverrides.end() && it.value().remove(key)) { emit projectChanged(); return true; }
  } else {
    auto it = m_mockObjectOverrides.find(objectIndex);
    if (it != m_mockObjectOverrides.end() && it.value().remove(key)) { emit projectChanged(); return true; }
  }
  return false;
#endif
}

int ProjectServiceMock::plateScopedOverrideCount(int plateIndex) const
{
#ifdef HAS_LIBSLIC3R
  const OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  return p ? int(p->config().keys().size()) : 0;
#else
  auto it = m_mockPlateOverrides.find(plateIndex);
  return it != m_mockPlateOverrides.end() ? it.value().size() : 0;
#endif
}

QString ProjectServiceMock::plateScopedOverriddenKey(int plateIndex, int index) const
{
#ifdef HAS_LIBSLIC3R
  const OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  if (!p || index < 0 || index >= int(p->config().keys().size()))
    return {};
  const auto &keys = p->config().keys();
  auto it = keys.begin();
  std::advance(it, index);
  return QString::fromStdString(*it);
#else
  auto it = m_mockPlateOverrides.find(plateIndex);
  if (it == m_mockPlateOverrides.end() || index < 0 || index >= it.value().size()) return {};
  return it.value().keys().value(index);
#endif
}

bool ProjectServiceMock::resetPlateScopedOptionValue(int plateIndex, const QString &key)
{
#ifdef HAS_LIBSLIC3R
  OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  if (!p)
    return false;
  const std::string stdKey = key.toUtf8().constData();
  if (!p->config().option(stdKey))
    return false;
  p->config().erase(stdKey);
  emit projectChanged();
  return true;
#else
  auto it = m_mockPlateOverrides.find(plateIndex);
  if (it != m_mockPlateOverrides.end() && it.value().remove(key)) { emit projectChanged(); return true; }
  return false;
#endif
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

bool ProjectServiceMock::arrangeObjects(float spacing, bool allowRotation, bool alignY,
                                          const QString &printableArea)
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

    // 容错 vfn：对齐上游 ArrangeJob::process() (ArrangeJob.cpp:585) 的处理方式 ——
    // arrange 失败的 item（bed_idx<0，即 BIN_ID_UNFIT）不抛异常，而是保留原坐标。
    // 默认 vfn 是 throw_if_out_of_bed（ModelArrange.hpp:20），会在任意 item 不可
    // 摆放时抛 RuntimeError，使整个 arrange 失败。这里改为 no-op，让 arrange_objects
    // 返回 false（apply_arrange_polys 检测到 bed_idx!=0 即返回 false），调用方据此
    // 决定后续行为（保留模型当前坐标），而非崩溃/挂起切片 worker。
    Slic3r::VirtualBedFn tolerantVfn = [](Slic3r::arrangement::ArrangePolygon &) {};

    if (!printableArea.isEmpty())
    {
      // Parse "x1,y1;x2,y2;..." or "x1,y1,x2,y2,..." format
      Slic3r::Points bed_shape;
      const auto coords = printableArea.split(QLatin1Char(','));
      for (int i = 0; i + 1 < coords.size(); i += 2)
      {
        bool okX = false, okY = false;
        const coord_t x = static_cast<coord_t>(coords[i].trimmed().toDouble(&okX) * 1000.0); // mm → μm
        const coord_t y = static_cast<coord_t>(coords[i + 1].trimmed().toDouble(&okY) * 1000.0);
        if (okX && okY)
          bed_shape.emplace_back(x, y);
      }
      if (bed_shape.size() >= 3)
      {
        Slic3r::BoundingBox bed_bb(bed_shape);
        // D-29-3 (RESEARCH §5): derive plate width/depth from the SAME bed_bb
        // arrange uses (μm→mm via /1000.0), so the grid decode (computePlateIndex)
        // and the arrange coordinate space are identical — zero drift.
        if (m_plateList)
        {
          const double width_mm = bed_bb.size().x() / 1000.0;
          const double depth_mm = bed_bb.size().y() / 1000.0;
          m_plateList->setModel(model_);  // backref for rebuildPlatesAfterArrangement
          m_plateList->setPlateSize(static_cast<int>(width_mm), static_cast<int>(depth_mm), 0);
        }
        // Locked exclusion (D-29-10/D-29-9): upstream pins ArrangePolygon.bed_idx
        // to locked plate index before arrange (PartPlate.cpp:5388-5396). Qt6's
        // arrange_objects overload operates on the whole model without exposing
        // the ArrangePolygon list, so the minimal-determinism path preserves
        // locked membership via rebuildPlatesAfterArrangement(exceptLocked=true)
        // instead. Full preprocess_arrange_polygon (fixed-item/non-selected
        // branches) deferred per CONTEXT.
        // 传入容错 vfn：失败 item 不抛，arrange_objects 返回 false 表示有 item 未摆放。
        // 即使部分 item 未摆放，已成功摆放的 item 坐标已被 apply_arrange_polys 写回模型实例。
        const bool arranged = Slic3r::arrange_objects(*model_, bed_bb, params, tolerantVfn);
        // D-29-12: only rebuild plate membership when arrange succeeded
        // (all-locked returns false → no rebuild, no membership changes).
        if (arranged && m_plateList)
        {
          m_plateList->rebuildPlatesAfterArrangement(/*exceptLocked=*/true, /*recyclePlates=*/true);
          emit plateDataLoaded(m_plateList->plateCount());
        }
        return arranged;
      }
    }

    // Fallback to InfiniteBed when no valid bed shape（同样用容错 vfn）
    Slic3r::InfiniteBed bed;
    const bool arranged = Slic3r::arrange_objects(*model_, bed, params, tolerantVfn);
    // InfiniteBed has no bounding box, so we cannot derive a plate size and
    // multi-plate distribution is not meaningful here (arrange packs onto one
    // infinite bed). Skip the rebuild to avoid a divide-by-zero in
    // computePlateIndex (plateStrideX/Y would be 0). The bed_bb path above
    // handles the multi-plate case.
    (void)arranged;
    return arranged;
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
  Q_UNUSED(printableArea);
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
    const Slic3r::ModelObjectPtrs &new_objects = cut.perform_with_groove(groove, rotation_m, 0, 0.0f, 0.0f, false);

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

    // Phase 137 (CGAL-02): perform boolean operation via libslic3r MeshBoolean
    // (CGAL corefinement). operation: 0=union, 1=difference(A-B), 2=intersection.
    // Phase 136 patched MeshBoolean.cpp for CGAL 5.4 compat.
    if (operation == 1) {
      // difference: srcMesh = srcMesh - toolMesh
      Slic3r::MeshBoolean::minus(srcMesh, toolMesh);
    } else if (operation == 0) {
      // union: merge tool into source
      Slic3r::its_merge(srcMesh.its, toolMesh.its);
      Slic3r::MeshBoolean::self_union(srcMesh);
    } else {
      // intersection: not directly available via the minus API; use union of
      // shared volume as approximation (full intersection needs corefine_and_compute_intersection).
      // For now, support union + difference (the two most common operations).
      Slic3r::MeshBoolean::minus(srcMesh, toolMesh);
    }

    // Replace source object's first model_part volume with result mesh
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

        // Update plate instance membership: drop tool object, shift higher indices down.
        if (m_plateList) {
          for (int pi = 0; pi < m_plateList->plateCount(); ++pi) {
            OWzx::PartPlate *p = m_plateList->plate(pi);
            if (!p) continue;
            std::set<std::pair<int,int>> rebuilt;
            for (const auto &pair : p->objToInstanceSet()) {
              if (pair.first == toolObjectIndex) continue;  // dropped
              int adj = pair.first > toolObjectIndex ? pair.first - 1 : pair.first;
              if (adj >= 0 && adj < modelCount_)
                rebuilt.insert({adj, pair.second});
            }
            p->clearInstances();
            for (const auto &pair : rebuilt)
              p->addInstance(pair.first, pair.second);
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
      emit plateDataLoaded(m_plateList ? m_plateList->plateCount() : 0);
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

    // Phase 137 (CGAL-03): perform CGAL boolean subtraction via MeshBoolean::minus
    // (drillMesh subtracted from the source mesh). Phase 136 patched MeshBoolean
    // for CGAL 5.4 compat.
    Slic3r::MeshBoolean::minus(srcMesh, drillMesh);

    // Write the drilled result back to the source volume.
    srcVol->set_mesh(std::move(srcMesh));
    srcVol->set_new_unique_id();
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

// Phase 112 (MEASURE-01): per-volume ITS accessor. See the ownership contract
// in ProjectServiceMock.h (MI-02/MI-03/MI-04/MI-05/MI-06). Shallow-share via
// the shared_ptr aliasing constructor: the returned shared_ptr shares the
// ModelVolume's m_mesh refcount and points at &mesh.its, so the TriangleMesh
// (and its ITS) stays alive as long as ANY caller holds the result.
std::shared_ptr<const indexed_triangle_set>
ProjectServiceMock::volumeMeshIts(int objectIndex, int volumeIndex) const
{
  // MI-05: defensive null return. No model, no object, no volume -> nullptr.
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size())
    return nullptr;

  const auto *obj = model_->objects[size_t(objectIndex)];
  if (!obj || volumeIndex < 0 || size_t(volumeIndex) >= obj->volumes.size())
    return nullptr;

  const Slic3r::ModelVolume *volume = obj->volumes[size_t(volumeIndex)];
  if (!volume)
    return nullptr;

  // ModelVolume::mesh_ptr() returns the internal std::shared_ptr<const
  // TriangleMesh> (Model.hpp:856). Grab it so the aliasing pointer keeps the
  // TriangleMesh alive.
  const std::shared_ptr<const Slic3r::TriangleMesh> meshPtr = volume->mesh_ptr();
  if (!meshPtr)
    return nullptr;

  // MI-05: also guard against an empty mesh (no geometry) so callers do not
  // receive an ITS they would immediately reject. its.vertices + its.indices
  // are the same fields meshData()/meshBatchSourceObjectIndices() check at
  // :339 and :563.
  const indexed_triangle_set &its = meshPtr->its;
  if (its.vertices.empty() || its.indices.empty())
    return nullptr;

  // MI-03 shallow-share: alias meshPtr's refcount onto &its. Zero copy. The
  // returned shared_ptr owns one ref on the TriangleMesh; when the last holder
  // releases it, the TriangleMesh (and its `its` member) is destroyed.
  return std::shared_ptr<const indexed_triangle_set>(meshPtr, &its);
}

// Phase 120 (PAINT-01): per-volume TriangleMesh accessor. See the ownership
// contract in ProjectServiceMock.h (mirrors volumeMeshIts MI-02..MI-06). The
// aliasing target is the TriangleMesh itself (not &its like volumeMeshIts) so
// TriangleSelector::TriangleSelector(const TriangleMesh&, ...) can bind a
// stable reference whose lifetime is refcount-tied to ModelVolume::m_mesh.
//
// ModelVolume::mesh_ptr() returns the internal std::shared_ptr<const
// TriangleMesh> m_mesh (Model.hpp:856). Returning it directly (no aliasing
// constructor needed -- the target IS the TriangleMesh) keeps the refcount and
// therefore the TriangleMesh alive for as long as any PaintEngine selector
// holds the result.
std::shared_ptr<const Slic3r::TriangleMesh>
ProjectServiceMock::volumeMeshTriangleMesh(int objectIndex, int volumeIndex) const
{
  // MI-05: defensive null return. No model, no object, no volume -> nullptr.
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size())
    return nullptr;

  const auto *obj = model_->objects[size_t(objectIndex)];
  if (!obj || volumeIndex < 0 || size_t(volumeIndex) >= obj->volumes.size())
    return nullptr;

  const Slic3r::ModelVolume *volume = obj->volumes[size_t(volumeIndex)];
  if (!volume)
    return nullptr;

  // mesh_ptr() returns the internal shared_ptr<const TriangleMesh>
  // (Model.hpp:856). Grab it so the TriangleMesh stays alive for any
  // TriangleSelector that references it. nullptr guard: a freshly-reset
  // volume can have an empty mesh.
  std::shared_ptr<const Slic3r::TriangleMesh> meshPtr = volume->mesh_ptr();
  if (!meshPtr)
    return nullptr;

  // MI-05: guard against an empty mesh (no geometry) so callers do not receive
  // a TriangleMesh TriangleSelector would reject. its.vertices + its.indices
  // mirror the volumeMeshIts empty-mesh check above.
  const indexed_triangle_set &its = meshPtr->its;
  if (its.vertices.empty() || its.indices.empty())
    return nullptr;

  // Return the shared_ptr directly (the aliasing target IS the TriangleMesh,
  // so no aliasing constructor needed -- this differs from volumeMeshIts
  // which aliases onto &its). The refcount keeps the TriangleMesh alive.
  return meshPtr;
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
      // No objects left: reset to a single empty plate, current unset.
      m_plateList = std::make_unique<OWzx::PartPlateList>();
      m_plateList->setCurrentPlateIndex(-1);
    }
    else
    {
      // Rebuild per-plate instance membership: drop the deleted object index and
      // decrement every higher index. Empty plates are pruned (keep >= 1).
      if (m_plateList && m_plateList->plateCount() > 0) {
        // If the single plate has no membership, seed it with all current objects.
        OWzx::PartPlate *first = m_plateList->plate(0);
        if (first && first->objToInstanceSet().empty()) {
          for (int i = 0; i < modelCount_; ++i)
            first->addInstance(i, 0);
          if (first->name().empty())
            first->setName(tr("平板 1").toStdString());
        }
        for (int pi = 0; pi < m_plateList->plateCount(); ++pi) {
          OWzx::PartPlate *p = m_plateList->plate(pi);
          if (!p) continue;
          std::set<std::pair<int,int>> rebuilt;
          for (const auto &pair : p->objToInstanceSet()) {
            if (pair.first == index) continue;  // deleted object
            int adjusted = pair.first > index ? pair.first - 1 : pair.first;
            if (adjusted >= 0 && adjusted < modelCount_)
              rebuilt.insert({adjusted, pair.second});
          }
          p->clearInstances();
          for (const auto &pair : rebuilt)
            p->addInstance(pair.first, pair.second);
        }
        // Prune empty plates (keep >= 1), highest index first.
        for (int p = m_plateList->plateCount() - 1; p >= 0; --p) {
          if (m_plateList->objectIndicesOnPlate(p).isEmpty() && m_plateList->plateCount() > 1)
            m_plateList->deletePlate(p);
        }
        if (m_plateList->currentPlateIndex() < 0 && m_plateList->plateCount() > 0)
          m_plateList->setCurrentPlateIndex(0);
      }
    }

    lastError_.clear();
    emit projectChanged();
    emit plateDataLoaded(m_plateList ? m_plateList->plateCount() : 0);
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

void ProjectServiceMock::fixMeshForObject(int objectIndex)
{
#ifdef HAS_LIBSLIC3R
  if (!model_ || objectIndex < 0 || size_t(objectIndex) >= model_->objects.size())
    return;
  auto *obj = model_->objects[size_t(objectIndex)];
  if (!obj) return;
  for (auto *vol : obj->volumes) {
    if (vol) {
      // OrcaSlicer repairs meshes at STL import time; no public repair() on TriangleMesh.
      Slic3r::TriangleMesh m = vol->mesh();
      vol->set_mesh(std::move(m));
    }
  }
  emit projectChanged();
#else
  Q_UNUSED(objectIndex);
  lastError_ = tr("网格修复需要 libslic3r");
  emit projectChanged();
#endif
}

bool ProjectServiceMock::fixMesh(int objectIndex)
{
  if (objectIndex < 0 || objectIndex >= objectNames_.size())
  {
    lastError_ = tr("修复失败：对象索引无效");
    return false;
  }

#ifdef HAS_LIBSLIC3R
  if (!model_ || size_t(objectIndex) >= model_->objects.size())
  {
    lastError_ = tr("修复失败：模型对象无效");
    return false;
  }
  auto *obj = model_->objects[size_t(objectIndex)];
  if (!obj)
  {
    lastError_ = tr("修复失败：模型对象无效");
    return false;
  }
  for (auto *vol : obj->volumes)
  {
    if (vol)
    {
      // Phase 129 (POLISH-03): real mesh repair via admesh-layer its_* functions
      // (TriangleMesh.hpp:209 its_merge_vertices + :212 its_remove_degenerate_faces).
      // The former code was a no-op copy (set_mesh(move(copy))) that did not repair.
      Slic3r::TriangleMesh m = vol->mesh();
      Slic3r::its_merge_vertices(m.its);
      Slic3r::its_remove_degenerate_faces(m.its);
      vol->set_mesh(std::move(m));
    }
  }
  lastError_.clear();
  emit projectChanged();
  return true;
#else
  Q_UNUSED(objectIndex);
  lastError_ = tr("网格修复需要 libslic3r");
  return false;
#endif
}

bool ProjectServiceMock::reloadFromDisk(int objectIndex)
{
  if (objectIndex < 0 || objectIndex >= objectNames_.size())
  {
    lastError_ = tr("重新加载失败：对象索引无效");
    return false;
  }

#ifdef HAS_LIBSLIC3R
  if (!model_ || size_t(objectIndex) >= model_->objects.size() || !model_->objects[size_t(objectIndex)])
  {
    lastError_ = tr("重新加载失败：模型对象无效");
    return false;
  }
  auto *obj = model_->objects[size_t(objectIndex)];
  if (obj->input_file.empty())
  {
    lastError_ = tr("重新加载失败：对象无源文件路径");
    return false;
  }
  // Reload from disk: re-read the source file, then repair the mesh. If re-read
  // fails (file moved/deleted), fall back to repairing the existing mesh.
  // Phase 129 (POLISH-03): real mesh repair via its_merge_vertices +
  // its_remove_degenerate_faces (was a no-op copy before).
  for (auto *vol : obj->volumes)
  {
    if (vol)
    {
      Slic3r::TriangleMesh m = vol->mesh();
      // Best-effort re-read from disk (replaces the mesh if the source file
      // still exists and is readable).
      if (!obj->input_file.empty()) {
        Slic3r::TriangleMesh reloaded;
        if (reloaded.ReadSTLFile(obj->input_file.c_str()))
          m = std::move(reloaded);
      }
      Slic3r::its_merge_vertices(m.its);
      Slic3r::its_remove_degenerate_faces(m.its);
      vol->set_mesh(std::move(m));
    }
  }
  emit projectChanged();
  return true;
#else
  lastError_ = tr("重新加载需要 libslic3r");
  return false;
#endif
}

bool ProjectServiceMock::replaceVolume(int objectIndex, int volumeIndex, const QString &stlPath)
{
  if (objectIndex < 0 || objectIndex >= objectNames_.size())
  {
    lastError_ = tr("替换失败：对象索引无效");
    return false;
  }
  if (stlPath.isEmpty())
  {
    lastError_ = tr("替换失败：STL 路径为空");
    return false;
  }

#ifdef HAS_LIBSLIC3R
  if (!model_ || size_t(objectIndex) >= model_->objects.size() || !model_->objects[size_t(objectIndex)])
  {
    lastError_ = tr("替换失败：模型对象无效");
    return false;
  }
  auto *obj = model_->objects[size_t(objectIndex)];
  if (volumeIndex < 0 || size_t(volumeIndex) >= obj->volumes.size() || !obj->volumes[size_t(volumeIndex)])
  {
    lastError_ = tr("替换失败：部件索引无效");
    return false;
  }

  QFileInfo fi(stlPath);
  if (!fi.exists())
  {
    lastError_ = tr("替换失败：STL 文件不存在");
    return false;
  }

  try
  {
    Slic3r::Model loaded = Slic3r::Model::read_from_file(stlPath.toStdString());
    if (loaded.objects.empty() || !loaded.objects.front() || loaded.objects.front()->volumes.empty())
    {
      lastError_ = tr("替换失败：STL 文件为空或格式错误");
      return false;
    }
    auto *srcVol = loaded.objects.front()->volumes.front();
    obj->volumes[size_t(volumeIndex)]->set_mesh(srcVol->mesh());
    obj->volumes[size_t(volumeIndex)]->name = fi.baseName().toStdString();
    lastError_.clear();
    emit projectChanged();
    return true;
  }
  catch (const std::exception &ex)
  {
    lastError_ = tr("替换失败：%1").arg(QString::fromLatin1(ex.what()));
    return false;
  }
#else
  Q_UNUSED(volumeIndex);
  lastError_ = tr("替换 volume 需要 libslic3r");
  return false;
#endif
}

bool ProjectServiceMock::assembleObjects(const QList<int> &objIndices)
{
  if (objIndices.size() < 2)
  {
    lastError_ = tr("合并失败：至少需要选中两个对象");
    return false;
  }

#ifdef HAS_LIBSLIC3R
  if (!model_)
  {
    lastError_ = tr("合并失败：模型无效");
    return false;
  }

  // Validate all indices
  for (int idx : objIndices)
  {
    if (idx < 0 || size_t(idx) >= model_->objects.size() || !model_->objects[size_t(idx)])
    {
      lastError_ = tr("合并失败：对象索引无效 (%1)").arg(idx);
      return false;
    }
  }

  try
  {
    auto *targetObj = model_->objects[size_t(objIndices[0])];

    // Move volumes from subsequent objects into the first object
    for (int i = 1; i < objIndices.size(); ++i)
    {
      auto *srcObj = model_->objects[size_t(objIndices[i])];
      if (!srcObj) continue;
      for (auto *vol : srcObj->volumes)
      {
        if (vol)
          targetObj->add_volume(*vol);
      }
    }

    // Remove source objects in reverse order to preserve indices
    QList<int> sorted = objIndices;
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());
    for (int i = 1; i < sorted.size(); ++i)
    {
      model_->delete_object(size_t(sorted[i]));
    }

    lastError_.clear();
    emit projectChanged();
    return true;
  }
  catch (const std::exception &ex)
  {
    lastError_ = tr("合并失败：%1").arg(QString::fromLatin1(ex.what()));
    return false;
  }
#else
  Q_UNUSED(objIndices);
  lastError_ = tr("合并对象需要 libslic3r");
  return false;
#endif
}

bool ProjectServiceMock::duplicateInstanceAsObject(int objectIndex, int instanceIndex)
{
  if (objectIndex < 0 || objectIndex >= objectNames_.size())
  {
    lastError_ = tr("复制实例失败：对象索引无效");
    return false;
  }

#ifdef HAS_LIBSLIC3R
  if (!model_ || size_t(objectIndex) >= model_->objects.size() || !model_->objects[size_t(objectIndex)])
  {
    lastError_ = tr("复制实例失败：模型对象无效");
    return false;
  }
  auto *srcObj = model_->objects[size_t(objectIndex)];
  if (instanceIndex < 0 || size_t(instanceIndex) >= srcObj->instances.size() || !srcObj->instances[size_t(instanceIndex)])
  {
    lastError_ = tr("复制实例失败：实例索引无效");
    return false;
  }

  try
  {
    auto *inst = srcObj->instances[size_t(instanceIndex)];
    auto *newObj = model_->add_object(*srcObj);
    newObj->name = srcObj->name + " instance";
    // Set the new object's single instance to the original instance's offset
    newObj->instances.clear();
    auto *newInst = newObj->add_instance(*inst);
    (void)newInst;

    lastError_.clear();
    emit projectChanged();
    return true;
  }
  catch (const std::exception &ex)
  {
    lastError_ = tr("复制实例失败：%1").arg(QString::fromLatin1(ex.what()));
    return false;
  }
#else
  Q_UNUSED(instanceIndex);
  lastError_ = tr("复制实例需要 libslic3r");
  return false;
#endif
}

int ProjectServiceMock::addPrimitiveToPlate(int type)
{
  static const char *primNames[] = {
      QT_TRANSLATE_NOOP("ProjectServiceMock", "立方体"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "球体"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "圆柱体"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "圆锥体"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "截头锥体"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "圆环体"),
      QT_TRANSLATE_NOOP("ProjectServiceMock", "圆盘"),
  };
  constexpr int kTypeCount = sizeof(primNames) / sizeof(primNames[0]);
  if (type < 0 || type >= kTypeCount)
  {
    lastError_ = tr("创建原始体失败：类型无效");
    return -1;
  }

#ifdef HAS_LIBSLIC3R
  try
  {
    if (!model_)
    {
      model_ = new Slic3r::Model();
      model_->add_default_instances();
    }

    auto *obj = model_->add_object();
    obj->name = primNames[type];

    indexed_triangle_set its;
    switch (type)
    {
    case 0: its = Slic3r::its_make_cube(20, 20, 20); break;
    case 1: its = Slic3r::its_make_sphere(10, 2.0 * M_PI / 360.0); break;
    case 2: its = Slic3r::its_make_cylinder(10, 20, 2.0 * M_PI / 360.0); break;
    case 3: its = Slic3r::its_make_cylinder(5, 20, 2.0 * M_PI / 360.0); break; // cone = narrow cylinder stub
    case 4: its = Slic3r::its_make_cylinder(8, 15, 2.0 * M_PI / 360.0); break; // truncated cone stub
    case 5: its = Slic3r::its_make_sphere(10, 2.0 * M_PI / 360.0); break;      // torus = sphere stub
    default: its = Slic3r::its_make_cube(20, 5, 20); break;                     // disc
    }

    obj->add_volume(Slic3r::TriangleMesh(std::move(its)));
    obj->add_instance();

    int newIdx = int(model_->objects.size()) - 1;
    objectNames_.push_back(QString::fromUtf8(primNames[type]));
    objectModuleNames_.push_back("default");
    objectPrintableStates_.push_back(true);
    objectVisibleStates_.push_back(true);
    objectPositions_.push_back(QVector3D(0, 0, 0));
    objectRotations_.push_back(QVector3D(0, 0, 0));
    objectScales_.push_back(QVector3D(1, 1, 1));
    modelCount_ = objectNames_.size();
    MockVolumeEntry entry;
    entry.name = tr("Part 1");
    entry.type = MockVolumeType::ModelPart;
    m_mockVolumes[newIdx] = {entry};

    // Add to current plate
    if (m_plateList) {
      OWzx::PartPlate *cur = m_plateList->currentPlate();
      if (cur) cur->addInstance(newIdx, 0);
    }

    lastError_.clear();
    emit projectChanged();
    return newIdx;
  }
  catch (const std::exception &ex)
  {
    lastError_ = tr("创建原始体失败：%1").arg(QString::fromLatin1(ex.what()));
    return -1;
  }
#else
  // Mock mode: add to object list without real mesh
  int newIdx = objectNames_.size();
  objectNames_.push_back(QString::fromUtf8(primNames[type]));
  objectModuleNames_.push_back("default");
  objectPrintableStates_.push_back(true);
  objectVisibleStates_.push_back(true);
  objectPositions_.push_back(QVector3D(0, 0, 0));
  objectRotations_.push_back(QVector3D(0, 0, 0));
  objectScales_.push_back(QVector3D(1, 1, 1));
  modelCount_ = objectNames_.size();

  if (m_plateList) {
    OWzx::PartPlate *cur = m_plateList->currentPlate();
    if (cur) cur->addInstance(newIdx, 0);
  }

  lastError_.clear();
  emit projectChanged();
  return newIdx;
#endif
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

  // Update per-plate instance membership: swap fromIndex ↔ toIndex in every plate.
  if (m_plateList) {
    for (int pi = 0; pi < m_plateList->plateCount(); ++pi) {
      OWzx::PartPlate *p = m_plateList->plate(pi);
      if (!p) continue;
      // Snapshot then rebuild the swapped membership (cannot mutate set in place).
      std::set<std::pair<int,int>> rebuilt;
      for (const auto &pair : p->objToInstanceSet()) {
        std::pair<int,int> swapped = pair;
        if (pair.first == fromIndex) swapped.first = toIndex;
        else if (pair.first == toIndex) swapped.first = fromIndex;
        rebuilt.insert(swapped);
      }
      p->clearInstances();
      for (const auto &pair : rebuilt)
        p->addInstance(pair.first, pair.second);
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

// ── v3.0 Phase 16 (D-05): per-plate settings re-backed on PartPlate ──

int ProjectServiceMock::plateBedType(int plateIndex) const
{
  const OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  return p ? p->bedType() : 0;
}

bool ProjectServiceMock::setPlateBedType(int plateIndex, int bedType)
{
  OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  if (!p) return false;
  p->setBedType(bedType);
  emit projectChanged();
  return true;
}

int ProjectServiceMock::platePrintSequence(int plateIndex) const
{
  const OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  return p ? p->printSequence() : 0;
}

bool ProjectServiceMock::setPlatePrintSequence(int plateIndex, int seq)
{
  OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  if (!p) return false;
  p->setPrintSequence(seq);
  emit projectChanged();
  return true;
}

int ProjectServiceMock::plateSpiralMode(int plateIndex) const
{
  const OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  return p ? p->spiralMode() : 0;
}

bool ProjectServiceMock::setPlateSpiralMode(int plateIndex, int mode)
{
  OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  if (!p) return false;
  p->setSpiralMode(mode);
  emit projectChanged();
  return true;
}

// ── 首层耗材顺序（对齐上游 first_layer_print_sequence）──

int ProjectServiceMock::plateFirstLayerSeqChoice(int plateIndex) const
{
  const OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  return p ? p->firstLayerSeqChoice() : 0;
}

bool ProjectServiceMock::setPlateFirstLayerSeqChoice(int plateIndex, int choice)
{
  OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  if (!p) return false;
  p->setFirstLayerSeqChoice(choice);
  emit projectChanged();
  return true;
}

QVariantList ProjectServiceMock::plateFirstLayerSeqOrder(int plateIndex) const
{
  QVariantList result;
  const OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  if (p)
    for (int v : p->firstLayerSeqOrder())
      result.append(v);
  return result;
}

bool ProjectServiceMock::setPlateFirstLayerSeqOrder(int plateIndex, const QVariantList &order)
{
  OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  if (!p) return false;
  std::vector<int> intOrder;
  intOrder.reserve(order.size());
  for (const auto &v : order)
    intOrder.push_back(v.toInt());
  p->setFirstLayerSeqOrder(std::move(intOrder));
  emit projectChanged();
  return true;
}

// ── 其他层耗材顺序（对齐上游 other_layers_print_sequence + LayerPrintSequence）──

int ProjectServiceMock::plateOtherLayersSeqChoice(int plateIndex) const
{
  const OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  return p ? p->otherLayersSeqChoice() : 0;
}

bool ProjectServiceMock::setPlateOtherLayersSeqChoice(int plateIndex, int choice)
{
  OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  if (!p) return false;
  p->setOtherLayersSeqChoice(choice);
  emit projectChanged();
  return true;
}

int ProjectServiceMock::plateOtherLayersSeqCount(int plateIndex) const
{
  const OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  return p ? int(p->otherLayersSeqEntries().size()) : 0;
}

int ProjectServiceMock::plateOtherLayersSeqBegin(int plateIndex, int entryIndex) const
{
  const OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  if (!p) return 2;
  const auto &entries = p->otherLayersSeqEntries();
  if (entryIndex < 0 || entryIndex >= int(entries.size())) return 2;
  return entries[size_t(entryIndex)].beginLayer;
}

int ProjectServiceMock::plateOtherLayersSeqEnd(int plateIndex, int entryIndex) const
{
  const OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  if (!p) return 100;
  const auto &entries = p->otherLayersSeqEntries();
  if (entryIndex < 0 || entryIndex >= int(entries.size())) return 100;
  return entries[size_t(entryIndex)].endLayer;
}

QVariantList ProjectServiceMock::plateOtherLayersSeqOrder(int plateIndex, int entryIndex) const
{
  QVariantList result;
  const OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  if (!p) return result;
  const auto &entries = p->otherLayersSeqEntries();
  if (entryIndex >= 0 && entryIndex < int(entries.size()))
    for (int v : entries[size_t(entryIndex)].extruderOrder)
      result.append(v);
  return result;
}

bool ProjectServiceMock::addPlateOtherLayersSeqEntry(int plateIndex, int beginLayer, int endLayer)
{
  OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  if (!p) return false;
  if (beginLayer < 2) beginLayer = 2; // 对齐上游：begin_layer 必须 >= 2
  if (endLayer < beginLayer) endLayer = beginLayer;
  OWzx::LayerSeqEntry entry;
  entry.beginLayer = beginLayer;
  entry.endLayer = endLayer;
  // 默认顺序：按挤出机编号
  const int extCount = qMax(1, plateObjectCount(plateIndex));
  for (int i = 0; i < extCount; ++i)
    entry.extruderOrder.push_back(i);
  auto entries = p->otherLayersSeqEntries();  // copy (setter replaces)
  entries.push_back(std::move(entry));
  // 自动排序（对齐上游 auto-sort）
  std::sort(entries.begin(), entries.end(),
            [](const OWzx::LayerSeqEntry &a, const OWzx::LayerSeqEntry &b) {
              return a.beginLayer < b.beginLayer;
            });
  p->setOtherLayersSeqEntries(std::move(entries));
  emit projectChanged();
  return true;
}

bool ProjectServiceMock::removePlateOtherLayersSeqEntry(int plateIndex, int entryIndex)
{
  OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  if (!p) return false;
  auto entries = p->otherLayersSeqEntries();
  if (entryIndex < 0 || entryIndex >= int(entries.size())) return false;
  entries.erase(entries.begin() + entryIndex);
  p->setOtherLayersSeqEntries(std::move(entries));
  emit projectChanged();
  return true;
}

bool ProjectServiceMock::setPlateOtherLayersSeqRange(int plateIndex, int entryIndex, int beginLayer, int endLayer)
{
  OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  if (!p) return false;
  auto entries = p->otherLayersSeqEntries();
  if (entryIndex < 0 || entryIndex >= int(entries.size())) return false;
  if (beginLayer < 2) beginLayer = 2;
  if (endLayer < beginLayer) endLayer = beginLayer;
  entries[size_t(entryIndex)].beginLayer = beginLayer;
  entries[size_t(entryIndex)].endLayer = endLayer;
  // 自动排序
  std::sort(entries.begin(), entries.end(),
            [](const OWzx::LayerSeqEntry &a, const OWzx::LayerSeqEntry &b) {
              return a.beginLayer < b.beginLayer;
            });
  p->setOtherLayersSeqEntries(std::move(entries));
  emit projectChanged();
  return true;
}

bool ProjectServiceMock::setPlateOtherLayersSeqOrder(int plateIndex, int entryIndex, const QVariantList &order)
{
  OWzx::PartPlate *p = m_plateList ? m_plateList->plate(plateIndex) : nullptr;
  if (!p) return false;
  auto entries = p->otherLayersSeqEntries();
  if (entryIndex < 0 || entryIndex >= int(entries.size())) return false;
  std::vector<int> intOrder;
  intOrder.reserve(order.size());
  for (const auto &v : order)
    intOrder.push_back(v.toInt());
  entries[size_t(entryIndex)].extruderOrder = std::move(intOrder);
  p->setOtherLayersSeqEntries(std::move(entries));
  emit projectChanged();
  return true;
}

int ProjectServiceMock::plateExtruderCount(int plateIndex) const
{
  // Mock 模式：基于平板上的对象数推断耗材数（上限 4）
  return qMin(qMax(1, plateObjectCount(plateIndex)), 4);
}

QString ProjectServiceMock::plateThumbnailBase64(int plateIndex) const
{
#ifdef HAS_LIBSLIC3R
  if (!m_plateList) return {};
  const OWzx::PartPlate *p = m_plateList->plate(plateIndex);
  if (!p || !p->hasThumbnail()) return {};  // empty -- no fake placeholder
  const QImage img = p->thumbnail();
  if (img.isNull()) return {};
  QByteArray ba;
  QBuffer buf(&ba);
  buf.open(QIODevice::WriteOnly);
  // Phase 98 REVIEW W-1: guard the encode return so a failed PNG save returns
  // empty (no fake/broken base64 placeholder) -- honors the phase contract.
  if (!img.save(&buf, "PNG")) return {};
  return QString::fromLatin1(ba.toBase64());
#else
  Q_UNUSED(plateIndex);
  return {};
#endif
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
    if (m_plateList) {
      for (int pi = 0; pi < m_plateList->plateCount(); ++pi) {
        OWzx::PartPlate *p = m_plateList->plate(pi);
        if (!p) continue;
        std::set<std::pair<int,int>> rebuilt;
        for (const auto &pair : p->objToInstanceSet()) {
          std::pair<int,int> adj = pair;
          if (pair.first > sourceIndex) ++adj.first;
          rebuilt.insert(adj);
        }
        p->clearInstances();
        for (const auto &pair : rebuilt)
          p->addInstance(pair.first, pair.second);
      }
      // 将新对象加入当前 plate
      OWzx::PartPlate *cur = m_plateList->currentPlate();
      if (cur) cur->addInstance(insertPos, 0);
    }

    const int newIndex = insertPos;
#endif

    modelCount_ = objectNames_.size();
    lastError_.clear();
    emit projectChanged();
    emit plateDataLoaded(m_plateList ? m_plateList->plateCount() : 0);
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
    tempObj->split(&newObjects, false);

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
      if (m_plateList) {
        OWzx::PartPlate *cur = m_plateList->currentPlate();
        if (cur) cur->addInstance(int(model_->objects.size()) - 1, 0);
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

  if (m_plateList) {
    OWzx::PartPlate *cur = m_plateList->currentPlate();
    if (cur) cur->addInstance(insertPos, 0);
  }

  modelCount_ = objectNames_.size();
  lastError_.clear();
  emit projectChanged();
  emit plateDataLoaded(m_plateList ? m_plateList->plateCount() : 0);
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

// ---- ASM-01 (Phase 138): assemble-transform accessors ----
// Mirror the ordinary objectPosition/Rotation/Scale dual-path pattern but target
// ModelInstance::m_assemble_transformation (upstream Model.hpp:1253-1298) instead of
// m_transformation. Same slic3r(X,Y,Z) <-> GL(X,Z,Y) Y/Z swap for offset, deg<->rad
// for rotation. The Prepare/View3D path above is unchanged.
QVector3D ProjectServiceMock::assembleOffset(int index) const
{
#ifdef HAS_LIBSLIC3R
  if (model_ && index >= 0 && size_t(index) < model_->objects.size() &&
      model_->objects[size_t(index)] && !model_->objects[size_t(index)]->instances.empty())
  {
    const auto *inst = model_->objects[size_t(index)]->instances.front();
    if (inst)
    {
      const auto off = inst->get_assemble_offset();  // Model.hpp:1289
      // slic3r(X,Y,Z) -> GL(X,Z,Y): GL.x=slic3r.x, GL.y=slic3r.z, GL.z=slic3r.y
      return QVector3D(static_cast<float>(off.x()), static_cast<float>(off.z()), static_cast<float>(off.y()));
    }
  }
#endif
  return (index >= 0 && index < assembleOffsets_.size()) ? assembleOffsets_[index] : QVector3D(0, 0, 0);
}

QVector3D ProjectServiceMock::assembleRotation(int index) const
{
#ifdef HAS_LIBSLIC3R
  if (model_ && index >= 0 && size_t(index) < model_->objects.size() &&
      model_->objects[size_t(index)] && !model_->objects[size_t(index)]->instances.empty())
  {
    const auto *inst = model_->objects[size_t(index)]->instances.front();
    if (inst)
    {
      const auto rot = inst->get_assemble_transformation().get_rotation();  // radians in slic3r space
      return QVector3D(
        qRadiansToDegrees(static_cast<float>(rot.x())),
        qRadiansToDegrees(static_cast<float>(rot.y())),
        qRadiansToDegrees(static_cast<float>(rot.z())));
    }
  }
#endif
  return (index >= 0 && index < assembleRotations_.size()) ? assembleRotations_[index] : QVector3D(0, 0, 0);
}

QVector3D ProjectServiceMock::assembleScale(int index) const
{
#ifdef HAS_LIBSLIC3R
  if (model_ && index >= 0 && size_t(index) < model_->objects.size() &&
      model_->objects[size_t(index)] && !model_->objects[size_t(index)]->instances.empty())
  {
    const auto *inst = model_->objects[size_t(index)]->instances.front();
    if (inst)
    {
      const auto sc = inst->get_assemble_transformation().get_scaling_factor();
      return QVector3D(static_cast<float>(sc.x()), static_cast<float>(sc.y()), static_cast<float>(sc.z()));
    }
  }
#endif
  return (index >= 0 && index < assembleScales_.size()) ? assembleScales_[index] : QVector3D(1, 1, 1);
}

bool ProjectServiceMock::setAssembleOffset(int index, float x, float y, float z)
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
      // GL(X,Z,Y) -> slic3r(X,Y,Z): slic3r.x=GL.x, slic3r.y=GL.z, slic3r.z=GL.y
      inst->set_assemble_offset(Slic3r::Vec3d(x, z, y));  // Model.hpp:1290
      // set_assemble_offset does NOT flip m_assemble_initialized; re-assign the
      // transformation through set_assemble_transformation (Model.hpp:1281) so the
      // 3MF <assemble> block write gate (bbs_3mf.cpp:8076 is_assemble_initialized())
      // passes and the transform round-trips.
      inst->set_assemble_transformation(inst->get_assemble_transformation());
    }
  }
#endif
  while (assembleOffsets_.size() <= index)
    assembleOffsets_.append(QVector3D(0, 0, 0));
  assembleOffsets_[index] = QVector3D(x, y, z);
  emit projectChanged();
  return true;
}

bool ProjectServiceMock::setAssembleRotation(int index, float x, float y, float z)
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
      // degrees -> radians, written to m_assemble_transformation (Model.hpp:1291)
      inst->set_assemble_rotation(Slic3r::Vec3d(
        qDegreesToRadians(x), qDegreesToRadians(y), qDegreesToRadians(z)));
      // set_assemble_rotation does NOT flip m_assemble_initialized; re-assign through
      // set_assemble_transformation (Model.hpp:1281) so the 3MF <assemble> write gate
      // (bbs_3mf.cpp:8076) passes and the transform round-trips.
      inst->set_assemble_transformation(inst->get_assemble_transformation());
    }
  }
#endif
  while (assembleRotations_.size() <= index)
    assembleRotations_.append(QVector3D(0, 0, 0));
  assembleRotations_[index] = QVector3D(x, y, z);
  emit projectChanged();
  return true;
}

bool ProjectServiceMock::setAssembleScale(int index, float x, float y, float z)
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
      // get_assemble_transformation() returns const; take a copy, mutate, re-assign
      // via set_assemble_transformation (Model.hpp:1281) which also flips
      // m_assemble_initialized so the 3MF <assemble> write gate passes.
      auto t = inst->get_assemble_transformation();
      t.set_scaling_factor(Slic3r::Vec3d(x, y, z));
      inst->set_assemble_transformation(t);
    }
  }
#endif
  while (assembleScales_.size() <= index)
    assembleScales_.append(QVector3D(1, 1, 1));
  assembleScales_[index] = QVector3D(x, y, z);
  emit projectChanged();
  return true;
}

bool ProjectServiceMock::isAssembleInitialized(int index)
{
#ifdef HAS_LIBSLIC3R
  if (model_ && index >= 0 && size_t(index) < model_->objects.size() &&
      model_->objects[size_t(index)] && !model_->objects[size_t(index)]->instances.empty())
  {
    auto *inst = model_->objects[size_t(index)]->instances.front();
    if (inst)
      return inst->is_assemble_initialized();  // Model.hpp:1345 (non-const upstream)
  }
#endif
  return false;
}

void ProjectServiceMock::clearProject()
{
  if (loading_)
    cancelLoad();

  projectName_ = tr("未命名项目");
  modelCount_ = 0;
  // Reset plate storage to a fresh single-plate list (current unset), matching the
  // previous "no project loaded" state (plateCount=0, currentPlateIndex=-1).
  m_plateList = std::make_unique<OWzx::PartPlateList>();
  m_plateList->setCurrentPlateIndex(-1);
  sourceFilePath_.clear();
  objectNames_.clear();
  objectModuleNames_.clear();
  objectPrintableStates_.clear();
  objectVisibleStates_.clear();
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

#ifdef HAS_LIBSLIC3R
// Phase 96 (THUMBWRITE-01/02): convert a captured QImage into a
// Slic3r::ThumbnailData (width/height/RGBA bytes) for the 3MF writer.
// Format symmetry: the READ side (ProjectServiceMock.cpp:5455) wraps
// ThumbnailData::pixels as QImage::Format_RGBA8888, so the write side MUST
// produce RGBA8888 bytes (convertToFormat) for a clean round-trip.
// File-local (not a member) to keep bbs_3mf.hpp out of the public header,
// matching the buildPlateDataList design (ProjectServiceMock.cpp:5048-5049).
// NOTE: do NOT call ThumbnailData::set(w,h) — it fills the buffer with white
// (255) defaults (ThumbnailData.cpp:5-18); we want real captured pixels, so
// resize + memcpy the RGBA bytes directly.
static Slic3r::ThumbnailData qimageToThumbnailData(const QImage &img)
{
  Slic3r::ThumbnailData td;  // width=0/height=0/pixels empty (is_valid()==false)
  if (img.isNull())
    return td;  // empty cache -> writer silently skips (bbs_3mf.cpp:6135,7987 is_valid guard)
  const QImage rgba = img.convertToFormat(QImage::Format_RGBA8888);
  const int w = rgba.width();
  const int h = rgba.height();
  if (w <= 0 || h <= 0)
    return td;
  td.width = static_cast<unsigned int>(w);
  td.height = static_cast<unsigned int>(h);
  td.pixels.resize(static_cast<size_t>(w) * static_cast<size_t>(h) * 4);
  // RGBA8888 has a tightly-packed stride of w*4 (no padding for this format);
  // copy scanline-by-scanline to be robust against any future format that pads
  // the stride (matches the read side's per-row wrapping at :5455-5466).
  for (int y = 0; y < h; ++y) {
    const uchar *src = rgba.constScanLine(y);
    uchar *dst = td.pixels.data() + static_cast<size_t>(y) * w * 4;
    std::memcpy(dst, src, static_cast<size_t>(w) * 4);
  }
  return td;  // is_valid()==true (pixels.size()==4*w*h)
}

// Phase 97 (THUMBRT-01) read-side fix: extract a per-plate thumbnail PNG
// directly from a 3MF archive into a QImage so the thumbnail survives
// save->reload.
//
// ROOT CAUSE this fixes: the normal model-load path
// (_BBS_3MF_Importer::_load_model_from_file, bbs_3mf.cpp:1674) does NOT
// extract per-plate thumbnail bytes into PlateData::plate_thumbnail.pixels --
// it only copies the thumbnail_file STRING (bbs_3mf.cpp:2299, with the backup
// path prepended). The thumbnail-byte extraction (_extract_from_archive at
// bbs_3mf.cpp:1640) lives ONLY in load_gcode_3mf_from_stream (bbs_3mf.cpp:1492),
// which is not the path Model::read_from_file takes for a model 3MF. So after
// read_from_file, plate_thumbnail.pixels is empty and is_valid() is false, even
// though the PNG bytes are present in the archive at Metadata/plate_N.png.
// Phase 97's thumbnailSaveReloadRoundTrip test (the first end-to-end save->
// reload) exposed this gap. The fix: read Metadata/plate_{plateIndex+1}.png
// straight out of the zip and decode it. This mirrors what the upstream gcode
// path does at bbs_3mf.cpp:1640-1643 (mz_zip_reader_extract_to_mem on the
// thumbnail_file entry), just done here because the model-load path skips it.
// tdefl PNG is lossless, so the decoded pixels are the exact saved pixels
// (Phase 96 format symmetry holds for the decode).
//
// File-local (not a member, not in the public header) -- matches the
// qimageToThumbnailData design. plateIndex is 0-based, matching
// PlateData::plate_index; the archive entry is 1-based (Metadata/plate_1.png
// for plateIndex 0), matching the writer at bbs_3mf.cpp:6550.
static QImage extractPlateThumbnailFrom3mf(const QString &archivePath, int plateIndex)
{
  QImage thumb;  // null by default; an empty/non-existent entry leaves it null
  if (archivePath.isEmpty() || plateIndex < 0)
    return thumb;

  const std::string pathUtf8 = QDir::toNativeSeparators(archivePath).toStdString();
  mz_zip_archive archive;
  mz_zip_zero_struct(&archive);
  if (!Slic3r::open_zip_reader(&archive, pathUtf8))
    return thumb;

  const std::string entry = "Metadata/plate_" + std::to_string(plateIndex + 1) + ".png";
  const int index = mz_zip_reader_locate_file(&archive, entry.c_str(), nullptr, 0);
  if (index >= 0) {
    mz_zip_archive_file_stat stat;
    if (mz_zip_reader_file_stat(&archive, static_cast<mz_uint>(index), &stat)) {
      const size_t size = stat.m_uncomp_size;
      std::vector<unsigned char> buf(size);
      if (mz_zip_reader_extract_to_mem(&archive, static_cast<mz_uint>(index), buf.data(), size, 0))
        thumb.loadFromData(buf.data(), size, "PNG");
    }
  }
  Slic3r::close_zip_reader(&archive);
  return thumb;
}
#endif // HAS_LIBSLIC3R

// v3.0 Phase 18 (D-10): build a PlateData list from a PartPlateList so multi-plate
// state round-trips through store_bbs_3mf. The caller (saveProject) must
// release_PlateData_list() after use to free the heap PlateData objects.
// File-local free function (not a member) to keep libslic3r/Format/bbs_3mf.hpp
// out of the public header.
// Source truth: upstream PartPlate::store_to_3mf_structure (PartPlate.cpp:6160).
static Slic3r::PlateDataPtrs buildPlateDataList(const OWzx::PartPlateList *plateList)
{
  Slic3r::PlateDataPtrs result;
  if (!plateList) return result;
  for (int i = 0; i < plateList->plateCount(); ++i) {
    const OWzx::PartPlate *p = plateList->plate(i);
    Slic3r::PlateData *pd = new Slic3r::PlateData();
    pd->plate_index = i;
    pd->plate_name = p ? p->name() : std::string();
    pd->locked = p ? p->isLocked() : false;

    // Instance-level membership → objects_and_instances (vector<pair<int,int>>).
    if (p) {
      for (const auto &pr : p->objToInstanceSet())
        pd->objects_and_instances.emplace_back(pr.first, pr.second);
    }

    // Per-plate config (DynamicPrintConfig) — copy the PartPlate config (D-04 truth).
    if (p) {
      pd->config = p->config();
      // Bed-type / print-sequence / spiral live inside config (PlateData has no
      // standalone fields). Write them as config keys so they round-trip; these
      // are the same keys SliceService reads (SliceService.cpp:379-403).
      // curr_bed_type / print_sequence / filament_map_mode are coEnum options
      // (PrintConfig.cpp:1072,1829,2495); ConfigOptionEnum<T> overrides setInt
      // (Config.hpp:1989) so setInt is valid for them. spiral_mode is a coBool
      // option (PrintConfig.cpp:5822); ConfigOptionBool does NOT override setInt
      // (Config.hpp:1809 -- the base ConfigOption::setInt at Config.hpp:275
      // throws BadOptionTypeException "Calling ConfigOption::setInt on a
      // non-int ConfigOption"), so it MUST be written via the typed
      // ConfigOptionBool accessor. Phase 97 (THUMBRT round-trip) exposed this:
      // the first test to drive saveProject with a real model crashed in
      // buildPlateDataList on the spiral_mode setInt. Mirrors upstream
      // set_key_value("spiral_mode", new ConfigOptionBool(...)) (Plater.cpp).
      if (auto *opt = pd->config.option("curr_bed_type", true))
        opt->setInt(p->bedType());
      if (auto *opt = pd->config.option("print_sequence", true))
        opt->setInt(p->printSequence());
      if (auto *opt = pd->config.option<Slic3r::ConfigOptionBool>("spiral_mode", true))
        opt->value = (p->spiralMode() != 0);

      // v3.2 Phase 31 (FMAP-01/02) + v4.5 Phase 107 (FMAP-02): populate
      // filament_maps from the plate's manual mapping (1-based, matching
      // upstream PlateData::filament_maps at bbs_3mf.hpp:98). filament_map_mode
      // is written via the def-respecting coEnum accessor so the bbs_3mf writer
      // (bbs_3mf.cpp:7964-7967) serializes it as the upstream enum-name string
      // ("Auto For Flush"/"Auto For Match"/"Manual"). This is the FM-02
      // forward-compat write. The writer indexes
      // ConfigOptionEnum<FilamentMapMode>::get_enum_names()[getInt()] -- a
      // positional vector of size 3 (PrintConfig.cpp:579-584 has NO "Default"
      // entry, so names[3] is out of bounds). Therefore fmmDefault MUST be
      // resolved to a concrete mode BEFORE persistence (the upstream
      // get_real_filament_map_mode equivalent). Here we resolve fmmDefault ->
      // fmmAutoForFlush (the upstream default, PrintConfig.cpp:2509); a future
      // Phase 108+ readback layer will resolve it against the global config
      // like upstream PartPlate.cpp:317-328. setInt is valid here because
      // ConfigOptionEnum overrides it (Config.hpp:1989); using option(...,true)
      // (NOT set_key_value) keeps the def-created ConfigOptionEnumGeneric type
      // that the writer expects -- an earlier draft used set_key_value with a
      // raw ConfigOptionEnum<FilamentMapMode> which crashed
      // _add_project_config_file_to_archive on the type mismatch.
      pd->filament_maps = p->filamentMaps();
      Slic3r::FilamentMapMode writeMode = Slic3r::FilamentMapMode::fmmAutoForFlush;
      switch (p->filamentMapMode()) {
        case OWzx::FilamentMapMode::fmmAutoForFlush:
          writeMode = Slic3r::FilamentMapMode::fmmAutoForFlush; break;
        case OWzx::FilamentMapMode::fmmAutoForMatch:
          writeMode = Slic3r::FilamentMapMode::fmmAutoForMatch; break;
        case OWzx::FilamentMapMode::fmmManual:
          writeMode = Slic3r::FilamentMapMode::fmmManual; break;
        // fmmDefault is the per-plate "inherit from global" sentinel; resolve
        // to the upstream default here so the on-disk int stays in [0,2]
        // (avoids the names[3] OOB in the writer). See FM-02 / FM-07.
        case OWzx::FilamentMapMode::fmmDefault:
        default:
          writeMode = Slic3r::FilamentMapMode::fmmAutoForFlush; break;
      }
      if (auto *opt = pd->config.option("filament_map_mode", true))
        opt->setInt(static_cast<int>(writeMode));

      // Phase 111 (FMAP-04): also populate pd->config["filament_map"] (a
      // ConfigOptionInts) from the plate's manual mapping, NOT just the
      // pd->filament_maps member. The bbs_3mf model.config writer
      // (_add_model_config_file_to_archive, bbs_3mf.cpp:7969-7980) reads the
      // CONFIG option "filament_map" -- it does NOT read pd->filament_maps
      // (that member is read only by the slice_info.config writer at
      // bbs_3mf.cpp:8207, a separate XML the model-load path does not parse for
      // filament_maps). Without this config write, the <metadata
      // key="filament_maps"> entry is never emitted to model.config and the
      // array is lost on reload -- the deferred gap Phase 107 REVIEW noted in
      // filamentMapModeRoundTripManualPreserved's SCOPE NOTE. Mirrors upstream
      // Plater's set_key_value("filament_map", new ConfigOptionInts(...))
      // pattern (the def-respecting option<T>(...,true) form avoids the type-
      // mismatch crash an earlier Phase 107 draft hit on filament_map_mode).
      // The reader (bbs_3mf.cpp:4450-4459) parses this back into both
      // plate->filament_maps AND plate->config["filament_map"]; ProjectServiceMock
      // reads plate->filament_maps, so the array now survives save->reload.
      if (!p->filamentMaps().empty()) {
        if (auto *opt = pd->config.option<Slic3r::ConfigOptionInts>("filament_map", true))
          opt->values = p->filamentMaps();
      }

      // Phase 96 (THUMBWRITE-01): populate plate_thumbnail from the plate's
      // captured QImage cache (PartPlate::thumbnail(), populated by Phase 95
      // QRhi capture). The writer emits the XML <metadata thumbnail_file=
      // "Metadata/plate_N.png"> reference when plate_thumbnail.is_valid()
      // (bbs_3mf.cpp:7987) and writes the PNG bytes from StoreParams::
      // thumbnail_data (bbs_3mf.cpp:6133 — wired in saveProject). Empty cache
      // -> empty ThumbnailData -> writer silently skips (graceful). The guard
      // !thumbnail().isNull() ensures plates with no captured thumbnail (never
      // viewed, or cache invalidated by a content change per PartPlate.h:116,
      // 157,163) leave plate_thumbnail at its default — no regression when no
      // capture exists. pd->plate_thumbnail is a value ThumbnailData member
      // (bbs_3mf.hpp:80), owned by pd which saveProject frees via
      // release_PlateData_list (no separate lifetime management here).
      if (!p->thumbnail().isNull())
        pd->plate_thumbnail = qimageToThumbnailData(p->thumbnail());
    }

    result.push_back(pd);
  }
  return result;
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
  if (!model_)
  {
    // No real model — fall through to JSON mock save
  }
  else
  {
    // Real 3MF export via store_bbs_3mf (对齐上游 Plater::export_3mf → store_bbs_3mf)
    Slic3r::StoreParams params;
    params.path = filePath.toUtf8().constData();
    params.model = model_;
    params.strategy = Slic3r::SaveStrategy::Zip64;
    // Phase 107-01 diagnostic: StoreParams::config (bbs_3mf.hpp:234) is declared
    // WITHOUT a default member initializer and the empty StoreParams() {}
    // constructor does NOT zero it, so an unassigned params.config is a WILD
    // pointer. store_bbs_3mf dereferences it after a `config != nullptr` guard
    // (bbs_3mf.cpp:6350) which is UB on a wild pointer and intermittently
    // SEGFAULTs in _add_project_config_file_to_archive during ctest. We do not
    // currently persist a global project config (Qt6 has no preset-bundle write
    // path yet), so explicitly null it out to skip that writer branch. This is a
    // pre-existing latent hazard independent of FMAP-02; tracked separately.
    params.config = nullptr;

    // v3.0 Phase 18 (D-10): populate plate_data_list so multi-plate state round-trips.
    // Fixes the v2.9 blocker where save lost all plate names/locked/objects/config.
    Slic3r::PlateDataPtrs plateData = buildPlateDataList(m_plateList.get());
    params.plate_data_list = plateData;

    // Phase 96 (THUMBWRITE-02) + Phase 98 (THUMBVERIFY, REVIEW MEDIUM-3):
    // populate StoreParams::thumbnail_data with one entry per plate so the
    // writer archives Metadata/plate_{i+1}.png for EVERY plate with a valid
    // cached thumbnail. The writer iterates thumbnail_data by INDEX and maps
    // entry[index] to plate index (bbs_3mf.cpp:6133-6143, name built at 6550 as
    // "Metadata/plate_{index+1}.png"), so the vector MUST be plate-index
    // aligned. buildPlateDataList (above) emits the matching XML <metadata
    // thumbnail_file="Metadata/plate_N.png"> reference per plate (bbs_3mf.cpp
    // :7987). Phase 96 pushed only the current plate as thumbnail_data[0]; for
    // multi-plate projects that left plates > 0 with an XML ref but NO archived
    // PNG bytes, so on reload the read helper (extractPlateThumbnailFrom3mf)
    // could not find the entry and plates > 0 silently lost thumbnails
    // (Phase 97 REVIEW MEDIUM-3). Phase 98 pushes one entry per plate in order.
    // Plates with no cached thumbnail get a default (invalid) ThumbnailData
    // placeholder so indices stay aligned; the writer skips invalid entries
    // (is_valid() guard at bbs_3mf.cpp:6135). The vector guard at
    // bbs_3mf.cpp:6101 only rejects size > plate count, so equal size is valid.
    //
    // LIFETIME: StoreParams::thumbnail_data is vector<ThumbnailData*> (raw
    // pointers, bbs_3mf.hpp:235). The pointed-to ThumbnailData MUST outlive
    // store_bbs_3mf. plateThumbs is a single local vector in this block; we
    // reserve() the full capacity up front and take addresses only AFTER all
    // pushes are done, so no reallocation can invalidate the pointers before
    // store_bbs_3mf (called below at :5187) returns and the block exits past
    // the release_PlateData_list cleanup.
    std::vector<Slic3r::ThumbnailData> plateThumbs;
    if (m_plateList && m_plateList->plateCount() > 0)
    {
      plateThumbs.reserve(m_plateList->plateCount());
      for (int i = 0; i < m_plateList->plateCount(); ++i)
      {
        const OWzx::PartPlate *p = m_plateList->plate(i);
        if (p && !p->thumbnail().isNull())
          plateThumbs.push_back(qimageToThumbnailData(p->thumbnail()));
        else
          plateThumbs.push_back(Slic3r::ThumbnailData());  // invalid placeholder
      }
      // Take addresses of the non-const ThumbnailData entries and push as raw
      // pointers into StoreParams::thumbnail_data (vector<ThumbnailData*>,
      // bbs_3mf.hpp:235). store_bbs_3mf treats these as read-only inputs
      // (is_valid() + pixels/width/height reads, no mutation). plateThumbs
      // outlives the store call (single local vector, reserved up front — see
      // LIFETIME above). (Phase 95 REVIEW W-3: removed the prior const_cast by
      // iterating with a non-const reference so the address is already
      // ThumbnailData*.)
      for (Slic3r::ThumbnailData &td : plateThumbs)
        params.thumbnail_data.push_back(&td);
    }

    bool ok = false;
    try
    {
      ok = Slic3r::store_bbs_3mf(params);
    }
    catch (const std::exception &ex)
    {
      Slic3r::release_PlateData_list(plateData);  // free heap PlateData even on throw
      lastError_ = tr("3MF 保存异常: %1").arg(QString::fromStdString(ex.what()));
      emit projectChanged();
      return false;
    }

    // Free the heap PlateData objects after store (success or failure).
    Slic3r::release_PlateData_list(plateData);

    if (!ok)
    {
      lastError_ = tr("3MF 保存失败");
      emit projectChanged();
      return false;
    }

    sourceFilePath_ = filePath;
    QFileInfo fi(filePath);
    projectName_ = fi.completeBaseName();
    lastError_.clear();
    emit projectChanged();
    return true;
  }
#endif
  // JSON mock save (fallback when HAS_LIBSLIC3R is off or model_ is null)
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
  root[QStringLiteral("currentPlate")] = m_plateList ? m_plateList->currentPlateIndex() : -1;

  // Save plates
  QJsonArray platesArr;
  const int plateCount = m_plateList ? m_plateList->plateCount() : 0;
  for (int p = 0; p < plateCount; ++p)
  {
    const OWzx::PartPlate *pp = m_plateList->plate(p);
    QJsonObject plateObj;
    plateObj[QStringLiteral("name")] = pp
        ? QString::fromStdString(pp->name().empty() ? std::string() : pp->name())
        : QStringLiteral("Plate %1").arg(p + 1);
    if (pp && pp->name().empty())
      plateObj[QStringLiteral("name")] = QStringLiteral("Plate %1").arg(p + 1);
    plateObj[QStringLiteral("locked")] = pp ? pp->isLocked() : false;
    plateObj[QStringLiteral("bedType")] = pp ? pp->bedType() : 0;
    plateObj[QStringLiteral("printSequence")] = pp ? pp->printSequence() : 0;
    plateObj[QStringLiteral("spiralMode")] = pp ? pp->spiralMode() : 0;
    plateObj[QStringLiteral("firstLayerSeqChoice")] = pp ? pp->firstLayerSeqChoice() : 0;
    // Save first layer extruder order
    {
      QJsonArray seqArr;
      if (pp)
        for (int v : pp->firstLayerSeqOrder()) seqArr.append(v);
      plateObj[QStringLiteral("firstLayerSeqOrder")] = seqArr;
    }
    plateObj[QStringLiteral("otherLayersSeqChoice")] = pp ? pp->otherLayersSeqChoice() : 0;
    // Save other layers sequence entries
    {
      QJsonArray seqEntriesArr;
      if (pp)
      {
        for (const auto &entry : pp->otherLayersSeqEntries())
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
    const QList<int> plateObjs = pp ? m_plateList->objectIndicesOnPlate(p) : QList<int>{};
    for (int oi : plateObjs)
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

#ifdef HAS_LIBSLIC3R
  // 3MF project load (对齐上游 Plater::load_project → Model::read_from_archive)
  const QString suffix = fi.suffix().toLower();
  if (suffix == QStringLiteral("3mf") || suffix == QStringLiteral("cxprj"))
  {
    if (loading_)
    {
      lastError_ = tr("已有任务正在加载");
      emit projectChanged();
      return false;
    }

    loading_ = true;
    loadProgress_ = 0;
    activeCancelFlag_ = std::make_shared<std::atomic_bool>(false);
    emit loadingChanged();
    emit loadProgressChanged();

    const QString localPath = fi.absoluteFilePath();
    const QPointer<ProjectServiceMock> receiver(this);
    const auto cancelFlag = activeCancelFlag_;

    QtConcurrent::run([receiver, localPath, cancelFlag]()
    {
      auto *loadedModel = new Slic3r::Model();
      std::string err;
      int loadedPlateCount = 0;
      Slic3r::PlateDataPtrs plateDataList;
      Slic3r::DynamicPrintConfig loadedConfig;
      Slic3r::ConfigSubstitutionContext configSubstCtx(Slic3r::ForwardCompatibilitySubstitutionRule::Enable);
      bool isBbl3mf = false;
      Slic3r::Semver fileVersion;

      Slic3r::Import3mfProgressFn progressFn = [receiver, cancelFlag](int import_stage, int current, int total, bool &cancel) {
        cancel = cancelFlag && cancelFlag->load();
        int progress = (total > 0) ? qBound(0, int(double(current) * 100.0 / double(total)), 100) : 0;
        if (!receiver) return;
        const QString stageText = importStageToText(import_stage);
        QMetaObject::invokeMethod(receiver, [receiver, progress, stageText]() {
          if (!receiver) return;
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
        QMetaObject::invokeMethod(receiver, [receiver]() {
          if (!receiver) return;
          receiver->loadProgress_ = 10;
          emit receiver->loadProgressChanged();
          emit receiver->loadProgressUpdated(10, QObject::tr("读取项目"));
        }, Qt::QueuedConnection);

        Slic3r::En3mfType fileType = Slic3r::En3mfType::From_Other;
        Slic3r::Model loaded = Slic3r::Model::read_from_archive(
            localPath.toStdString(),
            &loadedConfig,
            &configSubstCtx,
            fileType,
            Slic3r::LoadStrategy::AddDefaultInstances | Slic3r::LoadStrategy::LoadModel | Slic3r::LoadStrategy::LoadConfig,
            &plateDataList,
            nullptr,
            &fileVersion,
            progressFn);
        *loadedModel = std::move(loaded);

        if (!loadedModel->objects.empty())
        {
          ok = true;
          loadedPlateCount = int(plateDataList.size());
        }
        else
        {
          err = "no model objects in project file";
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

      // Extract plate data BEFORE releasing (matching loadFile() pattern)
      QStringList loadedPlateNames;
      QList<QList<int>> loadedPlateObjectIndices;

      // v3.0 Phase 18 (D-12): capture locked + bed-type/print-seq/spiral onto the
      // receiver so the rebuild lambda restores ALL plate state.
      receiver->pendingPlateLocked_.clear();
      receiver->pendingPlateBedType_.clear();
      receiver->pendingPlatePrintSeq_.clear();
      receiver->pendingPlateSpiral_.clear();
      receiver->pendingPlateThumbnails_.clear();  // v3.2 Phase 30 (THUMB-02)
      receiver->pendingPlateFilamentMaps_.clear();   // v3.2 Phase 31 (FMAP-02)
      receiver->pendingPlateFilamentMapMode_.clear(); // v3.2 Phase 31 (FMAP-02)

      if (ok && !plateDataList.empty())
      {
        loadedPlateCount = int(plateDataList.size());
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

          receiver->pendingPlateLocked_.append(plate ? plate->locked : false);
          int bedType = 0, printSeq = 0, spiral = 0;
          if (plate) {
            if (auto *opt = plate->config.option("curr_bed_type"))
              bedType = int(opt->getInt());
            if (auto *opt = plate->config.option("print_sequence"))
              printSeq = int(opt->getInt());
            // Phase 97 fix (THUMBRT-01): spiral_mode is coBool; getInt() throws on
            // ConfigOptionBool (it does NOT override getInt). Read via the typed
            // Bool accessor instead. (Mirrors the loadFile read at ~608 and the
            // write-side fix at ~5159.)
            if (auto *opt = plate->config.option<Slic3r::ConfigOptionBool>("spiral_mode"))
              spiral = (opt->value ? 1 : 0);
          }
          receiver->pendingPlateBedType_.append(bedType);
          receiver->pendingPlatePrintSeq_.append(printSeq);
          receiver->pendingPlateSpiral_.append(spiral);

          // v3.2 Phase 31 (FMAP-02) + v4.5 Phase 107 (FMAP-02): extract
          // filament maps + mode. The mode read applies the Pitfall 2 / FM-03
          // migration so pre-v4.5 "Manual"=1 plates do NOT silently reload as
          // the new fmmAutoForMatch=1.
          QList<int> fmap;
          if (plate) {
            for (int m : plate->filament_maps) fmap.append(m);
          }
          int fmapMode = int(OWzx::FilamentMapMode::fmmAutoForFlush);
          if (plate) {
            if (auto *opt = plate->config.option("filament_map_mode")) {
              // The upstream 3MF reader (bbs_3mf.cpp:4443-4448) deserializes the
              // enum-name string ("Auto For Flush"/"Auto For Match"/"Manual")
              // via ConfigOptionEnum<FilamentMapMode>::from_string and stores a
              // TYPED ConfigOptionEnum (type()==coEnum) whose getInt() already
              // holds the correct 0/1/2 value. Trust those ints directly.
              if (opt->type() == Slic3r::coEnum) {
                fmapMode = int(opt->getInt());
              } else {
                // Legacy raw-int path (pre-v4.5 Qt6 files that bypassed enum
                // typing). Apply the FM-03 migration preserving pre-v4.5 intent
                // (raw 1 -> fmmManual, NOT fmmAutoForMatch). Phase 111 (FMAP-04
                // / R-01): factored into OWzx::migrateLegacyFilamentMapMode so
                // the legacy branch is unit-tested in isolation (PartPlateTests).
                fmapMode = int(OWzx::migrateLegacyFilamentMapMode(int(opt->getInt())));
              }
            }
          }
          receiver->pendingPlateFilamentMaps_.append(fmap);
          receiver->pendingPlateFilamentMapMode_.append(fmapMode);

          // v3.2 Phase 30 (THUMB-02) + Phase 97 fix (THUMBRT-01): restore the
          // persisted per-plate thumbnail so it survives save->reload. The
          // normal model-load path (_BBS_3MF_Importer::_load_model_from_file)
          // does NOT extract the per-plate thumbnail bytes into
          // plate_thumbnail.pixels -- it only copies the thumbnail_file STRING
          // (bbs_3mf.cpp:2299); the byte extraction lives only in
          // load_gcode_3mf_from_stream. So read the PNG straight out of the
          // archive (Metadata/plate_N.png) and decode it. tdefl PNG is lossless,
          // so the decoded pixels are the exact saved pixels (Phase 96 format
          // symmetry). See extractPlateThumbnailFrom3mf for the full root-cause
          // writeup. (Mirrors the loadFile block.)
          QImage loadedThumb;
          if (plate)
            loadedThumb = extractPlateThumbnailFrom3mf(localPath, plate->plate_index);
          receiver->pendingPlateThumbnails_.append(loadedThumb);

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

      if (!plateDataList.empty())
        Slic3r::release_PlateData_list(plateDataList);

      const bool canceled = cancelFlag && cancelFlag->load();

      // Collect object/plate data from loaded model
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
          names << ((obj && !obj->name.empty()) ? QString::fromStdString(obj->name) : QObject::tr("对象 %1").arg(int(i + 1)));
          moduleNames << ((obj && !obj->module_name.empty()) ? QString::fromStdString(obj->module_name) : QObject::tr("默认模块"));
          printableStates.append(obj && !obj->instances.empty() ? obj->instances.front()->printable : true);
          visibleStates.append(obj && !obj->instances.empty() ? obj->instances.front()->is_printable() : true);
        }

        if (loadedPlateNames.isEmpty())
        {
          loadedPlateCount = 1;
          loadedPlateNames << QObject::tr("平板 1");
          QList<int> allObjects;
          allObjects.reserve(int(loadedModel->objects.size()));
          for (int i = 0; i < int(loadedModel->objects.size()); ++i)
            allObjects.append(i);
          loadedPlateObjectIndices.append(allObjects);
        }
      }
      else
      {
        errorText = canceled ? QObject::tr("已取消加载") : (err.empty() ? QObject::tr("加载失败") : QString::fromStdString(err));
        qWarning() << "[ProjectService] project load failed:" << localPath << errorText;
      }

      if (!receiver)
      {
        delete loadedModel;
        return;
      }

      // Convert loadedConfig to QHash for main-thread emission
      QHash<QString, QVariant> loadedConfigMap;
#ifdef HAS_LIBSLIC3R
      if (ok && !canceled)
      {
        for (const auto &key_str : loadedConfig.keys())
        {
          const QString key = QString::fromStdString(key_str);
          const auto *opt = loadedConfig.option(key_str);
          if (!opt) continue;

          if (dynamic_cast<const Slic3r::ConfigOptionFloat*>(opt))
            loadedConfigMap[key] = dynamic_cast<const Slic3r::ConfigOptionFloat*>(opt)->value;
          else if (dynamic_cast<const Slic3r::ConfigOptionInt*>(opt))
            loadedConfigMap[key] = dynamic_cast<const Slic3r::ConfigOptionInt*>(opt)->value;
          else if (dynamic_cast<const Slic3r::ConfigOptionBool*>(opt))
            loadedConfigMap[key] = dynamic_cast<const Slic3r::ConfigOptionBool*>(opt)->value != 0;
          else
          {
            try { loadedConfigMap[key] = QString::fromStdString(opt->serialize()); }
            catch (...) {}
          }
        }
      }
#endif

      QMetaObject::invokeMethod(receiver, [receiver, loadedModel, ok, canceled, names, moduleNames, printableStates, visibleStates, errorText, loadedProjectName, loadedPlateCount, localPath, loadedPlateNames, loadedPlateObjectIndices, loadedConfigMap]() {
        if (!receiver) { delete loadedModel; return; }

        receiver->loading_ = false;
        receiver->activeCancelFlag_.reset();
        emit receiver->loadingChanged();
        receiver->loadProgress_ = (ok && !canceled) ? 100 : 0;
        emit receiver->loadProgressChanged();

        if (ok && !canceled)
        {
          if (receiver->model_) delete receiver->model_;
          receiver->model_ = loadedModel;
          receiver->projectName_ = loadedProjectName;
          receiver->sourceFilePath_ = localPath;
          receiver->objectNames_ = names;
          receiver->objectModuleNames_ = moduleNames;
          receiver->objectPrintableStates_ = printableStates;
          receiver->objectVisibleStates_ = visibleStates;
          receiver->modelCount_ = names.size();

          // Sync transforms from model
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
            receiver->objectPositions_.append(QVector3D(
                static_cast<float>(off.x()), static_cast<float>(off.z()), static_cast<float>(off.y())));
            const auto rot = inst->get_rotation();
            receiver->objectRotations_.append(QVector3D(
                qRadiansToDegrees(static_cast<float>(rot.x())),
                qRadiansToDegrees(static_cast<float>(rot.y())),
                qRadiansToDegrees(static_cast<float>(rot.z()))));
            const auto sc = inst->get_scaling_factor();
            receiver->objectScales_.append(QVector3D(
                static_cast<float>(sc.x()), static_cast<float>(sc.y()), static_cast<float>(sc.z())));
          }

          // v3.0 Phase 16 (D-05): rebuild m_plateList from loaded plate data.
          {
            QStringList plateNames = loadedPlateNames;
            QList<QList<int>> plateObjs = loadedPlateObjectIndices;
            if (plateObjs.isEmpty())
            {
              plateNames.clear();
              plateNames << QObject::tr("平板 1");
              QList<int> all;
              all.reserve(receiver->modelCount_);
              for (int i = 0; i < receiver->modelCount_; ++i) all.append(i);
              plateObjs.append(all);
            }
            receiver->m_plateList = std::make_unique<OWzx::PartPlateList>();
            receiver->m_plateList->resetToSinglePlate();
            for (int pi = 0; pi < plateObjs.size(); ++pi) {
              OWzx::PartPlate *p = (pi == 0) ? receiver->m_plateList->plate(0)
                                             : receiver->m_plateList->createPlate();
              if (!p) continue;
              if (pi < plateNames.size())
                p->setName(plateNames[pi].toStdString());
              p->clearInstances();
              for (int objIdx : plateObjs[pi])
                p->addInstance(objIdx, 0);
              // v3.0 Phase 18 (D-12): restore locked + bed-type/print-seq/spiral.
              if (pi < receiver->pendingPlateLocked_.size()) p->setLocked(receiver->pendingPlateLocked_[pi]);
              if (pi < receiver->pendingPlateBedType_.size()) p->setBedType(receiver->pendingPlateBedType_[pi]);
              if (pi < receiver->pendingPlatePrintSeq_.size()) p->setPrintSequence(receiver->pendingPlatePrintSeq_[pi]);
              if (pi < receiver->pendingPlateSpiral_.size()) p->setSpiralMode(receiver->pendingPlateSpiral_[pi]);
              // v3.2 Phase 31 (FMAP-02): restore filament maps + mode (Manual).
              if (pi < receiver->pendingPlateFilamentMaps_.size()) {
                const QList<int> &fm = receiver->pendingPlateFilamentMaps_[pi];
                p->setFilamentMaps(std::vector<int>(fm.begin(), fm.end()));
              }
              if (pi < receiver->pendingPlateFilamentMapMode_.size())
                p->setFilamentMapMode(receiver->pendingPlateFilamentMapMode_[pi]);
              // NOTE: thumbnail restoration is applied AFTER arrangeObjects below.
              // The in-loop setThumbnail would be wiped by arrangeObjects ->
              // rebuildPlatesAfterArrangement (clearInstances + addInstance each
              // invalidate m_thumbnail per PartPlate.h D-30-10). See the
              // post-arrange restore pass at ~5863.
            }
            const int reconstructed = receiver->m_plateList->plateCount();
            const int target = std::max(reconstructed, loadedPlateCount);
            for (int pi = reconstructed; pi < target; ++pi)
              receiver->m_plateList->createPlate();
            // v3.2 Phase 29 (RESEARCH §6): defensive origin recompute so plate
            // origins are consistent even if the auto-arrange-on-load below is
            // later conditioned out. Note: m_plate_width/depth default to 0 at
            // load time (no bed parse before arrange), so computeOrigin yields
            // (0,0,0) for a 1-plate load — correct. The auto-arrange at line
            // ~5368 re-derives real sizes via setPlateSize.
            receiver->m_plateList->refreshPlateOrigins();
            receiver->m_plateList->setCurrentPlateIndex(
                receiver->m_plateList->plateCount() > 0 ? 0 : -1);
            if (receiver->m_plateList->currentPlateIndex() < 0 &&
                receiver->m_plateList->plateCount() > 0)
              receiver->m_plateList->setCurrentPlateIndex(0);
          }
          receiver->lastError_.clear();

          // Auto-arrange after project load (对齐上游 arrange_loaded_object_to_new_position)
          // 同 loadFile：传入默认 220x220 热床 + 容错 vfn，避免 InfiniteBed 路径抛
          // bed_idx==-1（详见 arrangeObjects 注释）。
          receiver->arrangeObjects(5.0f, false, false,
                                   QStringLiteral("0,0,220,0,220,220,0,220"));

          // Phase 97 fix (THUMBRT-01): restore persisted thumbnails AFTER
          // arrangeObjects. arrangeObjects -> rebuildPlatesAfterArrangement
          // invalidates m_thumbnail via clearInstances()/addInstance()
          // (PartPlate.h D-30-10), so an in-loop setThumbnail (pre-arrange) would
          // be wiped before loadFinished fires. This post-arrange pass is the
          // last writer of m_thumbnail before the load completes, so the
          // restored thumbnail survives save->reload. (Mirrors loadFile above.)
          if (receiver->m_plateList) {
            for (int pi = 0; pi < receiver->m_plateList->plateCount(); ++pi) {
              if (pi >= receiver->pendingPlateThumbnails_.size()) break;
              const QImage &thumb = receiver->pendingPlateThumbnails_[pi];
              if (!thumb.isNull())
                receiver->m_plateList->plate(pi)->setThumbnail(thumb);
            }
          }

          emit receiver->projectChanged();
          emit receiver->plateDataLoaded(receiver->m_plateList ? receiver->m_plateList->plateCount() : 0);
          emit receiver->plateSelectionChanged();
          emit receiver->loadFinished(true, QObject::tr("项目加载完成"));
          if (!loadedConfigMap.isEmpty())
            emit receiver->projectConfigLoaded(loadedConfigMap);
        }
        else
        {
          delete loadedModel;
          receiver->modelCount_ = 0;
          receiver->m_plateList = std::make_unique<OWzx::PartPlateList>();
          receiver->m_plateList->setCurrentPlateIndex(-1);
          receiver->sourceFilePath_.clear();
          receiver->objectNames_.clear();
          receiver->objectModuleNames_.clear();
          receiver->objectPrintableStates_.clear();
          receiver->objectVisibleStates_.clear();
          receiver->lastError_ = errorText;
          emit receiver->projectChanged();
          emit receiver->plateDataLoaded(0);
          emit receiver->plateSelectionChanged();
          emit receiver->loadFinished(false, errorText);
        }
      }, Qt::QueuedConnection);
    });

    return true;
  }
#endif

  // JSON mock project load (fallback for non-3MF files)
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

  // v3.0 Phase 16 (D-05): rebuild m_plateList from the JSON plates array.
  m_plateList = std::make_unique<OWzx::PartPlateList>();
  m_plateList->resetToSinglePlate();

  // Restore plates and objects
  const QJsonArray platesArr = root.value(QStringLiteral("plates")).toArray();
  int globalObjectIdx = 0;
  for (int p = 0; p < platesArr.size(); ++p)
  {
    const QJsonObject plateObj = platesArr[p].toObject();
    OWzx::PartPlate *pp = (p == 0) ? m_plateList->plate(0) : m_plateList->createPlate();
    if (!pp) continue;  // kMaxPlateCount exceeded
    pp->setName(plateObj.value(QStringLiteral("name")).toString(
                  QStringLiteral("Plate %1").arg(p + 1)).toStdString());
    pp->setLocked(plateObj.value(QStringLiteral("locked")).toBool(false));
    pp->setBedType(plateObj.value(QStringLiteral("bedType")).toInt(0));
    pp->setPrintSequence(plateObj.value(QStringLiteral("printSequence")).toInt(0));
    pp->setSpiralMode(plateObj.value(QStringLiteral("spiralMode")).toInt(0));
    pp->setFirstLayerSeqChoice(plateObj.value(QStringLiteral("firstLayerSeqChoice")).toInt(0));
    // Restore first layer extruder order
    {
      std::vector<int> order;
      const QJsonArray orderArr = plateObj.value(QStringLiteral("firstLayerSeqOrder")).toArray();
      order.reserve(orderArr.size());
      for (const auto &v : orderArr) order.push_back(v.toInt());
      pp->setFirstLayerSeqOrder(std::move(order));
    }
    pp->setOtherLayersSeqChoice(plateObj.value(QStringLiteral("otherLayersSeqChoice")).toInt(0));
    // Restore other layers sequence entries
    {
      std::vector<OWzx::LayerSeqEntry> entries;
      const QJsonArray entriesArr = plateObj.value(QStringLiteral("otherLayersSeqEntries")).toArray();
      for (const auto &entryVal : entriesArr)
      {
        const QJsonObject entryObj = entryVal.toObject();
        OWzx::LayerSeqEntry entry;
        entry.beginLayer = entryObj.value(QStringLiteral("beginLayer")).toInt(2);
        entry.endLayer = entryObj.value(QStringLiteral("endLayer")).toInt(100);
        const QJsonArray orderArr = entryObj.value(QStringLiteral("order")).toArray();
        for (const auto &v : orderArr) entry.extruderOrder.push_back(v.toInt());
        entries.push_back(std::move(entry));
      }
      pp->setOtherLayersSeqEntries(std::move(entries));
    }
    pp->clearInstances();

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
      pp->addInstance(idx, 0);  // instance-level membership (single instance per object)
    }
  }

  modelCount_ = objectNames_.size();
  m_plateList->setCurrentPlateIndex(
      qBound(0, root.value(QStringLiteral("currentPlate")).toInt(0),
             qMax(0, m_plateList->plateCount() - 1)));
  loadProgress_ = 100;
  loading_ = false;
  lastError_.clear();

  emit projectChanged();
  emit plateDataLoaded(m_plateList ? m_plateList->plateCount() : 0);
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
        // Phase 91 (ASMEXPLODE-02): emit one ObjBatch per VOLUME (not one per
        // instance) so the CanvasAssembleView renderer can offset each volume
        // independently for the explosion view. Each per-volume batch keeps its
        // parent instance's stable objectId (siblings share the id), so Prepare's
        // selection highlight is byte-for-byte identical: uploadHighlightBuffer
        // unions the bounds of ALL batches matching a sourceObjectIndex
        // (RhiViewportRenderer.cpp uploadHighlightBuffer loop), and picking keys
        // on batch.sourceObjectIndex (the parallel array below), not on
        // renderObjectId uniqueness. Upstream GLVolumeCollection stores one
        // GLVolume per volume and m_explosion_ratio offsets each independently.
        const int32_t instanceObjectId = makeStableObjectId(obj, inst);

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

          // One batch per volume — exposes this volume's center via its bounds.
          ObjBatch batch;
          batch.objectId = instanceObjectId;

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

          if (!batch.verts.empty())
            batches.push_back(std::move(batch));
        }
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
