#pragma once

// AppToolRegistry — OWzx-only AI control surface (decision record:
// docs/ai-control.md).
//
// Exposes the application's user-visible AND internal operations as
// JSON-schema'd tools so an AI agent (embedded Claude Agent SDK sidecar over
// MCP, or any other MCP client) can drive the whole slicer: scene objects,
// presets/config, slicing, UI navigation, undo/redo.
//
// Design constraints:
//   - Pure C++ layer under src/core/ai; NO qml_gui headers here. UI actions go
//     through the AppToolUiProvider interface, implemented by the composition
//     root (BackendContext). Headless tests pass nullptr and UI tools report
//     unavailable.
//   - All execute() handlers MUST run on the GUI thread (they touch QObject
//     services and models). Transports (MCP server) dispatch accordingly.
//   - Every tool is a thin wrapper over an EXISTING ViewModel/Service API —
//     no parallel business logic.

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

#include <functional>
#include <vector>

class ProjectServiceMock;
class SliceService;
class ConfigViewModel;
class EditorViewModel;

namespace OWzx {

/// UI actions the registry cannot reach from src/core. Implemented by the
/// composition root; null in headless tests.
class AppToolUiProvider {
 public:
  virtual ~AppToolUiProvider() = default;
  virtual bool switchPage(int position) = 0;
  virtual bool toggleSidebar() = 0;
  virtual int currentPage() const = 0;
};

struct AppToolResult {
  bool ok = true;
  QJsonObject data;
  QString error;

  static AppToolResult success(QJsonObject data = {}) {
    AppToolResult r;
    r.data = std::move(data);
    return r;
  }
  static AppToolResult failure(const QString &error) {
    AppToolResult r;
    r.ok = false;
    r.error = error;
    return r;
  }

  QJsonObject toJson() const {
    QJsonObject out = data;
    out.insert(QStringLiteral("ok"), ok);
    if (!ok)
      out.insert(QStringLiteral("error"), error);
    return out;
  }
};

struct AppTool {
  QString name;
  QString description;
  QJsonObject inputSchema;  // JSON Schema "object" describing arguments.
  bool destructive = false; // transport shows a confirmation card first
  std::function<AppToolResult(const QJsonObject &args)> execute;
};

class AppToolRegistry final {
 public:
  AppToolRegistry(ProjectServiceMock *project, SliceService *slice,
                  ConfigViewModel *config, EditorViewModel *editor,
                  AppToolUiProvider *ui = nullptr);

  /// MCP tools/list payload: [{name, description, inputSchema}].
  QJsonArray toolDefinitions() const;

  /// Runs one tool by name. GUI thread only. Unknown name / bad args -> failure.
  AppToolResult execute(const QString &name, const QJsonObject &args);

  /// True when a tool with this name is registered.
  bool hasTool(const QString &name) const;

  const std::vector<AppTool> &tools() const { return tools_; }

 private:
  void buildTools();

  ProjectServiceMock *project_ = nullptr;
  SliceService *slice_ = nullptr;
  ConfigViewModel *config_ = nullptr;
  EditorViewModel *editor_ = nullptr;
  AppToolUiProvider *ui_ = nullptr;
  std::vector<AppTool> tools_;
};

}  // namespace OWzx
