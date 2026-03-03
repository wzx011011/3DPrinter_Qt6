#pragma once
#include <QObject>
#include <QVariantList>

class ModelMallViewModel : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QVariantList models READ models NOTIFY modelsChanged)
  Q_PROPERTY(int categoryIndex READ categoryIndex WRITE setCategoryIndex NOTIFY categoryIndexChanged)
  Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)

public:
  explicit ModelMallViewModel(QObject *parent = nullptr);

  QVariantList models() const;

  // Individual item accessors - use these from QML to avoid Qt6 V4 VariantAssociationObject crash
  Q_INVOKABLE int modelCount() const;
  Q_INVOKABLE QString modelName(int i) const;
  Q_INVOKABLE QString modelAuthor(int i) const;
  Q_INVOKABLE int modelLikes(int i) const;
  Q_INVOKABLE bool modelFree(int i) const;
  Q_INVOKABLE double modelPrice(int i) const;

  int categoryIndex() const { return m_categoryIndex; }
  QString searchQuery() const { return m_searchQuery; }

signals:
  void modelsChanged();
  void categoryIndexChanged();
  void searchQueryChanged();

public slots:
  void setCategoryIndex(int idx);
  void setSearchQuery(const QString &q);
  void downloadModel(int index);
  void refresh();

private:
  void loadMockModels();
  // Store as structs - not QVariantList member to prevent V4 GC destructor crash
  struct ModelEntry
  {
    QString name, author;
    int likes;
    bool free;
    double price;
  };
  QList<ModelEntry> m_modelEntries;
  int m_categoryIndex = 0;
  QString m_searchQuery;
};
