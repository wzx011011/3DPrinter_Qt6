// AppToolRegistry implementation — OWzx-only AI control surface.
// See AppToolRegistry.h for the design contract. Every tool below wraps an
// existing ViewModel/Service API; nothing here re-implements business logic.

#include "AppToolRegistry.h"

#include <QFileInfo>
#include <QVector3D>

#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceService.h"
#include "core/viewmodels/ConfigViewModel.h"
#include "core/viewmodels/EditorViewModel.h"
#include "qml_gui/Models/ConfigOptionModel.h"

namespace OWzx {
namespace {

// ── JSON Schema helpers (MCP-compatible "object" schemas) ──────────────────
QJsonObject objSchema(QJsonObject props, QJsonArray required = {}) {
  QJsonObject s;
  s.insert(QStringLiteral("type"), QStringLiteral("object"));
  s.insert(QStringLiteral("properties"), props);
  if (!required.isEmpty())
    s.insert(QStringLiteral("required"), required);
  return s;
}

QJsonObject prop(const QString &type, const QString &description) {
  QJsonObject p;
  p.insert(QStringLiteral("type"), type);
  p.insert(QStringLiteral("description"), description);
  return p;
}

QJsonObject numberProp(const QString &description) {
  return prop(QStringLiteral("number"), description);
}

QJsonObject intProp(const QString &description) {
  return prop(QStringLiteral("integer"), description);
}

QJsonObject stringProp(const QString &description) {
  return prop(QStringLiteral("string"), description);
}

QJsonObject boolProp(const QString &description) {
  return prop(QStringLiteral("boolean"), description);
}

QJsonObject vec3Prop(const QString &description) {
  QJsonObject xyz;
  xyz.insert(QStringLiteral("x"), numberProp(QStringLiteral("X component")));
  xyz.insert(QStringLiteral("y"), numberProp(QStringLiteral("Y component")));
  xyz.insert(QStringLiteral("z"), numberProp(QStringLiteral("Z component")));
  QJsonObject p = prop(QStringLiteral("object"), description);
  p.insert(QStringLiteral("properties"), xyz);
  return p;
}

QString sliceStateName(SliceService::State state) {
  switch (state) {
    case SliceService::State::Idle: return QStringLiteral("idle");
    case SliceService::State::Slicing: return QStringLiteral("slicing");
    case SliceService::State::Exporting: return QStringLiteral("exporting");
    case SliceService::State::Completed: return QStringLiteral("completed");
    case SliceService::State::Cancelled: return QStringLiteral("cancelled");
    case SliceService::State::Error: return QStringLiteral("error");
  }
  return QStringLiteral("unknown");
}

QJsonObject vec3ToJson(const QVector3D &v) {
  QJsonObject o;
  o.insert(QStringLiteral("x"), v.x());
  o.insert(QStringLiteral("y"), v.y());
  o.insert(QStringLiteral("z"), v.z());
  return o;
}

QJsonArray intListToJson(const QList<int> &list) {
  QJsonArray a;
  for (int v : list) a.append(v);
  return a;
}

QString normalizePrintableArea(const QVariant &value) {
  QString serialized;
  if (value.typeId() == QMetaType::QVariantList) {
    const QVariantList points = value.toList();
    QStringList parts;
    parts.reserve(points.size());
    for (const QVariant &point : points)
      parts.append(point.toString());
    serialized = parts.join(QLatin1Char(','));
  } else if (value.typeId() == QMetaType::QStringList) {
    serialized = value.toStringList().join(QLatin1Char(','));
  } else {
    serialized = value.toString();
  }

  // Preset imports historically serialize points as "0x0,300x0,..." while
  // arrangeObjects consumes flat coordinate pairs separated by commas.
  return serialized.replace(QLatin1Char('x'), QLatin1Char(','));
}

}  // namespace

AppToolRegistry::AppToolRegistry(ProjectServiceMock *project, SliceService *slice,
                                 ConfigViewModel *config, EditorViewModel *editor,
                                 AppToolUiProvider *ui)
    : project_(project), slice_(slice), config_(config), editor_(editor), ui_(ui) {
  buildTools();
}

QJsonArray AppToolRegistry::toolDefinitions() const {
  QJsonArray defs;
  for (const AppTool &tool : tools_) {
    QJsonObject d;
    d.insert(QStringLiteral("name"), tool.name);
    d.insert(QStringLiteral("description"), tool.description);
    d.insert(QStringLiteral("inputSchema"), tool.inputSchema);
    // MCP-standard annotations: the sidecar auto-allows read-only tools and
    // only routes destructive ones to the user confirmation card.
    QJsonObject annotations;
    annotations.insert(QStringLiteral("readOnlyHint"), !tool.destructive);
    annotations.insert(QStringLiteral("destructiveHint"), tool.destructive);
    d.insert(QStringLiteral("annotations"), annotations);
    defs.append(d);
  }
  return defs;
}

AppToolResult AppToolRegistry::execute(const QString &name, const QJsonObject &args) {
  for (const AppTool &tool : tools_) {
    if (tool.name == name)
      return tool.execute(args);
  }
  return AppToolResult::failure(
      QStringLiteral("Unknown tool: %1").arg(name));
}

bool AppToolRegistry::hasTool(const QString &name) const {
  for (const AppTool &tool : tools_) {
    if (tool.name == name)
      return true;
  }
  return false;
}

void AppToolRegistry::buildTools() {
  // ── Query tools ───────────────────────────────────────────────────────────
  tools_.push_back(AppTool{
      QStringLiteral("get_app_state"),
      QStringLiteral("Overall application state: project name, source file, "
                     "object/plate counts, current plate, current UI page and "
                     "slice state. Call this first in a new conversation."),
      objSchema({}), false,
      [this](const QJsonObject &) -> AppToolResult {
        QJsonObject st;
        st.insert(QStringLiteral("projectName"), project_->projectName());
        st.insert(QStringLiteral("sourceFile"), project_->sourceFilePath());
        st.insert(QStringLiteral("objectCount"), project_->modelCount());
        st.insert(QStringLiteral("plateCount"), project_->plateCount());
        st.insert(QStringLiteral("currentPlateIndex"), project_->currentPlateIndex());
        st.insert(QStringLiteral("selectedObjectIndex"), editor_->selectedObjectIndex());
        if (ui_)
          st.insert(QStringLiteral("currentPage"), ui_->currentPage());
        QJsonObject slice;
        slice.insert(QStringLiteral("state"),
                     sliceStateName(slice_->sliceState()));
        slice.insert(QStringLiteral("progressPercent"), slice_->progress());
        slice.insert(QStringLiteral("busy"), slice_->slicing());
        slice.insert(QStringLiteral("statusLabel"), slice_->statusLabel());
        st.insert(QStringLiteral("slice"), slice);
        return AppToolResult::success(st);
      }});

  tools_.push_back(AppTool{
      QStringLiteral("get_scene"),
      QStringLiteral("All model objects in the scene with transforms "
                     "(position mm, rotation degrees, scale), printable/visible "
                     "flags, volume and instance counts, and owning plate."),
      objSchema({}), false,
      [this](const QJsonObject &) -> AppToolResult {
        const QStringList names = project_->objectNames();
        const int count = project_->modelCount();
        QJsonArray objects;
        for (int i = 0; i < count && i < names.size(); ++i) {
          QJsonObject o;
          o.insert(QStringLiteral("index"), i);
          o.insert(QStringLiteral("name"), names.at(i));
          o.insert(QStringLiteral("position"), vec3ToJson(project_->objectPosition(i)));
          o.insert(QStringLiteral("rotationDeg"), vec3ToJson(project_->objectRotation(i)));
          o.insert(QStringLiteral("scale"), vec3ToJson(project_->objectScale(i)));
          o.insert(QStringLiteral("printable"), project_->objectPrintable(i));
          o.insert(QStringLiteral("visible"), project_->objectVisible(i));
          o.insert(QStringLiteral("instanceCount"), project_->objectInstanceCount(i));
          o.insert(QStringLiteral("volumeCount"), project_->objectVolumeCount(i));
          o.insert(QStringLiteral("plateIndex"), project_->plateIndexForObject(i));
          objects.append(o);
        }
        QJsonObject d;
        d.insert(QStringLiteral("objects"), objects);
        return AppToolResult::success(d);
      }});

  tools_.push_back(AppTool{
      QStringLiteral("get_plate_info"),
      QStringLiteral("Per-plate info: name, member object indices, locked, "
                     "printable, sliced flag and estimated time when sliced."),
      objSchema({}), false,
      [this](const QJsonObject &) -> AppToolResult {
        const QStringList names = project_->plateNames();
        const int count = project_->plateCount();
        const QVariantList stale = editor_->stalePlateIndices();
        QJsonArray plates;
        for (int i = 0; i < count; ++i) {
          QJsonObject p;
          p.insert(QStringLiteral("index"), i);
          p.insert(QStringLiteral("name"), i < names.size() ? names.at(i) : QString());
          p.insert(QStringLiteral("objectIndices"),
                   intListToJson(project_->plateObjectIndices(i)));
          p.insert(QStringLiteral("locked"), project_->isPlateLocked(i));
          p.insert(QStringLiteral("printable"), project_->isPlatePrintable(i));
          p.insert(QStringLiteral("readyForSlice"),
                   project_->isPlateReadyForSlice(i));
          const bool sliced = slice_->hasPlateResult(i);
          p.insert(QStringLiteral("sliced"), sliced);
          if (sliced)
            p.insert(QStringLiteral("estimatedTime"), slice_->plateEstimatedTime(i));
          p.insert(QStringLiteral("staleResult"), stale.contains(i));
          plates.append(p);
        }
        QJsonObject d;
        d.insert(QStringLiteral("plates"), plates);
        d.insert(QStringLiteral("currentPlateIndex"), project_->currentPlateIndex());
        return AppToolResult::success(d);
      }});

  tools_.push_back(AppTool{
      QStringLiteral("get_slice_status"),
      QStringLiteral("Slice job status: state, progress percent, status label "
                     "and per-plate results (estimated time / weight / layers). "
                     "Poll this after slice_plate or slice_all_plates."),
      objSchema({}), false,
      [this](const QJsonObject &) -> AppToolResult {
        QJsonObject st;
        st.insert(QStringLiteral("state"), sliceStateName(slice_->sliceState()));
        st.insert(QStringLiteral("progressPercent"), slice_->progress());
        st.insert(QStringLiteral("busy"), slice_->slicing());
        st.insert(QStringLiteral("statusLabel"), slice_->statusLabel());
        st.insert(QStringLiteral("resultPlateIndex"), slice_->resultPlateIndex());
        QJsonArray results;
        const int plateCount = project_->plateCount();
        for (int i = 0; i < plateCount; ++i) {
          if (!slice_->hasPlateResult(i))
            continue;
          QJsonObject r;
          r.insert(QStringLiteral("plateIndex"), i);
          r.insert(QStringLiteral("estimatedTime"), slice_->plateEstimatedTime(i));
          r.insert(QStringLiteral("weight"), slice_->plateWeight(i));
          r.insert(QStringLiteral("layers"), slice_->plateLayerCount(i));
          r.insert(QStringLiteral("outputPath"), slice_->plateOutputPath(i));
          results.append(r);
        }
        st.insert(QStringLiteral("plateResults"), results);
        QJsonArray stale;
        for (const QVariant &v : editor_->stalePlateIndices())
          stale.append(v.toInt());
        st.insert(QStringLiteral("stalePlateIndices"), stale);
        return AppToolResult::success(st);
      }});

  tools_.push_back(AppTool{
      QStringLiteral("get_object_info"),
      QStringLiteral("Mesh statistics and volume list for one object: triangle "
                     "count, volume mm3, open edges, per-volume names/types."),
      objSchema(
          {{QStringLiteral("index"), intProp(QStringLiteral("Object index (see get_scene)"))}},
          {QStringLiteral("index")}),
      false,
      [this](const QJsonObject &args) -> AppToolResult {
        const int index = args.value(QStringLiteral("index")).toInt(-1);
        if (index < 0 || index >= project_->modelCount())
          return AppToolResult::failure(
              QStringLiteral("Object index %1 out of range (0..%2)")
                  .arg(index).arg(project_->modelCount() - 1));
        const QStringList names = project_->objectNames();
        QJsonObject o;
        o.insert(QStringLiteral("index"), index);
        if (index < names.size())
          o.insert(QStringLiteral("name"), names.at(index));
        o.insert(QStringLiteral("instanceCount"), project_->objectInstanceCount(index));
#ifdef HAS_LIBSLIC3R
        o.insert(QStringLiteral("triangleCount"), project_->objectTriangleCount(index));
        o.insert(QStringLiteral("volumeMm3"), project_->objectVolume(index));
        o.insert(QStringLiteral("openEdges"), project_->objectOpenEdges(index));
        o.insert(QStringLiteral("repairedErrors"), project_->objectRepairedErrors(index));
#endif
        QJsonArray volumes;
        const int volumeCount = project_->objectVolumeCount(index);
        for (int vi = 0; vi < volumeCount; ++vi) {
          QJsonObject v;
          v.insert(QStringLiteral("index"), vi);
          v.insert(QStringLiteral("name"), project_->objectVolumeName(index, vi));
          v.insert(QStringLiteral("type"), project_->objectVolumeTypeLabel(index, vi));
          volumes.append(v);
        }
        o.insert(QStringLiteral("volumes"), volumes);
        return AppToolResult::success(o);
      }});

  tools_.push_back(AppTool{
      QStringLiteral("list_config_keys"),
      QStringLiteral("Discover writable configuration keys with labels, types, "
                     "current values, units, ranges and enum options across the "
                     "printer/filament/print tiers. ALWAYS call this before "
                     "set_config_value to learn exact key names and value "
                     "formats."),
      objSchema({
          {QStringLiteral("filter"),
           stringProp(QStringLiteral("Case-insensitive substring matched against key and label"))},
          {QStringLiteral("tier"),
           prop(QStringLiteral("string"),
                QStringLiteral("Restrict to one tier: printer | filament | print"))},
          {QStringLiteral("limit"),
           intProp(QStringLiteral("Max entries to return (default 200)"))},
      }),
      false,
      [this](const QJsonObject &args) -> AppToolResult {
        const QString filter = args.value(QStringLiteral("filter")).toString();
        const QString tierArg = args.value(QStringLiteral("tier")).toString();
        const int limit = args.value(QStringLiteral("limit")).toInt(200);

        struct TierInfo {
          const char *name;
          QObject *object;
        };
        const TierInfo tiers[] = {
            {"printer", config_->machineOptions()},
            {"filament", config_->filamentOptions()},
            {"print", config_->printOptions()},
        };

        QJsonArray keys;
        bool truncated = false;
        int emitted = 0;
        for (const TierInfo &tier : tiers) {
          if (!tierArg.isEmpty() && QString::fromLatin1(tier.name) != tierArg)
            continue;
          auto *model = qobject_cast<ConfigOptionModel *>(tier.object);
          if (!model)
            continue;
          const int rows = model->rowCount();
          for (int row = 0; row < rows; ++row) {
            const QString key = model->optKey(row);
            const QString label = model->optLabel(row);
            if (!filter.isEmpty() &&
                !key.contains(filter, Qt::CaseInsensitive) &&
                !label.contains(filter, Qt::CaseInsensitive))
              continue;
            if (emitted >= limit) {
              truncated = true;
              break;
            }
            QJsonObject e;
            e.insert(QStringLiteral("tier"), QString::fromLatin1(tier.name));
            e.insert(QStringLiteral("key"), key);
            e.insert(QStringLiteral("label"), label);
            e.insert(QStringLiteral("type"), model->optType(row));
            e.insert(QStringLiteral("currentValue"),
                     QJsonValue::fromVariant(model->optValue(row)));
            e.insert(QStringLiteral("category"), model->optCategory(row));
            const QString unit = model->optUnit(row);
            if (!unit.isEmpty())
              e.insert(QStringLiteral("unit"), unit);
            const bool ro = model->optReadonly(row);
            if (ro)
              e.insert(QStringLiteral("readonly"), true);
            const QStringList enums = model->optEnumLabelsList(row);
            if (!enums.isEmpty()) {
              QJsonArray ev;
              for (const QString &s : enums) ev.append(s);
              e.insert(QStringLiteral("enumValues"), ev);
            }
            const double mn = model->optMin(row);
            const double mx = model->optMax(row);
            if (mn != 0.0 || mx != 0.0) {
              e.insert(QStringLiteral("min"), mn);
              e.insert(QStringLiteral("max"), mx);
            }
            keys.append(e);
            ++emitted;
          }
          if (truncated)
            break;
        }
        QJsonObject d;
        d.insert(QStringLiteral("keys"), keys);
        d.insert(QStringLiteral("count"), emitted);
        d.insert(QStringLiteral("truncated"), truncated);
        return AppToolResult::success(d);
      }});

  // ── Scene actions ─────────────────────────────────────────────────────────
  tools_.push_back(AppTool{
      QStringLiteral("load_model"),
      QStringLiteral("Import one model file (STL/OBJ/3MF/STEP) onto a plate. "
                     "Returns the new object count."),
      objSchema(
          {{QStringLiteral("path"), stringProp(QStringLiteral("Absolute path to the model file"))},
           {QStringLiteral("plateIndex"), intProp(QStringLiteral("Target plate (default: current)"))}},
          {QStringLiteral("path")}),
      false,
      [this](const QJsonObject &args) -> AppToolResult {
        const QString path = args.value(QStringLiteral("path")).toString();
        if (path.isEmpty() || !QFileInfo::exists(path))
          return AppToolResult::failure(
              QStringLiteral("Model file not found: %1").arg(path));
        const int plate = args.value(QStringLiteral("plateIndex")).toInt(
            project_->currentPlateIndex());
        if (plate < 0 || plate >= project_->plateCount())
          return AppToolResult::failure(QStringLiteral("plateIndex out of range"));
        if (!project_->addFilesToPlate(plate, {path}))
          return AppToolResult::failure(QStringLiteral("Import failed: %1").arg(path));
        editor_->refreshAfterLoad();
        QJsonObject d;
        d.insert(QStringLiteral("loaded"), true);
        d.insert(QStringLiteral("plateIndex"), plate);
        d.insert(QStringLiteral("objectCountAfter"), project_->modelCount());
        return AppToolResult::success(d);
      }});

  tools_.push_back(AppTool{
      QStringLiteral("delete_object"),
      QStringLiteral("Delete one object from the project (undoable)."),
      objSchema(
          {{QStringLiteral("index"), intProp(QStringLiteral("Object index (see get_scene)"))}},
          {QStringLiteral("index")}),
      true,
      [this](const QJsonObject &args) -> AppToolResult {
        const int index = args.value(QStringLiteral("index")).toInt(-1);
        if (index < 0 || index >= project_->modelCount())
          return AppToolResult::failure(QStringLiteral("index out of range"));
        // Route through the VM flow so the delete gets a proper undo command,
        // entry rebuild and slice-result invalidation (same as the UI button).
        if (!editor_->selectSourceObject(index))
          return AppToolResult::failure(QStringLiteral("select failed"));
        const int countBefore = project_->modelCount();
        editor_->deleteSelectedObjects();
        if (project_->modelCount() != countBefore - 1)
          return AppToolResult::failure(QStringLiteral("deleteObject failed"));
        return AppToolResult::success();
      }});

  tools_.push_back(AppTool{
      QStringLiteral("duplicate_object"),
      QStringLiteral("Duplicate one object (undoable); returns the new "
                     "object's index (appended at the end)."),
      objSchema(
          {{QStringLiteral("index"), intProp(QStringLiteral("Object index to duplicate"))}},
          {QStringLiteral("index")}),
      false,
      [this](const QJsonObject &args) -> AppToolResult {
        const int index = args.value(QStringLiteral("index")).toInt(-1);
        if (!editor_->selectSourceObject(index))
          return AppToolResult::failure(QStringLiteral("index out of range"));
        const int countBefore = project_->modelCount();
        editor_->duplicateSelectedObjects();
        if (project_->modelCount() != countBefore + 1)
          return AppToolResult::failure(QStringLiteral("duplicate failed"));
        QJsonObject d;
        d.insert(QStringLiteral("newIndex"), project_->modelCount() - 1);
        return AppToolResult::success(d);
      }});

  tools_.push_back(AppTool{
      QStringLiteral("set_object_transform"),
      QStringLiteral("Set position (mm), rotation (degrees) and/or scale of one "
                     "object. Omitted axes keep their current values. Returns "
                     "the final transform."),
      objSchema(
          {{QStringLiteral("index"), intProp(QStringLiteral("Object index"))},
           {QStringLiteral("position"), vec3Prop(QStringLiteral("Bed position in mm"))},
           {QStringLiteral("rotationDeg"), vec3Prop(QStringLiteral("Rotation in degrees"))},
           {QStringLiteral("scale"), vec3Prop(QStringLiteral("Scale factors (1 = 100%)"))}},
          {QStringLiteral("index")}),
      false,
      [this](const QJsonObject &args) -> AppToolResult {
        const int index = args.value(QStringLiteral("index")).toInt(-1);
        if (index < 0 || index >= project_->modelCount())
          return AppToolResult::failure(QStringLiteral("index out of range"));

        auto applyVec = [this, index, &args](const char *key,
                                      bool (ProjectServiceMock::*setter)(int, float, float, float),
                                      QVector3D (ProjectServiceMock::*getter)(int) const) {
          const QJsonObject v = args.value(QLatin1String(key)).toObject();
          if (v.isEmpty())
            return;
          (project_->*setter)(index,
                              float(v.value(QStringLiteral("x")).toDouble()),
                              float(v.value(QStringLiteral("y")).toDouble()),
                              float(v.value(QStringLiteral("z")).toDouble()));
        };
        applyVec("position", &ProjectServiceMock::setObjectPosition,
                 &ProjectServiceMock::objectPosition);
        applyVec("rotationDeg", &ProjectServiceMock::setObjectRotation,
                 &ProjectServiceMock::objectRotation);
        applyVec("scale", &ProjectServiceMock::setObjectScale,
                 &ProjectServiceMock::objectScale);
        editor_->refreshAfterExternalSceneChange();

        QJsonObject d;
        d.insert(QStringLiteral("index"), index);
        d.insert(QStringLiteral("position"), vec3ToJson(project_->objectPosition(index)));
        d.insert(QStringLiteral("rotationDeg"), vec3ToJson(project_->objectRotation(index)));
        d.insert(QStringLiteral("scale"), vec3ToJson(project_->objectScale(index)));
        return AppToolResult::success(d);
      }});

  tools_.push_back(AppTool{
      QStringLiteral("arrange_objects"),
      QStringLiteral("Auto-arrange all objects on the plates with libnest2d "
                     "packing (optional spacing and rotation)."),
      objSchema({
          {QStringLiteral("spacingMm"), numberProp(QStringLiteral("Gap between objects in mm (default 10)"))},
          {QStringLiteral("allowRotation"), boolProp(QStringLiteral("Allow 90-degree rotations (default false)"))},
      }),
      false,
      [this](const QJsonObject &args) -> AppToolResult {
        const float spacing = float(args.value(QStringLiteral("spacingMm")).toDouble(10.0));
        const bool rotation = args.value(QStringLiteral("allowRotation")).toBool(false);
        // Mirror EditorViewModel::arrangeAll(): arrange needs the real bed
        // shape from the printer preset. Without it the service falls back
        // to InfiniteBed, which returns false whenever the plate grid
        // assigns objects beyond bed 0 (observed live: "arrange failed").
        QString printableArea;
        if (config_)
          printableArea = normalizePrintableArea(
              config_->mergedConfigValues().value(QStringLiteral("printable_area")));
        if (!project_->arrangeObjects(spacing, rotation, /*alignY=*/false, printableArea))
          return AppToolResult::failure(QStringLiteral("arrange failed (all plates locked?)"));
        editor_->refreshAfterExternalSceneChange();
        return AppToolResult::success();
      }});

  tools_.push_back(AppTool{
      QStringLiteral("orient_objects"),
      QStringLiteral("Auto-orient all objects for best printability "
                     "(Slic3r::orientation::orient on every object)."),
      objSchema({}), false,
      [this](const QJsonObject &) -> AppToolResult {
        const int count = project_->modelCount();
        int oriented = 0;
        for (int i = 0; i < count; ++i) {
          if (project_->orientObject(i))
            ++oriented;
        }
        if (oriented == 0 && count > 0)
          return AppToolResult::failure(QStringLiteral("orient failed for all objects"));
        editor_->refreshAfterExternalSceneChange();
        QJsonObject d;
        d.insert(QStringLiteral("orientedCount"), oriented);
        return AppToolResult::success(d);
      }});

  // ── Config actions ────────────────────────────────────────────────────────
  tools_.push_back(AppTool{
      QStringLiteral("set_config_value"),
      QStringLiteral("Set one configuration value on the active tier (same "
                     "pipeline as editing it in the settings UI). Call "
                     "list_config_keys first for exact key names and value "
                     "formats (percents are strings like \"15%\"). Returns the "
                     "applied value."),
      objSchema(
          {{QStringLiteral("key"), stringProp(QStringLiteral("Config key, e.g. wall_loops / sparse_infill_density / nozzle_temp"))},
           {QStringLiteral("value"), prop(QStringLiteral("string"),
                                          QStringLiteral("New value as a string (\"4\", \"true\", \"15%\", \"0.2mm\"); format must match list_config_keys currentValue"))}},
          {QStringLiteral("key"), QStringLiteral("value")}),
      false,
      [this](const QJsonObject &args) -> AppToolResult {
        const QString key = args.value(QStringLiteral("key")).toString();
        if (key.isEmpty())
          return AppToolResult::failure(QStringLiteral("key is required"));
        const QJsonValue raw = args.value(QStringLiteral("value"));
        const QVariant value = raw.toVariant();

        // Numeric values may need string coercion for string-typed options
        // (percents, enums by label) and vice versa — try the ladder.
        if (config_->setValue(key, value))
          return AppToolResult::success(
              QJsonObject{{QStringLiteral("key"), key},
                          {QStringLiteral("appliedValue"), raw}});
        if (raw.isDouble()) {
          const QString asString = QString::number(raw.toDouble());
          if (config_->setValue(key, asString))
            return AppToolResult::success(
                QJsonObject{{QStringLiteral("key"), key},
                            {QStringLiteral("appliedValue"), asString}});
        } else if (raw.isString()) {
          bool numOk = false;
          const double asDouble = raw.toString().toDouble(&numOk);
          if (numOk && config_->setValue(key, asDouble))
            return AppToolResult::success(
                QJsonObject{{QStringLiteral("key"), key},
                            {QStringLiteral("appliedValue"), asDouble}});
        }
        return AppToolResult::failure(
            QStringLiteral("setValue failed for key '%1' (unknown key or bad value type); "
                           "check list_config_keys")
                .arg(key));
      }});

  tools_.push_back(AppTool{
      QStringLiteral("select_preset"),
      QStringLiteral("Switch the active printer, filament or print preset by "
                     "name. Use list_config_keys-free preset names shown in "
                     "get_app_state, or the preset selectors' names."),
      objSchema(
          {{QStringLiteral("kind"), prop(QStringLiteral("string"), QStringLiteral("printer | filament | print"))},
           {QStringLiteral("name"), stringProp(QStringLiteral("Preset name"))}},
          {QStringLiteral("kind"), QStringLiteral("name")}),
      false,
      [this](const QJsonObject &args) -> AppToolResult {
        const QString kind = args.value(QStringLiteral("kind")).toString();
        const QString name = args.value(QStringLiteral("name")).toString();
        if (kind == QStringLiteral("printer")) {
          config_->setCurrentPrinterPreset(name);
          if (config_->currentPrinterPreset() != name)
            return AppToolResult::failure(QStringLiteral("preset not applied"));
        } else if (kind == QStringLiteral("filament")) {
          config_->setCurrentFilamentPreset(name);
          if (config_->currentFilamentPreset() != name)
            return AppToolResult::failure(QStringLiteral("preset not applied"));
        } else if (kind == QStringLiteral("print")) {
          config_->setCurrentPrintPreset(name);
          if (config_->currentPrintPreset() != name)
            return AppToolResult::failure(QStringLiteral("preset not applied"));
        } else {
          return AppToolResult::failure(
              QStringLiteral("kind must be printer|filament|print, got '%1'").arg(kind));
        }
        QJsonObject d;
        d.insert(QStringLiteral("kind"), kind);
        d.insert(QStringLiteral("name"), name);
        return AppToolResult::success(d);
      }});

  // ── Slicing actions (async; poll get_slice_status) ───────────────────────
  tools_.push_back(AppTool{
      QStringLiteral("slice_plate"),
      QStringLiteral("Start slicing one plate (async). Returns immediately; "
                     "poll get_slice_status until state is completed/error."),
      objSchema({
          {QStringLiteral("plateIndex"), intProp(QStringLiteral("Plate to slice (default: current)"))},
      }),
      false,
      [this](const QJsonObject &args) -> AppToolResult {
        const int plate = args.value(QStringLiteral("plateIndex")).toInt(
            project_->currentPlateIndex());
        if (plate < 0 || plate >= project_->plateCount())
          return AppToolResult::failure(QStringLiteral("plateIndex out of range"));
        if (!project_->isPlateReadyForSlice(plate))
          return AppToolResult::failure(
              QStringLiteral("Plate %1 is not ready (empty, outside printable "
                             "area or apply-invalid)").arg(plate));
        if (slice_->slicing())
          return AppToolResult::failure(QStringLiteral("a slice job is already running"));
        slice_->startSlicePlate(plate);
        QJsonObject d;
        d.insert(QStringLiteral("started"), true);
        d.insert(QStringLiteral("plateIndex"), plate);
        d.insert(QStringLiteral("poll"), QStringLiteral("get_slice_status"));
        return AppToolResult::success(d);
      }});

  tools_.push_back(AppTool{
      QStringLiteral("slice_all_plates"),
      QStringLiteral("Slice every printable, ready plate in sequence (async). "
                     "Poll get_slice_status."),
      objSchema({}), false,
      [this](const QJsonObject &) -> AppToolResult {
        if (slice_->slicing())
          return AppToolResult::failure(QStringLiteral("a slice job is already running"));
        editor_->requestSliceAll();
        return AppToolResult::success(
            QJsonObject{{QStringLiteral("started"), true},
                        {QStringLiteral("poll"), QStringLiteral("get_slice_status")}});
      }});

  tools_.push_back(AppTool{
      QStringLiteral("cancel_slice"),
      QStringLiteral("Cancel the in-flight slice or export job."),
      objSchema({}), false,
      [this](const QJsonObject &) -> AppToolResult {
        slice_->cancelSlice();
        return AppToolResult::success();
      }});

  tools_.push_back(AppTool{
      QStringLiteral("export_gcode"),
      QStringLiteral("Export a sliced plate's G-code to a path (plate must "
                     "already be sliced — check get_slice_status)."),
      objSchema(
          {{QStringLiteral("path"), stringProp(QStringLiteral("Destination .gcode file path"))},
           {QStringLiteral("plateIndex"), intProp(QStringLiteral("Plate to export (default: last result plate)"))}},
          {QStringLiteral("path")}),
      false,
      [this](const QJsonObject &args) -> AppToolResult {
        const QString path = args.value(QStringLiteral("path")).toString();
        if (path.isEmpty())
          return AppToolResult::failure(QStringLiteral("path is required"));
        const int plate = args.value(QStringLiteral("plateIndex")).toInt(
            slice_->resultPlateIndex());
        if (plate < 0 || !slice_->hasPlateResult(plate))
          return AppToolResult::failure(
              QStringLiteral("Plate %1 has no slice result; slice it first").arg(plate));
        if (!slice_->exportPlateGCodeToPath(plate, path))
          return AppToolResult::failure(QStringLiteral("export failed to start"));
        QJsonObject d;
        d.insert(QStringLiteral("started"), true);
        d.insert(QStringLiteral("plateIndex"), plate);
        d.insert(QStringLiteral("path"), path);
        return AppToolResult::success(d);
      }});

  // ── UI actions (visible application control) ──────────────────────────────
  tools_.push_back(AppTool{
      QStringLiteral("switch_page"),
      QStringLiteral("Switch the visible UI page. Pages: 0 home, 1 prepare, "
                     "2 preview, 3 device, 8 preferences (name or index "
                     "accepted)."),
      objSchema(
          {{QStringLiteral("page"), prop(QStringLiteral("string"),
                                        QStringLiteral("Page name (home|prepare|monitor|preferences) or numeric index \"0\"-\"8\" as string"))}},
          {QStringLiteral("page")}),
      false,
      [this](const QJsonObject &args) -> AppToolResult {
        if (!ui_)
          return AppToolResult::failure(QStringLiteral("UI control unavailable (headless)"));
        int page = -1;
        const QJsonValue v = args.value(QStringLiteral("page"));
        if (v.isString()) {
          const QString name = v.toString();
          if (name == QLatin1String("home")) page = 0;
          else if (name == QLatin1String("prepare")) page = 1;
          else if (name == QLatin1String("preview")) page = 2;
          else if (name == QLatin1String("monitor")) page = 3;
          else if (name == QLatin1String("preferences")) page = 8;
          else {
            bool numOk = false;
            const int asIndex = name.toInt(&numOk);
            if (numOk)
              page = asIndex;
            else
              return AppToolResult::failure(
                  QStringLiteral("Unknown page name '%1'").arg(name));
          }
        } else {
          page = v.toInt(-1);
        }
        if (!ui_->switchPage(page))
          return AppToolResult::failure(QStringLiteral("switchPage rejected"));
        QJsonObject d;
        d.insert(QStringLiteral("currentPage"), ui_->currentPage());
        return AppToolResult::success(d);
      }});

  tools_.push_back(AppTool{
      QStringLiteral("select_object"),
      QStringLiteral("Select one object in the UI (drives gizmos, object list "
                     "and side panels). Use -1 to clear the selection."),
      objSchema(
          {{QStringLiteral("index"), intProp(QStringLiteral("Object index; -1 clears selection"))}},
          {QStringLiteral("index")}),
      false,
      [this](const QJsonObject &args) -> AppToolResult {
        const int index = args.value(QStringLiteral("index")).toInt(-2);
        if (index == -2)
          return AppToolResult::failure(QStringLiteral("index is required"));
        if (index >= project_->modelCount())
          return AppToolResult::failure(QStringLiteral("index out of range"));
        if (index >= 0 && !editor_->selectSourceObject(index))
          return AppToolResult::failure(QStringLiteral("selection rejected"));
        else if (index < 0)
          editor_->selectObject(-1);  // filtered-index -1 == nothing selected
        QJsonObject d;
        d.insert(QStringLiteral("selectedSourceIndex"),
                 editor_->selectedSourceObjectIndex());
        return AppToolResult::success(d);
      }});

  tools_.push_back(AppTool{
      QStringLiteral("toggle_sidebar"),
      QStringLiteral("Collapse or expand the left settings sidebar."),
      objSchema({}), false,
      [this](const QJsonObject &) -> AppToolResult {
        if (!ui_)
          return AppToolResult::failure(QStringLiteral("UI control unavailable (headless)"));
        ui_->toggleSidebar();
        return AppToolResult::success();
      }});

  // ── Global actions ────────────────────────────────────────────────────────
  tools_.push_back(AppTool{
      QStringLiteral("undo"), QStringLiteral("Undo the last action."),
      objSchema({}), false,
      [this](const QJsonObject &) -> AppToolResult {
        editor_->undo();
        return AppToolResult::success();
      }});

  tools_.push_back(AppTool{
      QStringLiteral("redo"), QStringLiteral("Redo the last undone action."),
      objSchema({}), false,
      [this](const QJsonObject &) -> AppToolResult {
        editor_->redo();
        return AppToolResult::success();
      }});

  tools_.push_back(AppTool{
      QStringLiteral("save_project"),
      QStringLiteral("Save the project as .3mf. Without a path, reuses the "
                     "current project path (fails if never saved)."),
      objSchema({
          {QStringLiteral("path"), stringProp(QStringLiteral("Destination .3mf path (optional)"))},
      }),
      false,
      [this](const QJsonObject &args) -> AppToolResult {
        QString path = args.value(QStringLiteral("path")).toString();
        if (path.isEmpty())
          path = project_->currentProjectPath();
        if (path.isEmpty())
          return AppToolResult::failure(
              QStringLiteral("No path given and the project has never been saved"));
        if (!project_->saveProjectAs(path))
          return AppToolResult::failure(QStringLiteral("save failed"));
        QJsonObject d;
        d.insert(QStringLiteral("saved"), true);
        d.insert(QStringLiteral("path"), path);
        return AppToolResult::success(d);
      }});

  tools_.push_back(AppTool{
      QStringLiteral("clear_project"),
      QStringLiteral("Clear the whole workspace: removes all objects and plate "
                     "state (undo stack is also reset — NOT undoable)."),
      objSchema({}), true,
      [this](const QJsonObject &) -> AppToolResult {
        editor_->clearWorkspace();
        return AppToolResult::success();
      }});
}

}  // namespace OWzx
