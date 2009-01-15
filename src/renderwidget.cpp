/***************************************************************************
 *   Copyright (C) 2008 by Jean-Baptiste Mardelle (jb@kdenlive.org)        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/


#include <QDomDocument>
#include <QItemDelegate>
#include <QTreeWidgetItem>
#include <QHeaderView>

#include <KStandardDirs>
#include <KDebug>
#include <KMessageBox>
#include <KComboBox>

#include "kdenlivesettings.h"
#include "renderwidget.h"
#include "ui_saveprofile_ui.h"

const int GroupRole = Qt::UserRole;
const int ExtensionRole = GroupRole + 1;
const int StandardRole = GroupRole + 2;
const int RenderRole = GroupRole + 3;
const int ParamsRole = GroupRole + 4;
const int EditableRole = GroupRole + 5;

RenderWidget::RenderWidget(QWidget * parent): QDialog(parent) {
    m_view.setupUi(this);
    setWindowTitle(i18n("Rendering"));
    m_view.buttonDelete->setIcon(KIcon("trash-empty"));
    m_view.buttonDelete->setToolTip(i18n("Delete profile"));
    m_view.buttonDelete->setEnabled(false);

    m_view.buttonEdit->setIcon(KIcon("document-properties"));
    m_view.buttonEdit->setToolTip(i18n("Edit profile"));
    m_view.buttonEdit->setEnabled(false);

    m_view.buttonSave->setIcon(KIcon("document-new"));
    m_view.buttonSave->setToolTip(i18n("Create new profile"));

    m_view.buttonInfo->setIcon(KIcon("help-about"));

    if (KdenliveSettings::showrenderparams()) {
        m_view.buttonInfo->setDown(true);
    } else m_view.advanced_params->hide();

    m_view.experimentalrender->setChecked(KdenliveSettings::experimentalrender());

    m_view.experimentalrender->setToolTip(i18n("Changing the size of video when rendering\nis not fully supported, you may have problems\nwith some effects or title clips, so the export\nprofiles that resize your video are marked as\nexperimental"));

    connect(m_view.buttonInfo, SIGNAL(clicked()), this, SLOT(showInfoPanel()));

    connect(m_view.buttonSave, SIGNAL(clicked()), this, SLOT(slotSaveProfile()));
    connect(m_view.buttonEdit, SIGNAL(clicked()), this, SLOT(slotEditProfile()));
    connect(m_view.buttonDelete, SIGNAL(clicked()), this, SLOT(slotDeleteProfile()));
    connect(m_view.buttonStart, SIGNAL(clicked()), this, SLOT(slotExport()));
    connect(m_view.abort_job, SIGNAL(clicked()), this, SLOT(slotAbortCurrentJob()));
    connect(m_view.buttonClose, SIGNAL(clicked()), this, SLOT(hide()));
    connect(m_view.buttonClose2, SIGNAL(clicked()), this, SLOT(hide()));
    connect(m_view.out_file, SIGNAL(textChanged(const QString &)), this, SLOT(slotUpdateButtons()));
    connect(m_view.format_list, SIGNAL(currentRowChanged(int)), this, SLOT(refreshView()));
    connect(m_view.size_list, SIGNAL(currentRowChanged(int)), this, SLOT(refreshParams()));

    connect(m_view.render_guide, SIGNAL(clicked(bool)), this, SLOT(slotUpdateGuideBox()));
    connect(m_view.render_zone, SIGNAL(clicked(bool)), this, SLOT(slotUpdateGuideBox()));
    connect(m_view.render_full, SIGNAL(clicked(bool)), this, SLOT(slotUpdateGuideBox()));

    connect(m_view.guide_end, SIGNAL(activated(int)), this, SLOT(slotCheckStartGuidePosition()));
    connect(m_view.guide_start, SIGNAL(activated(int)), this, SLOT(slotCheckEndGuidePosition()));

    connect(m_view.format_selection, SIGNAL(activated(int)), this, SLOT(refreshView()));
    connect(m_view.experimentalrender, SIGNAL(stateChanged(int)), this, SLOT(slotUpdateExperimentalRendering()));

    m_view.buttonStart->setEnabled(false);
    m_view.guides_box->setVisible(false);
    parseProfiles();
    m_view.splitter->setStretchFactor(1, 5);
    m_view.splitter->setStretchFactor(0, 2);

    m_view.out_file->setMode(KFile::File);

    m_view.running_jobs->setItemDelegate(new RenderViewDelegate(this));
    QHeaderView *header = m_view.running_jobs->header();
    QFontMetrics fm = fontMetrics();
    //header->resizeSection(0, fm.width("typical-name-for-a-torrent.torrent"));
    header->setResizeMode(0, QHeaderView::Interactive);
    header->resizeSection(0, fm.width("typical-name-for-a-file.torrent"));
    header->setResizeMode(1, QHeaderView::Fixed);
    header->resizeSection(0, width() * 2 / 3);
    header->setResizeMode(1, QHeaderView::Interactive);
    //header->setResizeMode(1, QHeaderView::Fixed);

    focusFirstVisibleItem();
}

void RenderWidget::slotUpdateExperimentalRendering() {
    KdenliveSettings::setExperimentalrender(m_view.experimentalrender->isChecked());
    refreshView();
}


void RenderWidget::showInfoPanel() {
    if (m_view.advanced_params->isVisible()) {
        m_view.advanced_params->setVisible(false);
        m_view.buttonInfo->setDown(false);
        KdenliveSettings::setShowrenderparams(false);
    } else {
        m_view.advanced_params->setVisible(true);
        m_view.buttonInfo->setDown(true);
        KdenliveSettings::setShowrenderparams(true);
    }
}

void RenderWidget::slotUpdateGuideBox() {
    m_view.guides_box->setVisible(m_view.render_guide->isChecked());
}

void RenderWidget::slotCheckStartGuidePosition() {
    if (m_view.guide_start->currentIndex() > m_view.guide_end->currentIndex())
        m_view.guide_start->setCurrentIndex(m_view.guide_end->currentIndex());
}

void RenderWidget::slotCheckEndGuidePosition() {
    if (m_view.guide_end->currentIndex() < m_view.guide_start->currentIndex())
        m_view.guide_end->setCurrentIndex(m_view.guide_start->currentIndex());
}

void RenderWidget::setGuides(QDomElement guidesxml, double duration) {
    m_view.guide_start->clear();
    m_view.guide_end->clear();
    QDomNodeList nodes = guidesxml.elementsByTagName("guide");
    if (nodes.count() > 0) {
        m_view.guide_start->addItem(i18n("Render"), "0");
        m_view.render_guide->setEnabled(true);
    } else m_view.render_guide->setEnabled(false);
    for (int i = 0; i < nodes.count(); i++) {
        QDomElement e = nodes.item(i).toElement();
        if (!e.isNull()) {
            m_view.guide_start->addItem(e.attribute("comment"), e.attribute("time").toDouble());
            m_view.guide_end->addItem(e.attribute("comment"), e.attribute("time").toDouble());
        }
    }
    if (nodes.count() > 0)
        m_view.guide_end->addItem(i18n("End"), QString::number(duration));
}

void RenderWidget::slotUpdateButtons() {
    if (m_view.out_file->url().isEmpty()) m_view.buttonStart->setEnabled(false);
    else m_view.buttonStart->setEnabled(true);
}

void RenderWidget::slotSaveProfile() {
    Ui::SaveProfile_UI ui;
    QDialog *d = new QDialog(this);
    ui.setupUi(d);
    QString customGroup = i18n("Custom");
    QStringList groupNames;
    for (int i = 0; i < m_view.format_list->count(); i++)
        groupNames.append(m_view.format_list->item(i)->text());
    if (!groupNames.contains(customGroup)) groupNames.prepend(customGroup);
    ui.group_name->addItems(groupNames);
    int pos = ui.group_name->findText(customGroup);
    ui.group_name->setCurrentIndex(pos);

    ui.parameters->setText(m_view.advanced_params->toPlainText());
    ui.extension->setText(m_view.size_list->currentItem()->data(ExtensionRole).toString());
    ui.profile_name->setFocus();
    if (d->exec() == QDialog::Accepted && !ui.profile_name->text().simplified().isEmpty()) {
        QString exportFile = KStandardDirs::locateLocal("appdata", "export/customprofiles.xml");
        QDomDocument doc;
        QFile file(exportFile);
        doc.setContent(&file, false);
        file.close();

        QDomElement documentElement;
        bool groupExists = false;
        QString groupName;
        QString newProfileName = ui.profile_name->text().simplified();
        QString newGroupName = ui.group_name->currentText();
        QDomNodeList groups = doc.elementsByTagName("group");
        int i = 0;
        if (groups.count() == 0) {
            QDomElement profiles = doc.createElement("profiles");
            doc.appendChild(profiles);
        } else while (!groups.item(i).isNull()) {
                documentElement = groups.item(i).toElement();
                groupName = documentElement.attribute("name");
                kDebug() << "// SAVE, PARSING FROUP: " << i << ", name: " << groupName << ", LOOK FR: " << newGroupName;
                if (groupName == newGroupName) {
                    groupExists = true;
                    break;
                }
                i++;
            }
        if (!groupExists) {
            documentElement = doc.createElement("group");
            documentElement.setAttribute("name", ui.group_name->currentText());
            documentElement.setAttribute("renderer", "avformat");
            doc.documentElement().appendChild(documentElement);
        }
        QDomElement profileElement = doc.createElement("profile");
        profileElement.setAttribute("name", newProfileName);
        profileElement.setAttribute("extension", ui.extension->text().simplified());
        profileElement.setAttribute("args", ui.parameters->toPlainText().simplified());
        documentElement.appendChild(profileElement);

        //QCString save = doc.toString().utf8();

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            KMessageBox::sorry(this, i18n("Unable to write to file %1", exportFile));
            delete d;
            return;
        }
        QTextStream out(&file);
        out << doc.toString();
        file.close();
        parseProfiles(newGroupName, newProfileName);
    }
    delete d;
}

void RenderWidget::slotEditProfile() {
    QListWidgetItem *item = m_view.size_list->currentItem();
    if (!item) return;
    QString currentGroup = m_view.format_list->currentItem()->text();

    QString params = item->data(ParamsRole).toString();
    QString extension = item->data(ExtensionRole).toString();
    QString currentProfile = item->text();

    Ui::SaveProfile_UI ui;
    QDialog *d = new QDialog(this);
    ui.setupUi(d);
    QStringList groupNames;
    for (int i = 0; i < m_view.format_list->count(); i++)
        groupNames.append(m_view.format_list->item(i)->text());
    if (!groupNames.contains(currentGroup)) groupNames.prepend(currentGroup);
    ui.group_name->addItems(groupNames);
    int pos = ui.group_name->findText(currentGroup);
    ui.group_name->setCurrentIndex(pos);
    ui.profile_name->setText(currentProfile);
    ui.extension->setText(extension);
    ui.parameters->setText(params);
    ui.profile_name->setFocus();

    if (d->exec() == QDialog::Accepted) {
        slotDeleteProfile();
        QString exportFile = KStandardDirs::locateLocal("appdata", "export/customprofiles.xml");
        QDomDocument doc;
        QFile file(exportFile);
        doc.setContent(&file, false);
        file.close();

        QDomElement documentElement;
        bool groupExists = false;
        QString groupName;
        QString newProfileName = ui.profile_name->text();
        QString newGroupName = ui.group_name->currentText();
        QDomNodeList groups = doc.elementsByTagName("group");
        int i = 0;
        if (groups.count() == 0) {
            QDomElement profiles = doc.createElement("profiles");
            doc.appendChild(profiles);
        } else while (!groups.item(i).isNull()) {
                documentElement = groups.item(i).toElement();
                groupName = documentElement.attribute("name");
                kDebug() << "// SAVE, PARSING FROUP: " << i << ", name: " << groupName << ", LOOK FR: " << newGroupName;
                if (groupName == newGroupName) {
                    groupExists = true;
                    break;
                }
                i++;
            }
        if (!groupExists) {
            documentElement = doc.createElement("group");
            documentElement.setAttribute("name", ui.group_name->currentText());
            documentElement.setAttribute("renderer", "avformat");
            doc.documentElement().appendChild(documentElement);
        }
        QDomElement profileElement = doc.createElement("profile");
        profileElement.setAttribute("name", newProfileName);
        profileElement.setAttribute("extension", ui.extension->text().simplified());
        profileElement.setAttribute("args", ui.parameters->toPlainText().simplified());
        documentElement.appendChild(profileElement);

        //QCString save = doc.toString().utf8();

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            KMessageBox::sorry(this, i18n("Unable to write to file %1", exportFile));
            delete d;
            return;
        }
        QTextStream out(&file);
        out << doc.toString();
        file.close();
        parseProfiles(newGroupName, newProfileName);
    }
    delete d;
}

void RenderWidget::slotDeleteProfile() {
    QString currentGroup = m_view.format_list->currentItem()->text();
    QString currentProfile = m_view.size_list->currentItem()->text();

    QString exportFile = KStandardDirs::locateLocal("appdata", "export/customprofiles.xml");
    QDomDocument doc;
    QFile file(exportFile);
    doc.setContent(&file, false);
    file.close();

    QDomElement documentElement;
    bool groupExists = false;
    QString groupName;
    QDomNodeList groups = doc.elementsByTagName("group");
    int i = 0;

    while (!groups.item(i).isNull()) {
        documentElement = groups.item(i).toElement();
        groupName = documentElement.attribute("name");
        if (groupName == currentGroup) {
            QDomNodeList children = documentElement.childNodes();
            for (int j = 0; j < children.count(); j++) {
                QDomElement pro = children.at(j).toElement();
                if (pro.attribute("name") == currentProfile) {
                    groups.item(i).removeChild(children.at(j));
                    if (groups.item(i).childNodes().isEmpty())
                        doc.documentElement().removeChild(groups.item(i));
                    break;
                }
            }
            break;
        }
        i++;
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        KMessageBox::sorry(this, i18n("Unable to write to file %1", exportFile));
        return;
    }
    QTextStream out(&file);
    out << doc.toString();
    file.close();
    parseProfiles(currentGroup);
    focusFirstVisibleItem();
}

void RenderWidget::updateButtons() {
    if (!m_view.size_list->currentItem() || m_view.size_list->currentItem()->isHidden()) {
        m_view.buttonSave->setEnabled(false);
        m_view.buttonDelete->setEnabled(false);
        m_view.buttonEdit->setEnabled(false);
    } else {
        m_view.buttonSave->setEnabled(true);
        if (m_view.size_list->currentItem()->data(EditableRole).toString().isEmpty()) {
            m_view.buttonDelete->setEnabled(false);
            m_view.buttonEdit->setEnabled(false);
        } else {
            m_view.buttonDelete->setEnabled(true);
            m_view.buttonEdit->setEnabled(true);
        }
    }
}


void RenderWidget::focusFirstVisibleItem() {
    if (m_view.size_list->currentItem() && !m_view.size_list->currentItem()->isHidden()) {
        updateButtons();
        return;
    }
    for (uint ix = 0; ix < m_view.size_list->count(); ix++) {
        QListWidgetItem *item = m_view.size_list->item(ix);
        if (item && !item->isHidden()) {
            m_view.size_list->setCurrentRow(ix);
            break;
        }
    }
    if (!m_view.size_list->currentItem()) m_view.size_list->setCurrentRow(0);
    updateButtons();
}

void RenderWidget::slotExport() {
    QListWidgetItem *item = m_view.size_list->currentItem();
    if (!item) return;
    QFile f(m_view.out_file->url().path());
    if (f.exists()) {
        if (KMessageBox::warningYesNo(this, i18n("File already exists. Do you want to overwrite it ?")) != KMessageBox::Yes)
            return;
    }
    QStringList overlayargs;
    if (m_view.tc_overlay->isChecked()) {
        QString filterFile = KStandardDirs::locate("appdata", "metadata.properties");
        overlayargs << "meta.attr.timecode=1" << "meta.attr.timecode.markup=#timecode";
        overlayargs << "-attach" << "data_feed:attr_check" << "-attach";
        overlayargs << "data_show:" + filterFile << "_fezzik=1" << "dynamic=1";
    }
    double startPos = -1;
    double endPos = -1;
    if (m_view.render_guide->isChecked()) {
        startPos = m_view.guide_start->itemData(m_view.guide_start->currentIndex()).toDouble();
        endPos = m_view.guide_end->itemData(m_view.guide_end->currentIndex()).toDouble();
    }
    QString renderArgs = m_view.advanced_params->toPlainText();
    renderArgs.replace("%width", QString::number(m_profile.width));
    renderArgs.replace("%height", QString::number(m_profile.height));
    renderArgs.replace("%dar", "@" + QString::number(m_profile.display_aspect_num) + "/" + QString::number(m_profile.display_aspect_den));
    if (m_view.force_progressive->checkState() == Qt::Checked) renderArgs.append(" progressive=1");
    else if (m_view.force_progressive->checkState() == Qt::Unchecked) renderArgs.append(" progressive=0");

    // Check if the rendering profile is different from project profile,
    // in which case we need to use the producer_comsumer from MLT
    bool resizeProfile = false;

    QString std = item->data(ParamsRole).toString();
    if (resizeProfile == false && std.contains(" s=")) {
        QString subsize = std.section(" s=", 1, 1);
        subsize = subsize.section(' ', 0, 0).toLower();
        if (subsize != "%widthx%height") {
            const QString currentSize = QString::number(m_profile.width) + 'x' + QString::number(m_profile.height);
            if (subsize != currentSize) resizeProfile = true;
        }
    }

    emit doRender(m_view.out_file->url().path(), item->data(RenderRole).toString(), overlayargs, renderArgs.simplified().split(' '), m_view.render_zone->isChecked(), m_view.play_after->isChecked(), startPos, endPos, resizeProfile);
    m_view.tabWidget->setCurrentIndex(1);
}

void RenderWidget::setProfile(MltVideoProfile profile) {
    m_profile = profile;
    //WARNING: this way to tell the video standard is a bit hackish...
    if (m_profile.description.contains("pal", Qt::CaseInsensitive) || m_profile.description.contains("25", Qt::CaseInsensitive) || m_profile.description.contains("50", Qt::CaseInsensitive)) m_view.format_selection->setCurrentIndex(0);
    else m_view.format_selection->setCurrentIndex(1);
    m_view.force_progressive->setCheckState(Qt::PartiallyChecked);
    refreshView();
}

void RenderWidget::refreshView() {
    m_view.size_list->blockSignals(true);
    QListWidgetItem *item = m_view.format_list->currentItem();
    if (!item) {
        m_view.format_list->setCurrentRow(0);
        item = m_view.format_list->currentItem();
    }
    if (!item) return;
    QString std;
    QString group = item->text();
    QListWidgetItem *sizeItem;
    bool firstSelected = false;
    const QStringList formatsList = KdenliveSettings::supportedformats();
    const QStringList vcodecsList = KdenliveSettings::videocodecs();
    const QStringList acodecsList = KdenliveSettings::audiocodecs();
    for (int i = 0; i < m_view.size_list->count(); i++) {
        sizeItem = m_view.size_list->item(i);
        if (sizeItem->data(GroupRole) == group) {
            std = sizeItem->data(StandardRole).toString();
            if (!std.isEmpty()) {
                if (std.contains("PAL", Qt::CaseInsensitive)) sizeItem->setHidden(m_view.format_selection->currentIndex() != 0);
                else if (std.contains("NTSC", Qt::CaseInsensitive)) sizeItem->setHidden(m_view.format_selection->currentIndex() != 1);
            } else {
                sizeItem->setHidden(false);
                if (!firstSelected) m_view.size_list->setCurrentItem(sizeItem);
                firstSelected = true;
            }
            if (!KdenliveSettings::experimentalrender() && !sizeItem->isHidden()) {
                // hide experimental codecs (which do resize the video)
                std = sizeItem->data(ParamsRole).toString();
                if (std.contains(" s=")) {
                    QString subsize = std.section(" s=", 1, 1);
                    subsize = subsize.section(' ', 0, 0).toLower();
                    if (subsize != "%widthx%height") {
                        const QString currentSize = QString::number(m_profile.width) + 'x' + QString::number(m_profile.height);
                        if (subsize != currentSize) sizeItem->setHidden(true);
                    }
                }
            }
            if (!sizeItem->isHidden()) {
                // Make sure the selected profile uses an installed avformat codec / format
                std = sizeItem->data(ParamsRole).toString();

                if (!formatsList.isEmpty()) {
                    QString format;
                    if (std.startsWith("f=")) format = std.section("f=", 1, 1);
                    else if (std.contains(" f=")) format = std.section(" f=", 1, 1);
                    if (!format.isEmpty()) {
                        format = format.section(' ', 0, 0).toLower();
                        if (!formatsList.contains(format)) {
                            kDebug() << "*****  UNSUPPORTED F: " << format;
                            sizeItem->setHidden(true);
                        }
                    }
                }
                if (!acodecsList.isEmpty() && !sizeItem->isHidden()) {
                    QString format;
                    if (std.startsWith("acodec=")) format = std.section("acodec=", 1, 1);
                    else if (std.contains(" acodec=")) format = std.section(" acodec=", 1, 1);
                    if (!format.isEmpty()) {
                        format = format.section(' ', 0, 0).toLower();
                        if (!acodecsList.contains(format)) {
                            kDebug() << "*****  UNSUPPORTED ACODEC: " << format;
                            sizeItem->setHidden(true);
                        }
                    }
                }
                if (!vcodecsList.isEmpty() && !sizeItem->isHidden()) {
                    QString format;
                    if (std.startsWith("vcodec=")) format = std.section("vcodec=", 1, 1);
                    else if (std.contains(" vcodec=")) format = std.section(" vcodec=", 1, 1);
                    if (!format.isEmpty()) {
                        format = format.section(' ', 0, 0).toLower();
                        if (!vcodecsList.contains(format)) {
                            kDebug() << "*****  UNSUPPORTED VCODEC: " << format;
                            sizeItem->setHidden(true);
                        }
                    }
                }
            }
        } else sizeItem->setHidden(true);
    }
    focusFirstVisibleItem();
    m_view.size_list->blockSignals(false);
    refreshParams();
}

void RenderWidget::refreshParams() {
    QListWidgetItem *item = m_view.size_list->currentItem();
    if (!item || item->isHidden()) {
        m_view.advanced_params->clear();
        m_view.buttonStart->setEnabled(false);
        return;
    }
    QString params = item->data(ParamsRole).toString();
    QString extension = item->data(ExtensionRole).toString();
    m_view.advanced_params->setPlainText(params);
    m_view.advanced_params->setToolTip(params);
    KUrl url = m_view.out_file->url();
    if (!url.isEmpty()) {
        QString path = url.path();
        int pos = path.lastIndexOf('.') + 1;
        if (pos == 0) path.append('.') + extension;
        else path = path.left(pos) + extension;
        m_view.out_file->setUrl(KUrl(path));
    } else {
        m_view.out_file->setUrl(KUrl(QDir::homePath() + "/untitled." + extension));
    }
    m_view.out_file->setFilter("*." + extension);

    if (item->data(EditableRole).toString().isEmpty()) {
        m_view.buttonDelete->setEnabled(false);
        m_view.buttonEdit->setEnabled(false);
    } else {
        m_view.buttonDelete->setEnabled(true);
        m_view.buttonEdit->setEnabled(true);
    }
    m_view.buttonStart->setEnabled(true);
}

void RenderWidget::parseProfiles(QString group, QString profile) {
    m_view.size_list->clear();
    m_view.format_list->clear();
    QString exportFile = KStandardDirs::locate("appdata", "export/profiles.xml");
    parseFile(exportFile, false);
    exportFile = KStandardDirs::locateLocal("appdata", "export/customprofiles.xml");
    if (QFile::exists(exportFile)) parseFile(exportFile, true);
    refreshView();
    QList<QListWidgetItem *> child;
    child = m_view.format_list->findItems(group, Qt::MatchExactly);
    if (!child.isEmpty()) m_view.format_list->setCurrentItem(child.at(0));
    child = m_view.size_list->findItems(profile, Qt::MatchExactly);
    if (!child.isEmpty()) m_view.size_list->setCurrentItem(child.at(0));
}

void RenderWidget::parseFile(QString exportFile, bool editable) {
    QDomDocument doc;
    QFile file(exportFile);
    doc.setContent(&file, false);
    file.close();
    QDomElement documentElement;
    QDomElement profileElement;
    QDomNodeList groups = doc.elementsByTagName("group");

    if (groups.count() == 0) {
        kDebug() << "// Export file: " << exportFile << " IS BROKEN";
        return;
    }

    int i = 0;
    QString groupName;
    QString profileName;
    QString extension;
    QString prof_extension;
    QString renderer;
    QString params;
    QString standard;
    QListWidgetItem *item;
    while (!groups.item(i).isNull()) {
        documentElement = groups.item(i).toElement();
        groupName = documentElement.attribute("name", QString::null);
        extension = documentElement.attribute("extension", QString::null);
        renderer = documentElement.attribute("renderer", QString::null);
        if (m_view.format_list->findItems(groupName, Qt::MatchExactly).isEmpty())
            new QListWidgetItem(groupName, m_view.format_list);

        QDomNode n = groups.item(i).firstChild();
        while (!n.isNull()) {
            profileElement = n.toElement();
            profileName = profileElement.attribute("name");
            standard = profileElement.attribute("standard");
            params = profileElement.attribute("args");
            prof_extension = profileElement.attribute("extension");
            if (!prof_extension.isEmpty()) extension = prof_extension;
            item = new QListWidgetItem(profileName, m_view.size_list);
            item->setData(GroupRole, groupName);
            item->setData(ExtensionRole, extension);
            item->setData(RenderRole, renderer);
            item->setData(StandardRole, standard);
            item->setData(ParamsRole, params);
            if (editable) item->setData(EditableRole, "true");
            n = n.nextSibling();
        }

        i++;
    }
}

void RenderWidget::setRenderJob(const QString &dest, int progress) {
    QList<QTreeWidgetItem *> existing = m_view.running_jobs->findItems(dest, Qt::MatchExactly);
    if (!existing.isEmpty()) {
        if (progress == -1) {
            // Job finished successfully
            existing.at(0)->setIcon(0, KIcon("dialog-ok"));
            existing.at(0)->setData(1, Qt::UserRole, 100);
        } else if (progress == -2) {
            // Rendering crashed
            existing.at(0)->setIcon(0, KIcon("dialog-close"));
            existing.at(0)->setData(1, Qt::UserRole, 0);
        } else if (progress == -3) {
            // User aborted job
            existing.at(0)->setIcon(0, KIcon("dialog-close"));
            existing.at(0)->setData(1, Qt::UserRole, 100);
        } else existing.at(0)->setData(1, Qt::UserRole, progress);
        return;
    }
    QTreeWidgetItem *item = new QTreeWidgetItem(m_view.running_jobs, QStringList() << dest << QString());
    if (progress == -1) {
        // Job finished successfully
        item->setIcon(0, KIcon("dialog-ok"));
        item->setData(1, Qt::UserRole, 100);
    } else if (progress == -2) {
        // Rendering crashed
        item->setIcon(0, KIcon("dialog-close"));
        item->setData(1, Qt::UserRole, 0);
    } else if (progress == -3) {
        // User aborted job
        item->setIcon(0, KIcon("dialog-close"));
        item->setData(1, Qt::UserRole, 100);
    } else item->setData(1, Qt::UserRole, progress);

}

void RenderWidget::slotAbortCurrentJob() {
    QTreeWidgetItem *current = m_view.running_jobs->currentItem();
    if (current) emit abortProcess(current->text(0));
}

#include "renderwidget.moc"
