#include "GLViewportRenderer.h"
#include "GLViewport.h"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObjectFormat>
#include <QVector>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>
#include <cstring>
#include <cfloat>
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// Shaders
// ---------------------------------------------------------------------------
static const char *kVertSrc =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "uniform mat4 uMVP;\n"
    "void main(){ gl_Position = uMVP * vec4(aPos,1.0); }\n";

static const char *kFragSrc =
    "#version 330 core\n"
    "uniform vec4 uColor;\n"
    "out vec4 fragColor;\n"
    "void main(){ fragColor = uColor; }\n";

static const char *kMeshVertSrc =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "uniform mat4 uMVP;\n"
    "uniform vec3 uOffset;\n"
    "out vec3 vWorldPos;\n"
    "void main(){\n"
    "  vec3 p = aPos + uOffset;\n"
    "  vWorldPos = p;\n"
    "  gl_Position = uMVP * vec4(p,1.0);\n"
    "}\n";

static const char *kMeshFragSrc =
    "#version 330 core\n"
    "in  vec3 vWorldPos;\n"
    "out vec4 fragColor;\n"
    "uniform vec3  uBaseColor;\n"
    "uniform float uBrightness;\n"
    "void main(){\n"
    "  vec3 dx = dFdx(vWorldPos); vec3 dy = dFdy(vWorldPos);\n"
    "  vec3 n = normalize(cross(dx,dy));\n"
    "  float diff = clamp(dot(n,normalize(vec3(0.6,1.0,0.8))),0.0,1.0);\n"
    "  float l = (0.28 + 0.72*diff) * uBrightness;\n"
    "  fragColor = vec4(uBaseColor*l, 1.0);\n"
    "}\n";

// Gizmo shader: local verts shifted by uCenter and scaled by uGizmoScale
static const char *kGizmoVertSrc =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "uniform mat4  uMVP;\n"
    "uniform vec3  uGizmoCenter;\n"
    "uniform float uGizmoScale;\n"
    "void main(){\n"
    "  gl_Position = uMVP * vec4(uGizmoCenter + aPos*uGizmoScale, 1.0);\n"
    "}\n";

static const char *kGizmoFragSrc =
    "#version 330 core\n"
    "uniform vec4 uGizmoColor;\n"
    "out vec4 fragColor;\n"
    "void main(){ fragColor = uGizmoColor; }\n";

// ---------------------------------------------------------------------------
// Colour palette
// ---------------------------------------------------------------------------
static const QVector3D kObjColors[] = {
    QVector3D(0.949f,0.549f,0.220f),
    QVector3D(0.231f,0.553f,0.867f),
    QVector3D(0.298f,0.686f,0.455f),
    QVector3D(0.608f,0.349f,0.714f),
    QVector3D(0.906f,0.298f,0.235f),
    QVector3D(0.102f,0.737f,0.612f),
    QVector3D(0.945f,0.769f,0.059f),
    QVector3D(0.914f,0.118f,0.549f),
};
static constexpr int kNumColors = int(sizeof(kObjColors)/sizeof(kObjColors[0]));

// Gizmo axis colors (RGBA), index 0=X 1=Y 2=Z
static const QVector4D kAxisColors[3] = {
    QVector4D(0.90f,0.18f,0.18f,1.f),
    QVector4D(0.22f,0.80f,0.22f,1.f),
    QVector4D(0.18f,0.40f,0.95f,1.f),
};
static const QVector4D kAxisHighlight[3] = {
    QVector4D(1.00f,0.50f,0.50f,1.f),
    QVector4D(0.50f,1.00f,0.50f,1.f),
    QVector4D(0.50f,0.70f,1.00f,1.f),
};

// ---------------------------------------------------------------------------
// Platform geometry
// ---------------------------------------------------------------------------
struct GVertex { float x,y,z; };

static QVector<GVertex> buildGeometry(
    int &axisStart,int &axisCount,
    int &borderStart,int &borderCount,
    int &fineStart,int &fineCount,
    int &coarseStart,int &coarseCount)
{
  QVector<GVertex> v; v.reserve(512);
  axisStart=v.size();
  v.append({0,0,0}); v.append({30,0,0});
  v.append({0,0,0}); v.append({0,30,0});
  v.append({0,0,0}); v.append({0,0,30});
  axisCount=v.size()-axisStart;
  borderStart=v.size();
  const float P=220.f;
  v.append({0,0,0}); v.append({P,0,0});
  v.append({P,0,0}); v.append({P,0,P});
  v.append({P,0,P}); v.append({0,0,P});
  v.append({0,0,P}); v.append({0,0,0});
  borderCount=v.size()-borderStart;
  fineStart=v.size();
  for(float t=10.f;t<P-0.001f;t+=10.f){
    v.append({t,0,0}); v.append({t,0,P});
    v.append({0,0,t}); v.append({P,0,t});
  }
  fineCount=v.size()-fineStart;
  coarseStart=v.size();
  for(float t=50.f;t<P-0.001f;t+=50.f){
    v.append({t,0,0}); v.append({t,0,P});
    v.append({0,0,t}); v.append({P,0,t});
  }
  coarseCount=v.size()-coarseStart;
  return v;
}

// ---------------------------------------------------------------------------
// Ray-AABB (slab method)
// ---------------------------------------------------------------------------
static bool rayAABB(const QVector3D &o, const QVector3D &d,
                    const float bmin[], const float bmax[], float &t)
{
  float tmin=-FLT_MAX, tmax=FLT_MAX;
  const float ov[3]={o.x(),o.y(),o.z()};
  const float dv[3]={d.x(),d.y(),d.z()};
  for(int i=0;i<3;i++){
    if(std::abs(dv[i])<1e-8f){
      if(ov[i]<bmin[i]||ov[i]>bmax[i]) return false;
    } else {
      float inv=1.f/dv[i];
      float t1=(bmin[i]-ov[i])*inv, t2=(bmax[i]-ov[i])*inv;
      if(t1>t2) std::swap(t1,t2);
      tmin=std::max(tmin,t1); tmax=std::min(tmax,t2);
      if(tmin>tmax) return false;
    }
  }
  t=tmin>0.f?tmin:tmax;
  return t>0.f;
}

// ===========================================================================
// Constructor / Destructor
// ===========================================================================
GLViewportRenderer::GLViewportRenderer() = default;

GLViewportRenderer::~GLViewportRenderer()
{
  m_vao.destroy(); m_vbo.destroy();
  if(m_f){
    for(auto &b:m_meshBatches){
      if(b.vao) m_f->glDeleteVertexArrays(1,&b.vao);
      if(b.vbo) m_f->glDeleteBuffers(1,&b.vbo);
    }
    if(m_gizmoVao) m_f->glDeleteVertexArrays(1,&m_gizmoVao);
    if(m_gizmoVbo) m_f->glDeleteBuffers(1,&m_gizmoVbo);
  }
}

// ===========================================================================
// createFramebufferObject
// ===========================================================================
QOpenGLFramebufferObject *GLViewportRenderer::createFramebufferObject(const QSize &size)
{
  QOpenGLFramebufferObjectFormat fmt;
  fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
  fmt.setSamples(4);
  m_viewSize=size;
  return new QOpenGLFramebufferObject(size,fmt);
}

// ===========================================================================
// synchronize
// ===========================================================================
void GLViewportRenderer::synchronize(QQuickFramebufferObject *item)
{
  auto *vp=static_cast<GLViewport*>(item);
  m_viewSize=QSize(int(vp->width()),int(vp->height()));

  const auto events=vp->takeEvents();
  for(const auto &e:events)
  {
    switch(e.type)
    {
    case GLViewport::InputEvent::Press:
    {
      const float sx=e.x, sy=e.y;
      if(e.button==Qt::LeftButton)
      {
        // Priority 1: gizmo axis (only if object selected)
        if(m_selectedId>=0){
          int ax=pickGizmoAxis(sx,sy);
          if(ax>0){
            m_gizmoAxis=ax;
            auto [orig,dir]=computeRay(sx,sy);
            QVector3D axDir(ax==1?1:0, ax==2?1:0, ax==3?1:0);
            m_gizmoAxisT=rayToAxisT(orig,dir,axDir);
            m_freeXZDragging=false; m_mouseDragging=false;
            m_lastX=sx; m_lastY=sy;
            break;
          }
        }
        // Priority 2: object pick
        int hitId=pickObject(sx,sy);
        if(hitId>=0){
          m_selectedId=hitId;
          m_freeXZDragging=true;
          m_freeXZOrigin=rayXZIntersect(sx,sy);
          m_mouseDragging=false; m_gizmoAxis=0;
        } else {
          m_selectedId=-1;
          m_freeXZDragging=false; m_gizmoAxis=0;
          m_mouseDragging=true; m_dragButton=Qt::LeftButton;
        }
      } else {
        m_freeXZDragging=false; m_gizmoAxis=0;
        m_mouseDragging=true; m_dragButton=e.button;
      }
      m_lastX=sx; m_lastY=sy;
      break;
    }
    case GLViewport::InputEvent::Move:
    {
      const float dx=e.x-m_lastX, dy=e.y-m_lastY;
      if(m_gizmoAxis>0 && m_selectedId>=0)
      {
        // Axis-constrained drag
        QVector3D axDir(m_gizmoAxis==1?1:0, m_gizmoAxis==2?1:0, m_gizmoAxis==3?1:0);
        auto [orig,dir]=computeRay(e.x,e.y);
        float newT=rayToAxisT(orig,dir,axDir);
        float delta=newT-m_gizmoAxisT;
        m_objectOffsets[m_selectedId]+=delta*axDir;
        m_gizmoAxisT=newT;
      }
      else if(m_freeXZDragging && m_selectedId>=0)
      {
        // Screen-space drag: stable at any camera elevation angle.
        // Decompose screen delta into camera right / forward projected onto XZ plane.
        QMatrix4x4 view = m_camera.viewMatrix();
        // Row 0 of view matrix = camera right; Row 2 = camera back (-forward)
        QVector3D camRight(view(0,0), 0.f, view(0,2));
        QVector3D camFwd  (-view(2,0), 0.f, -view(2,2)); // pointing into scene
        if(camRight.lengthSquared()<1e-6f) camRight=QVector3D(1,0,0);
        else camRight.normalize();
        if(camFwd.lengthSquared()<1e-6f) camFwd=QVector3D(0,0,1);
        else camFwd.normalize();
        // world units per pixel: tan(fov/2)/halfHeight * distance
        const float vH = float(m_viewSize.height());
        const float scale = m_camera.distance() * 0.4142f / (vH > 1.f ? vH * 0.5f : 1.f);
        // dx>0 → right, dy>0 (screen down) → toward viewer = -camFwd
        m_objectOffsets[m_selectedId] += camRight * (dx * scale)
                                        - camFwd  * (dy * scale);
      }
      else if(m_mouseDragging)
      {
        if(m_dragButton==Qt::LeftButton)
          m_camera.orbit(dx*0.5f,-dy*0.5f);
        else if(m_dragButton==Qt::MiddleButton)
          m_camera.pan(dx,dy);
      }
      m_lastX=e.x; m_lastY=e.y;
      break;
    }
    case GLViewport::InputEvent::Release:
      m_mouseDragging=false; m_freeXZDragging=false; m_gizmoAxis=0;
      break;
    case GLViewport::InputEvent::Wheel:
      m_camera.zoom(e.wheelDelta); break;
    case GLViewport::InputEvent::FitView:
      m_camera.fitView(e.fitCX,e.fitCY,e.fitCZ,e.fitRadius); break;
    }
  }

  QByteArray newMesh;
  if(vp->takeMesh(newMesh,m_meshVersion)){
    m_pendingMesh=std::move(newMesh); m_meshDirty=true;
  }
}

// ===========================================================================
// render
// ===========================================================================
void GLViewportRenderer::render()
{
  if(!m_initialized) initialize();
  if(m_meshDirty){ uploadMesh(); m_meshDirty=false; }

  // Update gizmo center to follow selected object
  if(m_selectedId>=0){
    for(const auto &b:m_meshBatches){
      if(b.objectId==m_selectedId){
        QVector3D off(0,0,0);
        auto it=m_objectOffsets.find(m_selectedId);
        if(it!=m_objectOffsets.end()) off=it->second;
        m_gizmoCenter=QVector3D(
            (b.bboxMin[0]+b.bboxMax[0])*0.5f + off.x(),
            (b.bboxMin[1]+b.bboxMax[1])*0.5f + off.y(),
            (b.bboxMin[2]+b.bboxMax[2])*0.5f + off.z());
        break;
      }
    }
  }

  m_f->glEnable(GL_DEPTH_TEST);
  m_f->glEnable(GL_CULL_FACE); m_f->glCullFace(GL_BACK);
  m_f->glClearColor(0.208f,0.224f,0.243f,1.f);
  m_f->glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  const float aspect=m_viewSize.height()>0
      ? float(m_viewSize.width())/float(m_viewSize.height()) : 1.f;
  const QMatrix4x4 mvp=m_camera.projMatrix(aspect)*m_camera.viewMatrix();

  // 1. Mesh batches
  if(!m_meshBatches.empty()){
    m_meshProg.bind();
    m_meshProg.setUniformValue(m_uMeshMVP,mvp);
    for(auto &b:m_meshBatches){
      if(b.vertexCount<=0) continue;
      QVector3D off(0,0,0);
      auto it=m_objectOffsets.find(b.objectId);
      if(it!=m_objectOffsets.end()) off=it->second;
      m_meshProg.setUniformValue(m_uMeshOffset,off);
      m_meshProg.setUniformValue(m_uMeshBaseColor, kObjColors[b.objectId%kNumColors]);
      float bright=(b.objectId==m_selectedId)?1.55f:1.0f;
      m_meshProg.setUniformValue(m_uMeshBright,bright);
      m_f->glBindVertexArray(b.vao);
      m_f->glDrawArrays(GL_TRIANGLES,0,b.vertexCount);
      m_f->glBindVertexArray(0);
    }
    m_meshProg.release();
  }

  // 2. Gizmo (no depth test fighting with mesh)
  if(m_selectedId>=0 && m_gizmoVao)
    renderGizmo(mvp);

  // 3. Grid / axes
  m_f->glDisable(GL_CULL_FACE);
  m_prog.bind();
  m_prog.setUniformValue(m_uMVP,mvp);
  m_vao.bind();
  m_prog.setUniformValue(m_uColor,QVector4D(0.28f,0.30f,0.32f,1.f));
  m_f->glDrawArrays(GL_LINES,m_fineGridStart,m_fineGridCount);
  m_prog.setUniformValue(m_uColor,QVector4D(0.42f,0.44f,0.47f,1.f));
  m_f->glDrawArrays(GL_LINES,m_coarseGridStart,m_coarseGridCount);
  m_prog.setUniformValue(m_uColor,QVector4D(0.85f,0.85f,0.85f,1.f));
  m_f->glDrawArrays(GL_LINES,m_borderStart,m_borderCount);
  m_prog.setUniformValue(m_uColor,QVector4D(0.85f,0.18f,0.18f,1.f));
  m_f->glDrawArrays(GL_LINES,m_axisStart+0,2);
  m_prog.setUniformValue(m_uColor,QVector4D(0.18f,0.78f,0.22f,1.f));
  m_f->glDrawArrays(GL_LINES,m_axisStart+2,2);
  m_prog.setUniformValue(m_uColor,QVector4D(0.18f,0.38f,0.88f,1.f));
  m_f->glDrawArrays(GL_LINES,m_axisStart+4,2);
  m_vao.release();
  m_prog.release();

  m_f->glDisable(GL_DEPTH_TEST);
}

// ===========================================================================
// renderGizmo
// ===========================================================================
void GLViewportRenderer::renderGizmo(const QMatrix4x4 &mvp)
{
  // Compute scale so arrows are ~15% of camera distance
  float dist=(m_gizmoCenter - m_camera.eye()).length();
  float scale=dist*0.15f;
  if(scale<5.f) scale=5.f;

  m_f->glClear(GL_DEPTH_BUFFER_BIT); // draw gizmo on top
  m_f->glDisable(GL_CULL_FACE);

  m_gizmoProg.bind();
  m_gizmoProg.setUniformValue(m_uGizmoMVP, mvp);
  m_gizmoProg.setUniformValue(m_uGizmoCenter, m_gizmoCenter);
  m_gizmoProg.setUniformValue(m_uGizmoScale, scale);

  m_f->glBindVertexArray(m_gizmoVao);

  static constexpr int CONE_VERTS_STATIC = 6*3*2;

  for(int ax=0;ax<3;ax++){
    bool active=(m_gizmoAxis==ax+1);
    QVector4D col = active ? kAxisHighlight[ax] : kAxisColors[ax];
    m_gizmoProg.setUniformValue(m_uGizmoColor, col);

    // Shaft (thick lines via point-draw repeated pairs)
    m_f->glDrawArrays(GL_LINES, m_gizmoShaftBase[ax], 2);
    // Cone
    m_f->glDrawArrays(GL_TRIANGLES, m_gizmoConeBase[ax], CONE_VERTS_STATIC);
  }

  m_f->glBindVertexArray(0);
  m_gizmoProg.release();
  m_f->glEnable(GL_CULL_FACE);
}

// ===========================================================================
// initialize
// ===========================================================================
void GLViewportRenderer::initialize()
{
  m_f=QOpenGLContext::currentContext()->extraFunctions();
  m_f->initializeOpenGLFunctions();

  m_prog.addShaderFromSourceCode(QOpenGLShader::Vertex,   kVertSrc);
  m_prog.addShaderFromSourceCode(QOpenGLShader::Fragment, kFragSrc);
  if(!m_prog.link()) qWarning("[GL] base: %s",qPrintable(m_prog.log()));
  m_uMVP=m_prog.uniformLocation("uMVP");
  m_uColor=m_prog.uniformLocation("uColor");

  m_meshProg.addShaderFromSourceCode(QOpenGLShader::Vertex,   kMeshVertSrc);
  m_meshProg.addShaderFromSourceCode(QOpenGLShader::Fragment, kMeshFragSrc);
  if(!m_meshProg.link()) qWarning("[GL] mesh: %s",qPrintable(m_meshProg.log()));
  m_uMeshMVP       =m_meshProg.uniformLocation("uMVP");
  m_uMeshBaseColor =m_meshProg.uniformLocation("uBaseColor");
  m_uMeshOffset    =m_meshProg.uniformLocation("uOffset");
  m_uMeshBright    =m_meshProg.uniformLocation("uBrightness");

  m_gizmoProg.addShaderFromSourceCode(QOpenGLShader::Vertex,   kGizmoVertSrc);
  m_gizmoProg.addShaderFromSourceCode(QOpenGLShader::Fragment, kGizmoFragSrc);
  if(!m_gizmoProg.link()) qWarning("[GL] gizmo: %s",qPrintable(m_gizmoProg.log()));
  m_uGizmoMVP    =m_gizmoProg.uniformLocation("uMVP");
  m_uGizmoCenter =m_gizmoProg.uniformLocation("uGizmoCenter");
  m_uGizmoScale  =m_gizmoProg.uniformLocation("uGizmoScale");
  m_uGizmoColor  =m_gizmoProg.uniformLocation("uGizmoColor");

  const auto verts=buildGeometry(
      m_axisStart,m_axisCount,m_borderStart,m_borderCount,
      m_fineGridStart,m_fineGridCount,m_coarseGridStart,m_coarseGridCount);
  m_vao.create(); m_vao.bind();
  m_vbo.create(); m_vbo.bind();
  m_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
  m_vbo.allocate(verts.constData(),int(verts.size()*sizeof(GVertex)));
  m_prog.enableAttributeArray(0);
  m_prog.setAttributeBuffer(0,GL_FLOAT,0,3,sizeof(GVertex));
  m_vao.release(); m_vbo.release();

  buildGizmoGeometry();
  m_initialized=true;
}

// ===========================================================================
// buildGizmoGeometry  -- builds static XYZ arrow geometry in local space
// ===========================================================================
void GLViewportRenderer::buildGizmoGeometry()
{
  static const float kGizmoVerts[][3] = {
    {0.00000f,0.00000f,0.00000f},
    {0.78000f,0.00000f,0.00000f},
    {1.00000f,0.00000f,0.00000f},
    {0.78000f,0.05500f,0.00000f},
    {0.78000f,0.02750f,0.04763f},
    {0.78000f,0.00000f,0.00000f},
    {0.78000f,0.02750f,0.04763f},
    {0.78000f,0.05500f,0.00000f},
    {1.00000f,0.00000f,0.00000f},
    {0.78000f,0.02750f,0.04763f},
    {0.78000f,-0.02750f,0.04763f},
    {0.78000f,0.00000f,0.00000f},
    {0.78000f,-0.02750f,0.04763f},
    {0.78000f,0.02750f,0.04763f},
    {1.00000f,0.00000f,0.00000f},
    {0.78000f,-0.02750f,0.04763f},
    {0.78000f,-0.05500f,0.00000f},
    {0.78000f,0.00000f,0.00000f},
    {0.78000f,-0.05500f,0.00000f},
    {0.78000f,-0.02750f,0.04763f},
    {1.00000f,0.00000f,0.00000f},
    {0.78000f,-0.05500f,0.00000f},
    {0.78000f,-0.02750f,-0.04763f},
    {0.78000f,0.00000f,0.00000f},
    {0.78000f,-0.02750f,-0.04763f},
    {0.78000f,-0.05500f,0.00000f},
    {1.00000f,0.00000f,0.00000f},
    {0.78000f,-0.02750f,-0.04763f},
    {0.78000f,0.02750f,-0.04763f},
    {0.78000f,0.00000f,0.00000f},
    {0.78000f,0.02750f,-0.04763f},
    {0.78000f,-0.02750f,-0.04763f},
    {1.00000f,0.00000f,0.00000f},
    {0.78000f,0.02750f,-0.04763f},
    {0.78000f,0.05500f,0.00000f},
    {0.78000f,0.00000f,0.00000f},
    {0.78000f,0.05500f,0.00000f},
    {0.78000f,0.02750f,-0.04763f},
    {0.00000f,0.00000f,0.00000f},
    {0.00000f,0.78000f,0.00000f},
    {0.00000f,1.00000f,0.00000f},
    {0.05500f,0.78000f,0.00000f},
    {0.02750f,0.78000f,0.04763f},
    {0.00000f,0.78000f,0.00000f},
    {0.02750f,0.78000f,0.04763f},
    {0.05500f,0.78000f,0.00000f},
    {0.00000f,1.00000f,0.00000f},
    {0.02750f,0.78000f,0.04763f},
    {-0.02750f,0.78000f,0.04763f},
    {0.00000f,0.78000f,0.00000f},
    {-0.02750f,0.78000f,0.04763f},
    {0.02750f,0.78000f,0.04763f},
    {0.00000f,1.00000f,0.00000f},
    {-0.02750f,0.78000f,0.04763f},
    {-0.05500f,0.78000f,0.00000f},
    {0.00000f,0.78000f,0.00000f},
    {-0.05500f,0.78000f,0.00000f},
    {-0.02750f,0.78000f,0.04763f},
    {0.00000f,1.00000f,0.00000f},
    {-0.05500f,0.78000f,0.00000f},
    {-0.02750f,0.78000f,-0.04763f},
    {0.00000f,0.78000f,0.00000f},
    {-0.02750f,0.78000f,-0.04763f},
    {-0.05500f,0.78000f,0.00000f},
    {0.00000f,1.00000f,0.00000f},
    {-0.02750f,0.78000f,-0.04763f},
    {0.02750f,0.78000f,-0.04763f},
    {0.00000f,0.78000f,0.00000f},
    {0.02750f,0.78000f,-0.04763f},
    {-0.02750f,0.78000f,-0.04763f},
    {0.00000f,1.00000f,0.00000f},
    {0.02750f,0.78000f,-0.04763f},
    {0.05500f,0.78000f,0.00000f},
    {0.00000f,0.78000f,0.00000f},
    {0.05500f,0.78000f,0.00000f},
    {0.02750f,0.78000f,-0.04763f},
    {0.00000f,0.00000f,0.00000f},
    {0.00000f,0.00000f,0.78000f},
    {0.00000f,0.00000f,1.00000f},
    {0.05500f,0.00000f,0.78000f},
    {0.02750f,0.04763f,0.78000f},
    {0.00000f,0.00000f,0.78000f},
    {0.02750f,0.04763f,0.78000f},
    {0.05500f,0.00000f,0.78000f},
    {0.00000f,0.00000f,1.00000f},
    {0.02750f,0.04763f,0.78000f},
    {-0.02750f,0.04763f,0.78000f},
    {0.00000f,0.00000f,0.78000f},
    {-0.02750f,0.04763f,0.78000f},
    {0.02750f,0.04763f,0.78000f},
    {0.00000f,0.00000f,1.00000f},
    {-0.02750f,0.04763f,0.78000f},
    {-0.05500f,0.00000f,0.78000f},
    {0.00000f,0.00000f,0.78000f},
    {-0.05500f,0.00000f,0.78000f},
    {-0.02750f,0.04763f,0.78000f},
    {0.00000f,0.00000f,1.00000f},
    {-0.05500f,0.00000f,0.78000f},
    {-0.02750f,-0.04763f,0.78000f},
    {0.00000f,0.00000f,0.78000f},
    {-0.02750f,-0.04763f,0.78000f},
    {-0.05500f,0.00000f,0.78000f},
    {0.00000f,0.00000f,1.00000f},
    {-0.02750f,-0.04763f,0.78000f},
    {0.02750f,-0.04763f,0.78000f},
    {0.00000f,0.00000f,0.78000f},
    {0.02750f,-0.04763f,0.78000f},
    {-0.02750f,-0.04763f,0.78000f},
    {0.00000f,0.00000f,1.00000f},
    {0.02750f,-0.04763f,0.78000f},
    {0.05500f,0.00000f,0.78000f},
    {0.00000f,0.00000f,0.78000f},
    {0.05500f,0.00000f,0.78000f},
    {0.02750f,-0.04763f,0.78000f},
  };
  static constexpr int kGizmoN = 114;
  m_gizmoShaftBase[0]=0; m_gizmoShaftBase[1]=38; m_gizmoShaftBase[2]=76;
  m_gizmoConeBase[0]=2;  m_gizmoConeBase[1]=40;  m_gizmoConeBase[2]=78;

  m_f->glGenVertexArrays(1,&m_gizmoVao);
  m_f->glGenBuffers(1,&m_gizmoVbo);
  m_f->glBindVertexArray(m_gizmoVao);
  m_f->glBindBuffer(GL_ARRAY_BUFFER,m_gizmoVbo);
  m_f->glBufferData(GL_ARRAY_BUFFER,sizeof(kGizmoVerts),kGizmoVerts,GL_STATIC_DRAW);
  m_f->glEnableVertexAttribArray(0);
  m_f->glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),nullptr);
  m_f->glBindVertexArray(0);
  m_f->glBindBuffer(GL_ARRAY_BUFFER,0);
}

// ===========================================================================
// uploadMesh
// ===========================================================================
void GLViewportRenderer::uploadMesh()
{
  for(auto &b:m_meshBatches){
    if(b.vao) m_f->glDeleteVertexArrays(1,&b.vao);
    if(b.vbo) m_f->glDeleteBuffers(1,&b.vbo);
  }
  m_meshBatches.clear();

  if(m_pendingMesh.size()<int(sizeof(int32_t))) return;
  const char *p=m_pendingMesh.constData(), *end=p+m_pendingMesh.size();
  int32_t objCount=0;
  memcpy(&objCount,p,sizeof(int32_t)); p+=sizeof(int32_t);
  if(objCount<=0) return;
  m_meshBatches.reserve(objCount);

  for(int32_t i=0;i<objCount;++i){
    if(p+2*int(sizeof(int32_t))>end) break;
    int32_t objId=0, triCount=0;
    memcpy(&objId,   p,sizeof(int32_t)); p+=sizeof(int32_t);
    memcpy(&triCount,p,sizeof(int32_t)); p+=sizeof(int32_t);
    const int dataBytes=triCount*9*int(sizeof(float));
    if(p+dataBytes>end) break;

    MeshBatch b; b.objectId=int(objId); b.vertexCount=triCount*3;
    const float *fp=reinterpret_cast<const float*>(p);
    b.bboxMin[0]=b.bboxMax[0]=fp[0];
    b.bboxMin[1]=b.bboxMax[1]=fp[1];
    b.bboxMin[2]=b.bboxMax[2]=fp[2];
    for(int vi=0;vi<triCount*3;vi++)
      for(int ci=0;ci<3;ci++){
        float v=fp[vi*3+ci];
        if(v<b.bboxMin[ci]) b.bboxMin[ci]=v;
        if(v>b.bboxMax[ci]) b.bboxMax[ci]=v;
      }

    m_f->glGenVertexArrays(1,&b.vao); m_f->glGenBuffers(1,&b.vbo);
    m_f->glBindVertexArray(b.vao);
    m_f->glBindBuffer(GL_ARRAY_BUFFER,b.vbo);
    m_f->glBufferData(GL_ARRAY_BUFFER,dataBytes,p,GL_DYNAMIC_DRAW);
    m_f->glEnableVertexAttribArray(0);
    m_f->glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),nullptr);
    m_f->glBindVertexArray(0); m_f->glBindBuffer(GL_ARRAY_BUFFER,0);
    m_meshBatches.push_back(b);
    p+=dataBytes;
  }
  int tt=0; for(const auto &b:m_meshBatches) tt+=b.vertexCount/3;
  qInfo("[GL] %d batches, %d tris",int(m_meshBatches.size()),tt);
}

// ===========================================================================
// computeRay
// ===========================================================================
std::pair<QVector3D,QVector3D>
GLViewportRenderer::computeRay(float sx, float sy) const
{
  const float w=float(m_viewSize.width()), h=float(m_viewSize.height());
  if(w<=0||h<=0) return {{},{1,0,0}};
  QMatrix4x4 invPV=(m_camera.projMatrix(w/h)*m_camera.viewMatrix()).inverted();
  float ndcX=2.f*sx/w-1.f, ndcY=1.f-2.f*sy/h;
  QVector4D nearH=invPV*QVector4D(ndcX,ndcY,-1.f,1.f);
  QVector4D farH =invPV*QVector4D(ndcX,ndcY, 1.f,1.f);
  nearH/=nearH.w(); farH/=farH.w();
  QVector3D orig=nearH.toVector3D();
  return {orig,(farH.toVector3D()-orig).normalized()};
}

// ===========================================================================
// rayXZIntersect
// ===========================================================================
QVector3D GLViewportRenderer::rayXZIntersect(float sx, float sy) const
{
  auto [orig,dir]=computeRay(sx,sy);
  if(std::abs(dir.y())<1e-6f) return {0,0,0};
  float t=-orig.y()/dir.y();
  return orig+t*dir;
}

// ===========================================================================
// pickObject
// ===========================================================================
int GLViewportRenderer::pickObject(float sx, float sy) const
{
  auto [orig,dir]=computeRay(sx,sy);
  float tBest=FLT_MAX; int hitId=-1;
  for(const auto &b:m_meshBatches){
    float bmin[3], bmax[3];
    QVector3D off(0,0,0);
    auto it=m_objectOffsets.find(b.objectId);
    if(it!=m_objectOffsets.end()) off=it->second;
    for(int i=0;i<3;i++){
      float o=(i==0?off.x():i==1?off.y():off.z());
      bmin[i]=b.bboxMin[i]+o; bmax[i]=b.bboxMax[i]+o;
    }
    float t; if(rayAABB(orig,dir,bmin,bmax,t)&&t<tBest){ tBest=t; hitId=b.objectId; }
  }
  return hitId;
}

// ===========================================================================
// rayToAxisT  -- closest t along axisDir at gizmoCenter from screen ray
// ===========================================================================
float GLViewportRenderer::rayToAxisT(const QVector3D &orig, const QVector3D &dir,
                                     const QVector3D &axisDir) const
{
  // Solve shortest distance between two lines:
  // L1: orig + t*dir   L2: m_gizmoCenter + s*axisDir
  QVector3D w=orig-m_gizmoCenter;
  float b=QVector3D::dotProduct(dir,axisDir);
  float e=QVector3D::dotProduct(axisDir,w);
  float d=QVector3D::dotProduct(dir,w);
  float denom=1.f-b*b;
  if(std::abs(denom)<1e-8f) return e; // parallel
  return (e-b*d)/denom;
}

// ===========================================================================
// pickGizmoAxis  -- returns 1/2/3 for X/Y/Z, 0 if no hit
// ===========================================================================
int GLViewportRenderer::pickGizmoAxis(float sx, float sy) const
{
  if(m_meshBatches.empty()) return 0;

  // Compute gizmo scale for proximity threshold
  float dist=(m_gizmoCenter-m_camera.eye()).length();
  float scale=std::max(dist*0.15f,5.f);
  // Threshold = 8% of arrow length in world units
  float thresh=scale*0.08f;

  auto [orig,dir]=computeRay(sx,sy);

  static const QVector3D kAxes[3]={
      QVector3D(1,0,0), QVector3D(0,1,0), QVector3D(0,0,1)};

  float bestDist=FLT_MAX; int best=0;
  for(int ax=0;ax<3;ax++){
    // Closest distance from ray to axis line segment
    QVector3D axDir=kAxes[ax];
    QVector3D w=orig-m_gizmoCenter;
    float b=QVector3D::dotProduct(dir,axDir);
    float e=QVector3D::dotProduct(axDir,w);
    float d=QVector3D::dotProduct(dir,w);
    float denom=1.f-b*b;
    float t_ray, s_axis;
    if(std::abs(denom)<1e-8f){ t_ray=0; s_axis=e; }
    else {
      t_ray =(b*e-d)/denom;
      s_axis=(e -b*d)/denom;
    }
    if(t_ray<0) continue;             // behind camera
    s_axis/=scale;                    // normalise to [0..1] range
    if(s_axis<0||s_axis>1.0f) continue; // outside arrow length

    QVector3D p1=orig+t_ray*dir;
    QVector3D p2=m_gizmoCenter+s_axis*scale*axDir;
    float dist2=(p1-p2).length();
    if(dist2<thresh && dist2<bestDist){
      bestDist=dist2; best=ax+1;
    }
  }
  return best;
}
