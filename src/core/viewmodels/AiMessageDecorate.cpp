// AiMessageDecorate implementation — see AiMessageDecorate.h.

#include "AiMessageDecorate.h"

#include <QFileInfo>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

namespace OWzx::AiMessageDecorate {

QString prettyToolName(const QString &name) {
  const QString prefix = QStringLiteral("mcp__owzx__");
  return name.startsWith(prefix) ? name.mid(prefix.size()) : name;
}

bool isReadOnlyTool(const QString &name) {
  static const QSet<QString> kReadOnly = {
      QStringLiteral("get_app_state"),    QStringLiteral("get_scene"),
      QStringLiteral("get_plate_info"),   QStringLiteral("get_slice_status"),
      QStringLiteral("get_object_info"), QStringLiteral("list_config_keys")};
  return kReadOnly.contains(name);
}

QString toolLabel(const QString &name) {
  static const QHash<QString, QString> kLabels = {
      {QStringLiteral("get_app_state"), QStringLiteral("查看应用状态")},
      {QStringLiteral("get_scene"), QStringLiteral("查看场景对象")},
      {QStringLiteral("get_plate_info"), QStringLiteral("查看板信息")},
      {QStringLiteral("get_slice_status"), QStringLiteral("查看切片进度")},
      {QStringLiteral("get_object_info"), QStringLiteral("查看对象详情")},
      {QStringLiteral("list_config_keys"), QStringLiteral("查询可调参数")},
      {QStringLiteral("load_model"), QStringLiteral("导入模型")},
      {QStringLiteral("delete_object"), QStringLiteral("删除对象")},
      {QStringLiteral("duplicate_object"), QStringLiteral("复制对象")},
      {QStringLiteral("set_object_transform"), QStringLiteral("调整变换")},
      {QStringLiteral("arrange_objects"), QStringLiteral("自动排布")},
      {QStringLiteral("orient_objects"), QStringLiteral("自动摆向")},
      {QStringLiteral("set_config_value"), QStringLiteral("修改参数")},
      {QStringLiteral("select_preset"), QStringLiteral("切换预设")},
      {QStringLiteral("slice_plate"), QStringLiteral("切片")},
      {QStringLiteral("slice_all_plates"), QStringLiteral("切片全部板")},
      {QStringLiteral("cancel_slice"), QStringLiteral("取消切片")},
      {QStringLiteral("export_gcode"), QStringLiteral("导出 G-code")},
      {QStringLiteral("switch_page"), QStringLiteral("切换页面")},
      {QStringLiteral("select_object"), QStringLiteral("选中对象")},
      {QStringLiteral("toggle_sidebar"), QStringLiteral("收起/展开侧栏")},
      {QStringLiteral("undo"), QStringLiteral("撤销")},
      {QStringLiteral("redo"), QStringLiteral("重做")},
      {QStringLiteral("save_project"), QStringLiteral("保存项目")},
      {QStringLiteral("clear_project"), QStringLiteral("清空工作区")},
  };
  return kLabels.value(name, name);
}

namespace {

QString pageDisplayName(const QVariantMap &input) {
  const QVariant raw = input.value(QStringLiteral("page"));
  const QString text = raw.toString();
  if (text.compare(QStringLiteral("home"), Qt::CaseInsensitive) == 0)
    return QStringLiteral("首页");
  if (text.compare(QStringLiteral("prepare"), Qt::CaseInsensitive) == 0)
    return QStringLiteral("准备");
  if (text.compare(QStringLiteral("preview"), Qt::CaseInsensitive) == 0)
    return QStringLiteral("预览");
  if (text.compare(QStringLiteral("monitor"), Qt::CaseInsensitive) == 0 ||
      text.compare(QStringLiteral("device"), Qt::CaseInsensitive) == 0)
    return QStringLiteral("设备");
  if (text.compare(QStringLiteral("preferences"), Qt::CaseInsensitive) == 0)
    return QStringLiteral("偏好设置");
  return QStringLiteral("页面 %1").arg(text);
}

}  // namespace

QString toolDetail(const QString &name, const QVariantMap &input) {
  if (name == QLatin1String("load_model")) {
    const QString path = input.value(QStringLiteral("path")).toString();
    return path.isEmpty() ? QString() : QFileInfo(path).fileName();
  }
  if (name == QLatin1String("delete_object") ||
      name == QLatin1String("duplicate_object") ||
      name == QLatin1String("select_object") ||
      name == QLatin1String("set_object_transform")) {
    const int index = input.value(QStringLiteral("index"), -1).toInt();
    return index >= 0 ? QStringLiteral("第 %1 个对象").arg(index + 1) : QString();
  }
  if (name == QLatin1String("set_config_value")) {
    const QString key = input.value(QStringLiteral("key")).toString();
    if (key.isEmpty())
      return QString();
    return QStringLiteral("%1：%2").arg(
        key, input.value(QStringLiteral("value")).toString());
  }
  if (name == QLatin1String("select_preset")) {
    const QString name2 = input.value(QStringLiteral("name")).toString();
    return name2.isEmpty() ? QString()
                           : QStringLiteral("%1（%2）").arg(
                                 name2, input.value(QStringLiteral("kind")).toString());
  }
  if (name == QLatin1String("export_gcode") ||
      name == QLatin1String("save_project")) {
    const QString path = input.value(QStringLiteral("path")).toString();
    return path.isEmpty() ? QString() : QFileInfo(path).fileName();
  }
  if (name == QLatin1String("switch_page"))
    return pageDisplayName(input);
  return {};
}

QString riskText(const QString &name, const QVariantMap &input) {
  if (name == QLatin1String("delete_object")) {
    const int index = input.value(QStringLiteral("index"), -1).toInt();
    return index >= 0
               ? QStringLiteral("将从当前板删除第 %1 个对象，可通过撤销恢复。").arg(index + 1)
               : QStringLiteral("将从当前板删除该对象，可通过撤销恢复。");
  }
  if (name == QLatin1String("clear_project"))
    return QStringLiteral("将移除工作区的所有对象与板，且此操作不可撤销。");
  return {};
}

QString friendlyResult(bool ok, const QString &summary) {
  if (!ok)
    return summary.left(200);
  const QJsonDocument doc = QJsonDocument::fromJson(summary.toUtf8());
  if (!doc.isObject())
    return {};
  const QJsonObject obj = doc.object();
  if (obj.contains(QLatin1String("statusLabel")))
    return obj.value(QLatin1String("statusLabel")).toString();
  if (obj.contains(QLatin1String("appliedValue")))
    return QStringLiteral("已应用：%1 = %2")
        .arg(obj.value(QLatin1String("key")).toString(),
             obj.value(QLatin1String("appliedValue")).toVariant().toString());
  if (obj.contains(QLatin1String("path"))) {
    const QString path = obj.value(QLatin1String("path")).toString();
    if (!path.isEmpty())
      return QFileInfo(path).fileName();
  }
  if (obj.contains(QLatin1String("objectCountAfter")))
    return QStringLiteral("现有 %1 个对象")
        .arg(obj.value(QLatin1String("objectCountAfter")).toInt());
  if (obj.contains(QLatin1String("name")) && obj.contains(QLatin1String("kind")))
    return QStringLiteral("已切换到 %1").arg(obj.value(QLatin1String("name")).toString());
  return {};
}

QVariantMap decorateToolEntry(QVariantMap entry, const QString &rawName,
                              const QJsonObject &input) {
  const QString name = prettyToolName(rawName);
  const QVariantMap inputMap = input.toVariantMap();
  entry.insert(QStringLiteral("toolName"), name);
  entry.insert(QStringLiteral("toolLabel"), toolLabel(name));
  entry.insert(QStringLiteral("toolDetail"), toolDetail(name, inputMap));
  entry.insert(QStringLiteral("riskText"), riskText(name, inputMap));
  entry.insert(QStringLiteral("readOnly"), isReadOnlyTool(name));
  entry.insert(QStringLiteral("toolInput"), inputMap);
  return entry;
}

}  // namespace OWzx::AiMessageDecorate
