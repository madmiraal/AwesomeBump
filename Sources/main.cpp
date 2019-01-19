/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QApplication>
#include <QMessageBox>
#include <QGLFormat>
#include <QSurfaceFormat>
#include <QtDebug>
#include <QDateTime>
#include <QStandardPaths>
#include <QDesktopWidget>
#include <QStyleFactory>

#include "mainwindow.h"
#include "splashscreen.h"
#include "allaboutdialog.h"
#include "qopenglerrorcheck.h"
#include "display3dsettings.h"
//#include "CommonObjects.h"

#ifdef USE_OPENGL_330
#define GL_MAJOR 3
#define GL_MINOR 3
#else
#define GL_MAJOR 4
#define GL_MINOR 1
#endif

#define SplashImage ":/resources/logo/splash.png"

// Find data directory for each platform.
QString _find_data_dir(const QString& resource)
{
    if (resource.startsWith(":"))
        return resource;

    QString fpath = QApplication::applicationDirPath();
#if defined(Q_OS_MAC)
    fpath += "/../../../"+resource;
#elif defined(Q_OS_WIN32)
    fpath = resource;
#else
    fpath = resource;
#endif

    return fpath;
}

// Redirect qDebug() to file log file.
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);

    QString dt = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString txt = QString("[%1] ").arg(dt);

    switch (type)
    {
    case QtDebugMsg:
        txt += QString("{Debug} \t\t %1").arg(msg);
        break;
    case QtWarningMsg:
        txt += QString("{Warning} \t %1").arg(msg);
        break;
    case QtCriticalMsg:
        txt += QString("{Critical} \t %1").arg(msg);
        break;
    case QtFatalMsg:
        txt += QString("{Fatal} \t\t %1").arg(msg);
        abort();
        break;
    case QtInfoMsg:
        break;
    }

    // Avoid recursive calling.
    QFile outFile(AB_LOG);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        // Try any safe location.
        QString glob = QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).arg(QFileInfo(AB_LOG).fileName());
        outFile.setFileName(glob);
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Append))
            return;
    }

    QTextStream textStream(&outFile);
    textStream << txt << endl;
}

bool checkOpenGL(){

    QGLWidget *glWidget = new QGLWidget;

    QGLContext* glContext = (QGLContext *) glWidget->context();
    GLCHK( glContext->makeCurrent() );

    int glMajorVersion, glMinorVersion;

    glMajorVersion = glContext->format().majorVersion();
    glMinorVersion = glContext->format().minorVersion();

    qDebug() << "Running the " + QString(AWESOME_BUMP_VERSION);
    qDebug() << "Checking OpenGL widget:";
    qDebug() << "Widget OpenGL:" << QString("%1.%2").arg(glMajorVersion).arg(glMinorVersion);
    qDebug() << "Context valid:" << glContext->isValid() ;
    qDebug() << "OpenGL information:" ;
    qDebug() << "VENDOR:"       << (const char*)glGetString(GL_VENDOR) ;
    qDebug() << "RENDERER:"     << (const char*)glGetString(GL_RENDERER) ;
    qDebug() << "VERSION:"      << (const char*)glGetString(GL_VERSION) ;
    qDebug() << "GLSL VERSION:" << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) ;

    Display3DSettings::openGLVersion = GL_MAJOR + (GL_MINOR * 0.1);

    delete glWidget;

    // Check openGL version.
    if( glMajorVersion < GL_MAJOR || (glMajorVersion == GL_MAJOR && glMinorVersion < GL_MINOR))
    {
        qWarning() << QString("Error: This version of AwesomeBump does not support openGL versions lower than %1.%2 :(").arg(GL_MAJOR).arg(GL_MINOR) ;
        return false;
    }
    return true;
}

// Register delegates.
extern void regABSliderDelegates();
extern void regABColorDelegates();

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    regABSliderDelegates();
    regABColorDelegates();

    qInstallMessageHandler(customMessageHandler);

    qDebug() << "Starting application:";
    qDebug() << "Application dir:" << QApplication::applicationDirPath();
    qDebug() << "Data dir:" << _find_data_dir(RESOURCE_BASE);

    SplashScreen sp(&app);
    QPixmap szpx = QPixmap(SplashImage);
    QSize sz = szpx.size() * float(QApplication::desktop()->screenGeometry().width()) / 4.0 / float(szpx.size().width()); // 1/4 of the screen
    sp.resize(sz);
    sp.setPixmap(szpx.scaled(sz));
    sp.setMessage(VERSION_STRING "|Starting ...");
    sp.show(); app.processEvents();

    // Check for resource directory.
    QString resDir = _find_data_dir(RESOURCE_BASE);
    if (!QFileInfo(resDir+"Configs").isDir() || !QFileInfo(resDir+"Core").isDir()) {
#ifdef Q_OS_MAC
        return QMessageBox::critical(0, "Missing runtime files", QString("Missing runtime files\n\nCannot find runtime assets required to run the application (resource path: %1).").arg(resDir));
#else
        return QMessageBox::critical(0, "Missing runtime files", QString("Cannot find runtime assets required to run the application (resource path: %1).").arg(resDir));
#endif
    }

    // Choose proper GUI style from config.ini file.
    QSettings settings("config.ini", QSettings::IniFormat);
    QString guiStyle = settings.value("gui_style").toString();
    if (!guiStyle.isEmpty())
        app.setStyle(QStyleFactory::create( guiStyle ));

    // Customize some elements.
    app.setStyleSheet("QGroupBox { font-weight: bold; } ");

    // Load specific settings for GUI same for every preset.
    QSettings gui_settings("Configs/gui.ini", QSettings::IniFormat);
    QPalette palette;
    palette.setColor(QPalette::Shadow,QColor(gui_settings.value("slider_font_color","#000000").toString()));
    app.setPalette(palette);

    QFont font;
    font.setFamily(font.defaultFamily());
    font.setPixelSize(gui_settings.value("font_size",10).toInt());
    app.setFont(font);

    // Removing old log file.
    QFile::remove(AB_LOG);

    // Setup default context attributes.
    QGLFormat glFormat(QGL::SampleBuffers); // deprecated
    QSurfaceFormat format;

    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);

#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
    /*
     * Commenting out the next line because it causes rendering to fail.  QGLFormat::CoreProfile
     * disables all OpenGL functions that are depreciated as of OpenGL 3.0.  This fix is a workaround.
     * The full solution is to replace all depreciated OpenGL functions with their current implements.
    */
# if defined(Q_OS_MAC)
    glFormat.setProfile( QGLFormat::CoreProfile );
    format.setProfile( QSurfaceFormat::CoreProfile );
# endif
    glFormat.setVersion( GL_MAJOR, GL_MINOR );
    format.setVersion( GL_MAJOR, GL_MINOR );
#endif

    QGLFormat::setDefaultFormat(glFormat);
    QSurfaceFormat::setDefaultFormat(format);

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
    }else{
        MainWindow window;
        QObject::connect(&window,SIGNAL(initProgress(int)),&sp,SLOT(setProgress(int)));
        QObject::connect(&window,SIGNAL(initMessage(const QString&)),&sp,SLOT(setMessage(const QString&)));
        window.initializeApp();
        window.setWindowTitle(AWESOME_BUMP_VERSION);
        window.resize(window.sizeHint());
        int desktopArea = QApplication::desktop()->width() *
                QApplication::desktop()->height();
        int widgetArea = window.width() * window.height();
        if (((float)widgetArea / (float)desktopArea) < 0.75f)
            window.show();
        else
            window.showMaximized();
        sp.finish(&window);

        return app.exec();
    }
}
