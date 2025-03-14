import 'package:collection/collection.dart';
import 'package:flutter/foundation.dart';

typedef Validator = bool Function(String);

bool containsRestrictedChar(String value) {
  const invalidChars = [
    '!',
    '"',
    '\$',
    '&',
    '\'',
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
  ];
  return invalidChars.firstWhereOrNull((char) => value.contains(char)) != null;
}

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
      return [(value) => value.isNotEmpty];
    case 'EncryptConfig.EncryptionToken':
      return [(value) => value.isNotEmpty];
    case 'EncryptConfig.Path':
      return [
        (value) => value.trim().isNotEmpty,
        (value) => !containsRestrictedChar(value),
      ];
    case 'HostConfig.ApiPassword':
      return [(value) => value.isNotEmpty];
    case 'HostConfig.ApiPort':
      return [
        (value) {
          int? intValue = int.tryParse(value);
          if (intValue == null) {
            return false;
          }

          return (intValue > 0 && intValue < 65536);
        },
        (value) => Uri.tryParse('http://localhost:$value/') != null,
      ];
    case 'HostConfig.HostNameOrIp':
      return [
        (value) => value.trim().isNotEmpty,
        (value) => Uri.tryParse('http://$value:9000/') != null,
      ];
    case 'HostConfig.Protocol':
      return [(value) => value == "http" || value == "https"];
    case 'S3Config.AccessKey':
      return [(value) => value.trim().isNotEmpty];
    case 'S3Config.Bucket':
      return [(value) => value.trim().isNotEmpty];
    case 'S3Config.SecretKey':
      return [(value) => value.trim().isNotEmpty];
    case 'S3Config.URL':
      return [(value) => Uri.tryParse(value) != null];
    case 'SiaConfig.Bucket':
      return [(value) => value.trim().isNotEmpty];
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
    final checkKey = rootKey == null ? key : '$rootKey.$key';
    if (value is Map) {
      debugPrint('nested: $checkKey');
      validateSettings(
        value as Map<String, dynamic>,
        failed,
        rootKey: checkKey,
      );
    } else {
      debugPrint('validate: $checkKey--$value');
      for (var validator in getSettingValidators(checkKey)) {
        if (!validator(value.toString())) {
          failed.add(checkKey);
        }
      }
    }
  });

  return failed.isEmpty;
}
