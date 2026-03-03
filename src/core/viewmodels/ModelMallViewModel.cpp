#include "ModelMallViewModel.h"

ModelMallViewModel::ModelMallViewModel(QObject *parent) : QObject(parent)
{
  loadMockModels();
}

void ModelMallViewModel::loadMockModels()
{
  m_modelEntries.clear();
  const QStringList names = {
      "哥特式城堡", "机械臂", "花盆架", "桌面笔架", "齿轮装饰",
      "龙珠摆件", "电话支架", "迷你花瓶", "螺旋灯罩", "工具盒",
      "关节木偶", "航空发动机模型", "扭曲花瓶", "文件夹整理架"};
  const QStringList authors = {
      "CrealityDesign", "Maker3D", "PrintHub", "FDM_Pro", "QuickPrint",
      "ArtisanPrint", "TechModels", "HomeHero"};

  for (int i = 0; i < names.size(); ++i)
  {
    m_modelEntries.append({names[i], authors[i % authors.size()], 50 + i * 43, i % 4 == 0, 9.9 + i * 1.5});
  }
}

QVariantList ModelMallViewModel::models() const
{
  QVariantList result;
  result.reserve(m_modelEntries.size());
  for (const auto &e : m_modelEntries)
  {
    result.append(QVariantMap{{"name", e.name}, {"author", e.author}, {"likes", e.likes}, {"free", e.free}, {"price", e.price}});
  }
  return result;
}

int ModelMallViewModel::modelCount() const { return m_modelEntries.size(); }
QString ModelMallViewModel::modelName(int i) const { return (i >= 0 && i < m_modelEntries.size()) ? m_modelEntries[i].name : QString{}; }
QString ModelMallViewModel::modelAuthor(int i) const { return (i >= 0 && i < m_modelEntries.size()) ? m_modelEntries[i].author : QString{}; }
int ModelMallViewModel::modelLikes(int i) const { return (i >= 0 && i < m_modelEntries.size()) ? m_modelEntries[i].likes : 0; }
bool ModelMallViewModel::modelFree(int i) const { return (i >= 0 && i < m_modelEntries.size()) ? m_modelEntries[i].free : false; }
double ModelMallViewModel::modelPrice(int i) const { return (i >= 0 && i < m_modelEntries.size()) ? m_modelEntries[i].price : 0.0; }

void ModelMallViewModel::setCategoryIndex(int idx)
{
  if (m_categoryIndex != idx)
  {
    m_categoryIndex = idx;
    emit categoryIndexChanged();
    loadMockModels();
    emit modelsChanged();
  }
}

void ModelMallViewModel::setSearchQuery(const QString &q)
{
  if (m_searchQuery != q)
  {
    m_searchQuery = q;
    emit searchQueryChanged();
  }
}

void ModelMallViewModel::downloadModel(int index) { Q_UNUSED(index) }
void ModelMallViewModel::refresh()
{
  loadMockModels();
  emit modelsChanged();
}
