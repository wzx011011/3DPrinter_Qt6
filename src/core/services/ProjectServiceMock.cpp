#include "ProjectServiceMock.h"

#include <QFileInfo>
#include <QDebug>
#include <algorithm>
#include <QMetaObject>
#include <QPointer>
#include <QtConcurrent/QtConcurrentRun>

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
      Slic3r::Model loaded = Slic3r::Model::read_from_file(localPath.toStdString(),
                                                            nullptr,
                                                            nullptr,
                                                            Slic3r::LoadStrategy::AddDefaultInstances,
                                                            &plateDataList,
                                                            nullptr,
                                                            nullptr,
                                                            nullptr,
                                                            progressFn);
      *loadedModel = std::move(loaded);
      loadedPlateCount = int(plateDataList.size());
      ok = true;
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
    }

    if (!receiver)
    {
      delete loadedModel;
      return;
    }

    QMetaObject::invokeMethod(receiver, [receiver, loadedModel, ok, canceled, names, errorText, loadedProjectName, loadedPlateCount, localPath]() {
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
        receiver->plateCount_ = loadedPlateCount;
        receiver->lastError_.clear();

        emit receiver->projectChanged();
        emit receiver->plateDataLoaded(receiver->plateCount_);
        emit receiver->loadFinished(true, QObject::tr("加载完成"));
      }
      else
      {
        delete loadedModel;
        receiver->modelCount_ = 0;
        receiver->plateCount_ = 0;
        receiver->sourceFilePath_.clear();
        receiver->objectNames_.clear();
        receiver->lastError_ = errorText;
        emit receiver->projectChanged();
        emit receiver->plateDataLoaded(0);
        emit receiver->loadFinished(false, errorText);
      }
    }, Qt::QueuedConnection); });

  return true;

#else
  // Fallback mock
  modelCount_++;
  plateCount_ = 1;
  objectNames_ << fi.baseName();
  projectName_ = fi.baseName();
  sourceFilePath_ = fi.absoluteFilePath();
  lastError_.clear();
  loading_ = false;
  loadProgress_ = 100;
  activeCancelFlag_.reset();
  emit loadingChanged();
  emit loadProgressChanged();
  emit loadProgressUpdated(100, tr("完成导入"));
  qInfo() << "ProjectService (mock): loaded" << filePath;
  emit projectChanged();
  emit plateDataLoaded(plateCount_);
  emit loadFinished(true, tr("加载完成"));
  return true;
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

void ProjectServiceMock::importMockModel()
{
  modelCount_++;
  objectNames_ << QString("MockModel_%1").arg(modelCount_);
  emit projectChanged();
}

void ProjectServiceMock::clearProject()
{
  if (loading_)
    cancelLoad();

  projectName_ = tr("未命名项目");
  modelCount_ = 0;
  plateCount_ = 0;
  sourceFilePath_.clear();
  objectNames_.clear();
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
}

QByteArray ProjectServiceMock::meshData() const
{
#ifdef HAS_LIBSLIC3R
  if (!model_ || model_->objects.empty())
    return {};

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

  int32_t linearId = 0;

  for (const auto *obj : model_->objects)
  {
    for (const auto *inst : obj->instances)
    {
      // 实例变换矩阵 (世界坐标)
      const Slic3r::Transform3d &instMat = inst->get_transformation().get_matrix();

      ObjBatch batch;
      batch.objectId = linearId++;

      for (const auto *vol : obj->volumes)
      {
        // 只渲染实体几何，跳过修改器 / 支撑区域 / 布尔减体
        if (!vol->is_model_part())
          continue;

        // volume 自身的局部变换
        const Slic3r::Transform3d &volMat = vol->get_transformation().get_matrix();
        const Slic3r::Transform3d combined = instMat * volMat;

        const auto &its = vol->mesh().its; // const ref — 零拷贝

        for (const auto &face : its.indices)
        {
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
    }
  }

  if (batches.empty())
    return {};

  // ── 计算缓冲区大小并写入 ──────────────────────────────────────────────────
  int32_t objCount = static_cast<int32_t>(batches.size());
  int totalBytes = static_cast<int>(sizeof(int32_t)); // objectCount header
  for (const auto &b : batches)
    totalBytes += static_cast<int>(2 * sizeof(int32_t) +            // objectId + triangleCount
                                   b.verts.size() * sizeof(float)); // vertex data
  totalBytes += 6 * static_cast<int>(sizeof(float));                // bbox trailer

  QByteArray buf;
  buf.resize(totalBytes);
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
    p += static_cast<int>(b.verts.size() * sizeof(float));
  }

  // bbox trailer
  const float bbox[6] = {bminX, bminY, bminZ, bmaxX, bmaxY, bmaxZ};
  memcpy(p, bbox, 6 * sizeof(float));

  qInfo("[ProjectService] meshData: %d batches, bbox=[%.1f..%.1f, %.1f..%.1f, %.1f..%.1f]",
        objCount, bminX, bmaxX, bminY, bmaxY, bminZ, bmaxZ);
  return buf;
#else
  return {};
#endif
}
