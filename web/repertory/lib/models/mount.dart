import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:repertory/helpers.dart';
import 'package:repertory/types/mount_config.dart';

class Mount with ChangeNotifier {
  final MountConfig mountConfig;
  Mount(this.mountConfig, {isAdd = false}) {
    if (isAdd) {
      return;
    }
    refresh();
  }

  String get name => mountConfig.name;
  String get path => mountConfig.path;
  IconData? get state => mountConfig.state;
  String get type => mountConfig.type;

  Future<void> _fetch() async {
    try {
      final response = await http.get(
        Uri.parse(
          Uri.encodeFull('${getBaseUri()}/api/v1/mount?name=$name&type=$type'),
        ),
      );

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

      if (response.statusCode != 200) {
        return;
      }

      mountConfig.updateStatus(jsonDecode(response.body));
      notifyListeners();
    } catch (e) {
      debugPrint('$e');
    }
  }

  Future<void> mount(bool unmount, {String? location}) async {
    try {
      await http.post(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/mount?unmount=$unmount&name=$name&type=$type&location=$location',
          ),
        ),
      );

      return refresh();
    } catch (e) {
      debugPrint('$e');
    }
  }

  Future<void> refresh() async {
    await _fetch();
    return _fetchStatus();
  }

  Future<void> setValue(String key, String value) async {
    try {
      await http.put(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/set_value_by_name?name=$name&type=$type&key=$key&value=$value',
          ),
        ),
      );

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
