#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QAction>
#include <QLabel>
#include <QtDebug>
#include <QDateTime>
#include <QMessageBox>

#include <Property.h>
#include <PropertySet.h>
#include "Core/PropertyFloat.h"
#include "Core/PropertyInt.h"
#include "Delegates/PropertyDelegateFactory.h"
#include "Delegates/Utils/PropertyDelegateSliderBox.h"

#include <iostream>

#include "opengl3dimagewidget.h"
#include "opengl2dimagewidget.h"
#include "image.h"
#include "imagewidget.h"
#include "formmaterialindicesmanager.h"
#include "formsettingscontainer.h"
#include "dockwidget3dsettings.h"
#include "properties/Dialog3DGeneralSettings.h"
#include "dialoglogger.h"
#include "dialogshortcuts.h"
#include "properties/PropertyDelegateABColor.h"

// Compressed texture type.
enum CompressedFromTypes
{
    H_TO_D_AND_S_TO_N = 0,
    S_TO_D_AND_H_TO_N = 1
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    bSaveCheckedImages(false),
    bSaveCompressedFormImages(false)
{
    ui->setupUi(this);

    QtnPropertyDelegateFactory::staticInstance().registerDelegateDefault(
            &QtnPropertyFloatBase::staticMetaObject,
            &qtnCreateDelegate<QtnPropertyDelegateSlideBoxTyped<QtnPropertyFloatBase>, QtnPropertyFloatBase>,
            "SliderBox");

    QtnPropertyDelegateFactory::staticInstance().registerDelegateDefault(
            &QtnPropertyIntBase::staticMetaObject,
            &qtnCreateDelegate<QtnPropertyDelegateSlideBoxTyped<QtnPropertyIntBase>, QtnPropertyIntBase>,
            "SliderBox");

    regABColorDelegates();

    abSettings = new QtnPropertySetAwesomeBump(this);

    ImageWidget::recentDir = &recentDir;

    openGL2DImageWidget = new OpenGL2DImageWidget(this);
    openGL3DImageWidget = new OpenGL3DImageWidget(this, openGL2DImageWidget);
}

void MainWindow::initialiseWindow()
{
    //qInstallMessageHandler(customMessageHandler);

    qDebug() << "Starting application:";
    qDebug() << "Application dir:" << QApplication::applicationDirPath();

    qDebug() << "Initialization: Build image properties";
    emit initialisationProgress(10, "Build image properties");

    diffuseImageWidget   = new ImageWidget(this, openGL2DImageWidget, DIFFUSE_TEXTURE);
    normalImageWidget    = new ImageWidget(this, openGL2DImageWidget, NORMAL_TEXTURE);
    specularImageWidget  = new ImageWidget(this, openGL2DImageWidget, SPECULAR_TEXTURE);
    heightImageWidget    = new ImageWidget(this, openGL2DImageWidget, HEIGHT_TEXTURE);
    occlusionImageWidget = new ImageWidget(this, openGL2DImageWidget, OCCLUSION_TEXTURE);
    roughnessImageWidget = new ImageWidget(this, openGL2DImageWidget, ROUGHNESS_TEXTURE);
    metallicImageWidget  = new ImageWidget(this, openGL2DImageWidget, METALLIC_TEXTURE);
    grungeImageWidget    = new ImageWidget(this, openGL2DImageWidget, GRUNGE_TEXTURE);

    materialManager    = new FormMaterialIndicesManager(this, openGL2DImageWidget);

    qDebug() << "Initialization: Setup image properties";
    emit initialisationProgress(20, "Setup image properties");

    // Set pointers to images
    materialManager->imagesPointers[0] = diffuseImageWidget;
    materialManager->imagesPointers[1] = normalImageWidget;
    materialManager->imagesPointers[2] = specularImageWidget;
    materialManager->imagesPointers[3] = heightImageWidget;
    materialManager->imagesPointers[4] = occlusionImageWidget;
    materialManager->imagesPointers[5] = roughnessImageWidget;
    materialManager->imagesPointers[6] = metallicImageWidget;

    openGL2DImageWidget->setImageWidget(DIFFUSE_TEXTURE, diffuseImageWidget);
    openGL2DImageWidget->setImageWidget(NORMAL_TEXTURE, normalImageWidget);
    openGL2DImageWidget->setImageWidget(SPECULAR_TEXTURE, specularImageWidget);
    openGL2DImageWidget->setImageWidget(HEIGHT_TEXTURE, heightImageWidget);
    openGL2DImageWidget->setImageWidget(OCCLUSION_TEXTURE, occlusionImageWidget);
    openGL2DImageWidget->setImageWidget(ROUGHNESS_TEXTURE, roughnessImageWidget);
    openGL2DImageWidget->setImageWidget(METALLIC_TEXTURE, metallicImageWidget);
    openGL2DImageWidget->setImageWidget(GRUNGE_TEXTURE, grungeImageWidget);
    openGL2DImageWidget->setImageWidget(MATERIAL_TEXTURE, 0);

    // Setup GUI
    qDebug() << "Initialization: GUI setup";
    emit initialisationProgress(30, "GUI setup");

    // Settings container
    settingsContainer = new FormSettingsContainer;
    ui->verticalLayout2DImage->addWidget(settingsContainer);
    settingsContainer->hide();
    connect(settingsContainer, SIGNAL (reloadConfigFile()), this, SLOT (loadSettings()));
    connect(settingsContainer, SIGNAL (emitLoadAndConvert()), this, SLOT (convertFromBase()));
    connect(settingsContainer, SIGNAL (forceSaveCurrentConfig()), this, SLOT (saveSettings()));
    connect(ui->pushButtonProjectManager, SIGNAL (toggled(bool)), settingsContainer, SLOT (setVisible(bool)));

    // 3D settings widget
    dock3Dsettings = new DockWidget3DSettings(this, openGL3DImageWidget);

    ui->verticalLayout3DImage->addWidget(dock3Dsettings);
    setDockNestingEnabled(true);
    connect(dock3Dsettings, SIGNAL (signalSelectedShadingModel(int)), this, SLOT (selectShadingModel(int)));
    // show hide 3D settings
    connect(ui->pushButton3DSettings, SIGNAL (toggled(bool)), dock3Dsettings, SLOT (setVisible(bool)));

    //dialog3dGeneralSettings = new Dialog3DGeneralSettings(this);
    //connect(ui->pushButton3DGeneralSettings, SIGNAL (released()), dialog3dGeneralSettings, SLOT (show()));
    connect(ui->pushButton3DGeneralSettings, SIGNAL (released()), openGL3DImageWidget, SLOT (show3DGeneralSettingsDialog()));
    //connect(dialog3dGeneralSettings, SIGNAL (signalPropertyChanged()), openGL3DImageWidget, SLOT (repaint()));
    //connect(dialog3dGeneralSettings, SIGNAL (signalRecompileCustomShader()), openGL3DImageWidget, SLOT (recompileRenderShader()));

    ui->verticalLayout3DImage->addWidget(openGL3DImageWidget);
    ui->verticalLayout2DImage->addWidget(openGL2DImageWidget);

    qDebug() << "Initialization: Adding widgets.";
    emit initialisationProgress(40, "Adding widgets.");

    ui->verticalLayoutDiffuseImage        ->addWidget(diffuseImageWidget);
    ui->verticalLayoutNormalImage         ->addWidget(normalImageWidget);
    ui->verticalLayoutSpecularImage       ->addWidget(specularImageWidget);
    ui->verticalLayoutHeightImage         ->addWidget(heightImageWidget);
    ui->verticalLayoutOcclusionImage      ->addWidget(occlusionImageWidget);
    ui->verticalLayoutRoughnessImage      ->addWidget(roughnessImageWidget);
    ui->verticalLayoutMetallicImage       ->addWidget(metallicImageWidget);
    ui->verticalLayoutGrungeImage         ->addWidget(grungeImageWidget);
    ui->verticalLayoutMaterialIndicesImage->addWidget(materialManager);

    ui->tabWidget->setCurrentIndex(TAB_SETTINGS);
    
    connect(ui->tabWidget, SIGNAL (currentChanged(int)), this, SLOT (updateImage(int)));
    connect(ui->tabWidget, SIGNAL (tabBarClicked(int)), this, SLOT (updateImage(int)));

    connect(diffuseImageWidget,   SIGNAL (imageChanged()), this, SLOT (checkWarnings()));
    connect(occlusionImageWidget, SIGNAL (imageChanged()), this, SLOT (checkWarnings()));
    connect(grungeImageWidget,    SIGNAL (imageChanged()), this, SLOT (checkWarnings()));

    connect(diffuseImageWidget,   SIGNAL(imageChanged()), openGL2DImageWidget, SLOT (imageChanged()));
    connect(roughnessImageWidget, SIGNAL(imageChanged()), openGL2DImageWidget, SLOT (imageChanged()));
    connect(metallicImageWidget,  SIGNAL(imageChanged()), openGL2DImageWidget, SLOT (imageChanged()));

    connect(diffuseImageWidget,   SIGNAL (imageChanged()), this, SLOT (updateDiffuseImage()));
    connect(normalImageWidget,    SIGNAL (imageChanged()), this, SLOT (updateNormalImage()));
    connect(specularImageWidget,  SIGNAL (imageChanged()), this, SLOT (updateSpecularImage()));
    connect(heightImageWidget,    SIGNAL (imageChanged()), this, SLOT (updateHeightImage()));
    connect(occlusionImageWidget, SIGNAL (imageChanged()), this, SLOT (updateOcclusionImage()));
    connect(roughnessImageWidget, SIGNAL (imageChanged()), this, SLOT (updateRoughnessImage()));
    connect(metallicImageWidget,  SIGNAL (imageChanged()), this, SLOT (updateMetallicImage()));
    connect(grungeImageWidget,    SIGNAL (imageChanged()), this, SLOT (updateGrungeImage()));

    qDebug() << "Initialization: Connections and actions.";
    emit initialisationProgress(50, "Connections and actions.");

    // Connect grunge slots.
    connect(grungeImageWidget,    SIGNAL(toggleGrungeSettings(bool)), diffuseImageWidget  , SLOT (toggleGrungeImageSettingsGroup(bool)));
    connect(grungeImageWidget,    SIGNAL(toggleGrungeSettings(bool)), normalImageWidget   , SLOT (toggleGrungeImageSettingsGroup(bool)));
    connect(grungeImageWidget,    SIGNAL(toggleGrungeSettings(bool)), specularImageWidget , SLOT (toggleGrungeImageSettingsGroup(bool)));
    connect(grungeImageWidget,    SIGNAL(toggleGrungeSettings(bool)), heightImageWidget   , SLOT (toggleGrungeImageSettingsGroup(bool)));
    connect(grungeImageWidget,    SIGNAL(toggleGrungeSettings(bool)), occlusionImageWidget, SLOT (toggleGrungeImageSettingsGroup(bool)));
    connect(grungeImageWidget,    SIGNAL(toggleGrungeSettings(bool)), roughnessImageWidget, SLOT (toggleGrungeImageSettingsGroup(bool)));
    connect(grungeImageWidget,    SIGNAL(toggleGrungeSettings(bool)), metallicImageWidget , SLOT (toggleGrungeImageSettingsGroup(bool)));

    // Connect material manager slots.
    connect(materialManager, SIGNAL (materialChanged()), this, SLOT (replotAllImages()));
    connect(materialManager, SIGNAL (materialsToggled(bool)), ui->tabTilling, SLOT (setDisabled(bool)));
    // Disable conversion tool
    connect(materialManager, SIGNAL (materialsToggled(bool)), this, SLOT (materialsToggled(bool)));
    connect(openGL3DImageWidget, SIGNAL (materialColorPicked(QColor)), materialManager, SLOT (chooseMaterialByColor(QColor)));

    connect(diffuseImageWidget,  SIGNAL (imageLoaded(int,int)), this, SLOT (applyResizeImage(int,int)));
    connect(normalImageWidget,   SIGNAL (imageLoaded(int,int)), this, SLOT (applyResizeImage(int,int)));
    connect(specularImageWidget, SIGNAL (imageLoaded(int,int)), this, SLOT (applyResizeImage(int,int)));
    connect(heightImageWidget,   SIGNAL (imageLoaded(int,int)), this, SLOT (applyResizeImage(int,int)));
    connect(occlusionImageWidget,SIGNAL (imageLoaded(int,int)), this, SLOT (applyResizeImage(int,int)));
    connect(roughnessImageWidget,SIGNAL (imageLoaded(int,int)), this, SLOT (applyResizeImage(int,int)));
    connect(metallicImageWidget, SIGNAL (imageLoaded(int,int)), this, SLOT (applyResizeImage(int,int)));
    //connect(grungeImageProp,   SIGNAL (imageLoaded(int,int)), this, SLOT (applyResizeImage(int,int)));
    connect(materialManager,   SIGNAL (imageLoaded(int,int)), this, SLOT (applyResizeImage(int,int)));

    // Connect image reload settings signal.
    connect(diffuseImageWidget,   SIGNAL (reloadSettingsFromConfigFile(TextureType)), this, SLOT (loadImageSettings(TextureType)));
    connect(normalImageWidget,    SIGNAL (reloadSettingsFromConfigFile(TextureType)), this, SLOT (loadImageSettings(TextureType)));
    connect(specularImageWidget,  SIGNAL (reloadSettingsFromConfigFile(TextureType)), this, SLOT (loadImageSettings(TextureType)));
    connect(heightImageWidget,    SIGNAL (reloadSettingsFromConfigFile(TextureType)), this, SLOT (loadImageSettings(TextureType)));
    connect(occlusionImageWidget, SIGNAL (reloadSettingsFromConfigFile(TextureType)), this, SLOT (loadImageSettings(TextureType)));
    connect(roughnessImageWidget, SIGNAL (reloadSettingsFromConfigFile(TextureType)), this, SLOT (loadImageSettings(TextureType)));
    connect(metallicImageWidget,  SIGNAL (reloadSettingsFromConfigFile(TextureType)), this, SLOT (loadImageSettings(TextureType)));
    connect(grungeImageWidget,    SIGNAL (reloadSettingsFromConfigFile(TextureType)), this, SLOT (loadImageSettings(TextureType)));

    // Connect conversion signals.
    connect(diffuseImageWidget,   SIGNAL (conversionBaseConversionApplied()), this, SLOT (convertFromBase()));
    connect(normalImageWidget,    SIGNAL (conversionHeightToNormalApplied()), this, SLOT (convertFromHtoN()));
    connect(heightImageWidget,    SIGNAL (conversionNormalToHeightApplied()), this, SLOT (convertFromNtoH()));
    connect(occlusionImageWidget, SIGNAL (conversionHeightNormalToOcclusionApplied()), this, SLOT (convertFromHNtoOcc()));

    // Save signals.
    connect(ui->pushButtonSaveAll, SIGNAL (released()), this, SLOT (saveImages()));
    connect(ui->pushButtonSaveChecked, SIGNAL (released()), this, SLOT (saveCheckedImages()));
    connect(ui->pushButtonSaveAs, SIGNAL (released()), this, SLOT (saveCompressedForm()));

    // Image properties signals.
    connect(ui->comboBoxResizeWidth,  SIGNAL(currentIndexChanged(int)), this, SLOT (changeWidth(int)));
    connect(ui->comboBoxResizeHeight, SIGNAL(currentIndexChanged(int)), this, SLOT (changeHeight(int)));

    connect(ui->doubleSpinBoxRescaleWidth,  SIGNAL(valueChanged(double)), this, SLOT (scaleWidth(double)));
    connect(ui->doubleSpinBoxRescaleHeight, SIGNAL(valueChanged(double)), this, SLOT (scaleHeight(double)));

    connect(ui->pushButtonResizeApply,         SIGNAL(released()), this, SLOT (applyResizeImage()));
    connect(ui->pushButtonRescaleApply,        SIGNAL(released()), this, SLOT (applyScaleImage()));
    connect(ui->pushButtonReplotAll,           SIGNAL(released()), this, SLOT(replotAllImages()));
    connect(ui->pushButtonResetCameraPosition, SIGNAL(released()), openGL3DImageWidget, SLOT (resetCameraPosition()));
    connect(ui->pushButtonChangeCamPosition,   SIGNAL(toggled(bool)), openGL3DImageWidget,SLOT (toggleChangeCamPosition(bool)));

    connect(openGL3DImageWidget, SIGNAL (changeCamPositionApplied(bool)), ui->pushButtonChangeCamPosition, SLOT (setChecked(bool)));

    connect(ui->pushButtonToggleDiffuse,   SIGNAL(toggled(bool)), openGL3DImageWidget, SLOT (toggleDiffuseView(bool)));
    connect(ui->pushButtonToggleNormal,    SIGNAL(toggled(bool)), openGL3DImageWidget, SLOT (toggleNormalView(bool)));
    connect(ui->pushButtonToggleSpecular,  SIGNAL(toggled(bool)), openGL3DImageWidget, SLOT (toggleSpecularView(bool)));
    connect(ui->pushButtonToggleHeight,    SIGNAL(toggled(bool)), openGL3DImageWidget, SLOT (toggleHeightView(bool)));
    connect(ui->pushButtonToggleOcclusion, SIGNAL(toggled(bool)), openGL3DImageWidget, SLOT (toggleOcclusionView(bool)));
    connect(ui->pushButtonToggleRoughness, SIGNAL(toggled(bool)), openGL3DImageWidget, SLOT (toggleRoughnessView(bool)));
    connect(ui->pushButtonToggleMetallic,  SIGNAL(toggled(bool)), openGL3DImageWidget, SLOT (toggleMetallicView(bool)));

    connect(ui->pushButtonSaveCurrentSettings, SIGNAL(released()), this, SLOT(saveSettings()));
    connect(ui->comboBoxImageOutputFormat, SIGNAL(activated(int)), this, SLOT(setOutputFormat(int)));

    ui->progressBar->setValue(0);

    connect(ui->actionReplot,              SIGNAL(triggered()), this, SLOT (replotAllImages()));
    connect(ui->actionShowDiffuseImage,    SIGNAL(triggered()), this, SLOT (selectDiffuseTab()));
    connect(ui->actionShowNormalImage,     SIGNAL(triggered()), this, SLOT (selectNormalTab()));
    connect(ui->actionShowSpecularImage,   SIGNAL(triggered()), this, SLOT (selectSpecularTab()));
    connect(ui->actionShowHeightImage,     SIGNAL(triggered()), this, SLOT (selectHeightTab()));
    connect(ui->actionShowOcclusiontImage, SIGNAL(triggered()), this, SLOT (selectOcclusionTab()));
    connect(ui->actionShowRoughnessImage,  SIGNAL(triggered()), this, SLOT (selectRoughnessTab()));
    connect(ui->actionShowMetallicImage,   SIGNAL(triggered()), this, SLOT (selectMetallicTab()));
    connect(ui->actionShowGrungeTexture,   SIGNAL(triggered()), this, SLOT (selectGrungeTab()));
    connect(ui->actionShowMaterialsImage,  SIGNAL(triggered()), this, SLOT (selectMaterialsTab()));

    connect(ui->checkBoxSaveDiffuse,   SIGNAL(toggled(bool)), this, SLOT (showHideTextureTypes(bool)));
    connect(ui->checkBoxSaveNormal,    SIGNAL(toggled(bool)), this, SLOT (showHideTextureTypes(bool)));
    connect(ui->checkBoxSaveSpecular,  SIGNAL(toggled(bool)), this, SLOT (showHideTextureTypes(bool)));
    connect(ui->checkBoxSaveHeight,    SIGNAL(toggled(bool)), this, SLOT (showHideTextureTypes(bool)));
    connect(ui->checkBoxSaveOcclusion, SIGNAL(toggled(bool)), this, SLOT (showHideTextureTypes(bool)));
    connect(ui->checkBoxSaveRoughness, SIGNAL(toggled(bool)), this, SLOT (showHideTextureTypes(bool)));
    connect(ui->checkBoxSaveMetallic,  SIGNAL(toggled(bool)), this, SLOT (showHideTextureTypes(bool)));

    connect(ui->actionShowSettingsImage, SIGNAL (triggered()), this, SLOT (selectGeneralSettingsTab()));
    connect(ui->actionShowUVsTab, SIGNAL (triggered()), this, SLOT (selectUVsTab()));
    connect(ui->actionFitToScreen, SIGNAL (triggered()), this, SLOT (fitImage()));

    qDebug() << "Initialization: Perspective tool connections.";
    emit initialisationProgress(60, "Perspective tool connections.");

    // Connect perspective tool signals.
    connect(ui->pushButtonResetTransform, SIGNAL (released()), this, SLOT (resetTransform()));
    connect(ui->comboBoxPerspectiveTransformMethod, SIGNAL (activated(int)), openGL2DImageWidget, SLOT (selectPerspectiveTransformMethod(int)));
    connect(ui->comboBoxSeamlessMode, SIGNAL (activated(int)), this, SLOT (selectSeamlessMode(int)));
    connect(ui->comboBoxSeamlessContrastInputImage, SIGNAL (activated(int)), this, SLOT (selectContrastInputImage(int)));

    qDebug() << "Initialization: UV seamless connections.";
    emit initialisationProgress(70, "UV seamless connections.");

    // Connect uv seamless algorithms.
    connect(ui->checkBoxUVTranslationsFirst, SIGNAL (clicked()), this, SLOT (updateSliders()));
    connect(ui->horizontalSliderMakeSeamlessRadius, SIGNAL (sliderReleased()), this, SLOT (updateSliders()));
    connect(ui->horizontalSliderMakeSeamlessRadius, SIGNAL (valueChanged(int)), this, SLOT (updateSpinBoxes(int)));
    connect(ui->horizontalSliderSeamlessContrastStrenght, SIGNAL (sliderReleased()), this, SLOT (updateSliders()));
    connect(ui->horizontalSliderSeamlessContrastStrenght, SIGNAL (valueChanged(int)), this, SLOT (updateSpinBoxes(int)));
    connect(ui->horizontalSliderSeamlessContrastPower, SIGNAL (sliderReleased()), this, SLOT (updateSliders()));
    connect(ui->horizontalSliderSeamlessContrastPower, SIGNAL (valueChanged(int)), this, SLOT (updateSpinBoxes(int)));

    QButtonGroup *groupSimpleDirectionMode = new QButtonGroup( this );
    groupSimpleDirectionMode->addButton( ui->radioButtonSeamlessSimpleDirXY);
    groupSimpleDirectionMode->addButton( ui->radioButtonSeamlessSimpleDirX);
    groupSimpleDirectionMode->addButton( ui->radioButtonSeamlessSimpleDirY);
    connect(ui->radioButtonSeamlessSimpleDirXY, SIGNAL (released()), this, SLOT (updateSliders()));
    connect(ui->radioButtonSeamlessSimpleDirX,  SIGNAL (released()), this, SLOT (updateSliders()));
    connect(ui->radioButtonSeamlessSimpleDirY,  SIGNAL (released()), this, SLOT (updateSliders()));

    QButtonGroup *groupMirroMode = new QButtonGroup( this );
    groupMirroMode->addButton( ui->radioButtonMirrorModeX);
    groupMirroMode->addButton( ui->radioButtonMirrorModeY);
    groupMirroMode->addButton( ui->radioButtonMirrorModeXY);
    connect(ui->radioButtonMirrorModeX,  SIGNAL (released()), this, SLOT (updateSliders()));
    connect(ui->radioButtonMirrorModeY,  SIGNAL (released()), this, SLOT (updateSliders()));
    connect(ui->radioButtonMirrorModeXY, SIGNAL (released()), this, SLOT (updateSliders()));

    // Connect random mode signals.
    connect(ui->pushButtonRandomPatchesRandomize, SIGNAL (released()), this, SLOT (randomizeAngles()));
    connect(ui->pushButtonRandomPatchesReset, SIGNAL (released()), this, SLOT (resetRandomPatches()));
    connect(ui->horizontalSliderRandomPatchesRotate,      SIGNAL (sliderReleased()), this, SLOT (updateSliders()));
    connect(ui->horizontalSliderRandomPatchesInnerRadius, SIGNAL (sliderReleased()), this, SLOT (updateSliders()));
    connect(ui->horizontalSliderRandomPatchesOuterRadius, SIGNAL (sliderReleased()), this, SLOT (updateSliders()));
    connect(ui->horizontalSliderRandomPatchesRotate,      SIGNAL (valueChanged(int)), this, SLOT (updateSpinBoxes(int)));
    connect(ui->horizontalSliderRandomPatchesInnerRadius, SIGNAL (valueChanged(int)), this, SLOT (updateSpinBoxes(int)));
    connect(ui->horizontalSliderRandomPatchesOuterRadius, SIGNAL (valueChanged(int)), this, SLOT (updateSpinBoxes(int)));

    // Apply UVs tranformations
    connect(ui->pushButtonApplyUVtransformations, SIGNAL (released()), this, SLOT (applyCurrentUVsTransformations()));
    ui->groupBoxSimpleSeamlessMode->hide();
    ui->groupBoxMirrorMode->hide();
    ui->groupBoxRandomPatchesMode->hide();

    // Color picking signals.
    connect(diffuseImageWidget,   SIGNAL (pickImageColor(QtnPropertyABColor*)), openGL2DImageWidget, SLOT (pickImageColor( QtnPropertyABColor*)));
    connect(roughnessImageWidget, SIGNAL (pickImageColor(QtnPropertyABColor*)), openGL2DImageWidget, SLOT (pickImageColor( QtnPropertyABColor*)));
    connect(metallicImageWidget,  SIGNAL (pickImageColor(QtnPropertyABColor*)), openGL2DImageWidget, SLOT (pickImageColor( QtnPropertyABColor*)));

    // 2D imate tool box settings.
    QActionGroup *group = new QActionGroup( this );
    group->addAction(ui->actionTranslateUV);
    group->addAction(ui->actionGrabCorners);
    group->addAction(ui->actionScaleXY);
    ui->actionTranslateUV->setChecked(true);
    connect(ui->actionTranslateUV, SIGNAL (triggered()), this, SLOT (setUVManipulationMethod()));
    connect(ui->actionGrabCorners, SIGNAL (triggered()), this, SLOT (setUVManipulationMethod()));
    connect(ui->actionScaleXY,     SIGNAL (triggered()), this, SLOT (setUVManipulationMethod()));

    // Other settings.
    connect(ui->spinBoxMouseSensitivity, SIGNAL (valueChanged(int)), openGL3DImageWidget, SLOT (setCameraMouseSensitivity(int)));
    connect(ui->spinBoxFontSize, SIGNAL (valueChanged(int)), this, SLOT (changeGUIFontSize(int)));
    connect(ui->checkBoxToggleMouseLoop, SIGNAL (toggled(bool)), openGL3DImageWidget, SLOT (toggleMouseWrap(bool)));
    connect(ui->checkBoxToggleMouseLoop, SIGNAL (toggled(bool)), openGL2DImageWidget, SLOT (toggleMouseWrap(bool)));

    // Batch settings
    connect(ui->pushButtonImageBatchSource, SIGNAL (pressed()), this, SLOT (selectSourceImages()));
    connect(ui->pushButtonImageBatchOutput, SIGNAL (pressed()), this, SLOT (selectOutputPath()));
    connect(ui->pushButtonImageBatchRun, SIGNAL (pressed()), this, SLOT (runBatch()));

#ifdef Q_OS_MAC
    if(ui->statusbar && !ui->statusbar->testAttribute(Qt::WA_MacNormalSize))
        ui->statusbar->setAttribute(Qt::WA_MacSmallSize);
#endif

    // Checking for GUI styles
    QStringList guiStyleList = QStyleFactory::keys();
    qDebug() << "Supported GUI styles: " << guiStyleList.join(", ");
    ui->comboBoxGUIStyle->addItems(guiStyleList);
    ui->labelFontSize->setVisible(false);
    ui->spinBoxFontSize->setVisible(false);

    // Load settings.
    qDebug() << "Loading settings:" ;
    loadSettings();

    qDebug() << "Initialization: Loading default (initial) textures.";
    emit initialisationProgress(80, "Loading default (initial) textures.");

    emit initialisationProgress(90, "Updating main menu items.");

    aboutAction = new QAction(QIcon(":/resources/icons/cube.png"), tr("&About %1").arg(qApp->applicationName()), this);
    aboutAction->setToolTip(tr("Show information about AwesomeBump"));
    aboutAction->setMenuRole(QAction::AboutQtRole);
    aboutAction->setMenuRole(QAction::AboutRole);

    shortcutsAction = new QAction(QString("Shortcuts"),this);

    aboutQtAction = new QAction(QIcon(":/resources/icons/Qt.png"), tr("About &Qt"), this);
    aboutQtAction->setToolTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);

    logAction = new QAction("Show log file",this);
    dialogLogger    = new DialogLogger(this, AB_LOG);
    dialogShortcuts = new DialogShortcuts(this);
    //dialogLogger->setModal(true);
    dialogShortcuts->setModal(true);

    connect(aboutAction, SIGNAL (triggered()), this, SLOT (about()));
    connect(aboutQtAction, SIGNAL (triggered()), this, SLOT (aboutQt()));
    connect(logAction, SIGNAL (triggered()), dialogLogger, SLOT (showLog()));
    connect(shortcutsAction, SIGNAL (triggered()), dialogShortcuts, SLOT (show()));

    QMenu *help = menuBar()->addMenu(tr("&Help"));
    help->addAction(aboutAction);
    help->addAction(aboutQtAction);
    help->addAction(logAction);
    help->addAction(shortcutsAction);

    QAction *action = ui->toolBar->toggleViewAction();
    ui->menubar->addAction(action);

    selectDiffuseTab();

    // Hide warning icons.
    ui->pushButtonMaterialWarning  ->setVisible(false);
    ui->pushButtonConversionWarning->setVisible(false);
    ui->pushButtonGrungeWarning    ->setVisible(false);
    ui->pushButtonUVWarning        ->setVisible(false);
    ui->pushButtonOccWarning       ->setVisible(false);

    qDebug() << "Initialization: Done - UI ready.";
    emit initialisationProgress(100, tr("Done - UI ready."));

    setWindowTitle(AWESOME_BUMP_VERSION);
    resize(sizeHint());
}

MainWindow::~MainWindow()
{
    delete dialogLogger;
    delete dialogShortcuts;
    delete materialManager;
    delete settingsContainer;
    delete dock3Dsettings;
    delete diffuseImageWidget;
    delete normalImageWidget;
    delete specularImageWidget;
    delete heightImageWidget;
    delete occlusionImageWidget;
    delete roughnessImageWidget;
    delete grungeImageWidget;
    delete metallicImageWidget;
    delete openGL2DImageWidget;
    delete openGL3DImageWidget;
    delete abSettings;
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent( event );
    settingsContainer->close();
    openGL3DImageWidget->close();
    openGL2DImageWidget->close();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent( event );
    replotAllImages();
}

void MainWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent( event );
    replotAllImages();
}

void MainWindow::replotAllImages()
{
    TextureType lastActive = openGL2DImageWidget->getActiveTextureType();
    openGL2DImageWidget->enableShadowRender(true);

    // Skip grunge map if conversion is enabled
    if(openGL2DImageWidget->getConversionType() != CONVERT_FROM_D_TO_O)
    {
        updateImage(GRUNGE_TEXTURE);
    }

    updateImage(DIFFUSE_TEXTURE);
    updateImage(ROUGHNESS_TEXTURE);
    updateImage(METALLIC_TEXTURE);
    updateImage(HEIGHT_TEXTURE);
    // Recalulate normal.
    updateImage(NORMAL_TEXTURE);
    // Recalculate the ambient occlusion.
    updateImage(OCCLUSION_TEXTURE);
    updateImage(SPECULAR_TEXTURE);
    updateImage(MATERIAL_TEXTURE);

    openGL2DImageWidget->enableShadowRender(false);
    openGL2DImageWidget->setActiveTextureType(lastActive);
    openGL3DImageWidget->update();
}

void MainWindow::materialsToggled(bool toggle)
{
    static bool bLastValue;
    ui->pushButtonMaterialWarning->setVisible(toggle);
    ui->pushButtonUVWarning->setVisible(Image::seamlessMode != SEAMLESS_NONE);
    if(toggle)
    {
        bLastValue = diffuseImageWidget->getImage()->getProperties()->BaseMapToOthers.EnableConversion;
        diffuseImageWidget->getImage()->getProperties()->BaseMapToOthers.EnableConversion = false;
        ui->pushButtonUVWarning->setVisible(false);
        if(bLastValue)
            replotAllImages();
    }
    else
    {
        diffuseImageWidget->getImage()->getProperties()->BaseMapToOthers.EnableConversion = bLastValue;
    }
    diffuseImageWidget->getImage()->getProperties()->BaseMapToOthers.switchState(QtnPropertyStateInvisible,toggle);
}

void MainWindow::checkWarnings()
{
    ui->pushButtonConversionWarning->setVisible(diffuseImageWidget->getImage()->getProperties()->BaseMapToOthers.EnableConversion);
    ui->pushButtonGrungeWarning->setVisible(grungeImageWidget->getImage()->getProperties()->Grunge.OverallWeight.value() > 0);
    ui->pushButtonUVWarning->setVisible(Image::seamlessMode != SEAMLESS_NONE);

    bool bOccTest = (occlusionImageWidget->getInputImageType() == INPUT_FROM_HO_NO) ||
            (occlusionImageWidget->getInputImageType() == INPUT_FROM_HI_NI);
    ui->pushButtonOccWarning->setVisible(bOccTest);
}

void MainWindow::selectDiffuseTab()
{
    ui->tabWidget->setCurrentIndex(0);
    updateImage(DIFFUSE_TEXTURE);
}

void MainWindow::selectNormalTab()
{
    ui->tabWidget->setCurrentIndex(1);
    updateImage(NORMAL_TEXTURE);
}

void MainWindow::selectSpecularTab()
{
    ui->tabWidget->setCurrentIndex(2);
    updateImage(SPECULAR_TEXTURE);
}

void MainWindow::selectHeightTab()
{
    ui->tabWidget->setCurrentIndex(3);
    updateImage(HEIGHT_TEXTURE);
}

void MainWindow::selectOcclusionTab()
{
    ui->tabWidget->setCurrentIndex(4);
    updateImage(OCCLUSION_TEXTURE);
}

void MainWindow::selectRoughnessTab()
{
    ui->tabWidget->setCurrentIndex(5);
    updateImage(ROUGHNESS_TEXTURE);
}

void MainWindow::selectMetallicTab()
{
    ui->tabWidget->setCurrentIndex(6);
    updateImage(METALLIC_TEXTURE);
}

void MainWindow::selectMaterialsTab()
{
    ui->tabWidget->setCurrentIndex(7);
    updateImage(MATERIAL_TEXTURE);
}

void MainWindow::selectGrungeTab()
{
    ui->tabWidget->setCurrentIndex(8);
    updateImage(GRUNGE_TEXTURE);
}

void MainWindow::selectGeneralSettingsTab()
{
    ui->tabWidget->setCurrentIndex(TAB_SETTINGS);
}

void MainWindow::selectUVsTab()
{
    ui->tabWidget->setCurrentIndex(TAB_SETTINGS+1);
}

void MainWindow::fitImage()
{
    openGL2DImageWidget->resetView();
    openGL2DImageWidget->repaint();
}

void MainWindow::showHideTextureTypes(bool)
{
    //qDebug() << "Toggle processing images";

    bool value = ui->checkBoxSaveDiffuse->isChecked();
    openGL2DImageWidget->setSkipProcessing(DIFFUSE_TEXTURE, !value);
    ui->tabWidget->setTabEnabled(DIFFUSE_TEXTURE, value);
    ui->pushButtonToggleDiffuse->setVisible(value);
    ui->pushButtonToggleDiffuse->setChecked(value);
    ui->actionShowDiffuseImage->setVisible(value);

    value = ui->checkBoxSaveNormal->isChecked();
    openGL2DImageWidget->setSkipProcessing(NORMAL_TEXTURE, !value);
    ui->tabWidget->setTabEnabled(NORMAL_TEXTURE, value);
    ui->pushButtonToggleNormal->setVisible(value);
    ui->pushButtonToggleNormal->setChecked(value);
    ui->actionShowNormalImage->setVisible(value);

    value = ui->checkBoxSaveHeight->isChecked();
    openGL2DImageWidget->setSkipProcessing(OCCLUSION_TEXTURE, !value);
    ui->tabWidget->setTabEnabled(OCCLUSION_TEXTURE, value);
    ui->pushButtonToggleOcclusion->setVisible(value);
    ui->pushButtonToggleOcclusion->setChecked(value);
    ui->actionShowOcclusiontImage->setVisible(value);

    value = ui->checkBoxSaveOcclusion->isChecked();
    openGL2DImageWidget->setSkipProcessing(HEIGHT_TEXTURE, !value);
    ui->tabWidget->setTabEnabled(HEIGHT_TEXTURE, value);
    ui->pushButtonToggleHeight->setVisible(value);
    ui->pushButtonToggleHeight->setChecked(value);
    ui->actionShowHeightImage->setVisible(value);

    value = ui->checkBoxSaveSpecular->isChecked();
    openGL2DImageWidget->setSkipProcessing(SPECULAR_TEXTURE, !value);
    ui->tabWidget->setTabEnabled(SPECULAR_TEXTURE, value);
    ui->pushButtonToggleSpecular->setVisible(value);
    ui->pushButtonToggleSpecular->setChecked(value);
    ui->actionShowSpecularImage->setVisible(value);

    value = ui->checkBoxSaveRoughness->isChecked();
    openGL2DImageWidget->setSkipProcessing(ROUGHNESS_TEXTURE, !value);
    ui->tabWidget->setTabEnabled(ROUGHNESS_TEXTURE, value);
    ui->pushButtonToggleRoughness->setVisible(value);
    ui->pushButtonToggleRoughness->setChecked(value);
    ui->actionShowRoughnessImage->setVisible(value);

    value = ui->checkBoxSaveMetallic->isChecked();
    openGL2DImageWidget->setSkipProcessing(METALLIC_TEXTURE, !value);
    ui->tabWidget->setTabEnabled(METALLIC_TEXTURE, value);
    ui->pushButtonToggleMetallic->setVisible(value);
    ui->pushButtonToggleMetallic->setChecked(value);
    ui->actionShowMetallicImage->setVisible(value);
}

void MainWindow::saveImages()
{
    QString dir = QFileDialog::getExistingDirectory(
                this, tr("Choose Directory"),
                recentDir.absolutePath(),
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if( dir != "" ) saveAllImages(dir);
}

bool MainWindow::saveAllImages(const QString &dir)
{
    QFileInfo fileInfo(dir);
    if (!fileInfo.exists())
    {
        QMessageBox::information(
                    this, QGuiApplication::applicationDisplayName(),
                    tr("Cannot save to %1.").arg(QDir::toNativeSeparators(dir)));
        return false;
    }

    qDebug() << Q_FUNC_INFO << "Saving to dir:" << fileInfo.absoluteFilePath();

    diffuseImageWidget   ->setImageName(ui->lineEditOutputName->text());
    normalImageWidget    ->setImageName(ui->lineEditOutputName->text());
    heightImageWidget    ->setImageName(ui->lineEditOutputName->text());
    specularImageWidget  ->setImageName(ui->lineEditOutputName->text());
    occlusionImageWidget ->setImageName(ui->lineEditOutputName->text());
    roughnessImageWidget ->setImageName(ui->lineEditOutputName->text());
    metallicImageWidget  ->setImageName(ui->lineEditOutputName->text());

    replotAllImages();
    setCursor(Qt::WaitCursor);
    QCoreApplication::processEvents();
    ui->progressBar->setValue(0);

    if(!bSaveCompressedFormImages)
    {
        ui->labelProgressInfo->setText("Saving diffuse image...");
        if(!bSaveCheckedImages || ui->checkBoxSaveDiffuse->isChecked())
            diffuseImageWidget ->saveFileToDir(dir);
        ui->progressBar->setValue(15);
        ui->labelProgressInfo->setText("Saving normal image...");
        if(!bSaveCheckedImages || ui->checkBoxSaveNormal->isChecked())
            normalImageWidget  ->saveFileToDir(dir);
        ui->progressBar->setValue(30);
        ui->labelProgressInfo->setText("Saving specular image...");
        if(!bSaveCheckedImages || ui->checkBoxSaveSpecular->isChecked())
            specularImageWidget->saveFileToDir(dir);
        ui->progressBar->setValue(45);
        ui->labelProgressInfo->setText("Saving height image...");
        if(!bSaveCheckedImages || ui->checkBoxSaveHeight->isChecked())
            occlusionImageWidget  ->saveFileToDir(dir);
        ui->progressBar->setValue(60);
        ui->labelProgressInfo->setText("Saving occlusion image...");
        if(!bSaveCheckedImages || ui->checkBoxSaveOcclusion->isChecked())
            heightImageWidget  ->saveFileToDir(dir);
        ui->progressBar->setValue(75);
        ui->labelProgressInfo->setText("Saving roughness image...");
        if(!bSaveCheckedImages || ui->checkBoxSaveRoughness->isChecked())
            roughnessImageWidget  ->saveFileToDir(dir);
        ui->progressBar->setValue(90);
        ui->labelProgressInfo->setText("Saving metallic image...");
        if(!bSaveCheckedImages || ui->checkBoxSaveMetallic->isChecked())
            metallicImageWidget ->saveFileToDir(dir);
        ui->progressBar->setValue(100);
    }
    else // Save as compressed format
    {
        qDebug() << "Cannot save compressed format!";
        // This needs to be moved to OpenGL2DImageWidget.
        /*
        QCoreApplication::processEvents();
        openGL2DImageWidget->makeCurrent();

        QOpenGLFramebufferObject *diffuseFBOImage  = diffuseImageWidget->getImage()->getFBO();
        QOpenGLFramebufferObject *normalFBOImage   = normalImageWidget->getImage()->getFBO();
        QOpenGLFramebufferObject *specularFBOImage = specularImageWidget->getImage()->getFBO();
        QOpenGLFramebufferObject *heightFBOImage   = heightImageWidget->getImage()->getFBO();

        QImage diffuseImage = diffuseFBOImage->toImage() ;
        QImage normalImage  = normalFBOImage->toImage();
        QImage heightImage  = specularFBOImage->toImage();
        QImage specularImage= heightFBOImage->toImage();

        ui->progressBar->setValue(20);
        ui->labelProgressInfo->setText("Preparing images...");

        QCoreApplication::processEvents();

        QImage newDiffuseImage = QImage(diffuseImage.width(), diffuseImage.height(), QImage::Format_ARGB32);
        QImage newNormalImage  = QImage(diffuseImage.width(), diffuseImage.height(), QImage::Format_ARGB32);

        unsigned char* newDiffuseBuffer  = newDiffuseImage.bits();
        unsigned char* newNormalBuffer   = newNormalImage.bits();
        unsigned char* srcDiffuseBuffer  = diffuseImage.bits();
        unsigned char* srcNormalBuffer   = normalImage.bits();
        unsigned char* srcSpecularBuffer = specularImage.bits();
        unsigned char* srcHeightBuffer   = heightImage.bits();

        int w = diffuseImage.width();
        int h = diffuseImage.height();

        // Put height or specular color to alpha channel according to compression type
        switch(ui->comboBoxSaveAsOptions->currentIndex())
        {
        case(H_TO_D_AND_S_TO_N):
            for(int i = 0; i <h; i++)
            {
                for(int j = 0; j < w; j++)
                {
                    newDiffuseBuffer[4 * (i * w + j) + 0] = srcDiffuseBuffer[4 * (i * w + j)+0  ] ;
                    newDiffuseBuffer[4 * (i * w + j) + 1] = srcDiffuseBuffer[4 * (i * w + j)+1  ] ;
                    newDiffuseBuffer[4 * (i * w + j) + 2] = srcDiffuseBuffer[4 * (i * w + j)+2  ] ;
                    newDiffuseBuffer[4 * (i * w + j) + 3] = srcHeightBuffer[4 * (i * w + j)+0  ] ;

                    newNormalBuffer[4 * (i * w + j) + 0] = srcNormalBuffer[4 * (i * w + j)+0  ] ;
                    newNormalBuffer[4 * (i * w + j) + 1] = srcNormalBuffer[4 * (i * w + j)+1  ] ;
                    newNormalBuffer[4 * (i * w + j) + 2] = srcNormalBuffer[4 * (i * w + j)+2  ] ;
                    newNormalBuffer[4 * (i * w + j) + 3] = srcSpecularBuffer[4 * (i * w + j)+0  ] ;
                }
            }
            break;
        case(S_TO_D_AND_H_TO_N):
            for(int i = 0; i <h; i++)
            {
                for(int j = 0; j < w; j++)
                {
                    newDiffuseBuffer[4 * (i * w + j) + 0] = srcDiffuseBuffer[4 * (i * w + j)+0  ] ;
                    newDiffuseBuffer[4 * (i * w + j) + 1] = srcDiffuseBuffer[4 * (i * w + j)+1  ] ;
                    newDiffuseBuffer[4 * (i * w + j) + 2] = srcDiffuseBuffer[4 * (i * w + j)+2  ] ;
                    newDiffuseBuffer[4 * (i * w + j) + 3] = srcSpecularBuffer[4 * (i * w + j)+0  ] ;

                    newNormalBuffer[4 * (i * w + j) + 0] = srcNormalBuffer[4 * (i * w + j)+0  ] ;
                    newNormalBuffer[4 * (i * w + j) + 1] = srcNormalBuffer[4 * (i * w + j)+1  ] ;
                    newNormalBuffer[4 * (i * w + j) + 2] = srcNormalBuffer[4 * (i * w + j)+2  ] ;
                    newNormalBuffer[4 * (i * w + j) + 3] = srcHeightBuffer[4 * (i * w + j)+0  ] ;
                }
            }
            break;
        }

        ui->progressBar->setValue(50);
        ui->labelProgressInfo->setText("Saving diffuse image...");

        diffuseImageWidget->saveImageToDir(dir,newDiffuseImage);

        ui->progressBar->setValue(80);
        ui->labelProgressInfo->setText("Saving diffuse image...");
        normalImageWidget->saveImageToDir(dir,newNormalImage);
        */
    } // End of save as compressed formats.

    QCoreApplication::processEvents();
    ui->progressBar->setValue(100);
    ui->labelProgressInfo->setText("Done!");
    setCursor(Qt::ArrowCursor);

    return true;
}

void MainWindow::saveCheckedImages()
{
    bSaveCheckedImages = true;
    saveImages();
    bSaveCheckedImages = false;
}

void MainWindow::saveCompressedForm()
{
    bSaveCompressedFormImages = true;
    saveImages();
    bSaveCompressedFormImages = false;
}

void MainWindow::updateDiffuseImage()
{
    ui->lineEditOutputName->setText(diffuseImageWidget->getImageName());
    updateImageInformation();
    openGL2DImageWidget->repaint();

    // Replot normal if height was changed in attached mode.
    if(specularImageWidget->getInputImageType() == INPUT_FROM_DIFFUSE_OUTPUT)
    {
        openGL2DImageWidget->enableShadowRender(true);
        updateImage(SPECULAR_TEXTURE);
        //glImage->updateGL();
        openGL2DImageWidget->enableShadowRender(false);
        // set height tab back again
        updateImage(DIFFUSE_TEXTURE);
    }

    // Replot normal if height was changed in attached mode.
    if(roughnessImageWidget->getInputImageType() == INPUT_FROM_DIFFUSE_OUTPUT)
    {
        openGL2DImageWidget->enableShadowRender(true);
        updateImage(ROUGHNESS_TEXTURE);
        //glImage->updateGL();
        openGL2DImageWidget->enableShadowRender(false);
        // set height tab back again
        updateImage(DIFFUSE_TEXTURE);
    }

    // Replot normal if height was changed in attached mode.
    if(metallicImageWidget->getInputImageType() == INPUT_FROM_DIFFUSE_OUTPUT)
    {
        openGL2DImageWidget->enableShadowRender(true);
        updateImage(METALLIC_TEXTURE);
        //glImage->updateGL();
        openGL2DImageWidget->enableShadowRender(false);
        // set height tab back again
        updateImage(DIFFUSE_TEXTURE);
    }

    openGL3DImageWidget->repaint();
}

void MainWindow::updateNormalImage()
{
    ui->lineEditOutputName->setText(normalImageWidget->getImageName());
    openGL2DImageWidget->repaint();

    // Replot normal if was changed in attached mode.
    if(occlusionImageWidget->getInputImageType() == INPUT_FROM_HO_NO)
    {
        openGL2DImageWidget->enableShadowRender(true);
        updateImage(OCCLUSION_TEXTURE);
        //glImage->updateGL();
        openGL2DImageWidget->enableShadowRender(false);
        // set height tab back again
        updateImage(NORMAL_TEXTURE);
    }

    openGL3DImageWidget->repaint();
}

void MainWindow::updateSpecularImage()
{
    ui->lineEditOutputName->setText(specularImageWidget->getImageName());
    openGL2DImageWidget->repaint();
    openGL3DImageWidget->repaint();
}

void MainWindow::updateHeightImage()
{
    ui->lineEditOutputName->setText(heightImageWidget->getImageName());
    openGL2DImageWidget->repaint();

    // Replot normal if height was changed in attached mode.
    if(normalImageWidget->getInputImageType() == INPUT_FROM_HEIGHT_OUTPUT)
    {
        openGL2DImageWidget->enableShadowRender(true);
        updateImage(NORMAL_TEXTURE);
        //glImage->updateGL();
        openGL2DImageWidget->enableShadowRender(false);
        // set height tab back again
        updateImage(HEIGHT_TEXTURE);
    }

    // Replot normal if was changed in attached mode.
    if(specularImageWidget->getInputImageType() == INPUT_FROM_HEIGHT_OUTPUT)
    {
        openGL2DImageWidget->enableShadowRender(true);
        updateImage(SPECULAR_TEXTURE);
        //glImage->updateGL();
        openGL2DImageWidget->enableShadowRender(false);
        // set height tab back again
        updateImage(HEIGHT_TEXTURE);
    }

    // Replot normal if was changed in attached mode.
    if(occlusionImageWidget->getInputImageType() == INPUT_FROM_HI_NI||
            occlusionImageWidget->getInputImageType() == INPUT_FROM_HO_NO)
    {
        openGL2DImageWidget->enableShadowRender(true);
        updateImage(OCCLUSION_TEXTURE);
        //glImage->updateGL();
        openGL2DImageWidget->enableShadowRender(false);
        // set height tab back again
        updateImage(HEIGHT_TEXTURE);
    }

    // Replot normal if was changed in attached mode.
    if(roughnessImageWidget->getInputImageType() == INPUT_FROM_HEIGHT_OUTPUT)
    {
        openGL2DImageWidget->enableShadowRender(true);
        updateImage(ROUGHNESS_TEXTURE);
        //glImage->updateGL();
        openGL2DImageWidget->enableShadowRender(false);
        // set height tab back again
        updateImage(HEIGHT_TEXTURE);
    }

    // Replot normal if was changed in attached mode
    if(metallicImageWidget->getInputImageType() == INPUT_FROM_HEIGHT_OUTPUT)
    {
        openGL2DImageWidget->enableShadowRender(true);
        updateImage(METALLIC_TEXTURE);
        //glImage->updateGL();
        openGL2DImageWidget->enableShadowRender(false);
        // set height tab back again
        updateImage(HEIGHT_TEXTURE);
    }
    openGL3DImageWidget->repaint();
}

void MainWindow::updateOcclusionImage()
{
    ui->lineEditOutputName->setText(occlusionImageWidget->getImageName());
    openGL2DImageWidget->repaint();
    openGL3DImageWidget->repaint();
}

void MainWindow::updateRoughnessImage()
{
    ui->lineEditOutputName->setText(roughnessImageWidget->getImageName());
    openGL2DImageWidget->repaint();
    openGL3DImageWidget->repaint();
}

void MainWindow::updateMetallicImage()
{
    ui->lineEditOutputName->setText(metallicImageWidget->getImageName());
    openGL2DImageWidget->repaint();
    openGL3DImageWidget->repaint();
}

void MainWindow::updateGrungeImage()
{
    bool test = (grungeImageWidget->getImage()->getProperties()->Grunge.ReplotAll == true);

    // If replot enabled and grunge weight > 0 then replot all textures.
    if(test)
    {
        replotAllImages();
    }
    else
    {
        // Otherwise replot only the grunge map.
        openGL2DImageWidget->repaint();
        openGL3DImageWidget->repaint();
    }
}

void MainWindow::updateImageInformation()
{
    ui->labelCurrentImageWidth ->setNum(openGL2DImageWidget->getTextureWidth(DIFFUSE_TEXTURE));
    ui->labelCurrentImageHeight->setNum(openGL2DImageWidget->getTextureHeight(DIFFUSE_TEXTURE));
}

void MainWindow::initializeGL()
{
    static bool one_time = false;
    // Context is vallid at this moment
    if (!one_time)
    {
        one_time = true;
        qDebug() << "calling" << Q_FUNC_INFO;

        diffuseImageWidget  ->setImageName(ui->lineEditOutputName->text());
        normalImageWidget   ->setImageName(ui->lineEditOutputName->text());
        heightImageWidget   ->setImageName(ui->lineEditOutputName->text());
        specularImageWidget ->setImageName(ui->lineEditOutputName->text());
        occlusionImageWidget->setImageName(ui->lineEditOutputName->text());
        roughnessImageWidget->setImageName(ui->lineEditOutputName->text());
        metallicImageWidget ->setImageName(ui->lineEditOutputName->text());
        grungeImageWidget   ->setImageName(ui->lineEditOutputName->text());
    }
}

void MainWindow::initializeImages()
{
    qDebug() << "MainWindow::Initialization";
    QCoreApplication::processEvents();

    replotAllImages();
    // SSAO recalculation
    TextureType lastActive = openGL2DImageWidget->getActiveTextureType();

    updateImage(OCCLUSION_TEXTURE);
    //glImage->update();
    openGL2DImageWidget->setActiveTextureType(lastActive);
}

void MainWindow::updateImage(int tType)
{
    openGL2DImageWidget->setActiveTextureType((TextureType)tType);
    openGL3DImageWidget->update();
}

void MainWindow::changeWidth(int)
{
    if(ui->pushButtonResizePropTo->isChecked())
        ui->comboBoxResizeHeight->setCurrentText(ui->comboBoxResizeWidth->currentText());
}

void MainWindow::changeHeight(int)
{
    if(ui->pushButtonResizePropTo->isChecked())
        ui->comboBoxResizeWidth->setCurrentText(ui->comboBoxResizeHeight->currentText());
}

void MainWindow::applyResizeImage()
{
    QCoreApplication::processEvents();
    int width  = ui->comboBoxResizeWidth->currentText().toInt();
    int height = ui->comboBoxResizeHeight->currentText().toInt();
    qDebug() << "Image resize applied. Current image size is (" << width << "," << height << ")" ;

    int materiaIndex = Image::currentMaterialIndex;
    materialManager->disableMaterials();

    TextureType lastActive = openGL2DImageWidget->getActiveTextureType();
    openGL2DImageWidget->enableShadowRender(true);
    for(int i = 0 ; i < TEXTURES; i++)
    {
        if( i != GRUNGE_TEXTURE)
        {
            // Grunge map does not scale like other images.
            openGL2DImageWidget->resizeFBO(width,height);
            updateImage(i);
        }
    }
    openGL2DImageWidget->enableShadowRender(false);
    openGL2DImageWidget->setActiveTextureType(lastActive);
    replotAllImages();
    updateImageInformation();
    openGL3DImageWidget->repaint();

    // Replot all material group after image resize.
    Image::currentMaterialIndex = materiaIndex;
    if(materialManager->isEnabled())
    {
        materialManager->toggleMaterials(true);
    }
}

void MainWindow::applyResizeImage(int width, int height)
{
    QCoreApplication::processEvents();

    qDebug() << "Image resize applied. Current image size is (" << width << "," << height << ")" ;
    int materiaIndex = Image::currentMaterialIndex;
    materialManager->disableMaterials();
    TextureType lastActive = openGL2DImageWidget->getActiveTextureType();
    openGL2DImageWidget->enableShadowRender(true);
    for(int i = 0 ; i < TEXTURES; i++)
    {
        if( i != GRUNGE_TEXTURE)
        {
            openGL2DImageWidget->resizeFBO(width,height);
            updateImage(i);
        }
    }
    openGL2DImageWidget->enableShadowRender(false);
    openGL2DImageWidget->setActiveTextureType(lastActive);
    replotAllImages();
    updateImageInformation();
    openGL3DImageWidget->repaint();

    // Replot all material group after image resize.
    Image::currentMaterialIndex = materiaIndex;
    if(materialManager->isEnabled())
    {
        materialManager->toggleMaterials(true);
    }
}

void MainWindow::scaleWidth(double)
{
    if(ui->pushButtonRescalePropTo->isChecked())
    {
        ui->doubleSpinBoxRescaleHeight->setValue(ui->doubleSpinBoxRescaleWidth->value());
    }
}

void MainWindow::scaleHeight(double)
{
    if(ui->pushButtonRescalePropTo->isChecked())
    {
        ui->doubleSpinBoxRescaleWidth->setValue(ui->doubleSpinBoxRescaleHeight->value());
    }
}

void MainWindow::applyScaleImage()
{
    QCoreApplication::processEvents();
    float scale_width   = ui->doubleSpinBoxRescaleWidth ->value();
    float scale_height  = ui->doubleSpinBoxRescaleHeight->value();
    int width  = openGL2DImageWidget->getTextureWidth(DIFFUSE_TEXTURE) * scale_width;
    int height = openGL2DImageWidget->getTextureHeight(DIFFUSE_TEXTURE) * scale_height;

    qDebug() << "Image rescale applied. Current image size is (" << width << "," << height << ")" ;
    int materiaIndex = Image::currentMaterialIndex;
    materialManager->disableMaterials();
    TextureType lastActive = openGL2DImageWidget->getActiveTextureType();
    openGL2DImageWidget->enableShadowRender(true);
    for(int i = 0 ; i < TEXTURES; i++)
    {
        openGL2DImageWidget->resizeFBO(width,height);
        updateImage(i);
    }
    openGL2DImageWidget->enableShadowRender(false);
    openGL2DImageWidget->setActiveTextureType(lastActive);
    replotAllImages();
    updateImageInformation();
    openGL3DImageWidget->repaint();

    // Replot all material group after image resize.
    Image::currentMaterialIndex = materiaIndex;
    if(materialManager->isEnabled())
    {
        materialManager->toggleMaterials(true);
    }
}

void MainWindow::applyCurrentUVsTransformations()
{
    // Get current diffuse image (with applied UVs transformations).
    QImage diffuseImage = openGL2DImageWidget->getTextureFBOImage(DIFFUSE_TEXTURE);
    // Reset all the transformations
    ui->comboBoxSeamlessMode->setCurrentIndex(0);
    selectSeamlessMode(0);
    resetTransform();
    // Set as default.
    openGL2DImageWidget->setTextureImage(DIFFUSE_TEXTURE, diffuseImage);
    // Generate all textures based on new image.
    bool bConvValue = diffuseImageWidget->getImage()->getProperties()->BaseMapToOthers.EnableConversion;
    diffuseImageWidget->getImage()->getProperties()->BaseMapToOthers.EnableConversion = true;
    convertFromBase();
    diffuseImageWidget->getImage()->getProperties()->BaseMapToOthers.EnableConversion = bConvValue;
}

void MainWindow::selectSeamlessMode(int mode)
{
    // some gui interaction -> hide and show
    ui->groupBoxSimpleSeamlessMode->hide();
    ui->groupBoxMirrorMode->hide();
    ui->groupBoxRandomPatchesMode->hide();
    ui->groupBoxUVContrastSettings->setDisabled(false);
    ui->horizontalSliderSeamlessContrastPower->setEnabled(true);
    ui->comboBoxSeamlessContrastInputImage->setEnabled(true);
    ui->doubleSpinBoxSeamlessContrastPower->setEnabled(true);
    switch(mode)
    {
    case SEAMLESS_NONE:
        break;
    case SEAMLESS_SIMPLE:
        ui->groupBoxSimpleSeamlessMode->show();
        ui->labelContrastStrenght->setText("Contrast strenght");
        break;
    case SEAMLESS_MIRROR:
        ui->groupBoxMirrorMode->show();
        ui->groupBoxUVContrastSettings->setDisabled(true);
        break;
    case SEAMLESS_RANDOM:
        ui->groupBoxRandomPatchesMode->show();
        ui->labelContrastStrenght->setText("Radius");
        ui->doubleSpinBoxSeamlessContrastPower->setEnabled(false);
        ui->horizontalSliderSeamlessContrastPower->setEnabled(false);
        ui->comboBoxSeamlessContrastInputImage->setEnabled(false);
        break;
    default:
        break;
    }
    openGL2DImageWidget->selectSeamlessMode((SeamlessMode)mode);
    checkWarnings();
    replotAllImages();
}

void MainWindow::selectContrastInputImage(int mode)
{
    switch(mode)
    {
    case(0):
        Image::seamlessContrastImageType = INPUT_FROM_HEIGHT_INPUT;
        break;
    case(1):
        Image::seamlessContrastImageType = INPUT_FROM_DIFFUSE_INPUT;
        break;
    case(2):
        Image::seamlessContrastImageType = INPUT_FROM_NORMAL_INPUT;
        break;
    case(3):
        Image::seamlessContrastImageType = INPUT_FROM_OCCLUSION_INPUT;
        break;
    default:
        break;
    }
    replotAllImages();
}

void MainWindow::selectSourceImages()
{
    QString startPath;
    if(recentDir.exists())
        startPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first();
    else
        startPath = recentDir.absolutePath();

    QString source = QFileDialog::getExistingDirectory(
                this, tr("Select source directory"),
                startPath,
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    QDir dir(source);
    qDebug() << "Selecting source folder for batch processing: " << source;

    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.bmp" << "*.tga";
    QFileInfoList fileInfoList = dir.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot);

    ui->listWidgetImageBatch->clear();
    foreach (QFileInfo fileInfo, fileInfoList)
    {
        qDebug() << "Found:" << fileInfo.absoluteFilePath();
        ui->listWidgetImageBatch->addItem(fileInfo.fileName());
    }
    ui->lineEditImageBatchSource->setText(source);
}

void MainWindow::selectOutputPath()
{
    QString startPath;
    if(recentDir.exists())
        startPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first();
    else
        startPath = recentDir.absolutePath();

    QString path = QFileDialog::getExistingDirectory(
                this, tr("Select source directory"),
                startPath,
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    ui->lineEditImageBatchOutput->setText(path);
}

void MainWindow::runBatch()
{
    QString sourceFolder = ui->lineEditImageBatchSource->text();
    QString outputFolder = ui->lineEditImageBatchOutput->text();

    // Check if output path exists.
    if(!QDir(outputFolder).exists() || outputFolder == "")
    {
        QMessageBox msgBox;
        msgBox.setText("Info");
        msgBox.setInformativeText("Output path is not provided");
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.exec();
        return;
    }

    qDebug() << "Starting batch mode: this may take some time";
    while(ui->listWidgetImageBatch->count() > 0)
    {
        QListWidgetItem* item = ui->listWidgetImageBatch->takeItem(0);
        ui->labelBatchProgress->setText("Images left: " + QString::number(ui->listWidgetImageBatch->count()+1));
        ui->labelBatchProgress->repaint();
        QCoreApplication::processEvents();

        QString imageName = item->text();
        ui->lineEditOutputName->setText(imageName);
        QString imagePath = sourceFolder + "/" + imageName;

        qDebug() << "Processing image: " << imagePath;
        diffuseImageWidget->loadFile(imagePath);
        convertFromBase();
        saveAllImages(outputFolder);

        delete item;
        ui->listWidgetImageBatch->repaint();
        QCoreApplication::processEvents();
    }

    ui->labelBatchProgress->setText("Done.");
}


void MainWindow::randomizeAngles()
{
    Image::randomize();
    replotAllImages();
}

void MainWindow::resetRandomPatches()
{
    Image::randomReset();
    ui->horizontalSliderRandomPatchesRotate     ->setValue(Image::randomCommonPhase);
    ui->horizontalSliderRandomPatchesInnerRadius->setValue(Image::randomInnerRadius * 100.0);
    ui->horizontalSliderRandomPatchesOuterRadius->setValue(Image::randomOuterRadius * 100.0);
    updateSpinBoxes(0);
    replotAllImages();
}

void MainWindow::updateSpinBoxes(int)
{
    ui->doubleSpinBoxMakeSeamless->setValue(ui->horizontalSliderMakeSeamlessRadius->value()/100.0);

    // Random tilling mode.
    ui->doubleSpinBoxRandomPatchesAngle      ->setValue(ui->horizontalSliderRandomPatchesRotate     ->value());
    ui->doubleSpinBoxRandomPatchesInnerRadius->setValue(ui->horizontalSliderRandomPatchesInnerRadius->value()/100.0);
    ui->doubleSpinBoxRandomPatchesOuterRadius->setValue(ui->horizontalSliderRandomPatchesOuterRadius->value()/100.0);
    // Seamless strength.
    ui->doubleSpinBoxSeamlessContrastStrenght->setValue(ui->horizontalSliderSeamlessContrastStrenght->value()/100.0);
    ui->doubleSpinBoxSeamlessContrastPower->setValue(ui->horizontalSliderSeamlessContrastPower->value()/100.0);
}

void MainWindow::selectShadingModel(int i)
{
    if(i == 0) ui->tabWidget->setTabText(5,"Rgnss");
    if(i == 1) ui->tabWidget->setTabText(5,"Gloss");
}

void MainWindow::convertFromHtoN()
{
    openGL2DImageWidget->setConversionType(CONVERT_FROM_H_TO_N);
    openGL2DImageWidget->enableShadowRender(true);
    openGL2DImageWidget->setActiveTextureType(HEIGHT_TEXTURE);
    openGL2DImageWidget->enableShadowRender(true);
    openGL2DImageWidget->setConversionType(CONVERT_FROM_H_TO_N);
    openGL2DImageWidget->setActiveTextureType(NORMAL_TEXTURE);
    openGL2DImageWidget->enableShadowRender(false);
    openGL2DImageWidget->setConversionType(CONVERT_NONE);

    replotAllImages();

    qDebug() << "Conversion from height to normal applied";
}

void MainWindow::convertFromNtoH()
{
    openGL2DImageWidget->setConversionType(CONVERT_FROM_H_TO_N);// fake conversion
    openGL2DImageWidget->enableShadowRender(true);
    openGL2DImageWidget->setActiveTextureType(HEIGHT_TEXTURE);
    openGL2DImageWidget->setConversionType(CONVERT_FROM_N_TO_H);
    openGL2DImageWidget->enableShadowRender(true);
    openGL2DImageWidget->setActiveTextureType(NORMAL_TEXTURE);
    openGL2DImageWidget->setConversionType(CONVERT_FROM_N_TO_H);
    openGL2DImageWidget->enableShadowRender(true);
    openGL2DImageWidget->setActiveTextureType(HEIGHT_TEXTURE);
    openGL2DImageWidget->enableShadowRender(false);
    replotAllImages();

    qDebug() << "Conversion from normal to height applied";
}

void MainWindow::convertFromBase()
{
    TextureType lastActive = openGL2DImageWidget->getActiveTextureType();
    openGL2DImageWidget->setActiveTextureType(DIFFUSE_TEXTURE);
    qDebug() << "Conversion from Base to others started";
    normalImageWidget   ->setImageName(diffuseImageWidget->getImageName());
    heightImageWidget   ->setImageName(diffuseImageWidget->getImageName());
    specularImageWidget ->setImageName(diffuseImageWidget->getImageName());
    occlusionImageWidget->setImageName(diffuseImageWidget->getImageName());
    roughnessImageWidget->setImageName(diffuseImageWidget->getImageName());
    metallicImageWidget ->setImageName(diffuseImageWidget->getImageName());
    openGL2DImageWidget->setConversionType(CONVERT_FROM_D_TO_O);
    openGL2DImageWidget->update();
    openGL2DImageWidget->setConversionType(CONVERT_FROM_D_TO_O);
    replotAllImages();

    openGL2DImageWidget->setActiveTextureType(lastActive);
    openGL3DImageWidget->update();
    qDebug() << "Conversion from Base to others applied";
}

void MainWindow::convertFromHNtoOcc()
{
    openGL2DImageWidget->setConversionType(CONVERT_FROM_HN_TO_OC);
    openGL2DImageWidget->enableShadowRender(true);

    openGL2DImageWidget->setActiveTextureType(HEIGHT_TEXTURE);
    openGL2DImageWidget->setConversionType(CONVERT_FROM_HN_TO_OC);
    openGL2DImageWidget->enableShadowRender(true);

    openGL2DImageWidget->setActiveTextureType(NORMAL_TEXTURE);
    openGL2DImageWidget->setConversionType(CONVERT_FROM_HN_TO_OC);
    openGL2DImageWidget->enableShadowRender(true);

    openGL2DImageWidget->setActiveTextureType(OCCLUSION_TEXTURE);
    openGL2DImageWidget->enableShadowRender(false);

    replotAllImages();

    qDebug() << "Conversion from Height and Normal to Occlusion applied";
}

void MainWindow::updateSliders()
{
    updateSpinBoxes(0);
    Image::seamlessSimpleModeRadius          = ui->doubleSpinBoxMakeSeamless->value();
    Image::seamlessContrastStrength          = ui->doubleSpinBoxSeamlessContrastStrenght->value();
    Image::seamlessContrastPower             = ui->doubleSpinBoxSeamlessContrastPower->value();
    Image::randomCommonPhase = ui->doubleSpinBoxRandomPatchesAngle->value()/180.0*3.1415926;
    Image::randomInnerRadius = ui->doubleSpinBoxRandomPatchesInnerRadius->value();
    Image::randomOuterRadius = ui->doubleSpinBoxRandomPatchesOuterRadius->value();

    Image::bSeamlessTranslationsFirst = ui->checkBoxUVTranslationsFirst->isChecked();
    // Choose the proper mirror mode.
    if(ui->radioButtonMirrorModeXY->isChecked()) Image::seamlessMirroModeType = 0;
    if(ui->radioButtonMirrorModeX ->isChecked()) Image::seamlessMirroModeType = 1;
    if(ui->radioButtonMirrorModeY ->isChecked()) Image::seamlessMirroModeType = 2;

    // Choose the proper simple mode direction.
    if(ui->radioButtonSeamlessSimpleDirXY->isChecked()) Image::seamlessSimpleModeDirection = 0;
    if(ui->radioButtonSeamlessSimpleDirX ->isChecked()) Image::seamlessSimpleModeDirection = 1;
    if(ui->radioButtonSeamlessSimpleDirY ->isChecked()) Image::seamlessSimpleModeDirection = 2;

    openGL2DImageWidget ->repaint();
    openGL3DImageWidget->repaint();
}

void MainWindow::resetTransform()
{
    QVector2D corner(0,0);
    openGL2DImageWidget->updateCornersPosition(corner,corner,corner,corner);
    openGL2DImageWidget->updateCornersWeights(0,0,0,0);
    replotAllImages();
}

void MainWindow::setUVManipulationMethod()
{
    if(ui->actionTranslateUV->isChecked()) openGL2DImageWidget->selectUVManipulationMethod(UV_TRANSLATE);
    if(ui->actionGrabCorners->isChecked()) openGL2DImageWidget->selectUVManipulationMethod(UV_GRAB_CORNERS);
    if(ui->actionScaleXY->isChecked())     openGL2DImageWidget->selectUVManipulationMethod(UV_SCALE_XY);
}

QSize MainWindow::sizeHint() const
{
    return QSize(abSettings->d_win_w,abSettings->d_win_h);
}

void MainWindow::loadImageSettings(TextureType type)
{
    switch(type)
    {
    case DIFFUSE_TEXTURE:
        diffuseImageWidget    ->getImage()->getProperties()->copyValues(&abSettings->Diffuse);
        break;
    case NORMAL_TEXTURE:
        normalImageWidget     ->getImage()->getProperties()->copyValues(&abSettings->Normal);
        break;
    case SPECULAR_TEXTURE:
        specularImageWidget   ->getImage()->getProperties()->copyValues(&abSettings->Specular);
        break;
    case HEIGHT_TEXTURE:
        heightImageWidget     ->getImage()->getProperties()->copyValues(&abSettings->Height);
        break;
    case OCCLUSION_TEXTURE:
        occlusionImageWidget  ->getImage()->getProperties()->copyValues(&abSettings->Occlusion);
        break;
    case ROUGHNESS_TEXTURE:
        roughnessImageWidget  ->getImage()->getProperties()->copyValues(&abSettings->Roughness);
        break;
    case METALLIC_TEXTURE:
        metallicImageWidget   ->getImage()->getProperties()->copyValues(&abSettings->Metallic);
        break;
    case GRUNGE_TEXTURE:
        grungeImageWidget     ->getImage()->getProperties()->copyValues(&abSettings->Grunge);
        break;
    default:
        qWarning() << "Trying to load non supported image! Given textureType:" << type;
    }
    openGL2DImageWidget ->repaint();
    openGL3DImageWidget->repaint();
}

void MainWindow::showSettingsManager()
{
    settingsContainer->show();
}

void MainWindow::saveSettings()
{
    qDebug() << "Calling" << Q_FUNC_INFO << "Saving to :"<< QString(AB_INI);

    abSettings->d_win_w =  this->width();
    abSettings->d_win_h =  this->height();
    abSettings->tab_win_w = ui->tabWidget->width();
    abSettings->tab_win_h = ui->tabWidget->height();
    abSettings->recent_dir      = recentDir.absolutePath();
    abSettings->recent_mesh_dir = recentMeshDir.absolutePath();

    diffuseImageWidget->getImage()->getProperties()->suffix = ui->lineEditPostfixDiffuse->text();
    normalImageWidget->getImage()->getProperties()->suffix = ui->lineEditPostfixNormal->text();
    specularImageWidget->getImage()->getProperties()->suffix = ui->lineEditPostfixSpecular->text();
    heightImageWidget->getImage()->getProperties()->suffix = ui->lineEditPostfixHeight->text();
    occlusionImageWidget->getImage()->getProperties()->suffix = ui->lineEditPostfixOcclusion->text();
    roughnessImageWidget->getImage()->getProperties()->suffix = ui->lineEditPostfixRoughness->text();
    metallicImageWidget->getImage()->getProperties()->suffix = ui->lineEditPostfixMetallic->text();

    abSettings->gui_style=ui->comboBoxGUIStyle->currentText();

    // UV settings.
    abSettings->uv_tiling_type=ui->comboBoxSeamlessMode->currentIndex();
    abSettings->uv_tiling_radius=ui->horizontalSliderMakeSeamlessRadius->value();
    abSettings->uv_tiling_mirror_x=ui->radioButtonMirrorModeX->isChecked();
    abSettings->uv_tiling_mirror_y=ui->radioButtonMirrorModeY->isChecked();
    abSettings->uv_tiling_mirror_xy=ui->radioButtonMirrorModeXY->isChecked();
    abSettings->uv_tiling_random_inner_radius=ui->horizontalSliderRandomPatchesInnerRadius->value();
    abSettings->uv_tiling_random_outer_radius=ui->horizontalSliderRandomPatchesOuterRadius->value();
    abSettings->uv_tiling_random_rotate=ui->horizontalSliderRandomPatchesRotate->value();

    // UV contrast etc.
    abSettings->uv_translations_first=ui->checkBoxUVTranslationsFirst->isChecked();
    abSettings->uv_contrast_strength=ui->doubleSpinBoxSeamlessContrastStrenght->value();
    abSettings->uv_contrast_power=ui->doubleSpinBoxSeamlessContrastPower->value();
    abSettings->uv_contrast_input_image=ui->comboBoxSeamlessContrastInputImage->currentIndex();
    abSettings->uv_tiling_simple_dir_xy=ui->radioButtonSeamlessSimpleDirXY->isChecked();
    abSettings->uv_tiling_simple_dir_x=ui->radioButtonSeamlessSimpleDirX->isChecked();
    abSettings->uv_tiling_simple_dir_y=ui->radioButtonSeamlessSimpleDirY->isChecked();

    // Other parameters.
    abSettings->use_texture_interpolation=ui->checkBoxUseLinearTextureInterpolation->isChecked();
    abSettings->mouse_sensitivity=ui->spinBoxMouseSensitivity->value();
    abSettings->font_size=ui->spinBoxFontSize->value();
    abSettings->mouse_loop=ui->checkBoxToggleMouseLoop->isChecked();

    dock3Dsettings->saveSettings(abSettings);

    abSettings->Diffuse  .copyValues(diffuseImageWidget   ->getImage()->getProperties());
    abSettings->Specular .copyValues(specularImageWidget  ->getImage()->getProperties());
    abSettings->Normal   .copyValues(normalImageWidget    ->getImage()->getProperties());
    abSettings->Occlusion.copyValues(occlusionImageWidget ->getImage()->getProperties());
    abSettings->Height   .copyValues(heightImageWidget    ->getImage()->getProperties());
    abSettings->Metallic .copyValues(metallicImageWidget  ->getImage()->getProperties());
    abSettings->Roughness.copyValues(roughnessImageWidget ->getImage()->getProperties());
    abSettings->Grunge   .copyValues(grungeImageWidget    ->getImage()->getProperties());

    // Disable possibility to save conversion status
    //    abSettings->Diffuse.BaseMapToOthers.EnableConversion.setValue(false);

    QFile file( QString(AB_INI) );
    if( !file.open( QIODevice::WriteOnly ) )
        return;
    QTextStream stream(&file);
    QString data;
    abSettings->toStr(data);
    stream << data;
}

void MainWindow::changeGUIFontSize(int value)
{
    QFont font;
    font.setFamily(font.defaultFamily());
    font.setPixelSize(value);
    QApplication::setFont(font);
}

void MainWindow::setOutputFormat(int)
{
    Image::outputFormat = ui->comboBoxImageOutputFormat->currentText();
}

void MainWindow::loadSettings()
{
    static bool bFirstTime = true;

    qDebug() << "Calling" << Q_FUNC_INFO << " loading from " << QString(AB_INI);

    ImageWidget::loadingImages = true;

    QFile file( QString(AB_INI) );
    if( !file.open( QIODevice::ReadOnly ) )
        return;

    QTextStream stream(&file);
    QString data;

    //Skip one line
    stream.readLine();
    data = stream.readAll();
    abSettings->fromStr(data);

    QString name = abSettings->settings_name.value();
    ui->pushButtonProjectManager->setText("Project manager (" + name + ")");

    qDebug() << diffuseImageWidget->getImage()->getProperties()->suffix;

    diffuseImageWidget    ->getImage()->getProperties()->copyValues(&abSettings->Diffuse);
    specularImageWidget   ->getImage()->getProperties()->copyValues(&abSettings->Specular);
    normalImageWidget     ->getImage()->getProperties()->copyValues(&abSettings->Normal);
    occlusionImageWidget  ->getImage()->getProperties()->copyValues(&abSettings->Occlusion);
    heightImageWidget     ->getImage()->getProperties()->copyValues(&abSettings->Height);
    metallicImageWidget   ->getImage()->getProperties()->copyValues(&abSettings->Metallic);
    roughnessImageWidget  ->getImage()->getProperties()->copyValues(&abSettings->Roughness);
    grungeImageWidget     ->getImage()->getProperties()->copyValues(&abSettings->Grunge);

    qDebug() << diffuseImageWidget->getImage()->getProperties()->suffix;

    // Update general settings.
    if(bFirstTime)
    {
        this->resize(abSettings->d_win_w,abSettings->d_win_h);
        ui->tabWidget->resize(abSettings->tab_win_w,abSettings->tab_win_h);
    }

    showHideTextureTypes(true);

    ui->lineEditPostfixDiffuse  ->setText(diffuseImageWidget->getImage()->getProperties()->suffix);
    ui->lineEditPostfixNormal   ->setText(normalImageWidget->getImage()->getProperties()->suffix);
    ui->lineEditPostfixSpecular ->setText(specularImageWidget->getImage()->getProperties()->suffix);
    ui->lineEditPostfixHeight   ->setText(heightImageWidget->getImage()->getProperties()->suffix);
    ui->lineEditPostfixOcclusion->setText(occlusionImageWidget->getImage()->getProperties()->suffix);
    ui->lineEditPostfixRoughness->setText(roughnessImageWidget->getImage()->getProperties()->suffix);
    ui->lineEditPostfixMetallic ->setText(metallicImageWidget->getImage()->getProperties()->suffix);

    recentDir     = abSettings->recent_dir;
    recentMeshDir = abSettings->recent_mesh_dir;

    ui->checkBoxUseLinearTextureInterpolation->setChecked(abSettings->use_texture_interpolation);
    Image::bUseLinearInterpolation = ui->checkBoxUseLinearTextureInterpolation->isChecked();
    ui->comboBoxGUIStyle->setCurrentText(abSettings->gui_style);

    // UV Settings.
    ui->comboBoxSeamlessMode->setCurrentIndex(abSettings->uv_tiling_type);
    selectSeamlessMode(ui->comboBoxSeamlessMode->currentIndex());
    ui->horizontalSliderMakeSeamlessRadius->setValue(abSettings->uv_tiling_radius);
    ui->radioButtonMirrorModeX->setChecked(abSettings->uv_tiling_mirror_x);
    ui->radioButtonMirrorModeY->setChecked(abSettings->uv_tiling_mirror_y);
    ui->radioButtonMirrorModeXY->setChecked(abSettings->uv_tiling_mirror_xy);
    ui->horizontalSliderRandomPatchesInnerRadius->setValue(abSettings->uv_tiling_random_inner_radius);
    ui->horizontalSliderRandomPatchesOuterRadius->setValue(abSettings->uv_tiling_random_outer_radius);
    ui->horizontalSliderRandomPatchesRotate->setValue(abSettings->uv_tiling_random_rotate);

    ui->radioButtonSeamlessSimpleDirXY->setChecked(abSettings->uv_tiling_simple_dir_xy);
    ui->radioButtonSeamlessSimpleDirX->setChecked(abSettings->uv_tiling_simple_dir_x);
    ui->radioButtonSeamlessSimpleDirY->setChecked(abSettings->uv_tiling_simple_dir_y);

    ui->checkBoxUVTranslationsFirst->setChecked(abSettings->uv_translations_first);
    ui->horizontalSliderSeamlessContrastStrenght->setValue(abSettings->uv_contrast_strength*100);
    ui->horizontalSliderSeamlessContrastPower->setValue(abSettings->uv_contrast_power*100);
    ui->comboBoxSeamlessContrastInputImage->setCurrentIndex(abSettings->uv_contrast_input_image);

    // Other settings.
    ui->spinBoxMouseSensitivity->setValue(abSettings->mouse_sensitivity);
    ui->spinBoxFontSize->setValue(abSettings->font_size);
    ui->checkBoxToggleMouseLoop->setChecked(abSettings->mouse_loop);

    updateSliders();

    dock3Dsettings->loadSettings(abSettings);

    heightImageWidget->reloadSettings();

    ImageWidget::loadingImages = false;

    replotAllImages();

    openGL2DImageWidget ->repaint();
    openGL3DImageWidget->repaint();
    bFirstTime = false;
}

void MainWindow::about()
{
    QMessageBox::about(this, tr(AWESOME_BUMP_VERSION),
                       tr("AwesomeBump is an open source program designed to generate normal, "
                          "height, specular or ambient occlusion, roughness and metallic textures from a single image. "
                          "Since the image processing is done in 99% on GPU  the program runs very fast "
                          "and all the parameters can be changed in real time.\n "
                          "Program written by: \n Krzysztof Kolasinski and Pawel Piecuch (Copyright 2015-2016) with collaboration \n"
                          "with other people! See project collaborators list on github. "));
}

void MainWindow::aboutQt()
{
    QMessageBox::aboutQt(this, tr(AWESOME_BUMP_VERSION));
}

/*
if(!checkOpenGL()){
    AllAboutDialog msgBox;
    msgBox.setPixmap(":/resources/icons/icon-off.png");
    msgBox.setText("Fatal Error!");
    msgBox.setInformativeText(QString("Sorry but it seems that your graphics card does not support openGL %1.%2.\n"
                                      "Program will not run :(\n"
                                      "See " AB_LOG " file for more info.").arg(GL_MAJOR).arg(GL_MINOR));
    msgBox.setVersionText(AWESOME_BUMP_VERSION);
    msgBox.show();

    return app.exec();
}
*/

/*
void displayOpenGLInformation(bool includeExtensions)
{
    qDebug() << Q_FUNC_INFO;

    qDebug() << QString("OpenGL version: %1.%2")
                .arg(context()->format().majorVersion())
                .arg(context()->format().minorVersion());

    qDebug() << "GL version:"
             << (const char*)glGetString(GL_VERSION);
    qDebug() << "GL vendor:"
             << (const char*)glGetString(GL_VENDOR);
    qDebug() << "GL renderer:"
             << (const char*)glGetString(GL_RENDERER);
    qDebug() << "GLSL version:"
             << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

    if (!includeExtensions) return;

    GLint numberOfExentsions = 0;
    GLCHK( glGetIntegerv(GL_NUM_EXTENSIONS, &numberOfExentsions) );
    qDebug() << numberOfExentsions << " extensions:";
    for (int i = 0; i < numberOfExentsions; i++)
    {
        qDebug() << (const char*)glGetStringi(GL_EXTENSIONS, i);
    }
}
*/

void customMessageHandler(
        QtMsgType type,
        const QMessageLogContext& context,
        const QString& msg)
{
    Q_UNUSED(context);

    QString dateTimeString = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString errorMessage = QString("[%1] ").arg(dateTimeString);

    switch (type)
    {
    case QtDebugMsg:
        errorMessage += QString("{Debug} \t\t %1").arg(msg);
        break;
    case QtWarningMsg:
        errorMessage += QString("{Warning} \t %1").arg(msg);
        break;
    case QtCriticalMsg:
        errorMessage += QString("{Critical} \t %1").arg(msg);
        break;
    case QtFatalMsg:
        errorMessage += QString("{Fatal} \t\t %1").arg(msg);
        abort();
        break;
    case QtInfoMsg:
        break;
    }

    // Avoid recursive calling.
    QFile logFile(AB_LOG);
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        // Try any safe location.
        QString glob = QString("%1/%2")
                .arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation))
                .arg(QFileInfo(AB_LOG).fileName());
        logFile.setFileName(glob);
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append))
            return;
    }

    QTextStream logFileStream(&logFile);
    logFileStream << errorMessage << endl;
}
