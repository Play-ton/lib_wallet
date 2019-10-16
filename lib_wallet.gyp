# This file is part of Desktop App Toolkit,
# a set of libraries for developing nice desktop applications.
#
# For license and copyright information please follow this link:
# https://github.com/desktop-app/legal/blob/master/LEGAL

{
  'includes': [
    '../gyp/helpers/common/common.gypi',
  ],
  'targets': [{
    'target_name': 'lib_wallet',
    'includes': [
      '../gyp/helpers/common/library.gypi',
      '../gyp/helpers/modules/qt.gypi',
      '../gyp/helpers/modules/pch.gypi',
      '../gyp/helpers/modules/openssl.gypi',
      '../lib_ui/gyp/styles_rule.gypi',
      '../lib_ui/gyp/qrc_rule.gypi',
    ],
    'variables': {
      'src_loc': '.',
      'style_files': [
        '<(src_loc)/wallet/wallet.style',
      ],
      'qrc_files': [
        '<(src_loc)/qrc/wallet.qrc',
      ],
      'dependent_style_files': [
        '<(submodules_loc)/lib_ui/ui/colors.palette',
        '<(submodules_loc)/lib_ui/ui/basic.style',
        '<(submodules_loc)/lib_ui/ui/layers/layers.style',
        '<(submodules_loc)/lib_ui/ui/widgets/widgets.style',
      ],
      'style_timestamp': '<(SHARED_INTERMEDIATE_DIR)/update_dependent_styles_wallet.timestamp',
      'qrc_timestamp': '<(SHARED_INTERMEDIATE_DIR)/update_dependent_qrc_wallet.timestamp',
      'pch_source': '<(src_loc)/wallet/wallet_pch.cpp',
      'pch_header': '<(src_loc)/wallet/wallet_pch.h',
    },
    'dependencies': [
      '<(submodules_loc)/lib_ton/lib_ton.gyp:lib_ton',
      '<(submodules_loc)/lib_ui/lib_ui.gyp:lib_ui',
      '<(submodules_loc)/lib_lottie/lib_lottie.gyp:lib_lottie',
    ],
    'export_dependent_settings': [
      '<(submodules_loc)/lib_ton/lib_ton.gyp:lib_ton',
      '<(submodules_loc)/lib_ui/lib_ui.gyp:lib_ui',
      '<(submodules_loc)/lib_lottie/lib_lottie.gyp:lib_lottie',
    ],
    'sources': [
      '<@(qrc_files)',
      '<@(style_files)',
      '<(src_loc)/ui/address_label.cpp',
      '<(src_loc)/ui/address_label.h',
      '<(src_loc)/wallet/wallet_common.cpp',
      '<(src_loc)/wallet/wallet_common.h',
	  '<(src_loc)/wallet/wallet_confirm_transaction.cpp',
	  '<(src_loc)/wallet/wallet_confirm_transaction.h',
      '<(src_loc)/wallet/wallet_cover.cpp',
      '<(src_loc)/wallet/wallet_cover.h',
      '<(src_loc)/wallet/wallet_delete.cpp',
      '<(src_loc)/wallet/wallet_delete.h',
      '<(src_loc)/wallet/wallet_empty_history.cpp',
      '<(src_loc)/wallet/wallet_empty_history.h',
      '<(src_loc)/wallet/wallet_enter_passcode.cpp',
      '<(src_loc)/wallet/wallet_enter_passcode.h',
      '<(src_loc)/wallet/wallet_history.cpp',
      '<(src_loc)/wallet/wallet_history.h',
      '<(src_loc)/wallet/wallet_info.cpp',
      '<(src_loc)/wallet/wallet_info.h',
      '<(src_loc)/wallet/wallet_intro.cpp',
      '<(src_loc)/wallet/wallet_intro.h',
      '<(src_loc)/wallet/wallet_phrases.cpp',
      '<(src_loc)/wallet/wallet_phrases.h',
      '<(src_loc)/wallet/wallet_receive_grams.cpp',
      '<(src_loc)/wallet/wallet_receive_grams.h',
      '<(src_loc)/wallet/wallet_send_grams.cpp',
      '<(src_loc)/wallet/wallet_send_grams.h',
      '<(src_loc)/wallet/wallet_top_bar.cpp',
      '<(src_loc)/wallet/wallet_top_bar.h',
      '<(src_loc)/wallet/wallet_view_transaction.cpp',
      '<(src_loc)/wallet/wallet_view_transaction.h',
      '<(src_loc)/wallet/wallet_window.cpp',
      '<(src_loc)/wallet/wallet_window.h',
    ],
    'include_dirs': [
      '<(src_loc)',
    ],
    'direct_dependent_settings': {
      'include_dirs': [
        '<(src_loc)',
      ],
    },
  }],
}
