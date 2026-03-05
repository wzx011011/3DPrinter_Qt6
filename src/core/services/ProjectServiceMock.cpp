#include "ProjectServiceMock.h"

#include <QFileInfo>
#include <QDebug>
#include <QSet>
#include <algorithm>
#include <QMetaObject>
#include <QPointer>
#include <QtConcurrent/QtConcurrentRun>
#include <limits>
#include <cstdint>
#include <unordered_set>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Model.hpp>
#include <libslic3r/ModelObject.hpp>
#include <libslic3r/ModelVolume.hpp>
#include <libslic3r/ModelInstance.hpp>
#include <libslic3r/TriangleMesh.hpp>
#include <libslic3r/Format/3mf.hpp>
#include <libslic3r/Format/bbs_3mf.hpp>
#include <libslic3r/PrintConfig.hpp>
#include <cstring> // memcpy

namespace
{
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
    QString errorText;
    QString loadedProjectName;
    if (ok && !canceled)
    {
      loadedProjectName = QFileInfo(localPath).completeBaseName();
      const auto &objs = loadedModel->objects;
      names.reserve(int(objs.size()));
      for (size_t i = 0; i < objs.size(); ++i)
      {
        const auto *obj = objs[i];
        if (obj && !obj->name.empty())
          names << QString::fromStdString(obj->name);
        else
          names << QObject::tr("对象 %1").arg(int(i + 1));
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

    QMetaObject::invokeMethod(receiver, [receiver, loadedModel, ok, canceled, names, errorText, loadedProjectName, loadedPlateCount, localPath, loadedPlateNames, loadedPlateObjectIndices]() {
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
        receiver->modelCount_ = names.size();
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
        receiver->plateNames_.clear();
        receiver->plateObjectIndices_.clear();
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

QList<int> ProjectServiceMock::currentPlateObjectIndices() const
{
  if (currentPlateIndex_ < 0 || currentPlateIndex_ >= plateObjectIndices_.size())
    return {};
  return plateObjectIndices_[currentPlateIndex_];
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
    for (size_t i = 0; i < model_->objects.size(); ++i)
    {
      const auto *obj = model_->objects[i];
      if (obj && !obj->name.empty())
        objectNames_ << QString::fromStdString(obj->name);
      else
        objectNames_ << tr("对象 %1").arg(int(i + 1));
    }
#else
    objectNames_.removeAt(index);
#endif

    modelCount_ = objectNames_.size();
    if (modelCount_ <= 0)
    {
      plateCount_ = 0;
      currentPlateIndex_ = -1;
      plateNames_.clear();
      plateObjectIndices_.clear();
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
  plateNames_.clear();
  plateObjectIndices_.clear();
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
