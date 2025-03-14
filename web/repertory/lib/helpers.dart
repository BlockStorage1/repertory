import 'package:collection/collection.dart';
import 'package:flutter/foundation.dart';
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
      return {'EventLevel': 'info'};
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

List<Validator> getSettingValidators(String settingPath) {
  switch (settingPath) {
    case 'ApiAuth':
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
      return [
        (value) {
          int? intValue = int.tryParse(value);
          if (intValue == null) {
            return false;
          }

          return (intValue > 0 && intValue < 65536);
        },
        createUriValidator(host: 'localhost'),
      ];
    case 'HostConfig.HostNameOrIp':
      return createHostNameOrIpValidators();
    case 'HostConfig.Protocol':
      return [(value) => constants.protocolTypeList.contains(value)];
    case 'RemoteConfig.EncryptionToken':
      return [notEmptyValidator];
    case 'RemoteConfig.HostNameOrIp':
      return createHostNameOrIpValidators();
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
    if (value is Map) {
      validateSettings(
        value as Map<String, dynamic>,
        failed,
        rootKey: settingKey,
      );
    } else {
      for (var validator in getSettingValidators(settingKey)) {
        if (validator(value.toString())) {
          continue;
        }
        failed.add(settingKey);
      }
    }
  });

  return failed.isEmpty;
}
