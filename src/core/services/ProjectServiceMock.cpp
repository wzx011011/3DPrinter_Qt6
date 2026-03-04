#include "ProjectServiceMock.h"

#include <QFileInfo>
#include <QDebug>
#include <algorithm>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Model.hpp>
#include <libslic3r/ModelObject.hpp>
#include <libslic3r/ModelVolume.hpp>
#include <libslic3r/ModelInstance.hpp>
#include <libslic3r/TriangleMesh.hpp>
#include <libslic3r/Format/3mf.hpp>
#include <libslic3r/PrintConfig.hpp>
#include <cstring> // memcpy
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

QString ProjectServiceMock::lastError() const
{
  return lastError_;
}

bool ProjectServiceMock::loadFile(const QString &filePath)
{
  QFileInfo fi(filePath);
  if (!fi.exists())
  {
    lastError_ = tr("文件不存在: %1").arg(filePath);
    qWarning() << lastError_;
    emit projectChanged();
    return false;
  }

  const QString ext = fi.suffix().toLower();

#ifdef HAS_LIBSLIC3R
  qInfo() << "ProjectService: loading" << filePath << "via libslic3r...";

  // 每次加载前先清空，避免旧对象残留导致崩溃
  model_->clear_objects();

  if (ext == "3mf")
  {
    Slic3r::DynamicPrintConfig config;
    Slic3r::ConfigSubstitutionContext configSubst(
        Slic3r::ForwardCompatibilitySubstitutionRule::EnableSilent);

    bool ok = Slic3r::load_3mf(
        filePath.toUtf8().constData(),
        config,
        configSubst,
        model_,
        /* check_version */ false);

    if (!ok)
    {
      lastError_ = tr("3MF 加载失败: %1").arg(filePath);
      qWarning() << lastError_;
      emit projectChanged();
      return false;
    }
  }
  else
  {
    // STL/OBJ/AMF can be loaded via Model::read_from_file
    try
    {
      Slic3r::Model loaded = Slic3r::Model::read_from_file(
          filePath.toUtf8().constData());
      // Merge loaded objects into current model
      for (auto *obj : loaded.objects)
      {
        model_->add_object(*obj);
      }
    }
    catch (const std::exception &e)
    {
      lastError_ = tr("模型加载失败: %1").arg(QString::fromUtf8(e.what()));
      qWarning() << lastError_;
      emit projectChanged();
      return false;
    }
  }

  // Update state from model
  objectNames_.clear();
  for (const auto *obj : model_->objects)
  {
    objectNames_ << QString::fromStdString(obj->name);
  }
  modelCount_ = static_cast<int>(model_->objects.size());
  projectName_ = fi.baseName();
  lastError_.clear();

  qInfo() << "ProjectService: loaded" << modelCount_ << "objects from" << fi.fileName();
  emit projectChanged();
  return true;

#else
  // Fallback mock
  modelCount_++;
  objectNames_ << fi.baseName();
  projectName_ = fi.baseName();
  lastError_.clear();
  qInfo() << "ProjectService (mock): loaded" << filePath;
  emit projectChanged();
  return true;
#endif
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
