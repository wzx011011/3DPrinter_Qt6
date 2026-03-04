#include "ProjectServiceMock.h"

#include <QFileInfo>
#include <QDebug>
#include <algorithm>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Model.hpp>
#include <libslic3r/ModelObject.hpp>
#include <libslic3r/TriangleMesh.hpp>
#include <libslic3r/Format/3mf.hpp>
#include <libslic3r/PrintConfig.hpp>
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

  // ── 第一遍: 统计面数 + 计算包围盒（slic3r 坐标系）──────────────────
  size_t totalFaces = 0;
  float xmin = 1e30f, xmax = -1e30f;
  float ymin = 1e30f, ymax = -1e30f; // slic3r Y (→ GL Z)
  for (const auto *obj : model_->objects)
  {
    const Slic3r::TriangleMesh tm = obj->mesh();
    totalFaces += tm.its.indices.size();
    for (const auto &v : tm.its.vertices)
    {
      xmin = std::min(xmin, v.x()); xmax = std::max(xmax, v.x());
      ymin = std::min(ymin, v.y()); ymax = std::max(ymax, v.y());
    }
  }
  if (totalFaces == 0)
    return {};

  // 水平居中（XZ 平移），竖向 Y 保持从 0 起（模型坐在平台上）
  const float offsetX = (xmin + xmax) * 0.5f;
  const float offsetY = (ymin + ymax) * 0.5f; // slic3r Y -> GL Z

  // ── 第二遍: 填充顶点数组 ────────────────────────────────────────────
  QByteArray buf;
  buf.resize(static_cast<int>(totalFaces * 9 * sizeof(float)));
  float *dst = reinterpret_cast<float *>(buf.data());

  for (const auto *obj : model_->objects)
  {
    Slic3r::TriangleMesh tm = obj->mesh();
    const auto &verts = tm.its.vertices;
    const auto &faces = tm.its.indices;
    for (const auto &face : faces)
    {
      for (int k = 0; k < 3; ++k)
      {
        const auto &v = verts[face(k)];
        // 坐标系转换: libslic3r Z-up → GL Y-up，水平居中
        *dst++ = v.x() - offsetX;     // GL X = slic3r X (居中)
        *dst++ = v.z();                // GL Y = slic3r Z (高度，不偏移)
        *dst++ = v.y() - offsetY;     // GL Z = slic3r Y (居中)
      }
    }
  }
  return buf;
#else
  return {};
#endif
}
