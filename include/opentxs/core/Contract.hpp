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

#ifndef OPENTXS_CORE_CONTRACT_HPP
#define OPENTXS_CORE_CONTRACT_HPP

#include "opentxs/core/util/Common.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Nym.hpp"
#include "opentxs/core/OTStringXML.hpp"
#include "opentxs/core/Proto.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/Types.hpp"

#include <stdint.h>
#include <iosfwd>
#include <list>
#include <map>
#include <string>

namespace irr
{
namespace io
{
class IFileReadCallBack;
class IXMLBase;
template <class char_type, class super_class>
class IIrrXMLReader;

typedef IIrrXMLReader<char, IXMLBase> IrrXMLReader;
}  // namespace io
}  // namespace irr

namespace opentxs
{

class OTASCIIArmor;
class OTAsymmetricKey;
class OTPasswordData;
class OTSignature;
class Tag;

typedef std::list<OTSignature*> listOfSignatures;

String trim(const String& str);

class Contract
{

protected:
    /** Contract name as shown in the wallet. */
    String m_strName;

    /** Foldername for this contract (nyms, contracts, accounts, etc) */
    String m_strFoldername;

    /** Filename for this contract (usually an ID.) */
    String m_strFilename;

    /** Hash of the contract, including signatures. (the "raw file") */
    Identifier m_ID;

    /** The Unsigned Clear Text (XML contents without signatures.) */
    OTStringXML m_xmlUnsigned;

    /** The complete raw file including signatures. */
    String m_strRawFile;

    /** The Hash algorithm used for the signature */
    proto::HashType m_strSigHashType{proto::HASHTYPE_ERROR};

    /** CONTRACT, MESSAGE, TRANSACTION, LEDGER, TRANSACTION ITEM */
    String m_strContractType{"CONTRACT"};

    /** The default behavior for a contract, though occasionally overridden, is
     * to contain its own public keys internally, located on standard XML tags.
     * So when we load a contract, we find its public key, and we verify its
     * signature with it. (It self-verifies!) I could be talking about an x509
     * as well, since people will need these to be revokable. The
     * Issuer/Server/etc URL will also be located within the contract, on a
     * standard tag, so by merely loading a contract, a wallet will know how to
     * connect to the relevant server, and the wallet will be able to encrypt
     * messages meant for that server to its public key without the normally
     * requisite key exchange. ==> THE TRADER HAS ASSURANCE THAT, IF HIS
     * OUT-MESSAGE IS ENCRYPTED, HE KNOWS THE MESSAGE CAN ONLY BE DECRYPTED BY
     * THE SAME PERSON WHO SIGNED THAT CONTRACT. */
    mapOfNyms m_mapNyms;

    /** The PGP signatures at the bottom of the XML file. */
    listOfSignatures m_listSignatures;

    /** The version of this Contract file, in case the format changes in the
    future. */
    String m_strVersion{"2.0"};

    // TODO: perhaps move these to a common ancestor for ServerContract and
    // OTUnitDefinition. Maybe call it OTHardContract (since it should never
    // change.)
    String m_strEntityShortName;
    String m_strEntityLongName;
    String m_strEntityEmail;

    /** The legal conditions, usually human-readable, on a contract. */
    String::Map m_mapConditions;

    /** The XML file is in m_xmlUnsigned. Load it from there into members here.
     */
    bool LoadContractXML();

    /** return -1 if error, 0 if nothing, and 1 if the node was processed. */
    EXPORT virtual int32_t ProcessXMLNode(irr::io::IrrXMLReader*& xml);

public:
    /** Used by OTTransactionType::Factory and OTToken::Factory. In both cases,
     * it takes the input string, trims it, and if it's armored, it unarmors it,
     * with the result going into strOutput. On success, bool is returned, and
     * strFirstLine contains the first line from strOutput. */
    EXPORT static bool DearmorAndTrim(
        const String& strInput,
        String& strOutput,
        String& strFirstLine);

    /** The Method "RewriteContract" calls this. I put the meat into a static
     * method so I could use it from outside Contract as well. */
    static bool AddBookendsAroundContent(
        String& strOutput,
        const String& strContents,
        const String& strContractType,
        const proto::HashType hashType,
        const listOfSignatures& listSignatures);
    EXPORT static bool LoadEncodedTextField(
        irr::io::IrrXMLReader*& xml,
        OTASCIIArmor& ascOutput);
    EXPORT static bool LoadEncodedTextField(
        irr::io::IrrXMLReader*& xml,
        String& strOutput);
    static bool LoadEncodedTextFieldByName(
        irr::io::IrrXMLReader*& xml,
        OTASCIIArmor& ascOutput,
        const char* szName,
        String::Map* pmapExtraVars = nullptr);
    static bool LoadEncodedTextFieldByName(
        irr::io::IrrXMLReader*& xml,
        String& strOutput,
        const char* szName,
        String::Map* pmapExtraVars = nullptr);
    static bool SkipToElement(irr::io::IrrXMLReader*& xml);
    static bool SkipToTextField(irr::io::IrrXMLReader*& xml);
    static bool SkipAfterLoadingField(irr::io::IrrXMLReader*& xml);
    inline void SetIdentifier(const Identifier& theID) { m_ID = theID; }
    EXPORT Contract() = default;
    EXPORT Contract(
        const String& name,
        const String& foldername,
        const String& filename,
        const String& strID);
    EXPORT explicit Contract(const String& strID);
    EXPORT explicit Contract(const Identifier& theID);

    // TODO: a contract needs to have certain required fields in order to be
    // accepted for notarization. One of those should be a URL where anyone can
    // see a list of the approved e-notary servers, signed by the issuer.
    //
    // Why is this important?
    //
    // Because when the issuer connects to the e-notary to issue the currency,
    // he must upload the asset contract as part of that process. During the
    // same process, the e-notary connects to that standard URL and downloads a
    // RECORD, signed by the ISSUER, showing the e-notary on the accepted list
    // of transaction providers.
    //
    // Now the e-notary can make THAT record available to its clients (most
    // likely demanded by their wallet software) as proof that the issuer has,
    // in fact, issued digital assets on the e-notary server in question. This
    // provides proof that the issuer is, in fact, legally on the line for
    // whatever assets they have actually issued through that e-notary. The
    // issuer can make the total outstanding units available publicly, which
    // wallets can cross-reference with the public records on the transaction
    // servers. (The figures concerning total issued currency should match.)
    //
    // Of course, the transaction server could still lie, and publish a
    // falsified number instead of the actual total issued currency for a given
    // digital asset. Only systems can prevent that, based around separation of
    // powers. People will be more likely to trust the transaction provider who
    // has good accounting and code audit processes, with code fingerprints,
    // multiple passwords across neutral and bonded 3rd parties, insured, etc.
    // Ultimately these practices will be governed by the cost of insurance.
    //
    // But there WILL be winners who arise because they implement systems that
    // provide trust. And trust is a currency.
    //
    // (Currently the code loads the key FROM the contract itself, which won't
    // be possible when the issuer and transaction provider are two separate
    // entities. So this sort of protocol becomes necessary.)

    EXPORT virtual ~Contract();
    EXPORT virtual void Release();
    EXPORT void Release_Contract();
    EXPORT void ReleaseSignatures();

    /** This function is for those times when you already have the unsigned
     * version of the contract, and you have the signer, and you just want to
     * sign it and calculate its new ID from the finished result. */
    EXPORT virtual bool CreateContract(
        const String& strContract,
        const Nym& theSigner);

    /** CreateContract is great if you already know what kind of contract to
     * instantiate and have already done so. Otherwise this function will take
     * ANY flat text and use a generic Contract instance to sign it and then
     * write it to strOutput. This is due to the fact that OT was never really
     * designed for signing flat text, only contracts. */
    EXPORT static bool SignFlatText(
        String& strFlatText,
        const String& strContractType,  // "LEDGER" or "PURSE" etc.
        const Nym& theSigner,
        String& strOutput);
    EXPORT inline void GetName(String& strName) const { strName = m_strName; }
    EXPORT inline void SetName(const String& strName) { m_strName = strName; }
    EXPORT inline const String& GetContractType() const
    {
        return m_strContractType;
    }
    /** This function calls VerifyContractID, and if that checks out, then it
     * looks up the official "contract" key inside the contract by calling
     * GetContractPublicNym, and uses it to verify the signature on the
     * contract. So the contract is self-verifying. Right now only public keys
     * are supported, but soon contracts will also support x509 certs. */
    EXPORT virtual bool VerifyContract() const;

    /** Overriden for example in OTOffer, OTMarket. You can get it in string or
     * binary form. */
    EXPORT virtual void GetIdentifier(Identifier& theIdentifier) const;

    /** The Contract ID is a hash of the contract raw file. */
    EXPORT void GetIdentifier(String& theIdentifier) const;
    EXPORT void GetFilename(String& strFilename) const;

    /** assumes m_strFilename is already set. Then it reads that file into a
     * string. Then it parses that string into the object. */
    EXPORT virtual bool LoadContract();
    EXPORT bool LoadContract(const char* szFoldername, const char* szFilename);

    /** Just like it says. If you have a contract in string form, pass it in
     * here to import it. */
    EXPORT virtual bool LoadContractFromString(const String& theStr);

    /** fopens m_strFilename and reads it off the disk into m_strRawFile */
    bool LoadContractRawFile();

    /** parses m_strRawFile into the various member variables. Separating these
     * into two steps allows us to load contracts from other sources besides
     * files. */
    EXPORT bool ParseRawFile();

    /** data_folder/contracts/Contract-ID */
    EXPORT bool SaveToContractFolder();

    /** Saves the raw (pre-existing) contract text to any string you want to
     * pass in. */
    EXPORT bool SaveContractRaw(String& strOutput) const;

    /** Takes the pre-existing XML contents (WITHOUT signatures) and re-writes
     * the Raw data, adding the pre-existing signatures along with new signature
     * bookends. */
    EXPORT bool RewriteContract(String& strOutput) const;

    /** This saves the Contract to its own internal member string, m_strRawFile
     * (and does NOT actually save it to a file.) */
    EXPORT virtual bool SaveContract();

    /** Saves the contract to a specific filename */
    EXPORT virtual bool SaveContract(
        const char* szFoldername,
        const char* szFilename);

    /** Update the internal unsigned contents based on the member variables
     * default behavior does nothing. */
    EXPORT virtual void UpdateContents();

    /** Only used when first generating an asset or server contract. Meant for
     * contracts which never change after that point. Otherwise does the same
     * thing as UpdateContents. (But meant for a different purpose.) */
    EXPORT virtual void CreateContents();

    /** Overrides of CreateContents call this in order to add some common
     * internals. */
    EXPORT void CreateInnerContents(Tag& parent);

    /** Save the internal contents (m_xmlUnsigned) to an already-open file */
    EXPORT virtual bool SaveContents(std::ofstream& ofs) const;

    /** Saves the entire contract to a file that's already open (like a wallet).
     */
    EXPORT virtual bool SaveContractWallet(Tag& parent) const;

    EXPORT virtual bool DisplayStatistics(String& strContents) const;

    /** Save m_xmlUnsigned to a string that's passed in */
    EXPORT virtual bool SaveContents(String& strContents) const;
    EXPORT virtual bool SignContract(
        const Nym& theNym,
        const OTPasswordData* pPWData = nullptr);
    EXPORT bool SignContractAuthent(
        const Nym& theNym,
        const OTPasswordData* pPWData = nullptr);
    EXPORT bool SignWithKey(
        const OTAsymmetricKey& theKey,
        const OTPasswordData* pPWData = nullptr);
    EXPORT bool SignContract(
        const Nym& theNym,
        OTSignature& theSignature,
        const OTPasswordData* pPWData = nullptr);

    /** Uses authentication key instead of signing key. */
    EXPORT bool SignContractAuthent(
        const Nym& theNym,
        OTSignature& theSignature,
        const OTPasswordData* pPWData = nullptr);
    EXPORT bool SignContract(
        const OTAsymmetricKey& theKey,
        OTSignature& theSignature,
        const proto::HashType hashType,
        const OTPasswordData* pPWData = nullptr);

    /** Calculates a hash of m_strRawFile (the xml portion of the contract plus
    the signatures) and compares to m_ID (supposedly the same. The ID is
    calculated by hashing the file.)

    Be careful here--asset contracts and server contracts can have this ID. But
    a class such as OTAccount will change in its datafile as the balance
    changes. Thus, the account must have a Unique ID that is NOT a hash of its
    file.

    This means it's important to have the ID function overridable for
    OTAccount... This also means that my wallet MUST be signed, and these files
    should have and encryption option also. Because if someone changes my
    account ID in the file, I have no way of re-calculating it from the account
    file, which changes! So my copies of the account file and wallet file are
    the only records of that account ID which is a giant int64_t number. */
    EXPORT virtual bool VerifyContractID() const;
    EXPORT virtual void CalculateContractID(Identifier& newID) const;
    EXPORT virtual void CalculateAndSetContractID(Identifier& newID);

    /** So far not overridden anywhere (used to be OTTrade.) */
    EXPORT virtual bool VerifySignature(
        const Nym& theNym,
        const OTPasswordData* pPWData = nullptr) const;
    EXPORT virtual bool VerifySigAuthent(
        const Nym& theNym,
        const OTPasswordData* pPWData = nullptr) const;
    EXPORT virtual bool VerifyWithKey(
        const OTAsymmetricKey& theKey,
        const OTPasswordData* pPWData = nullptr) const;
    EXPORT bool VerifySignature(
        const Nym& theNym,
        const OTSignature& theSignature,
        const OTPasswordData* pPWData = nullptr) const;

    /** Uses authentication key instead of signing key. */
    EXPORT bool VerifySigAuthent(
        const Nym& theNym,
        const OTSignature& theSignature,
        const OTPasswordData* pPWData = nullptr) const;
    EXPORT bool VerifySignature(
        const OTAsymmetricKey& theKey,
        const OTSignature& theSignature,
        const proto::HashType hashType,
        const OTPasswordData* pPWData = nullptr) const;
    EXPORT const Nym* GetContractPublicNym() const;
};
}  // namespace opentxs
#endif  // OPENTXS_CORE_CONTRACT_HPP
