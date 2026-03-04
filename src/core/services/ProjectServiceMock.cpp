#include "ProjectServiceMock.h"

#include <QFileInfo>
#include <QDebug>
#include <algorithm>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Model.hpp>
#include <libslic3r/ModelObject.hpp>
#include <libslic3r/ModelVolume.hpp>
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

  // 第一遍: 统计面数 + 计算包围盒，直接用 const & 引用，避免拷贝大 Mesh
  size_t totalFaces = 0;
  float xmin = 1e30f, xmax = -1e30f;
  float ymin = 1e30f, ymax = -1e30f;
  for (const auto *obj : model_->objects)
  {
    for (const auto *vol : obj->volumes)
    {
      const auto &its = vol->mesh().its; // const ref — 零拷贝
      totalFaces += its.indices.size();
      for (const auto &v : its.vertices)
      {
        xmin = std::min(xmin, v.x()); xmax = std::max(xmax, v.x());
        ymin = std::min(ymin, v.y()); ymax = std::max(ymax, v.y());
      }
    }
  }
  if (totalFaces == 0)
    return {};

  const float offsetX = (xmin + xmax) * 0.5f;
  const float offsetY = (ymin + ymax) * 0.5f;

  // 第二遍: 填充顶点 float 数组
  QByteArray buf;
  buf.resize(static_cast<int>(totalFaces * 9 * sizeof(float)));
  float *dst = reinterpret_cast<float *>(buf.data());

  for (const auto *obj : model_->objects)
  {
    for (const auto *vol : obj->volumes)
    {
      const auto &its = vol->mesh().its;
      for (const auto &face : its.indices)
      {
        for (int k = 0; k < 3; ++k)
        {
          const auto &v = its.vertices[face(k)];
          *dst++ = v.x() - offsetX;  // GL X
          *dst++ = v.z();             // GL Y (高度)
          *dst++ = v.y() - offsetY;  // GL Z
        }
      }
    }
  }
  return buf;
#else
  return {};
#endif
}
