/*
 *  Copyright (c) 2017 Boudewijn Rempt <boud@valdyas.org>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "TouchDockerDock.h"

#include <QtQuickWidgets/QQuickWidget>
#include <QQmlEngine>
#include <QQmlContext>
#include <QAction>
#include <QUrl>
#include <QAction>
#include <QKeyEvent>
#include <QApplication>

#include <klocalizedstring.h>
#include <kactioncollection.h>
#include <ksharedconfig.h>
#include <kconfiggroup.h>

#include <kis_action_registry.h>
#include <KoDialog.h>
#include <KoResourcePaths.h>
#include <kis_icon.h>
#include <KoCanvasBase.h>
#include <KisViewManager.h>
#include <kis_canvas2.h>
#include <KisMainWindow.h>
#include <kis_config.h>
#include <KisPart.h>
#include <KisDocument.h>
#include <KisMimeDatabase.h>
#include <kis_action_manager.h>
#include <kis_action.h>
#include <kis_config.h>
#include <kis_canvas_resource_provider.h>
#include <Theme.h>
#include <Settings.h>
#include <DocumentManager.h>
#include <KisSketchView.h>
#include <brushengine/kis_paintop_preset.h>
#include <kis_paintop_box.h>
#include <kis_resource_server_provider.h>
#include <brushengine/kis_paintop_preset.h>

class TouchDockerDock::Private
{
public:
    Private()
    {
    }

    TouchDockerDock *q;
    bool allowClose {true};
    KisSketchView *sketchView {0};
    QString currentSketchPage;
    KoDialog *openDialog {0};
    KoDialog *saveAsDialog {0};

    QMap<QString, QString> buttonMapping;

    bool shiftOn {false};
    bool ctrlOn {false};
    bool altOn {false};
};


TouchDockerDock::TouchDockerDock()
    : QDockWidget(i18n("Touch Docker"))
    , d(new Private())
{

    QStringList defaultMapping = QStringList() << "decrease_opacity"
                                               << "increase_opacity"
                                               << "make_brush_color_lighter"
                                               << "make_brush_color_darker"
                                               << "decrease_brush_size"
                                               << "increase_brush_size"
                                               << "previous_preset"
                                               << "clear";

    QStringList mapping = KisConfig().readEntry<QString>("touchdockermapping", defaultMapping.join(',')).split(',');
    for (int i = 0; i < 8; ++i) {
        if (i < mapping.size()) {
            d->buttonMapping[QString("button%1").arg(i + 1)] = mapping[i];
        }
        else if (i < defaultMapping.size()) {
            d->buttonMapping[QString("button%1").arg(i + 1)] = defaultMapping[i];
        }
    }

    m_quickWidget = new QQuickWidget(this);
    setWidget(m_quickWidget);
    setEnabled(true);
    m_quickWidget->engine()->rootContext()->setContextProperty("mainWindow", this);

    m_quickWidget->engine()->addImportPath(KoResourcePaths::getApplicationRoot() + "/lib/qml/");
    m_quickWidget->engine()->addImportPath(KoResourcePaths::getApplicationRoot() + "/lib64/qml/");

    m_quickWidget->engine()->addPluginPath(KoResourcePaths::getApplicationRoot() + "/lib/qml/");
    m_quickWidget->engine()->addPluginPath(KoResourcePaths::getApplicationRoot() + "/lib64/qml/");

    Settings *settings = new Settings(this);
    DocumentManager::instance()->setSettingsManager(settings);
    m_quickWidget->engine()->rootContext()->setContextProperty("Settings", settings);

    Theme *theme = Theme::load(KSharedConfig::openConfig()->group("General").readEntry<QString>("theme", "default"),
                               m_quickWidget->engine());
    if (theme) {
        settings->setTheme(theme);
    }


    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->setSource(QUrl(KisConfig().readEntry<QString>("touchstrip", "qrc:/touchstrip.qml")));
}

TouchDockerDock::~TouchDockerDock()
{
}

bool TouchDockerDock::allowClose() const
{
    return d->allowClose;
}

void TouchDockerDock::setAllowClose(bool allow)
{
    d->allowClose = allow;
}

QString TouchDockerDock::currentSketchPage() const
{
    return d->currentSketchPage;
}

void TouchDockerDock::setCurrentSketchPage(QString newPage)
{
    d->currentSketchPage = newPage;
    emit currentSketchPageChanged();
}

void TouchDockerDock::closeEvent(QCloseEvent* event)
{
    if (!d->allowClose) {
        event->ignore();
        emit closeRequested();
    } else {
        event->accept();
    }
}

void TouchDockerDock::slotButtonPressed(const QString &id)
{
    if (id == "fileOpenButton") {
        showFileOpenDialog();
    }
    else if (id == "fileSaveButton" && m_canvas && m_canvas->viewManager() && m_canvas->viewManager()->document()) {
        bool batchMode = m_canvas->viewManager()->document()->fileBatchMode();
        m_canvas->viewManager()->document()->setFileBatchMode(true);
        m_canvas->viewManager()->document()->save(true, 0);
        m_canvas->viewManager()->document()->setFileBatchMode(batchMode);
    }
    else if (id == "fileSaveAsButton" && m_canvas && m_canvas->viewManager() && m_canvas->viewManager()->document()) {
        showFileSaveAsDialog();
    }
    else if (id == "brushSmall") {
        qDebug() << id;
        if (m_canvas) {
            m_canvas->viewManager()->resourceProvider()->setSize(50);
        }
    }
    else if (id == "brushMedium") {
        if (m_canvas) {
            m_canvas->viewManager()->resourceProvider()->setSize(250);
        }
    }
    else if (id == "brushLarge") {
        if (m_canvas) {
            m_canvas->viewManager()->resourceProvider()->setSize(750);
        }
    }
    else if (id == "preset1") {
        KisPaintOpPresetResourceServer * rserver = KisResourceServerProvider::instance()->paintOpPresetServer(false);
        KisPaintOpPresetSP preset = rserver->resourceByName("Basic_tip_default");
        if (m_canvas && preset) {
            m_canvas->viewManager()->paintOpBox()->restoreResource(preset.data());
        }

    }
    else if (id == "preset2") {
        KisPaintOpPresetResourceServer * rserver = KisResourceServerProvider::instance()->paintOpPresetServer(false);
        KisPaintOpPresetSP preset = rserver->resourceByName("Smudge_soft");
        if (m_canvas && preset) {
            m_canvas->viewManager()->paintOpBox()->restoreResource(preset.data());
        }
    }
    else if (id == "white") {
        QColor c(Qt::white);
        KoColor kc(c, KoColorSpaceRegistry::instance()->rgb8());
        if (m_canvas) {
            m_canvas->viewManager()->resourceProvider()->setFGColor(kc);
        }
    }
    else if (id == "black") {
        QColor c(Qt::black);
        KoColor kc(c, KoColorSpaceRegistry::instance()->rgb8());
        if (m_canvas) {
            m_canvas->viewManager()->resourceProvider()->setFGColor(kc);
        }
    }
    else if (id == "blue") {
        QColor c(Qt::blue);
        KoColor kc(c, KoColorSpaceRegistry::instance()->rgb8());
        if (m_canvas) {
            m_canvas->viewManager()->resourceProvider()->setFGColor(kc);
        }
    }
    else if (id == "green") {
        QColor c(Qt::green);
        KoColor kc(c, KoColorSpaceRegistry::instance()->rgb8());
        if (m_canvas) {
            m_canvas->viewManager()->resourceProvider()->setFGColor(kc);
        }
    }
    else if (id == "red") {
        QColor c(Qt::red);
        KoColor kc(c, KoColorSpaceRegistry::instance()->rgb8());
        if (m_canvas) {
            m_canvas->viewManager()->resourceProvider()->setFGColor(kc);
        }

    }
    else if (id == "yellow") {
        QColor c(Qt::yellow);
        KoColor kc(c, KoColorSpaceRegistry::instance()->rgb8());
        if (m_canvas) {
            m_canvas->viewManager()->resourceProvider()->setFGColor(kc);
        }
    }
    else if (id == "grey") {
        QColor c(Qt::gray);
        KoColor kc(c, KoColorSpaceRegistry::instance()->rgb8());
        if (m_canvas) {
            m_canvas->viewManager()->resourceProvider()->setFGColor(kc);
        }

    }
    else if (id == "close") {
        if (m_canvas) {
            m_canvas->viewManager()->document()->setModified(false);
        }
        KisPart::instance()->mainWindows().first()->slotFileQuit();
    }
    else {
        QAction *a = action(id);
        qDebug() << a << id;
        if (a) {
            if (a->isCheckable()) {
                a->toggle();
            }
            else {
                a->trigger();
            }
        }

        else if (id == "shift") {
            // set shift state for the next pointer event, somehow
            QKeyEvent event(d->shiftOn ? QEvent::KeyRelease : QEvent::KeyPress,
                            0,
                            Qt::ShiftModifier);
            QApplication::sendEvent(KisPart::instance()->currentMainwindow(), &event);
            d->shiftOn == !d->shiftOn;
        }
        else if (id == "ctrl") {
            // set ctrl state for the next pointer event, somehow
            QKeyEvent event(d->ctrlOn ? QEvent::KeyRelease : QEvent::KeyPress,
                            0,
                            Qt::ControlModifier);
            QApplication::sendEvent(KisPart::instance()->currentMainwindow(), &event);
            d->ctrlOn == !d->ctrlOn;
        }
        else if (id == "alt") {
            // set alt state for the next pointer event, somehow
            QKeyEvent event(d->altOn ? QEvent::KeyRelease : QEvent::KeyPress,
                            0,
                            Qt::AltModifier);
            QApplication::sendEvent(KisPart::instance()->currentMainwindow(), &event);
            d->altOn == !d->altOn;

        }
    }
}

void TouchDockerDock::slotOpenImage(QString path)
{
    if (d->openDialog) {
        d->openDialog->accept();
    }
    KisPart::instance()->currentMainwindow()->openDocument(QUrl::fromLocalFile(path), KisMainWindow::None);
}

void TouchDockerDock::slotSaveAs(QString path, QString mime)
{
    if (d->saveAsDialog) {
        d->saveAsDialog->accept();
    }
    m_canvas->viewManager()->document()->saveAs(QUrl::fromLocalFile(path), mime.toLatin1(), true);
    m_canvas->viewManager()->document()->waitForSavingToComplete();
}

void TouchDockerDock::hideFileOpenDialog()
{
    if (d->openDialog) {
        d->openDialog->accept();
    }
}

void TouchDockerDock::hideFileSaveAsDialog()
{
    if (d->saveAsDialog) {
        d->saveAsDialog->accept();
    }
}

QString TouchDockerDock::imageForButton(QString id)
{
    if (d->buttonMapping.contains(id)) {
        id = d->buttonMapping[id];
    }
    if (KisActionRegistry::instance()->hasAction(id)) {
        QString a = KisActionRegistry::instance()->getActionProperty(id, "icon");
        if (!a.isEmpty()) {
            return "image://icon/" + a;
        }
    }
    return QString();
}

QString TouchDockerDock::textForButton(QString id)
{
    if (d->buttonMapping.contains(id)) {
        id = d->buttonMapping[id];
    }
    if (KisActionRegistry::instance()->hasAction(id)) {
        QString a = KisActionRegistry::instance()->getActionProperty(id, "iconText");
        if (a.isEmpty()) {
            a = KisActionRegistry::instance()->getActionProperty(id, "text");
        }
        return a;
    }

    return id;
}

QAction *TouchDockerDock::action(QString id) const
{
    if (m_canvas && m_canvas->viewManager()) {
        if (d->buttonMapping.contains(id)) {
            id = d->buttonMapping[id];
        }
        return m_canvas->viewManager()->actionManager()->actionByName(id);
    }
    return 0;
}

void TouchDockerDock::showFileOpenDialog()
{
    if (!d->openDialog) {
        d->openDialog = createDialog("qrc:/opendialog.qml");
    }

    d->openDialog->exec();
}

void TouchDockerDock::showFileSaveAsDialog()
{
    if (!d->openDialog) {
        d->openDialog = createDialog("qrc:/saveasdialog.qml");
    }
    d->openDialog->exec();
}

KoDialog *TouchDockerDock::createDialog(const QString qml)
{
    KoDialog *dlg = new KoDialog(this);
    dlg->setButtons(KoDialog::None);

    QQuickWidget *quickWidget = new QQuickWidget(this);
    dlg->setMainWidget(quickWidget);

    setEnabled(true);
    quickWidget->engine()->rootContext()->setContextProperty("mainWindow", this);

    quickWidget->engine()->addImportPath(KoResourcePaths::getApplicationRoot() + "/lib/qml/");
    quickWidget->engine()->addImportPath(KoResourcePaths::getApplicationRoot() + "/lib64/qml/");

    quickWidget->engine()->addPluginPath(KoResourcePaths::getApplicationRoot() + "/lib/qml/");
    quickWidget->engine()->addPluginPath(KoResourcePaths::getApplicationRoot() + "/lib64/qml/");


    Settings *settings = new Settings(this);
    DocumentManager::instance()->setSettingsManager(settings);
    quickWidget->engine()->rootContext()->setContextProperty("Settings", settings);

    Theme *theme = Theme::load(KSharedConfig::openConfig()->group("General").readEntry<QString>("theme", "default"),
                               quickWidget->engine());
    settings->setTheme(theme);

    quickWidget->setSource(QUrl(qml));
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    dlg->setMinimumSize(1280, 768);

    return dlg;
}

QObject *TouchDockerDock::sketchKisView() const
{
    return d->sketchView;
}

void TouchDockerDock::setSketchKisView(QObject* newView)
{
    if (d->sketchView) {
        d->sketchView->disconnect(this);
    }

    if (d->sketchView != newView) {
        d->sketchView = qobject_cast<KisSketchView*>(newView);
        emit sketchKisViewChanged();
    }
}

void TouchDockerDock::setCanvas(KoCanvasBase *canvas)
{
    setEnabled(true);

    if (m_canvas == canvas) {
        return;
    }

    if (m_canvas) {
        m_canvas->disconnectCanvasObserver(this);
    }

    if (!canvas) {
        m_canvas = 0;
        return;
    }

    m_canvas = dynamic_cast<KisCanvas2*>(canvas);
}


void TouchDockerDock::unsetCanvas()
{
    setEnabled(true);
    m_canvas = 0;
}