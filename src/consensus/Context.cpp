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

#include "opentxs/consensus/Context.hpp"

#include "opentxs/api/Wallet.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/Nym.hpp"
#include "opentxs/core/String.hpp"

#ifndef OT_MAX_ACK_NUMS
#define OT_MAX_ACK_NUMS 100
#endif

namespace opentxs
{
Context::Context(
    const Identifier& local,
    const Identifier& remote,
    Wallet& wallet)
    : ot_super(wallet.Nym(local), 1)
    , wallet_(wallet)
{
    remote_nym_ = wallet_.Nym(remote);
    request_number_.store(0);
}

Context::Context(
    const proto::Context& serialized,
    Wallet& wallet)
    : ot_super(
        wallet.Nym(Identifier(serialized.localnym())), serialized.version())
    , wallet_(wallet)
    , local_nymbox_hash_(serialized.localnymboxhash())
    , remote_nymbox_hash_(serialized.remotenymboxhash())
{
    remote_nym_ = wallet_.Nym(Identifier(serialized.remotenym()));
    request_number_.store(serialized.requestnumber());

    for (const auto& it : serialized.acknowledgedrequestnumber()) {
        acknowledged_request_numbers_.insert(it);
    }

    for (const auto& it : serialized.availabletransactionnumber()) {
        available_transaction_numbers_.insert(it);
    }

    for (const auto& it : serialized.issuedtransactionnumber()) {
        issued_transaction_numbers_.insert(it);
    }

    signatures_.push_front(
        SerializedSignature(
            std::make_shared<proto::Signature>(serialized.signature())));
}

std::set<RequestNumber> Context::AcknowledgedNumbers() const
{
    Lock lock(lock_);

    return acknowledged_request_numbers_;
}

bool Context::AddAcknowledgedNumber(const RequestNumber req)
{
    Lock lock(lock_);

    auto output = acknowledged_request_numbers_.insert(req);

    while (OT_MAX_ACK_NUMS < acknowledged_request_numbers_.size()) {
        acknowledged_request_numbers_.erase(
            acknowledged_request_numbers_.begin());
    }

    return output.second;
}

proto::Context Context::contract(const Lock& lock) const
{
    auto output = serialize(lock);

    if (0 < signatures_.size()) {
        auto& sigProto = *output.mutable_signature();
        sigProto.CopyFrom(*signatures_.front());
    }

    return output;
}

void Context::finish_acknowledgements(
    const Lock& lock,
    const std::set<RequestNumber>& req)
{
    OT_ASSERT(verify_write_lock(lock));

    std::set<RequestNumber> toErase;

    for (const auto& number : acknowledged_request_numbers_) {
        if (0 == req.count(number)) {
            toErase.insert(number);
        }
    }

    for (const auto& it : toErase) {
        acknowledged_request_numbers_.erase(it);
    }
}

Identifier Context::GetID(const Lock& lock) const
{
    auto contract = IDVersion(lock);
    Identifier id;
    id.CalculateDigest(proto::ProtoAsData(contract));

    return id;
}

bool Context::HaveLocalNymboxHash() const
{
    return String(local_nymbox_hash_).Exists();
}

bool Context::HaveRemoteNymboxHash() const
{
    return String(remote_nymbox_hash_).Exists();
}

proto::Context Context::IDVersion(const Lock& lock) const
{
    OT_ASSERT(verify_write_lock(lock));

    proto::Context output;
    output.set_version(version_);

    switch (Type()) {
        case proto::CONSENSUSTYPE_SERVER : {
            if (nym_) {
                output.set_localnym(String(nym_->ID()).Get());
            }

            if (remote_nym_) {
                output.set_remotenym(String(remote_nym_->ID()).Get());
            }

            output.set_localnymboxhash(String(local_nymbox_hash_).Get());
            output.set_remotenymboxhash(String(remote_nymbox_hash_).Get());
        } break;
        case proto::CONSENSUSTYPE_CLIENT : {
            if (nym_) {
                output.set_remotenym(String(nym_->ID()).Get());
            }

            if (remote_nym_) {
                output.set_localnym(String(remote_nym_->ID()).Get());
            }

            output.set_remotenymboxhash(String(local_nymbox_hash_).Get());
            output.set_localnymboxhash(String(remote_nymbox_hash_).Get());
        } break;
        default : { OT_FAIL; }
    }

    output.set_requestnumber(request_number_.load());

    for (const auto& it : available_transaction_numbers_) {
        output.add_availabletransactionnumber(it);
    }

    for (const auto& it : issued_transaction_numbers_) {
        output.add_issuedtransactionnumber(it);
    }

    return output;
}

RequestNumber Context::IncrementRequest()
{
    return ++request_number_;
}

Identifier Context::LocalNymboxHash() const
{
    Lock lock(lock_);

    return local_nymbox_hash_;
}

std::string Context::Name() const
{
    Lock lock(lock_);

    return String(id(lock)).Get();
}

bool Context::NymboxHashMatch() const
{
    if (!HaveLocalNymboxHash()) { return false; }

    if (!HaveRemoteNymboxHash()) { return false; }

    return (local_nymbox_hash_ == remote_nymbox_hash_);
}

Identifier Context::RemoteNymboxHash() const
{
    Lock lock(lock_);

    return remote_nymbox_hash_;
}

bool Context::RemoveAcknowledgedNumber(const std::set<RequestNumber>& req)
{
    Lock lock(lock_);

    std::size_t removed = 0;

    for (const auto& number : req) {
        removed += acknowledged_request_numbers_.erase(number);
    }

    lock.unlock();

    return (0 < removed);
}

RequestNumber Context::Request() const
{
    return request_number_.load();
}

proto::Context Context::serialize(
    const Lock& lock,
    const proto::ConsensusType type) const
{
    OT_ASSERT(verify_write_lock(lock));

    proto::Context output;

    output.set_version(version_);
    output.set_type(type);

    if (nym_) {
        output.set_localnym(String(nym_->ID()).Get());
    }

    if (remote_nym_) {
        output.set_remotenym(String(remote_nym_->ID()).Get());
    }

    output.set_localnymboxhash(String(local_nymbox_hash_).Get());
    output.set_remotenymboxhash(String(remote_nymbox_hash_).Get());
    output.set_requestnumber(request_number_.load());

    for (const auto& it : acknowledged_request_numbers_) {
        output.add_acknowledgedrequestnumber(it);
    }

    for (const auto& it : available_transaction_numbers_) {
        output.add_availabletransactionnumber(it);
    }

    for (const auto& it : issued_transaction_numbers_) {
        output.add_issuedtransactionnumber(it);
    }

    return output;
}

OTData Context::Serialize() const
{
    return proto::ProtoAsData(Serialized());
}

proto::Context Context::Serialized() const
{
    Lock lock(lock_);

    return contract(lock);
}

void Context::SetLocalNymboxHash(const Identifier& hash)
{
    Lock lock(lock_);

    local_nymbox_hash_ = hash;

    CalculateID(lock);
}

void Context::SetRemoteNymboxHash(const Identifier& hash)
{
    Lock lock(lock_);

    remote_nymbox_hash_ = hash;

    CalculateID(lock);
}

void Context::SetRequest(const RequestNumber req)
{
    request_number_.store(req);
}

proto::Context Context::SigVersion(const Lock& lock) const
{
    OT_ASSERT(verify_write_lock(lock));

    auto output = serialize(lock, Type());
    output.clear_signature();

    return output;
}

bool Context::update_signature(const Lock& lock)
{
    if (!ot_super::update_signature(lock)) { return false; }

    bool success = false;

    signatures_.clear();
    auto serialized = SigVersion(lock);
    auto& signature = *serialized.mutable_signature();
    signature.set_version(2);
    signature.set_role(proto::SIGROLE_CONTEXT);
    success = nym_->SignProto(serialized, signature);

    if (success) {
        signatures_.emplace_front(new proto::Signature(signature));
    } else {
        otErr << __FUNCTION__ << ": failed to create signature."
              << std::endl;
    }

    return success;
}

bool Context::validate(const Lock& lock) const
{
    if (1 != signatures_.size()) { return false; }

    return verify_signature(lock, *signatures_.front());
}

bool Context::VerifyAcknowledgedNumber(const RequestNumber req) const
{
    return (0 < acknowledged_request_numbers_.count(req));
}

bool Context::verify_signature(
    const Lock& lock,
    const proto::Signature& signature) const
{
    if (!ot_super::verify_signature(lock, signature)) { return false; }

    auto serialized = SigVersion(lock);
    auto& sigProto = *serialized.mutable_signature();
    sigProto.CopyFrom(signature);

    return nym_->VerifyProto(serialized, sigProto);
}
} // namespace opentxs
