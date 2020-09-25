// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "wallet/wallet_view_transaction.h"

#include "wallet/wallet_common.h"
#include "wallet/wallet_phrases.h"
#include "ui/amount_label.h"
#include "ui/address_label.h"
#include "ui/lottie_widget.h"
#include "ui/wrap/padding_wrap.h"
#include "ui/widgets/buttons.h"
#include "ui/text/text_utilities.h"
#include "ton/ton_state.h"
#include "base/unixtime.h"
#include "styles/style_layers.h"
#include "styles/style_wallet.h"
#include "styles/palette.h"
#include "ton/ton_wallet.h"

#include <QtCore/QDateTime>
#include <QtGui/QtEvents>
#include <iostream>

namespace Wallet {
namespace {

struct TokenTransaction {
	Ton::TokenKind token;
	QString recipient;
	int64 amount;
	bool isSwapBack;
};

std::optional<TokenTransaction> TryGetTokenTransaction(
		const Ton::Transaction &data,
		const QString &tokenContractAddress,
		Ton::TokenKind selectedToken) {
	for (const auto& out : data.outgoing) {
		if (Ton::Wallet::ConvertIntoRaw(out.destination) == tokenContractAddress) {
			auto transaction = Ton::Wallet::ParseTokenTransaction(out.message);
			if (transaction.has_value() && !Ton::CheckTokenTransaction(selectedToken, *transaction)) {
				transaction.reset();
			}
			if (!transaction.has_value()) {
				continue;
			}

			return v::match(*transaction, [=](const Ton::TokenTransfer& transfer) {
				return TokenTransaction {
					.token = selectedToken,
					.recipient = transfer.dest,
					.amount = transfer.value,
					.isSwapBack = false,
				};
			}, [=](const Ton::TokenSwapBack& swapBack) {
				return TokenTransaction {
					.token = selectedToken,
					.recipient = swapBack.dest,
					.amount = swapBack.value,
					.isSwapBack = true,
				};
			});
		}
	}
	return {};
}

object_ptr<Ui::RpWidget> CreateSummary(
		not_null<Ui::RpWidget*> parent,
		const Ton::Transaction &data,
		const std::optional<TokenTransaction> &tokenTransaction) {
	const auto isTokenTransaction = tokenTransaction.has_value();
	const auto token = isTokenTransaction
		? tokenTransaction->token
		: Ton::TokenKind::DefaultToken;

	const auto showTransactionFee = isTokenTransaction || data.otherFee > 0;
	const auto showStorageFee = !isTokenTransaction && data.storageFee;

	const auto feeSkip = st::walletTransactionFeeSkip;
	const auto secondFeeSkip = st::walletTransactionSecondFeeSkip;
	const auto service = IsServiceTransaction(data);
	const auto height = st::walletTransactionSummaryHeight
		- (service ? st::walletTransactionValue.diamond : 0)
		+ (showTransactionFee ? (st::normalFont->height + feeSkip) : 0)
		+ (showStorageFee
			? (st::normalFont->height
				+ (showTransactionFee ? secondFeeSkip : feeSkip))
			: 0);
	auto result = object_ptr<Ui::FixedHeightWidget>(
		parent,
		height);

	const auto value = isTokenTransaction
		? -tokenTransaction->amount
		: CalculateValue(data);
	const auto balance = !service
		? result->lifetime().make_state<Ui::AmountLabel>(
			result.data(),
			rpl::single(FormatAmount(value, token, FormatFlag::Signed)),
			st::walletTransactionValue)
		: nullptr;

	const auto otherFee = showTransactionFee
		? Ui::CreateChild<Ui::FlatLabel>(
			result.data(),
			ph::lng_wallet_view_transaction_fee(ph::now).replace(
				"{amount}",
				FormatAmount(
					isTokenTransaction
						? -CalculateValue(data)
						: data.otherFee,
					Ton::TokenKind::DefaultToken).full),
			st::walletTransactionFee)
		: nullptr;

	const auto storageFee = showStorageFee
		? Ui::CreateChild<Ui::FlatLabel>(
			result.data(),
			ph::lng_wallet_view_storage_fee(ph::now).replace(
				"{amount}",
				FormatAmount(data.storageFee, Ton::TokenKind::DefaultToken).full),
			st::walletTransactionFee)
		: nullptr;

	rpl::combine(
		result->widthValue(),
		balance ? balance->widthValue() : rpl::single(0),
		otherFee ? otherFee->widthValue() : rpl::single(0),
		storageFee ? storageFee->widthValue() : rpl::single(0)
	) | rpl::start_with_next([=](int width, int bwidth, int, int) {
		auto top = st::walletTransactionValueTop;

		if (balance) {
			balance->move((width - bwidth) / 2, top);
			top += balance->height() + feeSkip;
		}
		if (otherFee) {
			otherFee->move((width - otherFee->width()) / 2, top);
			top += otherFee->height() + secondFeeSkip;
		}
		if (storageFee) {
			storageFee->move((width - storageFee->width()) / 2, top);
		}
	}, result->lifetime());

	return result;
}

void SetupScrollByDrag(
		not_null<Ui::BoxContent*> box,
		not_null<Ui::RpWidget*> child) {
	auto moves = child->events(
	) | rpl::filter([=](not_null<QEvent*> event) {
		return (event->type() == QEvent::MouseMove);
	});
	auto pressed = child->events(
	) | rpl::filter([=](not_null<QEvent*> event) {
		const auto type = event->type();
		static constexpr auto kLeft = Qt::LeftButton;
		return ((type == QEvent::MouseButtonPress)
			|| (type == QEvent::MouseButtonRelease))
			&& (static_cast<QMouseEvent*>(event.get())->button() == kLeft);
	}) | rpl::map([=](not_null<QEvent*> event) {
		return (event->type() == QEvent::MouseButtonPress);
	});
	auto pressedY = rpl::combine(
		std::move(pressed),
		std::move(moves)
	) | rpl::filter([](bool pressed, not_null<QEvent*> move) {
		return pressed;
	}) | rpl::map([](bool pressed, not_null<QEvent*> move) {
		const auto pos = static_cast<QMouseEvent*>(move.get())->globalPos();
		return pressed ? std::make_optional(pos.y()) : std::nullopt;
	}) | rpl::distinct_until_changed();

	rpl::combine(
		std::move(pressedY),
		box->geometryValue()
	) | rpl::start_with_next([=](std::optional<int> y, QRect geometry) {
		if (!y) {
			box->onDraggingScrollDelta(0);
			return;
		}
		const auto parent = box->parentWidget();
		const auto global = parent->mapToGlobal(geometry.topLeft());
		const auto top = global.y();
		const auto bottom = top + geometry.height();
		const auto delta = (*y < global.y())
			? (*y - top)
			: (*y > bottom)
			? (*y - bottom)
			: 0;
		box->onDraggingScrollDelta(delta);
	}, child->lifetime());
}

} // namespace

void ViewTransactionBox(
		not_null<Ui::GenericBox*> box,
		Ton::Transaction &&data,
		const QString& tokenContractAddress,
		Ton::TokenKind selectedToken,
		rpl::producer<
			not_null<std::vector<Ton::Transaction>*>> collectEncrypted,
		rpl::producer<
			not_null<const std::vector<Ton::Transaction>*>> decrypted,
		const Fn<void(QImage, QString)> &share,
		const Fn<void()> &decryptComment,
		const Fn<void(QString)> &send,
		const Fn<void(QString)> &reveal) {
	struct DecryptedText {
		QString text;
		bool success = false;
	};

	const auto tokenTransaction = TryGetTokenTransaction(
		data,
		tokenContractAddress,
		selectedToken);
	const auto isTokenTransaction = tokenTransaction.has_value();
	const auto isSwapBack = isTokenTransaction && tokenTransaction->isSwapBack;

	const auto service = IsServiceTransaction(data);

	box->setTitle(data.initializing
		? ph::lng_wallet_row_init()
		: service
		? ph::lng_wallet_row_service()
		: ph::lng_wallet_view_title());
	box->setStyle(service ? st::walletNoButtonsBox : st::walletBox);

	const auto id = data.id;
	const auto address = isTokenTransaction
		? tokenTransaction->recipient
		: ExtractAddress(data);
	const auto incoming = data.outgoing.empty();
	const auto encryptedComment = IsEncryptedMessage(data);
	const auto decryptedComment = encryptedComment
		? QString()
		: ExtractMessage(data);
	const auto hasComment = encryptedComment || !decryptedComment.isEmpty();
	auto decryptedText = rpl::producer<DecryptedText>();
	auto complexComment = [&] {
		decryptedText = std::move(
			decrypted
		) | rpl::map([=](
				not_null<const std::vector<Ton::Transaction>*> list) {
			const auto i = ranges::find(*list, id, &Ton::Transaction::id);
			return (i != end(*list)) ? std::make_optional(*i) : std::nullopt;
		}) | rpl::filter([=](const std::optional<Ton::Transaction> &value) {
			return value.has_value();
		}) | rpl::map([=](const std::optional<Ton::Transaction> &value) {
			return IsEncryptedMessage(*value)
				? DecryptedText{
					ph::lng_wallet_decrypt_failed(ph::now),
					false
				}
				: DecryptedText{ ExtractMessage(*value), true };
		}) | rpl::take(1) | rpl::start_spawning(box->lifetime());

		return rpl::single(
			Ui::Text::Link(ph::lng_wallet_click_to_decrypt(ph::now))
		) | rpl::then(rpl::duplicate(
			decryptedText
		) | rpl::map([=](const DecryptedText &decrypted) {
			return decrypted.text;
		}) | Ui::Text::ToWithEntities());
	};
	auto message = IsEncryptedMessage(data)
		? (complexComment() | rpl::type_erased())
		: rpl::single(Ui::Text::WithEntities(ExtractMessage(data)));

	box->addTopButton(st::boxTitleClose, [=] { box->closeBox(); });

	box->addRow(CreateSummary(box, data, tokenTransaction));

	if (!service) {
		AddBoxSubtitle(box, incoming
			? ph::lng_wallet_view_sender()
			: ph::lng_wallet_view_recipient());
		box->addRow(
			object_ptr<Ui::RpWidget>::fromRaw(Ui::CreateAddressLabel(
				box,
				address,
				st::walletTransactionAddress,
				[=] { share(QImage(), address); })),
			{
				st::boxRowPadding.left(),
				st::boxRowPadding.top(),
				st::boxRowPadding.right(),
				st::walletTransactionDateTop,
			});
	}

	AddBoxSubtitle(box, ph::lng_wallet_view_date());
	box->addRow(
		object_ptr<Ui::FlatLabel>(
			box,
			base::unixtime::parse(
				data.time
			).toString(Qt::DefaultLocaleLongDate),
			st::walletLabel),
		{
			st::boxRowPadding.left(),
			st::boxRowPadding.top(),
			st::boxRowPadding.right(),
			(hasComment
				? st::walletTransactionCommentTop
				: st::boxRowPadding.bottom()),
		});

	if (hasComment) {
		AddBoxSubtitle(box, ph::lng_wallet_view_comment());
		const auto comment = box->addRow(object_ptr<Ui::FlatLabel>(
			box,
			std::move(message),
			st::walletLabel));
		if (IsEncryptedMessage(data)) {
			std::move(
				decryptedText
			) | rpl::map([=](const DecryptedText &decrypted) {
				return decrypted.success;
			}) | rpl::start_with_next([=](bool success) {
				comment->setSelectable(success);
				if (!success) {
					comment->setTextColorOverride(st::boxTextFgError->c);
				}
			}, comment->lifetime());

			std::move(
				collectEncrypted
			) | rpl::take(
				1
			) | rpl::start_with_next([=](
				not_null<std::vector<Ton::Transaction>*> list) {
				list->push_back(data);
			}, comment->lifetime());

			comment->setClickHandlerFilter([=](const auto &...) {
				decryptComment();
				return false;
			});
		} else {
			comment->setSelectable(true);
		}
		SetupScrollByDrag(box, comment);
	}

	box->addRow(object_ptr<Ui::FixedHeightWidget>(
		box,
		st::walletTransactionBottomSkip));

	if (!service) {
		auto text = isSwapBack
			? ph::lng_wallet_view_reveal()
			: incoming
				? ph::lng_wallet_view_send_to_address()
				: ph::lng_wallet_view_send_to_recipient();
		box->addButton(
			std::move(text) | rpl::map([selectedToken](QString &&text) {
				return text.replace("{ticker}", Ton::toString(selectedToken));
			}),
			[=] {
				if (isSwapBack) {
					reveal(address);
				} else {
					send(address);
				}
			},
			st::walletBottomButton
		)->setTextTransform(Ui::RoundButton::TextTransform::NoTransform);
	}
}

} // namespace Wallet
