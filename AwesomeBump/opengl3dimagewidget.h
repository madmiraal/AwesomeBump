#ifndef OPENGL3DIMAGEWIDGET_H
#define OPENGL3DIMAGEWIDGET_H

#include <QOpenGLFunctions_4_0_Core>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QtMath>

#include "opengl2dimagewidget.h"
#include "display3dsettings.h"
#include "camera.h"
#include "mesh.h"
#include "properties/Dialog3DGeneralSettings.h"

#define KEY_SHOW_MATERIALS Qt::Key_S

class OpenGL3DImageWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_0_Core
{
    Q_OBJECT

public:
    OpenGL3DImageWidget(QWidget* parent, OpenGL2DImageWidget* openGL2DImageWidget);
    ~OpenGL3DImageWidget();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

signals:
    void readyGL();
    // Emit material index color.
    void materialColorPicked(QColor);
    void changeCamPositionApplied(bool);

public slots:
    void show3DGeneralSettingsDialog();
    void toggleDiffuseView(bool);
    void toggleSpecularView(bool);
    void toggleOcclusionView(bool);
    void toggleNormalView(bool);
    void toggleHeightView(bool);
    void toggleRoughnessView(bool);
    void toggleMetallicView(bool);
    void setCameraMouseSensitivity(int value);
    void resetCameraPosition();
    void toggleChangeCamPosition(bool toggle);
    void toggleMouseWrap(bool toggle);

    // Mesh functions.
    void loadMeshFromFile();
    bool loadMeshFile(const QString& fileName);
    void chooseMeshFile(const QString& fileName);

    // PBR functions.
    void chooseSkyBox(QString cubeMapName, bool bFirstTime = false);
    void updatePerformanceSettings(Display3DSettings settings);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void relativeMouseMoveEvent(int dx, int dy, bool *wrapMouse, Qt::MouseButtons buttons);
    void wheelEvent(QWheelEvent *event);
    void dropEvent(QDropEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);

    // Post processing tools.
    void resizeFBOs();
    void deleteFBOs();
    void applyNormalFilter(GLuint input_tex);
    void copyTexToFBO(GLuint input_tex, QOpenGLFramebufferObject* dst);
    void applyGaussFilter(GLuint input_tex,
                          QOpenGLFramebufferObject* auxFBO,
                          QOpenGLFramebufferObject* outputFBO, float radius = 10.0);
    void applyDofFilter(GLuint input_tex,
                        QOpenGLFramebufferObject* outputFBO);
    void applyGlowFilter(QOpenGLFramebufferObject* outputFBO);
    void applyToneFilter(GLuint input_tex, QOpenGLFramebufferObject* outputFBO);
    void applyLensFlaresFilter(GLuint input_tex, QOpenGLFramebufferObject* outputFBO);

private:
    QPointF pixelPosToViewPos(const QPointF& p);
    int glhUnProjectf(float &winx, float &winy, float &winz,
                      QMatrix4x4 &modelview, QMatrix4x4 &projection,
                      QVector4D& objectCoordinate);
    // Calculate prefiltered enviromental map.
    void bakeEnviromentalMaps();
    bool addTexture(GLenum COLOR_ATTACHMENTn);
    const GLuint& getAttachedTexture(GLuint index);
    void setFBOTextureParameters(QOpenGLFramebufferObject *fbo);

    QOpenGLVertexArrayObject* createVertexArray(Mesh* mesh);
    void drawTriangles(QOpenGLVertexArrayObject* vertexArray, unsigned int vertexCount, bool usePatches = false);
    QOpenGLTexture* createTextureCube(const QStringList& fileNames);

    // Render Shader Program
    QOpenGLShaderProgram* renderProgram;
    QOpenGLShaderProgram* line_program;
    QOpenGLShaderProgram* skybox_program;
    QOpenGLShaderProgram* env_program;

    OpenGL2DImageWidget* openGL2DImageWidget;

    bool bToggleDiffuseView;
    bool bToggleSpecularView;
    bool bToggleOcclusionView;
    bool bToggleNormalView;
    bool bToggleHeightView;
    bool bToggleRoughnessView;
    bool bToggleMetallicView;

    Display3DSettings display3Dparameters;
    // 3D shading & display settings dialog.
    Dialog3DGeneralSettings *settingsDialog;

    // 3D view parameters.
    QMatrix4x4 projectionMatrix;
    QMatrix4x4 modelViewMatrix;
    QMatrix3x3 NormalMatrix;
    QMatrix4x4 viewMatrix;
    QMatrix4x4 objectMatrix;

    QVector4D lightPosition;
    QVector4D cursorPositionOnPlane;

    float ratio;
    float zoom;
    // Light used for standard phong shading.
    AwesomeCamera camera;
    // To make smooth linear interpolation between two views.
    AwesomeCamera newCamera;

    double cameraInterpolation;
    // Second light - use camera class to rotate light
    AwesomeCamera lightDirection;
    QCursor lightCursor;

    // Displayed 3D mesh.
    Mesh* mesh;
    // Sky box cube.
    Mesh* skybox_mesh;
    // One trinagle used for calculation of prefiltered environment map.
    Mesh* env_mesh;
    // Quad mesh used for post processing
    Mesh* quad_mesh;

    // Vertex Array Object
    QOpenGLVertexArrayObject* meshArray;
    QOpenGLVertexArrayObject* skyboxMeshArray;
    QOpenGLVertexArrayObject* envMeshArray;
    QOpenGLVertexArrayObject* quadMeshArray;

    // Skybox texture.
    QOpenGLTexture* skyBoxTextureCube;
    // Control calculating diffuse environment map.
    bool bDiffuseMapBaked;

    // Post-processing variables.
    // All post processing functions
    std::map<std::string,QOpenGLShaderProgram*> post_processing_programs;
    // Holds pointer to current post-processing program
    QOpenGLShaderProgram *filter_program;
    QOpenGLFramebufferObject* colorFBO;
    QOpenGLFramebufferObject* outputFBO;
    QOpenGLFramebufferObject* auxFBO;
    // Glow FBOs.
    QOpenGLFramebufferObject* glowInputColor[4];
    QOpenGLFramebufferObject* glowOutputColor[4];
    // Tone mapping mipmaps FBOS.
    QOpenGLFramebufferObject* toneMipmaps[10];

    QVector<GLuint> attachments;

    QOpenGLTexture *lensFlareColorsTexture;
    QOpenGLTexture *lensDirtTexture;
    QOpenGLTexture *lensStarTexture;

    QPoint lastCursorPos;
    bool mouseUpdateIsQueued;
    bool blockMouseMovement;
    Qt::Key keyPressed;
    QCursor centerCamCursor;
    bool wrapMouse;
};

#endif // OPENGL3DIMAGEWIDGET_H
