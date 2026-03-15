#pragma once
#include <QObject>
#include <QVariantList>
#include <QStringList>


class ModelMallViewModel : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QStringList categories READ categories CONSTANT)
  Q_PROPERTY(int categoryIndex READ categoryIndex WRITE setCategoryIndex NOTIFY categoryIndexChanged)
  Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
  Q_PROPERTY(int sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
  Q_PROPERTY(int filteredCount READ filteredCount NOTIFY filteredCountChanged)
  Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
  Q_PROPERTY(bool webViewAvailable READ webViewAvailable CONSTANT)
  Q_PROPERTY(int selectedModelIndex READ selectedModelIndex NOTIFY selectedModelIndexChanged)
  Q_PROPERTY(bool detailDialogOpen READ detailDialogOpen NOTIFY detailDialogOpenChanged)

  // Navigation history (aligns with upstream on_back/on_forward browser history)
  Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY canGoBackChanged)
  Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY canGoForwardChanged)
  Q_PROPERTY(QString navigationTitle READ navigationTitle NOTIFY navigationTitleChanged)

  // Control panel visibility (aligns with upstream show_control(bool))
  Q_PROPERTY(bool controlPanelVisible READ controlPanelVisible WRITE setControlPanelVisible NOTIFY controlPanelVisibleChanged)

  // Publish mode (aligns with upstream go_to_publish vs go_to_mall)
  Q_PROPERTY(bool publishMode READ publishMode NOTIFY publishModeChanged)

public:
  explicit ModelMallViewModel(QObject *parent = nullptr);

  QStringList categories() const;

  // Individual filtered-item accessors (Q_INVOKABLE to avoid Qt6 V4 VariantAssociationObject crash)
  Q_INVOKABLE int filteredCount() const;
  Q_INVOKABLE QString modelName(int i) const;
  Q_INVOKABLE QString modelAuthor(int i) const;
  Q_INVOKABLE int modelDownloads(int i) const;
  Q_INVOKABLE double modelRating(int i) const;
  Q_INVOKABLE bool modelFree(int i) const;
  Q_INVOKABLE double modelPrice(int i) const;
  Q_INVOKABLE int modelCategoryIndex(int i) const;
  Q_INVOKABLE QString modelThumbnailColor(int i) const;
  Q_INVOKABLE QString modelThumbnailIcon(int i) const;
  Q_INVOKABLE bool modelFeatured(int i) const;
  Q_INVOKABLE QString modelDescription(int i) const;
  Q_INVOKABLE QString modelFileFormat(int i) const;
  Q_INVOKABLE int modelFileSizeKB(int i) const;
  Q_INVOKABLE QString modelPrintTime(int i) const;
  Q_INVOKABLE QString modelMaterialUsage(int i) const;
  Q_INVOKABLE QString modelTags(int i) const;

  // Download progress (aligns with upstream download interaction)
  Q_INVOKABLE int downloadProgress(int index) const;
  Q_INVOKABLE bool downloadCompleted(int index) const;
  Q_INVOKABLE void cancelDownload(int index);

  int selectedModelIndex() const { return m_selectedModelIndex; }
  bool detailDialogOpen() const { return m_detailDialogOpen; }

  int categoryIndex() const { return m_categoryIndex; }
  QString searchQuery() const { return m_searchQuery; }
  int sortMode() const { return m_sortMode; }
  bool isLoading() const { return m_isLoading; }
  bool webViewAvailable() const { return false; } // Placeholder: upstream uses wxWebView2

  // Navigation (aligns with upstream on_back/on_forward)
  bool canGoBack() const { return m_historyIndex > 0; }
  bool canGoForward() const { return m_historyIndex < m_navigationHistory.size() - 1; }
  QString navigationTitle() const { return m_navigationTitle; }

  // Control panel (aligns with upstream show_control(bool))
  bool controlPanelVisible() const { return m_controlPanelVisible; }

  // Publish mode (aligns with upstream go_to_publish)
  bool publishMode() const { return m_publishMode; }

signals:
  void filteredCountChanged();
  void categoryIndexChanged();
  void searchQueryChanged();
  void sortModeChanged();
  void isLoadingChanged();
  void selectedModelIndexChanged();
  void detailDialogOpenChanged();
  void canGoBackChanged();
  void canGoForwardChanged();
  void navigationTitleChanged();
  void controlPanelVisibleChanged();
  void publishModeChanged();

public slots:
  void setCategoryIndex(int idx);
  void setSearchQuery(const QString &q);
  void setSortMode(int mode);
  Q_INVOKABLE void downloadModel(int index);
  Q_INVOKABLE bool isDownloading(int index) const;
  void openModelDetail(int index);
  void closeDetailDialog();
  void refresh();

  // Navigation history (aligns with upstream on_back/on_forward)
  Q_INVOKABLE void goBack();
  Q_INVOKABLE void goForward();

  // Control panel (aligns with upstream show_control(bool))
  void setControlPanelVisible(bool visible);

  // Upstream alignment: go_to_mall / go_to_publish
  void loadMallUrl(const QString &url);
  void loadPublishUrl(const QString &url);

private:
  void loadMockModels();
  void pushNavigationState();
  void updateNavigationTitle();

public:
  // Store as structs - not QVariantList member to prevent V4 GC destructor crash
  struct ModelEntry
  {
    QString name, author, thumbnailColor, thumbnailIcon;
    QString description;
    QString fileFormat;
    int fileSizeKB;
    QString printTime;
    QString materialUsage;
    QString tags;
    int downloads;
    int categoryIndex; // index into m_categories
    double rating;
    bool free, featured;
    double price;
    bool downloading = false;
    int downloadProgress = 0;
    bool downloadCompleted = false;
  };

  // Navigation history state (aligns with upstream browser history back/forward)
  struct NavigationState {
    int categoryIndex;
    int sortMode;
    QString searchQuery;
  };

  // Sort modes matching upstream tabs: 0=Recommended, 1=Popular, 2=Newest, 3=Free
  static const int SORT_RECOMMENDED = 0;
  static const int SORT_POPULAR     = 1;
  static const int SORT_NEWEST      = 2;
  static const int SORT_FREE        = 3;

  QList<ModelEntry> m_allEntries;     // full mock catalog
  QList<int>        m_filteredIndices; // indices into m_allEntries after filtering
  QStringList        m_categories;
  int m_categoryIndex = 0; // 0 = all
  QString m_searchQuery;
  int m_sortMode = 0;
  bool m_isLoading = false;
  int m_selectedModelIndex = -1;
  bool m_detailDialogOpen = false;

  // Navigation history (aligns with upstream browser CanGoBack/CanGoForward)
  QList<NavigationState> m_navigationHistory;
  int m_historyIndex = -1;
  QString m_navigationTitle;

  // Control panel visibility (aligns with upstream show_control(bool))
  bool m_controlPanelVisible = true;

  // Publish mode (aligns with upstream go_to_publish)
  bool m_publishMode = false;

private:
  void applyFilter();
  void restoreNavigationState(const NavigationState &state);
};
