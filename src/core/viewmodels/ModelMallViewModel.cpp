#include "ModelMallViewModel.h"

#include <algorithm>
#include <QTimer>

ModelMallViewModel::ModelMallViewModel(QObject *parent) : QObject(parent)
{
  // Categories aligned with upstream CrealityPrint model mall taxonomy
  m_categories = {
      QT_TRANSLATE_NOOP("ModelMallViewModel", "All"),
      QT_TRANSLATE_NOOP("ModelMallViewModel", "Toys & Games"),
      QT_TRANSLATE_NOOP("ModelMallViewModel", "Home & Garden"),
      QT_TRANSLATE_NOOP("ModelMallViewModel", "Tools & Utility"),
      QT_TRANSLATE_NOOP("ModelMallViewModel", "Art & Fashion"),
      QT_TRANSLATE_NOOP("ModelMallViewModel", "Education")
  };

  loadMockModels();

  // Initialize navigation history with the default state (aligns with upstream browser history)
  pushNavigationState();
  updateNavigationTitle();
}

void ModelMallViewModel::loadMockModels()
{
  m_allEntries.clear();

  // 12 mock models across 4 categories (indices 1-4), matching upstream ModelMallDialog scope
  // Category 0 (All) shows everything (aligns with upstream default view)
  // Upstream loads a web-based mall; here we provide native QML mock for offline preview
  struct RawModel {
    const char *name, *author, *thumbColor, *thumbIcon;
    const char *description;
    const char *fileFormat;
    int fileSizeKB;
    const char *printTime;
    const char *materialUsage;
    const char *tags;
    int downloads, categoryIndex;
    double rating;
    bool free, featured;
    double price;
  };

  const RawModel raw[] = {
      // Toys & Games (cat 1)
      {"Articulated Dragon",    "CrealityDesign", "#1a3a5c", "\xf0\x9f\x90\x89",
       "Fully articulated dragon with print-in-place joints. No assembly required. Designed for 0.2mm layer height with 15% infill.",
       "STL", 4520, "6h 30m", "~120g PLA", "articulated,dragon,print-in-place,fun",
       12580, 1, 4.8, true,  true,  0.0},
      {"Mechanical Puzzle Box",  "Maker3D",        "#3a1a5c", "\xf0\x9f\x94\x92",
       "Intricate mechanical puzzle box with 6-step locking mechanism. Tolerances tuned for FDM printing.",
       "STL, 3MF", 2830, "4h 15m", "~85g PLA", "puzzle,mechanical,box,brain-teaser",
       8340, 1, 4.6, false, false, 12.9},
      {"Chess Set Gothic",       "PrintHub",       "#1a5c3a", "\xf0\x9f\x8f\xb0",
       "Complete gothic-themed chess set with 32 pieces. Each piece features detailed gothic architecture elements.",
       "STL", 8940, "12h 00m", "~340g PLA", "chess,gothic,board-game,decorative",
       6210, 1, 4.7, false, true,  9.9},

      // Home & Garden (cat 2)
      {"Self-Watering Planter",  "FDM_Pro",        "#1a5c5c", "\xf0\x9f\x8c\xb1",
       "Self-watering planter with wicking system. Holds 200ml water reservoir. Fits standard 6cm pots.",
       "STL", 1540, "2h 45m", "~45g PETG", "planter,self-watering,garden,home",
       9870, 2, 4.5, true,  true,  0.0},
      {"Desk Organizer Set",     "QuickPrint",     "#5c4a1a", "\xf0\x9f\x97\x82",
       "5-piece modular desk organizer: pen holder, phone stand, card slot, sticky note tray, and cable pass.",
       "3MF", 3210, "5h 10m", "~150g PLA", "organizer,desk,modular,office",
       7650, 2, 4.3, false, false,  6.9},
      {"Phone Stand Adjustable", "HomeHero",       "#5c1a2a", "\xf0\x9f\x93\xb1",
       "Adjustable phone stand with 15-80 degree tilt. Compatible with all smartphones up to 8 inches.",
       "STL", 890, "1h 30m", "~22g PLA", "phone-stand,adjustable,desktop,universal",
       15200, 2, 4.9, true,  true,  0.0},

      // Tools & Utility (cat 3)
      {"Hex Bit Organizer",      "TechModels",     "#2a2a3c", "\xf0\x9f\x94\xa7",
       "Magnetic hex bit organizer for standard 1/4\" bits. Holds 30 bits with labeled slots.",
       "STL", 1100, "1h 50m", "~35g PETG", "hex-bit,organizer,tool,magnetic",
       5890, 3, 4.4, true,  false,  0.0},
      {"Cable Management Clip",  "ArtisanPrint",   "#3c2a2a", "\xf0\x9f\x94\x8c",
       "Set of 6 adhesive-mount cable clips for 3-8mm cables. Prints flat, snaps together.",
       "STL", 420, "0h 45m", "~12g PLA", "cable,clip,management,organizer",
       4320, 3, 4.2, true,  false,  0.0},
      {"3D Print Calibration Cube","CrealityDesign","#3c3c2a", "\xe2\xac\xa1",
       "Standard 20mm calibration cube for testing dimensional accuracy, bridging, and retractions.",
       "STL", 58, "0h 20m", "~4g PLA", "calibration,cube,test,benchmark",
       21300, 3, 4.9, true,  true,  0.0},

      // Art & Fashion (cat 4)
      {"Geometric Vase",         "ArtisanPrint",   "#5c1a5c", "\xf0\x9f\x92\x90",
       "Modern geometric vase with Voronoi pattern. Double-wall design for strength. Watertight with PETG.",
       "STL, OBJ", 2780, "3h 40m", "~95g PETG", "vase,geometric,voronoi,decorative",
       7840, 4, 4.6, false, true, 15.9},
      {"Flexible Bracelet",      "FashionPrint",   "#5c3a1a", "\xf0\x9f\x92\x8e",
       "Flexible hinge bracelet with geometric pattern. Print-in-place, no supports needed. Fits most wrist sizes.",
       "STL", 680, "1h 10m", "~15g TPU", "bracelet,flexible,wearable,TPU",
       3460, 4, 4.1, false, false,  8.9},

      // Education (cat 5)
      {"DNA Double Helix",       "EduModels",      "#1a2a5c", "\xf0\x9f\xa7\xac",
       "Accurate DNA double helix model showing base pairs. 200mm tall with color-coded nucleotides.",
       "STL, 3MF", 5200, "8h 00m", "~200g PLA", "DNA,helix,biology,education",
       4560, 5, 4.7, true,  false,  0.0},
  };

  for (const auto &r : raw) {
    m_allEntries.append({
        QString::fromUtf8(r.name), QString::fromUtf8(r.author),
        QString::fromUtf8(r.thumbColor), QString::fromUtf8(r.thumbIcon),
        QString::fromUtf8(r.description), QString::fromUtf8(r.fileFormat),
        r.fileSizeKB, QString::fromUtf8(r.printTime), QString::fromUtf8(r.materialUsage),
        QString::fromUtf8(r.tags),
        r.downloads, r.categoryIndex, r.rating, r.free, r.featured, r.price
    });
  }

  applyFilter();
}

void ModelMallViewModel::applyFilter()
{
  m_filteredIndices.clear();

  for (int i = 0; i < m_allEntries.size(); ++i) {
    const auto &e = m_allEntries[i];

    // Category filter: index 0 = all (aligns with upstream default view)
    if (m_categoryIndex > 0 && e.categoryIndex != m_categoryIndex)
      continue;

    // Search filter: match name, author, or tags (aligns with upstream search behavior)
    if (!m_searchQuery.isEmpty()) {
      const QString q = m_searchQuery.toLower();
      if (!e.name.toLower().contains(q)
          && !e.author.toLower().contains(q)
          && !e.tags.toLower().contains(q))
        continue;
    }

    m_filteredIndices.append(i);
  }

  // Sort based on sort mode (aligned with upstream tabs: Recommended/Popular/Newest/Free)
  std::sort(m_filteredIndices.begin(), m_filteredIndices.end(), [this](int a, int b) {
    const auto &ea = m_allEntries[a];
    const auto &eb = m_allEntries[b];
    switch (m_sortMode) {
      case SORT_POPULAR: return ea.downloads > eb.downloads;
      case SORT_FREE:    return ea.free > eb.free; // free first
      case SORT_NEWEST:  return a > b;             // mock: later entry = newer
      case SORT_RECOMMENDED:
      default:           return ea.featured > eb.featured || (ea.featured == eb.featured && ea.rating > eb.rating);
    }
  });

  emit filteredCountChanged();
}

// --- Navigation history (aligns with upstream on_back/on_forward browser history) ---

void ModelMallViewModel::pushNavigationState()
{
  NavigationState state;
  state.categoryIndex = m_categoryIndex;
  state.sortMode = m_sortMode;
  state.searchQuery = m_searchQuery;

  // If we navigated back and then changed something, discard forward history
  if (m_historyIndex >= 0 && m_historyIndex < m_navigationHistory.size() - 1) {
    m_navigationHistory = m_navigationHistory.mid(0, m_historyIndex + 1);
  }

  // Cap history to prevent unbounded growth (keep last 50 entries)
  static const int MAX_HISTORY = 50;
  if (m_navigationHistory.size() >= MAX_HISTORY) {
    m_navigationHistory.removeFirst();
  }

  m_navigationHistory.append(state);
  m_historyIndex = m_navigationHistory.size() - 1;

  emit canGoBackChanged();
  emit canGoForwardChanged();
  updateNavigationTitle();
}

void ModelMallViewModel::restoreNavigationState(const NavigationState &state)
{
  m_categoryIndex = state.categoryIndex;
  m_sortMode = state.sortMode;
  m_searchQuery = state.searchQuery;
  emit categoryIndexChanged();
  emit sortModeChanged();
  emit searchQueryChanged();
  applyFilter();
  updateNavigationTitle();
}

void ModelMallViewModel::goBack()
{
  // Aligns with upstream ModelMallDialog::on_back -> m_browser->GoBack()
  if (m_historyIndex > 0) {
    m_historyIndex--;
    restoreNavigationState(m_navigationHistory[m_historyIndex]);
    emit canGoBackChanged();
    emit canGoForwardChanged();
  }
}

void ModelMallViewModel::goForward()
{
  // Aligns with upstream ModelMallDialog::on_forward -> m_browser->GoForward()
  if (m_historyIndex < m_navigationHistory.size() - 1) {
    m_historyIndex++;
    restoreNavigationState(m_navigationHistory[m_historyIndex]);
    emit canGoBackChanged();
    emit canGoForwardChanged();
  }
}

void ModelMallViewModel::updateNavigationTitle()
{
  // Build a navigation title showing current category + search (aligns with upstream URL bar concept)
  QString title;
  if (m_publishMode) {
    title = tr("Publish");
  } else {
    title = tr("3D Model Mall");
  }

  if (m_categoryIndex > 0 && m_categoryIndex < m_categories.size()) {
    title += " / " + m_categories[m_categoryIndex];
  }

  if (!m_searchQuery.isEmpty()) {
    title += " / \"" + m_searchQuery + "\"";
  }

  if (m_navigationTitle != title) {
    m_navigationTitle = title;
    emit navigationTitleChanged();
  }
}

// --- Control panel visibility (aligns with upstream show_control(bool)) ---

void ModelMallViewModel::setControlPanelVisible(bool visible)
{
  if (m_controlPanelVisible != visible) {
    m_controlPanelVisible = visible;
    emit controlPanelVisibleChanged();
  }
}

// --- Q_PROPERTY accessors ---

QStringList ModelMallViewModel::categories() const { return m_categories; }

// --- Q_INVOKABLE accessors (filtered view) ---

int ModelMallViewModel::filteredCount() const { return m_filteredIndices.size(); }

QString ModelMallViewModel::modelName(int i) const {
  return (i >= 0 && i < m_filteredIndices.size()) ? m_allEntries[m_filteredIndices[i]].name : QString{};
}
QString ModelMallViewModel::modelAuthor(int i) const {
  return (i >= 0 && i < m_filteredIndices.size()) ? m_allEntries[m_filteredIndices[i]].author : QString{};
}
int ModelMallViewModel::modelDownloads(int i) const {
  return (i >= 0 && i < m_filteredIndices.size()) ? m_allEntries[m_filteredIndices[i]].downloads : 0;
}
double ModelMallViewModel::modelRating(int i) const {
  return (i >= 0 && i < m_filteredIndices.size()) ? m_allEntries[m_filteredIndices[i]].rating : 0.0;
}
bool ModelMallViewModel::modelFree(int i) const {
  return (i >= 0 && i < m_filteredIndices.size()) ? m_allEntries[m_filteredIndices[i]].free : false;
}
double ModelMallViewModel::modelPrice(int i) const {
  return (i >= 0 && i < m_filteredIndices.size()) ? m_allEntries[m_filteredIndices[i]].price : 0.0;
}
int ModelMallViewModel::modelCategoryIndex(int i) const {
  return (i >= 0 && i < m_filteredIndices.size()) ? m_allEntries[m_filteredIndices[i]].categoryIndex : 0;
}
QString ModelMallViewModel::modelThumbnailColor(int i) const {
  return (i >= 0 && i < m_filteredIndices.size()) ? m_allEntries[m_filteredIndices[i]].thumbnailColor : QString{};
}
QString ModelMallViewModel::modelThumbnailIcon(int i) const {
  return (i >= 0 && i < m_filteredIndices.size()) ? m_allEntries[m_filteredIndices[i]].thumbnailIcon : QString{};
}
bool ModelMallViewModel::modelFeatured(int i) const {
  return (i >= 0 && i < m_filteredIndices.size()) ? m_allEntries[m_filteredIndices[i]].featured : false;
}
QString ModelMallViewModel::modelDescription(int i) const {
  return (i >= 0 && i < m_filteredIndices.size()) ? m_allEntries[m_filteredIndices[i]].description : QString{};
}
QString ModelMallViewModel::modelFileFormat(int i) const {
  return (i >= 0 && i < m_filteredIndices.size()) ? m_allEntries[m_filteredIndices[i]].fileFormat : QString{};
}
int ModelMallViewModel::modelFileSizeKB(int i) const {
  return (i >= 0 && i < m_filteredIndices.size()) ? m_allEntries[m_filteredIndices[i]].fileSizeKB : 0;
}
QString ModelMallViewModel::modelPrintTime(int i) const {
  return (i >= 0 && i < m_filteredIndices.size()) ? m_allEntries[m_filteredIndices[i]].printTime : QString{};
}
QString ModelMallViewModel::modelMaterialUsage(int i) const {
  return (i >= 0 && i < m_filteredIndices.size()) ? m_allEntries[m_filteredIndices[i]].materialUsage : QString{};
}
QString ModelMallViewModel::modelTags(int i) const {
  return (i >= 0 && i < m_filteredIndices.size()) ? m_allEntries[m_filteredIndices[i]].tags : QString{};
}

// --- Download progress (aligns with upstream ModelMallDialog download interaction) ---

int ModelMallViewModel::downloadProgress(int index) const
{
  if (index < 0 || index >= m_filteredIndices.size()) return 0;
  int realIdx = m_filteredIndices[index];
  if (realIdx < 0 || realIdx >= m_allEntries.size()) return 0;
  return m_allEntries[realIdx].downloadProgress;
}

bool ModelMallViewModel::downloadCompleted(int index) const
{
  if (index < 0 || index >= m_filteredIndices.size()) return false;
  int realIdx = m_filteredIndices[index];
  if (realIdx < 0 || realIdx >= m_allEntries.size()) return false;
  return m_allEntries[realIdx].downloadCompleted;
}

void ModelMallViewModel::cancelDownload(int index)
{
  if (index < 0 || index >= m_filteredIndices.size()) return;
  int realIdx = m_filteredIndices[index];
  if (realIdx < 0 || realIdx >= m_allEntries.size()) return;
  m_allEntries[realIdx].downloading = false;
  m_allEntries[realIdx].downloadProgress = 0;
  emit filteredCountChanged();
}

// --- Slots ---

void ModelMallViewModel::setCategoryIndex(int idx)
{
  if (m_categoryIndex != idx) {
    m_categoryIndex = idx;
    emit categoryIndexChanged();
    applyFilter();
    pushNavigationState();
  }
}

void ModelMallViewModel::setSearchQuery(const QString &q)
{
  if (m_searchQuery != q) {
    m_searchQuery = q;
    emit searchQueryChanged();
    applyFilter();
    pushNavigationState();
  }
}

void ModelMallViewModel::setSortMode(int mode)
{
  if (m_sortMode != mode) {
    m_sortMode = mode;
    emit sortModeChanged();
    applyFilter();
    pushNavigationState();
  }
}

void ModelMallViewModel::downloadModel(int index)
{
  // Aligns with upstream ModelMallDialog download via script messages
  if (index < 0 || index >= m_filteredIndices.size()) return;
  int realIdx = m_filteredIndices[index];
  if (realIdx < 0 || realIdx >= m_allEntries.size()) return;

  auto &entry = m_allEntries[realIdx];
  if (entry.downloading || entry.downloadCompleted) return;

  entry.downloading = true;
  entry.downloadProgress = 0;
  entry.downloadCompleted = false;
  emit filteredCountChanged();

  // Mock: simulate progressive download with data-driven steps (aligns with upstream download progress)
  static const int stepDelays[] = {200, 400, 600, 900, 1200, 1500, 1800, 2000};
  static const int stepValues[] = {10, 25, 40, 55, 70, 85, 95, 100};

  for (int s = 0; s < 8; ++s) {
    QTimer::singleShot(stepDelays[s], this, [this, realIdx, progress = stepValues[s]]() {
      if (realIdx < m_allEntries.size() && m_allEntries[realIdx].downloading) {
        if (progress >= 100) {
          m_allEntries[realIdx].downloading = false;
          m_allEntries[realIdx].downloadProgress = 100;
          m_allEntries[realIdx].downloadCompleted = true;
          m_allEntries[realIdx].downloads++;
        } else {
          m_allEntries[realIdx].downloadProgress = progress;
        }
        emit filteredCountChanged();
      }
    });
  }
}

bool ModelMallViewModel::isDownloading(int index) const
{
  if (index < 0 || index >= m_filteredIndices.size()) return false;
  int realIdx = m_filteredIndices[index];
  if (realIdx < 0 || realIdx >= m_allEntries.size()) return false;
  return m_allEntries[realIdx].downloading;
}

void ModelMallViewModel::openModelDetail(int index)
{
  // Upstream alignment: WebViewPanel::OpenModelDetail() opens browser with makerhub URL
  // Mock: show native QML detail dialog with expanded model information
  if (index >= 0 && index < m_filteredIndices.size()) {
    m_selectedModelIndex = index;
    m_detailDialogOpen = true;
    emit selectedModelIndexChanged();
    emit detailDialogOpenChanged();
  }
}

void ModelMallViewModel::closeDetailDialog()
{
  m_detailDialogOpen = false;
  emit detailDialogOpenChanged();
}

void ModelMallViewModel::refresh()
{
  m_isLoading = true;
  emit isLoadingChanged();
  // Simulate async loading delay (aligns with upstream wxWebView reload)
  QTimer::singleShot(800, this, [this]() {
    loadMockModels();
    m_isLoading = false;
    emit isLoadingChanged();
  });
}

void ModelMallViewModel::loadMallUrl(const QString &url)
{
  // Upstream alignment: ModelMallDialog::go_to_mall(url) -> WebView::LoadUrl(m_browser, url)
  Q_UNUSED(url)
  m_publishMode = false;
  emit publishModeChanged();
  updateNavigationTitle();
}

void ModelMallViewModel::loadPublishUrl(const QString &url)
{
  // Upstream alignment: ModelMallDialog::go_to_publish(url) -> WebView::LoadUrl(m_browser, url)
  Q_UNUSED(url)
  m_publishMode = true;
  emit publishModeChanged();
  updateNavigationTitle();
}
