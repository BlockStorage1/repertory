import 'package:collection/collection.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:repertory/constants.dart' as constants;

typedef Validator = bool Function(String);

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

createHostNameOrIpValidators() => <Validator>[
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
        'SiaConfig': {'Bucket': 'default'},
      };
  }

  return {};
}

void displayErrorMessage(context, String text) {
  if (!context.mounted) {
    return;
  }

  ScaffoldMessenger.of(
    context,
  ).showSnackBar(SnackBar(content: Text(text, textAlign: TextAlign.center)));
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
      return "HTTP basic authentication password";
    case 'HostConfig.ApiPassword':
      return "RENTERD_API_PASSWORD";
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

Map<String, dynamic> convertAllToString(Map<String, dynamic> settings) {
  settings.forEach((key, value) {
    if (value is Map<String, dynamic>) {
      convertAllToString(value);
      return;
    }

    if (value is String) {
      return;
    }

    settings[key] = value.toString();
  });

  return settings;
}
