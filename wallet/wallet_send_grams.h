// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "ui/layers/generic_box.h"

namespace Ton {
enum class TokenKind;
struct WalletState;
} // namespace Ton

namespace Wallet {

struct PreparedInvoice;

enum class InvoiceField {
	Address,
	Amount,
	Comment,
};

void SendGramsBox(
	not_null<Ui::GenericBox*> box,
	const std::variant<PreparedInvoice, QString> &invoice,
    rpl::producer<Ton::WalletState> state,
	rpl::producer<std::optional<Ton::TokenKind>> selectedToken,
	const Fn<void(PreparedInvoice, Fn<void(InvoiceField)> error)> &done);

} // namespace Wallet
