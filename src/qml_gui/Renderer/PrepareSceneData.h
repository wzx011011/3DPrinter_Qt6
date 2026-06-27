#pragma once

#include <QByteArray>
#include <QList>
#include <QtGlobal>

class PrepareSceneData
{
public:
  struct Vertex
  {
    float x = 0.0f;
    float y = 0.0f;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    bool operator==(const Vertex &other) const;
  };

  struct ModelVertex
  {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
  };

  struct ModelBounds
  {
    float minX = 0.0f;
    float minY = 0.0f;
    float minZ = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
    float maxZ = 0.0f;
  };

  struct ModelBatch
  {
    int renderObjectId = -1;
    int sourceObjectIndex = -1;
    int firstVertex = 0;
    int vertexCount = 0;
    ModelBounds bounds;
  };

  enum DirtyFlag : quint32
  {
    DirtyNone = 0,
    DirtyBed = 1u << 0,
    DirtyPlate = 1u << 1,
    DirtyMesh = 1u << 2,
    DirtyVisibility = 1u << 3,
    DirtyGpu = 1u << 4,
    DirtySelection = 1u << 5,
    DirtyCamera = 1u << 6
  };

  PrepareSceneData();

  void setBed(float widthMm,
              float depthMm,
              float originX,
              float originY,
              int shapeType,
              float diameterMm);
  void setShowBed(bool showBed);
  void setPlateContext(int currentPlateIndex, int plateCount, const QList<int> &activeObjectIndices);
  void setMeshGeneration(qint64 generation);
  void setModelMeshData(const QByteArray &meshData,
                        const QList<int> &batchSourceObjectIndices,
                        const QList<int> &activeSourceObjectIndices);
  void setSelectedSourceObjectIndex(int sourceObjectIndex);
  void setHoveredSourceObjectIndex(int sourceObjectIndex);

  quint32 peekDirtyFlags() const;
  quint32 takeDirtyFlags();
  void clearDirtyFlags();

  float bedWidth() const;
  float bedDepth() const;
  float bedOriginX() const;
  float bedOriginY() const;
  int bedShapeType() const;
  float bedDiameter() const;
  bool showBed() const;

  float fineGridSpacingMm() const;
  float coarseGridSpacingMm() const;

  int currentPlateIndex() const;
  int plateCount() const;
  const QList<int> &activeObjectIndices() const;
  qint64 meshGeneration() const;

  const QList<Vertex> &bedFillVertices() const;
  const QList<Vertex> &bedLineVertices() const;
  const QList<ModelVertex> &modelVertices() const;
  const QList<ModelBatch> &modelBatches() const;
  const ModelBounds &modelBounds() const;
  bool hasModelBounds() const;
  int selectedSourceObjectIndex() const;
  int hoveredSourceObjectIndex() const;

private:
  static float sanitizeExtent(float value, float fallback);
  static bool nearlyEqual(float left, float right);
  static bool sameObjectIndices(const QList<int> &left, const QList<int> &right);

  void markDirty(quint32 flags);
  void rebuildBedGeometry();
  void clearModelGeometry();
  void updateModelBounds(const ModelVertex &vertex);
  static bool activeSourceContains(const QList<int> &activeSourceObjectIndices, int sourceObjectIndex);
  static quint32 colorForSourceObject(int sourceObjectIndex, float &r, float &g, float &b);
  void appendLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a);
  void appendRectFill(float left, float top, float right, float bottom);
  void appendRectBorder(float left, float top, float right, float bottom);

  float m_bedWidth = 220.0f;
  float m_bedDepth = 220.0f;
  float m_bedOriginX = 0.0f;
  float m_bedOriginY = 0.0f;
  int m_bedShapeType = 0;
  float m_bedDiameter = 220.0f;
  bool m_showBed = true;

  int m_currentPlateIndex = 0;
  int m_plateCount = 1;
  QList<int> m_activeObjectIndices;
  qint64 m_meshGeneration = 0;

  QList<Vertex> m_bedFillVertices;
  QList<Vertex> m_bedLineVertices;
  QList<ModelVertex> m_modelVertices;
  QList<ModelBatch> m_modelBatches;
  ModelBounds m_modelBounds;
  bool m_hasModelBounds = false;
  int m_selectedSourceObjectIndex = -1;
  int m_hoveredSourceObjectIndex = -1;
  quint32 m_dirtyFlags = DirtyNone;
};
