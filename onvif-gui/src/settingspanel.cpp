/*******************************************************************************
* settingspanel.cpp
*
* Copyright (c) 2022 Stephen Rhodes
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*******************************************************************************/

#ifdef _WIN32
#include <WS2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#endif

#include <QLabel>
#include <QGridLayout>

#include "settingspanel.h"
#include "mainwindow.h"

SettingsPanel::SettingsPanel(QMainWindow* parent)
{
    mainWindow = parent;
    connect(this, SIGNAL(msg(const QString&)), MW, SLOT(msg(const QString&)));

    networkInterfaces = new QComboBox();
    networkInterfaces->setMaximumWidth(180);
    QLabel *lbl03 = new QLabel("Select Network Interface");
    autoDiscovery = new QCheckBox("Auto Discovery");
    multiBroadcast = new QCheckBox("Multi Broadcast");
    broadcastRepeat = new QSpinBox();
    broadcastRepeat->setRange(2, 5);
    QLabel *lbl00 = new QLabel("Broadcast Repeat");
    commonUsername = new QLineEdit();
    commonUsername->setMaximumWidth(100);
    QLabel *lbl01 = new QLabel("Common Username");
    commonPassword = new QLineEdit();
    commonPassword->setMaximumWidth(100);
    QLabel *lbl02 = new QLabel("Common Password");

    QGridLayout *layout = new QGridLayout();
    layout->addWidget(lbl03,               0, 0, 1, 1);
    layout->addWidget(networkInterfaces,   0, 1, 1, 1);
    layout->addWidget(autoDiscovery,       1, 0, 1, 1);
    layout->addWidget(multiBroadcast,      2, 0, 1, 1);
    layout->addWidget(lbl00,               2, 1, 1 ,1);
    layout->addWidget(broadcastRepeat,     2, 2, 1, 1);
    layout->addWidget(lbl01,               3, 0, 1, 1);
    layout->addWidget(commonUsername,      3, 1, 1, 1);
    layout->addWidget(lbl02,               4, 0, 1, 1);
    layout->addWidget(commonPassword,      4, 1, 1, 1);
    setLayout(layout);

    getActiveNetworkInterfaces();

    commonUsername->setText(MW->settings->value(usernameKey, "").toString());
    commonPassword->setText(MW->settings->value(passwordKey, "").toString());
    autoDiscovery->setChecked(MW->settings->value(autoDiscKey, false).toBool());
    multiBroadcast->setChecked(MW->settings->value(multiBroadKey, false).toBool());
    broadcastRepeat->setValue(MW->settings->value(broadRepKey, 2).toInt());
    autoDiscoveryClicked(autoDiscovery->isChecked());

    QString netIntf = MW->settings->value(netIntfKey, "").toString();
    if (netIntf.length() > 0)
        networkInterfaces->setCurrentText(netIntf);



    connect(commonUsername, SIGNAL(editingFinished()), this, SLOT(usernameUpdated()));
    connect(commonPassword, SIGNAL(editingFinished()), this, SLOT(passwordUpdated()));
    connect(autoDiscovery, SIGNAL(clicked(bool)), this, SLOT(autoDiscoveryClicked(bool)));
    connect(multiBroadcast, SIGNAL(clicked(bool)), this, SLOT(multiBroadcastClicked(bool)));
    connect(broadcastRepeat, SIGNAL(valueChanged(int)), this, SLOT(broadcastRepeatChanged(int)));
    connect(networkInterfaces, SIGNAL(currentTextChanged(const QString&)), this, SLOT(netIntfChanged(const QString&)));
}

void SettingsPanel::autoDiscoveryClicked(bool checked)
{
    if (checked) {
        multiBroadcast->setEnabled(true);
        broadcastRepeat->setEnabled(true);
    }
    else {
        multiBroadcast->setEnabled(false);
        broadcastRepeat->setEnabled(false);
        multiBroadcast->setChecked(false);
    }

    MW->settings->setValue(autoDiscKey, autoDiscovery->isChecked());
    MW->settings->setValue(multiBroadKey, multiBroadcast->isChecked());
}

void SettingsPanel::multiBroadcastClicked(bool checked)
{
    Q_UNUSED(checked);
    MW->settings->setValue(multiBroadKey, multiBroadcast->isChecked());
}

void SettingsPanel::broadcastRepeatChanged(int value)
{
    MW->settings->setValue(broadRepKey, value);
}

void SettingsPanel::usernameUpdated()
{
    MW->settings->setValue(usernameKey, commonUsername->text());
}

void SettingsPanel::passwordUpdated()
{
    MW->settings->setValue(passwordKey, commonPassword->text());
}

void SettingsPanel::netIntfChanged(const QString& arg)
{
    MW->settings->setValue(netIntfKey, arg);
}

void SettingsPanel::getActiveNetworkInterfaces()
{
#ifdef _WIN32
    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;
    DWORD dwRetVal = 0;
    //UINT i;

    ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
    pAdapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof (IP_ADAPTER_INFO));
    if (pAdapterInfo == NULL) {
        emit msg("Error allocating memory needed to call GetAdaptersinfo");
        return;
    }

    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO *) malloc(ulOutBufLen);
        if (pAdapterInfo == NULL) {
            emit msg("Error allocating memory needed to call GetAdaptersinfo");
            return;
        }
    }

    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
        pAdapter = pAdapterInfo;
        while (pAdapter) {
            if (strcmp(pAdapter->IpAddressList.IpAddress.String, "0.0.0.0")) {
                char interface_info[1024];
                sprintf(interface_info, "%s - %s", pAdapter->IpAddressList.IpAddress.String, pAdapter->Description);
                emit msg(QString("Network interface info %1").arg(interface_info));
                networkInterfaces->addItem(QString(interface_info));
            }
            pAdapter = pAdapter->Next;
        }
    } else {
        emit msg(QString("GetAdaptersInfo failed with error: %1").arg(dwRetVal));
    }
    if (pAdapterInfo)
        free(pAdapterInfo);
#endif
}

void SettingsPanel::getCurrentlySelectedIP(char *buffer)
{
    QString selected = networkInterfaces->currentText();
    int index = selected.indexOf(" - ");
    int i = 0;
    for (i = 0; i < index; i++) {
        buffer[i] = selected.toLatin1().data()[i];
    }
    buffer[i] = '\0';
}