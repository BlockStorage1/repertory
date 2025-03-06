import 'package:flutter/foundation.dart';

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

String initialCaps(String txt) {
  if (txt.isEmpty) {
    return txt;
  }

  if (txt.length == 1) {
    return txt[0].toUpperCase();
  }

  return txt[0].toUpperCase() + txt.substring(1).toLowerCase();
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
          'ApiPort': '9980',
          'HostNameOrIp': 'localhost',
        },
        'SiaConfig': {'Bucket': 'default'},
      };
  }

  return {};
}
