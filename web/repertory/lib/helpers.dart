// helpers.dart

import 'package:convert/convert.dart';
import 'package:collection/collection.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/models/auth.dart';
import 'package:repertory/widgets/app_dropdown.dart';
import 'package:flutter_settings_ui/flutter_settings_ui.dart';
import 'package:sodium_libs/sodium_libs.dart' show SecureKey, StringX;

Future doShowDialog(BuildContext context, Widget child) => showDialog(
  context: context,
  builder: (context) {
    final theme = Theme.of(context);
    final scheme = theme.colorScheme;
    return Theme(
      data: theme.copyWith(
        dialogTheme: DialogThemeData(
          backgroundColor: darken(
            scheme.primary,
            0.95,
          ).withValues(alpha: constants.dropDownAlpha),
          surfaceTintColor: Colors.transparent,
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(constants.borderRadius),
            side: BorderSide(
              color: scheme.outlineVariant.withValues(
                alpha: constants.outlineAlpha,
              ),
              width: 1,
            ),
          ),
        ),
      ),
      child: child,
    );
  },
);

typedef Validator = bool Function(String);

class NullPasswordException implements Exception {
  String error() => 'password cannot be null';
}

class AuthenticationFailedException implements Exception {
  String error() => 'failed to authenticate user';
}

// ignore: prefer_function_declarations_over_variables
final Validator noRestrictedChars = (value) {
  return [
        '!',
        '"',
        '\$',
        '&',
        "'",
        '(',
        ')',
        '*',
        ';',
        '<',
        '>',
        '?',
        '[',
        ']',
        '`',
        '{',
        '}',
        '|',
      ].firstWhereOrNull((char) => value.contains(char)) ==
      null;
};

// ignore: prefer_function_declarations_over_variables
final Validator notEmptyValidator = (value) => value.isNotEmpty;

// ignore: prefer_function_declarations_over_variables
final Validator portIsValid = (value) {
  int? intValue = int.tryParse(value);
  if (intValue == null) {
    return false;
  }

  return (intValue > 0 && intValue < 65536);
};

// ignore: prefer_function_declarations_over_variables
final Validator trimNotEmptyValidator = (value) => value.trim().isNotEmpty;

createUriValidator<Validator>({host, port}) {
  return (value) =>
      Uri.tryParse('http://${host ?? value}:${port ?? value}/') != null;
}

List<Validator> createHostNameOrIpValidators() => <Validator>[
  trimNotEmptyValidator,
  createUriValidator(port: 9000),
];

Map<String, dynamic> createDefaultSettings(String mountType) {
  switch (mountType) {
    case 'Encrypt':
      return {
        'EncryptConfig': {'EncryptionToken': '', 'Path': ''},
      };
    case 'Remote':
      return {
        'RemoteConfig': {
          'ApiPort': 20000,
          'EncryptionToken': '',
          'HostNameOrIp': '',
        },
      };
    case 'S3':
      return {
        'S3Config': {
          'AccessKey': '',
          'Bucket': '',
          'ForceLegacyEncryption': false,
          'Region': 'any',
          'SecretKey': '',
          'URL': '',
          'UsePathStyle': false,
          'UseRegionInURL': false,
        },
      };
    case 'Sia':
      return {
        'HostConfig': {
          'ApiPassword': '',
          'ApiPort': 9980,
          'HostNameOrIp': 'localhost',
        },
        'SiaConfig': {'Bucket': ''},
      };
  }

  return {};
}

void displayAuthError(Auth auth) {
  if (!auth.authenticated || constants.navigatorKey.currentContext == null) {
    return;
  }

  displayErrorMessage(
    constants.navigatorKey.currentContext!,
    "Authentication failed",
    clear: true,
  );
}

void displayErrorMessage(
  BuildContext context,
  String text, {
  bool clear = false,
}) {
  if (!context.mounted) {
    return;
  }

  final messenger = ScaffoldMessenger.of(context);
  if (clear) {
    messenger.removeCurrentSnackBar();
  }

  messenger.showSnackBar(
    SnackBar(content: Text(text, textAlign: TextAlign.center)),
  );
}

String formatMountName(String type, String name) {
  if (type == 'remote') {
    return name.replaceAll('_', ':');
  }

  return name;
}

String getBaseUri() {
  if (kDebugMode || !kIsWeb) {
    return 'http://127.0.0.1:30000';
  }

  return Uri.base.origin;
}

String? getSettingDescription(String settingPath) {
  switch (settingPath) {
    case 'ApiPassword':
      return "HTTP authentication password";
    case 'ApiUser':
      return "HTTP authentication user";
    case 'HostConfig.ApiPassword':
      return "RENTERD_API_PASSWORD";
    case 'S3Config.ForceLegacyEncryption':
      return "Effectively disables Argon2id KDF";
    default:
      return null;
  }
}

List<Validator> getSettingValidators(String settingPath) {
  switch (settingPath) {
    case 'ApiPassword':
      return [notEmptyValidator];
    case 'DatabaseType':
      return [(value) => constants.databaseTypeList.contains(value)];
    case 'PreferredDownloadType':
      return [(value) => constants.downloadTypeList.contains(value)];
    case 'EventLevel':
      return [(value) => constants.eventLevelList.contains(value)];
    case 'EncryptConfig.EncryptionToken':
      return [notEmptyValidator];
    case 'EncryptConfig.Path':
      return [trimNotEmptyValidator, noRestrictedChars];
    case 'HostConfig.ApiPassword':
      return [notEmptyValidator];
    case 'HostConfig.ApiPort':
      return [portIsValid];
    case 'HostConfig.HostNameOrIp':
      return createHostNameOrIpValidators();
    case 'HostConfig.Protocol':
      return [(value) => constants.protocolTypeList.contains(value)];
    case 'Path':
      return [trimNotEmptyValidator];
    case 'RemoteConfig.ApiPort':
      return [notEmptyValidator, portIsValid];
    case 'RemoteConfig.EncryptionToken':
      return [notEmptyValidator];
    case 'RemoteConfig.HostNameOrIp':
      return createHostNameOrIpValidators();
    case 'RemoteMount.ApiPort':
      return [notEmptyValidator, portIsValid];
    case 'RemoteMount.EncryptionToken':
      return [notEmptyValidator];
    case 'RemoteMount.HostNameOrIp':
      return createHostNameOrIpValidators();
    case 'RingBufferFileSize':
      return [(value) => constants.ringBufferSizeList.contains(value)];
    case 'S3Config.AccessKey':
      return [trimNotEmptyValidator];
    case 'S3Config.Bucket':
      return [trimNotEmptyValidator];
    case 'S3Config.SecretKey':
      return [trimNotEmptyValidator];
    case 'S3Config.URL':
      return [trimNotEmptyValidator, (value) => Uri.tryParse(value) != null];
    case 'SiaConfig.Bucket':
      return [trimNotEmptyValidator];
  }

  return [];
}

String initialCaps(String txt) {
  if (txt.isEmpty) {
    return txt;
  }

  if (txt.length == 1) {
    return txt[0].toUpperCase();
  }

  return txt[0].toUpperCase() + txt.substring(1).toLowerCase();
}

bool validateSettings(
  Map<String, dynamic> settings,
  List<String> failed, {
  String? rootKey,
}) {
  settings.forEach((key, value) {
    final settingKey = rootKey == null ? key : '$rootKey.$key';
    if (value is Map<String, dynamic>) {
      validateSettings(value, failed, rootKey: settingKey);
      return;
    }

    for (var validator in getSettingValidators(settingKey)) {
      if (validator(value.toString())) {
        continue;
      }
      failed.add(settingKey);
    }
  });

  return failed.isEmpty;
}

Future<Map<String, dynamic>> convertAllToString(
  Map<String, dynamic> settings,
  SecureKey key,
) async {
  Future<Map<String, dynamic>> convert(Map<String, dynamic> settings) async {
    for (var entry in settings.entries) {
      if (entry.value is Map<String, dynamic>) {
        await convert(entry.value);
        continue;
      }

      if (entry.key == 'ApiPassword' ||
          entry.key == 'EncryptionToken' ||
          entry.key == 'SecretKey') {
        if (entry.value.isEmpty) {
          continue;
        }

        settings[entry.key] = encryptValue(entry.value, key);
        continue;
      }

      if (entry.value is String) {
        continue;
      }

      settings[entry.key] = entry.value.toString();
    }

    return settings;
  }

  return convert(settings);
}

String encryptValue(String value, SecureKey key) {
  if (value.isEmpty) {
    return value;
  }

  final sodium = constants.sodium;
  final crypto = sodium.crypto.aeadXChaCha20Poly1305IETF;

  final nonce = sodium.secureRandom(crypto.nonceBytes).extractBytes();
  final data = crypto.encrypt(
    additionalData: Uint8List.fromList('repertory'.toCharArray()),
    key: key,
    message: Uint8List.fromList(value.toCharArray()),
    nonce: nonce,
  );

  return hex.encode(nonce + data);
}

Map<String, dynamic> getChanged(
  Map<String, dynamic> original,
  Map<String, dynamic> updated,
) {
  if (DeepCollectionEquality().equals(original, updated)) {
    return {};
  }

  Map<String, dynamic> changed = {};
  original.forEach((key, value) {
    if (DeepCollectionEquality().equals(value, updated[key])) {
      return;
    }

    if (value is Map<String, dynamic>) {
      changed[key] = <String, dynamic>{};
      value.forEach((subKey, subValue) {
        if (DeepCollectionEquality().equals(subValue, updated[key][subKey])) {
          return;
        }

        changed[key][subKey] = updated[key][subKey];
      });
      return;
    }

    changed[key] = updated[key];
  });

  return changed;
}

Future<String?> editMountLocation(
  BuildContext context,
  List<String> available, {
  bool allowEmpty = false,
  String? location,
}) async {
  if (!context.mounted) {
    return location;
  }

  String? currentLocation = location;
  final controller = TextEditingController(text: currentLocation);
  return await doShowDialog(
    context,
    StatefulBuilder(
      builder: (context, setState) {
        return AlertDialog(
          actions: [
            TextButton(
              child: const Text('Cancel'),
              onPressed: () => Navigator.of(context).pop(null),
            ),
            TextButton(
              child: const Text('OK'),
              onPressed: () {
                final result = getSettingValidators('Path').firstWhereOrNull(
                  (validator) => !validator(currentLocation ?? ''),
                );
                if (result != null) {
                  return displayErrorMessage(
                    context,
                    "Mount location is not valid",
                  );
                }
                Navigator.of(context).pop(currentLocation);
              },
            ),
          ],
          content: available.isEmpty
              ? TextField(
                  autofocus: true,
                  controller: controller,
                  onChanged: (value) => setState(() => currentLocation = value),
                )
              : AppDropdown<String>(
                  labelOf: (s) => s,
                  labelText: "Select drive",
                  onChanged: (value) => setState(() => currentLocation = value),
                  prefixIcon: Icons.computer,
                  value: currentLocation,
                  values: available.toList(),
                ),
          title: const Text('Mount Location', textAlign: TextAlign.center),
        );
      },
    ),
  );
}

InputDecoration createCommonDecoration(
  ColorScheme colorScheme,
  String label, {
  bool filled = true,
  String? hintText,
  IconData? icon,
}) => InputDecoration(
  labelText: label,
  prefixIcon: icon == null ? null : Icon(icon),
  filled: filled,
  fillColor: colorScheme.primary.withValues(alpha: constants.primaryAlpha),
  hintText: hintText,
  border: OutlineInputBorder(
    borderRadius: BorderRadius.circular(constants.borderRadiusSmall),
    borderSide: BorderSide.none,
  ),
  focusedBorder: OutlineInputBorder(
    borderRadius: BorderRadius.circular(constants.borderRadiusSmall),
    borderSide: BorderSide(color: colorScheme.primary, width: 2),
  ),
  contentPadding: const EdgeInsets.all(constants.paddingSmall),
);

IconData getProviderTypeIcon(String mountType) {
  switch (mountType.toLowerCase()) {
    case "encrypt":
      return Icons.key_outlined;
    case "remote":
      return Icons.network_ping_outlined;
    default:
      return Icons.cloud_outlined;
  }
}

SettingsThemeData createSettingsTheme(ColorScheme scheme) {
  return SettingsThemeData(
    settingsListBackground: Colors.transparent,
    settingsSectionBackground: scheme.primary.withValues(
      alpha: constants.primaryAlpha,
    ),
    titleTextColor: scheme.onSurface.withValues(
      alpha: constants.primarySurfaceAlpha,
    ),
    trailingTextColor: scheme.onSurface.withValues(
      alpha: constants.primarySurfaceAlpha,
    ),
    tileDescriptionTextColor: scheme.onSurface.withValues(
      alpha: constants.secondarySurfaceAlpha,
    ),
    leadingIconsColor: scheme.onSurface.withValues(
      alpha: constants.primarySurfaceAlpha,
    ),
    dividerColor: scheme.outlineVariant.withValues(
      alpha: constants.outlineAlpha,
    ),
    tileHighlightColor: scheme.primary.withValues(
      alpha: constants.highlightAlpha,
    ),
  );
}

Color darken(Color color, [double percentage = 0.1]) {
  final hsl = HSLColor.fromColor(color);
  final hslDark = hsl.withLightness(
    (hsl.lightness - (hsl.lightness * percentage)).clamp(0.0, 1.0),
  );
  return hslDark.toColor();
}
