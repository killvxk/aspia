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

#ifndef ASPIA_HOST__UI__USER_DIALOG_H_
#define ASPIA_HOST__UI__USER_DIALOG_H_

#include "base/macros_magic.h"
#include "protocol/srp_user.pb.h"
#include "ui_user_dialog.h"

namespace aspia {

class UserDialog : public QDialog
{
    Q_OBJECT

public:
    UserDialog(const proto::SrpUserList& user_list, proto::SrpUser* user, QWidget* parent);
    ~UserDialog() = default;

protected:
    // QDialog implementation.
    bool eventFilter(QObject* object, QEvent* event) override;

private slots:
    void onCheckAllButtonPressed();
    void onCheckNoneButtonPressed();
    void onButtonBoxClicked(QAbstractButton* button);

private:
    void setAccountChanged(bool changed);

    Ui::UserDialog ui;

    const proto::SrpUserList& user_list_;
    proto::SrpUser* user_;

    bool account_changed_ = true;

    DISALLOW_COPY_AND_ASSIGN(UserDialog);
};

} // namespace aspia

#endif // ASPIA_HOST__UI__USER_DIALOG_H_
