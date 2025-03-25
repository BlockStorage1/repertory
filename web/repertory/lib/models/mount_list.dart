import 'dart:convert';
import 'dart:math';

import 'package:collection/collection.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart' show ModalRoute;
import 'package:http/http.dart' as http;
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart';
import 'package:repertory/models/auth.dart';
import 'package:repertory/models/mount.dart';
import 'package:repertory/types/mount_config.dart';

class MountList with ChangeNotifier {
  final Auth _auth;

  MountList(this._auth) {
    _auth.mountList = this;
    _auth.addListener(() {
      if (_auth.authenticated) {
        _fetch();
      }
    });
  }

  List<Mount> _mountList = [];

  Auth get auth => _auth;

  UnmodifiableListView<Mount> get items =>
      UnmodifiableListView<Mount>(_mountList);

  bool hasBucketName(String mountType, String bucket, {String? excludeName}) {
    final list = items.where(
      (item) => item.type.toLowerCase() == mountType.toLowerCase(),
    );

    return (excludeName == null
                ? list
                : list.whereNot(
                  (item) =>
                      item.name.toLowerCase() == excludeName.toLowerCase(),
                ))
            .firstWhereOrNull((Mount item) {
              return item.bucket != null &&
                  item.bucket!.toLowerCase() == bucket.toLowerCase();
            }) !=
        null;
  }

  bool hasConfigName(String name) {
    return items.firstWhereOrNull(
          (item) => item.name.toLowerCase() == name.toLowerCase(),
        ) !=
        null;
  }

  Future<void> _fetch() async {
    try {
      final auth = await _auth.createAuth();
      final response = await http.get(
        Uri.parse('${getBaseUri()}/api/v1/mount_list?auth=$auth'),
      );

      if (response.statusCode == 401) {
        displayAuthError(_auth);
        _auth.logoff();
        return;
      }

      if (response.statusCode == 404) {
        reset();
        return;
      }

      if (response.statusCode != 200) {
        return;
      }

      List<Mount> nextList = [];

      jsonDecode(response.body).forEach((type, value) {
        nextList.addAll(
          value
              .map(
                (name) =>
                    Mount(_auth, MountConfig(type: type, name: name), this),
              )
              .toList(),
        );
      });
      _sort(nextList);
      _mountList = nextList;

      notifyListeners();
    } catch (e) {
      debugPrint('$e');
    }
  }

  void _sort(list) {
    list.sort((a, b) {
      final res = a.type.compareTo(b.type);
      if (res != 0) {
        return res;
      }

      return a.name.compareTo(b.name);
    });
  }

  Future<bool> add(
    String type,
    String name,
    Map<String, dynamic> settings,
  ) async {
    var ret = false;

    var apiPort = settings['ApiPort'] ?? 10000;
    for (var mount in _mountList) {
      var port = mount.mountConfig.settings['ApiPort'] as int?;
      if (port != null) {
        apiPort = max(apiPort, port + 1);
      }
    }
    settings["ApiPort"] = apiPort;

    displayError() {
      if (constants.navigatorKey.currentContext == null) {
        return;
      }

      displayErrorMessage(
        constants.navigatorKey.currentContext!,
        'Add mount failed. Please try again.',
      );
    }

    try {
      final auth = await _auth.createAuth();
      final map = await convertAllToString(
        jsonDecode(jsonEncode(settings)),
        _auth.key,
      );
      final response = await http.post(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/add_mount?auth=$auth&name=$name&type=$type&config=${jsonEncode(map)}',
          ),
        ),
      );

      switch (response.statusCode) {
        case 200:
          ret = true;
          break;
        case 401:
          displayAuthError(_auth);
          _auth.logoff();
          break;
        case 404:
          reset();
          break;
        default:
          displayError();
          break;
      }
    } catch (e) {
      debugPrint('$e');
      displayError();
    }

    if (ret) {
      await _fetch();
    }

    return ret;
  }

  void clear() {
    _mountList = [];
    notifyListeners();
  }

  Future<void> reset() async {
    if (constants.navigatorKey.currentContext == null ||
        ModalRoute.of(constants.navigatorKey.currentContext!)?.settings.name !=
            '/') {
      await constants.navigatorKey.currentState?.pushReplacementNamed('/');
    }

    displayErrorMessage(
      constants.navigatorKey.currentContext!,
      'Mount removed externally. Reloading...',
    );

    clear();

    return _fetch();
  }
}
