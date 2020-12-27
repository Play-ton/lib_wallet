// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "wallet/wallet_cover.h"

#include "wallet/wallet_common.h"
#include "wallet/wallet_phrases.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/buttons.h"
#include "ui/amount_label.h"
#include "ui/lottie_widget.h"
#include "ui/inline_token_icon.h"
#include "ton/ton_state.h"
#include "styles/style_wallet.h"
#include "styles/palette.h"

#include <iostream>

namespace Wallet {
namespace {

not_null<Ui::RoundButton*> CreateCoverButton(
		not_null<QWidget*> parent,
		rpl::producer<QString> text,
		const style::icon &icon) {
	const auto result = Ui::CreateChild<Ui::RoundButton>(
		parent.get(),
		rpl::single(QString()),
		st::walletCoverButton);
	const auto label = Ui::CreateChild<Ui::FlatLabel>(
		result,
		std::move(text),
		st::walletCoverButtonLabel);
	label->setAttribute(Qt::WA_TransparentForMouseEvents);
	label->paintRequest(
	) | rpl::start_with_next([=, &icon](QRect clip) {
		auto p = QPainter(label);
		icon.paint(p, st::walletCoverIconPosition, label->width());
	}, label->lifetime());
	rpl::combine(
		result->widthValue(),
		label->widthValue()
	) | rpl::start_with_next([=](int outer, int width) {
		label->move((outer - width) / 2, st::walletCoverButton.textTop);
	}, label->lifetime());
	return result;
}

} // namespace

auto CoverState::selectedToken() const -> Ton::TokenKind {
	return v::match(asset, [](const SelectedToken &selectedToken) {
		return selectedToken.token;
	}, [&](const SelectedDePool &/*selectedDePool*/) {
		return Ton::TokenKind::DefaultToken;
	});
}

Cover::Cover(not_null<Ui::RpWidget*> parent, rpl::producer<CoverState> state)
: _widget(parent)
, _state(std::move(state)) {
	setupControls();
}

void Cover::setGeometry(QRect geometry) {
	_widget.setGeometry(geometry);
}

int Cover::height() const {
	return _widget.height();
}

rpl::producer<> Cover::sendRequests() const {
	return _sendRequests.events();
}

rpl::producer<> Cover::receiveRequests() const {
	return _receiveRequests.events();
}

rpl::lifetime &Cover::lifetime() {
	return _widget.lifetime();
}

void Cover::setupBalance() {
	auto amount = _state.value(
	) | rpl::map([](const CoverState &state) {
		return FormatAmount(
			std::max(state.unlockedBalance, 0LL),
			state.selectedToken(),
			FormatFlag::Rounded);
	});

	const auto balance = _widget.lifetime().make_state<Ui::AmountLabel>(
		&_widget,
		std::move(amount),
		st::walletCoverBalance);
	rpl::combine(
		_widget.sizeValue(),
		balance->widthValue()
	) | rpl::start_with_next([=](QSize size, int width) {
		const auto blockTop = (size.height()
			+ st::walletTopBarHeight
			- st::walletCoverInner) / 2 - st::walletTopBarHeight;
		const auto balanceTop = blockTop + st::walletCoverBalanceTop;
		balance->move((size.width() - width) / 2, balanceTop);
	}, balance->lifetime());

	auto lockedAmount = _state.value(
	) | rpl::map([](const CoverState &state) {
		return v::match(state.asset, [&](const SelectedToken &selectedToken) {
			return (state.lockedBalance > 0)
				? FormatAmount(state.lockedBalance, selectedToken.token, FormatFlag::Rounded).full
				: QString();
		}, [&](const SelectedDePool &selectedDePool) {
			return FormatAmount(state.lockedBalance, Ton::TokenKind::DefaultToken, FormatFlag::Rounded).full;
		});
	});

	const auto label = Ui::CreateChild<Ui::FlatLabel>(
		&_widget,
		(_state.current().useTestNetwork
			? ph::lng_wallet_cover_balance_test()
			: ph::lng_wallet_cover_balance()),
		st::walletCoverLabel);
	rpl::combine(
		_widget.sizeValue(),
		label->widthValue()
	) | rpl::start_with_next([=](QSize size, int width) {
		const auto blockTop = (size.height()
			+ st::walletTopBarHeight
			- st::walletCoverInner) / 2 - st::walletTopBarHeight;
		label->moveToLeft(
			(size.width() - width) / 2,
			blockTop + st::walletCoverLabelTop,
			size.width());
	}, label->lifetime());
	label->show();

	const auto locked = Ui::CreateChild<Ui::RpWidget>(
		&_widget);
	const auto lockedBalance = Ui::CreateChild<Ui::FlatLabel>(
		locked,
		rpl::duplicate(lockedAmount),
		st::walletCoverLocked);
	const auto lockedLabel = Ui::CreateChild<Ui::FlatLabel>(
		locked,
		ph::lng_wallet_cover_locked(),
		st::walletCoverLockedLabel);
	rpl::combine(
		lockedBalance->sizeValue(),
		lockedLabel->sizeValue()
	) | rpl::start_with_next([=](QSize balance, QSize label) {
		locked->resize(balance.width()
			+ st::walletDiamondSize
			+ st::walletCoverLocked.style.font->spacew
			+ label.width(),
			std::max(balance.height(), label.height()));
		lockedBalance->moveToRight(st::walletDiamondSize, 0);
		lockedLabel->moveToLeft(0, 0);
	}, locked->lifetime());
	rpl::combine(
		_widget.sizeValue(),
		locked->widthValue()
	) | rpl::start_with_next([=](QSize size, int width) {
		const auto blockTop = (size.height()
			+ st::walletTopBarHeight
			- st::walletCoverInner) / 2 - st::walletTopBarHeight;
		locked->moveToLeft(
			(size.width() - width) / 2,
			blockTop + st::walletCoverLabelTop,
			size.width());
	}, locked->lifetime());
	locked->show();

	rpl::combine(
		locked->paintRequest()
	) | rpl::start_with_next([=](const QRect &) {
		auto p = QPainter(locked);
		const auto diamondTop = 0;
		const auto diamondLeft = locked->width() - st::walletDiamondSize;
		Ui::PaintInlineTokenIcon(
			_state.current().selectedToken(),
			p,
			diamondLeft,
			diamondTop,
			st::walletCoverLocked.style.font);
	}, locked->lifetime());

	_state.value() | rpl::start_with_next([=](const CoverState &) {
		locked->update();
	}, locked->lifetime());

	std::move(
		lockedAmount
	) | rpl::map([](const QString &text) {
		return !text.isEmpty();
	}) | rpl::distinct_until_changed(
	) | rpl::start_with_next([=](bool hasLocked) {
		label->setVisible(!hasLocked);
		locked->setVisible(hasLocked);
	}, locked->lifetime());
}

void Cover::setupControls() {
	const auto syncLifetime = _widget.lifetime().make_state<rpl::lifetime>();
	const auto sync = syncLifetime->make_state<Ui::LottieAnimation>(
		&_widget,
		Ui::LottieFromResource("intro"));
	sync->start();

	_widget.sizeValue(
	) | rpl::start_with_next([=](QSize size) {
		const auto diamond = st::walletCoverBalance.diamond;
		const auto blockTop = (size.height()
			+ st::walletTopBarHeight
			- st::walletCoverInner) / 2 - st::walletTopBarHeight;
		const auto balanceTop = blockTop + st::walletCoverBalanceTop;
		sync->setGeometry(
			{ (size.width() - diamond) / 2, balanceTop, diamond, diamond });
	}, *syncLifetime);

	_state.value(
	) | rpl::filter([](const CoverState &state) {
		return state.justCreated
			|| (state.unlockedBalance != Ton::kUnknownBalance);
	}) | rpl::take(1) | rpl::start_with_next([=] {
		syncLifetime->destroy();
		setupBalance();
	}, *syncLifetime);

	auto hasUnlockedFunds = _state.value(
	) | rpl::map([](const CoverState &state) {
		return state.unlockedBalance > 0;
	}) | rpl::distinct_until_changed();

	auto selectedToken = _state.value(
    ) | rpl::map([](const CoverState &state) {
        return state.selectedToken();
    }) | rpl::distinct_until_changed();

    const auto replaceTickerTag = [] {
        return rpl::map([=](QString &&text, const std::optional<Ton::TokenKind> &selectedToken) {
            return text.replace("{ticker}", Ton::toString(selectedToken.value_or(Ton::TokenKind::DefaultToken)));
        });
    };

	const auto receive = CreateCoverButton(
		&_widget,
		rpl::conditional(
			rpl::duplicate(hasUnlockedFunds),
			ph::lng_wallet_cover_receive(),
			rpl::combine(
				ph::lng_wallet_cover_receive_full(),
				rpl::duplicate(selectedToken)
			) | replaceTickerTag()
		),
		st::walletCoverReceiveIcon);

	const auto send = CreateCoverButton(
		&_widget,
		_state.value(
		) | rpl::map([](const CoverState &state) {
			return v::match(state.asset, [](const SelectedToken &/*selectedToken*/) {
				return ph::lng_wallet_cover_send();
			}, [](const SelectedDePool &/*selectedDePool*/) {
				return ph::lng_wallet_cover_stake();
			});
		}) | rpl::flatten_latest(),
		st::walletCoverSendIcon);
	send->setTextTransform(Ui::RoundButton::TextTransform::NoTransform);

	rpl::combine(
		_state.value(),
		_widget.sizeValue(),
		std::move(hasUnlockedFunds)
	) | rpl::start_with_next([=](const CoverState &state, QSize size, bool hasUnlockedFunds) {
		const auto fullWidth = st::walletCoverButtonWidthFull;
		const auto left = (size.width() - fullWidth) / 2;
		const auto top = size.height()
			- st::walletCoverButtonBottom
			- receive->height();

		const auto [showSend, showReceive] = v::match(state.asset, [&](const SelectedToken &selectedToken) {
			return std::make_pair(hasUnlockedFunds, true);
		}, [&](const SelectedDePool &/*selectedDePool*/) {
			return std::make_pair(true, hasUnlockedFunds);
		});

		receive->setVisible(showReceive);
		receive->resizeToWidth(showReceive && showSend
			? st::walletCoverButtonWidth
			: fullWidth);
		receive->moveToLeft(left, top, size.width());

		send->setVisible(showSend);
		send->resizeToWidth(showReceive && showSend
			? st::walletCoverButtonWidth
			: fullWidth);
		send->moveToLeft(left, top, size.width());

		if (showReceive && showSend) {
			send->moveToLeft(left + fullWidth - send->width(),
							 top,
							 size.width());
		}
	}, receive->lifetime());

	receive->clicks(
	) | rpl::map([] {
		return rpl::empty_value();
	}) | rpl::start_to_stream(_receiveRequests, receive->lifetime());

	send->clicks(
	) | rpl::map([] {
		return rpl::empty_value();
	}) | rpl::start_to_stream(_sendRequests, send->lifetime());

	_widget.paintRequest(
	) | rpl::start_with_next([=](QRect clip) {
		QPainter(&_widget).fillRect(clip, st::walletTopBg);
	}, lifetime());
}

rpl::producer<CoverState> MakeCoverState(
		rpl::producer<Ton::WalletViewerState> state,
		rpl::producer<std::optional<SelectedAsset>> selectedAsset,
		bool justCreated,
		bool useTestNetwork) {
	return rpl::combine(
		std::move(state),
		std::move(selectedAsset)
	) | rpl::map([=](const Ton::WalletViewerState &data, const std::optional<SelectedAsset> &asset) {
		const auto &account = data.wallet.account;

		CoverState result{
			.asset = asset.value_or(SelectedToken { .token = Ton::TokenKind::DefaultToken }),
			.unlockedBalance = 0,
			.lockedBalance = 0,
			.justCreated = justCreated,
			.useTestNetwork = useTestNetwork,
		};

		v::match(result.asset, [&](const SelectedToken &selectedToken) {
			if (selectedToken.token == Ton::TokenKind::DefaultToken) {
				result.unlockedBalance = account.fullBalance - account.lockedBalance;
				result.lockedBalance = account.lockedBalance;
			} else {
				const auto it = data.wallet.tokenStates.find(selectedToken.token);
				if (it != data.wallet.tokenStates.end()) {
					result.unlockedBalance = it->second.fullBalance;
				}
			}
		}, [&](const SelectedDePool &selectedDePool) {
			const auto it = data.wallet.dePoolParticipantStates.find(selectedDePool.address);
			if (it != data.wallet.dePoolParticipantStates.end()) {
				result.unlockedBalance = it->second.withdrawValue;
				result.lockedBalance = it->second.total;
			}
		});

		return result;
	});
}

} // namespace Wallet
