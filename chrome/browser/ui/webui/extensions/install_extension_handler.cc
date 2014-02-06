// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/extensions/install_extension_handler.h"

#include "base/bind.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/extensions/crx_installer.h"
#include "chrome/browser/extensions/extension_install_prompt.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_system.h"
#include "chrome/browser/extensions/unpacked_installer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/feature_switch.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_view.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/drop_data.h"
#include "grit/generated_resources.h"
#include "net/base/net_util.h"
#include "ui/base/l10n/l10n_util.h"

namespace extensions {

InstallExtensionHandler::InstallExtensionHandler() {
}

InstallExtensionHandler::~InstallExtensionHandler() {
}

void InstallExtensionHandler::GetLocalizedValues(
    content::WebUIDataSource* source) {
  source->AddString(
      "extensionSettingsInstallDropTarget",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_INSTALL_DROP_TARGET));
  source->AddBoolean(
      "offStoreInstallEnabled",
      FeatureSwitch::easy_off_store_install()->IsEnabled());
}

void InstallExtensionHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "startDrag",
      base::Bind(&InstallExtensionHandler::HandleStartDragMessage,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "stopDrag",
      base::Bind(&InstallExtensionHandler::HandleStopDragMessage,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "installDroppedFile",
      base::Bind(&InstallExtensionHandler::HandleInstallMessage,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "installDroppedDirectory",
      base::Bind(&InstallExtensionHandler::HandleInstallDirectoryMessage,
                 base::Unretained(this)));
}

void InstallExtensionHandler::HandleStartDragMessage(const ListValue* args) {
  content::DropData* drop_data =
      web_ui()->GetWebContents()->GetView()->GetDropData();
  if (!drop_data) {
    DLOG(ERROR) << "No current drop data.";
    return;
  }

  if (drop_data->filenames.empty()) {
    DLOG(ERROR) << "Current drop data contains no files.";
    return;
  }

  const content::DropData::FileInfo& file_info = drop_data->filenames.front();

  file_to_install_ = base::FilePath::FromWStringHack(
      UTF16ToWide(file_info.path));
  // Use the display name if provided, for checking file names
  // (.path is likely a random hash value in that case).
  file_display_name_ =
      file_info.display_name.empty() ? file_info.path : file_info.display_name;
}

void InstallExtensionHandler::HandleStopDragMessage(const ListValue* args) {
  file_to_install_.clear();
  file_display_name_.clear();
}

void InstallExtensionHandler::HandleInstallMessage(const ListValue* args) {
  if (file_to_install_.empty()) {
    LOG(ERROR) << "No file captured to install.";
    return;
  }

  Profile* profile = Profile::FromBrowserContext(
      web_ui()->GetWebContents()->GetBrowserContext());
  scoped_ptr<ExtensionInstallPrompt> prompt(
      new ExtensionInstallPrompt(web_ui()->GetWebContents()));
  scoped_refptr<CrxInstaller> crx_installer(CrxInstaller::Create(
      ExtensionSystem::Get(profile)->extension_service(),
      prompt.Pass()));
  crx_installer->set_error_on_unsupported_requirements(true);
  crx_installer->set_off_store_install_allow_reason(
      CrxInstaller::OffStoreInstallAllowedFromSettingsPage);
  crx_installer->set_install_wait_for_idle(false);

  const bool kCaseSensitive = false;

  if (EndsWith(file_display_name_, ASCIIToUTF16(".user.js"), kCaseSensitive)) {
    crx_installer->InstallUserScript(
        file_to_install_,
        net::FilePathToFileURL(file_to_install_));
  } else if (EndsWith(file_display_name_,
                      ASCIIToUTF16(".crx"),
                      kCaseSensitive)) {
    crx_installer->InstallCrx(file_to_install_);
  } else {
    CHECK(false);
  }

  file_to_install_.clear();
  file_display_name_.clear();
}

void InstallExtensionHandler::HandleInstallDirectoryMessage(
    const ListValue* args) {
  Profile* profile = Profile::FromBrowserContext(
      web_ui()->GetWebContents()->GetBrowserContext());
  UnpackedInstaller::Create(
      ExtensionSystem::Get(profile)->
          extension_service())->Load(file_to_install_);
}

}  // namespace extensions
