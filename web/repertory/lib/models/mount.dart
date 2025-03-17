import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:repertory/helpers.dart';
import 'package:repertory/models/mount_list.dart';
import 'package:repertory/types/mount_config.dart';

class Mount with ChangeNotifier {
  final MountConfig mountConfig;
  final MountList? _mountList;
  bool _isMounting = false;
  Mount(this.mountConfig, this._mountList, {isAdd = false}) {
    if (isAdd) {
      return;
    }
    refresh();
  }

  String? get bucket => mountConfig.bucket;
  String get id => '${type}_$name';
  bool? get mounted => mountConfig.mounted;
  String get name => mountConfig.name;
  String get path => mountConfig.path;
  String get provider => mountConfig.provider;
  String get type => mountConfig.type;

  Future<void> _fetch() async {
    try {
      final response = await http.get(
        Uri.parse(
          Uri.encodeFull('${getBaseUri()}/api/v1/mount?name=$name&type=$type'),
        ),
      );

      if (response.statusCode == 404) {
        _mountList?.reset();
        return;
      }

      if (response.statusCode != 200) {
        return;
      }

      mountConfig.updateSettings(jsonDecode(response.body));

      notifyListeners();
    } catch (e) {
      debugPrint('$e');
    }
  }

  Future<void> _fetchStatus() async {
    try {
      final response = await http.get(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/mount_status?name=$name&type=$type',
          ),
        ),
      );

      if (response.statusCode == 404) {
        _mountList?.reset();
        return;
      }

      if (response.statusCode != 200) {
        return;
      }

      mountConfig.updateStatus(jsonDecode(response.body));
      notifyListeners();
    } catch (e) {
      debugPrint('$e');
    }
  }

  Future<bool> mount(bool unmount, {String? location}) async {
    try {
      _isMounting = true;

      mountConfig.mounted = null;
      notifyListeners();

      final response = await http.post(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/mount?unmount=$unmount&name=$name&type=$type&location=$location',
          ),
        ),
      );

      if (response.statusCode == 404) {
        _isMounting = false;
        _mountList?.reset();
        return true;
      }

      await refresh(force: true);
      _isMounting = false;

      if (!unmount && response.statusCode == 500) {
        return false;
      }
    } catch (e) {
      debugPrint('$e');
    }

    _isMounting = false;
    return true;
  }

  Future<void> refresh({bool force = false}) async {
    if (!force && _isMounting) {
      return;
    }

    await _fetch();
    return _fetchStatus();
  }

  Future<void> setValue(String key, String value) async {
    try {
      final response = await http.put(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/set_value_by_name?name=$name&type=$type&key=$key&value=$value',
          ),
        ),
      );

      if (response.statusCode == 404) {
        _mountList?.reset();
        return;
      }

      if (response.statusCode != 200) {
        return;
      }

      return refresh();
    } catch (e) {
      debugPrint('$e');
    }
  }

  Future<String?> getMountLocation() async {
    try {
      final response = await http.get(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/get_mount_location?name=$name&type=$type',
          ),
        ),
      );

      if (response.statusCode != 200) {
        return null;
      }

      return jsonDecode(response.body)['Location'] as String;
    } catch (e) {
      debugPrint('$e');
    }

    return null;
  }
}
