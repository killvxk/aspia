//
// Aspia Project
// Copyright (C) 2018 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#include "client/ui/desktop_panel.h"

#include <QMenu>
#include <QToolButton>

#include "base/logging.h"
#include "client/ui/key_sequence_dialog.h"
#include "client/ui/select_screen_action.h"

namespace aspia {

DesktopPanel::DesktopPanel(proto::SessionType session_type, QWidget* parent)
    : QFrame(parent)
{
    ui.setupUi(this);

    connect(ui.action_settings, &QAction::triggered, this, &DesktopPanel::settingsButton);
    connect(ui.action_autosize, &QAction::triggered, this, &DesktopPanel::onAutosizeButton);
    connect(ui.action_fullscreen, &QAction::triggered, this, &DesktopPanel::onFullscreenButton);
    connect(ui.action_autoscroll, &QAction::triggered, this, &DesktopPanel::autoScrollChanged);

    createScreensMenu();
    createAdditionalMenu(session_type);

    if (session_type == proto::SESSION_TYPE_DESKTOP_MANAGE)
    {
        connect(ui.action_cad, &QAction::triggered, this, &DesktopPanel::onCtrlAltDel);
    }
    else
    {
        DCHECK(session_type == proto::SESSION_TYPE_DESKTOP_VIEW);
        ui.action_cad->setVisible(false);
    }

    connect(ui.action_file_transfer, &QAction::triggered, [this]()
    {
        emit startSession(proto::SESSION_TYPE_FILE_TRANSFER);
    });

    ui.frame->hide();
    adjustSize();

    hide_timer_id_ = startTimer(std::chrono::seconds(1));
}

void DesktopPanel::setScreenList(const proto::desktop::ScreenList& screen_list)
{
    delete screens_group_;

    screens_group_ = new QActionGroup(this);

    connect(screens_group_, &QActionGroup::triggered, [this](QAction* action)
    {
        SelectScreenAction* screen_action = dynamic_cast<SelectScreenAction*>(action);
        if (!screen_action)
            return;

        emit screenSelected(screen_action->screen());
    });

    SelectScreenAction* full_desktop_action = new SelectScreenAction(screens_group_);

    screens_group_->addAction(full_desktop_action);
    screens_menu_->addAction(full_desktop_action);

    for (int i = 0; i < screen_list.screen_size(); ++i)
    {
        SelectScreenAction* action = new SelectScreenAction(screen_list.screen(i), screens_group_);

        screens_group_->addAction(action);
        screens_menu_->addAction(action);
    }

    ui.action_monitors->setEnabled(true);
}

void DesktopPanel::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == hide_timer_id_)
    {
        killTimer(hide_timer_id_);
        hide_timer_id_ = 0;

        ui.frame->setFixedWidth(ui.toolbar->width());

        ui.toolbar->hide();
        ui.frame->show();

        adjustSize();
        return;
    }

    QFrame::timerEvent(event);
}

void DesktopPanel::enterEvent(QEvent* event)
{
    leaved_ = false;

    if (allow_hide_)
    {
        if (hide_timer_id_)
        {
            killTimer(hide_timer_id_);
            hide_timer_id_ = 0;
        }

        ui.toolbar->show();
        ui.frame->hide();

        adjustSize();
    }

    QFrame::enterEvent(event);
}

void DesktopPanel::leaveEvent(QEvent* event)
{
    leaved_ = true;

    if (allow_hide_)
        delayedHide();

    QFrame::leaveEvent(event);
}

void DesktopPanel::onFullscreenButton(bool checked)
{
    if (checked)
    {
        ui.action_fullscreen->setIcon(
            QIcon(QStringLiteral(":/icon/application-resize-actual.png")));
    }
    else
    {
        ui.action_fullscreen->setIcon(
            QIcon(QStringLiteral(":/icon/application-resize-full.png")));
    }

    emit switchToFullscreen(checked);
}

void DesktopPanel::onAutosizeButton()
{
    if (ui.action_fullscreen->isChecked())
    {
        ui.action_fullscreen->setIcon(
            QIcon(QStringLiteral(":/icon/application-resize-full.png")));
        ui.action_fullscreen->setChecked(false);
    }

    emit switchToAutosize();
}

void DesktopPanel::onCtrlAltDel()
{
    emit keySequence(Qt::ControlModifier | Qt::AltModifier | Qt::Key_Delete);
}

void DesktopPanel::onKeySequence()
{
    QKeySequence key_sequence = KeySequenceDialog::keySequence(this);

    for (int i = 0; i < key_sequence.count(); ++i)
        emit keySequence(key_sequence[i]);
}

void DesktopPanel::createAdditionalMenu(proto::SessionType session_type)
{
    // Create a menu and add actions to it.
    additional_menu_ = new QMenu(this);
    additional_menu_->addAction(ui.action_scaling);
    additional_menu_->addAction(ui.action_autoscroll);
    additional_menu_->addSeparator();

    if (session_type == proto::SESSION_TYPE_DESKTOP_MANAGE)
        additional_menu_->addAction(ui.action_key_sequence);

    additional_menu_->addAction(ui.action_screenshot);

    // Set the menu for the button on the toolbar.
    ui.action_menu->setMenu(additional_menu_);

    QToolButton* button = qobject_cast<QToolButton*>(ui.toolbar->widgetForAction(ui.action_menu));
    button->setPopupMode(QToolButton::InstantPopup);

    // Now we connect all the necessary signals and slots.
    if (session_type == proto::SESSION_TYPE_DESKTOP_MANAGE)
        connect(ui.action_key_sequence, &QAction::triggered, this, &DesktopPanel::onKeySequence);

    connect(ui.action_scaling, &QAction::toggled, [this](bool checked)
    {
        ui.action_autoscroll->setEnabled(!checked);
        emit scalingChanged(checked);
    });

    connect(ui.action_screenshot, &QAction::triggered, this, &DesktopPanel::takeScreenshot);
    connect(additional_menu_, &QMenu::aboutToShow, [this]() { allow_hide_ = false; });
    connect(additional_menu_, &QMenu::aboutToHide, [this]()
    {
        allow_hide_ = true;

        if (leaved_)
            delayedHide();
    });
}

void DesktopPanel::createScreensMenu()
{
    screens_menu_ = new QMenu(this);
    screens_group_ = new QActionGroup(this);

    SelectScreenAction* full_screen_action = new SelectScreenAction(screens_group_);
    screens_group_->addAction(full_screen_action);
    screens_menu_->addAction(full_screen_action);

    ui.action_monitors->setMenu(screens_menu_);

    QToolButton* button = qobject_cast<QToolButton*>(ui.toolbar->widgetForAction(ui.action_monitors));
    button->setPopupMode(QToolButton::InstantPopup);

    connect(screens_menu_, &QMenu::aboutToShow, [this]() { allow_hide_ = false; });
    connect(screens_menu_, &QMenu::aboutToHide, [this]()
    {
        allow_hide_ = true;

        if (leaved_)
            delayedHide();
    });
}

void DesktopPanel::delayedHide()
{
    if (!ui.action_pin->isChecked() && !hide_timer_id_)
        hide_timer_id_ = startTimer(std::chrono::seconds(1));
}

} // namespace aspia
