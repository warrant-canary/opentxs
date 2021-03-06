/************************************************************
 *
 *                 OPEN TRANSACTIONS
 *
 *       Financial Cryptography and Digital Cash
 *       Library, Protocol, API, Server, CLI, GUI
 *
 *       -- Anonymous Numbered Accounts.
 *       -- Untraceable Digital Cash.
 *       -- Triple-Signed Receipts.
 *       -- Cheques, Vouchers, Transfers, Inboxes.
 *       -- Basket Currencies, Markets, Payment Plans.
 *       -- Signed, XML, Ricardian-style Contracts.
 *       -- Scripted smart contracts.
 *
 *  EMAIL:
 *  fellowtraveler@opentransactions.org
 *
 *  WEBSITE:
 *  http://www.opentransactions.org/
 *
 *  -----------------------------------------------------
 *
 *   LICENSE:
 *   This Source Code Form is subject to the terms of the
 *   Mozilla Public License, v. 2.0. If a copy of the MPL
 *   was not distributed with this file, You can obtain one
 *   at http://mozilla.org/MPL/2.0/.
 *
 *   DISCLAIMER:
 *   This program is distributed in the hope that it will
 *   be useful, but WITHOUT ANY WARRANTY; without even the
 *   implied warranty of MERCHANTABILITY or FITNESS FOR A
 *   PARTICULAR PURPOSE.  See the Mozilla Public License
 *   for more details.
 *
 ************************************************************/

#include "opentxs/api/Settings.hpp"

#include "opentxs/core/Log.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/util/Assert.hpp"
#include "opentxs/core/util/OTPaths.hpp"

// NOTE: cstdlib HAS to be included here above SimpleIni, since for some reason
// it uses stdlib functions without including that header.
#include <cstdlib>
#include <simpleini/SimpleIni.h>
#include <stdint.h>
#include <cinttypes>
#include <memory>
#include <ostream>
#include <string>

namespace opentxs
{

class Settings::SettingsPvt
{
private:
    SettingsPvt(const SettingsPvt&);
    SettingsPvt& operator=(const SettingsPvt&);

public:
    CSimpleIniA iniSimple;

    SettingsPvt()
        : iniSimple()
    {
    }
};

bool Settings::Init()
{
    // First Load, Create new fresh config file if failed loading.
    if (!Load()) {
        otOut << __FUNCTION__
              << ": Note: Unable to Load Config. Creating a new file."
              << "\n";
        if (!Reset()) return false;
        if (!Save()) return false;
    }

    if (!Reset()) return false;

    // Second Load, Throw Assert if Failed loading.
    if (!Load()) {
        otErr << __FUNCTION__ << ": Error: Unable to load config file."
              << " It should exist, as we just saved it!\n";
        OT_FAIL;
    }

    return true;
}

bool Settings::Load(const String& strConfigurationFileExactPath)
{
    if (!strConfigurationFileExactPath.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strConfigurationFileExactPath"
              << " is Empty!\n";
        return false;
    }

    bool bFolderCreated(false);
    if (!OTPaths::BuildFilePath(
            strConfigurationFileExactPath, bFolderCreated)) {
        OT_FAIL;
    };

    if (!IsEmpty()) {
        otErr << __FUNCTION__ << ": Bad: "
              << "p_Settings"
              << " is not Empty!\n";
        OT_FAIL;
    }

    std::int64_t lFilelength = 0;
    if (!OTPaths::FileExists(
            strConfigurationFileExactPath,
            lFilelength))  // we don't have a config file, lets
                           // create a blank one first.
    {
        pvt_->iniSimple.Reset();  // clean the config.

        SI_Error rc = pvt_->iniSimple.SaveFile(
            strConfigurationFileExactPath.Get());  // save a new file.
        if (0 > rc) return false;                  // error!

        pvt_->iniSimple.Reset();  // clean the config (again).
    }

    SI_Error rc = pvt_->iniSimple.LoadFile(strConfigurationFileExactPath.Get());
    if (0 > rc)
        return false;
    else
        return true;
}

bool Settings::Save(const String& strConfigurationFileExactPath)
{
    if (!strConfigurationFileExactPath.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strConfigurationFileExactPath"
              << " is Empty!\n";
        return false;
    }

    SI_Error rc = pvt_->iniSimple.SaveFile(strConfigurationFileExactPath.Get());
    if (0 > rc)
        return false;
    else
        return true;
}

bool Settings::LogChange_str(
    const String& strSection,
    const String& strKey,
    const String& strValue)
{
    if (!strSection.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strSection"
              << " is Empty!\n";
        OT_FAIL;
    }
    if (!strKey.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strKey"
              << " is Empty!\n";
        OT_FAIL;
    }

    const char* const szValue = (strValue.Exists() && !strValue.Compare(""))
                                    ? strValue.Get()
                                    : "nullptr";

    String strCategory, strOption;
    if (!Log::StringFill(strCategory, strSection.Get(), 12)) return false;
    if (!Log::StringFill(strOption, strKey.Get(), 30, " to:")) return false;

    otWarn << "Setting " << strCategory << " " << strOption << " " << szValue
           << " \n";
    return true;
}

Settings::Settings(const String& strConfigFilePath)
    : pvt_(new SettingsPvt())
    , b_Loaded(false)
    , m_strConfigurationFileExactPath(strConfigFilePath)
{
    if (!m_strConfigurationFileExactPath.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "m_strConfigurationFileExactPath"
              << " is Empty!\n";
        OT_FAIL;
    }

    if (!Init()) {
        OT_FAIL;
    }
}

void Settings::SetConfigFilePath(const String& strConfigFilePath)
{
    m_strConfigurationFileExactPath.Set(strConfigFilePath.Get());
}

bool Settings::HasConfigFilePath()
{
    return m_strConfigurationFileExactPath.Exists();
}

Settings::Settings()
    : pvt_(new SettingsPvt())
    , b_Loaded(false)
{
}

Settings::~Settings() {
    Save();
    Reset();
}

bool Settings::Load()
{
    b_Loaded = false;

    if (Load(m_strConfigurationFileExactPath)) {
        b_Loaded = true;
        return true;
    } else
        return false;
}

bool Settings::Save() { return Save(m_strConfigurationFileExactPath); }

const bool& Settings::IsLoaded() const { return b_Loaded; }

bool Settings::Reset()
{
    b_Loaded = false;
    pvt_->iniSimple.Reset();
    return true;
}

bool Settings::IsEmpty() const { return pvt_->iniSimple.IsEmpty(); }

bool Settings::Check_str(
    const String& strSection,
    const String& strKey,
    String& out_strResult,
    bool& out_bKeyExist) const
{
    if (!strSection.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strSection"
              << " is Empty!\n";
        OT_FAIL;
    }
    if (strSection.Compare("")) {
        otErr << __FUNCTION__ << ": Error: "
              << "strSection"
              << " is Blank!\n";
        OT_FAIL;
    }

    if (!strKey.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strKey"
              << " is Empty!\n";
        OT_FAIL;
    }
    if (strKey.Compare("")) {
        otErr << __FUNCTION__ << ": Error: "
              << "strKey"
              << " is Blank!\n";
        OT_FAIL;
    }

    const char* szVar =
        pvt_->iniSimple.GetValue(strSection.Get(), strKey.Get(), nullptr);
    String strVar(szVar);

    if (strVar.Exists() && !strVar.Compare("")) {
        out_bKeyExist = true;
        out_strResult = strVar;
    } else {
        out_bKeyExist = false;
        out_strResult = "";
    }

    return true;
}

bool Settings::Check_long(
    const String& strSection,
    const String& strKey,
    std::int64_t& out_lResult,
    bool& out_bKeyExist) const
{
    if (!strSection.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strSection"
              << " is Empty!\n";
        OT_FAIL;
    }
    if (strSection.Compare("")) {
        otErr << __FUNCTION__ << ": Error: "
              << "strSection"
              << " is Blank!\n";
        OT_FAIL;
    }

    if (!strKey.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strKey"
              << " is Empty!\n";
        OT_FAIL;
    }
    if (strKey.Compare("")) {
        otErr << __FUNCTION__ << ": Error: "
              << "strKey"
              << " is Blank!\n";
        OT_FAIL;
    }

    const char* szVar =
        pvt_->iniSimple.GetValue(strSection.Get(), strKey.Get(), nullptr);
    String strVar(szVar);

    if (strVar.Exists() && !strVar.Compare("")) {
        out_bKeyExist = true;
        out_lResult =
            pvt_->iniSimple.GetLongValue(strSection.Get(), strKey.Get(), 0);
    } else {
        out_bKeyExist = false;
        out_lResult = 0;
    }

    return true;
}

bool Settings::Check_bool(
    const String& strSection,
    const String& strKey,
    bool& out_bResult,
    bool& out_bKeyExist) const
{
    if (!strSection.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strSection"
              << " is Empty!\n";
        OT_FAIL;
    }
    if (strSection.Compare("")) {
        otErr << __FUNCTION__ << ": Error: "
              << "strSection"
              << " is Blank!\n";
        OT_FAIL;
    }

    if (!strKey.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strKey"
              << " is Empty!\n";
        OT_FAIL;
    }
    if (strKey.Compare("")) {
        otErr << __FUNCTION__ << ": Error: "
              << "strKey"
              << " is Blank!\n";
        OT_FAIL;
    }

    const char* szVar =
        pvt_->iniSimple.GetValue(strSection.Get(), strKey.Get(), nullptr);
    String strVar(szVar);

    if (strVar.Exists() &&
        (strVar.Compare("false") || strVar.Compare("true"))) {
        out_bKeyExist = true;
        if (strVar.Compare("true"))
            out_bResult = true;
        else
            out_bResult = false;
    } else {
        out_bKeyExist = false;
        out_bResult = false;
    }

    return true;
}

bool Settings::Set_str(
    const String& strSection,
    const String& strKey,
    const String& strValue,
    bool& out_bNewOrUpdate,
    const String& strComment)
{
    if (!strSection.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strSection"
              << " is Empty!\n";
        OT_FAIL;
    }
    if (strSection.Compare("")) {
        otErr << __FUNCTION__ << ": Error: "
              << "strSection"
              << " is Blank!\n";
        OT_FAIL;
    }

    if (!strKey.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strKey"
              << " is Empty!\n";
        OT_FAIL;
    }
    if (strKey.Compare("")) {
        otErr << __FUNCTION__ << ": Error: "
              << "strKey"
              << " is Blank!\n";
        OT_FAIL;
    }

    // if (nullptr == m_strConfigurationFileExactPath){ otErr << "%s: Error:
    // %s is a nullptr!\n", __FUNCTION__, "p_iniSimple"); OT_FAIL; }

    const char* const szValue =
        (strValue.Exists() && !strValue.Compare("")) ? strValue.Get() : nullptr;
    const char* const szComment =
        (strComment.Exists() && !strComment.Compare("")) ? strComment.Get()
                                                         : nullptr;

    String strOldValue, strNewValue;
    bool bOldKeyExist = false, bNewKeyExist = false;

    // Check if Old Key exists.
    if (!Check_str(strSection, strKey, strOldValue, bOldKeyExist)) return false;

    if (bOldKeyExist) {
        if (strValue.Compare(strOldValue)) {
            out_bNewOrUpdate = false;
            return true;
        }
    }

    // Log to Output Setting Change
    if (!LogChange_str(strSection, strKey, strValue)) return false;

    // Set New Value
    SI_Error rc = pvt_->iniSimple.SetValue(
        strSection.Get(), strKey.Get(), szValue, szComment, true);
    if (0 > rc) return false;

    if (nullptr ==
        szValue)  // We set the key's value to null, thus removing it.
    {
        if (bOldKeyExist)
            out_bNewOrUpdate = true;
        else
            out_bNewOrUpdate = false;

        return true;
    }

    // Check if the new value is the same as intended.
    if (!Check_str(strSection, strKey, strNewValue, bNewKeyExist)) return false;

    if (bNewKeyExist) {
        if (strValue.Compare(strNewValue)) {
            // Success
            out_bNewOrUpdate = true;
            return true;
        }
    }

    // If we get here, error!
    OT_FAIL;
}

bool Settings::Set_long(
    const String& strSection,
    const String& strKey,
    const std::int64_t& lValue,
    bool& out_bNewOrUpdate,
    const String& strComment)
{
    if (!strSection.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strSection"
              << " is Empty!\n";
        OT_FAIL;
    }
    if (strSection.Compare("")) {
        otErr << __FUNCTION__ << ": Error: "
              << "strSection"
              << " is Blank!\n";
        OT_FAIL;
    }

    if (!strKey.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strKey"
              << " is Empty!\n";
        OT_FAIL;
    }
    if (strKey.Compare("")) {
        otErr << __FUNCTION__ << ": Error: "
              << "strKey"
              << " is Blank!\n";
        OT_FAIL;
    }

    String strValue;
    strValue.Format("%" PRId64, lValue);

    const char* const szComment =
        (strComment.Exists() && !strComment.Compare("")) ? strComment.Get()
                                                         : nullptr;

    String strOldValue, strNewValue;
    bool bOldKeyExist = false, bNewKeyExist = false;

    // Check if Old Key exists.
    if (!Check_str(strSection, strKey, strOldValue, bOldKeyExist)) return false;

    if (bOldKeyExist) {
        if (strValue.Compare(strOldValue)) {
            out_bNewOrUpdate = false;
            return true;
        }
    }

    // Log to Output Setting Change
    if (!LogChange_str(strSection, strKey, strValue)) return false;

    // Set New Value
    SI_Error rc = pvt_->iniSimple.SetLongValue(
        strSection.Get(), strKey.Get(), lValue, szComment, false, true);
    if (0 > rc) return false;

    // Check if the new value is the same as intended.
    if (!Check_str(strSection, strKey, strNewValue, bNewKeyExist)) return false;

    if (bNewKeyExist) {
        if (strValue.Compare(strNewValue)) {
            // Success
            out_bNewOrUpdate = true;
            return true;
        }
    }

    // If we get here, error!
    OT_FAIL;
}

bool Settings::Set_bool(
    const String& strSection,
    const String& strKey,
    const bool& bValue,
    bool& out_bNewOrUpdate,
    const String& strComment)
{
    if (!strSection.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strSection"
              << " is Empty!\n";
        OT_FAIL;
    }
    if (!strKey.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strKey"
              << " is Empty!\n";
        OT_FAIL;
    }
    const String strValue(bValue ? "true" : "false");

    return Set_str(strSection, strKey, strValue, out_bNewOrUpdate, strComment);
}

bool Settings::CheckSetSection(
    const String& strSection,
    const String& strComment,
    bool& out_bIsNewSection)
{
    if (!strSection.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strSection"
              << " is Empty!\n";
        OT_FAIL;
    }
    if (!strComment.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strComment"
              << " is Empty!\n";
        OT_FAIL;
    }

    const char* const szComment =
        (strComment.Exists() && !strComment.Compare("")) ? strComment.Get()
                                                         : nullptr;

    const std::int64_t lSectionSize =
        pvt_->iniSimple.GetSectionSize(strSection.Get());

    if (1 > lSectionSize) {
        out_bIsNewSection = true;
        SI_Error rc = pvt_->iniSimple.SetValue(
            strSection.Get(), nullptr, nullptr, szComment, false);
        if (0 > rc) return false;
    } else {
        out_bIsNewSection = false;
    }
    return true;
}

bool Settings::CheckSet_str(
    const String& strSection,
    const String& strKey,
    const String& strDefault,
    String& out_strResult,
    bool& out_bIsNew,
    const String& strComment)
{
    std::string temp = out_strResult.Get();
    bool success = CheckSet_str(
        strSection, strKey, strDefault, temp, out_bIsNew, strComment);
    out_strResult = String(temp);

    return success;
}

bool Settings::CheckSet_str(
    const String& strSection,
    const String& strKey,
    const String& strDefault,
    std::string& out_strResult,
    bool& out_bIsNew,
    const String& strComment)
{
    if (!strSection.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strSection"
              << " is Empty!\n";
        OT_FAIL;
    }
    if (!strKey.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strKey"
              << " is Empty!\n";
        OT_FAIL;
    }

    const char* const szDefault =
        (strDefault.Exists() && !strDefault.Compare("")) ? strDefault.Get()
                                                         : nullptr;

    String strTempResult;
    bool bKeyExist = false;
    if (!Check_str(strSection, strKey, strTempResult, bKeyExist)) return false;

    if (bKeyExist) {
        // Already have a key, lets use it's value.
        out_bIsNew = false;
        out_strResult = strTempResult.Get();
        return true;
    } else {
        bool bNewKeyCheck;
        if (!Set_str(strSection, strKey, strDefault, bNewKeyCheck, strComment))
            return false;

        if (nullptr == szDefault)  // The Default is to have no key.
        {
            // Success
            out_bIsNew = false;
            out_strResult = "";
            return true;
        }

        if (bNewKeyCheck) {
            // Success
            out_bIsNew = true;
            out_strResult = strDefault.Get();
            return true;
        }
    }

    // If we get here, error!
    OT_FAIL;
}

bool Settings::CheckSet_long(
    const String& strSection,
    const String& strKey,
    const std::int64_t& lDefault,
    std::int64_t& out_lResult,
    bool& out_bIsNew,
    const String& strComment)
{
    if (!strSection.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strSection"
              << " is Empty!\n";
        OT_FAIL;
    }
    if (!strKey.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strKey"
              << " is Empty!\n";
        OT_FAIL;
    }

    std::int64_t lTempResult = 0;
    bool bKeyExist = false;
    if (!Check_long(strSection, strKey, lTempResult, bKeyExist)) return false;

    if (bKeyExist) {
        // Already have a key, lets use it's value.
        out_bIsNew = false;
        out_lResult = lTempResult;
        return true;
    } else {
        bool bNewKeyCheck;
        if (!Set_long(strSection, strKey, lDefault, bNewKeyCheck, strComment))
            return false;
        if (bNewKeyCheck) {
            // Success
            out_bIsNew = true;
            out_lResult = lDefault;
            return true;
        }
    }

    // If we get here, error!
    OT_FAIL;
}

bool Settings::CheckSet_bool(
    const String& strSection,
    const String& strKey,
    const bool& bDefault,
    bool& out_bResult,
    bool& out_bIsNew,
    const String& strComment)
{
    if (!strSection.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strSection"
              << " is Empty!\n";
        OT_FAIL;
    }
    if (!strKey.Exists()) {
        otErr << __FUNCTION__ << ": Error: "
              << "strKey"
              << " is Empty!\n";
        OT_FAIL;
    }

    bool bKeyExist = false, bTempResult = false;
    if (!Check_bool(strSection, strKey, bTempResult, bKeyExist)) return false;

    if (bKeyExist) {
        // Already have a key, lets use it's value.
        out_bIsNew = false;
        out_bResult = bTempResult;
        return true;
    } else {
        bool bNewKeyCheck;
        if (!Set_bool(strSection, strKey, bDefault, bNewKeyCheck, strComment))
            return false;
        if (bNewKeyCheck) {
            // Success
            out_bIsNew = true;
            out_bResult = bDefault;
            return true;
        }
    }

    // If we get here, error!
    OT_FAIL;
}

bool Settings::SetOption_bool(
    const String& strSection,
    const String& strKey,
    bool& bVariableName)
{
    bool bNewOrUpdate;
    return CheckSet_bool(
        strSection, strKey, bVariableName, bVariableName, bNewOrUpdate);
}

}  // namespace opentxs
